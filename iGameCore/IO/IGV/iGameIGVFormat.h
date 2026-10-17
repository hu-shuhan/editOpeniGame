#ifndef iGameIGVFormat_h
#define iGameIGVFormat_h

/**
 * @file  iGameIGVFormat.h
 * @brief IGV（iGameVis 通用格式）文件内布局常量表 —— 规范冻结的唯一来源。
 *
 * 设计约束（见 kia/igv分阶段实现方案.md §1.4 三条范围原则）：
 *   原则 1  只装 iGameVis 有的东西 —— 本表**不**为未实现的能力预留取值。
 *   原则 2  学 VTKHDF 的 schema 设计思想，不照抄它的 schema。
 *   原则 3  信息完整性以 iGameVis 自身为准。
 *
 * 本文件是纯静态数据，**不依赖 HDF5**、不依赖任何 iGame 数据模型，
 * 因此可被写侧、读侧、元数据快路径、以及将来的 Web 侧共用。
 *
 * 硬性约定：所有组名 / 数据集名 / 属性名 / 取值字符串**只在此处定义**，
 * 禁止在 .cpp 中散落字符串字面量（否则规范冻结形同虚设）。
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "iGameMacro.h"

IGAME_NAMESPACE_BEGIN

namespace igv {

// =============================================================================
// 1. 版本
// =============================================================================

/// IGV 当前版本 [major, minor]。major 不兼容时递增；minor 向后兼容递增。
inline constexpr int64_t kVersion[2] = {1, 0};

/// 本实现支持的最高 major。文件 major 高于此值 → 拒绝读取（见 §10.3）。
inline constexpr int64_t kSupportedMajor = 1;

/// 本实现支持的 minor。文件 minor 高于此值 → 接受，未知 attribute/dataset 跳过。
inline constexpr int64_t kSupportedMinor = 0;

// =============================================================================
// 2. 组名
// =============================================================================

/// 根组名（A2：IGV 自有，非 /VTKHDF）。
inline constexpr std::string_view kRootGroup = "IGV";

/// 属性分组名。分别承载 point / cell / 其它依附类型的属性。
inline constexpr std::string_view kGroupPointData = "PointData";
inline constexpr std::string_view kGroupCellData = "CellData";
inline constexpr std::string_view kGroupFieldData = "FieldData";

/// 多块子组名前缀。**仅用于"识别出这不是单块文件"**，不作为类型取值预留
/// （§1.4 原则 1：不为未实现的能力预留）。见 §12.2。
inline constexpr std::string_view kBlockGroupPrefix = "Block_";

// =============================================================================
// 3. 根组 attribute 名
// =============================================================================

inline constexpr std::string_view kAttrType = "Type";                        ///< 定长 ASCII，VTKHDF 同名值
inline constexpr std::string_view kAttrIGVDataType = "IGVDataType";          ///< 定长 ASCII，iGame 原名
inline constexpr std::string_view kAttrVersion = "Version";                  ///< **与 kDatasetVersion 同名**：见下方说明
inline constexpr std::string_view kAttrBounds = "Bounds";                    ///< float64[6]，IGV 自主增值项（IGC 不存）
inline constexpr std::string_view kAttrTimeValue = "TimeValue";              ///< float64 标量，可选
inline constexpr std::string_view kAttrTimeStep = "TimeStep";                ///< int64 标量，可选
inline constexpr std::string_view kAttrGenerator = "Generator";              ///< 定长 ASCII，可选
inline constexpr std::string_view kAttrCellTypeEncoding = "CellTypeEncoding";///< 定长 ASCII，单元码制式
inline constexpr std::string_view kAttrFixedCellSize = "FixedCellSize";      ///< int64 标量，可选（定长网格快速路径）
inline constexpr std::string_view kAttrDimensions = "Dimensions";            ///< int64[3]，仅结构化网格

// =============================================================================
// 4. 数据集名（几何 / 拓扑）
// =============================================================================

inline constexpr std::string_view kDatasetPoints = "Points";                              ///< f32 (n,3)
inline constexpr std::string_view kDatasetConnectivity = "Connectivity";                  ///< i32 (nconn)
inline constexpr std::string_view kDatasetOffsets = "Offsets";                            ///< i64 (ncells+1)
inline constexpr std::string_view kDatasetTypes = "Types";                                ///< u8  (ncells)
inline constexpr std::string_view kDatasetNumberOfPoints = "NumberOfPoints";              ///< i64 (1)
inline constexpr std::string_view kDatasetNumberOfCells = "NumberOfCells";                ///< i64 (1)
inline constexpr std::string_view kDatasetNumberOfConnectivityIds = "NumberOfConnectivityIds"; ///< i64 (1)

/// `Version` 在文件里是**根组下的 dataset**（i64[2]），而不是 attribute。
///
/// 这一点 P0 的 h5dump 基线（kia/vtkhdf/append_slice.txt:20）与 VTKHDF 规范一致：
/// `DATASET /VTKHDF/Version [H5T_STD_I64LE] SIMPLE { ( 2 ) / ( 2 ) } = 2, 6`。
///
/// 注意它与 `kAttrVersion` 同名但**载体不同**——使用 `kAttrVersion` 去建 dataset
/// 是能编译的（都只是字符串），所以这里刻意给出独立的常量名以免混用。
inline constexpr std::string_view kDatasetVersion = "Version";

/// 非结构族（SurfaceMesh / VolumeMesh / UnstructuredMesh）的必填拓扑数据集集合。
inline constexpr std::array<std::string_view, 6> kUnstructuredTopologyDatasets = {
    kDatasetConnectivity,
    kDatasetOffsets,
    kDatasetTypes,
    kDatasetNumberOfPoints,
    kDatasetNumberOfCells,
    kDatasetNumberOfConnectivityIds,
};

/// 计数组集合（用于"读取侧必须交叉校验"的实现）。
inline constexpr std::array<std::string_view, 3> kCountDatasets = {
    kDatasetNumberOfPoints,
    kDatasetNumberOfCells,
    kDatasetNumberOfConnectivityIds,
};

// =============================================================================
// 5. 属性数据集上的 attribute 名（§3.5 属性约定）
// =============================================================================

inline constexpr std::string_view kAttrAttributeType = "AttributeType";     ///< 定长 ASCII，见 kAttributeTypeTokens
inline constexpr std::string_view kAttrAttachment = "attachment";           ///< 定长 ASCII，见 kAttachmentTokens
inline constexpr std::string_view kAttrRange = "range";                     ///< float64[2] 或 float64[2*(1+components)]
inline constexpr std::string_view kAttrIGAttributeType = "IGAttributeType"; ///< int64 标量，iGame 枚举原值（保真冗余）
inline constexpr std::string_view kAttrIGAttachmentType = "IGAttachmentType"; ///< int64 标量，iGame 枚举原值

/// 组级"激活属性"标记（对齐 VTKHDF 的 PointData/Scalars 写法）。
inline constexpr std::array<std::string_view, 5> kGroupActiveMarkers = {
    "Scalars", "Vectors", "Normals", "TCoords", "Tensors",
};

// =============================================================================
// 6. Type 取值（VTKHDF 同名值，仅为降低外部读者的猜测成本 —— §1.6 第 1 条）
// =============================================================================

inline constexpr std::string_view kTypePolyData = "PolyData";
inline constexpr std::string_view kTypeUnstructuredGrid = "UnstructuredGrid";
inline constexpr std::string_view kTypeStructuredGrid = "StructuredGrid";

// 注意：以下 VTKHDF 类型**刻意不定义**（§1.4 原则 1，iGameVis 无此表达能力）：
//   ImageData、RectilinearGrid、HyperTreeGrid、OverlappingAMR、
//   PartitionedDataSetCollection、MultiBlockDataSet
// 若将来 iGame 补齐了对应能力（见 kia/未来任务.md F16），再在此处新增 —— 这是"跟随"而非"预留"。

// =============================================================================
// 7. IGVDataType 取值（iGame 原名，用于 Type 粒度不足时的下钻 —— §3.2）
// =============================================================================

/// iGame 侧的网格类型名。读取时**优先**取此值，缺失则回落到 Type 推导（向前兼容）。
inline constexpr std::string_view kIGVDataTypePointSet = "PointSet";
inline constexpr std::string_view kIGVDataTypeSurfaceMesh = "SurfaceMesh";
inline constexpr std::string_view kIGVDataTypeVolumeMesh = "VolumeMesh";
inline constexpr std::string_view kIGVDataTypeUnstructuredMesh = "UnstructuredMesh";
inline constexpr std::string_view kIGVDataTypeStructuredMesh = "StructuredMesh";

// =============================================================================
// 8. CellTypeEncoding 取值
// =============================================================================

/// 单元码制式：iGame 的 IGCellType 数值（见 iGameCellType.h）。
/// 与 VTK 标准码在常用单元上逐个一致，但 IG_FACE / IG_VOLUME / IG_POLYHEDRON
/// 不一致 —— 因此必须显式声明，不能靠猜。详见 iGameIGVCellType.h。
inline constexpr std::string_view kCellTypeEncodingIGCellType = "IGCellType";

// =============================================================================
// 9. AttributeType 取值（用词取自 VTKHDF 的 vtkDataSetAttributes，映射 iGame AttributeType）
// =============================================================================

inline constexpr std::string_view kAttributeTypeScalars = "Scalars";   ///< IG_SCALAR
inline constexpr std::string_view kAttributeTypeVectors = "Vectors";   ///< IG_VECTOR
inline constexpr std::string_view kAttributeTypeNormals = "Normals";   ///< IG_NORMAL
inline constexpr std::string_view kAttributeTypeTCoords = "TCoords";   ///< IG_TCOORD
inline constexpr std::string_view kAttributeTypeTensors = "Tensors";   ///< IG_TENSOR
inline constexpr std::string_view kAttributeTypeRGB = "RGB";           ///< IG_RGB（VTKHDF 无对应，IGV 自主命名）

// =============================================================================
// 10. attachment 取值（对应 iGame 的 AttributeWithElementType）
// =============================================================================

inline constexpr std::string_view kAttachmentPoint = "point";         ///< IG_POINT
inline constexpr std::string_view kAttachmentCell = "cell";           ///< IG_CELL
inline constexpr std::string_view kAttachmentEdge = "edge";           ///< IG_EDGE
inline constexpr std::string_view kAttachmentPointBox = "point_box";  ///< IG_POINT_BOX
inline constexpr std::string_view kAttachmentCellBox = "cell_box";    ///< IG_CELL_BOX
inline constexpr std::string_view kAttachmentChange = "change";       ///< IG_CHANGE
inline constexpr std::string_view kAttachmentDragPoint = "dragpoint"; ///< IG_DRAGPOINT
inline constexpr std::string_view kAttachmentMidPoint = "mid_point";  ///< IG_MID_POINT

// =============================================================================
// 11. 尺寸 / 约束常量（规范层面的硬约束，非性能调优）
// =============================================================================

/// 点坐标分量数。固定 3（§5.1 步骤 1.4）。
inline constexpr int kPointsComponents = 3;

/// 结构化网格 Dimensions 的元素个数。
inline constexpr int kDimensionsCount = 3;

/// Bounds 数组长度 [xmin,xmax,ymin,ymax,zmin,zmax]。
inline constexpr int kBoundsCount = 6;

/// 单元类型码的取值范围上界（u8 存储）。见 iGameIGVCellType.h 的值域核对。
inline constexpr int kTypesDtypeBytes = 1;

// =============================================================================
// 11b. IGV dtype 标识（§3.3 / §3.5）
// =============================================================================
//
// 这个枚举刻意放在本文件（纯 C++、无 HDF5）而不是 iGameIGVHdf5Util.h：
//   `IGVReader` 的头文件需要按 dtype 分派暂存缓冲，而该头文件**不得**引入
//   `#include <hdf5.h>`（§13 第 7 条）。把 dtype 与"到 hid_t 的映射"分开，
//   就能让公共头只依赖前者。
//
// 到 `hid_t` 的映射（FileTypeOf / NativeTypeOf / DTypeFromFileType）留在
// iGameIGVHdf5Util.h，因为它们必须看见 HDF5 的类型宏。

/// 规范要求的数据类型标识。与 §3.5 的 IGV dtype 表一一对应。
enum class DType {
    Unknown = 0,
    F32,   ///< H5T_IEEE_F32LE  Points / FloatArray
    F64,   ///< H5T_IEEE_F64LE  DoubleArray
    U8,    ///< H5T_STD_U8LE    UnsignedCharArray / Types
    I32,   ///< H5T_STD_I32LE   IdArray -> Connectivity
    I64,   ///< H5T_STD_I64LE   Offsets / NumberOf* / Version / Dimensions
};

[[nodiscard]] inline constexpr const char* DTypeName(DType t) noexcept {
    switch (t) {
        case DType::F32: return "f32(H5T_IEEE_F32LE)";
        case DType::F64: return "f64(H5T_IEEE_F64LE)";
        case DType::U8:  return "u8(H5T_STD_U8LE)";
        case DType::I32: return "i32(H5T_STD_I32LE)";
        case DType::I64: return "i64(H5T_STD_I64LE)";
        default:         return "unknown";
    }
}

[[nodiscard]] inline constexpr size_t SizeOfDType(DType t) noexcept {
    switch (t) {
        case DType::F32: return 4;
        case DType::F64: return 8;
        case DType::U8:  return 1;
        case DType::I32: return 4;
        case DType::I64: return 8;
        default:         return 0;
    }
}

// 注意：chunk 尺寸与压缩级别**不属于格式约束**，而是写侧的调优策略
// （见 kia/igv分阶段实现方案.md §5.3，由 P7 实测回调）。因此不在此处定义。

// =============================================================================
// 12. 字符串查找辅助（写侧校验 / 读侧反查共用，避免递归包含 iGameType.h）
// =============================================================================

/// Type 取值是否为本实现支持的合法值。
[[nodiscard]] inline constexpr bool IsSupportedTypeToken(std::string_view token) noexcept {
    return token == kTypePolyData || token == kTypeUnstructuredGrid || token == kTypeStructuredGrid;
}

/// IGVDataType 取值是否为本实现支持的合法值。
[[nodiscard]] inline constexpr bool IsSupportedIGVDataTypeToken(std::string_view token) noexcept {
    return token == kIGVDataTypePointSet || token == kIGVDataTypeSurfaceMesh ||
           token == kIGVDataTypeVolumeMesh || token == kIGVDataTypeUnstructuredMesh ||
           token == kIGVDataTypeStructuredMesh;
}

/// attribute 分组名是否合法。
[[nodiscard]] inline constexpr bool IsAttributeGroupName(std::string_view name) noexcept {
    return name == kGroupPointData || name == kGroupCellData || name == kGroupFieldData;
}

/// 是否为多块子组名（仅用于识别，见 §12.2）。
[[nodiscard]] inline constexpr bool IsBlockGroupName(std::string_view name) noexcept {
    return name.size() > kBlockGroupPrefix.size() &&
           name.compare(0, kBlockGroupPrefix.size(), kBlockGroupPrefix) == 0;
}

/// attachment 取值是否合法。
[[nodiscard]] inline constexpr bool IsSupportedAttachmentToken(std::string_view token) noexcept {
    return token == kAttachmentPoint || token == kAttachmentCell || token == kAttachmentEdge ||
           token == kAttachmentPointBox || token == kAttachmentCellBox ||
           token == kAttachmentChange || token == kAttachmentDragPoint ||
           token == kAttachmentMidPoint;
}

/// AttributeType 取值是否合法。
[[nodiscard]] inline constexpr bool IsSupportedAttributeTypeToken(std::string_view token) noexcept {
    return token == kAttributeTypeScalars || token == kAttributeTypeVectors ||
           token == kAttributeTypeNormals || token == kAttributeTypeTCoords ||
           token == kAttributeTypeTensors || token == kAttributeTypeRGB;
}

// =============================================================================
// 13. 写侧选项与读取侧返回结构
// =============================================================================
//
// 它们放在本文件（而非 writer/reader 的头文件）有两个原因：
//   1. 纯 C++ 数据，不依赖 HDF5，可被 WASM 侧与测试共用（§4.2 对本文件的定位）。
//   2. IGVWriter 与 IGVMetadataReader 都要用 IGVMetadata（P1 写盘后立即回读校验、
//      P3 快路径返回），放在任一侧的头里都会造成无谓的相互包含。
//
// 数值口径与 §5.3 / §7.1 一致。

/// 写盘方式。
enum class IGVWriteMode {
    /**
     * 内存镜像：HDF5 core VFD 在内存里生成完整 file image，再交给
     * `FileWriter::SaveBufferDataToFile()` 落盘。
     *
     * 这是默认值，因为它复用了基类这套已被三个平台验证过的落盘实现
     * （§4.1 支点 1），且天然支持「写出到内存」而不只是文件。
     * 代价是峰值内存 ≈ 文件大小（R2），大文件需改用 DirectPath。
     */
    MemoryImage = 0,

    /**
     * 直写目标路径：`H5Fcreate(path)` 直接写盘，绕开内存镜像。
     *
     * 用于规避 R2（core VFD 在 >2GB 时 OOM）。此模式下
     * `FileWriter::SaveBufferDataToFile()` 不应再落一次盘 —— 实现上把
     * `GenerateBuffers()` 的产物体积记为 0，使落盘成为无操作。
     * P7-4 会实测该路径并定稿自动切换阈值（§9 P7）。
     */
    DirectPath = 1,
};

/// 压缩策略（A4 / A14）。
enum class IGVCompression {
    /// 逐数组 gzip(deflate level 1) + chunk；小 dataset（< kChunkMinBytes）自动不压缩。
    GzipDefault = 0,
    /// 显式关闭压缩，且不 chunk。
    None = 1,
};

/// 写侧可调参数。默认值即规范默认（A4：逐数组可配，默认 gzip(1) + chunk）。
struct IGVWriteOptions {
    IGVWriteMode writeMode{IGVWriteMode::MemoryImage};
    IGVCompression compression{IGVCompression::GzipDefault};

    /// `Generator` attribute 的取值。留空则不写该 attribute（它是可选项）。
    std::string generator{"iGameVis IGVWriter"};

    /// 若为 true，则**不做** P1 内部的「写完立即回读校验计数组」自检（§5.2）。
    ///
    /// 默认 false（即做校验）—— §5.2 明确要求计数组必须来自"回读实际落盘维度"，
    /// 自检是这条要求的可执行证据。只有性能敏感场景才应关闭。
    bool skipCountReadbackCheck{false};
};

/// 一个属性数组的元数据（§7.1）。
struct IGVArrayInfo {
    std::string name;
    std::string attributeType;  ///< "Scalars" / "Vectors" / ...
    std::string attachment;     ///< "point" / "cell" / ...
    int64_t count{0};           ///< 元素数（不是标量数）
    int components{1};          ///< 每元素分量数
    int valueSize{4};           ///< 4 / 8 / 1
    std::vector<double> range;  ///< 预计算范围，长度 2 或 2*(1+components)
};

/// 单块 IGV 文件的元数据（§7.1）。
struct IGVMetadata {
    std::string type;         ///< VTKHDF 对齐值（PolyData / UnstructuredGrid / StructuredGrid）
    std::string igvDataType;  ///< iGame 原名（PointSet / SurfaceMesh / ...）

    int64_t versionMajor{1};
    int64_t versionMinor{0};

    /// [xmin,xmax,ymin,ymax,zmin,zmax]。**未写出 Bounds attribute 时全 0** ——
    /// 由 `hasBounds` 区分"真的是全 0 包围盒"与"没写"（§7.1）。
    double bounds[6]{0, 0, 0, 0, 0, 0};
    bool hasBounds{false};

    int64_t dimensions[3]{0, 0, 0};
    bool hasDimensions{false};

    int64_t pointCount{0};

    int64_t cellCount{0};
    bool hasCellCount{false};

    int64_t connectivityIdCount{0};
    bool hasConnectivityIdCount{false};

    int64_t fixedCellSize{-1};  ///< -1 == 无 FixedCellSize attribute

    double timeValue{0.0};
    bool hasTimeValue{false};
    int64_t timeStep{0};
    bool hasTimeStep{false};

    std::string generator;
    std::vector<IGVArrayInfo> arrays;
};

} // namespace igv

IGAME_NAMESPACE_END

#endif // iGameIGVFormat_h
