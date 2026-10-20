/**
 * @file  TestIGVMetadata.cpp
 * @brief P3 验收程序：IGV 元数据快路径（V3.1–V3.5）。
 *
 * 对应 `kia/igv分阶段实现方案.md` §7（元数据快路径）/ §9 P3 / §11.3（零解压可断言化）。
 *
 * ## 这个测试为什么存在（防止什么退化）
 *
 * 快路径的价值全部建立在一个物理事实上：**不读 dataset 数据就不会解压 gzip chunk**。
 * 这条约束如果只写在文档里，第一次有人为了省事在快路径里顺手 `H5Dread` 一下 `Points`
 * 就会悄悄破功，而且**功能测试全绿**（返回值依然正确，只是慢了几十秒）——这类退化用
 * 「结果对不对」永远测不出来，只能用**调用计数**测。
 *
 * 因此本测试的核心不是「元数据对不对」，而是：
 *   1. 元数据必须与全量读路径**独立对得上**（两套代码，结果一致才说明双方都对，V3.3）；
 *   2. 快路径期间 `IGVStats::h5dreadCount` 必须恰好为 **1**（唯一允许的是 16 字节的
 *      `Version` dataset，写侧 `compress=false` 永不解压）；任何数据读取都会让它 > 1。
 *
 * ## 重要边界用例及其原因
 *
 * - **`hasBounds == false` 的文件（写侧未提供 Bounds）**：快路径**不得**回落到读 `Points`
 *   去现算包围盒（§7.2 末条）。这类文件在 V3.3 中跳过 bounds 比对，而不是让测试去「补算」。
 * - **`PointSet`（无拓扑）**：`cellCount` 必须为 0 且 `hasCellCount == false`，
 *   防止把「没有单元」误报成「0 个单元」，从而与「未知」混淆。
 * - **多分量属性（components > 1）**：`range` 长度必须是 `2*(1+components)`，
 *   `components` 必须取自 dataspace 的第 2 维而不是靠猜；且 range 的数值要与
 *   **读回数据重算**的结果一致（不拿文件里的 range 自证，避免「写出和写错一起错」）。
 * - **内存镜像路径**：`ReadFromMemory` 与 `ReadFromFile` 必须给出**完全相同**的元数据
 *   （C/S 端拿到的是字节流而不是路径）。
 *
 * ## 修复/实现提交状态
 *
 * **待提交**（P3 实现尚未提交）。首次提交后，用下面的命令回填真实提交号并更新本段：
 *
 * ```
 * git log --diff-filter=A --format="%h %s" -- Examples/IO/TestIGVMetadata.cpp
 * ```
 */

#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>

#include <IGV/iGameIGVMetadataReader.h>
#include <IGV/iGameIGVStats.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace iGame;

namespace {

int g_failures = 0;
int g_checks = 0;

void Check(bool condition, const std::string& what) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  [FAIL] " << what << "\n";
    }
}

/// 相对误差比较（对 0 附近退化为绝对比较）。
bool NearlyEqual(double a, double b, double rel = 1e-6) {
    if (!std::isfinite(a) || !std::isfinite(b)) { return a == b; }
    const double scale = std::max(1.0, std::max(std::fabs(a), std::fabs(b)));
    return std::fabs(a - b) / scale <= rel;
}

std::string Str(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.6g", v);
    return buf;
}

/// attachment 枚举 → IGV token（与写侧 §3.8 一致）。
std::string AttachmentToken(IGenum a) {
    switch (a) {
        case IG_POINT: return "point";
        case IG_CELL: return "cell";
        case IG_EDGE: return "edge";
        case IG_POINT_BOX: return "point_box";
        case IG_CELL_BOX: return "cell_box";
        case IG_CHANGE: return "change";
        case IG_DRAGPOINT: return "dragpoint";
        case IG_MID_POINT: return "mid_point";
        default: return {};
    }
}

/// 从读回的数组**重新计算** range（§8.2 语义），用于与快路径报告的 range 交叉验证。
///
/// 为什么不直接比较文件里写的 range：那等于用「写出方的输出」给「同一份输出」做证词。
/// 重算走的是「数据 → range」，与写侧「数据 → range」独立实现，双方一致才有意义。
std::vector<double> ComputeRange(ArrayObject::Pointer array) {
    std::vector<double> out;
    if (!array) { return out; }

    const int components = array->GetDimension();
    const IGsize elements = array->GetNumberOfElements();
    const IGsize values = array->GetNumberOfValues();
    if (components <= 0 || elements <= 0 || values == 0) { return out; }

    if (components == 1) {
        double mn = std::numeric_limits<double>::max();
        double mx = -std::numeric_limits<double>::max();
        bool any = false;
        for (IGsize i = 0; i < values; ++i) {
            const double v = array->GetValue(i);
            if (!std::isfinite(v)) { continue; }
            any = true;
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
        if (!any) { return {0.0, 0.0}; }
        return {mn, mx};
    }

    double magMin = std::numeric_limits<double>::max();
    double magMax = -std::numeric_limits<double>::max();
    std::vector<double> cmin(static_cast<size_t>(components), std::numeric_limits<double>::max());
    std::vector<double> cmax(static_cast<size_t>(components), -std::numeric_limits<double>::max());
    bool any = false;

    for (IGsize e = 0; e < elements; ++e) {
        double sum = 0.0;
        for (int c = 0; c < components; ++c) {
            const double v = array->GetValue(e * static_cast<IGsize>(components) + static_cast<IGsize>(c));
            if (!std::isfinite(v)) { continue; }
            any = true;
            sum += v * v;
            const size_t k = static_cast<size_t>(c);
            cmin[k] = std::min(cmin[k], v);
            cmax[k] = std::max(cmax[k], v);
        }
        const double mag = std::sqrt(sum);
        magMin = std::min(magMin, mag);
        magMax = std::max(magMax, mag);
    }
    if (!any) {
        out.assign(2u * (1u + static_cast<size_t>(components)), 0.0);
        return out;
    }
    out.push_back(magMin);
    out.push_back(magMax);
    for (int c = 0; c < components; ++c) {
        out.push_back(cmin[static_cast<size_t>(c)]);
        out.push_back(cmax[static_cast<size_t>(c)]);
    }
    return out;
}

/// 全量读侧的独立快照（第二套代码，用来给快路径作证）。
struct FullArray {
    std::string name;
    std::string attachment;
    int components{1};
    int64_t count{0};
    int valueSize{0};
    std::vector<double> range;  ///< 由数据重算，见 ComputeRange
};

struct FullStats {
    int64_t pointCount{0};
    int64_t cellCount{0};
    bool hasCellCount{false};
    bool hasBounds{false};
    double bounds[6]{0, 0, 0, 0, 0, 0};
    std::vector<FullArray> arrays;
};

FullStats AnalyzeFull(DataObject::Pointer obj) {
    FullStats s;
    if (!obj) { return s; }

    double bmin[3] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                      std::numeric_limits<double>::max()};
    double bmax[3] = {-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
                      -std::numeric_limits<double>::max()};

    if (auto ps = DynamicCast<PointSet>(obj)) {
        if (Points::Pointer pts = ps->GetPoints()) {
            s.pointCount = static_cast<int64_t>(pts->GetNumberOfPoints());
            for (int64_t i = 0; i < s.pointCount; ++i) {
                const Point p = pts->GetPoint(static_cast<IGsize>(i));
                for (int a = 0; a < 3; ++a) {
                    bmin[a] = std::min(bmin[a], static_cast<double>(p[a]));
                    bmax[a] = std::max(bmax[a], static_cast<double>(p[a]));
                }
            }
            if (s.pointCount > 0) { s.hasBounds = true; }
        }
    }
    if (CellArray::Pointer cells = obj->GetCellArray()) {
        s.cellCount = static_cast<int64_t>(cells->GetNumberOfCells());
        s.hasCellCount = true;
    }
    if (s.hasBounds) {
        for (int a = 0; a < 3; ++a) {
            s.bounds[2 * a] = bmin[a];
            s.bounds[2 * a + 1] = bmax[a];
        }
    }

    if (AttributeSet* set = obj->GetAttributeSet()) {
        const IGsize n = set->GetNumberOfAttributes();
        for (IGsize i = 0; i < n; ++i) {
            AttributeSet::Attribute& a = set->GetAttribute(i);
            if (a.IsNone()) { continue; }
            ArrayObject::Pointer arr = a.GetPointer();
            if (!arr) { continue; }
            FullArray fa;
            fa.name = arr->GetName();
            fa.attachment = AttachmentToken(a.GetAttachmentType());
            fa.components = arr->GetDimension();
            fa.count = static_cast<int64_t>(arr->GetNumberOfElements());
            fa.valueSize = static_cast<int>(arr->GetArrayTypedSize());
            fa.range = ComputeRange(arr);
            s.arrays.emplace_back(std::move(fa));
        }
    }
    return s;
}

/// 收集候选 .igv 文件（argv 优先；否则从可执行文件所在目录逐级向上找）。
std::vector<fs::path> CollectFiles(int argc, char** argv) {
    std::vector<fs::path> files;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const fs::path p(argv[i]);
            std::error_code ec;
            if (fs::is_directory(p, ec)) {
                for (const auto& e : fs::recursive_directory_iterator(p)) {
                    if (e.is_regular_file() && e.path().extension() == ".igv") { files.push_back(e.path()); }
                }
            } else if (fs::is_regular_file(p, ec)) {
                files.push_back(p);
            }
        }
    } else {
        std::vector<fs::path> roots;
#if defined(IGAME_IGV_SOURCE_DIR)
        // CMake 注入的源码目录：让测试不依赖"当前工作目录"（跨 IDE / CTest 都稳）。
        roots.push_back(fs::path(IGAME_IGV_SOURCE_DIR));
#endif
        if (argv[0] != nullptr && *argv[0] != '\0') {
            std::error_code ec;
            roots.push_back(fs::absolute(fs::path(argv[0]), ec).parent_path());
        }
        roots.push_back(fs::current_path());

        for (fs::path root : roots) {
            for (int up = 0; up < 5 && !root.empty(); ++up) {
                for (const char* sub : {"Examples/IGVTestModels", "Examples/Models",
                                        "igv_roundtrip_tmp", "Examples/Release/igv_roundtrip_tmp"}) {
                    const fs::path dir = root / sub;
                    std::error_code ec;
                    if (!fs::is_directory(dir, ec)) { continue; }
                    for (const auto& e : fs::directory_iterator(dir)) {
                        if (e.is_regular_file() && e.path().extension() == ".igv") { files.push_back(e.path()); }
                    }
                }
                root = root.parent_path();
            }
        }
    }

    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    return files;
}

/// 对一个文件跑 V3.1–V3.5。
void CheckOneFile(const fs::path& path, int64_t& outPoints, int64_t& outCells) {
    std::cout << "\n== " << path.string() << "\n";

    igv::IGVMetadata meta;
    std::string error;
    const auto t0 = std::chrono::steady_clock::now();
    const bool metaOk = FileIO::ReadIGVMetadata(path.string(), meta, &error);
    const auto t1 = std::chrono::steady_clock::now();
    const uint64_t reads = igv::IGVStats::h5dreadCount.load();
    const uint64_t opens = igv::IGVStats::datasetOpenCount.load();
    const uint64_t areads = igv::IGVStats::h5areadCount.load();

    Check(metaOk, "V3.1 ReadIGVMetadata 成功" + (metaOk ? std::string() : ("：" + error)));
    if (!metaOk) { return; }

    // ---- V3.2：快路径只允许读 Version ----
    Check(reads == 1, "V3.2 快路径 h5dreadCount == 1（实际 " + std::to_string(reads) +
                              "；> 1 说明读了 dataset 数据，零解压已破功）");
    std::cout << "   stats: h5dread=" << reads << " datasetOpen=" << opens << " h5aread=" << areads << "\n";

    // ---- 基本字段（V3.1） ----
    Check(!meta.type.empty(), "V3.1 type 非空（" + meta.type + "）");
    Check(!meta.igvDataType.empty(), "V3.1 igvDataType 非空（" + meta.igvDataType + "）");
    Check(meta.versionMajor == 1 && meta.versionMinor == 0,
          "V3.1 version == {1,0}（实际 {" + std::to_string(meta.versionMajor) + "," +
                  std::to_string(meta.versionMinor) + "}）");
    // 0 点的文件是**合法**的（P1 的拒绝路径用例会在临时目录里留下 reject_empty.igv 这类
    // 产物，它本身是结构合法的空网格）。本程序会顺手扫描这些临时目录，因此不能把
    // "点数 == 0" 当失败；下面的逐值校验对空网格本来也没有意义，直接跳过并**明说**。
    if (meta.pointCount == 0) {
        // ASCII only: new console output must not add to the file's pre-existing
        // non-ASCII debt (see kia/P5实施记录.md section 5).
        std::cout << "   [skip] this file has 0 points (empty mesh / a rejection-path artifact); "
                     "the point-count dependent V3.1 checks do not apply to it\n";
        outPoints = 0;
        outCells = meta.hasCellCount ? meta.cellCount : 0;
        return;
    }
    Check(meta.pointCount > 0, "V3.1 pointCount > 0（" + std::to_string(meta.pointCount) + "）");
    Check(meta.hasDimensions == (meta.igvDataType == "StructuredMesh"),
          "V3.1 hasDimensions 只在 StructuredMesh 上为真");

    // ---- 全量读：第二套代码（V3.3） ----
    const auto t2 = std::chrono::steady_clock::now();
    DataObject::Pointer obj = FileIO::ReadFile(path.string());
    const auto t3 = std::chrono::steady_clock::now();
    Check(obj != nullptr, "V3.3 全量读成功");
    if (!obj) { return; }

    const FullStats full = AnalyzeFull(obj);
    outPoints = meta.pointCount;
    outCells = meta.cellCount;

    Check(meta.pointCount == full.pointCount,
          "V3.3 pointCount 一致（快 " + std::to_string(meta.pointCount) + " vs 全量 " +
                  std::to_string(full.pointCount) + "）");
    Check(meta.hasCellCount == full.hasCellCount,
          "V3.3 hasCellCount 一致（快 " + std::to_string(meta.hasCellCount) + " vs 全量 " +
                  std::to_string(full.hasCellCount) + "）");
    if (meta.hasCellCount && full.hasCellCount) {
        Check(meta.cellCount == full.cellCount,
              "V3.3 cellCount 一致（快 " + std::to_string(meta.cellCount) + " vs 全量 " +
                      std::to_string(full.cellCount) + "）");
    }

    // ---- V3.3：bounds。文件没写 Bounds 时**不允许**回落算（§7.2 末条） ----
    if (!meta.hasBounds) {
        std::cout << "   bounds: 文件未提供（hasBounds=false），按 §7.2 不回落读 Points\n";
    } else {
        Check(full.hasBounds, "V3.3 文件有 Bounds → 全量读也能算出 bounds");
        if (full.hasBounds) {
            for (int a = 0; a < 3; ++a) {
                Check(NearlyEqual(meta.bounds[2 * a], full.bounds[2 * a]) &&
                              NearlyEqual(meta.bounds[2 * a + 1], full.bounds[2 * a + 1]),
                      "V3.3 bounds 轴 " + std::to_string(a) + " 一致（快 [" + Str(meta.bounds[2 * a]) + "," +
                              Str(meta.bounds[2 * a + 1]) + "] vs 全量 [" + Str(full.bounds[2 * a]) + "," +
                              Str(full.bounds[2 * a + 1]) + "]）");
            }
        }
    }

    // ---- V3.1：属性清单逐项对应 ----
    Check(meta.arrays.size() == full.arrays.size(),
          "V3.1 属性个数一致（快 " + std::to_string(meta.arrays.size()) + " vs 全量 " +
                  std::to_string(full.arrays.size()) + "）");

    for (const igv::IGVArrayInfo& info : meta.arrays) {
        Check(!info.attributeType.empty(), "V3.1 属性 '" + info.name + "' attributeType 非空");
        Check(!info.attachment.empty(), "V3.1 属性 '" + info.name + "' attachment 非空");

        const size_t expectedRange = (info.components == 1)
                                         ? 2u
                                         : 2u * (1u + static_cast<size_t>(info.components));
        Check(info.range.size() == expectedRange,
              "V3.1 属性 '" + info.name + "' range 长度 == " + std::to_string(expectedRange) + "（实际 " +
                      std::to_string(info.range.size()) + "）");

        const FullArray* match = nullptr;
        for (const FullArray& fa : full.arrays) {
            if (fa.name == info.name) { match = &fa; break; }
        }
        Check(match != nullptr, "V3.3 全量读里能找到属性 '" + info.name + "'");
        if (match == nullptr) { continue; }

        Check(match->attachment == info.attachment,
              "V3.1 属性 '" + info.name + "' attachment 一致（快 " + info.attachment + " vs 全量 " +
                      match->attachment + "）");
        Check(match->components == info.components,
              "V3.1 属性 '" + info.name + "' components 一致（快 " + std::to_string(info.components) +
                      " vs 全量 " + std::to_string(match->components) + "）");
        Check(match->count == info.count,
              "V3.1 属性 '" + info.name + "' count 一致（快 " + std::to_string(info.count) + " vs 全量 " +
                      std::to_string(match->count) + "）");
        Check(match->valueSize == info.valueSize,
              "V3.1 属性 '" + info.name + "' valueSize 一致（快 " + std::to_string(info.valueSize) +
                      " vs 全量 " + std::to_string(match->valueSize) + "）");

        // 1.x：快路径报出的定宽 dtype（DataType/DataTypeBits）必须与字节宽度自洽，
        // 且能被解析回一个合法 DType。这条防的是"字段填了但填错"——比不填更危险。
        {
            const igv::DType reported = igv::ParseDTypeToken(info.dTypeToken);
            Check(reported != igv::DType::Unknown,
                  "V3.1 array '" + info.name + "' reports a fixed-width DataType token ('" +
                          info.dTypeToken + "')");
            Check(igv::DTypeBits(reported) == info.dTypeBits && info.dTypeBits == info.valueSize * 8,
                  "V3.1 array '" + info.name + "' DataTypeBits (" + std::to_string(info.dTypeBits) +
                          ") agrees with valueSize (" + std::to_string(info.valueSize) + " bytes)");
        }

        // range 与「读回数据重算」一致（不是拿文件的 range 自证）
        if (match->range.size() == info.range.size()) {
            bool same = true;
            for (size_t k = 0; k < info.range.size(); ++k) {
                if (!NearlyEqual(info.range[k], match->range[k], 1e-6)) { same = false; break; }
            }
            Check(same, "V3.1 属性 '" + info.name + "' range 与数据重算值一致");
        } else {
            Check(false, "V3.1 属性 '" + info.name + "' range 与数据重算值的长度不同");
        }
    }

    // ---- 内存路径必须与文件路径等价 ----
    {
        std::ifstream in(path, std::ios::binary);
        std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        Check(!bytes.empty(), "内存路径：文件可读入缓冲");
        if (!bytes.empty()) {
            igv::IGVMetadata memMeta;
            std::string memError;
            const bool ok = IGVMetadataReader::ReadFromMemory(bytes.data(), bytes.size(), memMeta, &memError);
            Check(ok, "内存路径 ReadFromMemory 成功" + (ok ? std::string() : ("：" + memError)));
            if (ok) {
                bool sameArrays = memMeta.arrays.size() == meta.arrays.size();
                for (size_t i = 0; sameArrays && i < meta.arrays.size(); ++i) {
                    sameArrays = memMeta.arrays[i].name == meta.arrays[i].name &&
                                 memMeta.arrays[i].components == meta.arrays[i].components &&
                                 memMeta.arrays[i].count == meta.arrays[i].count;
                }
                Check(memMeta.pointCount == meta.pointCount && memMeta.cellCount == meta.cellCount &&
                              memMeta.type == meta.type && memMeta.igvDataType == meta.igvDataType &&
                              sameArrays,
                      "内存路径与文件路径元数据一致");
            }
        }
    }

    const double metaMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double fullMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "   time: meta=" << Str(metaMs) << "ms  full=" << Str(fullMs) << "ms  ratio="
              << Str(fullMs > 0.0 ? metaMs / fullMs : 0.0) << "\n";

    // ---- V3.4：性能量级（只对足够大的文件断言，避免小文件固定开销干扰） ----
    if (meta.pointCount >= 100000 && fullMs > 1.0) {
        Check(metaMs < fullMs * 0.1, "V3.4 大文件 t_meta < t_full * 0.1（" + Str(metaMs) + "ms vs " +
                                             Str(fullMs) + "ms）");
    }
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<fs::path> files = CollectFiles(argc, argv);
    if (files.empty()) {
        std::cout << "TestIGVMetadata: 没找到任何 .igv 文件（可把文件或目录作为参数传入）\n";
        return 1;
    }
    std::cout << "TestIGVMetadata: " << files.size() << " 个文件\n";

    int64_t maxPoints = 0;
    int64_t maxCells = 0;
    for (const fs::path& p : files) {
        int64_t points = 0;
        int64_t cells = 0;
        CheckOneFile(p, points, cells);
        maxPoints = std::max(maxPoints, points);
        maxCells = std::max(maxCells, cells);
    }

    std::cout << "\n大文件覆盖：max pointCount=" << maxPoints << " max cellCount=" << maxCells
              << "（V3.5 需要 >= 100000 点、且 Points 为 gzip 的文件；当前"
              << (maxPoints >= 100000 ? "已满足" : "未满足——请用大模型再跑一次") << "）\n";

    std::cout << "\n结果：" << (g_failures == 0 ? "全部通过" : "有失败") << "（checks=" << g_checks
              << " failures=" << g_failures << "）\n";
    return g_failures == 0 ? 0 : 1;
}
