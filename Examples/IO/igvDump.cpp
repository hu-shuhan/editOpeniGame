/**
 * @file  igvDump.cpp
 * @brief IGV P5 工具：把一个 `.igv` 的**结构**打印成可读文本（`h5dump -H -A` 的等价物）。
 *
 * 对应 kia/igv分阶段实现方案.md §9「P5 — 鲁棒性与大文件回归」第 2 条。
 *
 * ============================================================================
 * 为什么需要它
 * ============================================================================
 *
 * 1. **人工排查**：`.igv` 是 HDF5，出问题时第一件事是"文件里到底有什么"。
 *    `h5dump` 要额外安装、输出对非 HDF5 专家不友好，而且 4 GiB 的文件会打出
 *    几千万行数据。本工具只打**结构与元数据**，默认不打印数据。
 * 2. **golden 比对**（§11.4）：文本体积极小、可 diff、可在 code review 里直接看。
 *    仓库里 `kia/vtkhdf/*.txt` 就是这个套路。
 * 3. **不依赖外部可执行文件**：`--self-check` 自己造一个 `.igv`、自己 dump、自己断言
 *    关键行都在，因此它能作为 CTest 目标跑（不需要 h5dump，也不需要外部模型）。
 *
 * 用法：
 * ```
 * igvDump <file.igv>              # 结构 + attribute（不打印数据）
 * igvDump <file.igv> --data       # 额外打印每个 dataset 的前 N 个值（默认 8）
 * igvDump <file.igv> --limit 32   # 改 --data 的条数
 * igvDump <file.igv> --gui        # 打开 Information 面板（ParaView 风格，Qt 构建才有）
 * igvDump --gui                   # 空面板：拖放 / 文件 -> 打开
 * igvDump --self-check            # 自检：造样例 -> dump -> 断言关键行存在
 * ```
 *
 * **两种模式的定位差异**（都保留，因为用户习惯不同）：
 *   * 文本模式回答"文件里**有什么**结构"——给排查/比对用，可 diff、可贴进 issue；
 *   * `--gui` 回答"这些数据**是什么**"——Name / Type / Components / Ranges / Bounds，
 *     形状照 ParaView 的 Information 面板做（见 `Examples/IO/igvDump_GUI.md`）。
 *   两者都只走元数据快路径，都不解压任何 dataset。
 *
 * 提交状态：待提交。可用
 * `git log --diff-filter=A --format="%h %s" -- Examples/IO/igvDump.cpp` 查询引入它的提交。
 */

#include <IGV/iGameIGVFormat.h>
#include <IGV/iGameIGVHdf5Util.h>
#include <IGV/iGameIGVWriter.h>

#include <iGameAttributeSet.h>
#include <iGameCellType.h>
#include <iGameFlatArray.h>
#include <iGameUnstructuredMesh.h>

#include <hdf5.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef IGV_DUMP_QT
#include "IGVDumpInfoWindow.h"
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

namespace fs = std::filesystem;
using namespace iGame;

// =============================================================================
// HDF5 句柄
// =============================================================================

enum class Obj { File, Group, Dataset, Dataspace, Datatype, Attribute, Property };

struct Handle {
    hid_t id{-1};
    Obj kind{Obj::File};
    Handle() = default;
    Handle(hid_t h, Obj k) : id(h), kind(k) {}
    ~Handle() { Reset(); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
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

// =============================================================================
// 小工具
// =============================================================================

std::string Num(int64_t v) { return std::to_string(static_cast<long long>(v)); }

/// dtype 的可读名。优先用 IGV 的 dtype 映射（与规范同名），否则回落到 HDF5 类型类。
std::string DTypeText(hid_t type) {
    igv::DType dt = igv::DType::Unknown;
    std::string why;
    if (igv::DTypeFromFileType(type, dt, &why)) { return std::string(igv::DTypeName(dt)); }
    H5T_class_t cls = H5Tget_class(type);
    switch (cls) {
        case H5T_INTEGER: return H5Tget_sign(type) == H5T_SGN_NONE ? "uint" : "int";
        case H5T_FLOAT: return "float";
        case H5T_STRING: return "string";
        case H5T_ARRAY: return "array";
        case H5T_COMPOUND: return "compound";
        case H5T_REFERENCE: return "reference";
        case H5T_ENUM: return "enum";
        default: return "other";
    }
}

bool IsAsciiString(hid_t type) { return H5Tget_class(type) == H5T_STRING; }

std::string ReadStringValue(hid_t attr, hid_t type) {
    const size_t size = H5Tget_size(type);
    if (size == 0 || size > 4096) { return "<unreadable>"; }
    std::string buffer(size, '\0');
    if (H5Aread(attr, type, buffer.data()) < 0) { return "<unreadable>"; }
    const size_t nul = buffer.find('\0');
    if (nul != std::string::npos) { buffer.resize(nul); }
    return buffer;
}

/// 打印一个数值 attribute 的值（最多 8 个）。
std::string NumericAttributeValues(hid_t attr, hid_t type, size_t limit) {
    const hsize_t n = H5Aget_storage_size(attr);
    const size_t elementSize = H5Tget_size(type);
    if (elementSize == 0) { return "<unreadable>"; }
    const size_t count = static_cast<size_t>(n) / elementSize;
    if (count == 0) { return "[]"; }

    std::vector<unsigned char> raw(static_cast<size_t>(n));
    if (H5Aread(attr, type, raw.data()) < 0) { return "<unreadable>"; }

    const H5T_class_t cls = H5Tget_class(type);
    std::string out = "[";
    const size_t shown = std::min(count, limit);
    for (size_t i = 0; i < shown; ++i) {
        if (i > 0) { out += ", "; }
        char buf[64];
        if (cls == H5T_FLOAT && elementSize == 8) {
            double v = 0.0;
            std::memcpy(&v, raw.data() + i * elementSize, 8);
            std::snprintf(buf, sizeof(buf), "%.10g", v);
        } else if (cls == H5T_FLOAT && elementSize == 4) {
            float v = 0.0f;
            std::memcpy(&v, raw.data() + i * elementSize, 4);
            std::snprintf(buf, sizeof(buf), "%.7g", static_cast<double>(v));
        } else if (cls == H5T_INTEGER && elementSize == 8) {
            int64_t v = 0;
            std::memcpy(&v, raw.data() + i * elementSize, 8);
            std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
        } else if (cls == H5T_INTEGER && elementSize == 4) {
            int32_t v = 0;
            std::memcpy(&v, raw.data() + i * elementSize, 4);
            std::snprintf(buf, sizeof(buf), "%d", v);
        } else if (cls == H5T_INTEGER && elementSize == 1) {
            std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(raw[i]));
        } else {
            std::snprintf(buf, sizeof(buf), "?");
        }
        out += buf;
    }
    if (count > shown) { out += ", ... (" + Num(static_cast<int64_t>(count)) + " total)"; }
    out += "]";
    return out;
}

// =============================================================================
// 结构 dump
// =============================================================================

struct DumpOptions {
    bool withData{false};
    size_t valueLimit{8};
    std::string indent{"  "};
};

class Dumper {
public:
    explicit Dumper(DumpOptions options, std::string* out) : m_Options(options), m_Out(out) {}

    void PrintFile(hid_t file, const std::string& path) {
        Line(0, "FILE \"" + path + "\"");
        PrintAttributes(file, 0);
        // 根组本身
        Line(0, "GROUP \"/\" {");
        PrintGroupMembers(file, "/", 1);
        Line(0, "}");
    }

private:
    void Line(int depth, const std::string& text) {
        for (int i = 0; i < depth; ++i) { *m_Out += m_Options.indent; }
        *m_Out += text;
        *m_Out += "\n";
    }

    void PrintAttributes(hid_t loc, int depth) {
        const int n = H5Aget_num_attrs(loc);
        if (n <= 0) { return; }
        for (int i = 0; i < n; ++i) {
            const hid_t attr = H5Aopen_by_idx(loc, ".", H5_INDEX_NAME, H5_ITER_INC,
                                              static_cast<hsize_t>(i), H5P_DEFAULT, H5P_DEFAULT);
            if (attr < 0) { continue; }
            Handle attrHandle(attr, Obj::Attribute);
            char nameBuffer[1024] = {0};
            std::string attrName;
            if (H5Aget_name(attr, sizeof(nameBuffer), nameBuffer) >= 0) { attrName = nameBuffer; }
            const hid_t type = H5Aget_type(attr);
            Handle typeHandle(type, Obj::Datatype);
            const hid_t space = H5Aget_space(attr);
            Handle spaceHandle(space, Obj::Dataspace);

            std::string value;
            if (type >= 0 && IsAsciiString(type)) {
                value = "\"" + ReadStringValue(attr, type) + "\"";
            } else if (type >= 0) {
                value = NumericAttributeValues(attr, type, m_Options.valueLimit);
            }
            std::string shape;
            if (space >= 0) {
                const int rank = H5Sget_simple_extent_ndims(space);
                hsize_t dims[8] = {0};
                if (rank > 0 && rank <= 8) {
                    H5Sget_simple_extent_dims(space, dims, nullptr);
                    shape = " (";
                    for (int d = 0; d < rank; ++d) {
                        if (d > 0) { shape += ","; }
                        shape += Num(static_cast<int64_t>(dims[d]));
                    }
                    shape += ")";
                } else if (rank == 0) {
                    shape = " (scalar)";
                }
            }
            Line(depth, "ATTRIBUTE \"" + attrName + "\" = " + value +
                            "   [" + (type >= 0 ? DTypeText(type) : std::string("?")) + shape + "]");
        }
    }

    void PrintGroupMembers(hid_t group, const std::string& groupPath, int depth) {
        H5G_info_t info{};
        if (H5Gget_info(group, &info) < 0) { return; }
        const hsize_t n = info.nlinks;

        for (hsize_t i = 0; i < n; ++i) {
            char nameBuffer[1024] = {0};
            const ssize_t len = H5Lget_name_by_idx(group, ".", H5_INDEX_NAME, H5_ITER_INC, i,
                                                   nameBuffer, sizeof(nameBuffer), H5P_DEFAULT);
            if (len < 0) { continue; }
            const std::string name(nameBuffer);

            H5O_info2_t objectInfo{};
            if (H5Oget_info_by_name3(group, name.c_str(), &objectInfo, H5O_INFO_BASIC, H5P_DEFAULT) < 0) {
                Line(depth, "<unreadable link: " + name + ">");
                continue;
            }

            if (objectInfo.type == H5O_TYPE_GROUP) {
                Line(depth, "GROUP \"" + name + "\" {");
                Handle sub(H5Gopen2(group, name.c_str(), H5P_DEFAULT), Obj::Group);
                if (sub.Ok()) {
                    PrintAttributes(sub.Get(), depth + 1);
                    PrintGroupMembers(sub.Get(), groupPath + "/" + name, depth + 1);
                }
                Line(depth, "}");
            } else if (objectInfo.type == H5O_TYPE_DATASET) {
                PrintDataset(group, name, depth);
            } else {
                Line(depth, "OBJECT \"" + name + "\" (type " + Num(objectInfo.type) + ")");
            }
        }
    }

    void PrintDataset(hid_t group, const std::string& name, int depth) {
        Handle ds(H5Dopen2(group, name.c_str(), H5P_DEFAULT), Obj::Dataset);
        if (!ds.Ok()) {
            Line(depth, "DATASET \"" + name + "\" <cannot open>");
            return;
        }
        Handle type(H5Dget_type(ds.Get()), Obj::Datatype);
        Handle space(H5Dget_space(ds.Get()), Obj::Dataspace);

        std::string shape = "?";
        int rank = 0;
        hsize_t dims[8] = {0};
        if (space.Ok()) {
            rank = H5Sget_simple_extent_ndims(space.Get());
            if (rank > 0 && rank <= 8) {
                H5Sget_simple_extent_dims(space.Get(), dims, nullptr);
                shape = "(";
                for (int d = 0; d < rank; ++d) {
                    if (d > 0) { shape += ","; }
                    shape += Num(static_cast<int64_t>(dims[d]));
                }
                shape += ")";
            } else {
                shape = "(scalar)";
            }
        }

        std::string line = "DATASET \"" + name + "\"  " +
                           (type.Ok() ? DTypeText(type.Get()) : std::string("?")) + " " + shape;

        // chunk（只有分块存储才有）
        if (Handle dcpl(H5Dget_create_plist(ds.Get()), Obj::Property); dcpl.Ok()) {
            if (H5Pget_layout(dcpl.Get()) == H5D_CHUNKED) {
                hsize_t chunk[8] = {0};
                if (H5Pget_chunk(dcpl.Get(), rank <= 8 ? rank : 8, chunk) >= 0) {
                    line += "  chunk=(";
                    for (int d = 0; d < rank; ++d) {
                        if (d > 0) { line += ","; }
                        line += Num(static_cast<int64_t>(chunk[d]));
                    }
                    line += ")";
                }
            } else {
                line += "  contiguous";
            }
            // filter 链
            const int nfilters = H5Pget_nfilters(dcpl.Get());
            if (nfilters > 0) {
                line += "  filters=";
                for (int f = 0; f < nfilters; ++f) {
                    const H5Z_filter_t id = H5Pget_filter2(dcpl.Get(), static_cast<unsigned>(f),
                                                           nullptr, 0, nullptr, 0, nullptr, nullptr);
                    if (f > 0) { line += "+"; }
                    line += (id == H5Z_FILTER_DEFLATE) ? "gzip" : ("filter" + Num(id));
                }
            } else {
                line += "  uncompressed";
            }
        }

        const hsize_t storage = H5Dget_storage_size(ds.Get());
        line += "  storage=" + Num(static_cast<int64_t>(storage)) + " B";
        Line(depth, line);

        PrintAttributes(ds.Get(), depth + 1);

        if (m_Options.withData && type.Ok() && space.Ok()) {
            PrintDatasetValues(ds.Get(), type.Get(), space.Get(), name, dims, rank, depth + 1);
        }
    }

    void PrintDatasetValues(hid_t ds, hid_t type, hid_t space, const std::string& /*name*/,
                            const hsize_t* dims, int rank, int depth) {
        if (rank <= 0 || rank > 2) { return; }
        const size_t elementSize = H5Tget_size(type);
        if (elementSize == 0 || elementSize > 8) { return; }
        const hsize_t total = H5Sget_simple_extent_npoints(space);
        if (total == 0) { return; }

        // ⚠ 只读"够打印"的那一段。
        //
        // 这里踩过一个真实的堆溢出：`H5Dread(ds, type, H5S_ALL, H5S_ALL, ...)` 会按
        // **整个 dataspace** 写内存，而缓冲区只按 `valueLimit` 分配（本自检第一次
        // 运行就是 0xC0000374 堆损坏）。必须显式选 hyperslab + 等大的内存空间。
        hsize_t rows = dims[0];
        if (rank == 1) {
            const hsize_t want = std::max<hsize_t>(m_Options.valueLimit, 1);
            rows = std::min<hsize_t>(rows, want);
        } else {
            const hsize_t perRow = std::max<hsize_t>(dims[1], 1);
            const hsize_t wantRows = (m_Options.valueLimit + perRow - 1) / perRow;
            rows = std::min<hsize_t>(rows, std::max<hsize_t>(wantRows, 1));
        }
        if (rows == 0) { return; }

        hsize_t block[2] = {rows, 1};
        if (rank == 2) { block[1] = dims[1]; }

        Handle fspace(H5Dget_space(ds), Obj::Dataspace);
        hsize_t start[2] = {0, 0};
        if (!fspace.Ok() ||
            H5Sselect_hyperslab(fspace.Get(), H5S_SELECT_SET, start, nullptr, block, nullptr) < 0) {
            Line(depth, "DATA <cannot select>");
            return;
        }
        Handle mspace(H5Screate_simple(rank, block, nullptr), Obj::Dataspace);
        const hsize_t readCount = rows * (rank == 2 ? dims[1] : 1);
        std::vector<unsigned char> raw(static_cast<size_t>(readCount) * elementSize);
        if (!mspace.Ok() ||
            H5Dread(ds, type, mspace.Get(), fspace.Get(), H5P_DEFAULT, raw.data()) < 0) {
            Line(depth, "DATA <cannot read>");
            return;
        }

        const size_t shown = static_cast<size_t>(
            std::min<hsize_t>(readCount, std::max<hsize_t>(m_Options.valueLimit, 1)));
        std::string line = "DATA ";
        H5T_class_t cls = H5Tget_class(type);
        for (size_t i = 0; i < shown; ++i) {
            if (i > 0) { line += ", "; }
            char buf[64];
            if (cls == H5T_FLOAT && elementSize == 8) {
                double v = 0.0;
                std::memcpy(&v, raw.data() + i * elementSize, 8);
                std::snprintf(buf, sizeof(buf), "%.10g", v);
            } else if (cls == H5T_FLOAT && elementSize == 4) {
                float v = 0.0f;
                std::memcpy(&v, raw.data() + i * elementSize, 4);
                std::snprintf(buf, sizeof(buf), "%.7g", static_cast<double>(v));
            } else if (cls == H5T_INTEGER && elementSize == 8) {
                int64_t v = 0;
                std::memcpy(&v, raw.data() + i * elementSize, 8);
                std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
            } else if (cls == H5T_INTEGER && elementSize == 4) {
                int32_t v = 0;
                std::memcpy(&v, raw.data() + i * elementSize, 4);
                std::snprintf(buf, sizeof(buf), "%d", v);
            } else if (cls == H5T_INTEGER && elementSize == 1) {
                std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(raw[i]));
            } else {
                std::snprintf(buf, sizeof(buf), "?");
            }
            line += buf;
        }
        if (total > shown) { line += ", ... (" + Num(static_cast<int64_t>(total)) + " total)"; }
        Line(depth, line);
    }

    DumpOptions m_Options;
    std::string* m_Out;
};

bool DumpTo(hid_t file, const std::string& path, const DumpOptions& options, std::string& out) {
    out.clear();
    Dumper dumper(options, &out);
    dumper.PrintFile(file, path);
    return true;
}

bool DumpFile(const std::string& path, const DumpOptions& options, std::string& out, std::string& error) {
    Handle file(H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT), Obj::File);
    if (!file.Ok()) {
        error = "H5Fopen failed (not an HDF5 file, or the path is unreadable): " + path;
        return false;
    }
    return DumpTo(file.Get(), path, options, out);
}

// =============================================================================
// 自检：造一个样例 → dump → 断言关键行
// =============================================================================

/// 造一个"每类关键字段都有、且大到会触发 chunk + gzip"的小样例。
///
/// 尺寸必须让 `Points` 与 `Connectivity` 的未压缩字节数超过 §5.3 的 4 KiB 阈值，
/// 否则写出侧会按"小 dataset 不 chunk 不压缩"处理，dump 里就看不到
/// `chunk=` / `filters=gzip` —— 那两条断言会假失败（本自检第一版就是这样）。
/// 20x20x20 个六面体 = 8000 单元 / 9261 点：几毫秒就能造完，但足够越线。
UnstructuredMesh::Pointer MakeSelfCheckMesh() {
    const int n = 20;
    const int np = n + 1;

    auto mesh = UnstructuredMesh::New();
    Points::Pointer pts = Points::New();
    pts->Resize(static_cast<IGsize>(np) * np * np);
    IGsize idx = 0;
    for (int k = 0; k < np; ++k) {
        for (int j = 0; j < np; ++j) {
            for (int i = 0; i < np; ++i) {
                pts->RawPointer()[idx * 3 + 0] = static_cast<float>(i) * 0.5f;
                pts->RawPointer()[idx * 3 + 1] = static_cast<float>(j) * 0.5f;
                pts->RawPointer()[idx * 3 + 2] = static_cast<float>(k) * 0.5f;
                ++idx;
            }
        }
    }
    mesh->SetPoints(pts);

    CellArray::Pointer cells = CellArray::New();
    UnsignedIntArray::Pointer types = UnsignedIntArray::New();
    const auto pointIndex = [np](int i, int j, int k) -> igIndex {
        return static_cast<igIndex>((k * np + j) * np + i);
    };
    for (int k = 0; k < n; ++k) {
        for (int j = 0; j < n; ++j) {
            for (int i = 0; i < n; ++i) {
                igIndex hex[8] = {pointIndex(i, j, k),         pointIndex(i + 1, j, k),
                                  pointIndex(i + 1, j + 1, k), pointIndex(i, j + 1, k),
                                  pointIndex(i, j, k + 1),     pointIndex(i + 1, j, k + 1),
                                  pointIndex(i + 1, j + 1, k + 1), pointIndex(i, j + 1, k + 1)};
                cells->AddCellIds(hex, 8);
                types->AddValue(static_cast<unsigned int>(IG_HEXAHEDRON));
            }
        }
    }
    mesh->SetCells(cells, types);

    const IGsize pointCount = pts->GetNumberOfPoints();
    const IGsize cellCount = static_cast<IGsize>(n) * n * n;

    FloatArray::Pointer f = FloatArray::New();
    f->SetDimension(1);
    f->SetName("f_scalar");
    f->Resize(pointCount);
    for (IGsize i = 0; i < pointCount; ++i) { f->RawPointer()[i] = static_cast<float>(i) * 0.5f; }
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);

    DoubleArray::Pointer d = DoubleArray::New();
    d->SetDimension(1);
    d->SetName("d_scalar");
    d->Resize(pointCount);
    for (IGsize i = 0; i < pointCount; ++i) { d->RawPointer()[i] = 0.1 + 0.25 * static_cast<double>(i); }
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, d);

    UnsignedCharArray::Pointer u = UnsignedCharArray::New();
    u->SetDimension(1);
    u->SetName("u8_scalar");
    u->Resize(pointCount);
    for (IGsize i = 0; i < pointCount; ++i) {
        u->RawPointer()[i] = static_cast<unsigned char>(i % 251);
    }
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, u);

    FloatArray::Pointer c = FloatArray::New();
    c->SetDimension(3);
    c->SetName("cell_vec");
    c->Resize(cellCount);
    for (IGsize i = 0; i < cellCount * 3; ++i) { c->RawPointer()[i] = static_cast<float>(i) * 0.125f; }
    mesh->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_CELL, c);
    return mesh;
}

int RunSelfCheck() {
    const fs::path dir = fs::current_path() / "igv_dump_selfcheck_tmp";
    std::error_code ec;
    fs::create_directories(dir, ec);
    const fs::path file = dir / "igv_dump_selfcheck.igv";

    DataObject::Pointer mesh = MakeSelfCheckMesh();
    IGVWriter::Pointer writer = IGVWriter::New();
    if (!writer->WriteToFile(mesh, file.string())) {
        std::fprintf(stderr, "[FAIL] self-check: cannot write the sample .igv: %s\n",
                     writer->GetLastError().c_str());
        return 1;
    }

    std::string text;
    std::string error;
    DumpOptions options;
    options.withData = true;
    if (!DumpFile(file.string(), options, text, error)) {
        std::fprintf(stderr, "[FAIL] self-check: %s\n", error.c_str());
        return 1;
    }

    // 断言"该出现的东西真的出现了"。逐条列出，失败时能一眼看出缺哪一类。
    //
    // 刻意**不**把整行写死：dtype 文本在 IGV 里是 "f32(H5T_IEEE_F32LE)" 这种带 HDF5
    // 名字的形式，写死整行会让自检在无关改名上变红。这里按"名字"与"dtype/形状"分开断言。
    struct Expect {
        std::string needle;
        std::string why;
    };
    const std::vector<Expect> expectations = {
        {"GROUP \"IGV\"", "the /IGV root group"},
        {"ATTRIBUTE \"Type\"", "root attribute Type"},
        {"ATTRIBUTE \"IGVDataType\"", "root attribute IGVDataType"},
        {"ATTRIBUTE \"CellTypeEncoding\"", "root attribute CellTypeEncoding (A3)"},
        {"ATTRIBUTE \"Bounds\"", "root attribute Bounds"},
        {"DATASET \"Points\"  f32", "Points dtype is f32 (A8)"},
        {"(9261,3)", "Points shape is (npoints,3)"},
        {"DATASET \"NumberOfPoints\"  i64", "explicit NumberOfPoints (i64)"},
        {"DATASET \"NumberOfCells\"  i64", "explicit NumberOfCells (i64)"},
        {"DATASET \"NumberOfConnectivityIds\"  i64", "explicit NumberOfConnectivityIds (i64)"},
        {"DATASET \"Connectivity\"  i32", "Connectivity dtype is i32"},
        {"DATASET \"Offsets\"  i64", "Offsets dtype is i64 (not u32)"},
        {"(8001)", "Offsets shape is ncells+1"},
        {"DATASET \"Types\"  u8", "Types dtype is u8 (not u32)"},
        {"chunk=(", "chunk shape is visible in the dump"},
        {"filters=gzip", "the DEFLATE filter is visible in the dump"},
        {"GROUP \"PointData\"", "PointData group"},
        {"GROUP \"CellData\"", "CellData group"},
        {"GROUP \"FieldData\"", "FieldData group"},
        {"ATTRIBUTE \"attachment\"", "per-attribute attachment"},
        {"ATTRIBUTE \"range\"", "per-attribute range"},
        {"f_scalar", "f32 attribute"},
        {"d_scalar", "f64 attribute"},
        {"u8_scalar", "u8 attribute"},
        {"DATA ", "--data prints values"},
    };

    int failures = 0;
    for (const Expect& e : expectations) {
        const bool found = text.find(e.needle) != std::string::npos;
        if (found) {
            std::printf(" [ ok ] %-46s\n", e.why.c_str());
        } else {
            std::printf(" [FAIL] %-46s missing: '%s'\n", e.why.c_str(), e.needle.c_str());
            ++failures;
        }
    }

    // 反向自检：一个**不该**出现的东西（PointSet 才没有拓扑，这里必须没有 Block）。
    if (text.find("Block_0") != std::string::npos) {
        std::printf(" [FAIL] the dump must not invent a Block_0 group\n");
        ++failures;
    } else {
        std::printf(" [ ok ] no invented Block_0 group\n");
    }

    std::printf("\nchecks: %d total, failures %d\n", static_cast<int>(expectations.size()) + 1, failures);
    return failures == 0 ? 0 : 1;
}

void PrintUsage(const char* exe) {
    std::printf("Usage: %s <file.igv> [--data] [--limit N]\n"
                "       %s <file.igv> --gui   (Information panel; Qt builds only)\n"
                "       %s --gui              (empty panel: drag & drop / File > Open)\n"
                "       %s <file.igv> --text  (force the text mode, e.g. in scripts)\n"
                "       %s --gui --screenshot PNG [file.igv]\n"
                "                             (render one frame to PNG and exit; UI smoke test)\n"
                "       %s --self-check\n"
                "\n"
                "Launched with no arguments (double-click) a Qt build opens the empty panel.\n"
                "Launched by Explorer / 'Open with' (own console) with a file, it opens the\n"
                "panel too; from a command prompt it prints the text dump as before.\n",
                exe, exe, exe, exe, exe, exe);
}

#ifdef _WIN32
/// 这个控制台是不是**本进程独占**的？
///
/// 判据 `GetConsoleProcessList()`：只有本进程时，说明这个控制台是
/// 资源管理器双击 / 右键"打开方式"启动时系统为它单独创建的
/// （从 `cmd`/PowerShell 启动时，列表里还有那个 shell）。
///
/// 这个判据有两个用途，都依赖同一个事实：
///   1. GUI 模式下隐藏这个黑窗口（否则面板旁边会多一个空控制台）；
///   2. **不带显式模式参数时自动选 GUI**：双击一个 `.igv` 时，用户要的是面板，
///      不是把一个即将随进程结束而消失的文本 dump 打进黑窗口。
bool ConsoleIsExclusivelyOurs() {
    DWORD pids[4] = {0, 0, 0, 0};
    return ::GetConsoleProcessList(pids, 4) == 1;
}

void HideConsoleIfOwnedByThisProcess() {
    if (ConsoleIsExclusivelyOurs()) {
        if (HWND window = ::GetConsoleWindow()) {
            ::ShowWindow(window, SW_HIDE);
        }
    }
}
#else
bool ConsoleIsExclusivelyOurs() { return false; }
void HideConsoleIfOwnedByThisProcess() {}
#endif

int ReportGuiUnavailable() {
    // 明确告诉用户"这个构建没有 GUI"，而不是静默退化成文本模式 ——
    // 后者会让人以为 --gui 起作用了。
    std::fprintf(stderr,
                 "GUI mode is not available in this build: igvDump was configured without "
                 "Qt Widgets (ENABLE_QT_MODULE / Qt5).\n"
                 "The text mode still works; or rebuild with a Qt-enabled configuration.\n");
    return 3;
}

}  // namespace

int main(int argc, char** argv) {
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);  // 不让 HDF5 自己的 stderr 噪声混进来

    if (argc < 2) {
#ifdef IGV_DUMP_QT
        // **双击 exe**（没有参数）：开空面板，而不是打一行用法就退出。
        // 但只在自己独占控制台时这么做（= 资源管理器启动）：从终端敲 `igvDump`
        // 仍然打印用法（旧行为），这样脚本/CTest 里少写参数也不会弹出窗口把
        // 进程挂住。用法文本始终可以用 `--help` 拿到。
        if (ConsoleIsExclusivelyOurs()) {
            HideConsoleIfOwnedByThisProcess();
            return RunIGVDumpInfoWindow(std::string(), std::string());
        }
#endif
        PrintUsage(argv[0]);
        return 2;
    }
    if (std::string(argv[1]) == "--self-check") { return RunSelfCheck(); }
    if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        PrintUsage(argv[0]);
        return 0;
    }

    // 参数解析：新旧写法都收（`igvDump --gui file.igv` 与 `igvDump file.igv --gui`）。
    // 注意**文件仍是唯一的位置参数**，`--self-check` 只认 argv[1]（保持原有语义，
    // 也让 CTest 的 `igvDump --self-check` 一字不改地继续工作）。
    DumpOptions options;
    bool gui = false;
    bool forceText = false;
    bool limitGiven = false;
    std::string path;
    std::string screenshot;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--gui" || a == "-g") {
            gui = true;
        } else if (a == "--text" || a == "--console") {
            // 显式要文本：给脚本/批处理用，压过"双击自动 GUI"的推断。
            forceText = true;
        } else if (a == "--screenshot" && i + 1 < argc) {
            // UI 冒烟：渲染一帧后退出。
            screenshot = argv[++i];
            gui = true;
        } else if (a == "--data") {
            options.withData = true;
        } else if (a == "--limit" && i + 1 < argc) {
            options.valueLimit = static_cast<size_t>(std::atoi(argv[++i]));
            limitGiven = true;
        } else if (!a.empty() && a.front() == '-') {
            std::fprintf(stderr, "unknown argument: %s\n", a.c_str());
            PrintUsage(argv[0]);
            return 2;
        } else if (path.empty()) {
            path = a;
        } else {
            std::fprintf(stderr, "unexpected second file argument: %s\n", a.c_str());
            PrintUsage(argv[0]);
            return 2;
        }
    }

    // 模式推断（只在**没有显式指定**时有意义）：
    //   有文件 + 控制台是本进程独占（资源管理器双击 / 右键"打开方式"）
    //   + 没有 --data/--limit（那些是文本模式的参数）→ 用面板。
    // 从 cmd/PowerShell 启动时控制台不是独占的，因此旧脚本行为**一字不变**。
    bool autoGui = false;
    if (!gui && !forceText && !path.empty() && !options.withData && !limitGiven) {
        autoGui = ConsoleIsExclusivelyOurs();
    }

    if (gui || autoGui) {
        HideConsoleIfOwnedByThisProcess();
#ifdef IGV_DUMP_QT
        return RunIGVDumpInfoWindow(path, screenshot);
#else
        (void)screenshot;
        return ReportGuiUnavailable();
#endif
    }

    if (path.empty()) {
        PrintUsage(argv[0]);
        return 2;
    }

    std::string text;
    std::string error;
    if (!DumpFile(path, options, text, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    std::fwrite(text.data(), 1, text.size(), stdout);
    return 0;
}
