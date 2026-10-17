#include "iGameIGVWriter.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <unordered_map>

#include "Log/iGameLogger.h"
#include "MeshCodec/EncodeAdapter/iGameMeshEncodeAdapterFromDataObject.h"

#include "iGameIGVCellType.h"
#include "iGameIGVHdf5Util.h"

IGAME_NAMESPACE_BEGIN

namespace {

using namespace iGame::igv;

// =============================================================================
// iGame 枚举 <-> 规范 token（§3.2 / §3.6 / §3.8）
// =============================================================================

/// iGame `DataObjectType` → 根组 `Type`（VTKHDF 同名值）与 `IGVDataType`（iGame 原名）。
/// 表外的类型一律不列（§1.5 原则 1），由返回值 false 表达"unsupported"。
bool MapMeshType(IGenum meshType, std::string_view& outType, std::string_view& outIGVDataType) {
    switch (meshType) {
        case IG_POINT_SET:
            outType = kTypePolyData;
            outIGVDataType = kIGVDataTypePointSet;
            return true;
        case IG_SURFACE_MESH:
            outType = kTypePolyData;
            outIGVDataType = kIGVDataTypeSurfaceMesh;
            return true;
        case IG_VOLUME_MESH:
            outType = kTypeUnstructuredGrid;
            outIGVDataType = kIGVDataTypeVolumeMesh;
            return true;
        case IG_UNSTRUCTURED_MESH:
            outType = kTypeUnstructuredGrid;
            outIGVDataType = kIGVDataTypeUnstructuredMesh;
            return true;
        case IG_STRUCTURED_MESH:
            outType = kTypeStructuredGrid;
            outIGVDataType = kIGVDataTypeStructuredMesh;
            return true;
        default:
            return false;
    }
}

/// `AttributeType` → token（§3.6）。
std::string_view MapAttributeType(IGenum t) {
    switch (t) {
        case IG_SCALAR:  return kAttributeTypeScalars;
        case IG_VECTOR:  return kAttributeTypeVectors;
        case IG_NORMAL:  return kAttributeTypeNormals;
        case IG_TCOORD:  return kAttributeTypeTCoords;
        case IG_TENSOR:  return kAttributeTypeTensors;
        case IG_RGB:     return kAttributeTypeRGB;
        default:         return {};
    }
}

/// `AttributeWithElementType` → `attachment` token（§3.6 / §3.8）。
std::string_view MapAttachment(IGenum a) {
    switch (a) {
        case IG_POINT:      return kAttachmentPoint;
        case IG_CELL:       return kAttachmentCell;
        case IG_EDGE:       return kAttachmentEdge;
        case IG_POINT_BOX:  return kAttachmentPointBox;
        case IG_CELL_BOX:   return kAttachmentCellBox;
        case IG_CHANGE:     return kAttachmentChange;
        case IG_DRAGPOINT:  return kAttachmentDragPoint;
        case IG_MID_POINT:  return kAttachmentMidPoint;
        default:            return {};
    }
}

/// 依附类型 → 分组名（§3.8）。
std::string_view GroupOfAttachment(IGenum a) {
    if (a == IG_POINT) { return kGroupPointData; }
    if (a == IG_CELL) { return kGroupCellData; }
    return kGroupFieldData;
}

/// iGame 数组类 → 规范 dtype（§3.5 / §3.6 的 dtype 映射表）。
bool MapArrayDType(IGenum arrayType, DType& out) {
    switch (arrayType) {
        case IG_FloatArray:          out = DType::F32; return true;
        case IG_DoubleArray:         out = DType::F64; return true;
        case IG_UnsignedCharArray:   out = DType::U8;  return true;
        default:                     return false;
    }
}

/// 属性名是否是合法 HDF5 名（§5.1 步骤 1.8）。
/// HDF5 名是**路径**，含 '/' 会破坏分组结构；空名与以 '.' 开头的名会被 HDF5 保留语义吞掉。
bool IsValidHdf5Name(std::string_view name) {
    if (name.empty()) { return false; }
    if (name.front() == '.') { return false; }
    if (name.find('/') != std::string_view::npos) { return false; }
    if (name.find('\0') != std::string_view::npos) { return false; }
    return true;
}

/// 单元点数 → IGCellType（仅用于"无 Types 数组"的网格；§3.7）。
/// 只覆盖无歧义的线性单元。返回 false 表示无法由点数唯一确定类型。
bool CellTypeFromSize(IGsize size, IGCellType& out) {
    switch (size) {
        case 1: out = IG_VERTEX;     return true;
        case 2: out = IG_LINE;       return true;
        case 3: out = IG_TRIANGLE;   return true;
        case 4: out = IG_QUAD;       return true;
        case 8: out = IG_HEXAHEDRON; return true;
        // 5 / 6 / 7 有歧义（金字塔/棱柱/楔形/多边形），必须由 Types 决定
        default: return false;
    }
}

/// IGCellType → 固定点数。返回 0 表示该类型点数可变（多边形类）或未知。
///
/// **刻意不调用 `iGameCellType.h` 的自由函数 `GetCellSize(int)`** ——
/// 那个函数对 `IG_HEXAHEDRON` 返回 6（正确值 8），是 P0 记录在案的死代码笔误
/// （见 kia/P0实施记录.md §2.4）。虽然它当前全仓库零调用，但 IGV 的
/// Types↔Offsets 一致性自检若误用它，会**拒绝一切六面体网格**。
int FixedCellSizeOfType(int type) {
    switch (static_cast<IGCellType>(type)) {
        case IG_VERTEX:             return 1;
        case IG_LINE:               return 2;
        case IG_TRIANGLE:           return 3;
        case IG_QUAD:               return 4;
        case IG_TETRA:              return 4;
        case IG_HEXAHEDRON:         return 8;
        case IG_PYRAMID:            return 5;
        case IG_PRISM:              return 6;
        default:                    return 0;  // 多边形 / 高阶 / 未知：不做一致性断言
    }
}

/**
 * 从 `CellArray` 取出 IGV `Offsets`，并返回**真实的** Connectivity 元素数。
 *
 * ⚠ 为什么不能直接用适配器的 `GetCellIdOffset()` 与 `GetCellIdBufferSize()`
 *   —— 这是本阶段第二个真实 bug 的根因，属设计文档 §15 R12 警告的同一类问题：
 *
 *   `CellArray::AddCellIds()` 对 `m_Buffer` 采取**按需翻倍扩容**策略
 *   （`iGameCellArray.cpp:103-127`）：
 *       const IGsize newSize = std::max(endOffset, (beginOffset * 2));
 *       m_Buffer->Resize(newSize);
 *   因此 `m_Buffer->GetNumberOfIds()` 是**含 padding 的容量**，不是真实已用数。
 *   真实已用数只有 CellArray 自己知道：
 *       size_t CellArray::GetNumberOfCellIds() { return GetEndOffset(m_NumberOfCells - 1); }
 *
 *   实测（CountCellVertices_mixed_cells.vtk）：真实 Connectivity = 39，
 *   而 `GetCellIdArray()->GetNumberOfIds()` = 54（padding）。
 *   若拿 54 去写 `NumberOfConnectivityIds`，就会把 padding 一并写进文件，
 *   读侧交叉校验必然失败 —— 而这种错误**看起来是自洽的**，极难定位。
 *   （P0 记录里那条"写 Offsets 必须用 GetCellIdOffset 而非 bufferSize"是同一个坑，
 *     我在这里把 bufferSize 当成了真实长度。）
 *
 *   `CellArray` 的 offset 语义（`m_UseOffsets`）已经把两种情况统一好了：
 *       GetStartOffset(i) = m_UseOffsets ? m_Offsets[i]                 : m_FixedCellSize * i
 *       GetCellSize(i)    = m_UseOffsets ? m_Offsets[i+1]-m_Offsets[i]  : m_FixedCellSize
 *   所以 IGV 只应**基于这些权威接口推导**，而不是去猜 `m_Offsets` 的长度。
 *
 * @param cells           源 CellArray（非空）
 * @param cellCount       单元数
 * @param fixedCellSize   输出：定长网格的单元点数；变长时置 -1
 * @param outOffsets      输出：长度 ncells+1 的 IGV Offsets（原始顺序，不重排）
 * @return 真实的 Connectivity 元素数；无法确定时返回 -1
 */
int64_t BuildIgvOffsetsFromCellArray(CellArray* cells, int64_t cellCount,
                                     int64_t& fixedCellSize,
                                     std::vector<int64_t>& outOffsets) {
    fixedCellSize = -1;
    outOffsets.clear();

    if (cells == nullptr || cellCount <= 0) { return -1; }

    if (cells->IsUseOffSet()) {
        // 有真实 offset 数组：直接取权威值（§3.3：不做任何空间重排）
        outOffsets.resize(static_cast<size_t>(cellCount) + 1);
        for (int64_t i = 0; i <= cellCount; ++i) {
            outOffsets[static_cast<size_t>(i)] =
                static_cast<int64_t>(cells->GetStartOffset(static_cast<IGsize>(i)));
        }
        // GetStartOffset(ncells) == GetEndOffset(ncells-1) == 真实已用数
        return outOffsets.back();
    }

    // 定长网格：CellArray 用 m_FixedCellSize 表达，offset 需自行展开。
    // 定长下 GetCellSize(0) 就返回 m_FixedCellSize。
    const int64_t size = static_cast<int64_t>(cells->GetCellSize(0));
    if (size <= 0) { return -1; }

    fixedCellSize = size;
    outOffsets.resize(static_cast<size_t>(cellCount) + 1);
    for (int64_t i = 0; i <= cellCount; ++i) {
        outOffsets[static_cast<size_t>(i)] = i * size;
    }
    return cellCount * size;
}

// =============================================================================
// 暂存结构：先在内存里把一切算完并校验完，再写 HDF5
// =============================================================================
//
// 这样做的理由：§5.4 要求**每个拒绝路径都发生在写盘之前**。若边校验边写，
// 中途失败会留下半截文件，而且"rejected"与"HDF5 调用失败"两类错误会混在一起。

struct StagedAttribute {
    std::string name;
    std::string attachmentToken;   ///< "point" / "cell" / ...
    std::string attributeTypeToken;///< "Scalars" / "Vectors" / ...
    IGenum igAttributeType{IG_NONE};
    IGenum igAttachmentType{IG_NONE};
    std::string groupName;         ///< PointData / CellData / FieldData
    int64_t count{0};              ///< 元素数
    int components{1};
    DType dtype{DType::F32};
    std::vector<float> f32;
    std::vector<double> f64;
    std::vector<unsigned char> u8;
    std::vector<double> range;

    [[nodiscard]] const void* Data() const {
        switch (dtype) {
            case DType::F32: return f32.data();
            case DType::F64: return f64.data();
            case DType::U8:  return u8.data();
            default:         return nullptr;
        }
    }
    [[nodiscard]] size_t ValueCount() const {
        switch (dtype) {
            case DType::F32: return f32.size();
            case DType::F64: return f64.size();
            case DType::U8:  return u8.size();
            default:         return 0;
        }
    }
};

/// 计算逐属性 range（§8.2）。
///
/// components == 1：`[min, max]`（长度 2，**不重复存模长** —— 消除草案歧义）
/// components > 1 ：`[magMin, magMax, c0Min,c0Max, c1Min,c1Max, ...]`（长度 2*(1+k)）
///
/// NaN 处理按 IEEE 语义：比较恒 false，天然被跳过。全部为 NaN 时返回全 0（§8.2）。
template <typename T>
std::vector<double> ComputeRange(const T* data, int64_t count, int components) {
    const size_t k = static_cast<size_t>(components);
    std::vector<double> out;

    if (count <= 0 || k == 0) {
        out.assign(k <= 1 ? 2 : 2 * (1 + k), 0.0);
        return out;
    }

    if (k == 1) {
        double lo = std::numeric_limits<double>::quiet_NaN();
        double hi = lo;
        for (int64_t i = 0; i < count; ++i) {
            const double v = static_cast<double>(data[i]);
            if (std::isnan(v)) { continue; }
            if (std::isnan(lo) || v < lo) { lo = v; }
            if (std::isnan(hi) || v > hi) { hi = v; }
        }
        if (std::isnan(lo)) { lo = 0.0; }
        if (std::isnan(hi)) { hi = 0.0; }
        out = {lo, hi};
        return out;
    }

    std::vector<double> cmin(k, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> cmax(k, std::numeric_limits<double>::quiet_NaN());
    double magMin = std::numeric_limits<double>::quiet_NaN();
    double magMax = magMin;

    for (int64_t i = 0; i < count; ++i) {
        const T* rec = data + i * k;
        double sum = 0.0;
        bool anyFinite = false;
        for (size_t j = 0; j < k; ++j) {
            const double v = static_cast<double>(rec[j]);
            if (std::isnan(v)) { continue; }
            anyFinite = true;
            if (std::isnan(cmin[j]) || v < cmin[j]) { cmin[j] = v; }
            if (std::isnan(cmax[j]) || v > cmax[j]) { cmax[j] = v; }
            sum += v * v;
        }
        if (!anyFinite) { continue; }
        const double mag = std::sqrt(sum);
        if (std::isnan(magMin) || mag < magMin) { magMin = mag; }
        if (std::isnan(magMax) || mag > magMax) { magMax = mag; }
    }

    if (std::isnan(magMin)) { magMin = 0.0; }
    if (std::isnan(magMax)) { magMax = 0.0; }
    out.reserve(2 * (1 + k));
    out.push_back(magMin);
    out.push_back(magMax);
    for (size_t j = 0; j < k; ++j) {
        out.push_back(std::isnan(cmin[j]) ? 0.0 : cmin[j]);
        out.push_back(std::isnan(cmax[j]) ? 0.0 : cmax[j]);
    }
    return out;
}

} // namespace

// =============================================================================

void IGVWriter::Fail(const std::string& reason) {
    m_LastError = reason;
 IGAME_CORE_ERROR("[IGV] write rejected: {}", reason);
}

bool IGVWriter::GenerateBuffers() {
    m_MemoryImage.clear();
    m_LastError.clear();

    // 1. 前置校验（§5.1 步骤 1）
    if (!ValidateDataObject()) {
        return false;
    }

    if (m_Options.writeMode == IGVWriteMode::DirectPath) {
        // 说明（为什么这里仍然先建内存镜像再交给基类落盘）：
        //
        // `FileWriter::Execute()` 在 `GenerateBuffers()` 之后**无条件**调用
        // `SaveBufferDataToFile()`，而它用 `fopen(m_FilePath, "wb")` —— 会把文件截断。
        // 因此若在 GenerateBuffers() 里直接 H5Fcreate(path)，随后立刻被基类截成 0 字节。
        //
        // 真正的"绕开内存镜像直写"入口是 `WriteToFilePathDirect()`（§5.1 步骤 5 的
        // DirectPath 分支 / R2 / P7-4），它不经过 Execute()。而任何走 Execute() 的路径
        // 一律使用内存镜像 —— 与 IGCWriter 等既有 writer 的行为一致。
  IGAME_CORE_WARN("[IGV] writeMode=DirectPath has no effect on WriteToFile()"
                        " (the base class Execute() overwrites the file afterwards);"
                        " use WriteToFilePathDirect() instead."
                        " This run is treated as MemoryImage.");
        m_Options.writeMode = IGVWriteMode::MemoryImage;
    }

    // 5–10. 内存镜像
    if (!BuildFileImage(m_MemoryImage)) {
  if (m_LastError.empty()) { Fail("failed to build the HDF5 memory image"); }
        m_MemoryImage.clear();
        return false;
    }

    // 11. 交给基类落盘（§5.1 步骤 11–12）
    m_Buffers.resize(1, nullptr);
    m_Buffers[0] = CharArray::New();
    m_Buffers[0]->Resize(static_cast<IGsize>(m_MemoryImage.size()));
    if (!m_MemoryImage.empty()) {
        std::memcpy(m_Buffers[0]->RawPointer(), m_MemoryImage.data(), m_MemoryImage.size());
    }
    return true;
}

bool IGVWriter::WriteToFilePathDirect(DataObject::Pointer dataObject, const std::string& filePath) {
    m_MemoryImage.clear();
    m_LastError.clear();
    m_FilePath = filePath;
    SetDataObject(dataObject);
    // 直接走一次完整流程，但强制 DirectPath
    const IGVWriteOptions saved = m_Options;
    m_Options.writeMode = IGVWriteMode::DirectPath;

    // 复用 FileWriter::Execute() 的语义：它会把 input(0) 取到 m_DataObject。
    m_DataObject = dataObject;
    const bool ok = ValidateDataObject() && BuildFileAtPath(filePath);

    m_Options = saved;
 if (!ok && m_LastError.empty()) { Fail("HDF5 direct write failed: " + filePath); }
    return ok;
}

// =============================================================================
// 前置校验（§5.1 步骤 1 / §5.4 / V1.7）
// =============================================================================

bool IGVWriter::ValidateDataObject() {
    // 1.1 m_DataObject 非空
    if (!m_DataObject) {
  Fail("m_DataObject is empty");
        return false;
    }

    // 1.2 必须是 IGV 支持的五类网格之一
    std::string_view typeToken;
    std::string_view igvDataTypeToken;
    const IGenum meshType = m_DataObject->GetDataObjectType();
    if (!MapMeshType(meshType, typeToken, igvDataTypeToken)) {
        // 明确报出类型值（§5.4："非 5 类网格 → return false + 报出类型值"）
  Fail("unsupported data object type " + std::to_string(static_cast<long long>(meshType)) +
             " (only supports IG_POINT_SET(3) / IG_SURFACE_MESH(4) / IG_VOLUME_MESH(5) / "
             "IG_UNSTRUCTURED_MESH(6) / IG_STRUCTURED_MESH(7))");
        return false;
    }

    // 1.3 经适配器取统一视图（复用 IGC 已验证的取数逻辑，§4.1 支点）
    auto adapter = std::make_unique<MeshEncodeAdapterFromDataObject>(m_DataObject);

    // 1.4 / 1.5 分量数与 f32 硬校验（A8 / A9）。
    //
    // 注意 `Points` **本身没有** GetDimension() / GetArrayTypedSize()
    // —— 它的内部缓冲是 `FloatArray m_Buffer`（见 iGamePoints.h:82），
    // 通过 `ConvertToArray()` 才拿得到那个 ArrayObject。
    // 若对 Points 直接调这两个函数，会得到
    // "GetDimension is not a member of iGame::Points"。
    Points::Pointer points = adapter->GetPoints();
    if (!points) {
  Fail("GetPoints() is empty");
        return false;
    }

    // 1.4a 点数必须 > 0。
    //
    // 为什么必须在这里拦：IGV 的计数组 `NumberOfPoints` 与 `Points` 的 dataspace
    // 是同一个事实的两种表述，而 HDF5 允许 0 长度 dataset —— 也就是说"空点云"
    // **可以**被写成一个自洽的文件，但读侧会得到一个没有任何点的网格对象，
    // 后续渲染/遍历都可能拿到空缓冲。规范把 `Points` 定义为必填的几何数组，
    // 因此这里按"无法如实表达"拒绝，与 §5.4「每个拒绝都在写盘之前」一致。
    //
    // 这条缺口是 V1.7 第 3 项暴露的（P1 首次可运行验证时该用例失败）。
    if (points->GetNumberOfPoints() == 0) {
        Fail("Points is empty (NumberOfPoints == 0): a mesh must have at least one point");
        return false;
    }

    // `ConvertToArray()` 给出 Points 内部的那个 FloatArray（ArrayObject），
    // 分量数与值宽都要从它上面读。
    const FloatArray::Pointer pointArray = points->ConvertToArray();
    if (!pointArray) {
  Fail("Points::ConvertToArray() returned null");
        return false;
    }

    // 1.4 分量数必须为 3
    if (pointArray->GetDimension() != kPointsComponents) {
  Fail("Points component count must be " + std::to_string(kPointsComponents) +
             ", actual " + std::to_string(pointArray->GetDimension()));
        return false;
    }

    // 1.5 f32 硬校验（A8 / A9）——**不得退化为 assert**。
    if (pointArray->GetArrayTypedSize() != static_cast<IGsize>(sizeof(float))) {
  Fail("Points must be float32 (f32): actual bytes per component " +
             std::to_string(pointArray->GetArrayTypedSize()) +
             " (A8 allows only f32)");
        return false;
    }

    // 1.6 属性校验：elementCount / 名称 / 类型映射
    const IGenum type = m_DataObject->GetDataObjectType();
    const int64_t pointCount = static_cast<int64_t>(points->GetNumberOfPoints());
    const int64_t cellCount = (type == IG_POINT_SET)
                                  ? 0
                                  : static_cast<int64_t>(adapter->GetNumberOfCells());

    AttributeSet* attrSet = m_DataObject->GetAttributeSet();
    if (attrSet != nullptr) {
        const IGsize attrCount = attrSet->GetNumberOfAttributes();
        for (IGsize i = 0; i < attrCount; ++i) {
            AttributeSet::Attribute& attr = attrSet->GetAttribute(i);
            if (attr.IsNone()) { continue; }

            ArrayObject::Pointer array = attr.GetPointer();
            if (!array) { continue; }

            const std::string name = array->GetName();

            if (!IsValidHdf5Name(name)) {
    Fail("attribute name is not a valid HDF5 name: '" + name +
                     "' (must not be empty, must not start with '.', must not contain '/')");
                return false;
            }

            DType dt = DType::Unknown;
            if (!MapArrayDType(array->GetArrayType(), dt)) {
    Fail("attribute '" + name + "' has an array type outside IGV support (only "
                     "FloatArray(f32) / DoubleArray(f64) / UnsignedCharArray(u8))");
                return false;
            }

            if (MapAttributeType(attr.GetType()).empty()) {
    Fail("attribute '" + name + "' AttributeType=" +
                     std::to_string(static_cast<long long>(attr.GetType())) +
                     " cannot be mapped to an IGV token (supported: IG_SCALAR/IG_VECTOR/IG_NORMAL/"
                     "IG_TCOORD/IG_TENSOR/IG_RGB)");
                return false;
            }

            if (MapAttachment(attr.GetAttachmentType()).empty()) {
    Fail("attribute '" + name + "' attachmentType=" +
                     std::to_string(static_cast<long long>(attr.GetAttachmentType())) +
                     " cannot be mapped to an IGV token");
                return false;
            }

            const int components = array->GetDimension();
            if (components <= 0) {
    Fail("attribute '" + name + "' component count is " + std::to_string(components));
                return false;
            }

            const int64_t elementCount = static_cast<int64_t>(array->GetNumberOfElements());

            // §2.4(B)2 落地：写盘侧校验 elementCount，而不是读盘侧静默截断
            const IGenum attachment = attr.GetAttachmentType();
            if (attachment == IG_POINT) {
                if (elementCount != pointCount) {
     Fail("point attribute '" + name + "' elementCount=" +
                         std::to_string(elementCount) + " vs pointCount=" +
                         std::to_string(pointCount) + " mismatch");
                    return false;
                }
            } else if (attachment == IG_CELL) {
                // PointSet 没有单元：单元属性无法挂载
                if (type == IG_POINT_SET) {
     Fail("point cloud(PointSet)unsupportedcellattribute: '" + name + "'");
                    return false;
                }
                if (elementCount != cellCount) {
     Fail("cell attribute '" + name + "' elementCount=" +
                         std::to_string(elementCount) + " vs cellCount=" +
                         std::to_string(cellCount) + " mismatch");
                    return false;
                }
            }
            // 其余 attachment（edge/box/change/...）不进 PointData/CellData 的长度校验：
            // 它们的元素数与点/单元数无固定关系（§3.8 归入 FieldData）。
        }
    }

    // 1.7 Types 白名单校验放在拓扑暂存阶段（需要先把 Types 求出来）。

    // StructuredMesh 需要 Dimensions
    if (type == IG_STRUCTURED_MESH) {
        auto mesh = DynamicCast<StructuredMesh>(m_DataObject);
        const igIndex* dims = mesh->GetDimensionSize();
        if (!dims) {
   Fail("StructuredMesh::GetDimensionSize() returned null");
            return false;
        }
        const int64_t nx = dims[0], ny = dims[1], nz = dims[2];
        if (nx <= 0 || ny <= 0 || nz <= 0) {
   Fail("StructuredMesh Dimensions are invalid: [" + std::to_string(nx) + ", " +
                 std::to_string(ny) + ", " + std::to_string(nz) + "]");
            return false;
        }
        if (nx * ny * nz != pointCount) {
   Fail("StructuredMesh nx*ny*nz=" + std::to_string(nx * ny * nz) +
                 " vs pointCount=" + std::to_string(pointCount) + " mismatch");
            return false;
        }
    }

    return true;
}

// =============================================================================
// HDF5 建库
// =============================================================================

bool IGVWriter::BuildFileImage(std::vector<char>& outImage) {
    return BuildHdf5(std::string{}, &outImage);
}

bool IGVWriter::BuildFileAtPath(const std::string& path) {
    return BuildHdf5(path, nullptr);
}

bool IGVWriter::BuildHdf5(const std::string& pathForDirectWrite, std::vector<char>* memoryImage) {
    // -------------------------------------------------------------------------
    // 阶段 A：经适配器取统一视图并全部暂存到内存（写 HDF5 之前完成所有校验）
    // -------------------------------------------------------------------------

    auto adapter = std::make_unique<MeshEncodeAdapterFromDataObject>(m_DataObject);

    const IGenum meshType = m_DataObject->GetDataObjectType();
    std::string_view typeToken;
    std::string_view igvDataTypeToken;
    MapMeshType(meshType, typeToken, igvDataTypeToken);

    Points::Pointer points = adapter->GetPoints();
    const int64_t pointCount = static_cast<int64_t>(points->GetNumberOfPoints());
    const int64_t pointValueCount = pointCount * kPointsComponents;

    std::vector<float> pointData(static_cast<size_t>(pointValueCount));
    if (pointValueCount > 0) {
        std::memcpy(pointData.data(), points->RawPointer(),
                    static_cast<size_t>(pointValueCount) * sizeof(float));
    }

    // Bounds（§8.1）。复用 DataObject 既有能力（§8.1 末条要求先尝试复用）。
    std::vector<double> bounds(kBoundsCount, 0.0);
    bool hasBounds = false;
    if (pointCount > 0) {
        double bmin[3] = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
        double bmax[3] = {std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest()};
        for (int64_t i = 0; i < pointCount; ++i) {
            const float* p = pointData.data() + i * kPointsComponents;
            for (int a = 0; a < 3; ++a) {
                const double v = static_cast<double>(p[a]);
                if (v < bmin[a]) { bmin[a] = v; }
                if (v > bmax[a]) { bmax[a] = v; }
            }
        }
        for (int a = 0; a < 3; ++a) {
            bounds[2 * a] = bmin[a];
            bounds[2 * a + 1] = bmax[a];
        }
        hasBounds = true;
    }

    // ---- 拓扑（按类型分型，§3.1）----
    const bool isPointSet = (meshType == IG_POINT_SET);
    const bool isStructured = (meshType == IG_STRUCTURED_MESH);
    const bool hasTopology = !isPointSet && !isStructured;

    std::vector<int32_t> connectivity;
    std::vector<int64_t> offsets;
    std::vector<uint8_t> types;
    int64_t cellCount = 0;
    int64_t connectivityIdCount = 0;
    int64_t structuredDims[3] = {0, 0, 0};
    int64_t fixedCellSize = -1;

    if (isStructured) {
        auto mesh = DynamicCast<StructuredMesh>(m_DataObject);
        const igIndex* dims = mesh->GetDimensionSize();
        structuredDims[0] = dims[0];
        structuredDims[1] = dims[1];
        structuredDims[2] = dims[2];
    }

    if (hasTopology) {
        // 多面体（二级索引）在本期不支持：§3.7 明确 IGV 只存一级索引。
        // 必须在写盘前拒绝 —— 否则会写出"一级索引 + Types 标成多面体"的损坏文件，
        // 读取侧将走到未定义行为（V1.7 第 6 项）。
        if (adapter->IsSecondaryIndexPolyhedronMesh()) {
   Fail("polyhedron meshes (secondary index, e.g. IG_POLYHEDRON volumes->faces->points) "
                 "are not supported in this phase: IGV stores a primary index only "
                 "(spec section 3.7)");
            return false;
        }

        CellArray* cells = m_DataObject->GetCellArray();
        IdArray::Pointer cellIds = adapter->GetCellIdBuffer();

        if (cells == nullptr || !cellIds) {
   Fail("GetCellArray()/GetCellIdBuffer() returned null for a non-structural mesh");
            return false;
        }

        cellCount = static_cast<int64_t>(adapter->GetNumberOfCells());
        if (cellCount <= 0) {
   Fail("non-structural mesh has cell count " + std::to_string(cellCount));
            return false;
        }

        // ---- Offsets + 真实 Connectivity 长度 ----
        //
        // 走 CellArray 的权威接口（BuildIgvOffsetsFromCellArray 的注释解释了
        // 为什么**不能**用 GetCellIdBufferSize()/GetNumberOfIds()：那些含 padding）。
        connectivityIdCount = BuildIgvOffsetsFromCellArray(cells, cellCount, fixedCellSize, offsets);
        if (connectivityIdCount < 0) {
   Fail("cannot derive Offsets from CellArray (neither a real offset array nor a valid fixed-size mesh)");
            return false;
        }
        if (connectivityIdCount == 0) {
   Fail("Connectivity length is 0 (the mesh has no connectivity data)");
            return false;
        }

        // 交叉校验：padding 容量必须 >= 真实长度；反过来若真实长度 > 容量，
        // 说明 CellArray 状态自相矛盾（索引会越界读取）。
        const IGsize bufferCapacity = cellIds->GetNumberOfIds();
        if (static_cast<int64_t>(bufferCapacity) < connectivityIdCount) {
   Fail("CellArray state contradiction: true connectivity length(" +
                 std::to_string(connectivityIdCount) + ") > index buffer capacity(" +
                 std::to_string(bufferCapacity) + ")");
            return false;
        }

        // I2 / I3（首元素 0、严格递增）—— 这两条与缓冲容量无关，是纯规范不变量
        if (offsets.front() != 0) {
   Fail("Offsets[0] must be 0, actual " + std::to_string(offsets.front()));
            return false;
        }
        for (size_t i = 0; i + 1 < offsets.size(); ++i) {
            if (offsets[i + 1] <= offsets[i]) {
    Fail("Offsets must be strictly increasing, at index " + std::to_string(i) + ": " +
                     std::to_string(offsets[i]) + " -> " + std::to_string(offsets[i + 1]));
                return false;
            }
        }
        // I4：末值必须等于真实 Connectivity 长度（**不是** buffer 容量）
        if (offsets.back() != connectivityIdCount) {
   Fail("Offsets last value(" + std::to_string(offsets.back()) +
                 ") must equal the true connectivity length (" + std::to_string(connectivityIdCount) + ")");
            return false;
        }

        // ---- Connectivity（i32）。越界索引是损坏文件，写盘前拒绝 ----
        //
        // 逐单元经 `CellArray::GetCellIds()` 取，而不是直接扫 `m_Buffer` 的
        // 前 N 个元素：`m_Buffer` 是翻倍扩容的，其**有效前缀**就是 CellArray 的
        // offset 语义所描述的范围；用 GetCellIds 能保证拿到的是"每个单元真实
        // 的那段索引"，与 offsets 天然一致。同时顺带校验每单元点数与 Offsets
        // 差分相符 —— 若不符说明源对象自身不一致，必须在写盘前拒绝。
        connectivity.resize(static_cast<size_t>(connectivityIdCount));
        {
            size_t cursor = 0;
            for (int64_t c = 0; c < cellCount; ++c) {
                // 用取指针的重载（签名见 iGameCellArray.h:49）：
                //   int GetCellIds(IGsize cellId, const igIndex*& cell) const
                const igIndex* ids = nullptr;
                const int n = cells->GetCellIds(static_cast<IGsize>(c), ids);
                if (n <= 0 || ids == nullptr) {
     Fail("cell " + std::to_string(c) + " has point count " + std::to_string(n));
                    return false;
                }
                const int64_t expect = offsets[static_cast<size_t>(c) + 1] -
                                       offsets[static_cast<size_t>(c)];
                if (static_cast<int64_t>(n) != expect) {
     Fail("cell " + std::to_string(c) + " point count (" + std::to_string(n) +
                         ") differs from the Offsets difference (" + std::to_string(expect) + ")");
                    return false;
                }
                if (cursor + static_cast<size_t>(n) > connectivity.size()) {
     Fail("cell " + std::to_string(c) + " index exceeds the Connectivity length");
                    return false;
                }
                for (int k = 0; k < n; ++k) {
                    const igIndex id = ids[k];
                    if (id < 0 || static_cast<int64_t>(id) >= pointCount) {
      Fail("Connectivity[" + std::to_string(cursor + static_cast<size_t>(k)) +
                             "] = " + std::to_string(id) + " is out of range (pointCount = " +
                             std::to_string(pointCount) + ")");
                        return false;
                    }
                    connectivity[cursor + static_cast<size_t>(k)] = static_cast<int32_t>(id);
                }
                cursor += static_cast<size_t>(n);
            }
            if (cursor != connectivity.size()) {
    Fail("index count gathered per cell (" + std::to_string(cursor) +
                     ") vs the Offsets last value (" + std::to_string(connectivity.size()) + ") mismatch");
                return false;
            }
        }

        // ---- Types（§3.2 / §3.5.1）----
        types.resize(static_cast<size_t>(cellCount));
        if (meshType == IG_UNSTRUCTURED_MESH) {
            // 有权威来源：UnstructuredMesh::GetCellTypes()
            UnsignedIntArray::Pointer cellTypes = adapter->GetCellTypes();
            if (!cellTypes || static_cast<int64_t>(cellTypes->GetNumberOfValues()) != cellCount) {
    Fail("UnstructuredMesh::GetCellTypes() is null or has length (" +
                     std::to_string(cellTypes ? static_cast<long long>(
                                                     cellTypes->GetNumberOfValues())
                                               : -1) +
                     ") which differs from cell count (" + std::to_string(cellCount) + ")mismatch");
                return false;
            }
            for (int64_t i = 0; i < cellCount; ++i) {
                // GetValue() 返回 double；**不要**对它做位运算（& 0xFF 会报
                // "left operand has type double"），直接 static_cast 截断即可。
                types[static_cast<size_t>(i)] = static_cast<uint8_t>(
                    cellTypes->GetValue(static_cast<IGsize>(i)));
            }
        } else {
            // SurfaceMesh / VolumeMesh 无 Types 数组（适配器只对 UnstructuredMesh 暴露），
            // 由单元点数推导（§3.7）。**全部单元点数必须一致**，否则无法如实表达。
            for (int64_t i = 0; i < cellCount; ++i) {
                const int64_t size = offsets[static_cast<size_t>(i) + 1] -
                                     offsets[static_cast<size_t>(i)];
                IGCellType t = IG_EMPTY_CELL;
                if (!CellTypeFromSize(static_cast<IGsize>(size), t)) {
     Fail("SurfaceMesh/VolumeMesh has no Types array, and cell " + std::to_string(i) +
                         " has point count " + std::to_string(size) +
                         ", so IGCellType cannot be determined uniquely. Use UnstructuredMesh instead");
                    return false;
                }
                if (i > 0 && types[0] != static_cast<uint8_t>(t)) {
     Fail("SurfaceMesh/VolumeMesh has mixed cell sizes (cell 0 is " +
                         std::to_string(types[0]) + ", cell " + std::to_string(i) + " is " +
                         std::to_string(static_cast<int>(t)) +
                         "), which cannot be expressed faithfully without a Types array. Use UnstructuredMesh instead");
                    return false;
                }
                types[static_cast<size_t>(i)] = static_cast<uint8_t>(t);
            }
        }

        // §5.1 步骤 1.7 / §3.5.1 / V1.7 第 4–7 项：Types 白名单硬校验。
        // 这一条是写侧白名单的**唯一防线**（尤其 IG_POLYHEDRON）。
        for (int64_t i = 0; i < cellCount; ++i) {
            const int code = static_cast<int>(types[static_cast<size_t>(i)]);
            if (IsWritableCellType(code)) { continue; }

            if (const auto* forbidden = FindForbiddenCellType(code)) {
    Fail("Types[" + std::to_string(i) + "] = " + std::to_string(code) + " (" +
                     std::string(forbidden->name) + ") is forbidden to write: " +
                     std::string(forbidden->reason));
            } else {
    Fail("Types[" + std::to_string(i) + "] = " + std::to_string(code) +
                     " is not in the IGV cell-type whitelist (see iGameIGVCellType.h / spec section 3.5.1)");
            }
            return false;
        }

        // Types 与 Offsets 的一致性自检（读侧会交叉校验，写侧先自查）
        if (meshType == IG_UNSTRUCTURED_MESH) {
            for (int64_t i = 0; i < cellCount; ++i) {
                const int64_t size = offsets[static_cast<size_t>(i) + 1] -
                                     offsets[static_cast<size_t>(i)];
                const int expect = FixedCellSizeOfType(static_cast<int>(types[static_cast<size_t>(i)]));
                if (expect > 0 && size != expect) {
     Fail("cell " + std::to_string(i) + " has Types=" +
                         std::to_string(static_cast<int>(types[static_cast<size_t>(i)])) +
                         " expectedpoint count " + std::to_string(expect) + ", but the Offsets difference is " +
                         std::to_string(size));
                    return false;
                }
            }
        }
    }

    // ---- 属性暂存（§3.6 / §3.8）----
    std::vector<StagedAttribute> staged;
    AttributeSet* attrSet = m_DataObject->GetAttributeSet();
    if (attrSet != nullptr) {
        const IGsize attrCount = attrSet->GetNumberOfAttributes();
        for (IGsize i = 0; i < attrCount; ++i) {
            AttributeSet::Attribute& attr = attrSet->GetAttribute(i);
            // IsNone() 覆盖 pointer==null / type==IG_NONE / attachmentType==IG_NONE / isDeleted
            if (attr.IsNone()) { continue; }
            ArrayObject::Pointer array = attr.GetPointer();
            if (!array) { continue; }

            StagedAttribute sa;
            sa.name = array->GetName();
            sa.igAttributeType = attr.GetType();
            sa.igAttachmentType = attr.GetAttachmentType();
            sa.attributeTypeToken = std::string(MapAttributeType(sa.igAttributeType));
            sa.attachmentToken = std::string(MapAttachment(sa.igAttachmentType));
            sa.groupName = std::string(GroupOfAttachment(sa.igAttachmentType));
            sa.components = array->GetDimension();
            sa.count = static_cast<int64_t>(array->GetNumberOfElements());

            // ValidateDataObject() 已校验过以下三项，这里只是防御性重查
            // （BuildHdf5 也可能被 WriteToFilePathDirect 之外的路径直接调用）。
            if (!MapArrayDType(array->GetArrayType(), sa.dtype)) {
    Fail("attribute '" + sa.name + "' has an unsupported array type");
                return false;
            }
            if (sa.components <= 0) {
    Fail("attribute '" + sa.name + "' component count is " + std::to_string(sa.components));
                return false;
            }
            if (sa.attachmentToken.empty() || sa.attributeTypeToken.empty()) {
    Fail("attribute '" + sa.name + "' iGame enums cannot be mapped to IGV tokens");
                return false;
            }

            const size_t valueCount = static_cast<size_t>(array->GetNumberOfValues());
            if (sa.dtype == DType::F32) {
                auto f = DynamicCast<FloatArray>(array);
                sa.f32.assign(f->RawPointer(), f->RawPointer() + valueCount);
                sa.range = ComputeRange(sa.f32.data(), sa.count, sa.components);
            } else if (sa.dtype == DType::F64) {
                auto d = DynamicCast<DoubleArray>(array);
                sa.f64.assign(d->RawPointer(), d->RawPointer() + valueCount);
                sa.range = ComputeRange(sa.f64.data(), sa.count, sa.components);
            } else {
                auto u = DynamicCast<UnsignedCharArray>(array);
                sa.u8.assign(u->RawPointer(), u->RawPointer() + valueCount);
                sa.range = ComputeRange(sa.u8.data(), sa.count, sa.components);
            }

            staged.push_back(std::move(sa));
        }
    }

    // -------------------------------------------------------------------------
    // 阶段 B：建 HDF5（core VFD 或直连路径）
    // -------------------------------------------------------------------------

    ClearHdf5Errors();

    H5FileHandle file;
    if (memoryImage != nullptr) {
        // §4.1 支点 1 / §5.1 步骤 5：core VFD 在内存里生成完整 file image。
        //
        // backing_store = false 表示"不落盘到 backing file"，这正是我们要的：
        // 字节由 H5Fget_file_image 取回，落盘交给基类的 SaveBufferDataToFile。
        H5PropertyHandle fapl(H5Pcreate(H5P_FILE_ACCESS));
  if (!fapl.IsValid()) { ClearHdf5Errors(); Fail("H5Pcreate(FILE_ACCESS) failed"); return false; }

        if (H5Pset_fapl_core(fapl.Get(), static_cast<size_t>(1024 * 1024), 0) < 0) {
            ClearHdf5Errors();
   Fail("H5Pset_fapl_core failed");
            return false;
        }

        file.Reset(H5Fcreate("igv_memory_image.igv", H5F_ACC_TRUNC, H5P_DEFAULT, fapl.Get()));
        if (!file.IsValid()) {
            ClearHdf5Errors();
   Fail("H5Fcreate(core VFD) failed");
            return false;
        }
    } else {
        file.Reset(H5Fcreate(pathForDirectWrite.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));
        if (!file.IsValid()) {
            ClearHdf5Errors();
   Fail("H5Fcreate failed: " + pathForDirectWrite);
            return false;
        }
    }

    const bool compress = (m_Options.compression == IGVCompression::GzipDefault);

    // ---- /IGV 根组 + §3.1 的 attribute ----
    H5GroupHandle root(H5Gcreate2(file.Get(), std::string(kRootGroup).c_str(),
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    if (!root.IsValid()) {
        ClearHdf5Errors();
  Fail("H5Gcreate(/IGV) failed");
        return false;
    }

    if (!WriteAsciiAttribute(root.Get(), kAttrType, typeToken) ||
        !WriteAsciiAttribute(root.Get(), kAttrIGVDataType, igvDataTypeToken) ||
        !WriteAsciiAttribute(root.Get(), kAttrCellTypeEncoding, kCellTypeEncodingIGCellType)) {
        ClearHdf5Errors();
  Fail("failed to write root-group string attributes");
        return false;
    }

    {
        const int64_t version[2] = {kVersion[0], kVersion[1]};
        const hsize_t vdims[1] = {2};
        if (!WriteDataset(root.Get(), kDatasetVersion, DType::I64,
                          std::vector<hsize_t>(vdims, vdims + 1), version, false)) {
            ClearHdf5Errors();
   Fail("failed to write the Version dataset");
            return false;
        }
    }

    if (hasBounds) {
        if (!WriteArrayAttribute(root.Get(), kAttrBounds, DType::F64, bounds.data(), bounds.size())) {
            ClearHdf5Errors();
   Fail("failed to write the Bounds attribute");
            return false;
        }
    }

    if (isStructured) {
        if (!WriteI64ArrayAttribute(root.Get(), kAttrDimensions, structuredDims, 3)) {
            ClearHdf5Errors();
   Fail("failed to write the Dimensions attribute");
            return false;
        }
    }

    if (fixedCellSize > 0) {
        if (!WriteI64Attribute(root.Get(), kAttrFixedCellSize, fixedCellSize)) {
            ClearHdf5Errors();
   Fail("failed to write the FixedCellSize attribute");
            return false;
        }
    }

    if (!m_Options.generator.empty()) {
        if (!WriteAsciiAttribute(root.Get(), kAttrGenerator, m_Options.generator)) {
            ClearHdf5Errors();
   Fail("failed to write the Generator attribute");
            return false;
        }
    }

    // ---- 几何 / 拓扑 dataset ----
    {
        const hsize_t pdims[2] = {static_cast<hsize_t>(pointCount),
                                  static_cast<hsize_t>(kPointsComponents)};
        if (!WriteDataset(root.Get(), kDatasetPoints, DType::F32,
                          std::vector<hsize_t>(pdims, pdims + 2), pointData.data(), compress)) {
            ClearHdf5Errors();
   Fail("failed to write the Points dataset");
            return false;
        }
    }

    if (hasTopology) {
        const hsize_t cdims[1] = {static_cast<hsize_t>(connectivity.size())};
        if (!WriteDataset(root.Get(), kDatasetConnectivity, DType::I32,
                          std::vector<hsize_t>(cdims, cdims + 1), connectivity.data(), compress)) {
            ClearHdf5Errors();
   Fail("failed to write the Connectivity dataset");
            return false;
        }

        const hsize_t odims[1] = {static_cast<hsize_t>(offsets.size())};
        if (!WriteDataset(root.Get(), kDatasetOffsets, DType::I64,
                          std::vector<hsize_t>(odims, odims + 1), offsets.data(), compress)) {
            ClearHdf5Errors();
   Fail("failed to write the Offsets dataset");
            return false;
        }

        const hsize_t tdims[1] = {static_cast<hsize_t>(types.size())};
        if (!WriteDataset(root.Get(), kDatasetTypes, DType::U8,
                          std::vector<hsize_t>(tdims, tdims + 1), types.data(), compress)) {
            ClearHdf5Errors();
   Fail("failed to write the Types dataset");
            return false;
        }
    }

    // ---- §5.2：三个计数组必须用**回读实际落盘维度**填充，不能用写前期望值 ----
    //
    // 理由（§5.2 / R12）：一旦写前计算与实际写出不一致（例如属性被静默截断），
    // 用期望值填的计数组会"自洽地错"，读取侧的交叉校验（§6.3）将无法发现问题。
    // 只有计数组独立于数组长度来源时，它才有校验价值。
    {
        int64_t actualPoints = 0;
        if (!ReadbackElementCount(root.Get(), kDatasetPoints, actualPoints)) {
            ClearHdf5Errors();
   Fail("failed to read back the Points dimensions for NumberOfPoints");
            return false;
        }
        if (!WriteCountDataset(root.Get(), kDatasetNumberOfPoints, actualPoints)) {
            ClearHdf5Errors();
   Fail("failed to write NumberOfPoints");
            return false;
        }
        if (!m_Options.skipCountReadbackCheck && actualPoints != pointCount) {
            ClearHdf5Errors();
   Fail("NumberOfPoints read back as (" + std::to_string(actualPoints) +
                 ") but the intended value was (" + std::to_string(pointCount) + ") mismatch");
            return false;
        }

        if (hasTopology) {
            int64_t actualCells = 0;
            int64_t actualConn = 0;
            if (!ReadbackElementCount(root.Get(), kDatasetTypes, actualCells) ||
                !ReadbackElementCount(root.Get(), kDatasetConnectivity, actualConn)) {
                ClearHdf5Errors();
    Fail("failed to read back the source dimensions for NumberOfCells/NumberOfConnectivityIds");
                return false;
            }
            if (!WriteCountDataset(root.Get(), kDatasetNumberOfCells, actualCells) ||
                !WriteCountDataset(root.Get(), kDatasetNumberOfConnectivityIds, actualConn)) {
                ClearHdf5Errors();
    Fail("failed to write NumberOfCells/NumberOfConnectivityIds");
                return false;
            }
            if (!m_Options.skipCountReadbackCheck &&
                (actualCells != cellCount || actualConn != connectivityIdCount)) {
                ClearHdf5Errors();
    Fail("count datasets read back as (ncells=" + std::to_string(actualCells) + ", nconn=" +
                     std::to_string(actualConn) + ") but the intended values were (ncells=" +
                     std::to_string(cellCount) + ", nconn=" +
                     std::to_string(connectivityIdCount) + ") mismatch");
                return false;
            }
        }
    }

    // ---- 属性组与 dataset（§3.6 / §3.8）----
    for (std::string_view groupName :
         {kGroupPointData, kGroupCellData, kGroupFieldData}) {
        H5GroupHandle g(H5Gcreate2(root.Get(), std::string(groupName).c_str(),
                                   H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        if (!g.IsValid()) {
            ClearHdf5Errors();
   Fail("H5Gcreate(" + std::string(groupName) + ") failed");
            return false;
        }
    }

    for (const StagedAttribute& sa : staged) {
        H5GroupHandle g(H5Gopen2(root.Get(), sa.groupName.c_str(), H5P_DEFAULT));
        if (!g.IsValid()) {
            ClearHdf5Errors();
   Fail("H5Gopen(" + sa.groupName + ") failed");
            return false;
        }

        const hsize_t dims[2] = {static_cast<hsize_t>(sa.count),
                                 static_cast<hsize_t>(sa.components)};
        // 标量属性写成 rank-1，多分量写成 rank-2（与 golden 样例一致，
        // 也与 VTKHDF 的 (n) / (n, k) 惯例一致）
        const std::vector<hsize_t> shape =
            (sa.components == 1) ? std::vector<hsize_t>{dims[0]}
                                 : std::vector<hsize_t>{dims[0], dims[1]};

        if (!WriteDataset(g.Get(), sa.name, sa.dtype, shape, sa.Data(), compress)) {
            ClearHdf5Errors();
   Fail("failed to write attribute dataset '" + sa.name + "' failed");
            return false;
        }

        H5DatasetHandle ds(H5Dopen2(g.Get(), sa.name.c_str(), H5P_DEFAULT));
        if (!ds.IsValid()) {
            ClearHdf5Errors();
   Fail("failed to reopen attribute dataset '" + sa.name + "' failed");
            return false;
        }

        if (!WriteAsciiAttribute(ds.Get(), kAttrAttributeType, sa.attributeTypeToken) ||
            !WriteAsciiAttribute(ds.Get(), kAttrAttachment, sa.attachmentToken)) {
            ClearHdf5Errors();
   Fail("failed to write attribute '" + sa.name + "' AttributeType/attachment");
            return false;
        }

        // 规范：components==1 → 长度 2；components>1 → 长度 2*(1+components)
        const size_t expectedRangeLen =
            (sa.components == 1) ? 2 : 2 * (1 + static_cast<size_t>(sa.components));
        if (sa.range.size() != expectedRangeLen) {
            ClearHdf5Errors();
   Fail("attribute '" + sa.name + "' range length (" + std::to_string(sa.range.size()) +
                 ") should be " + std::to_string(expectedRangeLen));
            return false;
        }
        if (!WriteArrayAttribute(ds.Get(), kAttrRange, DType::F64, sa.range.data(), sa.range.size())) {
            ClearHdf5Errors();
   Fail("failed to write attribute '" + sa.name + "' range");
            return false;
        }

        // iGame 枚举原值（冗余但保真，§3.6）
        if (!WriteI64Attribute(ds.Get(), kAttrIGAttributeType,
                              static_cast<int64_t>(sa.igAttributeType)) ||
            !WriteI64Attribute(ds.Get(), kAttrIGAttachmentType,
                              static_cast<int64_t>(sa.igAttachmentType))) {
            ClearHdf5Errors();
   Fail("failed to write attribute '" + sa.name + "' iGame enum raw values");
            return false;
        }
    }

    // ---- §5.1 步骤 9–10：flush + 取回 file image（必须在 H5Fclose 之前） ----
    if (memoryImage != nullptr) {
        if (H5Fflush(file.Get(), H5F_SCOPE_GLOBAL) < 0) {
            ClearHdf5Errors();
   Fail("H5Fflush failed");
            return false;
        }

        const ssize_t imageSize = H5Fget_file_image(file.Get(), nullptr, 0);
        if (imageSize <= 0) {
            ClearHdf5Errors();
   Fail("H5Fget_file_image failed to report the image size");
            return false;
        }

        memoryImage->assign(static_cast<size_t>(imageSize), 0);
        if (H5Fget_file_image(file.Get(), memoryImage->data(), static_cast<size_t>(imageSize)) < 0) {
            ClearHdf5Errors();
            memoryImage->clear();
   Fail("H5Fget_file_image failed to copy the image");
            return false;
        }
    } else {
        if (H5Fflush(file.Get(), H5F_SCOPE_GLOBAL) < 0) {
            ClearHdf5Errors();
   Fail("H5Fflush failed");
            return false;
        }
    }

    return true;
}

IGAME_NAMESPACE_END
