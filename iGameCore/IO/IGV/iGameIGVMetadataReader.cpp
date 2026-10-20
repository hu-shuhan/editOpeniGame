#include "iGameIGVMetadataReader.h"

#include <hdf5.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "Log/iGameLogger.h"
#include "iGameIGVHdf5Util.h"
#include "iGameIGVStats.h"

IGAME_NAMESPACE_BEGIN

namespace {

using namespace igv;

// -----------------------------------------------------------------------------
// 小工具
// -----------------------------------------------------------------------------

/// 失败收口：清 HDF5 错误栈（§5.4：不透传错误栈到上层日志）+ 写 error。
bool Fail(std::string* error, const std::string& message) {
    ClearHdf5Errors();
    if (error != nullptr) { *error = message; }
    return false;
}

/// H5Literate 回调：只收集 **dataset** 名。
///
/// `H5Oget_info_by_name3` 读的是对象头（对象类型属于链接信息），**不触碰 dataset 数据**，
/// 满足 §7.2 对快路径的约束。组自身的 attribute（如 `Scalars` 活跃标记）不是链接，
/// 因此不会混进这里 —— 正是我们要的。
herr_t CollectDatasetNames(hid_t group, const char* name, const H5L_info_t* /*info*/, void* opData) {
    auto* out = static_cast<std::vector<std::string>*>(opData);
    if (out == nullptr || name == nullptr) { return 0; }
    H5O_info2_t info{};
    if (H5Oget_info_by_name3(group, name, &info, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
        ClearHdf5Errors();
        return 0;  // 取不到信息的孩子跳过，不中断遍历
    }
    if (info.type == H5O_TYPE_DATASET) { out->emplace_back(name); }
    return 0;
}

bool ListDatasets(hid_t group, std::vector<std::string>& out) {
    out.clear();
    if (H5Literate(group, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, CollectDatasetNames, &out) < 0) {
        ClearHdf5Errors();
        return false;
    }
    return true;
}

/// H5Literate 回调：只收集 **group** 名（与 CollectDatasetNames 对称）。
///
/// 用途只有一个：P6 的"未实现容器"探测（`Block_*` / `Steps`，见 §12.1.3 / §12.2）。
/// 同样只读对象头，**不触碰任何 dataset 数据**，因此 `h5dreadCount` 不变。
herr_t CollectGroupNames(hid_t group, const char* name, const H5L_info_t* /*info*/, void* opData) {
    auto* out = static_cast<std::vector<std::string>*>(opData);
    if (out == nullptr || name == nullptr) { return 0; }
    H5O_info2_t info{};
    if (H5Oget_info_by_name3(group, name, &info, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
        ClearHdf5Errors();
        return 0;
    }
    if (info.type == H5O_TYPE_GROUP) { out->emplace_back(name); }
    return 0;
}

bool ListGroups(hid_t group, std::vector<std::string>& out) {
    out.clear();
    if (H5Literate(group, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr, CollectGroupNames, &out) < 0) {
        ClearHdf5Errors();
        return false;
    }
    return true;
}

/// P6：根组下若存在"本期明确不实现"的数据容器（多块 / 单文件内嵌时变）→ 拒绝。
///
/// **快路径也要拒绝，理由不是"读不了"，而是"别让调用方以为这个文件能打开"**：
/// 文件浏览器用本函数填列表，若它给出一份看起来正常的包围盒与属性清单，
/// 用户点开时才会撞上 `IGVReader` 的拒绝 —— 那是最差的时机。
/// 与其在 UI 上制造"能列出但打不开"的文件，不如在列清单这一步就说清原因。
/// 见 §12.1.3 / §12.2 与 P6-1 / P6-3b。
bool RejectUnsupportedContainer(hid_t root, std::string* error) {
    std::vector<std::string> groups;
    if (!ListGroups(root, groups)) {
        return Fail(error, "cannot enumerate the subgroups of /IGV");
    }
    for (const std::string& g : groups) {
        switch (ClassifyUnsupportedContainer(g)) {
            case UnsupportedContainer::MultiBlock:
                return Fail(error, "/IGV contains the block subgroup '" + g +
                                       "': MultiBlock is not supported in this release"
                                       " (design doc section 12.2 / P6)");
            case UnsupportedContainer::TimeSeries:
                return Fail(error, "/IGV contains the time-series group '" + g +
                                       "': a single-file time series is not supported in this release"
                                       " (design doc section 12.1 / P6)");
            case UnsupportedContainer::None:
            default:
                break;
        }
    }
    return true;
}

/// attribute 是否存在（用 H5Aexists，不打开）。
bool AttributeExists(hid_t loc, std::string_view name) {
    return H5Aexists(loc, std::string(name).c_str()) > 0;
}

/// 必填的定长 ASCII attribute（区分"缺失"与"读失败"以给出可定位的报错）。
bool RequireAscii(hid_t loc, std::string_view name, std::string& out,
                  const std::string& owner, std::string* error) {
    if (!AttributeExists(loc, name)) {
        return Fail(error, owner + " is missing the required attribute '" + std::string(name) + "'");
    }
    if (!ReadAsciiAttribute(loc, name, out)) {
        return Fail(error, owner + " attribute '" + std::string(name) + "' is not a readable fixed-length ASCII attribute");
    }
    return true;
}

/// 可选的 float64 数组 attribute：存在则要求长度精确等于 @p expected。
bool OptionalF64Array(hid_t loc, std::string_view name, size_t expected,
                      std::vector<double>& out, bool& present,
                      const std::string& owner, std::string* error) {
    present = false;
    if (!AttributeExists(loc, name)) { return true; }

    if (!ReadF64ArrayAttribute(loc, name, 0, out)) {
        return Fail(error, owner + " attribute '" + std::string(name) + "' is not a readable float64 attribute");
    }
    if (expected != 0 && out.size() != expected) {
        return Fail(error, owner + " attribute '" + std::string(name) + "' has " +
                             std::to_string(out.size()) + " elements, expected " + std::to_string(expected));
    }
    present = true;
    return true;
}

/// 可选的 int64 数组 attribute（长度精确校验）。
bool OptionalI64Array(hid_t loc, std::string_view name, size_t expected,
                      std::vector<int64_t>& out, bool& present,
                      const std::string& owner, std::string* error) {
    present = false;
    if (!AttributeExists(loc, name)) { return true; }

    if (!ReadI64ArrayAttribute(loc, name, 0, out)) {
        return Fail(error, owner + " attribute '" + std::string(name) + "' is not a readable int64 attribute");
    }
    if (expected != 0 && out.size() != expected) {
        return Fail(error, owner + " attribute '" + std::string(name) + "' has " +
                             std::to_string(out.size()) + " elements, expected " + std::to_string(expected));
    }
    present = true;
    return true;
}

/// 读 dataset 的 dataspace 维度（**只读对象头，不解压**）。
bool DatasetShape(hid_t group, std::string_view name, std::vector<hsize_t>& dims, DType& dtype,
                  std::string* error, const std::string& context) {
    IGVStats::datasetOpenCount.fetch_add(1, std::memory_order_relaxed);

    H5DatasetHandle ds(H5Dopen2(group, std::string(name).c_str(), H5P_DEFAULT));
    if (!ds.IsValid()) { return Fail(error, context + ": cannot open dataset '" + std::string(name) + "'"); }

    H5DataspaceHandle space(H5Dget_space(ds.Get()));
    if (!space.IsValid()) { return Fail(error, context + ": cannot read dataspace of '" + std::string(name) + "'"); }

    const int rank = H5Sget_simple_extent_ndims(space.Get());
    if (rank < 0) { return Fail(error, context + ": invalid dataspace rank of '" + std::string(name) + "'"); }

    dims.assign(static_cast<size_t>(rank), 0);
    std::vector<hsize_t> maxDims(static_cast<size_t>(rank), 0);
    if (rank > 0 && H5Sget_simple_extent_dims(space.Get(), dims.data(), maxDims.data()) < 0) {
        return Fail(error, context + ": cannot read dataspace dimensions of '" + std::string(name) + "'");
    }

    H5TypeHandle type(H5Dget_type(ds.Get()));
    if (!type.IsValid()) { return Fail(error, context + ": cannot read datatype of '" + std::string(name) + "'"); }
    if (!DTypeFromFileType(type.Get(), dtype)) {
        return Fail(error, context + ": dataset '" + std::string(name) + "' has a datatype outside IGV support");
    }
    return true;
}

/// DType → 每元素字节数（§7.1 的 `valueSize`）。宽度只由 DType 决定（§3.5）。
int ValueSizeOf(DType t) { return static_cast<int>(SizeOfDType(t)); }

// -----------------------------------------------------------------------------
// 各段解析
// -----------------------------------------------------------------------------

/// 根组 attribute + Version + 计数（§3.1 / §3.4 / §3.10）。
bool ParseRoot(const H5GroupHandle& root, IGVMetadata& out, std::string* error) {
    // 第 0 步：未实现的数据容器（多块 / 单文件内嵌时变）直接拒绝（P6）。
    // 放在最前：这两个容器一旦存在，根组里那份"父块"的 metadata 就**不代表文件的内容**，
    // 后面读出什么都会误导调用方。
    if (!RejectUnsupportedContainer(root.Get(), error)) { return false; }

    if (!RequireAscii(root.Get(), kAttrType, out.type, "/IGV", error)) { return false; }
    if (!RequireAscii(root.Get(), kAttrIGVDataType, out.igvDataType, "/IGV", error)) { return false; }

    // Version 是**根组下的 dataset**（i64[2]），不是 attribute（见 kDatasetVersion 的说明）。
    // 这是快路径里唯一允许的一次 H5Dread：写侧 compress=false，读它不会解压任何 chunk。
    if (!DatasetExists(root.Get(), kDatasetVersion)) {
        return Fail(error, "/IGV is missing the required dataset 'Version' (not an IGV file?)");
    }
    std::vector<int64_t> version;
    if (!ReadDataset(root.Get(), kDatasetVersion, DType::I64, 2, version)) {
        return Fail(error, "/IGV dataset 'Version' is not readable as int64[2]");
    }
    out.versionMajor = version[0];
    out.versionMinor = version[1];

    // CellTypeEncoding：含 Types 时**必须**存在且为 "IGCellType"（§3.5.1，读侧硬规则）。
    if (DatasetExists(root.Get(), kDatasetTypes)) {
        std::string encoding;
        if (!RequireAscii(root.Get(), kAttrCellTypeEncoding, encoding, "/IGV", error)) { return false; }
        if (encoding != kCellTypeEncodingIGCellType) {
            return Fail(error, "/IGV CellTypeEncoding is '" + encoding + "', expected '" +
                               std::string(kCellTypeEncodingIGCellType) + "'");
        }
    }

    // Bounds：可选。**没有时不回落读 Points**（§7.2 末条）——由调用方决定是否降级。
    std::vector<double> bounds;
    if (!OptionalF64Array(root.Get(), kAttrBounds, 6, bounds, out.hasBounds, "/IGV", error)) { return false; }
    if (out.hasBounds) {
        for (int i = 0; i < 6; ++i) { out.bounds[i] = bounds[static_cast<size_t>(i)]; }
    }

    // Dimensions：可选，仅结构化网格。
    std::vector<int64_t> dims;
    if (!OptionalI64Array(root.Get(), kAttrDimensions, 3, dims, out.hasDimensions, "/IGV", error)) { return false; }
    if (out.hasDimensions) {
        for (int i = 0; i < 3; ++i) { out.dimensions[i] = dims[static_cast<size_t>(i)]; }
    }

    // 三个可选标量 attribute。
    if (ReadI64Attribute(root.Get(), kAttrFixedCellSize, out.fixedCellSize)) {
        // 默认值 -1 表示"没有该 attribute"，读到才覆盖。
    }
    if (ReadF64Attribute(root.Get(), kAttrTimeValue, out.timeValue)) { out.hasTimeValue = true; }
    if (ReadI64Attribute(root.Get(), kAttrTimeStep, out.timeStep)) { out.hasTimeStep = true; }
    ReadAsciiAttribute(root.Get(), kAttrGenerator, out.generator);  // 可选，缺失即空串

    // ---- 计数：一律来自 dataspace，不读计数组的**值**（那需要 H5Dread） ----
    if (!DatasetExists(root.Get(), kDatasetPoints)) {
        return Fail(error, "/IGV is missing the required dataset 'Points'");
    }
    std::vector<hsize_t> pointsDims;
    DType pointsType = DType::Unknown;
    if (!DatasetShape(root.Get(), kDatasetPoints, pointsDims, pointsType, error, "/IGV")) { return false; }
    if (pointsDims.size() != 2 || pointsDims[1] != 3) {
        return Fail(error, "/IGV 'Points' must be (n,3), got rank " + std::to_string(pointsDims.size()) +
                           (pointsDims.size() == 2 ? " with second dim " + std::to_string(pointsDims[1]) : std::string()));
    }
    out.pointCount = static_cast<int64_t>(pointsDims[0]);

    // 单元数：Types 优先（非结构族），否则由 Offsets 的 (ncells+1) 反推。
    if (DatasetExists(root.Get(), kDatasetTypes)) {
        std::vector<hsize_t> typesDims;
        DType typesType = DType::Unknown;
        if (!DatasetShape(root.Get(), kDatasetTypes, typesDims, typesType, error, "/IGV")) { return false; }
        if (typesDims.size() != 1) {
            return Fail(error, "/IGV 'Types' must be rank-1");
        }
        out.cellCount = static_cast<int64_t>(typesDims[0]);
        out.hasCellCount = true;
    }
    if (DatasetExists(root.Get(), kDatasetOffsets)) {
        std::vector<hsize_t> offsetsDims;
        DType offsetsType = DType::Unknown;
        if (!DatasetShape(root.Get(), kDatasetOffsets, offsetsDims, offsetsType, error, "/IGV")) { return false; }
        if (offsetsDims.size() != 1 || offsetsDims[0] == 0) {
            return Fail(error, "/IGV 'Offsets' must be rank-1 with at least one element");
        }
        const int64_t cellsFromOffsets = static_cast<int64_t>(offsetsDims[0]) - 1;
        if (out.hasCellCount && cellsFromOffsets != out.cellCount) {
            return Fail(error, "/IGV cross-check failed: Types implies " + std::to_string(out.cellCount) +
                               " cells but Offsets implies " + std::to_string(cellsFromOffsets));
        }
        out.cellCount = cellsFromOffsets;
        out.hasCellCount = true;
    }
    // ---- TopologyIndex（可选 attribute；缺省 = Primary）+ 多面体面数 ----
    //
    // 面数从 `FaceOffsets` 的 **dataspace** 推出（nfaces = len-1），不读它的值 ——
    // 与三个计数组的处理一致，保持快路径零解压（§11.3）。
    {
        std::string topologyIndex;
        if (ReadAsciiAttribute(root.Get(), kAttrTopologyIndex, topologyIndex)) {
            if (topologyIndex != kTopologyIndexPrimary &&
                topologyIndex != kTopologyIndexSecondary) {
                return Fail(error, "/IGV TopologyIndex='" + topologyIndex +
                                   "' is unknown (allowed: '" +
                                   std::string(kTopologyIndexPrimary) + "' / '" +
                                   std::string(kTopologyIndexSecondary) + "')");
            }
            out.topologyIndex = topologyIndex;
        }

        if (out.topologyIndex == kTopologyIndexSecondary) {
            for (std::string_view required : kPolyhedronTopologyDatasets) {
                if (!DatasetExists(root.Get(), required)) {
                    return Fail(error, "/IGV declares TopologyIndex=Secondary but the required "
                                       "dataset '" + std::string(required) + "' is missing");
                }
            }
            if (!DatasetExists(root.Get(), kDatasetTypes)) {
                return Fail(error, "/IGV declares TopologyIndex=Secondary but has no Types dataset");
            }
            std::vector<hsize_t> faceDims;
            DType faceType = DType::Unknown;
            if (!DatasetShape(root.Get(), kDatasetFaceOffsets, faceDims, faceType, error, "/IGV")) {
                return false;
            }
            if (faceDims.size() != 1 || faceDims[0] < 2) {
                return Fail(error, "/IGV 'FaceOffsets' must be rank-1 with at least two elements");
            }
            out.faceCount = static_cast<int64_t>(faceDims[0]) - 1;
            out.hasFaceCount = true;
        }
    }

    if (DatasetExists(root.Get(), kDatasetConnectivity)) {
        std::vector<hsize_t> connDims;
        DType connType = DType::Unknown;
        if (!DatasetShape(root.Get(), kDatasetConnectivity, connDims, connType, error, "/IGV")) { return false; }
        if (connDims.size() != 1) {
            return Fail(error, "/IGV 'Connectivity' must be rank-1");
        }
        out.connectivityIdCount = static_cast<int64_t>(connDims[0]);
        out.hasConnectivityIdCount = true;
    }

    return true;
}

/// 一个属性组（PointData / CellData / FieldData）下的全部属性 dataset。
bool ParseAttributeGroup(const H5GroupHandle& root, std::string_view groupName,
                        IGVMetadata& out, std::string* error) {
    if (!DatasetExists(root.Get(), groupName)) { return true; }  // 组是可选的

    H5GroupHandle group(H5Gopen2(root.Get(), std::string(groupName).c_str(), H5P_DEFAULT));
    if (!group.IsValid()) {
        return Fail(error, "/IGV/" + std::string(groupName) + " is not an openable group");
    }

    std::vector<std::string> names;
    if (!ListDatasets(group.Get(), names)) {
        return Fail(error, "cannot enumerate /IGV/" + std::string(groupName));
    }

    for (const std::string& name : names) {
        const std::string where = "/IGV/" + std::string(groupName) + "/" + name;

        // 一次 H5Dopen 取全部信息：attribute（§3.6）+ dataspace/dtype（§7.1）。
        // datasetOpenCount 只在这里 + DatasetShape 里递增，语义是"打开了几个 dataset 的元数据"。
        IGVStats::datasetOpenCount.fetch_add(1, std::memory_order_relaxed);
        H5DatasetHandle ds(H5Dopen2(group.Get(), name.c_str(), H5P_DEFAULT));
        if (!ds.IsValid()) { return Fail(error, where + ": cannot open dataset"); }

        IGVArrayInfo info;
        info.name = name;
        if (!RequireAscii(ds.Get(), kAttrAttributeType, info.attributeType, where, error)) { return false; }
        if (!RequireAscii(ds.Get(), kAttrAttachment, info.attachment, where, error)) { return false; }

        std::vector<double> range;
        bool hasRange = false;
        if (!OptionalF64Array(ds.Get(), kAttrRange, 0, range, hasRange, where, error)) { return false; }
        if (!hasRange) { return Fail(error, where + " is missing the required attribute 'range' (§3.6)"); }
        info.range = std::move(range);

        // ---- shape / dtype（不读数据） ----
        H5DataspaceHandle space(H5Dget_space(ds.Get()));
        if (!space.IsValid()) { return Fail(error, where + ": cannot read dataspace"); }
        const int rank = H5Sget_simple_extent_ndims(space.Get());
        if (rank < 0) { return Fail(error, where + ": invalid dataspace rank"); }

        std::vector<hsize_t> dims(static_cast<size_t>(rank), 0);
        std::vector<hsize_t> maxDims(static_cast<size_t>(rank), 0);
        if (rank > 0 && H5Sget_simple_extent_dims(space.Get(), dims.data(), maxDims.data()) < 0) {
            return Fail(error, where + ": cannot read dataspace dimensions");
        }

        if (rank == 1) {
            info.count = static_cast<int64_t>(dims[0]);
            info.components = 1;
        } else if (rank == 2) {
            info.count = static_cast<int64_t>(dims[0]);
            info.components = static_cast<int>(dims[1]);
        } else {
            return Fail(error, where + ": attribute datasets must be rank-1 or rank-2, got rank " + std::to_string(rank));
        }
        if (info.components <= 0) { return Fail(error, where + ": component count must be positive"); }

        H5TypeHandle type(H5Dget_type(ds.Get()));
        if (!type.IsValid()) { return Fail(error, where + ": cannot read datatype"); }
        DType dtype = DType::Unknown;
        if (!DTypeFromFileType(type.Get(), dtype)) {
            return Fail(error, where + ": datatype is not one of the 10 fixed-width IGV types "
                                       "(Int8/16/32/64, UInt8/16/32/64, Float32/64)");
        }
        info.valueSize = ValueSizeOf(dtype);
        if (info.valueSize == 0) { return Fail(error, where + ": unsupported value size"); }
        info.dTypeToken = std::string(DTypeToken(dtype));
        info.dTypeBits = DTypeBits(dtype);

        // ---- 与显式声明交叉校验（§3.6）：HDF5 类型 vs DataType vs DataTypeBits ----
        // 快路径也只读 attribute / 对象头，**不**碰 dataset 数据，因此这里加校验
        // 不会破坏 P3 的"零解压"不变量。
        {
            std::string declaredToken;
            if (ReadAsciiAttribute(ds.Get(), kAttrDataType, declaredToken)) {
                const DType declared = ParseDTypeToken(declaredToken);
                if (declared == DType::Unknown) {
                    return Fail(error, where + ": DataType='" + declaredToken +
                                       "' is not one of the 10 fixed-width types");
                }
                if (declared != dtype) {
                    return Fail(error, where + ": DataType='" + declaredToken +
                                       "' disagrees with the HDF5 datatype (" +
                                       std::string(DTypeToken(dtype)) + ")");
                }
            }
            int64_t declaredBits = 0;
            if (ReadI64Attribute(ds.Get(), kAttrDataTypeBits, declaredBits) &&
                declaredBits != static_cast<int64_t>(DTypeBits(dtype))) {
                return Fail(error, where + ": DataTypeBits=" + std::to_string(declaredBits) +
                                   " disagrees with the HDF5 datatype (" +
                                   std::to_string(DTypeBits(dtype)) + " bit)");
            }
        }

        // ---- 交叉校验：range 长度编码了 components（§3.6） ----
        const size_t expectedRange = (info.components == 1)
                                         ? 2u
                                         : 2u * (1u + static_cast<size_t>(info.components));
        if (info.range.size() != expectedRange) {
            return Fail(error, where + ": 'range' has " + std::to_string(info.range.size()) +
                               " elements but components=" + std::to_string(info.components) +
                               " requires " + std::to_string(expectedRange));
        }

        // ---- 交叉校验：point / cell 属性的元素数必须与计数一致（§6.3） ----
        if (info.attachment == kAttachmentPoint && out.pointCount > 0 && info.count != out.pointCount) {
            return Fail(error, where + ": point attribute has " + std::to_string(info.count) +
                               " elements but Points has " + std::to_string(out.pointCount) + " points");
        }
        if (info.attachment == kAttachmentCell && out.hasCellCount && info.count != out.cellCount) {
            return Fail(error, where + ": cell attribute has " + std::to_string(info.count) +
                               " elements but the file has " + std::to_string(out.cellCount) + " cells");
        }

        out.arrays.emplace_back(std::move(info));
    }
    return true;
}

/// 打开 HDF5 之后的所有解析（文件路径与内存镜像共用同一套流程）。
bool ParseHdf5(hid_t fileId, IGVMetadata& out, std::string* error) {
    out = IGVMetadata{};  // 失败时不残留半成品

    H5GroupHandle root(H5Gopen2(fileId, std::string(kRootGroup).c_str(), H5P_DEFAULT));
    if (!root.IsValid()) {
        return Fail(error, "not an IGV file: missing the root group '/" + std::string(kRootGroup) + "'");
    }

    if (!ParseRoot(root, out, error)) { return false; }
    if (!ParseAttributeGroup(root, kGroupPointData, out, error)) { return false; }
    if (!ParseAttributeGroup(root, kGroupCellData, out, error)) { return false; }
    if (!ParseAttributeGroup(root, kGroupFieldData, out, error)) { return false; }
    return true;
}

} // namespace

// =============================================================================
// 对外入口
// =============================================================================

bool IGVMetadataReader::ReadFromFile(const std::string& path, IGVMetadata& out, std::string* error) {
    if (path.empty()) { return Fail(error, "empty file path"); }

    H5FileHandle file(H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));
    if (!file.IsValid()) {
        return Fail(error, "cannot open '" + path + "' as HDF5 (not an IGV file, or unreadable)");
    }
    IGAME_CORE_DEBUG("[IGV] metadata fast path: {}", path);
    return ParseHdf5(file.Get(), out, error);
}

bool IGVMetadataReader::ReadFromMemory(const void* data, size_t size, IGVMetadata& out,
                                       std::string* error) {
    if (data == nullptr || size == 0) { return Fail(error, "empty memory image"); }

    // core VFD + file image：与 IGVReader 的内存路径同构，但**不注册 udata 回调**——
    // 我们不向 HDF5 传自定义 udata，因此不需要 `H5Pset_file_image_callbacks`
    // （它的硬校验只要求"给了 udata 就必须给回调"）。调用方缓冲只需活过本次调用，
    // 与 `FileReader::SetMemoryBuffer` 的"非拥有"语义一致。
    H5PropertyHandle fapl(H5Pcreate(H5P_FILE_ACCESS));
    if (!fapl.IsValid()) { return Fail(error, "H5Pcreate(FILE_ACCESS) failed"); }

    if (H5Pset_fapl_core(fapl.Get(), static_cast<size_t>(1024 * 1024), false) < 0) {
        return Fail(error, "H5Pset_fapl_core failed");
    }
    // H5Pset_file_image 的参数是 `void*`（非 const）；HDF5 会复制镜像，
    // 且我们绝不写它 —— const_cast 在这里是有意为之并已注释。
    void* image = const_cast<void*>(data);
    if (H5Pset_file_image(fapl.Get(), image, size) < 0) {
        return Fail(error, "H5Pset_file_image failed");
    }

    H5FileHandle file(H5Fopen("igv_metadata_image.igv", H5F_ACC_RDONLY, fapl.Get()));
    if (!file.IsValid()) { return Fail(error, "cannot open the memory image as HDF5 (not an IGV file)"); }

    IGAME_CORE_DEBUG("[IGV] metadata fast path: memory image ({} bytes)", size);
    return ParseHdf5(file.Get(), out, error);
}

IGAME_NAMESPACE_END
