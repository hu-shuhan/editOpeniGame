#ifndef iGameIGVHdf5Util_h
#define iGameIGVHdf5Util_h

/**
 * @file  iGameIGVHdf5Util.h
 * @brief IGV 的 HDF5 底层小工具集合（attribute / dataset / dtype / chunk / filter）。
 *
 * 设计约束（见 kia/igv分阶段实现方案.md §4.2、§4.4、§13 第 7 条）：
 *   **HDF5 头文件只允许出现在 IGV 的 .cpp 中**，公共头一律不得 include <hdf5.h>
 *   —— 否则 HDF5 的类型/宏会污染所有既有编译单元。
 *
 *   因此本文件**只允许被 iGameIGVWriter.cpp / iGameIGVReader.cpp /
 *   （将来的）iGameIGVMetadataReader.cpp 这类 .cpp include**，不得被任何
 *   对外头文件 include。文件名用 .h 而非 .cpp 只是为了共享实现，
 *   职责上等同于"私有实现细节"。
 *
 * 与 iGameIGVFormat.h 的分工：
 *   iGameIGVFormat.h —— **字符串与数值常量**（纯数据，无 HDF5）
 *   iGameIGVHdf5Util.h —— **HDF5 调用包装**（只有函数，无规范常量）
 *
 * 为什么要有这一层（而不是在 writer/reader 里直接调 HDF5）：
 *   1. 每个 HDF5 C API 都有 hid_t 的 close 语义与错误栈处理，散落调用必然漏 close
 *      （§5.4 要求"HDF5 任一调用失败逐层 H5Eclear 后 return false"）。
 *   2. P3 的 MetadataReader 需要"只读 attribute、绝不碰 dataset"这条硬约束
 *      （§7.2）。把 attribute 读取集中在一处，这条约束才可能被审计与测试。
 *   3. P3 明确建议把底层小工具共享、只让流程编排独立（§9 P3 "为什么它必须
 *      独立于 IGVReader"）。
 */

#include <hdf5.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "iGameIGVFormat.h"

IGAME_NAMESPACE_BEGIN

namespace igv {

// =============================================================================
// 0. 计数器（把"零 H5Dread"这类设计约束变成可断言事实）
// =============================================================================

/**
 * §11.3 要求的可注入计数器。
 *
 * `IGVReader` 与 `IGVMetadataReader` 各自在 `H5Dopen` / `H5Dread` 处递增。
 * P3 的 V3.2 断言 `ReadIGVMetadata` 期间 `h5dreadCount == 0 && datasetOpenCount == 0`
 * —— 若将来有人为了"方便"在快路径里读了一下 `Points`，测试会立刻红。
 */
struct IGVStats {
    static std::atomic<uint64_t> h5dreadCount;      ///< H5Dread 调用次数
    static std::atomic<uint64_t> datasetOpenCount;  ///< H5Dopen 调用次数
    static std::atomic<uint64_t> h5areadCount;      ///< H5Aread 调用次数

    static void Reset() {
        h5dreadCount.store(0, std::memory_order_relaxed);
        datasetOpenCount.store(0, std::memory_order_relaxed);
        h5areadCount.store(0, std::memory_order_relaxed);
    }
};

inline std::atomic<uint64_t> IGVStats::h5dreadCount{0};
inline std::atomic<uint64_t> IGVStats::datasetOpenCount{0};
inline std::atomic<uint64_t> IGVStats::h5areadCount{0};

// =============================================================================
// 1. 错误处理
// =============================================================================

/// 清空 HDF5 错误栈。
///
/// §5.4：HDF5 任一调用失败要"逐层 H5Eclear 后 return false，不透传 HDF5 错误栈
/// 到上层日志"。HDF5 的错误栈是全局的，不清会在日志里刷出大量与本次失败无关的
/// 历史错误，掩盖真实原因。
inline void ClearHdf5Errors() {
    H5Eclear2(H5E_DEFAULT);
}

// =============================================================================
// 2. RAII 句柄
// =============================================================================

/// 通用 hid_t RAII 包装。避免每条 return false 路径都要记得 close。
template <herr_t (*CloseFn)(hid_t)>
class H5Handle {
public:
    H5Handle() = default;
    explicit H5Handle(hid_t id) : m_Id(id) {}
    ~H5Handle() { Reset(); }

    H5Handle(const H5Handle&) = delete;
    H5Handle& operator=(const H5Handle&) = delete;

    H5Handle(H5Handle&& other) noexcept : m_Id(other.m_Id) { other.m_Id = kInvalid; }
    H5Handle& operator=(H5Handle&& other) noexcept {
        if (this != &other) {
            Reset();
            m_Id = other.m_Id;
            other.m_Id = kInvalid;
        }
        return *this;
    }

    void Reset(hid_t id = kInvalid) {
        if (m_Id != kInvalid && m_Id >= 0) { CloseFn(m_Id); }
        m_Id = id;
    }

    [[nodiscard]] hid_t Get() const { return m_Id; }
    [[nodiscard]] bool IsValid() const { return m_Id != kInvalid && m_Id >= 0; }

    /// 释放所有权（调用方负责 close）
    hid_t Release() {
        const hid_t id = m_Id;
        m_Id = kInvalid;
        return id;
    }

    static constexpr hid_t kInvalid = -1;

private:
    hid_t m_Id{kInvalid};
};

using H5FileHandle = H5Handle<H5Fclose>;
using H5GroupHandle = H5Handle<H5Gclose>;
using H5DatasetHandle = H5Handle<H5Dclose>;
using H5AttributeHandle = H5Handle<H5Aclose>;
using H5DataspaceHandle = H5Handle<H5Sclose>;
using H5TypeHandle = H5Handle<H5Tclose>;
using H5PropertyHandle = H5Handle<H5Pclose>;

/** 属性读取的最大长度。规范里的字符串 attribute 都是短的（Type/IGVDataType/
 *  CellTypeEncoding/attachment/AttributeType），128 字节有充足余量。 */
inline constexpr size_t kMaxAsciiAttributeBytes = 128;

// =============================================================================
// 3. dtype：枚举在 iGameIGVFormat.h（纯 C++），这里只做 → hid_t 的映射
// =============================================================================
//
// DType / DTypeName / SizeOfDType 定义在 iGameIGVFormat.h —— 那样公共头
// （如 iGameIGVReader.h）可以在不引入 hdf5.h 的前提下按 dtype 分派缓冲。

/// 文件侧（写入用）的定宽小端类型。
[[nodiscard]] inline hid_t FileTypeOf(DType t) {
    switch (t) {
        case DType::F32: return H5T_IEEE_F32LE;
        case DType::F64: return H5T_IEEE_F64LE;
        case DType::U8:  return H5T_STD_U8LE;
        case DType::I32: return H5T_STD_I32LE;
        case DType::I64: return H5T_STD_I64LE;
        default:         return H5T_NATIVE_INT;  // 不应到达；由调用方先校验
    }
}

/// 内存侧（读写用）的 native 类型。§3.5.0："读入按 H5T_NATIVE_* 匹配，
/// 规避 long/int 平台宽度歧义"。
[[nodiscard]] inline hid_t NativeTypeOf(DType t) {
    switch (t) {
        case DType::F32: return H5T_NATIVE_FLOAT;
        case DType::F64: return H5T_NATIVE_DOUBLE;
        case DType::U8:  return H5T_NATIVE_UCHAR;
        case DType::I32: return H5T_NATIVE_INT32;
        case DType::I64: return H5T_NATIVE_INT64;
        default:         return H5T_NATIVE_INT;
    }
}

/**
 * 把文件名侧类型识别成 `DType`。
 *
 * **必须做严格识别，不能"看起来是 4 字节就当 float"**：若写出方用了
 * `H5T_NATIVE_FLOAT`（非定宽）或大端，读取侧必须拒绝而不是照读 ——
 * 否则会在不同平台上静默得到字节序错误的数值。
 *
 * @param requireLittleEndian  为 true 时要求端序必须是小端（读侧默认 true）
 */
[[nodiscard]] inline bool DTypeFromFileType(hid_t typeId, DType& out,
                                            bool requireLittleEndian = true) {
    if (typeId < 0) { return false; }

    const H5T_class_t cls = H5Tget_class(typeId);
    const size_t size = H5Tget_size(typeId);
    const H5T_sign_t sign = (cls == H5T_INTEGER) ? H5Tget_sign(typeId) : H5T_SGN_NONE;

    H5T_order_t order = H5T_ORDER_NONE;
    if (cls == H5T_FLOAT || cls == H5T_INTEGER) { order = H5Tget_order(typeId); }

    // 端序：小端要求（§3.3）。
    // 注意 H5Tget_order 对"同 native"的类型可能返回 H5T_ORDER_NONE（表示无端序概念，
    // 例如某些虚拟类型）。H5T_IEEE_F32LE / H5T_STD_I32LE 明确返回 LE。
    if (requireLittleEndian && order != H5T_ORDER_LE && order != H5T_ORDER_NONE) {
        return false;
    }

    if (cls == H5T_FLOAT) {
        if (size == 4) { out = DType::F32; return true; }
        if (size == 8) { out = DType::F64; return true; }
        return false;
    }
    if (cls == H5T_INTEGER) {
        if (sign == H5T_SGN_NONE) {
            if (size == 1) { out = DType::U8; return true; }
            return false;  // 无符号 i32/i64 不在规范内
        }
        if (size == 4) { out = DType::I32; return true; }
        if (size == 8) { out = DType::I64; return true; }
        return false;
    }
    return false;
}

// =============================================================================
// 4. 定长 ASCII attribute（§3.3：定长 ASCII + H5T_STR_NULLPAD）
// =============================================================================

/// 建一个定长 ASCII 类型（NULLPAD，与 VTKHDF / P0 golden 样例一致）。
[[nodiscard]] inline H5TypeHandle MakeAsciiType(size_t fixedSize) {
    if (fixedSize == 0) { fixedSize = 1; }
    H5TypeHandle t(H5Tcopy(H5T_C_S1));
    if (!t.IsValid()) { return t; }
    if (H5Tset_size(t.Get(), fixedSize) < 0) { return H5TypeHandle{}; }
    if (H5Tset_strpad(t.Get(), H5T_STR_NULLPAD) < 0) { return H5TypeHandle{}; }
    if (H5Tset_cset(t.Get(), H5T_CSET_ASCII) < 0) { return H5TypeHandle{}; }
    return t;
}

/**
 * 写文本 attribute（定长 ASCII / NULLPAD）。
 *
 * 这是零解压元数据的载体：attribute 存在 HDF5 对象头里，**永不压缩**
 * （§3.3），因此读取时不需要任何 dataset 解压。
 */
inline bool WriteAsciiAttribute(hid_t loc, std::string_view name, std::string_view value) {
    // 规范里的 attribute 值都以 '\0' 结尾的定长串存储；长度取"包括结尾 NUL"
    // 与实际内容长度的较大者，保证 reader 侧按字符串读时一定能看到结尾。
    const size_t textLen = value.size();
    const size_t fixedSize = (textLen == 0) ? 1 : textLen;  // 不含 NUL 恰好占满，靠 NULLPAD 语义读

    H5TypeHandle type = MakeAsciiType(fixedSize);
    if (!type.IsValid()) { ClearHdf5Errors(); return false; }

    const std::string nameStr(name);
    H5DataspaceHandle space(H5Screate(H5S_SCALAR));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    H5AttributeHandle attr(H5Acreate2(loc, nameStr.c_str(), type.Get(), space.Get(),
                                     H5P_DEFAULT, H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    // H5T_STR_NULLPAD + 定长 size：写入的缓冲必须正好 size 字节。
    std::vector<char> buf(fixedSize, '\0');
    if (textLen > 0) { std::memcpy(buf.data(), value.data(), textLen); }

    if (H5Awrite(attr.Get(), type.Get(), buf.data()) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

/// 写 int64 标量 attribute。
inline bool WriteI64Attribute(hid_t loc, std::string_view name, int64_t value) {
    const std::string nameStr(name);
    H5DataspaceHandle space(H5Screate(H5S_SCALAR));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    H5AttributeHandle attr(H5Acreate2(loc, nameStr.c_str(), H5T_STD_I64LE, space.Get(),
                                     H5P_DEFAULT, H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    if (H5Awrite(attr.Get(), H5T_NATIVE_INT64, &value) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

/// 写 float64 标量 attribute。
inline bool WriteF64Attribute(hid_t loc, std::string_view name, double value) {
    const std::string nameStr(name);
    H5DataspaceHandle space(H5Screate(H5S_SCALAR));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    H5AttributeHandle attr(H5Acreate2(loc, nameStr.c_str(), H5T_IEEE_F64LE, space.Get(),
                                     H5P_DEFAULT, H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    if (H5Awrite(attr.Get(), H5T_NATIVE_DOUBLE, &value) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

/// 写一维定长数组 attribute（文件侧用定宽小端，内存侧用 native）。
inline bool WriteArrayAttribute(hid_t loc, std::string_view name, DType fileType,
                               const void* data, size_t count) {
    if (data == nullptr || count == 0) { return false; }

    const std::string nameStr(name);
    const hsize_t dims[1] = {static_cast<hsize_t>(count)};
    H5DataspaceHandle space(H5Screate_simple(1, dims, nullptr));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    H5AttributeHandle attr(H5Acreate2(loc, nameStr.c_str(), FileTypeOf(fileType), space.Get(),
                                     H5P_DEFAULT, H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    if (H5Awrite(attr.Get(), NativeTypeOf(fileType), data) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

/// 写 int64 数组 attribute。
inline bool WriteI64ArrayAttribute(hid_t loc, std::string_view name,
                                   const int64_t* data, size_t count) {
    if (data == nullptr || count == 0) { return false; }
    return WriteArrayAttribute(loc, name, DType::I64, data, count);
}

// =============================================================================
// 5. attribute 读取（只碰 attribute，绝不碰 dataset —— §7.2）
// =============================================================================

/**
 * 读文本 attribute。
 *
 * 同时兼容两种写法，因为**读取侧必须能读外部实现者写出的合规文件**：
 *   1. 定长 ASCII（`H5T_STRING` + `H5T_STR_NULLPAD`）—— 规范要求，我们自己也这么写；
 *   2. h5py 常见写法（`H5T_VARIABLE` 长度 ASCII）—— P0 的 golden 样例由 h5py 产出，
 *      虽然实测 h5py 在本机产出的是定长，但外部工具的写法不应成为拒绝理由。
 *
 * 不做转换直接按 native 读会失败或截断，因此这里显式分派。
 */
inline bool ReadAsciiAttribute(hid_t loc, std::string_view name, std::string& out) {
    const std::string nameStr(name);
    if (H5Aexists(loc, nameStr.c_str()) <= 0) { return false; }

    H5AttributeHandle attr(H5Aopen(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    H5TypeHandle type(H5Aget_type(attr.Get()));
    if (!type.IsValid()) { ClearHdf5Errors(); return false; }

    if (H5Tget_class(type.Get()) != H5T_STRING) { ClearHdf5Errors(); return false; }

    out.clear();

    if (H5Tis_variable_str(type.Get()) > 0) {
        char* raw = nullptr;
        IGVStats::h5areadCount.fetch_add(1, std::memory_order_relaxed);
        if (H5Aread(attr.Get(), type.Get(), &raw) < 0) { ClearHdf5Errors(); return false; }
        if (raw != nullptr) {
            out.assign(raw);
            H5free_memory(raw);
        }
        return true;
    }

    const size_t size = H5Tget_size(type.Get());
    if (size == 0 || size > kMaxAsciiAttributeBytes) { ClearHdf5Errors(); return false; }

    std::vector<char> buf(size + 1, '\0');
    IGVStats::h5areadCount.fetch_add(1, std::memory_order_relaxed);
    if (H5Aread(attr.Get(), type.Get(), buf.data()) < 0) { ClearHdf5Errors(); return false; }

    // 无论 NULLPAD / NULLTERM / SPACEPAD，都在第一个 NUL 处截断；
    // SPACEPAD 时再去掉尾部空格。
    out.assign(buf.data(), strnlen(buf.data(), size));
    while (!out.empty() && out.back() == ' ') { out.pop_back(); }
    return true;
}

/// 读 int64 标量 attribute。
inline bool ReadI64Attribute(hid_t loc, std::string_view name, int64_t& out) {
    const std::string nameStr(name);
    if (H5Aexists(loc, nameStr.c_str()) <= 0) { return false; }

    H5AttributeHandle attr(H5Aopen(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    IGVStats::h5areadCount.fetch_add(1, std::memory_order_relaxed);
    if (H5Aread(attr.Get(), H5T_NATIVE_INT64, &out) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

/// 读 float64 标量 attribute。
inline bool ReadF64Attribute(hid_t loc, std::string_view name, double& out) {
    const std::string nameStr(name);
    if (H5Aexists(loc, nameStr.c_str()) <= 0) { return false; }

    H5AttributeHandle attr(H5Aopen(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    IGVStats::h5areadCount.fetch_add(1, std::memory_order_relaxed);
    if (H5Aread(attr.Get(), H5T_NATIVE_DOUBLE, &out) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

/// attribute 的元素个数（标量返回 1）。
inline bool AttributeElementCount(hid_t loc, std::string_view name, hsize_t& out) {
    const std::string nameStr(name);
    if (H5Aexists(loc, nameStr.c_str()) <= 0) { return false; }

    H5AttributeHandle attr(H5Aopen(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    H5DataspaceHandle space(H5Aget_space(attr.Get()));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    out = H5Sget_simple_extent_npoints(space.Get());
    return true;
}

/**
 * 读 float64 数组 attribute。
 *
 * @param expectedCount  期望元素数；0 表示不校验
 * @param out            读到的值
 * @return 成功且元素数符合期望时为 true
 */
inline bool ReadF64ArrayAttribute(hid_t loc, std::string_view name,
                                  size_t expectedCount, std::vector<double>& out) {
    const std::string nameStr(name);
    if (H5Aexists(loc, nameStr.c_str()) <= 0) { return false; }

    H5AttributeHandle attr(H5Aopen(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    H5DataspaceHandle space(H5Aget_space(attr.Get()));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    const hsize_t n = H5Sget_simple_extent_npoints(space.Get());
    if (expectedCount != 0 && n != static_cast<hsize_t>(expectedCount)) {
        ClearHdf5Errors();
        return false;
    }

    out.assign(static_cast<size_t>(n), 0.0);
    if (n == 0) { return true; }

    IGVStats::h5areadCount.fetch_add(1, std::memory_order_relaxed);
    if (H5Aread(attr.Get(), H5T_NATIVE_DOUBLE, out.data()) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

/// 读 int64 数组 attribute。
inline bool ReadI64ArrayAttribute(hid_t loc, std::string_view name,
                                  size_t expectedCount, std::vector<int64_t>& out) {
    const std::string nameStr(name);
    if (H5Aexists(loc, nameStr.c_str()) <= 0) { return false; }

    H5AttributeHandle attr(H5Aopen(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!attr.IsValid()) { ClearHdf5Errors(); return false; }

    H5DataspaceHandle space(H5Aget_space(attr.Get()));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    const hsize_t n = H5Sget_simple_extent_npoints(space.Get());
    if (expectedCount != 0 && n != static_cast<hsize_t>(expectedCount)) {
        ClearHdf5Errors();
        return false;
    }

    out.assign(static_cast<size_t>(n), 0);
    if (n == 0) { return true; }

    IGVStats::h5areadCount.fetch_add(1, std::memory_order_relaxed);
    if (H5Aread(attr.Get(), H5T_NATIVE_INT64, out.data()) < 0) { ClearHdf5Errors(); return false; }
    return true;
}

// =============================================================================
// 6. dataspace / 类型探查（**不打开 dataset 的数据**）
// =============================================================================
//
// 注意区分：
//   * `PeekDataset*` 系列会 H5Dopen —— 因此**不能**出现在元数据快路径里
//     （§7.2 禁止 H5Dopen）。它们只服务 IGVReader 与测试的结构校验。
//   * `Attribute*` 系列只 H5Aopen，可以在快路径使用。

/// dataset 是否存在（用 H5Lexists，不打开）。
[[nodiscard]] inline bool DatasetExists(hid_t loc, std::string_view name) {
    const std::string nameStr(name);
    return H5Lexists(loc, nameStr.c_str(), H5P_DEFAULT) > 0;
}

/// 打开 dataset 并读它的 dataspace 维度。返回 rank；失败返回 -1。
inline int PeekDatasetShape(hid_t loc, std::string_view name, std::vector<hsize_t>& dims,
                           DType* outType = nullptr, bool* outHasDeflate = nullptr,
                           std::vector<hsize_t>* outChunk = nullptr) {
    const std::string nameStr(name);
    IGVStats::datasetOpenCount.fetch_add(1, std::memory_order_relaxed);

    H5DatasetHandle ds(H5Dopen2(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!ds.IsValid()) { ClearHdf5Errors(); return -1; }

    H5DataspaceHandle space(H5Dget_space(ds.Get()));
    if (!space.IsValid()) { ClearHdf5Errors(); return -1; }

    const int rank = H5Sget_simple_extent_ndims(space.Get());
    if (rank < 0) { ClearHdf5Errors(); return -1; }

    dims.assign(static_cast<size_t>(rank), 0);
    std::vector<hsize_t> maxDims(static_cast<size_t>(rank), 0);
    if (rank > 0) {
        if (H5Sget_simple_extent_dims(space.Get(), dims.data(), maxDims.data()) < 0) {
            ClearHdf5Errors();
            return -1;
        }
    }

    if (outType != nullptr) {
        H5TypeHandle type(H5Dget_type(ds.Get()));
        if (!type.IsValid()) { ClearHdf5Errors(); return -1; }
        DType t = DType::Unknown;
        if (!DTypeFromFileType(type.Get(), t)) { ClearHdf5Errors(); return -1; }
        *outType = t;
    }

    if (outHasDeflate != nullptr || outChunk != nullptr) {
        H5PropertyHandle dcpl(H5Dget_create_plist(ds.Get()));
        if (!dcpl.IsValid()) { ClearHdf5Errors(); return -1; }

        if (outHasDeflate != nullptr) {
            bool found = false;
            const int nFilters = H5Pget_nfilters(dcpl.Get());
            for (int i = 0; i < nFilters; ++i) {
                unsigned int flags = 0;
                size_t cdNelmts = 0;
                const H5Z_filter_t f = H5Pget_filter2(dcpl.Get(), static_cast<unsigned>(i),
                                                     &flags, &cdNelmts, nullptr, 0, nullptr, nullptr);
                if (f == H5Z_FILTER_DEFLATE) { found = true; break; }
            }
            *outHasDeflate = found;
        }

        if (outChunk != nullptr) {
            const int layout = H5Pget_layout(dcpl.Get());
            if (layout == H5D_CHUNKED) {
                outChunk->assign(static_cast<size_t>(rank == 0 ? 1 : rank), 0);
                if (H5Pget_chunk(dcpl.Get(), rank, outChunk->data()) < 0) {
                    ClearHdf5Errors();
                    return -1;
                }
            } else {
                outChunk->clear();
            }
        }
    }

    return rank;
}

/// 读 dataset 的当前维度与最大维度（用于 §3.5.0 的「固定 dataspace」断言）。
inline bool ReadDatasetDimsAndMaxDims(hid_t loc, std::string_view name,
                                      std::vector<hsize_t>& dims,
                                      std::vector<hsize_t>& maxDims) {
    const std::string nameStr(name);
    IGVStats::datasetOpenCount.fetch_add(1, std::memory_order_relaxed);

    H5DatasetHandle ds(H5Dopen2(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!ds.IsValid()) { ClearHdf5Errors(); return false; }

    H5DataspaceHandle space(H5Dget_space(ds.Get()));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    const int rank = H5Sget_simple_extent_ndims(space.Get());
    if (rank < 0) { ClearHdf5Errors(); return false; }

    dims.assign(static_cast<size_t>(rank), 0);
    maxDims.assign(static_cast<size_t>(rank), 0);
    if (rank > 0) {
        if (H5Sget_simple_extent_dims(space.Get(), dims.data(), maxDims.data()) < 0) {
            ClearHdf5Errors();
            return false;
        }
    }
    return true;
}

// =============================================================================
// 7. dataset 写入（chunk + deflate）
// =============================================================================

/// chunk 尺寸策略（§5.3）。
///
/// ```
/// rank == 1                      → { min(n, kChunkRows) }
/// rank == 2 && dims[1] is record → { min(dims[0], kTargetRows), dims[1] }
/// 其中 kTargetRows 取使 chunk 字节数接近 kChunkTargetBytes 的值。
/// 小 dataset（未压缩字节数 < kChunkMinBytes）→ 不 chunk、不压缩。
/// ```
///
/// 阈值是 P7 之前使用的**初值**（§5.3 末注），由 P7-1…P7-3 实测回调。
inline constexpr size_t kChunkTargetBytes = 1024 * 1024;  ///< 1 MiB
inline constexpr size_t kChunkMinBytes = 4 * 1024;        ///< 4 KiB
inline constexpr unsigned kDeflateLevel = 1;              ///< A4：默认 gzip(1)

/// 计算 chunk 维度；返回空表示"不 chunk"。
[[nodiscard]] inline std::vector<hsize_t> ComputeChunkDims(const std::vector<hsize_t>& dims,
                                                          size_t elemBytes) {
    if (dims.empty() || elemBytes == 0) { return {}; }

    size_t totalBytes = elemBytes;
    for (hsize_t d : dims) {
        if (d == 0) { return {}; }  // 空 dataset 不 chunk
        totalBytes *= static_cast<size_t>(d);
    }
    if (totalBytes < kChunkMinBytes) { return {}; }

    const size_t recordBytes = elemBytes * (dims.size() > 1 ? static_cast<size_t>(dims[1]) : 1);
    if (recordBytes == 0) { return {}; }

    size_t rows = kChunkTargetBytes / recordBytes;
    if (rows == 0) { rows = 1; }
    if (rows > static_cast<size_t>(dims[0])) { rows = static_cast<size_t>(dims[0]); }

    std::vector<hsize_t> chunk(static_cast<size_t>(dims.size()), 0);
    chunk[0] = static_cast<hsize_t>(rows);
    // 沿元素维切分，**不得切断单条记录**（§3.3 chunk 约定）
    for (size_t i = 1; i < dims.size(); ++i) { chunk[i] = dims[i]; }
    return chunk;
}

/**
 * 写一个 dataset。
 *
 * @param dims        维度（当前维度，§A16 固定 dataspace：maxdims == dims）
 * @param elemCount   元素总数（= dims 的乘积）
 * @param data        内存数据（native 类型，HDF5 自动做端序转换）
 * @param compress    是否使用 gzip
 */
inline bool WriteDataset(hid_t loc, std::string_view name, DType dtype,
                         const std::vector<hsize_t>& dims, const void* data,
                         bool compress) {
    const std::string nameStr(name);
    const size_t bytes = SizeOfDType(dtype);

    // A16：本期用**固定** dataspace —— maxdims 与 dims 相同，且不含 H5S_UNLIMITED。
    H5DataspaceHandle space;
    if (dims.empty()) {
        space.Reset(H5Screate(H5S_SCALAR));
    } else {
        space.Reset(H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr));
    }
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    H5PropertyHandle dcpl(H5Pcreate(H5P_DATASET_CREATE));
    if (!dcpl.IsValid()) { ClearHdf5Errors(); return false; }

    const std::vector<hsize_t> chunk = compress ? ComputeChunkDims(dims, bytes)
                                               : std::vector<hsize_t>{};
    if (!chunk.empty()) {
        if (H5Pset_chunk(dcpl.Get(), static_cast<int>(chunk.size()), chunk.data()) < 0) {
            ClearHdf5Errors();
            return false;
        }
        if (H5Pset_deflate(dcpl.Get(), kDeflateLevel) < 0) { ClearHdf5Errors(); return false; }
    }

    H5DatasetHandle ds(H5Dcreate2(loc, nameStr.c_str(), FileTypeOf(dtype), space.Get(),
                                  H5P_DEFAULT, dcpl.Get(), H5P_DEFAULT));
    if (!ds.IsValid()) { ClearHdf5Errors(); return false; }

    if (data != nullptr) {
        if (H5Dwrite(ds.Get(), NativeTypeOf(dtype), H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
            ClearHdf5Errors();
            return false;
        }
    }
    return true;
}

/// 写一个元素数为 1 的 i64 dataset（三个计数组）。
inline bool WriteCountDataset(hid_t loc, std::string_view name, int64_t value) {
    const hsize_t dims[1] = {1};
    return WriteDataset(loc, name, DType::I64,
                        std::vector<hsize_t>(dims, dims + 1), &value, /*compress=*/false);
}

/**
 * 回读某个 dataset 的**实际落盘元素数**（§5.2）。
 *
 * §5.2 强制要求：计数组必须用"回读实际维度"填充，不能用写前算好的期望值。
 * 一旦写前计算与实际写出不一致（例如属性被静默截断），用期望值填的计数组会
 * 「自洽地错」，读取侧的交叉校验（§6.3）将无法发现问题。
 *
 * ⚠ 语义（曾写错，导致五类网格**全部**写出失败）：
 *   `NumberOfPoints` 是 **Points 的第一维**（点数），**不是** dims 的乘积。
 *   Points 的 dataspace 是 `(n, 3)`，乘积 `n*3` 是浮点标量个数而非点数
 *   —— 于是 9 个点被回读成 27、18 个点被回读成 54、60 个点被回读成 180，
 *   与写前值不符即被自检拒绝。
 *   同理：`NumberOfCells` 对应 Types 的唯一一维、`NumberOfConnectivityIds`
 *   对应 Connectivity 的唯一一维。
 *
 *   本函数只服务计数组的数据来源，其中 Points 是唯一的 rank-2 例外，
 *   因此明确规定：**取第一维**；rank-0（标量 dataspace）返回 1。
 */
inline bool ReadbackElementCount(hid_t loc, std::string_view name, int64_t& out) {
    std::vector<hsize_t> dims;
    if (PeekDatasetShape(loc, name, dims) < 0) { return false; }

    if (dims.empty()) {
        out = 1;  // 标量 dataspace
        return true;
    }
    // 只取第一维：Points (n,3) 的"point count"是 n，不是 n*3。
    out = static_cast<int64_t>(dims[0]);
    return true;
}

// =============================================================================
// 8. dataset 读取
// =============================================================================

/// 读一个 dataset 的全部元素到 `out`（按 native 类型）。
///
/// @param expectedElemCount 期望元素数；0 表示不校验
template <typename T>
inline bool ReadDataset(hid_t loc, std::string_view name, DType expectedType,
                        size_t expectedElemCount, std::vector<T>& out) {
    const std::string nameStr(name);
    IGVStats::datasetOpenCount.fetch_add(1, std::memory_order_relaxed);

    H5DatasetHandle ds(H5Dopen2(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!ds.IsValid()) { ClearHdf5Errors(); return false; }

    H5TypeHandle type(H5Dget_type(ds.Get()));
    if (!type.IsValid()) { ClearHdf5Errors(); return false; }

    DType actual = DType::Unknown;
    if (!DTypeFromFileType(type.Get(), actual)) { ClearHdf5Errors(); return false; }
    if (actual != expectedType) { ClearHdf5Errors(); return false; }

    H5DataspaceHandle space(H5Dget_space(ds.Get()));
    if (!space.IsValid()) { ClearHdf5Errors(); return false; }

    const hsize_t n = H5Sget_simple_extent_npoints(space.Get());
    if (expectedElemCount != 0 && n != static_cast<hsize_t>(expectedElemCount)) {
        ClearHdf5Errors();
        return false;
    }

    out.assign(static_cast<size_t>(n), T{});
    if (n == 0) { return true; }

    IGVStats::h5dreadCount.fetch_add(1, std::memory_order_relaxed);
    if (H5Dread(ds.Get(), NativeTypeOf(expectedType), H5S_ALL, H5S_ALL, H5P_DEFAULT,
                out.data()) < 0) {
        ClearHdf5Errors();
        return false;
    }
    return true;
}

/// 就地改写 dataset 中的一个元素（仅供测试的"畸形文件"制作使用，产品路径不用）。
inline bool WriteDatasetElementI64(hid_t loc, std::string_view name, size_t index, int64_t value) {
    const std::string nameStr(name);
    IGVStats::datasetOpenCount.fetch_add(1, std::memory_order_relaxed);

    H5DatasetHandle ds(H5Dopen2(loc, nameStr.c_str(), H5P_DEFAULT));
    if (!ds.IsValid()) { ClearHdf5Errors(); return false; }

    H5DataspaceHandle fileSpace(H5Dget_space(ds.Get()));
    if (!fileSpace.IsValid()) { ClearHdf5Errors(); return false; }

    const hsize_t start[1] = {static_cast<hsize_t>(index)};
    const hsize_t count[1] = {1};
    if (H5Sselect_hyperslab(fileSpace.Get(), H5S_SELECT_SET, start, nullptr, count, nullptr) < 0) {
        ClearHdf5Errors();
        return false;
    }

    H5DataspaceHandle memSpace(H5Screate_simple(1, count, nullptr));
    if (!memSpace.IsValid()) { ClearHdf5Errors(); return false; }

    if (H5Dwrite(ds.Get(), H5T_NATIVE_INT64, memSpace.Get(), fileSpace.Get(), H5P_DEFAULT,
                 &value) < 0) {
        ClearHdf5Errors();
        return false;
    }
    return true;
}

} // namespace igv

IGAME_NAMESPACE_END

#endif // iGameIGVHdf5Util_h
