/**
 * @file  TestIGVRobustness.cpp
 * @brief IGV P5 验收程序（一）：畸形文件必须**被拒绝**，且进程不崩溃、不 OOM。
 *
 * 对应 kia/igv分阶段实现方案.md §9「P5 — 鲁棒性与大文件回归」的 **P5-1…P5-11、P5-14**，
 * 以及 §6.3「读取侧鲁棒性」全表。记录见 kia/P5实施记录.md。
 *
 * P6 阶段（「不静默丢数据」的拒绝路径）在本程序里新增了 **P6-1…P6-6** 一节，
 * 记录见 kia/P6实施记录.md。新增该节的原因是一个具体的危险：含 `Block_*`（多块）或
 * `Steps`（单文件内嵌时变）的文件，若读取器只把 `/IGV` 自己的几何当整体返回，
 * **不会报错、却只给了用户一部分数据** —— 这种"部分成功"比拒绝危险得多。
 * 同一条危险在写侧的表现是"五类网格之一自带子块，写出时只写父对象"。
 * 该节因此同时钉住：读侧拒绝并返回 nullptr、`Block_` 前缀只认**组**不认**属性名**、
 * 以及格式表里**没有**为 MultiBlock 预留 `Type` 取值。
 *
 * ============================================================================
 * 为什么需要这个程序（要保证的正确行为）
 * ============================================================================
 *
 * P1 的读取器把 §6.3 的检查写进了代码，但**"写了 if 判断"不等于"校验有效"**
 * （测试准则 §2.6）。本程序的做法是：拿一份**合法**的 `.igv` 当种子，用 HDF5 C API
 * 精确地把它改成"只有一处是错的"，逐个喂给 `IGVReader`，断言：
 *
 *   1. `ReadFile()` 返回 `nullptr`（不是崩溃、不是"部分成功"）；
 *   2. `GetLastError()` 里含**能定位问题**的关键字（属性名 / 越界位置 / 实际值 …）；
 *   3. 进程还活着（能继续跑下一个用例）—— 这一条对 OOM 类缺陷才有意义。
 *
 * 边界用例（为什么必须有）：
 *   - **截断 10%/50%/99%**：HDF5 的失败模式随截断位置不同（超级块坏 / 元数据坏 /
 *     数据块缺），不能只测一个比例。
 *   - **`2^33` 元素的 dataspace**：`MulWouldOverflow` 只挡"乘出来超过 int64"，
 *     挡不住"算得出来但显然不可能"。不修的话 `vector::resize` 会先申请 34 GB，
 *     进程被 OOM 杀掉 —— 而这正是"拒绝失败"最隐蔽的形态（没有返回 false，是**死掉**）。
 *     判据用**物理**比例（声明字节数 / 文件里已分配字节数 ≤ 4096，deflate 的物理上限
 *     约 1032:1），不引入拍脑袋的阈值。
 *   - **属性长度 ≠ 点数**：属性自己的 dataspace 自洽，跨对象的一致性原先只有写侧把关；
 *     §6.3 要求读侧也拒绝。
 *   - **`PointData` 下名为 `Block_0` 的属性**（P6-3）：判块必须看"是不是组"，
 *     不能只看名字前缀，否则一份合法文件会被误拒。
 *
 * 明确**不**声称覆盖：
 *   - >4 GiB 与亿级单元：属 P5-12 / P5-13，见 `TestIGVLargeFile.cpp`（单独门控）。
 *   - 真有内存破坏（ASAN）类问题：本机 MSVC Release 不带 ASAN。fuzz 只能证明"没崩"，
 *     不能证明"没有未定义行为"。
 *   - 并发读同一个文件：§15 R1 提到的 HDF5 线程安全属产品侧约束，不在此验证。
 *   - 多块 / 时变的**读写实现**（P6 只做拒绝路径，见 kia/未来任务.md F5 / F6）。
 *
 * 提交状态：待提交。测试与实现同次首次提交，因此无法记录自引用提交号；
 * 可用 `git log --diff-filter=A --format="%h %s" -- Examples/IO/TestIGVRobustness.cpp`
 * 查询引入本文件的提交（P6 一节的提交主题："add: igv p6 reject path"）。
 */

#include <IGV/iGameIGVMetadataReader.h>
#include <IGV/iGameIGVReader.h>
#include <IGV/iGameIGVWriter.h>

#include <iGameAttributeSet.h>
#include <iGameCellType.h>
#include <iGameDrawObject.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>

#include <hdf5.h>

#include <Log/iGameLogger.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

using namespace iGame;
namespace fs = std::filesystem;

int g_checks = 0;
int g_failed = 0;

// =============================================================================
// 输出与断言（控制台一律英文 ASCII，见 kia/测试准则.md §2.0）
// =============================================================================

void Section(const std::string& name) {
    std::cerr << "\n------------------------------------------------------------------------------\n"
              << name << "\n"
              << "------------------------------------------------------------------------------\n";
}

void Check(bool ok, const std::string& what) {
    ++g_checks;
    std::cerr << (ok ? " [ ok ] " : " [FAIL] ") << what << "\n";
    if (!ok) { ++g_failed; }
}

void Info(const std::string& what) { std::cerr << " [info] " << what << "\n"; }

std::string Num(int64_t v) { return std::to_string(static_cast<long long>(v)); }

fs::path WorkDir() {
    static const fs::path dir = [] {
        fs::path d = fs::current_path() / "igv_robustness_tmp";
        std::error_code ec;
        fs::create_directories(d, ec);
        return d;
    }();
    return dir;
}

// =============================================================================
// HDF5 RAII
// =============================================================================

// HDF5 的不同对象要用不同的函数关闭，所以句柄带上"种类"标记，析构时按种类关闭。
enum class Obj { File, Group, Dataset, Dataspace, Datatype, Attribute, Property };

struct TypedHandle {
    hid_t id{-1};
    Obj kind{Obj::File};
    TypedHandle() = default;
    TypedHandle(hid_t h, Obj k) : id(h), kind(k) {}
    ~TypedHandle() { Reset(); }
    TypedHandle(const TypedHandle&) = delete;
    TypedHandle& operator=(const TypedHandle&) = delete;
    TypedHandle(TypedHandle&& o) noexcept : id(o.id), kind(o.kind) { o.id = -1; }
    TypedHandle& operator=(TypedHandle&& o) noexcept {
        if (this != &o) {
            Reset();
            id = o.id;
            kind = o.kind;
            o.id = -1;
        }
        return *this;
    }
    void Reset(hid_t h = -1) {
        if (id >= 0) {
            switch (kind) {
                case Obj::File: H5Fclose(id); break;
                case Obj::Group: H5Gclose(id); break;
                case Obj::Dataset: H5Dclose(id); break;
                case Obj::Dataspace: H5Sclose(id); break;
                case Obj::Datatype: H5Tclose(id); break;
                case Obj::Attribute: H5Aclose(id); break;
                case Obj::Property: H5Pclose(id); break;
            }
        }
        id = h;
    }
    bool Ok() const { return id >= 0; }
    hid_t Get() const { return id; }
};

/// 关掉 HDF5 自己的 stderr 报错打印。
///
/// fuzz 阶段会有成百上千次"注定失败"的读取，HDF5 默认会为每次失败往 stderr
/// 打印一整套调用栈 —— 那会把真正的断言输出淹掉，也会让 fuzz 慢一个数量级。
/// 读取器自己会 `H5Eclear`，但不会关掉自动打印，所以在测试里关。
void SilenceHdf5Errors() { H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr); }

// =============================================================================
// 文件级操作
// =============================================================================

bool CopyFileTo(const fs::path& from, const fs::path& to, std::string& error) {
    std::error_code ec;
    fs::remove(to, ec);
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        error = "copy failed: " + ec.message();
        return false;
    }
    return true;
}

std::vector<char> ReadAllBytes(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<char>{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool WriteAllBytes(const fs::path& p, const std::vector<char>& bytes) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) { return false; }
    if (!bytes.empty()) { out.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); }
    return out.good();
}

// =============================================================================
// 读回与断言
// =============================================================================

/// 读一个文件，返回是否被拒绝，并带回错误信息。
struct ReadOutcome {
    bool parsed{false};
    std::string error;
};

ReadOutcome TryRead(const fs::path& path) {
    ReadOutcome out;
    IGVReader::Pointer reader = IGVReader::New();
    DataObject::Pointer obj = reader->ReadFile(path.string());
    out.parsed = (obj != nullptr);
    out.error = reader->GetLastError();
    return out;
}

/// 从**内存**读同一份字节。
///
/// fuzz 用这条路而不是写临时文件，有两个理由：
///   1. 少一次磁盘往返，1000 次迭代快一个数量级；
///   2. 文件路径读会走基类 `FileReader::Open()` 的**文件映射**，而基类在
///      `Parsing()` 失败时**不调用 `Close()`**（见 主仓库问题.md 问题 3）——
///      映射泄漏会让该文件在进程退出前一直被锁住，于是"下一次覆盖同一个文件名"会失败。
///      内存路径不建任何映射，天然绕开这个上游缺陷。
ReadOutcome TryReadMemory(const std::vector<char>& bytes) {
    ReadOutcome out;
    IGVReader::Pointer reader = IGVReader::New();
    reader->SetMemoryBuffer(bytes.data(), bytes.size());
    out.parsed = reader->Execute();
    out.error = reader->GetLastError();
    return out;
}

/// 「必须拒绝且错误信息含关键字」的统一断言。
void ExpectReject(const std::string& what, const fs::path& path, const std::string& keyword) {
    const ReadOutcome out = TryRead(path);
    if (out.parsed) {
        Check(false, what + ": mutated file was ACCEPTED (it must be rejected)");
        return;
    }
    if (keyword.empty()) {
        Check(true, what + ": rejected");
        return;
    }
    const bool hasKeyword = out.error.find(keyword) != std::string::npos;
    Check(hasKeyword, what + ": rejected" + (hasKeyword ? "" : " but the message lacks '" + keyword +
                                                               "' - actual: " + out.error));
}

// =============================================================================
// 变异原语（全部基于 HDF5 C API，精确改一处）
// =============================================================================

struct RwFile {
    TypedHandle file;
    bool Open(const fs::path& p, std::string& error) {
        file.Reset(H5Fopen(p.string().c_str(), H5F_ACC_RDWR, H5P_DEFAULT));
        if (!file.Ok()) {
            error = "H5Fopen(RDWR) failed";
            return false;
        }
        return true;
    }
    hid_t Raw() const { return file.Get(); }
};

bool OpenObj(hid_t file, const char* path, TypedHandle& out, Obj kind, std::string& error) {
    out.Reset(H5Oopen(file, path, H5P_DEFAULT));
    if (!out.Ok()) {
        error = std::string("cannot open '") + path + "'";
        return false;
    }
    out.kind = kind;
    return true;
}

bool WriteI64Dataset(hid_t file, const char* path, const std::vector<int64_t>& values,
                     std::string& error) {
    TypedHandle ds(H5Dopen2(file, path, H5P_DEFAULT), Obj::Dataset);
    if (!ds.Ok()) {
        error = std::string("cannot open dataset '") + path + "'";
        return false;
    }
    if (H5Dwrite(ds.Get(), H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                 values.data()) < 0) {
        error = std::string("H5Dwrite failed on '") + path + "'";
        return false;
    }
    return true;
}

bool WriteI64Element(hid_t file, const char* path, hsize_t index, int64_t value,
                     std::string& error) {
    TypedHandle ds(H5Dopen2(file, path, H5P_DEFAULT), Obj::Dataset);
    if (!ds.Ok()) {
        error = std::string("cannot open dataset '") + path + "'";
        return false;
    }
    TypedHandle space(H5Dget_space(ds.Get()), Obj::Dataspace);
    if (!space.Ok() || H5Sselect_elements(space.Get(), H5S_SELECT_SET, 1, &index) < 0) {
        error = "cannot select element";
        return false;
    }
    TypedHandle mem(H5Screate(H5S_SCALAR), Obj::Dataspace);
    if (H5Dwrite(ds.Get(), H5T_NATIVE_INT64, mem.Get(), space.Get(), H5P_DEFAULT, &value) < 0) {
        error = "H5Dwrite failed";
        return false;
    }
    return true;
}

bool WriteI32Element(hid_t file, const char* path, hsize_t index, int32_t value,
                     std::string& error) {
    TypedHandle ds(H5Dopen2(file, path, H5P_DEFAULT), Obj::Dataset);
    if (!ds.Ok()) {
        error = std::string("cannot open dataset '") + path + "'";
        return false;
    }
    TypedHandle space(H5Dget_space(ds.Get()), Obj::Dataspace);
    if (!space.Ok() || H5Sselect_elements(space.Get(), H5S_SELECT_SET, 1, &index) < 0) {
        error = "cannot select element";
        return false;
    }
    TypedHandle mem(H5Screate(H5S_SCALAR), Obj::Dataspace);
    if (H5Dwrite(ds.Get(), H5T_NATIVE_INT32, mem.Get(), space.Get(), H5P_DEFAULT, &value) < 0) {
        error = "H5Dwrite failed";
        return false;
    }
    return true;
}

bool WriteU8Element(hid_t file, const char* path, hsize_t index, unsigned char value,
                    std::string& error) {
    TypedHandle ds(H5Dopen2(file, path, H5P_DEFAULT), Obj::Dataset);
    if (!ds.Ok()) {
        error = std::string("cannot open dataset '") + path + "'";
        return false;
    }
    TypedHandle space(H5Dget_space(ds.Get()), Obj::Dataspace);
    if (!space.Ok() || H5Sselect_elements(space.Get(), H5S_SELECT_SET, 1, &index) < 0) {
        error = "cannot select element";
        return false;
    }
    TypedHandle mem(H5Screate(H5S_SCALAR), Obj::Dataspace);
    if (H5Dwrite(ds.Get(), H5T_NATIVE_UCHAR, mem.Get(), space.Get(), H5P_DEFAULT, &value) < 0) {
        error = "H5Dwrite failed";
        return false;
    }
    return true;
}

bool ReadI64Dataset(hid_t file, const char* path, std::vector<int64_t>& out, std::string& error) {
    TypedHandle ds(H5Dopen2(file, path, H5P_DEFAULT), Obj::Dataset);
    if (!ds.Ok()) {
        error = std::string("cannot open dataset '") + path + "'";
        return false;
    }
    TypedHandle space(H5Dget_space(ds.Get()), Obj::Dataspace);
    if (!space.Ok()) {
        error = "cannot get space";
        return false;
    }
    hssize_t n = H5Sget_simple_extent_npoints(space.Get());
    if (n < 0) {
        error = "cannot get npoints";
        return false;
    }
    out.assign(static_cast<size_t>(n), 0);
    if (n > 0 && H5Dread(ds.Get(), H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data()) < 0) {
        error = "H5Dread failed";
        return false;
    }
    return true;
}

/// 用一个新的定长 ASCII 值替换某个对象的同名 attribute（长度可以不同）。
bool ReplaceAsciiAttribute(hid_t loc, const char* name, const std::string& value,
                           std::string& error) {
    H5Adelete(loc, name);  // 不存在也没关系
    TypedHandle type(H5Tcopy(H5T_C_S1), Obj::Datatype);
    if (!type.Ok() || H5Tset_size(type.Get(), value.size() + 1) < 0 ||
        H5Tset_strpad(type.Get(), H5T_STR_NULLPAD) < 0) {
        error = "cannot build the string type";
        return false;
    }
    TypedHandle space(H5Screate(H5S_SCALAR), Obj::Dataspace);
    if (!space.Ok()) {
        error = "cannot create scalar space";
        return false;
    }
    TypedHandle attr(H5Acreate2(loc, name, type.Get(), space.Get(), H5P_DEFAULT, H5P_DEFAULT),
                     Obj::Attribute);
    if (!attr.Ok()) {
        error = std::string("cannot create attribute '") + name + "'";
        return false;
    }
    std::string buffer = value;
    buffer.push_back('\0');
    if (H5Awrite(attr.Get(), type.Get(), buffer.data()) < 0) {
        error = std::string("cannot write attribute '") + name + "'";
        return false;
    }
    return true;
}

bool WriteF64ArrayAttribute(hid_t loc, const char* name, const std::vector<double>& values,
                            std::string& error) {
    H5Adelete(loc, name);
    hsize_t dim = values.size();
    TypedHandle space(H5Screate_simple(1, &dim, nullptr), Obj::Dataspace);
    TypedHandle attr(H5Acreate2(loc, name, H5T_NATIVE_DOUBLE, space.Get(), H5P_DEFAULT, H5P_DEFAULT),
                     Obj::Attribute);
    if (!space.Ok() || !attr.Ok()) {
        error = std::string("cannot create attribute '") + name + "'";
        return false;
    }
    if (H5Awrite(attr.Get(), H5T_NATIVE_DOUBLE, values.data()) < 0) {
        error = std::string("cannot write attribute '") + name + "'";
        return false;
    }
    return true;
}

bool WriteFixedStringAttr(hid_t loc, const char* name, const std::string& value,
                          std::string& error) {
    return ReplaceAsciiAttribute(loc, name, value, error);
}

/// 删掉一个对象（组 / dataset / 命名 datatype 都适用）。
bool DeleteObject(hid_t file, const char* path, std::string& error) {
    if (H5Ldelete(file, path, H5P_DEFAULT) < 0) {
        error = std::string("cannot delete '") + path + "'";
        return false;
    }
    return true;
}

/// 替换一个属性 dataset：新的元素数 / 分量数，并写入真实数据（storage > 0）。
///
/// 为什么要重写而不是"原地改 dataspace"：HDF5 的固定 dataspace 不能改维度，
/// 所以"篡改 dataspace"在文件层面就等价于"用另一个 dataspace 重建该 dataset"。
/// 重建时必须把 §3.6 要求的 attribute（AttributeType / attachment / range）一起补上，
/// 否则读取器会先因为缺 attribute 而拒绝，测不到长度校验那一条。
bool RecreateAttributeDataset(hid_t file, const char* groupPath, const char* name,
                              int64_t count, int components, const std::string& attachment,
                              std::string& error) {
    TypedHandle group(H5Gopen2(file, groupPath, H5P_DEFAULT), Obj::Group);
    if (!group.Ok()) {
        error = std::string("cannot open group '") + groupPath + "'";
        return false;
    }
    H5Ldelete(group.Get(), name, H5P_DEFAULT);

    hsize_t dims[2] = {static_cast<hsize_t>(count), static_cast<hsize_t>(components)};
    TypedHandle space(H5Screate_simple(components > 1 ? 2 : 1, dims, nullptr), Obj::Dataspace);
    TypedHandle ds(H5Dcreate2(group.Get(), name, H5T_IEEE_F32LE, space.Get(), H5P_DEFAULT,
                              H5P_DEFAULT, H5P_DEFAULT),
                   Obj::Dataset);
    if (!space.Ok() || !ds.Ok()) {
        error = std::string("cannot create dataset '") + name + "'";
        return false;
    }

    std::vector<float> zeros(static_cast<size_t>(count) * static_cast<size_t>(components), 0.0f);
    if (H5Dwrite(ds.Get(), H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, zeros.data()) < 0) {
        error = "cannot write the replacement data";
        return false;
    }

    if (!WriteFixedStringAttr(ds.Get(), "AttributeType", "Scalars", error)) { return false; }
    if (!WriteFixedStringAttr(ds.Get(), "attachment", attachment, error)) { return false; }
    std::vector<double> range = (components == 1) ? std::vector<double>{0.0, 0.0}
                                                  : std::vector<double>(2 * (1 + components), 0.0);
    return WriteF64ArrayAttribute(ds.Get(), "range", range, error);
}

/// 造一个"dataspace 声明巨大、文件里只有一点点数据"的属性 dataset。
///
/// `count * components` 用 `int64_t` 算得出来、但远超文件的物理存储 —— 这正是
/// P5-11 要挡的那类坏元数据。默认写入一个 chunk 让 storage > 0，从而命中
/// 「声明字节数 / 已分配字节数」那条物理判据（而不是"完全没有分配"那条）。
///
/// `writeFirstChunk = false` 用于"乘积溢出 int64"的用例：那种 dataspace 的
/// 行内偏移乘以元素大小本身就会溢出 HDF5 内部的地址计算，物理上写不进去；
/// 而读取器的 `MulWouldOverflow()` 在**读属性之前**就会拒绝，所以不需要数据。
bool RecreateHugeAttributeDataset(hid_t file, const char* groupPath, const char* name,
                                  int64_t count, int components, bool writeFirstChunk,
                                  std::string& error) {
    TypedHandle group(H5Gopen2(file, groupPath, H5P_DEFAULT), Obj::Group);
    if (!group.Ok()) {
        error = std::string("cannot open group '") + groupPath + "'";
        return false;
    }
    H5Ldelete(group.Get(), name, H5P_DEFAULT);

    hsize_t dims[2] = {static_cast<hsize_t>(count), static_cast<hsize_t>(components)};
    hsize_t chunk[2] = {1024, static_cast<hsize_t>(components)};

    TypedHandle space(H5Screate_simple(components > 1 ? 2 : 1, dims, nullptr), Obj::Dataspace);
    TypedHandle dcpl(H5Pcreate(H5P_DATASET_CREATE), Obj::Property);
    if (!space.Ok() || !dcpl.Ok() ||
        H5Pset_chunk(dcpl.Get(), components > 1 ? 2 : 1, chunk) < 0) {
        error = "cannot build the huge dataspace";
        return false;
    }
    TypedHandle ds(H5Dcreate2(group.Get(), name, H5T_IEEE_F32LE, space.Get(), H5P_DEFAULT,
                              dcpl.Get(), H5P_DEFAULT),
                   Obj::Dataset);
    if (!ds.Ok()) {
        error = std::string("cannot create dataset '") + name + "'";
        return false;
    }

    // 只写第一个 chunk：storage ≈ 4 KiB，而 dataspace 声称有 count 个元素。
    if (writeFirstChunk) {
        hsize_t start[2] = {0, 0};
        hsize_t block[2] = {1024, static_cast<hsize_t>(components)};
        TypedHandle fspace(H5Dget_space(ds.Get()), Obj::Dataspace);
        if (!fspace.Ok() || H5Sselect_hyperslab(fspace.Get(), H5S_SELECT_SET, start, nullptr, block,
                                                nullptr) < 0) {
            error = "cannot select the first chunk";
            return false;
        }
        TypedHandle mspace(H5Screate_simple(components > 1 ? 2 : 1, block, nullptr), Obj::Dataspace);
        std::vector<float> zeros(1024 * static_cast<size_t>(components), 0.0f);
        if (!mspace.Ok() ||
            H5Dwrite(ds.Get(), H5T_NATIVE_FLOAT, mspace.Get(), fspace.Get(), H5P_DEFAULT,
                     zeros.data()) < 0) {
            error = "cannot write the first chunk";
            return false;
        }
    }

    // 属性照抄一份，好让读取器先走到"读 dataspace"那一步。
    if (!WriteFixedStringAttr(ds.Get(), "AttributeType", "Scalars", error)) { return false; }
    if (!WriteFixedStringAttr(ds.Get(), "attachment", "point", error)) { return false; }
    return WriteF64ArrayAttribute(ds.Get(), "range", {0.0, 0.0}, error);
}

/// 把 `/IGV/Points` 换成一维（rank 1）数组，用于验证 rank 校验。
bool RecreatePointsAsRankOne(hid_t file, int64_t totalValues, std::string& error) {
    TypedHandle root(H5Gopen2(file, "/IGV", H5P_DEFAULT), Obj::Group);
    if (!root.Ok()) {
        error = "cannot open /IGV";
        return false;
    }
    H5Ldelete(root.Get(), "Points", H5P_DEFAULT);
    hsize_t dim = static_cast<hsize_t>(totalValues);
    TypedHandle space(H5Screate_simple(1, &dim, nullptr), Obj::Dataspace);
    TypedHandle ds(H5Dcreate2(root.Get(), "Points", H5T_IEEE_F32LE, space.Get(), H5P_DEFAULT,
                              H5P_DEFAULT, H5P_DEFAULT),
                   Obj::Dataset);
    if (!space.Ok() || !ds.Ok()) {
        error = "cannot recreate Points";
        return false;
    }
    std::vector<float> zeros(static_cast<size_t>(totalValues), 0.0f);
    if (H5Dwrite(ds.Get(), H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, zeros.data()) < 0) {
        error = "cannot write Points";
        return false;
    }
    return true;
}

// =============================================================================
// 夹具：一份"五脏俱全"的合法 .igv（混合单元 + f32/f64/u8 属性）
// =============================================================================

UnstructuredMesh::Pointer MakeSeedMesh() {
    auto mesh = UnstructuredMesh::New();

    Points::Pointer pts = Points::New();
    pts->Resize(18);
    std::vector<float> coords(18 * 3);
    int64_t k = 0;
    for (int z = 0; z < 2; ++z) {
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                coords[k * 3 + 0] = static_cast<float>(x) + 0.1234567f * static_cast<float>(z);
                coords[k * 3 + 1] = static_cast<float>(y) - 0.7654321f * static_cast<float>(z);
                coords[k * 3 + 2] = static_cast<float>(z) * 1.5f;
                ++k;
            }
        }
    }
    std::memcpy(pts->RawPointer(), coords.data(), coords.size() * sizeof(float));
    mesh->SetPoints(pts);

    // 4 个**混合**单元（三角 / 四边形 / 四面体 / 六面体）—— 这样 Types、Offsets、
    // Connectivity 三条路径都被真的用到。
    CellArray::Pointer cells = CellArray::New();
    igIndex tri[3] = {0, 1, 3};
    igIndex quad[4] = {0, 1, 4, 3};
    igIndex tet[4] = {0, 1, 3, 9};
    igIndex hex[8] = {0, 1, 3, 4, 9, 10, 12, 13};
    cells->AddCellIds(tri, 3);
    cells->AddCellIds(quad, 4);
    cells->AddCellIds(tet, 4);
    cells->AddCellIds(hex, 8);

    UnsignedIntArray::Pointer types = UnsignedIntArray::New();
    types->AddValue(static_cast<unsigned int>(IG_TRIANGLE));
    types->AddValue(static_cast<unsigned int>(IG_QUAD));
    types->AddValue(static_cast<unsigned int>(IG_TETRA));
    types->AddValue(static_cast<unsigned int>(IG_HEXAHEDRON));
    mesh->SetCells(cells, types);

    AttributeSet* set = mesh->GetAttributeSet();

    FloatArray::Pointer f = FloatArray::New();
    f->SetDimension(1);
    f->SetName("f_scalar");
    f->Resize(18);
    for (IGsize i = 0; i < 18; ++i) { f->RawPointer()[i] = static_cast<float>(i) * 0.5f; }
    set->AddAttribute(IG_SCALAR, IG_POINT, f);

    DoubleArray::Pointer d = DoubleArray::New();
    d->SetDimension(1);
    d->SetName("d_scalar");
    d->Resize(18);
    for (IGsize i = 0; i < 18; ++i) { d->RawPointer()[i] = 0.1 + 0.3333333333333333 * i; }
    set->AddAttribute(IG_SCALAR, IG_POINT, d);

    UnsignedCharArray::Pointer u = UnsignedCharArray::New();
    u->SetDimension(1);
    u->SetName("u8_scalar");
    u->Resize(18);
    for (IGsize i = 0; i < 18; ++i) { u->RawPointer()[i] = static_cast<unsigned char>(i * 13); }
    set->AddAttribute(IG_SCALAR, IG_POINT, u);

    FloatArray::Pointer c = FloatArray::New();
    c->SetDimension(3);
    c->SetName("cell_vec");
    c->Resize(4);
    for (IGsize i = 0; i < 12; ++i) { c->RawPointer()[i] = static_cast<float>(i) * 1.25f; }
    set->AddAttribute(IG_VECTOR, IG_CELL, c);

    return mesh;
}

/// 结构化种子：用于验证 Dimensions 相关分支（P5 表中未单列，但同属 §6.3）。
StructuredMesh::Pointer MakeStructuredSeed() {
    auto mesh = StructuredMesh::New();
    const int nx = 4;
    const int ny = 3;
    const int nz = 3;
    Points::Pointer pts = Points::New();
    pts->Resize(static_cast<IGsize>(nx * ny * nz));
    IGsize idx = 0;
    for (int z = 0; z < nz; ++z) {
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                pts->RawPointer()[idx * 3 + 0] = static_cast<float>(x);
                pts->RawPointer()[idx * 3 + 1] = static_cast<float>(y);
                pts->RawPointer()[idx * 3 + 2] = static_cast<float>(z);
                ++idx;
            }
        }
    }
    mesh->SetPoints(pts);
    igIndex dim[3] = {nx, ny, nz};
    mesh->SetDimensionSize(dim);
    mesh->GenStructuredCellConnectivities();

    FloatArray::Pointer f = FloatArray::New();
    f->SetDimension(1);
    f->SetName("p_scalar");
    f->Resize(static_cast<IGsize>(nx * ny * nz));
    for (IGsize i = 0; i < f->GetNumberOfValues(); ++i) { f->RawPointer()[i] = static_cast<float>(i); }
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);

    // 单元属性：长度必须等于 (nx-1)(ny-1)(nz-1)
    FloatArray::Pointer c = FloatArray::New();
    c->SetDimension(1);
    c->SetName("c_scalar");
    c->Resize(static_cast<IGsize>((nx - 1) * (ny - 1) * (nz - 1)));
    for (IGsize i = 0; i < c->GetNumberOfValues(); ++i) { c->RawPointer()[i] = static_cast<float>(i) * 2.f; }
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, c);
    return mesh;
}

bool WriteSeed(const DataObject::Pointer& obj, const fs::path& path, std::string& error) {
    IGVWriter::Pointer writer = IGVWriter::New();
    if (!writer->WriteToFile(obj, path.string())) {
        error = writer->GetLastError();
        return false;
    }
    return true;
}

// =============================================================================
// 变异用例表
// =============================================================================

struct MutationCase {
    std::string title;              ///< 人读的标题（P5-x）
    std::string fileName;           ///< 变异后的文件名
    std::function<bool(hid_t file, const fs::path& path, std::string&)> mutate;
    std::string expectKeyword;      ///< 错误信息里必须出现的关键字
    bool expectAccept{false};       ///< P5-7 用：期望**被接受**
};

}  // namespace

int main(int argc, char** argv) {
    bool quick = false;
    int fuzzIterations = 1000;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--quick") { quick = true; }
        else if (a == "--fuzz" && i + 1 < argc) { fuzzIterations = std::atoi(argv[++i]); }
    }

    std::cerr << "================================================================\n"
              << "IGV P5 acceptance (1/2): malformed files must be rejected, never crash\n"
              << "Covers kia/igv implementation plan section 9 (P5-1..P5-11, P5-14)\n"
              << "================================================================\n"
              << "Working dir   : " << fs::current_path().string() << "\n"
              << "Temp dir      : " << WorkDir().string() << "\n";

    SilenceHdf5Errors();

    // -------------------------------------------------------------------------
    Section("P5-0 build clean seed files (the baseline every mutation starts from)");
    // -------------------------------------------------------------------------
    const fs::path seed = WorkDir() / "seed_unstructured.igv";
    const fs::path structuredSeed = WorkDir() / "seed_structured.igv";
    {
        std::string error;
        DataObject::Pointer mesh = MakeSeedMesh();
        if (!WriteSeed(mesh, seed, error)) {
            Check(false, "P5-0: cannot write the clean seed: " + error);
            return 1;
        }
        DataObject::Pointer sm = MakeStructuredSeed();
        if (!WriteSeed(sm, structuredSeed, error)) {
            Check(false, "P5-0: cannot write the structured seed: " + error);
            return 1;
        }
        Info("P5-0: seed = " + Num(static_cast<int64_t>(fs::file_size(seed))) + " B, structured seed = " +
             Num(static_cast<int64_t>(fs::file_size(structuredSeed))) + " B");

        const ReadOutcome ok = TryRead(seed);
        Check(ok.parsed, "P5-0: the clean seed itself is accepted (otherwise every rejection below is meaningless)");
    }

    // -------------------------------------------------------------------------
    Section("P5-1 truncated files (10% / 50% / 99%) are rejected without crashing");
    // -------------------------------------------------------------------------
    {
        const std::vector<char> bytes = ReadAllBytes(seed);
        for (int pct : {10, 50, 99}) {
            const size_t keep = bytes.size() * static_cast<size_t>(pct) / 100;
            std::vector<char> truncated(bytes.begin(), bytes.begin() + static_cast<ptrdiff_t>(keep));
            const fs::path p = WorkDir() / ("p51_truncated_" + Num(pct) + ".igv");
            if (!WriteAllBytes(p, truncated)) {
                Check(false, "P5-1: cannot write the truncated file");
                continue;
            }
            ExpectReject("P5-1 truncated to " + Num(pct) + "% (" + Num(static_cast<int64_t>(keep)) + " B)",
                         p, "");
        }
    }

    // -------------------------------------------------------------------------
    Section("P5-2..P5-9, P5-11 single-field mutations");
    // -------------------------------------------------------------------------

    const std::vector<MutationCase> cases = {
        // P5-2：Connectivity 里出现 pointCount（越界一位）
        {"P5-2 Connectivity index out of range", "p52_conn_oob.igv",
         [](hid_t f, const fs::path&, std::string& e) { return WriteI32Element(f, "/IGV/Connectivity", 0, 18, e); },
         "out of range"},

        // P5-3：交换两个相邻 offset → 非单调
        {"P5-3 Offsets are no longer monotonic", "p53_offsets_swap.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             std::vector<int64_t> off;
             if (!ReadI64Dataset(f, "/IGV/Offsets", off, e)) { return false; }
             if (off.size() < 4) { e = "seed has too few offsets"; return false; }
             std::swap(off[1], off[2]);
             return WriteI64Dataset(f, "/IGV/Offsets", off, e);
         },
         "strictly increasing"},

        // P5-4：Offsets 末值 != Connectivity 长度
        {"P5-4 Offsets last value disagrees with Connectivity length", "p54_offsets_last.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             std::vector<int64_t> off;
             if (!ReadI64Dataset(f, "/IGV/Offsets", off, e)) { return false; }
             off.back() -= 1;
             return WriteI64Dataset(f, "/IGV/Offsets", off, e);
         },
         "Offsets last value"},

        // P5-5：point 属性的元素数 = pointCount + 1
        {"P5-5 attribute length != point count", "p55_attr_len.igv",
         [](hid_t f, const fs::path& p, std::string& e) {
             (void)p;
             return RecreateAttributeDataset(f, "/IGV/PointData", "f_scalar", 19, 1, "point", e);
         },
         "section 6.3 requires rejection"},

        // P5-6：Version major = 2
        {"P5-6 incompatible major version (2.0)", "p56_version_major.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             return WriteI64Dataset(f, "/IGV/Version", {2, 0}, e);
         },
         "incompatible IGV version"},

        // P5-8：未知 Type 字符串
        {"P5-8 unknown Type string", "p58_type_unknown.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             TypedHandle root(H5Gopen2(f, "/IGV", H5P_DEFAULT), Obj::Group);
             if (!root.Ok()) { e = "cannot open /IGV"; return false; }
             return ReplaceAsciiAttribute(root.Get(), "Type", "NoSuchMesh", e);
         },
         "unknown Type value"},

        // P5-9：删掉 /IGV 根组
        {"P5-9 missing /IGV root group", "p59_no_root.igv",
         [](hid_t f, const fs::path&, std::string& e) { return DeleteObject(f, "/IGV", e); },
         "root group is not /IGV"},

        // P5-11（a）：2^33 个元素但文件里只有 4 KiB —— 必须在分配前拒绝
        {"P5-11a absurd dataspace (2^33 elements) is rejected before allocating", "p511a_huge.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             return RecreateHugeAttributeDataset(f, "/IGV/PointData", "f_scalar", (int64_t{1} << 33), 1, true, e);
         },
         "corrupt"},

        // P5-11（b）：count * components 直接溢出 int64
        // 取 count = 3 * 2^60、components = 4：乘积约 1.38e19，
        //   * 超过 int64 上限（9.22e18）→ 触发 MulWouldOverflow；
        //   * 仍小于 hsize_t 上限（1.84e19）→ HDF5 肯建出这个 dataspace，
        //     否则连"造出坏元数据"都做不到（这是本用例第一版踩到的坑）。
        {"P5-11b count*components overflows int64", "p511b_overflow.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             return RecreateHugeAttributeDataset(f, "/IGV/PointData", "f_scalar", (int64_t{3} << 60), 4, false, e);
         },
         "overflows"},

        // ---- 以下属 §6.3 同一张表，成本几乎为零，一并钉住 ----
        // 注意：禁止表的 name 是 "Face"（不是 "IG_FACE"），reason 里写了"value 3 is VTK_LINE"。
        {"+ Types contains the forbidden code IG_FACE(3)", "px_types_forbidden.igv",
         [](hid_t f, const fs::path&, std::string& e) { return WriteU8Element(f, "/IGV/Types", 0, 3, e); },
         "writable whitelist"},

        {"+ Types contains an unknown code (200)", "px_types_unknown.igv",
         [](hid_t f, const fs::path&, std::string& e) { return WriteU8Element(f, "/IGV/Types", 0, 200, e); },
         "whitelist"},

        {"+ CellTypeEncoding is wrong", "px_cte_wrong.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             TypedHandle root(H5Gopen2(f, "/IGV", H5P_DEFAULT), Obj::Group);
             if (!root.Ok()) { e = "cannot open /IGV"; return false; }
             return ReplaceAsciiAttribute(root.Get(), "CellTypeEncoding", "VTK", e);
         },
         "CellTypeEncoding"},

        {"+ CellTypeEncoding is missing", "px_cte_missing.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             TypedHandle root(H5Gopen2(f, "/IGV", H5P_DEFAULT), Obj::Group);
             if (!root.Ok()) { e = "cannot open /IGV"; return false; }
             if (H5Adelete(root.Get(), "CellTypeEncoding") < 0) {
                 e = "cannot delete CellTypeEncoding";
                 return false;
             }
             return true;
         },
         "no CellTypeEncoding"},

        {"+ attribute attachment token is unknown", "px_attach_unknown.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             TypedHandle ds(H5Dopen2(f, "/IGV/PointData/f_scalar", H5P_DEFAULT), Obj::Dataset);
             if (!ds.Ok()) { e = "cannot open the attribute dataset"; return false; }
             return ReplaceAsciiAttribute(ds.Get(), "attachment", "sideways", e);
         },
         "attachment value"},

        {"+ range length is invalid (3 instead of 2)", "px_range_len.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             TypedHandle ds(H5Dopen2(f, "/IGV/PointData/f_scalar", H5P_DEFAULT), Obj::Dataset);
             if (!ds.Ok()) { e = "cannot open the attribute dataset"; return false; }
             return WriteF64ArrayAttribute(ds.Get(), "range", {0.0, 1.0, 2.0}, e);
         },
         "range length"},

        {"+ Points has rank 1 instead of (n,3)", "px_points_rank.igv",
         [](hid_t f, const fs::path&, std::string& e) { return RecreatePointsAsRankOne(f, 54, e); },
         "Points dataspace"},

        {"+ NumberOfPoints disagrees with Points.dims[0]", "px_count_mismatch.igv",
         [](hid_t f, const fs::path&, std::string& e) {
             return WriteI64Dataset(f, "/IGV/NumberOfPoints", {17}, e);
         },
         "NumberOfPoints"},

        {"+ Version dataset is missing", "px_version_missing.igv",
         [](hid_t f, const fs::path&, std::string& e) { return DeleteObject(f, "/IGV/Version", e); },
         "Version"},

        // 注：`Block_*` 子组（多块）的拒绝用例从这张表挪到了下面的 P6 一节 ——
        // 那一节用的是更强的夹具（**两个**块 + 一份完整的 `/IGV` 几何），
        // 且同时断言"返回 nullptr"与"错误信息指明容器种类"，见 P6-1 / P6-2。
    };

    for (const MutationCase& c : cases) {
        const fs::path mutated = WorkDir() / c.fileName;
        std::string error;
        if (!CopyFileTo(seed, mutated, error)) {
            Check(false, c.title + ": " + error);
            continue;
        }
        {
            RwFile f;
            if (!f.Open(mutated, error)) {
                Check(false, c.title + ": " + error);
                continue;
            }
            if (!c.mutate(f.Raw(), mutated, error)) {
                Check(false, c.title + ": mutation failed: " + error);
                continue;
            }
        }  // 关闭文件后再读，确保 HDF5 已 flush
        ExpectReject(c.title, mutated, c.expectKeyword);
    }

    // -------------------------------------------------------------------------
    Section("P5-7 a higher MINOR version must be ACCEPTED (forward compatible)");
    // -------------------------------------------------------------------------
    {
        const fs::path p = WorkDir() / "p57_version_minor.igv";
        std::string error;
        bool ok = CopyFileTo(seed, p, error);
        if (ok) {
            RwFile f;
            ok = f.Open(p, error) && WriteI64Dataset(f.Raw(), "/IGV/Version", {1, 99}, error);
        }
        if (!ok) {
            Check(false, "P5-7: " + error);
        } else {
            const ReadOutcome out = TryRead(p);
            Check(out.parsed, "P5-7: version 1.99 is accepted (minor version is forward compatible)");
            if (!out.parsed) { Info("P5-7: rejected with: " + out.error); }
        }
    }

    // -------------------------------------------------------------------------
    Section("P5-10 write side rejects illegal attribute names");
    // -------------------------------------------------------------------------
    {
        struct NameCase {
            std::string title;
            std::string name;
        };
        const std::vector<NameCase> nameCases = {
            {"P5-10a attribute name contains '/' (HDF5 name is a path)", "a/b"},
            {"P5-10b attribute name is empty", ""},
            {"P5-10c attribute name starts with '.'", ".hidden"},
        };
        for (const NameCase& nc : nameCases) {
            DataObject::Pointer mesh = MakeSeedMesh();
            FloatArray::Pointer f = FloatArray::New();
            f->SetDimension(1);
            f->SetName(nc.name);
            f->Resize(18);
            for (IGsize i = 0; i < 18; ++i) { f->RawPointer()[i] = static_cast<float>(i); }
            mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);

            IGVWriter::Pointer writer = IGVWriter::New();
            const fs::path out = WorkDir() / "p510_illegal_name.igv";
            const bool wrote = writer->WriteToFile(mesh, out.string());
            Check(!wrote, nc.title + ": write is rejected");
            const std::string err = writer->GetLastError();
            Check(err.find("name") != std::string::npos,
                  nc.title + ": the error message mentions the attribute name (actual: " + err + ")");
        }
    }

    // -------------------------------------------------------------------------
    Section("P6 MultiBlock / time-series containers: probe and reject, never drop data");
    // -------------------------------------------------------------------------
    //
    // 背景（要保证的正确行为）：
    //   §12.1 / §12.2 明确本期**不实现**多块与时变的读写，并把 P6 的唯一目的定为
    //   「不静默丢数据」：含 `Block_*` 子组（多块）或 `Steps` 组（单文件内嵌时变）的文件
    //   必须**拒绝**，绝不能把第一块 / 第 0 步当整体返回 —— 那种"部分成功"会让用户
    //   以为数据完整，比报错危险得多（P6-1 / P6-2）。
    //
    //   P6-3 钉的是同一设计里相反的一面：`PointData` 下一个**名为 Block_0 的属性**
    //   不是块。若实现用"名字里出现 Block_"来判块，这条会误伤合法文件。
    //
    //   构造这些夹具时**保留 `/IGV` 自己那份完整几何/拓扑**，这是最危险的形态：
    //   只读 `/IGV` 也能"成功"拿到一份看起来完整的网格，于是"部分成功"很难被发现。
    //
    //   P6-6 是**写侧**的同一条危险：五类网格之一若自带子块，类型检查会放行，
    //   而写出实现只遍历父对象自己的几何 —— 子块就被静默丢弃。
    //
    // 明确**不**声称覆盖：真正读写多块 / 时变（属 kia/未来任务.md F5 / F6，本期不做）。

    // --- P6-1 + P6-2: 多块探测、显式拒绝、返回 nullptr ---
    {
        const fs::path p = WorkDir() / "p61_multiblock.igv";
        std::string error;
        bool ok = CopyFileTo(seed, p, error);
        if (ok) {
            RwFile f;
            ok = f.Open(p, error);
            if (ok) {
                for (const char* b : {"Block_0", "Block_1"}) {
                    hid_t g = H5Gcreate2(f.Raw(), (std::string("/IGV/") + b).c_str(), H5P_DEFAULT,
                                         H5P_DEFAULT, H5P_DEFAULT);
                    if (g < 0) {
                        error = std::string("cannot create ") + b;
                        ok = false;
                        break;
                    }
                    H5Gclose(g);
                }
            }
        }
        if (!ok) {
            Check(false, "P6-1: " + error);
        } else {
            Info("P6-1: fixture has /IGV/Block_0 and /IGV/Block_1 on top of a complete /IGV block");
            ExpectReject("P6-1 a file with Block_* subgroups", p, "MultiBlock is not supported");

            // P6-2：必须返回 nullptr。`TryRead()` 的 `parsed` 就是
            // `ReadFile(...) != nullptr`，所以上面已断言了这一点；这里把
            // 「不静默丢数据」写成显式判据并检查错误信息指明了是哪一种容器。
            const ReadOutcome out = TryRead(p);
            Check(!out.parsed,
                  "P6-2 the reader returns nullptr - no partial object that silently drops the blocks");
            Check(out.error.find("MultiBlock") != std::string::npos,
                  "P6-2 the rejection names MultiBlock (actual: " + out.error + ")");

            // P6-2b：**元数据快路径**也必须拒绝。理由不是"读不了"（它本来就只读 attribute），
            // 而是"别让文件浏览器列出一份看起来正常的清单、点开才发现打不开"。
            igv::IGVMetadata meta;
            std::string metaError;
            const bool metaOk = IGVMetadataReader::ReadFromFile(p.string(), meta, &metaError);
            Check(!metaOk, "P6-2b the metadata fast path rejects the block file too (no misleading listing)");
            Check(metaError.find("MultiBlock") != std::string::npos,
                  "P6-2b the metadata rejection names MultiBlock (actual: " + metaError + ")");
        }
    }

    // --- P6-3: PointData 下名为 Block_0 的**属性**不能被误判为块 ---
    {
        const fs::path p = WorkDir() / "p63_attr_named_block.igv";
        std::string error;
        bool ok = CopyFileTo(seed, p, error);
        if (ok) {
            RwFile f;
            ok = f.Open(p, error);
            if (ok) {
                // 硬链接把 f_scalar 挂到同一组下的名字 Block_0：attribute
                // （AttributeType / attachment / range）与数据一并过来，是一份**完全合法**的属性。
                if (H5Lcreate_hard(f.Raw(), "/IGV/PointData/f_scalar", f.Raw(),
                                   "/IGV/PointData/Block_0", H5P_DEFAULT, H5P_DEFAULT) < 0) {
                    error = "cannot hard-link f_scalar to Block_0";
                    ok = false;
                }
            }
        }
        if (!ok) {
            Check(false, "P6-3: " + error);
        } else {
            IGVReader::Pointer reader = IGVReader::New();
            DataObject::Pointer obj = reader->ReadFile(p.string());
            Check(obj != nullptr,
                  "P6-3 an attribute named Block_0 does not trip the block detector (file is accepted)");
            if (!obj) {
                Info("P6-3: rejected with: " + reader->GetLastError());
            } else {
                const igv::IGVMetadata& meta = reader->GetParsedMetadata();
                Check(meta.pointCount == 18 && meta.cellCount == 4,
                      "P6-3 the mesh itself is still the seed (18 points / 4 cells), actual " +
                          Num(meta.pointCount) + " / " + Num(meta.cellCount));

                AttributeSet* set = obj->GetAttributeSet();
                AttributeSet::Attribute& block = set->GetAttribute(std::string("Block_0"), IG_SCALAR);
                Check(!block.IsNone(), "P6-3 the attribute 'Block_0' exists in the read-back object");
                if (!block.IsNone()) {
                    auto arr = DynamicCast<FloatArray>(block.GetPointer());
                    Check(arr != nullptr && arr->GetNumberOfValues() == 18,
                          "P6-3 'Block_0' is a float array of 18 values (name and value both preserved)");
                    bool same = (arr != nullptr && arr->GetNumberOfValues() == 18);
                    if (same) {
                        for (IGsize i = 0; i < 18; ++i) {
                            // 种子里 f_scalar 的第 i 个值是 i * 0.5f（精确可表示）
                            if (arr->RawPointer()[i] != static_cast<float>(i) * 0.5f) { same = false; }
                        }
                    }
                    Check(same, "P6-3 'Block_0' keeps the same values as f_scalar (bit-identical)");
                }
            }
        }
    }

    // --- P6-3b: /IGV/Steps（单文件内嵌时变）同样必须拒绝 ---
    {
        const fs::path p = WorkDir() / "p63b_steps.igv";
        std::string error;
        bool ok = CopyFileTo(seed, p, error);
        if (ok) {
            RwFile f;
            ok = f.Open(p, error);
            if (ok) {
                hid_t g = H5Gcreate2(f.Raw(), "/IGV/Steps", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                if (g < 0) {
                    error = "cannot create /IGV/Steps";
                    ok = false;
                } else {
                    H5Gclose(g);
                }
            }
        }
        if (!ok) {
            Check(false, "P6-3b: " + error);
        } else {
            ExpectReject("P6-3b a file with a Steps group (single-file time series)", p,
                         "time series is not supported");

            // 元数据快路径同样拒绝（与 P6-2b 同理，见 §12.1.3）。
            igv::IGVMetadata meta;
            std::string metaError;
            const bool metaOk = IGVMetadataReader::ReadFromFile(p.string(), meta, &metaError);
            Check(!metaOk, "P6-3c the metadata fast path rejects the Steps file too");
            Check(metaError.find("time series") != std::string::npos,
                  "P6-3c the metadata rejection names the time series (actual: " + metaError + ")");
        }
    }

    // --- P6-4: 单块文件不受影响（拒绝分支不误伤正常文件）---
    {
        IGVReader::Pointer reader = IGVReader::New();
        DataObject::Pointer obj = reader->ReadFile(seed.string());
        const igv::IGVMetadata& meta = reader->GetParsedMetadata();
        Check(obj != nullptr && meta.pointCount == 18 && meta.cellCount == 4,
              "P6-4 a normal single-block file is unaffected (18 points / 4 cells read back)");

        IGVReader::Pointer structured = IGVReader::New();
        DataObject::Pointer sobj = structured->ReadFile(structuredSeed.string());
        const igv::IGVMetadata& smeta = structured->GetParsedMetadata();
        Check(sobj != nullptr && smeta.pointCount == 36 && smeta.hasDimensions,
              "P6-4 the structured single-block seed is unaffected too (36 points + Dimensions)");
    }

    // --- P6-5: 格式表里不存在 MultiBlock 的 Type 取值（§1.4 原则 1）---
    {
        using namespace iGame::igv;
        Check(!IsSupportedTypeToken("MultiBlock") && !IsSupportedTypeToken("MultiBlockDataSet"),
              "P6-5 no MultiBlock Type token exists in the frozen format table (no schema reserved)");
        Check(IsSupportedTypeToken("UnstructuredGrid") && IsSupportedTypeToken("PolyData") &&
                  IsSupportedTypeToken("StructuredGrid"),
              "P6-5 the three real Type tokens are still the only supported ones");
        Check(ClassifyUnsupportedContainer("Block_7") == UnsupportedContainer::MultiBlock,
              "P6-5 'Block_7' is classified as a MultiBlock container");
        Check(ClassifyUnsupportedContainer("Steps") == UnsupportedContainer::TimeSeries,
              "P6-5 'Steps' is classified as a time-series container");
        Check(ClassifyUnsupportedContainer("PointData") == UnsupportedContainer::None &&
                  ClassifyUnsupportedContainer("CellData") == UnsupportedContainer::None &&
                  ClassifyUnsupportedContainer("FieldData") == UnsupportedContainer::None,
              "P6-5 the three attribute groups are NOT classified as blocks");
        Check(!IsBlockGroupName("Block_") && IsBlockGroupName("Block_0"),
              "P6-5 a bare 'Block_' prefix is not a block name");

        // 反向钉住：把根组 Type 改成 "MultiBlock" 必须被当作**未知 Type** 拒绝，
        // 而不是被"识别"成某种已支持的类型。
        const fs::path p = WorkDir() / "p65_type_multiblock.igv";
        std::string error;
        bool ok = CopyFileTo(seed, p, error);
        if (ok) {
            RwFile f;
            ok = f.Open(p, error);
            if (ok) {
                TypedHandle root(H5Gopen2(f.Raw(), "/IGV", H5P_DEFAULT), Obj::Group);
                if (!root.Ok()) {
                    error = "cannot open /IGV";
                    ok = false;
                } else {
                    ok = ReplaceAsciiAttribute(root.Get(), "Type", "MultiBlock", error);
                }
            }
        }
        if (!ok) {
            Check(false, "P6-5: " + error);
        } else {
            ExpectReject("P6-5 a file whose Type says MultiBlock", p, "unknown Type value");
        }
    }

    // --- P6-6（写侧）: 带子块的对象绝不能被"写一部分" ---
    {
        // (a) 真正的危险窗口：**五类网格之一自带子块**。类型检查会放行，
        //     若写侧不专门拦，写出实现只遍历父对象自己的几何 → 子块静默丢失。
        DataObject::Pointer parent = MakeSeedMesh();
        parent->AddSubDataObject(MakeSeedMesh());
        IGVWriter::Pointer w1 = IGVWriter::New();
        const fs::path outA = WorkDir() / "p66_mesh_with_block.igv";
        const bool wroteA = w1->WriteToFile(parent, outA.string());
        Check(!wroteA, "P6-6a a mesh carrying sub-blocks is rejected (its blocks must not be dropped silently)");
        Check(w1->GetLastError().find("MultiBlock is not supported") != std::string::npos,
              "P6-6a the write rejection names MultiBlock (actual: " + w1->GetLastError() + ")");

        // (b) 真实的多块父对象形态（DrawObject + 子对象，CGNS 多 zone / VTM / PVD / IGCM）：
        //     原先只报"unsupported data object type"，看不出真实原因是多块。
        DrawObject::Pointer root = DrawObject::New();
        root->AddSubDataObject(MakeSeedMesh());
        IGVWriter::Pointer w2 = IGVWriter::New();
        const fs::path outB = WorkDir() / "p66_drawobject_blocks.igv";
        const bool wroteB = w2->WriteToFile(root, outB.string());
        Check(!wroteB, "P6-6b a DrawObject carrying blocks is rejected");
        Check(w2->GetLastError().find("sub-block") != std::string::npos,
              "P6-6b the rejection reports how many blocks would be dropped (actual: " +
                  w2->GetLastError() + ")");

        // (c) 单块对象仍然写得出来（拒绝分支不误伤正常写出）
        IGVWriter::Pointer w3 = IGVWriter::New();
        const fs::path outC = WorkDir() / "p66_single_block_ok.igv";
        Check(w3->WriteToFile(MakeSeedMesh(), outC.string()),
              "P6-6c a single-block object is still written normally");
    }

    // -------------------------------------------------------------------------
    Section("P5-KNOWN pin: a rejected file-based read leaks the file mapping (upstream, see main-repo problem 3)");
    // -------------------------------------------------------------------------
    //
    // 这一节断言的是"缺陷**仍然**存在"。它一旦变红，说明主仓库把
    // `FileReader::Execute()` 的失败路径补上了 Close()，那时应当把本节改成
    // "拒绝后仍可删除文件"的正向断言，并把 P5-14 改成直接复用同一个文件名。
    {
        const fs::path p = WorkDir() / "p5known_locked.igv";
        std::string error;
        bool ok = CopyFileTo(seed, p, error);
        if (ok) {
            // 用截断造一个必然被拒绝的文件（不需要 HDF5 打开它）。
            std::vector<char> bytes = ReadAllBytes(p);
            bytes.resize(bytes.size() / 2);
            ok = WriteAllBytes(p, bytes);
        }
        if (!ok) {
            Check(false, "P5-KNOWN: " + error);
        } else {
            const ReadOutcome out = TryRead(p);
            Check(!out.parsed, "P5-KNOWN: the truncated file is rejected (as expected)");

            std::error_code ec;
            const bool removed = fs::remove(p, ec);
            Check(!removed,
                  "P5-KNOWN: the rejected file is still locked, i.e. the mapping leaked "
                  "(pin: when this turns red the upstream fix landed - see problem 3)");
            if (removed) {
                Info("P5-KNOWN: the file could be removed - the upstream leak appears to be fixed");
            }
        }
    }

    // -------------------------------------------------------------------------
    Section("P5-14 mutation fuzz: random byte flips must never crash");
    // -------------------------------------------------------------------------
    {
        const std::vector<char> original = ReadAllBytes(seed);
        std::mt19937 rng(20261019u);  // 固定种子：失败可复现
        std::uniform_int_distribution<size_t> anyOffset(0, original.empty() ? 0 : original.size() - 1);
        // 数据区（后 90%）更容易产生"能打开但内容被篡改"的用例，比只砸超级块更有价值。
        const size_t dataStart = original.size() - (original.size() * 9 / 10);
        std::uniform_int_distribution<size_t> dataOffset(dataStart, original.empty() ? 0 : original.size() - 1);
        std::uniform_int_distribution<int> flipCount(1, 6);
        std::uniform_int_distribution<int> bitMask(1, 255);
        std::uniform_int_distribution<int> where(0, 9);

        int rejected = 0;
        int accepted = 0;
        int exceptions = 0;
        int executed = 0;

        // 每次失败都会走一遍 IGVReader::Fail → IGAME_CORE_ERROR。fuzz 里"失败"是**预期**
        // 结果，逐条打日志会把真正的断言淹掉（也让 1000 次迭代慢一个量级），
        // 因此临时把 core logger 关掉，结束后恢复。
        const auto previousLevel = iGame::Log::GetCoreLogger()->level();
        iGame::Log::GetCoreLogger()->set_level(spdlog::level::off);
        const int total = quick ? std::min(fuzzIterations, 100) : fuzzIterations;
        for (int i = 0; i < total; ++i) {
            std::vector<char> mutated = original;
            const int flips = flipCount(rng);
            for (int k = 0; k < flips; ++k) {
                const size_t pos = (where(rng) < 7) ? dataOffset(rng) : anyOffset(rng);
                mutated[pos] = static_cast<char>(static_cast<unsigned char>(mutated[pos]) ^ bitMask(rng));
            }
            ++executed;
            try {
                const ReadOutcome out = TryReadMemory(mutated);
                if (out.parsed) { ++accepted; } else { ++rejected; }
            } catch (const std::exception& e) {
                ++exceptions;
                Info("P5-14: iteration " + Num(i) + " threw: " + e.what());
            } catch (...) {
                ++exceptions;
                Info("P5-14: iteration " + Num(i) + " threw a non-standard exception");
            }
        }
        iGame::Log::GetCoreLogger()->set_level(previousLevel);

        Info("P5-14: " + Num(executed) + " mutations -> rejected=" + Num(rejected) +
             " accepted=" + Num(accepted) + " exceptions=" + Num(exceptions));
        Check(exceptions == 0,
              "P5-14: no mutation escaped as an exception (an escaping bad_alloc means an OOM-class bug)");
        Check(rejected + accepted == executed,
              "P5-14: every mutation either parsed or was cleanly rejected (" + Num(rejected + accepted) +
                  " of " + Num(executed) + ")");
        Check(accepted > 0,
              "P5-14: at least one mutated file still parsed (the fuzzer reaches the data region, not only the "
              "superblock)");

        // 最后确认种子本身没被 fuzz 破坏（否则上面所有断言都在测一份坏文件）。
        const ReadOutcome seedStillOk = TryRead(seed);
        Check(seedStillOk.parsed, "P5-14: the seed file is still valid after the fuzz loop");
    }

    // -------------------------------------------------------------------------
    Section("summary");
    // -------------------------------------------------------------------------
    std::cerr << "\n------------------------------------------------------------------------------\n"
              << "checks: " << g_checks << " total, failures " << g_failed << "\n";
    if (g_failed == 0) {
        std::cerr << "ALL PASSED: every malformed input was rejected and nothing crashed.\n";
        return 0;
    }
    std::cerr << "FAILED: see the [FAIL] lines above.\n";
    return 1;
}
