#include "iGameIGVReader.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <utility>

#include "Log/iGameLogger.h"
#include "MeshCodec/DecodeAdapter/iGameMeshDecodeAdapterToDataObject.h"
#include "MeshCodec/Utils/iGameCodecPayload.h"

#include "iGameIGVCellType.h"
#include "iGameIGVHdf5Util.h"

// hid_t 的宽度假设校验。
//
// iGameIGVReader.h 用 `int64_t` 传递 HDF5 句柄（因为公共头不得 include
// hdf5.h，而 HDF5 的 hid_t 是 `typedef long long`，不能在头里前向 typedef 成
// int64_t 而不产生"两个不同 typedef"的问题）。宽度假设必须钉死在编译期：
// 一旦某平台不符，这里立刻失败，而不是静默用错类型。
static_assert(sizeof(hid_t) == sizeof(int64_t),
              "iGame::IGVReader passes HDF5 handles as int64_t and assumes hid_t is 64-bit;"
              " if HDF5 changes the width of hid_t, the note in iGameIGVReader.h must be updated too");

IGAME_NAMESPACE_BEGIN

namespace {

using namespace iGame::igv;

// =============================================================================
// token -> iGame 枚举
// =============================================================================

IGenum AttachmentFromToken(std::string_view token) {
    if (token == kAttachmentPoint)     { return IG_POINT; }
    if (token == kAttachmentCell)      { return IG_CELL; }
    if (token == kAttachmentEdge)      { return IG_EDGE; }
    if (token == kAttachmentPointBox)  { return IG_POINT_BOX; }
    if (token == kAttachmentCellBox)   { return IG_CELL_BOX; }
    if (token == kAttachmentChange)    { return IG_CHANGE; }
    if (token == kAttachmentDragPoint) { return IG_DRAGPOINT; }
    if (token == kAttachmentMidPoint)  { return IG_MID_POINT; }
    return IG_NONE;
}

IGenum AttributeTypeFromToken(std::string_view token) {
    if (token == kAttributeTypeScalars) { return IG_SCALAR; }
    if (token == kAttributeTypeVectors) { return IG_VECTOR; }
    if (token == kAttributeTypeNormals) { return IG_NORMAL; }
    if (token == kAttributeTypeTCoords) { return IG_TCOORD; }
    // 注意是 IG_TENSOR（单数）—— iGameType.h:81-91 的枚举里没有 IG_TENSORS。
    if (token == kAttributeTypeTensors) { return IG_TENSOR; }
    if (token == kAttributeTypeRGB)     { return IG_RGB; }
    return IG_NONE;
}

/// 分组名 -> 隐含的 attachment（dataset 缺 `attachment` attribute 时的兜底）。
IGenum AttachmentFromGroup(std::string_view groupName) {
    if (groupName == kGroupPointData) { return IG_POINT; }
    if (groupName == kGroupCellData)  { return IG_CELL; }
    // FieldData 无法从组名推断依附类型 —— 必须由文件显式声明。
    return IG_NONE;
}

bool MulWouldOverflow(int64_t a, int64_t b) {
    if (a == 0 || b == 0) { return false; }
    if (a < 0 || b < 0) { return true; }
    return a > (std::numeric_limits<int64_t>::max() / b);
}

/**
 *
 * 为什么需要它：`MulWouldOverflow()` 只能挡住"乘出来超过 int64"的荒谬维度，
 * 挡不住 `2^32` 这种**算得出来但显然不可能**的维度 —— 而后者会让
 * `std::vector::resize()` 在 `H5Dread` **之前**就去申请 TB 级内存，进程直接被
 *
 * 判据是**物理**的，不是拍脑袋的阈值：
 *   - IGV 的数据集要么不压缩（声明字节数 == 文件内已分配字节数），
 *   - 要么只走 gzip(deflate)。deflate 单流的物理压缩比上限约 1032:1，
 *     因此「解压后字节数 / 已分配字节数」超过 4096 就必然是坏元数据。
 *
 * 这条检查只读对象头与存储布局，**不读任何数据**（不增加 H5Dread 计数）。
 *
 * @param dataset      已打开的 dataset 句柄
 * @param label        报错用的可读名字（"Points" / "attribute 'pid'" …）
 * @param elementCount dataspace 声明的元素数
 * @param elementBytes 单个元素的字节数（dtype 宽度 × 分量数）
 * @param error        失败时的原因
 */
/// deflate 的物理压缩比上限约 1032:1，取 4096 留 4x 余量。
inline constexpr int64_t kMaxStorageExpansion = 4096;

bool CheckDeclaredStorage(hid_t dataset, std::string_view label, int64_t elementCount,
                          int64_t elementBytes, std::string& error) {
    if (elementCount <= 0 || elementBytes <= 0) { return true; }  // 空数组无需校验

    const std::string name(label);
    const hsize_t storage = H5Dget_storage_size(dataset);
    if (storage == 0) {
        ClearHdf5Errors();
        error = name + " declares " + std::to_string(elementCount) +
                " element(s) but the file allocates no storage for it (corrupt or hand-made file)";
        return false;
    }

    // elementCount * elementBytes 已在调用方做过整数溢出检查。
    const int64_t declaredBytes = elementCount * elementBytes;
    const int64_t maxBytes = static_cast<int64_t>(storage) * kMaxStorageExpansion;
    if (declaredBytes > maxBytes) {
        error = name + " declares " + std::to_string(declaredBytes) + " byte(s) (" +
                std::to_string(elementCount) + " elements x " + std::to_string(elementBytes) +
                " B) but the file only allocates " + std::to_string(storage) +
                " byte(s) (expansion ratio " + std::to_string(kMaxStorageExpansion) +
                "x is already the physical limit of deflate) - this dataspace is corrupt, "
                "rejecting before allocating";
        return false;
    }
    return true;
}

// =============================================================================
// =============================================================================

struct NameCollector {
    std::vector<std::string>* groups = nullptr;
    std::vector<std::string>* datasets = nullptr;
};

herr_t CollectNamesCallback(hid_t group, const char* name, const H5L_info_t* /*info*/,
                           void* opData) {
    auto* collector = static_cast<NameCollector*>(opData);
    if (collector == nullptr || name == nullptr) { return 0; }

    // H5Oget_info_by_name3 读的是对象头（对象类型属于链接信息），
    H5O_info2_t info{};
    if (H5Oget_info_by_name3(group, name, &info, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
        ClearHdf5Errors();
        return 0;  // 跳过取不到信息的孩子，不中断遍历
    }

    if (info.type == H5O_TYPE_GROUP) {
        if (collector->groups) { collector->groups->emplace_back(name); }
    } else if (info.type == H5O_TYPE_DATASET) {
        if (collector->datasets) { collector->datasets->emplace_back(name); }
    }
    return 0;
}

/// 枚举一个组下的 group / dataset 名。
bool ListGroupMembers(hid_t group, std::vector<std::string>* groups,
                     std::vector<std::string>* datasets) {
    NameCollector collector;
    collector.groups = groups;
    collector.datasets = datasets;
    if (H5Literate(group, H5_INDEX_NAME, H5_ITER_NATIVE, nullptr,
                   CollectNamesCallback, &collector) < 0) {
        ClearHdf5Errors();
        return false;
    }
    return true;
}

// =============================================================================
// HDF5 file image 的 ownership 回调（R13 的落实方式）
// =============================================================================
//
//   `FileReader` 的 `SetMemoryBuffer(data, size)` 语义是"**非拥有**，调用方负责
//   缓冲的生命周期"（`iGameFileReader.h:93-96`）。而 HDF5 的
//   `H5Pset_file_image(fapl, buf, len)` 签名是 `void*`（非 const），
//   并且**会复制**这个缓冲；`H5Fclose` 时是否再碰它取决于回调设置。
//
//   直接 `const_cast` 掉 `m_MemoryBuffer` 的 const 很脆（它本身是
//   `const char* const`，需要两步转换），而且把"谁在什么时候释放"这个
//   关键问题留成了隐含约定 —— 正是 R13 描述的"语义不一致"。
//
// 本实现的处置：**显式接管**，用 `H5Pset_file_image_callbacks` 把四个回调
// 全部写出来，语义完全可审计：
//
//   * 交给 HDF5 的 `udata` 是我们自己 malloc 的一小块头（缓冲指针 + 长度），
//     所以回调里随时知道边界；
//   * `image_memcpy` 只读调用方的缓冲（数据源是**我们**），从不写它；
//   * `image_free` 只释放 `image_malloc` 分配的内存（它把指针当作我们给的
//     udata 头来看，并能区分"是不是我们分配的"），**绝不** free 调用方的缓冲；
//   * `image_realloc` 同理。
//
//   结果是：调用方缓冲的生命周期要求与 `FileReader` 原有语义完全一致
//   —— 只需活过 `Execute()`，HDF5 不会在 `H5Fclose` 时释放它。

/// 传给 HDF5 的 `udata`：我们自己拥有的小头，记着调用方缓冲的边界。
struct MemoryImageOwner {
    const char* data{nullptr};
    size_t size{0};
};

void* ImageMalloc(size_t size, H5FD_file_image_op_t op, void* udata) {
    (void)op;
    (void)udata;
    // core VFD 会为工作副本申请内存；交给系统分配器，
    // 并在 image_free 里按"不是 owner 头"判定后 free 掉。
    return std::malloc(size);
}

void* ImageMemcpy(void* dest, const void* src, size_t size, H5FD_file_image_op_t op,
                  void* udata) {
    (void)op;
    (void)udata;
    return std::memcpy(dest, src, size);
}

void* ImageRealloc(void* mem, size_t size, H5FD_file_image_op_t op, void* udata) {
    auto* owner = static_cast<MemoryImageOwner*>(udata);
    if (mem == owner) {
        // 不允许 realloc 我们自己的 owner 头 —— 它的生命周期由 MemoryImageGuard 管。
        return nullptr;
    }
    (void)op;
    return std::realloc(mem, size);
}
herr_t ImageFree(void* mem, H5FD_file_image_op_t op, void* udata) {
    (void)op;
    (void)udata;
    // 关键（R13）：**不**释放调用方的缓冲，也不释放 owner 头本身
    // —— 我们的 owner 由 MemoryImageGuard 释放，HDF5 拿到的副本由 udata_free 释放。
    //
    // 这里可以无条件 free(mem)：`image_free` 只被 HDF5 用来释放它经
    // `image_malloc` 申请的那块**工作副本**。调用方缓冲从不会作为 ptr 传进来
    // —— HDF5 只是把它当作 `image_memcpy` 的**源**。
    // （必须用 memcmp 之外的方式区分"是不是我们的 owner"：owner 是
    //  `sizeof(MemoryImageOwner)` 的小块，而工作副本是 >= 文件大小的块，
    //  两者不可能相等；`mem == udata` 那条判断因此是多余且不可靠的，
    //  已删除。）
    if (mem != nullptr) { std::free(mem); }
    return 0;
}

/// `udata_copy`：把 owner 头复制一份交给 HDF5 拥有。
///
/// ⚠ 这两个回调**必须提供**：HDF5 的 `H5Pset_file_image_callbacks()` 有一条硬校验
/// ——「udata 非空时，udata_copy / udata_free 也必须设置」，否则直接失败并报
///
///     H5Pset_file_image_callbacks(): udata callbacks must be set if udata is set
///
/// 也就是说 **P1 的"内存路径"从来没有真正跑通过**：`H5Pset_file_image_callbacks`
/// 一直返回失败，V1.5 因此长期红着（而且被别的测试 bug 掩盖了）。
/// 这条是 V1.5 修好夹具之后才暴露出来的真实实现缺陷。
///
/// 语义澄清（与 R13 的关系）：HDF5 会复制 fapl，从而需要复制 `udata`。
/// 我们让它复制，并让 `udata_free` 释放那份副本；**调用方缓冲**依旧完全不受影响
/// —— 回调里没有任何一处会 free 或写 `owner->data`。
void* ImageUdataCopy(void* udata) {
    auto* src = static_cast<MemoryImageOwner*>(udata);
    if (src == nullptr) { return nullptr; }
    auto* copy = static_cast<MemoryImageOwner*>(std::malloc(sizeof(MemoryImageOwner)));
    if (copy == nullptr) { return nullptr; }
    *copy = *src;
    return copy;
}

herr_t ImageUdataFree(void* udata) {
    std::free(udata);       // 只释放我们自己 malloc 的 owner 副本
    return 0;
}

/// `MemoryImageOwner` 的 RAII 持有者：确保 HDF5 全部关闭后再释放 owner 头。
class MemoryImageGuard {
public:
    MemoryImageGuard() = default;
    ~MemoryImageGuard() { std::free(m_Owner); }

    MemoryImageGuard(const MemoryImageGuard&) = delete;
    MemoryImageGuard& operator=(const MemoryImageGuard&) = delete;

    bool Init(const char* data, size_t size) {
        m_Owner = static_cast<MemoryImageOwner*>(std::malloc(sizeof(MemoryImageOwner)));
        if (m_Owner == nullptr) { return false; }
        m_Owner->data = data;
        m_Owner->size = size;
        return true;
    }

    [[nodiscard]] MemoryImageOwner* Get() const { return m_Owner; }

private:
    MemoryImageOwner* m_Owner{nullptr};
};

/// 从"原始字节 + dtype"建出对应的 iGame 数组（10 类，**无降级**）。
template <class ArrayT, typename T>
ArrayObject::Pointer MakeTypedAttributeArray(const std::vector<unsigned char>& bytes,
                                            int components, const std::string& name) {
    const size_t elemBytes = sizeof(T) * static_cast<size_t>(std::max(components, 1));
    const size_t elements = (elemBytes == 0) ? 0 : bytes.size() / elemBytes;

    typename ArrayT::Pointer arr = ArrayT::New();
    arr->SetDimension(components);
    arr->SetName(name);
    arr->Resize(static_cast<IGsize>(elements));
    if (!bytes.empty()) { std::memcpy(arr->RawPointer(), bytes.data(), bytes.size()); }
    return arr;
}

/// 按 dtype 分派（与写侧的 StageAttributeData 一一对应）。
ArrayObject::Pointer MakeAttributeArray(const std::vector<unsigned char>& bytes, DType dtype,
                                        int components, const std::string& name) {
    switch (dtype) {
        case DType::F32: return MakeTypedAttributeArray<FloatArray, float>(bytes, components, name);
        case DType::F64: return MakeTypedAttributeArray<DoubleArray, double>(bytes, components, name);
        case DType::U8:  return MakeTypedAttributeArray<UnsignedCharArray, unsigned char>(bytes, components, name);
        case DType::I8:  return MakeTypedAttributeArray<CharArray, char>(bytes, components, name);
        case DType::I16: return MakeTypedAttributeArray<ShortArray, short>(bytes, components, name);
        case DType::U16: return MakeTypedAttributeArray<UnsignedShortArray, unsigned short>(bytes, components, name);
        case DType::I32: return MakeTypedAttributeArray<IntArray, int>(bytes, components, name);
        case DType::U32: return MakeTypedAttributeArray<UnsignedIntArray, unsigned int>(bytes, components, name);
        case DType::I64: return MakeTypedAttributeArray<LongLongArray, long long>(bytes, components, name);
        case DType::U64: return MakeTypedAttributeArray<UnsignedLongLongArray, unsigned long long>(bytes, components, name);
        default:         return nullptr;
    }
}

} // namespace

// =============================================================================
// token -> iGame 枚举（类成员转发）
// =============================================================================

IGenum IGVReader::MapAttachmentToIGenum(std::string_view token) {
    return AttachmentFromToken(token);
}

IGenum IGVReader::MapAttributeTypeToIGenum(std::string_view token) {
    return AttributeTypeFromToken(token);
}

IGenum IGVReader::MapIGVDataTypeToMeshType(std::string_view igvDataType,
                                          std::string_view type) {
    // —— 向前兼容：老文件无 IGVDataType 也能读。
    if (!igvDataType.empty()) {
        if (igvDataType == kIGVDataTypePointSet)         { return IG_POINT_SET; }
        if (igvDataType == kIGVDataTypeSurfaceMesh)      { return IG_SURFACE_MESH; }
        if (igvDataType == kIGVDataTypeVolumeMesh)       { return IG_VOLUME_MESH; }
        if (igvDataType == kIGVDataTypeUnstructuredMesh) { return IG_UNSTRUCTURED_MESH; }
        if (igvDataType == kIGVDataTypeStructuredMesh)   { return IG_STRUCTURED_MESH; }
        return IG_NONE;  // 有值但未知：拒绝，不猜
    }

    if (type == kTypeStructuredGrid) { return IG_STRUCTURED_MESH; }
    if (type == kTypeUnstructuredGrid) {
        // Type 粒度无法区分 VolumeMesh 与 UnstructuredMesh；二者在本期的
        return IG_UNSTRUCTURED_MESH;
    }
    if (type == kTypePolyData) {
        // 这里先给 SurfaceMesh；若文件里**没有**任何拓扑 dataset，
        // ParseGeometryAndTopology 会把它改判为 PointSet。
        return IG_SURFACE_MESH;
    }
    return IG_NONE;
}

// =============================================================================
// 失败
// =============================================================================

void IGVReader::Fail(const std::string& reason) {
    m_LastError = reason;
 IGAME_CORE_ERROR("[IGV] read rejected: {}", reason);
}

// =============================================================================
// =============================================================================

bool IGVReader::Parsing() {
    m_LastError.clear();
    m_Meta = igv::IGVMetadata{};
    m_Points.clear();
    m_Connectivity.clear();
    m_Offsets.clear();
    m_Types.clear();
    m_Attributes.clear();
    m_HasTopology = false;
    m_FixedCellSize = -1;
    m_MeshType = IG_NONE;

    ClearHdf5Errors();

    // 进度条：与 VTU/VTP/CGNS 等既有 Reader 同一套 Filter::UpdateProgress 机制
    // （0→1；UI 的 igQtProgressBarWidget 监听 Command::ProgressEvent）。
    // 这里必须**主动**推进：加进度前 IGV 整段读盘没有任何回调，用户看到的是一个
    // 只出现、不前进的进度条。分段与既有 Reader 保持同一量级：
    //   0.02 开始 → 0.06 文件打开 → 0.10 /IGV 定位 → 0.15 根属性
    //   → 0.15..0.55 逐属性 → 0.55..0.85 几何/拓扑（ParseGeometryAndTopology 内）
    //   → 0.85..1.0 重建（RebuildFromParsed / CreateDataObject 内）
    this->UpdateProgress(0.02);

    H5FileHandle file;
    if (m_UseMemoryBuffer) {
        if (m_MemoryBuffer == nullptr || m_MemoryBufferSize == 0) {
   Fail("memory buffer is empty (SetMemoryBuffer received invalid data/size)");
            return false;
        }

        // R14 主动检测：若磁盘上存在同名文件，HDF5 有可能忽略 file image
        // 去打开磁盘文件，从而**静默读错数据**。这里显式发现并告警，
        // 让 V1.5 的用例（"磁盘上不存在同名文件"）真正有意义。
        if (!m_FilePath.empty() && std::filesystem::exists(m_FilePath)) {
   IGAME_CORE_WARN("[IGV] reading from a memory image, but m_FilePath='{}' and a file "
                            "with that name EXISTS on disk. HDF5 could in theory ignore the file "
                            "image and read the on-disk file instead. Tests "
                            "should keep the data at a path where no same-named file exists.",
                            m_FilePath);
        }

        H5PropertyHandle fapl(H5Pcreate(H5P_FILE_ACCESS));
  if (!fapl.IsValid()) { ClearHdf5Errors(); Fail("H5Pcreate(FILE_ACCESS) failed"); return false; }

        // backing_store = false：不落盘到 backing file。
        if (H5Pset_fapl_core(fapl.Get(), static_cast<size_t>(1024 * 1024), false) < 0) {
            ClearHdf5Errors();
   Fail("H5Pset_fapl_core failed");
            return false;
        }

        // 显式接管 file image 的 ownership（R13）——见文件内 MemoryImageOwner 的说明。
        MemoryImageGuard imageGuard;
        if (!imageGuard.Init(m_MemoryBuffer, m_MemoryBufferSize)) {
   Fail("failed to allocate the file-image owner header");
            return false;
        }

        H5FD_file_image_callbacks_t callbacks{};
        callbacks.image_malloc = &ImageMalloc;
        callbacks.image_memcpy = &ImageMemcpy;
        callbacks.image_realloc = &ImageRealloc;
        callbacks.image_free = &ImageFree;
        // ★ 这两个回调**不可省略**：udata 非空时 HDF5 要求它们必须存在
        //   （H5Pset_file_image_callbacks 的硬校验）。二者只复制/释放 owner 头，
        //   与调用方缓冲无关 —— 详见 ImageUdataCopy 的说明。
        callbacks.udata_copy = &ImageUdataCopy;
        callbacks.udata_free = &ImageUdataFree;
        callbacks.udata = imageGuard.Get();
        if (H5Pset_file_image_callbacks(fapl.Get(), &callbacks) < 0) {
            ClearHdf5Errors();
   Fail("H5Pset_file_image_callbacks failed");
            return false;
        }

        // H5Pset_file_image 的 buf 参数是 `void*`（非 const），而调用方的缓冲是
        // `const char*`（`FileReader::SetMemoryBuffer` 的语义就是非拥有、只读）。
        // 我们既然注册了 image_memcpy 回调，HDF5 就**不会**去写这块内存；
        // 为满足签名需要做一次 const 剥离。这是唯一一处，且有注释与
        // image_free/image_memcpy 的实现共同保证"绝不写、绝不 free 调用方缓冲"。
        void* const raw = const_cast<char*>(imageGuard.Get()->data);
        if (H5Pset_file_image(fapl.Get(), raw, imageGuard.Get()->size) < 0) {
            ClearHdf5Errors();
   Fail("H5Pset_file_image failed (memorypath)");
            return false;
        }

        // H5F_CLOSE_WEAK：关闭时不因仍有打开的对象而失败（R13）。
        if (H5Pset_fclose_degree(fapl.Get(), H5F_CLOSE_WEAK) < 0) {
            ClearHdf5Errors();
   Fail("H5Pset_fclose_degree failed");
            return false;
        }

        // 名字刻意不是磁盘上可能存在的路径，避免 HDF5 误开磁盘文件（R14）。
        file.Reset(H5Fopen("igv_memory_image.igv", H5F_ACC_RDONLY, fapl.Get()));
        if (!file.IsValid()) {
            ClearHdf5Errors();
   Fail("H5Fopen(memory image) failed - the data is not a valid HDF5 file");
            return false;
        }
    } else {
        if (m_FilePath.empty()) {
   Fail("filepathis empty");
            return false;
        }
        file.Reset(H5Fopen(m_FilePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));
        if (!file.IsValid()) {
            ClearHdf5Errors();
   Fail("H5Fopen failed (not an HDF5 file, or the path is unreadable): " + m_FilePath);
            return false;
        }
    }

    // ---- 步骤 3：打开 /IGV 组 ----
    // 格式只会把"文件损坏"和"格式不对"两类错误混在一起。
    this->UpdateProgress(0.06);
    H5GroupHandle root(H5Gopen2(file.Get(), std::string(kRootGroup).c_str(), H5P_DEFAULT));
    if (!root.IsValid()) {
        ClearHdf5Errors();
  Fail("the root group is not /IGV - this is not an IGV file");
        return false;
    }
    this->UpdateProgress(0.10);

    //
    // 本阶段唯一的目的是**不静默丢数据**：
    //   * 含 `Block_*` 的文件是多块文件 —— 若把 `/IGV` 自己的几何当整体返回，
    //     用户会以为拿到了全部块，而实际只拿到"父块的壳"；
    //   * 含 `Steps` 的文件是单文件内嵌时变 —— 同样存在"只返回第 0 步"的风险。
    // 两者都必须在**读任何数据之前**拒绝，且错误信息要指明是哪一种（P6-1 / P6-3b）。
    //
    // （未知 attribute / dataset 跳过），所以只拒绝这两类**已被规范命名**的容器。
    {
        std::vector<std::string> groups;
        if (!ListGroupMembers(root.Get(), &groups, nullptr)) {
   Fail("H5Literate(/IGV) failed");
            return false;
        }
        for (const std::string& g : groups) {
            switch (ClassifyUnsupportedContainer(g)) {
                case UnsupportedContainer::MultiBlock:
                    // 绝不允许把第一块当整体返回 —— 那种"部分成功"会让用户以为数据完整，
     Fail("found block subgroup '" + g +
                         "': MultiBlock is not supported in this release. "
                         "Reading is rejected so that the remaining blocks are not dropped silently");
                    return false;
                case UnsupportedContainer::TimeSeries:
     Fail("found time-series group '" + g +
                         "': a single-file time series is not supported in this release. "
                         "Reading is rejected so that the other time steps are not dropped silently");
                    return false;
                case UnsupportedContainer::None:
                default:
                    break;
            }
        }
    }

    // ---- 步骤 4–5：根组 attribute + 版本检查 + 类型解析 ----
    if (!ParseRootAttributes(static_cast<int64_t>(root.Get()))) {
        return false;
    }
    this->UpdateProgress(0.15);

    // ---- 步骤 6：三个属性组的成员名先一次性收集（只取名字，不打开 dataset）----
    //
    // 为什么要先收集：进度条需要「总数」才能给出有意义的百分比。分两遍的代价只有
    // 3 次 H5Literate（既有的属性枚举逻辑本来就要做这一步，这里只是把"枚举"与
    // "解析"分开），不会多读任何数据。
    struct AttributeGroupInfo {
        std::string name;
        std::vector<std::string> datasets;
    };
    std::vector<AttributeGroupInfo> attributeGroups;
    attributeGroups.reserve(3);
    size_t totalAttributes = 0;
    for (std::string_view groupName : {kGroupPointData, kGroupCellData, kGroupFieldData}) {
        H5GroupHandle g(H5Gopen2(root.Get(), std::string(groupName).c_str(), H5P_DEFAULT));
        if (!g.IsValid()) {
            ClearHdf5Errors();
   Fail("missing attribute group /IGV/" + std::string(groupName) +
                 " - the spec requires all three attribute groups to exist");
            return false;
        }

        std::vector<std::string> datasetNames;
        if (!ListGroupMembers(g.Get(), nullptr, &datasetNames)) {
   Fail("H5Literate(/IGV/" + std::string(groupName) + ") failed");
            return false;
        }
        totalAttributes += datasetNames.size();
        attributeGroups.push_back({std::string(groupName), std::move(datasetNames)});
    }

    // ---- 步骤 7：逐属性解析（进度 0.15 → 0.55，按已完成属性数推进）----
    size_t parsedAttributes = 0;
    for (const AttributeGroupInfo& group : attributeGroups) {
        H5GroupHandle g(H5Gopen2(root.Get(), group.name.c_str(), H5P_DEFAULT));
        if (!g.IsValid()) {
            ClearHdf5Errors();
   Fail("missing attribute group /IGV/" + group.name +
                 " - the spec requires all three attribute groups to exist");
            return false;
        }
        for (const std::string& name : group.datasets) {
            if (!ParseOneAttribute(static_cast<int64_t>(g.Get()), group.name, name)) {
                return false;
            }
            ++parsedAttributes;
            if (totalAttributes > 0) {
                this->UpdateProgress(
                        0.15 + 0.40 * static_cast<double>(parsedAttributes) /
                                       static_cast<double>(totalAttributes));
            }
        }
    }
    this->UpdateProgress(0.55);

    if (!ParseGeometryAndTopology(static_cast<int64_t>(root.Get()))) {
        return false;
    }

    // 这一步必须在 ParseGeometryAndTopology 之后：pointCount / cellCount 到这里才确定。
    if (!ValidateAttributeLengths()) {
        return false;
    }

    // ---- 步骤 9：H5Fclose 由 H5FileHandle 析构自动完成 ----
    // CreateDataObject() 不会再触碰任何 HDF5 句柄（R3）。
    return true;
}

// =============================================================================
// =============================================================================

bool IGVReader::ParseRootAttributes(int64_t rootId) {
    const hid_t root = static_cast<hid_t>(rootId);

    // ---- Type（必填）----
    if (!ReadAsciiAttribute(root, kAttrType, m_Meta.type)) {
        ClearHdf5Errors();
  Fail("missing or unreadable root attribute 'Type'");
        return false;
    }
    if (!IsSupportedTypeToken(m_Meta.type)) {
  Fail("unknown Type value '" + m_Meta.type +
             "' (supported types: PolyData / UnstructuredGrid / StructuredGrid)");
        return false;
    }

    // ---- IGVDataType（可选，向前兼容）----
    if (!ReadAsciiAttribute(root, kAttrIGVDataType, m_Meta.igvDataType)) {
        m_Meta.igvDataType.clear();  // 老文件没有它 —— 允许，回落到 Type
    } else if (!IsSupportedIGVDataTypeToken(m_Meta.igvDataType)) {
  Fail("unknown IGVDataType value '" + m_Meta.igvDataType + "'");
        return false;
    }

    // ---- Version（**dataset**，不是 attribute —— 见 kDatasetVersion 的说明）----
    {
        std::vector<int64_t> version;
        if (!ReadDataset(root, kDatasetVersion, DType::I64, 2, version)) {
            ClearHdf5Errors();
   Fail("missing or unreadable /IGV/Version (expected i64[2])");
            return false;
        }
        m_Meta.versionMajor = version[0];
        m_Meta.versionMinor = version[1];
    }

    if (m_Meta.versionMajor != kSupportedMajor) {
  Fail("incompatible IGV version " + std::to_string(m_Meta.versionMajor) + "." +
             std::to_string(m_Meta.versionMinor) + ": this implementation supports major=" +
             std::to_string(kSupportedMajor) +
             (m_Meta.versionMajor > kSupportedMajor ? ", please upgrade iGameVis"
                                                    : ", this implementation cannot read older major versions"));
        return false;
    }
    if (m_Meta.versionMinor > kSupportedMinor) {
  IGAME_CORE_INFO("[IGV] file minor version ({}) is newer than this implementation ({}); "
                        "accepting it as backward-compatible. Unknown attributes/datasets will "
                        "be skipped.",
                        m_Meta.versionMinor, kSupportedMinor);
    }

    // ---- Bounds（可选增值项，IGV 自主，非 VTKHDF 要求）----
    {
        std::vector<double> bounds;
        if (ReadF64ArrayAttribute(root, kAttrBounds, 0, bounds)) {
            if (bounds.size() != static_cast<size_t>(kBoundsCount)) {
                ClearHdf5Errors();
    Fail("Bounds attribute length should be " + std::to_string(kBoundsCount) +
                     ", actual " + std::to_string(bounds.size()));
                return false;
            }
            for (int i = 0; i < kBoundsCount; ++i) { m_Meta.bounds[i] = bounds[i]; }
            m_Meta.hasBounds = true;
        } else {
            m_Meta.hasBounds = false;
        }
    }

    // ---- Dimensions（仅结构化网格）----
    {
        std::vector<int64_t> dims;
        if (ReadI64ArrayAttribute(root, kAttrDimensions, 0, dims)) {
            if (dims.size() != static_cast<size_t>(kDimensionsCount)) {
                ClearHdf5Errors();
    Fail("Dimensions attribute length should be " + std::to_string(kDimensionsCount) +
                     ", actual " + std::to_string(dims.size()));
                return false;
            }
            for (int i = 0; i < kDimensionsCount; ++i) { m_Meta.dimensions[i] = dims[i]; }
            m_Meta.hasDimensions = true;
        } else {
            m_Meta.hasDimensions = false;
        }
    }

    // ---- FixedCellSize（可选快速路径提示）----
    {
        int64_t size = -1;
        if (ReadI64Attribute(root, kAttrFixedCellSize, size)) {
            if (size <= 0) {
    Fail("FixedCellSize value is invalid: " + std::to_string(size));
                return false;
            }
            m_FixedCellSize = size;
            m_Meta.fixedCellSize = size;
        }
    }

    // ---- TimeValue / TimeStep / Generator（全部可选）----
    {
        double v = 0.0;
        if (ReadF64Attribute(root, kAttrTimeValue, v)) {
            m_Meta.timeValue = v;
            m_Meta.hasTimeValue = true;
        }
        int64_t s = 0;
        if (ReadI64Attribute(root, kAttrTimeStep, s)) {
            m_Meta.timeStep = s;
            m_Meta.hasTimeStep = true;
        }
        ReadAsciiAttribute(root, kAttrGenerator, m_Meta.generator);
    }

    // ---- 类型推导 ----
    m_MeshType = MapIGVDataTypeToMeshType(m_Meta.igvDataType, m_Meta.type);
    if (m_MeshType == IG_NONE) {
  Fail("cannot derive the mesh type from Type='" + m_Meta.type + "' / IGVDataType='" + m_Meta.igvDataType +
             "'");
        return false;
    }

    return true;
}

// =============================================================================
// =============================================================================

bool IGVReader::ParseOneAttribute(int64_t groupId, const std::string& groupName,
                                  const std::string& name) {
    const hid_t group = static_cast<hid_t>(groupId);

    if (name.empty() || name.front() == '.') {
  Fail("invalid attribute name '" + name + "' (must not be empty, must not start with '.')");
        return false;
    }

    // 打开属性 dataset 并**一直在它上面**工作。
    //
    // ⚠ 这里有一个真实 bug 的根因（V1.1 的 SurfaceMesh/VolumeMesh 曾因此全部读回失败）：
    //   `ReadAsciiAttribute(loc, name, out)` / `ReadF64ArrayAttribute(loc, ...)` 内部的
    //   语义是「在**对象 loc** 上打开一个名为 name 的 **attribute**」。
    //   HDF5 的 `H5Aexists/H5Aopen` 对 loc 的解释是"该对象自身的属性"。
    //   若把 loc 传成父**组**（PointData），HDF5 就会去找
    //   `/IGV/PointData/range` 这个**对象**（dataset），当然找不到 ——
    //   于是报出极具误导性的 "has no 'range' attribute"，而文件里明明有。
    //   同理 `PeekDatasetShape(group, name, ...)` 内部 `H5Dopen2(group, name)` 也会去找
    //   `/IGV/PointData/pid` 这个 dataset。
    //
    //   修正：先 `H5Dopen2` 拿到属性 dataset，后续 attribute 与数据读取**全部在这个
    //   dataset 句柄上**进行。这比给每个辅助函数都加一个"组 + 名字"参数更省，
    //   而且与 HDF5 的语义天然一致。
    H5DatasetHandle ds(H5Dopen2(group, name.c_str(), H5P_DEFAULT));
    if (!ds.IsValid()) {
        ClearHdf5Errors();
  Fail("cannot open attribute dataset '" + groupName + "/" + name + "'");
        return false;
    }
    const hid_t obj = ds.Get();

    // dataspace 维度与 dtype（只读元数据）
    std::vector<hsize_t> shape;
    DType dtype = DType::Unknown;
    if (PeekDatasetShape(group, name, shape, &dtype) < 0) {
        ClearHdf5Errors();
  Fail("attribute dataset '" + groupName + "/" + name +
             "' cannot be read (dtype outside the spec, or invalid dataspace)");
        return false;
    }
    if (shape.empty() || shape.size() > 2) {
  Fail("attribute dataset '" + groupName + "/" + name + "' rank should be 1 or 2, actual " +
             std::to_string(shape.size()));
        return false;
    }

    ParsedAttribute pa;
    pa.name = name;
    pa.groupName = groupName;
    pa.dtype = dtype;
    pa.count = static_cast<int64_t>(shape[0]);
    pa.components = (shape.size() == 2) ? static_cast<int>(shape[1]) : 1;

    if (pa.components <= 0) {
  Fail("attribute '" + name + "' component count is " + std::to_string(pa.components));
        return false;
    }
    if (MulWouldOverflow(pa.count, pa.components)) {
  Fail("attribute '" + name + "' count*components overflows (count=" +
             std::to_string(pa.count) + ", components=" + std::to_string(pa.components) + ")");
        return false;
    }
    {
        // 与 Points 同理：属性 dataset 的 dataspace 也可能被篡改。
        std::string why;
        const int64_t elementBytes =
            static_cast<int64_t>(SizeOfDType(pa.dtype)) * static_cast<int64_t>(pa.components);
        if (!CheckDeclaredStorage(obj, "attribute '" + name + "'", pa.count, elementBytes, why)) {
            Fail(why);
            return false;
        }
    }

    //
    // 三者是**独立的事实来源**：HDF5 类型来自对象头，`DataType`/`DataTypeBits` 来自
    // 的猜测。老文件（1.0）没有这两个 attribute，此时**只用 HDF5 类型**（向后兼容）。
    {
        std::string declaredToken;
        if (ReadAsciiAttribute(obj, kAttrDataType, declaredToken)) {
            const DType declared = ParseDTypeToken(declaredToken);
            if (declared == DType::Unknown) {
   Fail("attribute '" + name + "' declares DataType='" + declaredToken +
                     "', which is not one of the 10 fixed-width types (section 3.6)");
                return false;
            }
            if (declared != pa.dtype) {
   Fail("attribute '" + name + "' declares DataType=" + declaredToken +
                     " but its HDF5 type is " + std::string(DTypeToken(pa.dtype)) +
                     " (section 3.6 requires the two to agree)");
                return false;
            }
        }
        int64_t declaredBits = 0;
        if (ReadI64Attribute(obj, kAttrDataTypeBits, declaredBits)) {
            if (declaredBits != static_cast<int64_t>(DTypeBits(pa.dtype))) {
   Fail("attribute '" + name + "' declares DataTypeBits=" +
                     std::to_string(declaredBits) + " but its HDF5 type is " +
                     std::to_string(DTypeBits(pa.dtype)) + " bit (section 3.6 requires agreement)");
                return false;
            }
        }
    }

    // ---- attachment：优先 dataset 的 attribute，缺失则按组名兜底 ----
    if (!ReadAsciiAttribute(obj, kAttrAttachment, pa.attachmentToken)) {
        const IGenum implied = AttachmentFromGroup(groupName);
        if (implied == IG_NONE) {
   Fail("attribute '" + name + "' has no 'attachment' attribute and its group " + groupName +
                 " does not imply one (FieldData entries must declare 'attachment' explicitly)");
            return false;
        }
        pa.attachmentToken.clear();
        pa.attachmentTypeValue = implied;
    } else {
        if (!IsSupportedAttachmentToken(pa.attachmentToken)) {
   Fail("attribute '" + name + "' attachment value '" + pa.attachmentToken + "' unknown");
            return false;
        }
        pa.attachmentTypeValue = AttachmentFromToken(pa.attachmentToken);
        if (pa.attachmentTypeValue == IG_NONE) {
   Fail("attribute '" + name + "' attachment cannot be mapped to an iGame enum");
            return false;
        }
    }

    // ---- AttributeType ----
    if (!ReadAsciiAttribute(obj, kAttrAttributeType, pa.attributeTypeToken)) {
        pa.attributeTypeValue = IG_SCALAR;
        pa.attributeTypeToken = std::string(kAttributeTypeScalars);
  IGAME_CORE_WARN("[IGV] attribute '{}' has no AttributeType; treating it as Scalars", name);
    } else {
        if (!IsSupportedAttributeTypeToken(pa.attributeTypeToken)) {
   Fail("attribute '" + name + "' AttributeType value '" + pa.attributeTypeToken + "' unknown");
            return false;
        }
        pa.attributeTypeValue = AttributeTypeFromToken(pa.attributeTypeToken);
    }

    {
        std::vector<double> range;
        if (!ReadF64ArrayAttribute(obj, kAttrRange, 0, range)) {
            ClearHdf5Errors();
   Fail("attribute '" + name + "' has no 'range' attribute (required by the spec, see section 3.6)");
            return false;
        }
        const size_t expectedShort = 2;
        const size_t expectedLong = 2 * (1 + static_cast<size_t>(pa.components));
        const size_t expected = (pa.components == 1) ? expectedShort : expectedLong;
        if (range.size() != expected) {
   Fail("attribute '" + name + "' range length " + std::to_string(range.size()) +
                 " is invalid (components=" + std::to_string(pa.components) + ", should be " +
                 std::to_string(expected) + ")");
            return false;
        }
        pa.range = std::move(range);
    }

    // ---- 读数据（native 类型，HDF5 负责端序转换）----
    // 一条路径覆盖全部 10 类定宽 dtype：宽度由 dtype 决定，见 ReadDatasetAsBytes。
    const size_t total = static_cast<size_t>(pa.count * pa.components);
    if (!ReadDatasetAsBytesFrom(obj, dtype, total, pa.bytes)) {
        ClearHdf5Errors();
   Fail("failed to read attribute dataset '" + name + "' (expected " +
                 std::string(DTypeName(dtype)) + ")");
        return false;
    }
    m_Attributes.push_back(std::move(pa));
    return true;
}

// =============================================================================
// =============================================================================

bool IGVReader::ParseGeometryAndTopology(int64_t rootId) {
    const hid_t root = static_cast<hid_t>(rootId);

    const bool isPointSet = (m_MeshType == IG_POINT_SET);
    const bool isStructured = (m_MeshType == IG_STRUCTURED_MESH);

    // ---- Points（必填）----
    {
        std::vector<hsize_t> shape;
        DType dtype = DType::Unknown;
        if (PeekDatasetShape(root, kDatasetPoints, shape, &dtype) < 0) {
            ClearHdf5Errors();
   Fail("missing or unreadable /IGV/Points");
            return false;
        }
        if (shape.size() != 2 || shape[1] != static_cast<hsize_t>(kPointsComponents)) {
   Fail("Points dataspace must be (n, 3), actual rank=" +
                 std::to_string(shape.size()) +
                 (shape.size() == 2 ? (" dims[1]=" + std::to_string(shape[1])) : std::string()));
            return false;
        }
        // A8：Points 只允许 f32
        if (dtype != DType::F32) {
   Fail(std::string("Points dtype must be f32 (H5T_IEEE_F32LE), actual ") +
                 DTypeName(dtype) + "");
            return false;
        }

        const int64_t n = static_cast<int64_t>(shape[0]);
        if (MulWouldOverflow(n, kPointsComponents)) {
   Fail("Points n*3 overflows (n=" + std::to_string(n) + ")");
            return false;
        }
        {
            // 先确认"声明的字节数"没有荒谬到超出文件的物理存储，
            // 再 resize —— 否则一份被篡改的 dataspace 会让进程在分配处被 OOM 杀掉。
            H5DatasetHandle ds(H5Dopen2(root, std::string(kDatasetPoints).c_str(), H5P_DEFAULT));
            std::string why;
            if (!ds.IsValid()) {
                ClearHdf5Errors();
                Fail("cannot open /IGV/Points");
                return false;
            }
            if (!CheckDeclaredStorage(ds.Get(), kDatasetPoints, n,
                                      static_cast<int64_t>(kPointsComponents) * 4, why)) {
                Fail(why);
                return false;
            }
        }
        m_Points.clear();
        if (n > 0 && !ReadDataset(root, kDatasetPoints, DType::F32,
                                  static_cast<size_t>(n) * kPointsComponents, m_Points)) {
            ClearHdf5Errors();
   Fail("read /IGV/Points failed");
            return false;
        }
        m_Meta.pointCount = n;
    }
    // 进度：Points 是最大的一块数据，读完它就该让进度条走一大步。
    this->UpdateProgress(0.62);

    const int64_t pointCount = m_Meta.pointCount;

    // ---- TopologyIndex（可选 attribute；缺省 = Primary）----
    //
    // 它声明的是 `Connectivity` 的**语义**：一级索引里是"单元→点"，二级索引
    // （多面体）里是"体→面"。这不是能靠猜的东西 —— 与 `CellTypeEncoding`（A3）
    // 同一套思路：约定必须写在文件里，读侧只做校验。
    {
        std::string mode;
        if (ReadAsciiAttribute(root, kAttrTopologyIndex, mode)) {
            m_Meta.topologyIndex = mode;
            if (mode == kTopologyIndexSecondary) {
                m_SecondaryIndex = true;
            } else if (mode != kTopologyIndexPrimary) {
   Fail("TopologyIndex='" + mode + "' is unknown (allowed: '" +
                 std::string(kTopologyIndexPrimary) + "' / '" +
                 std::string(kTopologyIndexSecondary) + "')");
                return false;
            }
        }
    }

    {
        std::vector<int64_t> v;
        if (!DatasetExists(root, kDatasetNumberOfPoints) ||
            !ReadDataset(root, kDatasetNumberOfPoints, DType::I64, 1, v)) {
            ClearHdf5Errors();
   Fail("missing the required count dataset NumberOfPoints (the spec requires it to be written explicitly, see section 3.4)");
            return false;
        }
        if (v[0] != pointCount) {
   Fail("NumberOfPoints(" + std::to_string(v[0]) + ") != Points.dims[0](" +
                 std::to_string(pointCount) + ") - count dataset disagrees with the array length");
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    if (isPointSet) {
        // 说明写出方没有遵守分型规则；继续读会**静默丢掉拓扑**
        // （iGame 的 PointSet 没有单元表达能力），因此必须拒绝。
        for (std::string_view bad : {kDatasetConnectivity, kDatasetOffsets, kDatasetTypes,
                                     kDatasetNumberOfCells,
                                     kDatasetNumberOfConnectivityIds,
                                     kDatasetFaceConnectivity, kDatasetFaceOffsets,
                                     kDatasetNumberOfFaces}) {
            if (DatasetExists(root, bad)) {
    Fail("Type/IGVDataType declares PointSet, but a topology dataset exists: '" +
                     std::string(bad) +
                     "'. A PointSet cannot carry cells; continuing would silently drop the"
                     " topology, so this is rejected");
                return false;
            }
        }
        m_HasTopology = false;
        m_Meta.hasCellCount = false;
        this->UpdateProgress(0.85); // 无拓扑分支：到此 Parsing 的数据读取已经走完
        return true;
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    if (isStructured) {
        if (!m_Meta.hasDimensions) {
   Fail("IGVDataType=StructuredMesh but the Dimensions attribute is missing");
            return false;
        }
        const int64_t nx = m_Meta.dimensions[0];
        const int64_t ny = m_Meta.dimensions[1];
        const int64_t nz = m_Meta.dimensions[2];
        if (nx <= 0 || ny <= 0 || nz <= 0) {
   Fail("Dimensions invalid: [" + std::to_string(nx) + ", " + std::to_string(ny) +
                 ", " + std::to_string(nz) + "]");
            return false;
        }
        if (MulWouldOverflow(nx * ny, nz)) {
   Fail("Dimensions product overflow");
            return false;
        }
        if (nx * ny * nz != pointCount) {
   Fail("Dimensions product(" + std::to_string(nx * ny * nz) +
                 ") != pointCount(" + std::to_string(pointCount) + ") - section 6.3 requires rejection");
            return false;
        }
        if (DatasetExists(root, kDatasetConnectivity) ||
            DatasetExists(root, kDatasetOffsets) ||
            DatasetExists(root, kDatasetTypes)) {
   Fail("IGVDataType=StructuredMesh must not carry Connectivity/Offsets/Types"
                 " (a structured mesh expresses its topology implicitly through Dimensions)");
            return false;
        }
        m_HasTopology = false;
        m_Meta.hasCellCount = false;
        this->UpdateProgress(0.85); // 结构化：拓扑由 Dimensions 隐式表达，数据读取已走完
        return true;
    }

    // -------------------------------------------------------------------------
    // 非结构族：Connectivity / Offsets / Types / 三个计数组 全部必填
    // -------------------------------------------------------------------------
    for (std::string_view required : kUnstructuredTopologyDatasets) {
        if (!DatasetExists(root, required)) {
   Fail("non-structural file is missing the required dataset '" + std::string(required) +
                 "'");
            return false;
        }
    }

    int64_t declaredCells = 0;
    int64_t declaredConn = 0;
    {
        std::vector<int64_t> v;
        if (!ReadDataset(root, kDatasetNumberOfCells, DType::I64, 1, v)) {
            ClearHdf5Errors();
   Fail("cannotread NumberOfCells");
            return false;
        }
        declaredCells = v[0];
        if (!ReadDataset(root, kDatasetNumberOfConnectivityIds, DType::I64, 1, v)) {
            ClearHdf5Errors();
   Fail("cannotread NumberOfConnectivityIds");
            return false;
        }
        declaredConn = v[0];
    }
    if (declaredCells < 0 || declaredConn < 0) {
  Fail("count dataset is negative: NumberOfCells=" + std::to_string(declaredCells) +
             ", NumberOfConnectivityIds=" + std::to_string(declaredConn));
        return false;
    }
    m_Meta.cellCount = declaredCells;
    m_Meta.hasCellCount = true;
    m_Meta.connectivityIdCount = declaredConn;
    m_Meta.hasConnectivityIdCount = true;

    // ---- Connectivity（i32）----
    {
        std::vector<hsize_t> shape;
        DType dtype = DType::Unknown;
        if (PeekDatasetShape(root, kDatasetConnectivity, shape, &dtype) < 0) {
            ClearHdf5Errors();
   Fail("failed to read the dataspace/dtype of /IGV/Connectivity");
            return false;
        }
        if (shape.size() != 1) {
   Fail("Connectivity rank must be 1, actual " + std::to_string(shape.size()));
            return false;
        }
        if (dtype != DType::I32) {
   Fail(std::string("Connectivity dtype must be i32 (H5T_STD_I32LE), actual ") +
                 DTypeName(dtype));
            return false;
        }
        if (static_cast<int64_t>(shape[0]) != declaredConn) {
   Fail("NumberOfConnectivityIds(" + std::to_string(declaredConn) +
                 ") != Connectivity length(" + std::to_string(shape[0]) +
                 ") - count dataset disagrees with the array length");
            return false;
        }
        {
            H5DatasetHandle ds(H5Dopen2(root, std::string(kDatasetConnectivity).c_str(), H5P_DEFAULT));
            std::string why;
            if (!ds.IsValid()) {
                ClearHdf5Errors();
                Fail("cannot open /IGV/Connectivity");
                return false;
            }
            if (!CheckDeclaredStorage(ds.Get(), kDatasetConnectivity, declaredConn, 4, why)) {
                Fail(why);
                return false;
            }
        }
        if (declaredConn > 0 &&
            !ReadDataset(root, kDatasetConnectivity, DType::I32,
                         static_cast<size_t>(declaredConn), m_Connectivity)) {
            ClearHdf5Errors();
   Fail("read /IGV/Connectivity failed");
            return false;
        }
    }

    // ---- Offsets（i64）----
    this->UpdateProgress(0.72);    {
        std::vector<hsize_t> shape;
        DType dtype = DType::Unknown;
        if (PeekDatasetShape(root, kDatasetOffsets, shape, &dtype) < 0) {
            ClearHdf5Errors();
   Fail("failed to read the dataspace/dtype of /IGV/Offsets");
            return false;
        }
        if (shape.size() != 1) {
   Fail("Offsets rank must be 1, actual " + std::to_string(shape.size()));
            return false;
        }
        if (dtype != DType::I64) {
   Fail(std::string("Offsets dtype must be i64 (H5T_STD_I64LE), actual ") +
                 DTypeName(dtype));
            return false;
        }
        if (static_cast<int64_t>(shape[0]) != declaredCells + 1) {
   Fail("Offsets length(" + std::to_string(shape[0]) +
                 ") != NumberOfCells+1(" + std::to_string(declaredCells + 1) +
                 ") - count dataset disagrees with the array length");
            return false;
        }
        {
            H5DatasetHandle ds(H5Dopen2(root, std::string(kDatasetOffsets).c_str(), H5P_DEFAULT));
            std::string why;
            if (!ds.IsValid()) {
                ClearHdf5Errors();
                Fail("cannot open /IGV/Offsets");
                return false;
            }
            if (!CheckDeclaredStorage(ds.Get(), kDatasetOffsets, declaredCells + 1, 8, why)) {
                Fail(why);
                return false;
            }
        }
        if (!ReadDataset(root, kDatasetOffsets, DType::I64,
                         static_cast<size_t>(declaredCells + 1), m_Offsets)) {
            ClearHdf5Errors();
   Fail("read /IGV/Offsets failed");
            return false;
        }
    }
    if (!m_Offsets.empty()) {
        if (m_Offsets.front() != 0) {
   Fail("Offsets[0] must be 0, actual " + std::to_string(m_Offsets.front()));
            return false;
        }
        for (size_t i = 0; i + 1 < m_Offsets.size(); ++i) {
            if (m_Offsets[i + 1] <= m_Offsets[i]) {
    Fail("Offsets must be strictly increasing: at index " + std::to_string(i) + " - " +
                     std::to_string(m_Offsets[i]) + " -> " +
                     std::to_string(m_Offsets[i + 1]));
                return false;
            }
        }
        if (m_Offsets.back() != declaredConn) {
   Fail("Offsets last value(" + std::to_string(m_Offsets.back()) +
                 ") != Connectivity length(" + std::to_string(declaredConn) + ")");
            return false;
        }
    }

    // ---- 多面体的面表（二级索引）：必须在校验 Connectivity 之前读出来 ----
    if (m_SecondaryIndex) {
        if (!ParsePolyhedronFaces(root, declaredCells, pointCount)) { return false; }
    } else {
        // 一级索引却带着面表 = 写出方自相矛盾；静默忽略等于丢数据，因此拒绝。
        for (std::string_view extra : kPolyhedronTopologyDatasets) {
            if (DatasetExists(root, extra)) {
   Fail(std::string("the file contains the polyhedron-only dataset '") +
                     std::string(extra) + "' but does not declare TopologyIndex=" +
                     std::string(kTopologyIndexSecondary) +
                     " (a primary-index file must not carry face tables)");
                return false;
            }
        }
    }

    const int64_t connectivityLimit = m_SecondaryIndex ? m_FaceCount : pointCount;
    const char* const limitName = m_SecondaryIndex ? "faceCount" : "pointCount";
    for (size_t i = 0; i < m_Connectivity.size(); ++i) {
        const int64_t id = static_cast<int64_t>(m_Connectivity[i]);
        if (id < 0 || id >= connectivityLimit) {
   Fail("Connectivity[" + std::to_string(i) + "] = " + std::to_string(id) +
                 " is out of range (" + std::string(limitName) + " = " +
                 std::to_string(connectivityLimit) + ")");
            return false;
        }
    }

    this->UpdateProgress(0.78);
    {
        std::vector<hsize_t> shape;
        DType dtype = DType::Unknown;
        if (PeekDatasetShape(root, kDatasetTypes, shape, &dtype) < 0) {
            ClearHdf5Errors();
   Fail("failed to read the dataspace/dtype of /IGV/Types");
            return false;
        }
        if (shape.size() != 1) {
   Fail("Types rank must be 1, actual " + std::to_string(shape.size()));
            return false;
        }
        if (dtype != DType::U8) {
   Fail(std::string("Types dtype must be u8 (H5T_STD_U8LE), actual ") +
                 DTypeName(dtype));
            return false;
        }
        if (static_cast<int64_t>(shape[0]) != declaredCells) {
   Fail("NumberOfCells(" + std::to_string(declaredCells) + ") != Types length(" +
                 std::to_string(shape[0]) + ") - count dataset disagrees with the array length");
            return false;
        }
        {
            H5DatasetHandle ds(H5Dopen2(root, std::string(kDatasetTypes).c_str(), H5P_DEFAULT));
            std::string why;
            if (!ds.IsValid()) {
                ClearHdf5Errors();
                Fail("cannot open /IGV/Types");
                return false;
            }
            if (!CheckDeclaredStorage(ds.Get(), kDatasetTypes, declaredCells, 1, why)) {
                Fail(why);
                return false;
            }
        }
        if (declaredCells > 0 &&
            !ReadDataset(root, kDatasetTypes, DType::U8, static_cast<size_t>(declaredCells),
                         m_Types)) {
            ClearHdf5Errors();
   Fail("read /IGV/Types failed");
            return false;
        }
    }

    this->UpdateProgress(0.82); // Types 已读入，剩下白名单校验与 dataspace 检查
    if (m_SecondaryIndex && m_Types.empty()) {
   Fail("TopologyIndex=Secondary requires the Types dataset with one IG_POLYHEDRON entry per cell");
        return false;
    }
    for (size_t i = 0; i < m_Types.size(); ++i) {
        const int code = static_cast<int>(m_Types[i]);
        if (m_SecondaryIndex) {
            // 二级索引里 IG_POLYHEDRON(8) 是合法类型；别的类型说明模式与数据不一致。
            if (IsWritableCellTypeInSecondaryIndex(code)) { continue; }
   Fail("Types[" + std::to_string(i) + "] = " + std::to_string(code) +
                 " but TopologyIndex=Secondary requires every cell to be IG_POLYHEDRON(8)");
            return false;
        }
        if (IsWritableCellType(code)) { continue; }
        if (const auto* forbidden = FindForbiddenCellType(code)) {
   Fail("Types[" + std::to_string(i) + "] = " + std::to_string(code) + " (" +
                 std::string(forbidden->name) + ") is not in the IGV writable whitelist: " +
                 std::string(forbidden->reason));
        } else {
   Fail("Types[" + std::to_string(i) + "] = " + std::to_string(code) +
                 " is not in the IGV cell-type whitelist (an unknown code must be rejected,"
                 " never skipped or treated as 0)");
        }
        return false;
    }

    // ---- CellTypeEncoding 必须存在（A3：低值段整体错位，没有它就只能猜）----
    {
        std::string encoding;
        if (!ReadAsciiAttribute(root, kAttrCellTypeEncoding, encoding)) {
            ClearHdf5Errors();
   Fail("the file contains Types but has no CellTypeEncoding attribute - "
                 "the encoding scheme must not be guessed");
            return false;
        }
        if (encoding != kCellTypeEncodingIGCellType) {
   Fail("CellTypeEncoding='" + encoding + "' is unsupported (this implementation only supports '" +
                 std::string(kCellTypeEncodingIGCellType) + "')");
            return false;
        }
    }

    //
    // 这条检查把"本期用固定 dataspace"变成可检查事实。若将来 F6/F9 引入可扩展
    // 而不是静默漂移。
    for (std::string_view name : kUnstructuredTopologyDatasets) {
        std::vector<hsize_t> dims;
        std::vector<hsize_t> maxDims;
        if (!ReadDatasetDimsAndMaxDims(root, name, dims, maxDims)) {
            ClearHdf5Errors();
   Fail("cannot read dataset '" + std::string(name) + "'");
            return false;
        }
        for (size_t i = 0; i < maxDims.size(); ++i) {
            if (maxDims[i] == H5S_UNLIMITED || maxDims[i] != dims[i]) {
    Fail("dataset '" + std::string(name) +
                     "' uses an extensible dataspace, but this release requires a fixed dataspace"
                     ". If extensibility is really wanted,"
                     " the spec and the section 6.3 cross-checks must be updated together");
                return false;
            }
        }
    }

    m_HasTopology = true;
    this->UpdateProgress(0.85); // 几何/拓扑读毕（0.55 → 0.85 覆盖三个大数组）
    return true;
}

// =============================================================================
// =============================================================================

///
/// 校验项与一级索引同构 —— 偏移从 0 起、严格递增、末值等于数组长度、索引不越界、
/// 显式计数与数组长度一致。它必须在 Connectivity 越界校验**之前**跑完，因为二级
/// 索引里的 Connectivity 存的是面 id（上限是 faceCount，不是 pointCount）。
bool IGVReader::ParsePolyhedronFaces(int64_t rootId, int64_t declaredCells, int64_t pointCount) {
    const hid_t root = static_cast<hid_t>(rootId);

    for (std::string_view name : kPolyhedronTopologyDatasets) {
        if (!DatasetExists(root, name)) {
   Fail("TopologyIndex=Secondary but the required dataset '" + std::string(name) +
                 "' is missing");
            return false;
        }
    }

    // ---- NumberOfFaces（显式计数，A15 思路：数量不从 dataspace 推导）----
    std::vector<int64_t> faces;
    if (!ReadDataset(root, kDatasetNumberOfFaces, DType::I64, 1, faces)) {
        ClearHdf5Errors();
   Fail("cannot read the required count dataset NumberOfFaces");
        return false;
    }
    if (faces[0] <= 0) {
   Fail("NumberOfFaces must be positive, actual " + std::to_string(faces[0]));
        return false;
    }

    // ---- FaceOffsets ----
    {
        std::vector<hsize_t> shape;
        DType dtype = DType::Unknown;
        if (PeekDatasetShape(root, kDatasetFaceOffsets, shape, &dtype) < 0) {
            ClearHdf5Errors();
   Fail("missing or unreadable /IGV/FaceOffsets");
            return false;
        }
        if (shape.size() != 1) {
   Fail("FaceOffsets rank must be 1, actual " + std::to_string(shape.size()));
            return false;
        }
        if (dtype != DType::I64) {
   Fail(std::string("FaceOffsets dtype must be i64 (H5T_STD_I64LE), actual ") + DTypeName(dtype));
            return false;
        }
        if (static_cast<int64_t>(shape[0]) != faces[0] + 1) {
   Fail("NumberOfFaces(" + std::to_string(faces[0]) + ") + 1 != FaceOffsets length(" +
                 std::to_string(shape[0]) + ") - count dataset disagrees with the array length");
            return false;
        }
        if (!ReadDataset(root, kDatasetFaceOffsets, DType::I64,
                         static_cast<size_t>(faces[0] + 1), m_FaceOffsets)) {
            ClearHdf5Errors();
   Fail("read /IGV/FaceOffsets failed");
            return false;
        }
        if (m_FaceOffsets.front() != 0) {
   Fail("FaceOffsets[0] must be 0, actual " + std::to_string(m_FaceOffsets.front()));
            return false;
        }
        for (size_t i = 0; i + 1 < m_FaceOffsets.size(); ++i) {
            if (m_FaceOffsets[i + 1] <= m_FaceOffsets[i]) {
   Fail("FaceOffsets must be strictly increasing: at index " + std::to_string(i) + " - " +
                     std::to_string(m_FaceOffsets[i]) + " -> " +
                     std::to_string(m_FaceOffsets[i + 1]));
                return false;
            }
        }
    }

    // ---- FaceConnectivity（面→点）----
    {
        std::vector<hsize_t> shape;
        DType dtype = DType::Unknown;
        if (PeekDatasetShape(root, kDatasetFaceConnectivity, shape, &dtype) < 0) {
            ClearHdf5Errors();
   Fail("missing or unreadable /IGV/FaceConnectivity");
            return false;
        }
        if (shape.size() != 1) {
   Fail("FaceConnectivity rank must be 1, actual " + std::to_string(shape.size()));
            return false;
        }
        if (dtype != DType::I32) {
   Fail(std::string("FaceConnectivity dtype must be i32 (H5T_STD_I32LE), actual ") +
                 DTypeName(dtype));
            return false;
        }
        if (m_FaceOffsets.back() != static_cast<int64_t>(shape[0])) {
   Fail("FaceOffsets last value(" + std::to_string(m_FaceOffsets.back()) +
                 ") != FaceConnectivity length(" + std::to_string(shape[0]) + ")");
            return false;
        }
        if (!ReadDataset(root, kDatasetFaceConnectivity, DType::I32,
                         static_cast<size_t>(shape[0]), m_FaceConnectivity)) {
            ClearHdf5Errors();
   Fail("read /IGV/FaceConnectivity failed");
            return false;
        }
        for (size_t i = 0; i < m_FaceConnectivity.size(); ++i) {
            const int64_t id = static_cast<int64_t>(m_FaceConnectivity[i]);
            if (id < 0 || id >= pointCount) {
   Fail("FaceConnectivity[" + std::to_string(i) + "] = " + std::to_string(id) +
                     " is out of range (pointCount = " + std::to_string(pointCount) + ")");
                return false;
            }
        }
    }

    if (declaredCells <= 0) {
   Fail("polyhedron file has no cells");
        return false;
    }

    m_FaceCount = faces[0];
    m_Meta.faceCount = faces[0];
    m_Meta.hasFaceCount = true;
    return true;
}

bool IGVReader::ValidateAttributeLengths() {
    const int64_t pointCount = m_Meta.pointCount;

    // 结构化网格：单元数由 Dimensions 隐式决定，与 iGame 的
    // `StructuredMesh::GetNumberOfCells()` 保持同一口径
    // （`size[2] > 1` 取体数，否则取面数 → (nx-1)(ny-1)max(nz-1,1)）。
    int64_t structuredCells = -1;
    if (m_MeshType == IG_STRUCTURED_MESH && m_Meta.hasDimensions) {
        const int64_t nx = m_Meta.dimensions[0];
        const int64_t ny = m_Meta.dimensions[1];
        const int64_t nz = m_Meta.dimensions[2];
        if (nx > 1 && ny > 1 && nz >= 1) {
            structuredCells = (nx - 1) * (ny - 1) * ((nz > 1) ? (nz - 1) : 1);
        }
    }

    for (const ParsedAttribute& pa : m_Attributes) {
        if (pa.attachmentTypeValue == IG_POINT) {
            if (pa.count != pointCount) {
                Fail("attribute '" + pa.name + "' (" + pa.groupName + ") has " +
                     std::to_string(pa.count) + " element(s) but it is attached to points and the mesh has " +
                     std::to_string(pointCount) + " point(s) - section 6.3 requires rejection");
                return false;
            }
            continue;
        }
        if (pa.attachmentTypeValue != IG_CELL) { continue; }  // 其余依附类型无网格计数可对

        if (m_MeshType == IG_POINT_SET) {
            Fail("attribute '" + pa.name + "' is attached to cells, but the file declares a PointSet "
                 "(no cells at all) - section 6.3 / 3.3");
            return false;
        }
        if (m_MeshType == IG_STRUCTURED_MESH) {
            if (structuredCells >= 0 && pa.count != structuredCells) {
                Fail("attribute '" + pa.name + "' (" + pa.groupName + ") has " +
                     std::to_string(pa.count) + " element(s) but Dimensions [" +
                     std::to_string(m_Meta.dimensions[0]) + "," + std::to_string(m_Meta.dimensions[1]) +
                     "," + std::to_string(m_Meta.dimensions[2]) + "] imply " +
                     std::to_string(structuredCells) + " cell(s) - section 6.3 requires rejection");
                return false;
            }
            continue;
        }
        if (pa.count != m_Meta.cellCount) {
            Fail("attribute '" + pa.name + "' (" + pa.groupName + ") has " + std::to_string(pa.count) +
                 " element(s) but it is attached to cells and the mesh has " +
                 std::to_string(m_Meta.cellCount) + " cell(s) - section 6.3 requires rejection");
            return false;
        }
    }
    return true;
}

// =============================================================================
// =============================================================================

bool IGVReader::RebuildFromParsed() {
    // 进度 0.85 起步：Parsing() 结束时已经走到 0.85（见 ParseGeometryAndTopology）。
    // 重建阶段的剩余 15% 按「拓扑 → 逐属性 → range 挂载」三段分配。
    this->UpdateProgress(0.85);

    auto adapter = std::make_unique<MeshDecodeAdapterToDataObject>();

    // 步骤 2：SetMeshType 必须在一切之前（它负责创建 m_DataObj）
    adapter->SetMeshType(m_MeshType);

    // 步骤 3：点坐标（f32 已在 Parsing 阶段强校验）
    adapter->SetPointBuffer(m_Points);

    // 步骤 4：拓扑分支
    if (m_MeshType == IG_POINT_SET) {
    } else if (m_MeshType == IG_STRUCTURED_MESH) {
        // 注意：SetDimensionSize **会写穿** axisSize（s[2] = 1），所以传本地副本。
        int axisSize[3] = {static_cast<int>(m_Meta.dimensions[0]),
                           static_cast<int>(m_Meta.dimensions[1]),
                           static_cast<int>(m_Meta.dimensions[2])};
        // 对齐 iGameMeshDecoderFilter.h:381 的既有做法：
        // 结构化网格不写拓扑 payload，由尺寸 + 点数隐式生成连通性。
        adapter->SetStructuredMeshDimension(axisSize);
    } else if (m_SecondaryIndex) {
        // 多面体：适配器要的是"每单元 面数 / 每面 点数"，文件里存的是偏移，先换算。
        std::vector<unsigned int> v2fIndex(m_Connectivity.begin(), m_Connectivity.end());
        std::vector<unsigned int> f2pIndex(m_FaceConnectivity.begin(), m_FaceConnectivity.end());
        std::vector<unsigned int> v2fSize;
        std::vector<unsigned int> f2pSize;
        v2fSize.reserve(m_Offsets.size() > 0 ? m_Offsets.size() - 1 : 0);
        for (size_t i = 0; i + 1 < m_Offsets.size(); ++i) {
            v2fSize.push_back(static_cast<unsigned int>(m_Offsets[i + 1] - m_Offsets[i]));
        }
        f2pSize.reserve(m_FaceOffsets.size() > 0 ? m_FaceOffsets.size() - 1 : 0);
        for (size_t i = 0; i + 1 < m_FaceOffsets.size(); ++i) {
            f2pSize.push_back(static_cast<unsigned int>(m_FaceOffsets[i + 1] - m_FaceOffsets[i]));
        }
        adapter->AddSecondaryIndexCells(v2fIndex, v2fSize, f2pIndex, f2pSize);
    } else {
        const int64_t nCells = m_Meta.hasCellCount ? m_Meta.cellCount : 0;
        std::vector<uint32_t> ids(m_Connectivity.begin(), m_Connectivity.end());
        std::vector<uint32_t> types(m_Types.begin(), m_Types.end());

        std::vector<uint32_t> cellSizes(static_cast<size_t>(nCells));
        bool allSame = (nCells > 0);
        int firstSize = 0;
        for (int64_t i = 0; i < nCells; ++i) {
            const int64_t size = m_Offsets[static_cast<size_t>(i) + 1] -
                                 m_Offsets[static_cast<size_t>(i)];
            cellSizes[static_cast<size_t>(i)] = static_cast<uint32_t>(size);
            if (i == 0) { firstSize = static_cast<int>(size); }
            else if (size != firstSize) { allSame = false; }
        }

        const bool hasTypes = !types.empty();

        if (allSame && firstSize > 0) {
            if (hasTypes) {
                adapter->AddUnstructuredFixedCells(ids, firstSize, types);
            } else {
                // SurfaceMesh / VolumeMesh 的定长分支
                adapter->AddSameTypeFixedCells(ids, firstSize);
            }
        } else {
            if (hasTypes) {
                adapter->AddUnstructuredPolyCells(ids, cellSizes, types);
            } else {
                adapter->AddSameTypePolyCells(ids, cellSizes);
            }
        }
    }

    // 步骤 5：属性 —— 直接从"原始字节 + dtype"建 iGame 数组（10 类 dtype 一条路径）
    //
    // 为什么不再经 `AttributeBuffer` / `adapter->AddAttribute()`：
    //   那个结构只承载 float / double / uint8 三种负载（iGameCodecPayload.h），
    //   其余 dtype 在适配器里会被当成 double 处理 —— 对交换格式就是静默丢精度。
    //   IGV 要无损承载 VTK 的全部标量类型，所以属性数组在这里直接构造；
    //   几何 / 拓扑仍然复用适配器（A5 的支点不变）。
    this->UpdateProgress(0.90); // 拓扑已建好

    DataObject::Pointer obj = adapter->GetOutput();
    if (!obj) {
   Fail("MeshDecodeAdapterToDataObject::GetOutput() returned null");
        return false;
    }
    AttributeSet* attrSet = obj->GetAttributeSet();
    if (attrSet == nullptr) {
   Fail("the rebuilt object has no AttributeSet");
        return false;
    }

    size_t rebuiltAttributes = 0;
    for (const ParsedAttribute& pa : m_Attributes) {
        ArrayObject::Pointer arr = MakeAttributeArray(pa.bytes, pa.dtype, pa.components, pa.name);
        if (!arr) {
   Fail("attribute '" + pa.name + "' cannot be rebuilt as " +
                 std::string(DTypeToken(pa.dtype)));
            return false;
        }
        // 用返回的索引定位（而不是按名字再查一遍）：同一个名字可以同时存在于
        // PointData 与 CellData（CCM/Plane 这类数据就是这样），按名字查会指向错的那条。
        const IGsize attrIndex =
                attrSet->AddAttribute(pa.attributeTypeValue, pa.attachmentTypeValue, arr);

        // 步骤 7：把文件里的预计算 range 挂回属性。
        //
        // iGame dataRange has (components+1) rows:
        //   element 0   = magnitude min/max
        //   element 1+j = component j min/max
        // IGV stores a flat array:
        //   components == 1 -> [min, max]
        //   components >  1 -> [magMin, magMax, c0Min, c0Max, ...]
        // The scalar UI and color mapping read element 1, so the scalar branch must fill
        // both element 0 and element 1.
        AttributeSet::Attribute& attr = attrSet->GetAttribute(attrIndex);
        if (!attr.IsNone()) {
            DoubleArray::Pointer range = DoubleArray::New();
            range->SetDimension(2);
            range->Resize(static_cast<IGsize>(pa.components) + 1);
            if (pa.components == 1) {
                if (pa.range.size() >= 2) {
                    double lo = pa.range[0];
                    double hi = pa.range[1];
                    if (lo > hi) { std::swap(lo, hi); }
                    const double absHi = std::max(std::fabs(lo), std::fabs(hi));
                    const double absLo = (lo <= 0.0 && hi >= 0.0)
                                                 ? 0.0
                                                 : std::min(std::fabs(lo), std::fabs(hi));
                    // element 0 = magnitude range, element 1 = raw component range.
                    range->SetElement(0, {absLo, absHi});
                    range->SetElement(1, {lo, hi});
                }
            } else if (pa.range.size() >= 2 * (1 + static_cast<size_t>(pa.components))) {
                range->SetElement(0, {pa.range[0], pa.range[1]});
                for (int j = 0; j < pa.components; ++j) {
                    range->SetElement(static_cast<IGsize>(j) + 1,
                                      {pa.range[2 + 2 * static_cast<size_t>(j)],
                                       pa.range[3 + 2 * static_cast<size_t>(j)]});
                }
            }
            attr.SetDataRange(range);
        }

        ++rebuiltAttributes;
        if (!m_Attributes.empty()) {
            this->UpdateProgress(
                    0.90 + 0.06 * static_cast<double>(rebuiltAttributes) /
                                   static_cast<double>(m_Attributes.size()));
        }
    }
    this->UpdateProgress(0.96);

    // 步骤 7b：Bounds 一致性自检（**不拒绝** —— 写出方可能是另一实现；
    if (m_Meta.hasBounds && m_Meta.pointCount > 0) {
        double bmin[3] = {std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max()};
        double bmax[3] = {std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest()};
        for (int64_t i = 0; i < m_Meta.pointCount; ++i) {
            const float* p = m_Points.data() + i * kPointsComponents;
            for (int a = 0; a < 3; ++a) {
                const double v = static_cast<double>(p[a]);
                if (v < bmin[a]) { bmin[a] = v; }
                if (v > bmax[a]) { bmax[a] = v; }
            }
        }
        for (int a = 0; a < 3; ++a) {
            const double fileMin = m_Meta.bounds[2 * a];
            const double fileMax = m_Meta.bounds[2 * a + 1];
            const double denom = std::max(1.0, std::max(std::fabs(fileMin), std::fabs(fileMax)));
            if (std::fabs(fileMin - bmin[a]) / denom > 1e-6 ||
                std::fabs(fileMax - bmax[a]) / denom > 1e-6) {
    IGAME_CORE_WARN("[IGV] Bounds disagrees with the computed bounding box (axis {}): file [{}, {}] vs computed [{}, {}]",
                                a, fileMin, fileMax, bmin[a], bmax[a]);
            }
        }
    }

    m_Output = obj;
    // 0.98：range 挂载与 Bounds 自检都已完成，只剩 CreateDataObject 收尾。
    this->UpdateProgress(0.98);
    return true;
}

bool IGVReader::CreateDataObject() {
    if (m_AssertHdf5Closed) {
        // 时序自检（R3）：到这一步 HDF5 句柄必须已全部关闭 ——
        // 本实现不得依赖"Close() 之后还能读 HDF5"。
  IGAME_CORE_INFO("[IGV] CreateDataObject() called with all HDF5 handles already closed"
                        " (SetAssertHdf5ClosedInCreateDataObject(true))");
    }
    if (!RebuildFromParsed()) {
        return false;
    }
    // 与 VTU/VTP/CGNS 等既有 Reader 一致：成功时把进度推到 1.0，
    // 之后由 FileReader::Execute() 统一复位（resetProgressUI）。
    this->UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
