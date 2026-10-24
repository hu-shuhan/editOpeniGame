#ifndef iGameIGVFormat_h
#define iGameIGVFormat_h

/**
 * @file  iGameIGVFormat.h
 * @brief IGV file layout constants: group/dataset/attribute names, type tokens, dtypes.
 *
 * This header is pure C++ and does not include HDF5, so writer, reader, metadata reader,
 * and future Web-side code share the same vocabulary.
 *
 * All literal names used on disk are defined here. Do not scatter group/dataset/attribute
 * strings across .cpp files.
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
inline constexpr std::string_view kBlockGroupPrefix = "Block_";

///
/// IGV 本期既不写也不读该组，也没有任何"Steps 的字段定义"。
/// 它存在的唯一理由与 `kBlockGroupPrefix` 相同 —— P6 的目标是**不静默丢数据**：
/// 一个带 `Steps` 组的文件含多个时间步，若我们只把第 0 步当整体返回，
/// 用户会以为拿到了全部数据。因此宁可在读取入口就拒绝并说清原因。
inline constexpr std::string_view kTimeSeriesGroup = "Steps";

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

///
/// 语义（只在 `TopologyIndex = "Secondary"` 时出现，见 `kAttrTopologyIndex`）：
///   `Connectivity`     = **体 → 面** 的面 id（而不是点 id）
///   `Offsets`          = 体 → 面的偏移（ncells+1）
///   `FaceConnectivity` = **面 → 点** 的点 id
///   `FaceOffsets`      = 面 → 点的偏移（nfaces+1）
///
/// 为什么不像 VTK legacy 那样把每个面的点重复写进每个体：那会丢掉"共享面"
/// 的身份（两个相邻单元引用同一个面），并让文件体积按面数×相邻单元数膨胀。
/// iGame 的多面体本来就是二级索引（`VolumeMesh::GetFaces()`），IGV 直接存它。
inline constexpr std::string_view kDatasetFaceConnectivity = "FaceConnectivity"; ///< i32 (nFaceIds)
inline constexpr std::string_view kDatasetFaceOffsets = "FaceOffsets";           ///< i64 (nFaces+1)
inline constexpr std::string_view kDatasetNumberOfFaces = "NumberOfFaces";       ///< i64 (1)

/// 根组 attribute：拓扑索引形态（**只在多面体时写出**，缺省即 `Primary`）。
///
/// 存在的理由：`Connectivity` 在两种模式下含义不同（点 id vs 面 id）。这不是
/// 可以"看 Types 猜"的东西 —— 写清声明、读侧校验，与 `CellTypeEncoding`（A3）
/// 是同一套思路：**约定必须写在文件里，而不是让人猜**。
inline constexpr std::string_view kAttrTopologyIndex = "TopologyIndex";

/// `TopologyIndex` 取值。
inline constexpr std::string_view kTopologyIndexPrimary = "Primary";     ///< 缺省：Connectivity 是单元→点
inline constexpr std::string_view kTopologyIndexSecondary = "Secondary"; ///< 多面体：Connectivity 是体→面

inline constexpr std::array<std::string_view, 3> kPolyhedronTopologyDatasets = {
    kDatasetFaceConnectivity,
    kDatasetFaceOffsets,
    kDatasetNumberOfFaces,
};

/// 计数组集合（用于"读取侧必须交叉校验"的实现）。
inline constexpr std::array<std::string_view, 3> kCountDatasets = {
    kDatasetNumberOfPoints,
    kDatasetNumberOfCells,
    kDatasetNumberOfConnectivityIds,
};

// =============================================================================
// =============================================================================

inline constexpr std::string_view kAttrAttributeType = "AttributeType";     ///< 定长 ASCII，见 kAttributeTypeTokens
inline constexpr std::string_view kAttrAttachment = "attachment";           ///< 定长 ASCII，见 kAttachmentTokens
inline constexpr std::string_view kAttrRange = "range";                     ///< float64[2] 或 float64[2*(1+components)]
inline constexpr std::string_view kAttrIGAttributeType = "IGAttributeType"; ///< int64 标量，iGame 枚举原值（保真冗余）
inline constexpr std::string_view kAttrIGAttachmentType = "IGAttachmentType"; ///< int64 标量，iGame 枚举原值

/// **数据类型显式声明**（1.x）。取值见 `DTypeToken()`：`Int8`/`UInt16`/`Float64`/…
///
/// 为什么在 HDF5 类型之外还要再写一遍：HDF5 的类型是**自描述**的，但它只告诉自己
/// 有多宽、是整型还是浮点，不告诉读者"这是不是 VTK 的 `long`"这类**语义**。
/// 把定宽名显式写出来，读取方（包括外部实现者）不必做任何平台推断；
inline constexpr std::string_view kAttrDataType = "DataType";

/// 数据类型位宽（i64 标量，8/16/32/64）。
///
/// 与 `DataType` 冗余，但两者是**独立事实**：写侧两者不一致 → 文件自相矛盾；
inline constexpr std::string_view kAttrDataTypeBits = "DataTypeBits";

/// 组级"激活属性"标记（对齐 VTKHDF 的 PointData/Scalars 写法）。
inline constexpr std::array<std::string_view, 5> kGroupActiveMarkers = {
    "Scalars", "Vectors", "Normals", "TCoords", "Tensors",
};

// =============================================================================
// =============================================================================

inline constexpr std::string_view kTypePolyData = "PolyData";
inline constexpr std::string_view kTypeUnstructuredGrid = "UnstructuredGrid";
inline constexpr std::string_view kTypeStructuredGrid = "StructuredGrid";

//   ImageData、RectilinearGrid、HyperTreeGrid、OverlappingAMR、
//   PartitionedDataSetCollection、MultiBlockDataSet

// =============================================================================
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
// 11. 尺寸 / 约束常量
// =============================================================================

inline constexpr int kPointsComponents = 3;

/// 结构化网格 Dimensions 的元素个数。
inline constexpr int kDimensionsCount = 3;

/// Bounds 数组长度 [xmin,xmax,ymin,ymax,zmin,zmax]。
inline constexpr int kBoundsCount = 6;

/// 单元类型码的取值范围上界（u8 存储）。见 iGameIGVCellType.h 的值域核对。
inline constexpr int kTypesDtypeBytes = 1;

// =============================================================================
// =============================================================================
//
// 这个枚举刻意放在本文件（纯 C++、无 HDF5）而不是 iGameIGVHdf5Util.h：
//   `IGVReader` 的头文件需要按 dtype 分派暂存缓冲，而该头文件**不得**引入
//   就能让公共头只依赖前者。
//
// 到 `hid_t` 的映射（FileTypeOf / NativeTypeOf / DTypeFromFileType）留在
// iGameIGVHdf5Util.h，因为它们必须看见 HDF5 的类型宏。

///
/// **为什么这里有 10 种、而不是只留 f32/f64/u8**：
/// IGV 是交换格式，属性数组必须与来源**等宽无损**。VTK 的标量类型共 12 个
/// （`char`/`short`/`int`/`long long`、各自的无符号版、`float`/`double`，外加
/// `bit` 与 `id type`），其中 `long` 的宽度**依赖平台**（LP64=8 / LLP64=4）。
/// 把 VTK 的类型名直接写进交换格式，就会得到"同一份文件在两台机器上解析成不同
/// 字节宽"的经典事故 —— 所以 IGV 只承认**定宽**类型（与 NumPy 的
/// int8…uint64 / float32 / float64 一一对应），并把宽度**显式写进文件**
///
/// 枚举值只在编译期使用，不写进文件（文件里存的是 HDF5 类型 + 上面两个 attribute）。
enum class DType {
    Unknown = 0,
    // ---- 1.0 起的五类：几何/拓扑 dataset 与最常用的属性 ----
    F32,   ///< H5T_IEEE_F32LE  Points / FloatArray          float   (32 bit)
    F64,   ///< H5T_IEEE_F64LE  DoubleArray                  double  (64 bit)
    U8,    ///< H5T_STD_U8LE    UnsignedCharArray / Types    uint8   (8 bit)
    I32,   ///< H5T_STD_I32LE   Connectivity / IntArray      int32   (32 bit)
    I64,   ///< H5T_STD_I64LE   Offsets / 计数组 / LongLong  int64   (64 bit)
    // ---- 1.x 补齐：属性数组现在可以无损承载 VTK 的全部标量类型 ----
    I8,    ///< H5T_STD_I8LE    CharArray                    int8    (8 bit)
    I16,   ///< H5T_STD_I16LE   ShortArray                   int16   (16 bit)
    U16,   ///< H5T_STD_U16LE   UnsignedShortArray           uint16  (16 bit)
    U32,   ///< H5T_STD_U32LE   UnsignedIntArray             uint32  (32 bit)
    U64,   ///< H5T_STD_U64LE   UnsignedLongLongArray        uint64  (64 bit)
};

[[nodiscard]] inline constexpr const char* DTypeName(DType t) noexcept {
    switch (t) {
        case DType::F32: return "f32(H5T_IEEE_F32LE)";
        case DType::F64: return "f64(H5T_IEEE_F64LE)";
        case DType::U8:  return "u8(H5T_STD_U8LE)";
        case DType::I32: return "i32(H5T_STD_I32LE)";
        case DType::I64: return "i64(H5T_STD_I64LE)";
        case DType::I8:  return "i8(H5T_STD_I8LE)";
        case DType::I16: return "i16(H5T_STD_I16LE)";
        case DType::U16: return "u16(H5T_STD_U16LE)";
        case DType::U32: return "u32(H5T_STD_U32LE)";
        case DType::U64: return "u64(H5T_STD_U64LE)";
        default:         return "unknown";
    }
}

[[nodiscard]] inline constexpr int DTypeBits(DType t) noexcept {
    switch (t) {
        case DType::U8:
        case DType::I8:  return 8;
        case DType::I16:
        case DType::U16: return 16;
        case DType::F32:
        case DType::I32:
        case DType::U32: return 32;
        case DType::F64:
        case DType::I64:
        case DType::U64: return 64;
        default:         return 0;
    }
}

/// 定宽类型名 —— 也就是 `DataType` attribute 的取值。
///
/// 刻意**不用** VTK 的 `int` / `long` / `double`：那些名字会诱导读者按平台宽度去
/// 解释（用户的原始诉求正是"避免 vtk legacy 的语义模糊"）。这里名字本身就含宽度，
/// 与 `DataTypeBits` 互为交叉校验。
[[nodiscard]] inline constexpr std::string_view DTypeToken(DType t) noexcept {
    switch (t) {
        case DType::I8:  return "Int8";
        case DType::I16: return "Int16";
        case DType::I32: return "Int32";
        case DType::I64: return "Int64";
        case DType::U8:  return "UInt8";
        case DType::U16: return "UInt16";
        case DType::U32: return "UInt32";
        case DType::U64: return "UInt64";
        case DType::F32: return "Float32";
        case DType::F64: return "Float64";
        default:         return {};
    }
}

/// `DataType` 取值 -> DType。未知返回 `DType::Unknown`（调用方必须拒绝，不得猜）。
[[nodiscard]] inline constexpr DType ParseDTypeToken(std::string_view token) noexcept {
    if (token == "Int8") { return DType::I8; }
    if (token == "Int16") { return DType::I16; }
    if (token == "Int32") { return DType::I32; }
    if (token == "Int64") { return DType::I64; }
    if (token == "UInt8") { return DType::U8; }
    if (token == "UInt16") { return DType::U16; }
    if (token == "UInt32") { return DType::U32; }
    if (token == "UInt64") { return DType::U64; }
    if (token == "Float32") { return DType::F32; }
    if (token == "Float64") { return DType::F64; }
    return DType::Unknown;
}

/// 面板 / 日志用的标签：`Float64(64bit)`。
[[nodiscard]] inline std::string DTypeLabel(DType t) {
    const std::string_view token = DTypeToken(t);
    if (token.empty()) { return "unknown"; }
    return std::string(token) + "(" + std::to_string(DTypeBits(t)) + "bit)";
}

/// 对应的 VTK / NumPy 名字（**只**用于 GUI tooltip 与文档，不写进文件）。
[[nodiscard]] inline constexpr std::string_view DTypeVTKName(DType t) noexcept {
    switch (t) {
        case DType::I8:  return "VTK_CHAR / signed char / int8";
        case DType::I16: return "VTK_SHORT / short / int16";
        case DType::I32: return "VTK_INT / int / int32";
        case DType::I64: return "VTK_LONG_LONG / long long / int64";
        case DType::U8:  return "VTK_UNSIGNED_CHAR / unsigned char / uint8";
        case DType::U16: return "VTK_UNSIGNED_SHORT / unsigned short / uint16";
        case DType::U32: return "VTK_UNSIGNED_INT / unsigned int / uint32";
        case DType::U64: return "VTK_UNSIGNED_LONG_LONG / unsigned long long / uint64";
        case DType::F32: return "VTK_FLOAT / float / float32";
        case DType::F64: return "VTK_DOUBLE / double / float64";
        default:         return "unknown";
    }
}

[[nodiscard]] inline constexpr bool IsFloatDType(DType t) noexcept {
    return t == DType::F32 || t == DType::F64;
}

[[nodiscard]] inline constexpr bool IsIntegerDType(DType t) noexcept {
    switch (t) {
        case DType::I8:
        case DType::I16:
        case DType::I32:
        case DType::I64:
        case DType::U8:
        case DType::U16:
        case DType::U32:
        case DType::U64: return true;
        default:         return false;
    }
}

[[nodiscard]] inline constexpr size_t SizeOfDType(DType t) noexcept {
    return static_cast<size_t>(DTypeBits(t)) / 8u;
}

// 注意：chunk 尺寸与压缩级别**不属于格式约束**，而是写侧的调优策略

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

[[nodiscard]] inline constexpr bool IsBlockGroupName(std::string_view name) noexcept {
    return name.size() > kBlockGroupPrefix.size() &&
           name.compare(0, kBlockGroupPrefix.size(), kBlockGroupPrefix) == 0;
}

///
/// 为什么不直接返回 bool：`Block_*` 与时变是**两件不同的事**，
/// 报错必须说清是哪一件（用户据此才知道该换哪种导出方式）。
enum class UnsupportedContainer {
    None = 0,     ///< 不是已知的未实现容器（含三个属性组、未知的普通组）
    MultiBlock,
    TimeSeries,
};

/// 按组名判别它是不是"本实现明确不支持、必须拒绝"的数据容器。
///
/// 刻意**只**认这两个已知名字，而不是"凡未知组皆拒绝"：
/// 所以读侧不能对未知组一概拒绝；这里只对"已经被规范命名、且确实携带整块数据"
/// 的两个容器做拒绝，从而在不破坏前向兼容的前提下消除静默丢数据。
[[nodiscard]] inline constexpr UnsupportedContainer ClassifyUnsupportedContainer(
        std::string_view name) noexcept {
    if (IsBlockGroupName(name)) { return UnsupportedContainer::MultiBlock; }
    if (name == kTimeSeriesGroup) { return UnsupportedContainer::TimeSeries; }
    return UnsupportedContainer::None;
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
//   2. IGVWriter 与 IGVMetadataReader 都要用 IGVMetadata（P1 写盘后立即回读校验、
//      P3 快路径返回），放在任一侧的头里都会造成无谓的相互包含。
//

/// 写盘方式。
enum class IGVWriteMode {
    /**
     * 内存镜像：HDF5 core VFD 在内存里生成完整 file image，再交给
     * `FileWriter::SaveBufferDataToFile()` 落盘。
     *
     * 这是默认值，因为它复用了基类这套已被三个平台验证过的落盘实现
     * 代价是峰值内存 ≈ 文件大小（R2），大文件需改用 DirectPath。
     */
    MemoryImage = 0,

    /**
     * 直写目标路径：`H5Fcreate(path)` 直接写盘，绕开内存镜像。
     *
     * 用于规避 R2（core VFD 在 >2GB 时 OOM）。此模式下
     * `FileWriter::SaveBufferDataToFile()` 不应再落一次盘 —— 实现上把
     * `GenerateBuffers()` 的产物体积记为 0，使落盘成为无操作。
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

    ///
    /// 自检是这条要求的可执行证据。只有性能敏感场景才应关闭。
    bool skipCountReadbackCheck{false};
};

struct IGVArrayInfo {
    std::string name;
    std::string attributeType;  ///< "Scalars" / "Vectors" / ...
    std::string attachment;     ///< "point" / "cell" / ...
    int64_t count{0};           ///< 元素数（不是标量数）
    int components{1};          ///< 每元素分量数
    int valueSize{4};           ///< 每元素字节数：1 / 2 / 4 / 8
    /// 定宽数据类型名（`DataType` attribute 的取值；1.0 的老文件缺失时由 HDF5 类型推断）。
    /// 面板展示 `Float64(64bit)` 就靠这两个字段 —— 名字与位宽互为校验。
    std::string dTypeToken;     ///< "Int8" / "UInt16" / "Float64" / ...
    int dTypeBits{0};           ///< 8 / 16 / 32 / 64
    std::vector<double> range;  ///< 预计算范围，长度 2 或 2*(1+components)
};

struct IGVMetadata {
    std::string type;         ///< VTKHDF 对齐值（PolyData / UnstructuredGrid / StructuredGrid）
    std::string igvDataType;  ///< iGame 原名（PointSet / SurfaceMesh / ...）

    int64_t versionMajor{1};
    int64_t versionMinor{0};

    /// [xmin,xmax,ymin,ymax,zmin,zmax]。**未写出 Bounds attribute 时全 0** ——
    double bounds[6]{0, 0, 0, 0, 0, 0};
    bool hasBounds{false};

    int64_t dimensions[3]{0, 0, 0};
    bool hasDimensions{false};

    int64_t pointCount{0};

    int64_t cellCount{0};
    bool hasCellCount{false};

    int64_t connectivityIdCount{0};
    bool hasConnectivityIdCount{false};

    /// 多面体（二级索引）时才有：面数（由 `FaceOffsets` 的 dataspace 推出，不读数据）。
    int64_t faceCount{0};
    bool hasFaceCount{false};

    /// `TopologyIndex` attribute 的原值；空串表示文件没写（= `Primary`）。
    std::string topologyIndex;

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
