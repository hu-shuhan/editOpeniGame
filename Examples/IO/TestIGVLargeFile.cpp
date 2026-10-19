/**
 * @file  TestIGVLargeFile.cpp
 * @brief IGV P5 验收程序（二）：>4 GiB 单文件 + 亿级单元（P5-12 / P5-13）。
 *
 * 对应 kia/igv分阶段实现方案.md §9「P5 — 鲁棒性与大文件回归」的 P5-12、P5-13。
 * **默认不构建**：由 `-DIGAME_ENABLE_IGV_LARGE_TESTS=ON` 打开，理由见下。
 *
 * ============================================================================
 * 为什么单独一个程序、且默认关闭
 * ============================================================================
 *
 * 默认规模（1e8 个六面体）实测需要：
 *   * 内存：DataObject 约 5.3 GB（点 1.2 GB + 连通性 3.2 GB + 偏移 0.8 GB + 类型 0.1 GB）；
 *   * 磁盘：**不压缩**的 `.igv` 约 5.3 GB（压缩会把它压到 4 GiB 以下，那样就测不到
 *     ">4 GiB 文件"这件事了，所以这里刻意用 `compression=none`）；
 *   * 时间：分钟级（造网格 + 写 + 读回 + 校验）。
 *
 * 让每个开发者与 CI 都无条件承担这个代价是不合适的，所以：
 *   * CMake 侧用 `IGAME_ENABLE_IGV_LARGE_TESTS` 门控；
 *   * 程序侧支持 `--cells N` 与 `--quick`（≈200k 单元、几十 MB），
 *     平时的冒烟验证用 quick，真正的 §P5-12 判据只在默认/显式规模下评。
 *
 * ============================================================================
 * 这个程序要证明什么
 * ============================================================================
 *
 * 1. **写出**一份 >4 GiB 的 `.igv` 成功，且走的是**直写路径**（`DirectPath`）——
 *    core VFD 的内存镜像会再吃掉一份文件大小的内存（R2），5.3 GB 的数据 + 5.3 GB
 *    镜像在 16 GB 机器上会直接失败。所以这里同时验证 P7-4 的前置条件。
 * 2. **读回**成功，点数 / 单元数 / 最后一个 offset / 抽样连通性全部正确 ——
 *    也就是"64 位偏移与 i64 Offsets 没有溢出"。
 * 3. **峰值内存**有记录（P5-13 明确要求），供后续 P7 决策参考。
 *
 * 明确**不**声称覆盖：`Offsets` 超过 `u32` 范围（>4.29e9 个连通性索引）需要
 * ≥5e8 个单元、约 20 GB 内存，本轮不覆盖（见 kia/P5实施记录.md §5）。
 *
 * 提交状态：待提交。可用
 * `git log --diff-filter=A --format="%h %s" -- Examples/IO/TestIGVLargeFile.cpp` 查询。
 */

#include <IGV/iGameIGVReader.h>
#include <IGV/iGameIGVWriter.h>

#include <iGameCellType.h>
#include <iGameFlatArray.h>
#include <iGameUnstructuredMesh.h>

#include <hdf5.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#else
#include <fstream>
#include <unistd.h>
#endif

namespace {

using namespace iGame;
namespace fs = std::filesystem;

int g_checks = 0;
int g_failed = 0;

void Check(bool ok, const std::string& what) {
    ++g_checks;
    std::cout << (ok ? " [ ok ] " : " [FAIL] ") << what << "\n";
    if (!ok) { ++g_failed; }
}

void Info(const std::string& what) { std::cout << " [info] " << what << "\n"; }

std::string Num(int64_t v) { return std::to_string(static_cast<long long>(v)); }

std::string Bytes(int64_t v) {
    char buf[64];
    if (v >= (int64_t{1} << 30)) {
        std::snprintf(buf, sizeof(buf), "%.2f GiB", static_cast<double>(v) / 1073741824.0);
    } else if (v >= (1 << 20)) {
        std::snprintf(buf, sizeof(buf), "%.1f MiB", static_cast<double>(v) / 1048576.0);
    } else if (v >= 1024) {
        std::snprintf(buf, sizeof(buf), "%.1f KiB", static_cast<double>(v) / 1024.0);
    } else {
        std::snprintf(buf, sizeof(buf), "%lld B", static_cast<long long>(v));
    }
    return buf;
}

double Seconds(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

/// 进程峰值工作集（字节）。拿不到就返回 -1，调用方必须如实说明"未测量"。
int64_t PeakWorkingSetBytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (K32GetProcessMemoryInfo(GetCurrentProcess(),
                                reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                                sizeof(counters)) == 0) {
        return -1;
    }
    return static_cast<int64_t>(counters.PeakWorkingSetSize);
#else
    std::ifstream status("/proc/self/status");
    std::string key;
    while (status >> key) {
        if (key == "VmHWM:") {
            int64_t kb = 0;
            status >> kb;
            return kb * 1024;
        }
        status.ignore(4096, '\n');
    }
    return -1;
#endif
}

/// 把 cells 分解成一个尽量接近立方体的 (nx, ny, nz)。
///
/// 为什么要"接近立方体"：同样的单元数下，表面积越小 → 点数越少 → 文件越小。
/// 1e8 = 1000x1000x100 时点是 1.01e8；若取 1e8 x 1 x 1 则点数是 2e8，凭空多一倍。
bool FactorizeBalanced(int64_t cells, int64_t& nx, int64_t& ny, int64_t& nz) {
    if (cells < 1) { return false; }
    int64_t best = cells;
    bool found = false;
    // nx 从小到大试到立方根；取"最大可行 nx"（越接近立方体越好）。
    for (int64_t a = 1; a * a * a <= cells; ++a) {
        if (cells % a != 0) { continue; }
        const int64_t rest = cells / a;
        for (int64_t b = a; b * b <= rest; ++b) {
            if (rest % b != 0) { continue; }
            const int64_t c = rest / b;
            const int64_t maxDim = std::max({a, b, c});
            if (!found || maxDim < best) {
                found = true;
                best = maxDim;
                nx = a;
                ny = b;
                nz = c;
            }
        }
    }
    return found;
}

struct Options {
    int64_t cells{100000000};  ///< 默认 1e8（P5-13）
    bool quick{false};
    bool keep{false};
    fs::path dir;
};

void PrintUsage(const char* exe) {
    std::cout << "Usage: " << exe << " [--cells N] [--quick] [--dir PATH] [--keep]\n"
              << "  --cells N    number of hexahedral cells (default 100000000 = 1e8)\n"
              << "  --quick      smoke size (200000 cells); the >4 GiB check is NOT evaluated\n"
              << "  --dir PATH   where the huge .igv is written (default: <cwd>/igv_large_tmp)\n"
              << "  --keep       do not delete the .igv afterwards\n";
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--cells" && i + 1 < argc) {
            opt.cells = std::atoll(argv[++i]);
        } else if (a == "--quick") {
            opt.quick = true;
            opt.cells = 200000;
        } else if (a == "--keep") {
            opt.keep = true;
        } else if (a == "--dir" && i + 1 < argc) {
            opt.dir = argv[++i];
        } else if (a == "-h" || a == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else {
            std::cout << "unknown argument: " << a << "\n";
            PrintUsage(argv[0]);
            return 2;
        }
    }
    if (opt.dir.empty()) { opt.dir = fs::current_path() / "igv_large_tmp"; }
    std::error_code ec;
    fs::create_directories(opt.dir, ec);
    if (ec) {
        std::cout << "[FAIL] cannot create the output directory: " << opt.dir.string() << "\n";
        return 1;
    }
    const fs::path file = opt.dir / "large.igv";

    std::cout << "================================================================\n"
              << "IGV P5 acceptance (2/2): >4 GiB single file and 1e8 cells\n"
              << "Covers kia/igv implementation plan section 9 (P5-12 / P5-13)\n"
              << "================================================================\n";

    int64_t nx = 0, ny = 0, nz = 0;
    if (!FactorizeBalanced(opt.cells, nx, ny, nz)) {
        std::cout << "[FAIL] cannot factor " << Num(opt.cells) << " into a 3-D grid\n";
        return 1;
    }
    const int64_t points = (nx + 1) * (ny + 1) * (nz + 1);
    const int64_t connectivity = opt.cells * 8;
    const int64_t offsets = opt.cells + 1;
    const int64_t estimatedBytes = points * 3 * 4 + connectivity * 4 + offsets * 8 + opt.cells;

    std::cout << "grid          : " << Num(nx) << " x " << Num(ny) << " x " << Num(nz) << " = "
              << Num(opt.cells) << " hexahedral cells\n"
              << "points        : " << Num(points) << "\n"
              << "expected size : " << Bytes(estimatedBytes) << " (uncompressed)\n"
              << "output        : " << file.string() << "\n";

    if (points > 2147483647LL) {
        std::cout << "[FAIL] " << Num(points)
                  << " points exceeds the i32 connectivity index range; use fewer cells\n";
        return 1;
    }

    // 磁盘够不够？先问清楚，别跑到一半才失败（5.3 GB 的临时文件很难看）。
    const fs::space_info space = fs::space(opt.dir, ec);
    if (!ec) {
        Info("free disk on the target volume: " + Bytes(static_cast<int64_t>(space.available)));
        if (static_cast<int64_t>(space.available) < estimatedBytes * 12 / 10) {
            std::cout << "[FAIL] not enough free disk: need about "
                      << Bytes(estimatedBytes * 12 / 10) << ", have "
                      << Bytes(static_cast<int64_t>(space.available)) << "\n";
            return 1;
        }
    }

    // -------------------------------------------------------------------------
    // 1) 造一个巨大的规则六面体网格（批量填充，不走 AddCellIds 的逐单元路径）
    // -------------------------------------------------------------------------
    auto t0 = std::chrono::steady_clock::now();
    auto mesh = UnstructuredMesh::New();
    {
        Points::Pointer pts = Points::New();
        pts->Resize(static_cast<IGsize>(points));
        float* p = pts->RawPointer();
        // 规则笛卡尔网格：坐标直接算，不查表。
        for (int64_t k = 0; k < nz + 1; ++k) {
            for (int64_t j = 0; j < ny + 1; ++j) {
                for (int64_t i = 0; i < nx + 1; ++i) {
                    const int64_t id = (k * (ny + 1) + j) * (nx + 1) + i;
                    p[id * 3 + 0] = static_cast<float>(i) * 0.5f;
                    p[id * 3 + 1] = static_cast<float>(j) * 0.5f;
                    p[id * 3 + 2] = static_cast<float>(k) * 0.5f;
                }
            }
        }
        mesh->SetPoints(pts);

        IdArray::Pointer ids = IdArray::New();
        ids->SetNumberOfIds(static_cast<IGsize>(connectivity));
        igIndex* c = ids->RawPointer();
        UnsignedIntArray::Pointer off = UnsignedIntArray::New();
        off->Resize(static_cast<IGsize>(offsets));
        unsigned int* o = off->RawPointer();

        int64_t cell = 0;
        for (int64_t k = 0; k < nz; ++k) {
            for (int64_t j = 0; j < ny; ++j) {
                for (int64_t i = 0; i < nx; ++i) {
                    const int64_t base = (k * (ny + 1) + j) * (nx + 1) + i;
                    const int64_t row = (ny + 1) * (nx + 1);
                    igIndex* c0 = c + cell * 8;
                    c0[0] = static_cast<igIndex>(base);
                    c0[1] = static_cast<igIndex>(base + 1);
                    c0[2] = static_cast<igIndex>(base + 1 + (nx + 1));
                    c0[3] = static_cast<igIndex>(base + (nx + 1));
                    c0[4] = static_cast<igIndex>(base + row);
                    c0[5] = static_cast<igIndex>(base + row + 1);
                    c0[6] = static_cast<igIndex>(base + row + 1 + (nx + 1));
                    c0[7] = static_cast<igIndex>(base + row + (nx + 1));
                    o[cell] = static_cast<unsigned int>(cell * 8);
                    ++cell;
                }
            }
        }
        o[opt.cells] = static_cast<unsigned int>(connectivity);

        CellArray::Pointer cells = CellArray::New();
        cells->SetData(ids, off);

        UnsignedIntArray::Pointer types = UnsignedIntArray::New();
        types->Resize(static_cast<IGsize>(opt.cells));
        std::fill(types->RawPointer(), types->RawPointer() + opt.cells,
                  static_cast<unsigned int>(IG_HEXAHEDRON));
        mesh->SetCells(cells, types);
        cells->Squeeze();
    }
    auto t1 = std::chrono::steady_clock::now();
    Info("grid built in " + std::to_string(Seconds(t0, t1)) + " s, peak working set " +
         Bytes(PeakWorkingSetBytes()));

    // -------------------------------------------------------------------------
    // 2) 写出：直写路径 + 不压缩（压缩会让文件掉到 4 GiB 以下，测不到"大文件"）
    // -------------------------------------------------------------------------
    //
    // ⚠ 必须直接调 `WriteToFilePathDirect()`，不能只设 `writeMode = DirectPath` 再走
    // `WriteToFile()`：后者是基类 `Execute()` 的流程（GenerateBuffers → 落盘），
    // IGVWriter 看到这种组合会**降级成内存镜像**并打 WARN —— 5.3 GB 的数据会因此
    // 需要 10.6 GB 峰值内存（本程序 --quick 首次运行就是这样，见 P5 实施记录）。
    igv::IGVWriteOptions options;
    options.writeMode = igv::IGVWriteMode::DirectPath;
    options.compression = igv::IGVCompression::None;
    options.generator = "iGameVis IGVWriter (P5 large-file regression)";

    IGVWriter::Pointer writer = IGVWriter::New();
    writer->SetWriteOptions(options);
    const auto t2 = std::chrono::steady_clock::now();
    const bool wrote = writer->WriteToFilePathDirect(mesh, file.string());
    const auto t3 = std::chrono::steady_clock::now();
    Check(wrote, "P5-12: writing a " + Bytes(estimatedBytes) + " .igv succeeds (direct-write path)");
    if (!wrote) {
        std::cout << "        writer error: " << writer->GetLastError() << "\n";
        return 1;
    }
    Info("write took " + std::to_string(Seconds(t2, t3)) + " s (no in-memory file image)");

    const int64_t fileSize = static_cast<int64_t>(fs::file_size(file, ec));
    Check(!ec && fileSize > 0, "P5-12: the .igv exists and is non-empty (" + Bytes(fileSize) + ")");
    Info("file size on disk: " + Bytes(fileSize) + " (" + Num(fileSize) + " bytes)");

    if (opt.quick) {
        // 不伪造通过：quick 模式明确声明"这条判据没有评"。
        Info("quick mode: the '> 4 GiB' judgement is NOT evaluated in this run "
             "(re-run without --quick to evaluate P5-12)");
    } else {
        constexpr int64_t kFourGiB = int64_t{4} * 1024 * 1024 * 1024;
        Check(fileSize > kFourGiB, "P5-12: the file exceeds 4 GiB (" + Bytes(fileSize) + " > " +
                                       Bytes(kFourGiB) + ")");
        Check(fileSize > estimatedBytes * 9 / 10,
              "P5-12: with compression=none the file size matches the raw array size (no silent compaction)");
    }

    // 写完就释放源对象，避免"源 + 读回"两份同时在内存里（5.3 GB x2）。
    mesh = nullptr;
    const int64_t peakDuringWrite = PeakWorkingSetBytes();
    Info("peak working set after write: " + Bytes(peakDuringWrite));

    // -------------------------------------------------------------------------
    // 3) 直接检查**文件里**的 Offsets：这是"i64 偏移没有溢出/没有被截断"的原始证据
    // -------------------------------------------------------------------------
    //
    // 为什么不能只看读回对象的 `GetOffset()`：解码适配器对**定长**网格走
    // `AddUnstructuredFixedCells()`，于是重建出来的 CellArray 用"固定单元大小"表达，
    // `GetOffset()` 只有 1 个元素（{0}）—— 这是设计如此（§6.2 步骤 4），
    // 但它把"文件里的 Offsets 到底对不对"这件事藏起来了，所以这里回到文件本身去看。
    {
        hid_t fileId = H5Fopen(file.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (fileId < 0) {
            Check(false, "P5-12: cannot reopen the .igv with HDF5 to inspect /IGV/Offsets");
        } else {
            hid_t ds = H5Dopen2(fileId, "/IGV/Offsets", H5P_DEFAULT);
            if (ds < 0) {
                Check(false, "P5-12: /IGV/Offsets is missing from the file");
            } else {
                hid_t space = H5Dget_space(ds);
                hsize_t dims[2] = {0, 0};
                H5Sget_simple_extent_dims(space, dims, nullptr);
                Check(static_cast<int64_t>(dims[0]) == offsets,
                      "P5-12: the file stores Offsets as ncells+1 (" + Num(static_cast<int64_t>(dims[0])) + ")");

                // 读最后一个元素（跨过 4 GiB 之后的位置）与中间那个。
                const auto readAt = [&](hsize_t index, int64_t& value) -> bool {
                    hsize_t start[1] = {index};
                    hsize_t count[1] = {1};
                    if (H5Sselect_hyperslab(space, H5S_SELECT_SET, start, nullptr, count, nullptr) < 0) {
                        return false;
                    }
                    hid_t mem = H5Screate_simple(1, count, nullptr);
                    const herr_t rc = H5Dread(ds, H5T_NATIVE_INT64, mem, space, H5P_DEFAULT, &value);
                    H5Sclose(mem);
                    return rc >= 0;
                };
                int64_t last = -1;
                if (readAt(static_cast<hsize_t>(offsets - 1), last)) {
                    Check(last == connectivity,
                          "P5-12: Offsets[last] == 8 * cells (" + Num(last) + ") - no 32-bit overflow");
                } else {
                    Check(false, "P5-12: cannot read the last Offsets element");
                }
                int64_t mid = -1;
                if (readAt(static_cast<hsize_t>(opt.cells / 2), mid)) {
                    Check(mid == (opt.cells / 2) * 8,
                          "P5-12: Offsets[mid] is exact (" + Num(mid) + ")");
                } else {
                    Check(false, "P5-12: cannot read the middle Offsets element");
                }
                H5Sclose(space);
                H5Dclose(ds);
            }
            H5Fclose(fileId);
        }
    }

    // -------------------------------------------------------------------------
    // 4) 读回并校验（64 位偏移 / i64 Offsets 不溢出的实证）
    // -------------------------------------------------------------------------
    const auto t4 = std::chrono::steady_clock::now();
    IGVReader::Pointer reader = IGVReader::New();
    DataObject::Pointer back = reader->ReadFile(file.string());
    const auto t5 = std::chrono::steady_clock::now();
    Check(back != nullptr, "P5-12: the >4 GiB file is read back successfully");
    if (!back) {
        std::cout << "        reader error: " << reader->GetLastError() << "\n";
        if (!opt.keep) { fs::remove(file, ec); }
        return 1;
    }
    Info("read took " + std::to_string(Seconds(t4, t5)) + " s, peak working set " +
         Bytes(PeakWorkingSetBytes()));

    auto backMesh = DynamicCast<UnstructuredMesh>(back);
    Check(backMesh != nullptr, "P5-12: the object read back is an UnstructuredMesh");
    if (backMesh) {
        const int64_t backPoints = static_cast<int64_t>(backMesh->GetNumberOfPoints());
        const int64_t backCells = static_cast<int64_t>(backMesh->GetNumberOfCells());
        Check(backPoints == points, "P5-12: point count preserved (" + Num(backPoints) + ")");
        Check(backCells == opt.cells, "P5-12: cell count preserved (" + Num(backCells) + ")");

        // 读回对象里的 Offsets 说明：定长网格会被适配器折叠成 fixedCellSize，
        // 所以这里只如实记录，不当作断言（真正的位置校验在上一节直接读文件）。
        if (CellArray::Pointer cells = backMesh->GetCells()) {
            if (UnsignedIntArray::Pointer off = cells->GetOffset()) {
                Info("read-back Offsets array length = " +
                     Num(static_cast<int64_t>(off->GetNumberOfValues())) +
                     " (a fixed-size mesh is expected to collapse to 1; the file itself was checked above)");
            }

            // 抽样连通性：第一个单元、最后一个单元、中间那个。
            const auto checkCell = [&](int64_t cellId, const std::string& label) {
                const igIndex* ids = nullptr;
                const int n = cells->GetCellIds(static_cast<IGsize>(cellId), ids);
                if (n != 8 || ids == nullptr) {
                    Check(false, "P5-12: " + label + " should have 8 vertices, got " + Num(n));
                    return;
                }
                const int64_t i = cellId % nx;
                const int64_t j = (cellId / nx) % ny;
                const int64_t k = cellId / (nx * ny);
                const int64_t base = (k * (ny + 1) + j) * (nx + 1) + i;
                const int64_t row = (ny + 1) * (nx + 1);
                const igIndex expected[8] = {
                    static_cast<igIndex>(base), static_cast<igIndex>(base + 1),
                    static_cast<igIndex>(base + 1 + (nx + 1)), static_cast<igIndex>(base + (nx + 1)),
                    static_cast<igIndex>(base + row), static_cast<igIndex>(base + row + 1),
                    static_cast<igIndex>(base + row + 1 + (nx + 1)),
                    static_cast<igIndex>(base + row + (nx + 1))};
                bool same = true;
                for (int v = 0; v < 8; ++v) {
                    if (ids[v] != expected[v]) {
                        same = false;
                        break;
                    }
                }
                Check(same, "P5-12: connectivity of " + label + " (cell " + Num(cellId) + ") is exact");
            };
            checkCell(0, "the first cell");
            checkCell(opt.cells / 2, "the middle cell");
            checkCell(opt.cells - 1, "the last cell");
        } else {
            Check(false, "P5-12: read-back mesh has no CellArray");
        }

        if (UnsignedIntArray* types = backMesh->GetCellTypes()) {
            const IGsize n = types->GetNumberOfValues();
            bool allHex = (static_cast<int64_t>(n) == opt.cells);
            for (IGsize i = 0; allHex && i < n; i += (n > 1000 ? 997 : 1)) {
                if (static_cast<int>(types->GetValue(i)) != IG_HEXAHEDRON) { allHex = false; }
            }
            Check(allHex, "P5-12: Types are all IG_HEXAHEDRON (sampled) across " + Num(static_cast<int64_t>(n)) +
                              " cells");
        } else {
            Check(false, "P5-12: read-back mesh has no Types array");
        }
    }

    // -------------------------------------------------------------------------
    // 4) 收尾
    // -------------------------------------------------------------------------
    back = nullptr;
    const int64_t peak = PeakWorkingSetBytes();
    Info("peak working set (whole run): " + Bytes(peak) +
         (peak >= 0 ? "" : " (not measured on this platform)"));

    if (opt.keep) {
        Info("keeping the file: " + file.string());
    } else {
        fs::remove(file, ec);
        Info("removed the temporary file (" + std::string(ec ? "FAILED: " + ec.message() : "ok") + ")");
    }

    std::cout << "\n----------------------------------------------------------------------\n"
              << "checks: " << g_checks << " total, failures " << g_failed << "\n";
    if (g_failed == 0) {
        std::cout << "ALL PASSED: the large-file path works (see the sizes and timings above).\n";
        return 0;
    }
    std::cout << "FAILED: see the [FAIL] lines above.\n";
    return 1;
}
