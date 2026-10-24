// ParallelVolumeRendering.cpp — 并行体绘制入口（阶段 3 分布式合成 + 阶段 4 CPU 后端 +
//   阶段 5 交互窗口 + 阶段 6 C/S 服务端）
//
// CLI（命名参数，默认 CPU 后端）：
//   <program> -i <input> [-t <timestep>] [--resample <res>] [--gpu|--cpu]
//             [--interactive] [--server [--port <n>] [--autoplay-interval <ms>]]
//             [-f <field>] [--direct|--binary-swap] [--blockwise|--pixelwise]
//     -i, --input <file>   输入数据（.pvd/.vtm/.igcm 多分块，或 .vtr/.vts/.vtu 单块）
//     -f, --field <name>   直接指定渲染字段（srun 批处理无 stdin 时必需）
//     -t, --timestep <n>   PVD 时间步（默认 0；非 PVD 忽略）
//     -r, --resample <n>   每块重采样分辨率（默认 64，最小 2）
//         --gpu / --cpu    渲染后端（默认 --cpu；--cpu 无头、不依赖 OpenGL/GLFW）
//         --interactive    交互窗口（阶段 5，仅 --cpu 后端）：rank 0 弹窗显示合成结果，
//                          左键拖动旋转、滚轮缩放、左下角 colorbar、拖动期间显示 fps
//         --server          C/S 服务端（阶段 6，仅 --cpu 后端）：rank 0 开放 TCP 端口
//                          供前端（ParallelVolumeClient）连接，流式回传合成图
//         --port <n>        --server 监听端口（默认 11111）
//         --direct / --binary-swap
//                          图像合成策略：稀疏 ROI 汇聚（默认）/ binary-swap 交换
//         --blockwise / --pixelwise
//                          合成的深度口径：块级超块中心深度（默认）/ 逐像素首命中深度
//         --autoplay-interval <ms>
//                          A 键自动播放的换帧间隔（默认 2000 = 2 秒一帧，仅 --server）
// 未指定 --field 时，启动后 rank0 列出该数据可渲染的字段（点/单元标量、向量），提示按
// 名称或编号选择，随后把所选字段广播给所有 rank。
//
// 流程（对标 UnifiedVersion 的 TestPVolumeRender.cpp）：
//   多分块 → iGameVolumeDistributor 分发 → iGameVolumeResampleFilter 重采样 →
//   全局标量范围 AllReduce + 相机参数 Broadcast（保证各 rank 传输函数/投影矩阵一致）→
//   每个 rank 无头渲染自己超块（透明背景 + 预乘 alpha + 深度）→
//   iGameCompositePass 深度有序合成 → rank 0 输出合成 PNG。
//
// 输出（写入 <exe>/out/<时间戳>_*）：
//   - 每个 rank 1 张自己的局部渲染图（_rank<rank>.png，+Z 视角，用于查验分发/分区）；
//   - rank 0 输出 6 个主轴视角（±X/±Y/±Z）的合成图（_composited_<axis>.png，用于查验
//     各分块是否按深度正确合成为一张完整体）。
//
// 进程模型：
//   --cpu（默认）：每个 rank 用 iGameVolumeRayCastCPU 无头渲染自己超块（纯 CPU、
//                 不依赖 OpenGL/GLFW，超算可用），再经 iGameCompositePass 合成；
//   --gpu：所有 rank（含 rank 0）用隐藏 GLFW 窗口离屏渲染（GPU 验证通路，Windows
//          下配软件 GL / Mesa llvmpipe 即可无显示器运行）。
// -n 1 与 -n N 走同一代码路径，合成结果可直接逐像素对比。
// 注意：本头（winsock2）必须最先包含，先于任何会引入 windows.h 的 iGame 头（iGameScene
// -> GLVendor -> glad -> windows.h），否则 Windows 下 winsock.h 与 winsock2.h 冲突。
#include "ParallelVolumeProtocol.h"
#include "iGameFileIO.h"
#include "iGameParallelContext.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameCompositePass.h"
#include "iGameThreadPool.h"
#include "iGameVolumeRayCastCPU.h"
#include "iGameVolumeTransferFunction.h"
#include "VolumeMeshAlgorithm/iGameVolumeDistributor.h"
#include "VolumeMeshAlgorithm/iGameVolumeResampleFilter.h"
#include "iGameResourcePath.h"
#include "ParallelVolumeServer.h"    // 必须先于 ParallelVolumeInteractive.h（winsock2 先于 windows.h）
#include "ParallelVolumeInteractive.h"
#include "ParallelVolumePixelComposite.h" // --pixelwise 逐像素合成（pvr 模块内，不动 iGameCore）

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#if defined(__linux__)
#  include <sched.h>   // sched_getaffinity：拿到本进程真正可用的核数（srun --cpu-bind 后）
#endif
#if !defined(_WIN32)
#  include <unistd.h>  // gethostname / getpid（启动日志里的节点名与进程号）
#endif
#if defined(_WIN32)
// Winsock2 已由 ParallelVolumeProtocol.h 先行包含，这里再引入 windows.h 不会冲突
// （否则 windows.h 里的 winsock.h 会与 winsock2.h 重定义）。
#  include <windows.h> // GetComputerNameA / GetCurrentProcessId
#endif

#include <GLFW/glfw3.h>

// 单头文件 PNG 编码器（自包含，含 DEFLATE）。仅本示例用于输出最终合成图。
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../ThirdParty/glfw-3.4/deps/stb_image_write.h"

namespace {
// ---------------------------------------------------------------------------
// 本进程真正可用的核数（用于设定 iGame ThreadPool 的线程数）。
//
// 为什么必须显式设置：iGameThreadPool 的默认线程数是编译期常量 12，与运行时实际可用
// 的核数无关。超算上用 `srun --cpu-bind=cores` 把每个 rank 绑到 1 个核上（56 rank/节点），
// 此时每个 rank 再开 12 个线程就是 12 倍超订，线程切换开销会把每帧渲染时间放大数倍；
// 更糟的是 ThreadPool 单例会按 std::thread::hardware_concurrency() 预创建线程池，在没有
// 显式设置线程数时每个 rank 会创建 56 个线程（56 rank/节点 → 3136 线程/节点）。
//
// 取值优先级：OMP_NUM_THREADS（作业脚本常用来约束线程数） > sched_getaffinity（Linux，
// 反映 cgroup/cpu-bind 的真实可用核） > std::thread::hardware_concurrency()。
// ---------------------------------------------------------------------------
int DetectUsableCoreCount() {
    if (const char* omp = std::getenv("OMP_NUM_THREADS")) {
        if (*omp != '\0') {
            const int v = std::atoi(omp);
            if (v >= 1) { return v; }
        }
    }
    int n = 0;
#if defined(__linux__)
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) == 0) { n = CPU_COUNT(&set); }
#endif
    if (n <= 0) {
        n = static_cast<int>(std::thread::hardware_concurrency());
    }
    if (n <= 0) { n = 1; }
    return n;
}

// ---------------------------------------------------------------------------
// 启动 / 加载日志用的小工具。
//
// 为什么用「主机名」定位节点，而不是 MPI_Get_processor_name：
//   · 本示例必须能在 IGAME_ENABLE_MPI=OFF 的单进程构建下编译（ParallelContext 退化为
//     Rank()==0 / Size()==1）；直接调用 MPI 符号会破坏无 MPI 构建；
//   · gethostname（Linux/macOS）与 GetComputerName（Windows）在所有档次的机器（本机、
//     容器、超算计算节点）上都存在，返回的就是「这个进程跑在哪台机器上」，与
//     MPI_Get_processor_name 返回的节点名是同一信息；
//   · 用 rank 0 汇总出的「节点 -> rank 区间」映射，就能定位「哪个分块文件在哪台机器上
//     加载」，并且对 1000 rank 也只打印几十行。
// ---------------------------------------------------------------------------
std::string LocalHostName() {
#if defined(_WIN32)
    char buf[256] = {0};
    DWORD len = static_cast<DWORD>(sizeof(buf));
    if (GetComputerNameA(buf, &len) != 0 && len > 0) {
        return std::string(buf, static_cast<size_t>(len));
    }
#else
    char buf[256] = {0};
    if (gethostname(buf, sizeof(buf) - 1) == 0 && buf[0] != '\0') {
        return std::string(buf);
    }
#endif
    const char* env = std::getenv("HOSTNAME");
    if (!env || !*env) { env = std::getenv("COMPUTERNAME"); }
    return (env && *env) ? std::string(env) : std::string("unknown");
}

long CurrentProcessId() {
#if defined(_WIN32)
    return static_cast<long>(GetCurrentProcessId());
#else
    return static_cast<long>(getpid());
#endif
}

const char* TargetArchName() {
#if defined(_M_X64) || defined(__x86_64__)
    return "x86_64";
#elif defined(_M_ARM64) || defined(__aarch64__)
    return "aarch64";
#elif defined(__EMSCRIPTEN__)
    return "wasm";
#else
    return "unknown";
#endif
}

std::string CompilerName() {
#if defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_VER);
#elif defined(__clang__)
    return "Clang " + std::to_string(__clang_major__) + "." +
           std::to_string(__clang_minor__) + "." +
           std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
    return "GCC " + std::to_string(__GNUC__) + "." +
           std::to_string(__GNUC_MINOR__) + "." +
           std::to_string(__GNUC_PATCHLEVEL__);
#else
    return "unknown";
#endif
}

// 环境变量快照：未设置时返回 "-"（日志里显式可见，而不是留空）。
std::string EnvOrDash(const char* name) {
    const char* v = std::getenv(name);
    return (v && *v) ? std::string(v) : std::string("-");
}

// 人类可读字节数："1.23 GiB (1324567890 bytes)"；< 1 KiB 时只打整数 B。
std::string FormatBytes(double bytes) {
    static const char* kUnits[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    if (!(bytes > 0.0)) { return "0 B"; }
    int unit = 0;
    double v = bytes;
    while (v >= 1024.0 && unit < 5) {
        v /= 1024.0;
        ++unit;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(unit == 0 ? 0 : 2) << v << ' '
        << kUnits[unit];
    if (unit > 0) {
        oss << " (" << static_cast<long long>(bytes) << " bytes)";
    }
    return oss.str();
}

std::string FormatSeconds(double sec) {
    std::ostringstream oss;
    if (sec < 1.0) {
        // 亚秒级用毫秒，避免大量 0.00s 看不出差异（重采样常只有几毫秒）。
        oss << std::fixed << std::setprecision(sec < 0.1 ? 2 : 1) << sec * 1000.0
            << "ms";
    } else {
        oss << std::fixed << std::setprecision(sec < 10.0 ? 2 : 1) << sec << "s";
    }
    return oss.str();
}

std::string FormatMs(double ms) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(ms < 10.0 ? 2 : 1) << ms << "ms";
    return oss.str();
}

// 体素数：134217728 (134.21 Mvox)。
std::string FormatVoxels(double v) {
    std::ostringstream oss;
    oss << static_cast<long long>(v);
    if (v >= 1e9) {
        oss << " (" << std::fixed << std::setprecision(2) << v / 1e9 << " Gvox)";
    } else if (v >= 1e6) {
        oss << " (" << std::fixed << std::setprecision(2) << v / 1e6 << " Mvox)";
    } else if (v >= 1e3) {
        oss << " (" << std::fixed << std::setprecision(1) << v / 1e3 << " Kvox)";
    }
    return oss.str();
}

double MsSince(const std::chrono::steady_clock::time_point& a,
               const std::chrono::steady_clock::time_point& b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

double SecSince(const std::chrono::steady_clock::time_point& a,
                const std::chrono::steady_clock::time_point& b) {
    return std::chrono::duration<double>(b - a).count();
}

// 打印「节点 -> rank 区间」映射（仅 rank 0 调用）。连续 rank 合并成 "0-17" 这样的区间，
// 1000 rank / 56 节点也只占几十行；节点数超过 kMaxNodes 时截断，避免日志爆炸。
void PrintNodeMap(const std::map<std::string, std::vector<int>>& byNode, int size) {
    std::cout << "[nodes] " << byNode.size() << " node(s) hosting " << size
              << " rank(s):\n";
    constexpr std::size_t kMaxNodes = 64;
    std::size_t printed = 0;
    for (const auto& kv : byNode) {
        if (printed++ >= kMaxNodes) {
            std::cout << "[nodes] ... (" << (byNode.size() - kMaxNodes)
                      << " more node(s) omitted)\n";
            break;
        }
        std::cout << "[nodes]   " << kv.first << " (" << kv.second.size()
                  << " rank(s)): ";
        const auto& ranks = kv.second;
        for (std::size_t i = 0; i < ranks.size();) {
            std::size_t j = i;
            while (j + 1 < ranks.size() && ranks[j + 1] == ranks[j] + 1) { ++j; }
            if (i > 0) { std::cout << ", "; }
            if (j > i) {
                std::cout << ranks[i] << '-' << ranks[j];
            } else {
                std::cout << ranks[i];
            }
            i = j + 1;
        }
        std::cout << '\n';
    }
    std::cout.flush();
}

// 本进程写入 PNG 的时间戳（秒级）；同一批合成图用同一时间戳。
std::string MakeTimestamp() {    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);   // MSVC: errno_t localtime_s(struct tm*, const time_t*)
#else
    localtime_r(&t, &tm);   // POSIX: struct tm* localtime_r(const time_t*, struct tm*)
#endif
    char buf[64]{};
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
    return std::string(buf);
}

// 与 iGameVolumeRayCastGPU::UploadVolumeTexture 相同的字段选择口径：
// 激活属性 -> 点标量/向量 -> 单元标量/向量。
iGame::ArrayObject::Pointer PickScalarField(iGame::StructuredMesh* mesh,
                                            bool& isCell) {
    auto* attrs = mesh->GetAttributeSet();
    if (!attrs) { return nullptr; }

    const int nAttr = static_cast<int>(attrs->GetNumberOfAttributes());
    const int ai = mesh->GetAttributeIndex();
    if (ai >= 0 && ai < nAttr) {
        auto& a = attrs->GetAttribute(ai);
        if (!a.IsNone() && a.pointer &&
            (a.type == IG_SCALAR || a.type == IG_VECTOR)) {
            isCell = (a.attachmentType == IG_CELL);
            return a.pointer;
        }
    }
    for (int i = 0; i < nAttr; ++i) {
        auto& a = attrs->GetAttribute(i);
        if (a.IsNone() || !a.pointer) { continue; }
        if (a.attachmentType == IG_POINT &&
            (a.type == IG_SCALAR || a.type == IG_VECTOR)) {
            isCell = false;
            return a.pointer;
        }
    }
    for (int i = 0; i < nAttr; ++i) {
        auto& a = attrs->GetAttribute(i);
        if (a.IsNone() || !a.pointer) { continue; }
        if (a.attachmentType == IG_CELL &&
            (a.type == IG_SCALAR || a.type == IG_VECTOR)) {
            isCell = true;
            return a.pointer;
        }
    }
    return nullptr;
}

// 字段第 i 个元素的大小：标量取分量 0，向量取模长（与 GPU 上传口径一致）。
double FieldMagnitude(iGame::ArrayObject* arr, IGsize i) {
    const int dim = arr->GetDimension();
    if (dim <= 1) { return arr->GetValue(i); }
    std::vector<float> e(static_cast<size_t>(dim));
    arr->GetElement(i, e);
    double m = 0.0;
    for (int d = 0; d < dim; ++d) {
        m += static_cast<double>(e[static_cast<size_t>(d)]) * e[static_cast<size_t>(d)];
    }
    return std::sqrt(m);
}

// 计算本 rank 体数据的局部标量范围（用于 AllReduce 求全局范围）。
// mask 非空时跳过无效体素：重采样产物里未被任何分块覆盖的点标量被写成 0.0（mask=0），
// 若计入会把正值字段的 min 拉成 0.0，污染颜色映射范围（对标 PVR_REF vtkProbeFilter
// 用 NaN 无效点、min/max 比较自动跳过无效点的做法）。
bool ComputeLocalScalarRange(iGame::StructuredMesh* mesh,
                             iGame::UnsignedCharArray* mask, double& mn,
                             double& mx) {
    bool isCell = false;
    auto arr = PickScalarField(mesh, isCell);
    if (!arr) { return false; }

    const IGsize n = arr->GetNumberOfElements();
    if (n <= 0) { return false; }

    const unsigned char* maskData = mask ? mask->RawPointer() : nullptr;
    const IGsize maskCount = mask ? mask->GetNumberOfElements() : 0;

    bool first = true;
    for (IGsize i = 0; i < n; ++i) {
        if (maskData && i < maskCount && maskData[i] == 0) { continue; }
        const double v = FieldMagnitude(arr.get(), i);
        if (first) {
            mn = mx = v;
            first = false;
        } else {
            mn = std::min(mn, v);
            mx = std::max(mx, v);
        }
    }
    if (first) { return false; }
    if (mx <= mn) { mx = mn + 1.0; }
    return true;
}

// 垂直翻转 RGBA8（GL 左下角原点 -> PNG 自上而下）。
void FlipRGBAVertically(std::vector<unsigned char>& img, int width, int height) {
    const int rowBytes = width * 4;
    std::vector<unsigned char> tmp(static_cast<size_t>(rowBytes));
    for (int row = 0; row < height / 2; ++row) {
        unsigned char* a = img.data() + static_cast<size_t>(row) * rowBytes;
        unsigned char* b = img.data() +
                           static_cast<size_t>(height - 1 - row) * rowBytes;
        std::memcpy(tmp.data(), a, static_cast<size_t>(rowBytes));
        std::memcpy(a, b, static_cast<size_t>(rowBytes));
        std::memcpy(b, tmp.data(), static_cast<size_t>(rowBytes));
    }
}

// ---------------------------------------------------------------------------
// 命令行参数解析（命名参数）
// ---------------------------------------------------------------------------

struct CliOptions {
    std::string input;
    std::string field; // 非交互指定渲染字段（--field，跳过 stdin 交互，供 srun 批处理）
    int timestep{0};
    int resPerChunk{64};
    bool useGPU{false}; // false = CPU 后端（默认）
    bool interactive{false}; // true = 交互窗口（阶段 5，仅 CPU 后端有效）
    bool server{false}; // true = C/S 服务端（阶段 6，仅 CPU 后端有效）
    bool useBinarySwap{false}; // true = binary-swap 合成（对标 IceT icetBSwapCompose）
    // 合成的深度口径（--blockwise 默认 / --pixelwise）：
    //   false = blockwise：每 rank 一个「超块中心深度」，rank0 按该全局块序整幅 over
    //   true  = pixelwise：每个像素用各自的「首命中深度」排序 over
    //                       （与 --direct / --binary-swap 正交组合；实现在
    //                        ParallelVolumePixelComposite.h，不动 iGameCore）
    bool usePixelwise{false};
    int port{11111};    // --server 监听端口
    // 两档 LOD（仅 --server / --interactive）：拖动中用低清分辨率 + 更大步长。
    // 步长以「全局体素尺寸」为单位（1.0 = 一个体素），必须全局一致，否则块间密度不均。
    double hqStepScale{1.5}; // 松手（高清）每步跨越多少个体素
    double lqStepScale{4.0}; // 拖动（低清）每步跨越多少个体素
    int lqDivisor{2};   // 拖动时的分辨率除数（2 => 512x512）
    // A 键自动播放的换帧间隔（毫秒，仅 --server）：默认 2000 = 2 秒一帧。
    int autoPlayIntervalMs{2000};
    // 传输函数「标量最小值处」的不透明度（底色雾 floor，单位为「每单位距离」= 每体素）。
    //  0（默认）  ：最小值区域完全透明（历史行为）。取景居中由「可见包围盒」（见下）解决，
    //              不需要靠抬高最小值不透明度让整块数据可见 —— 那会多出一层蓝底、并把
    //              ROI 撑到整屏。
    //  <0        ：自动 —— 按「整个包围盒对角线上的累计雾 = 10%」反推（想看整个域时用）；
    //  >0        ：直接用这个值（每体素不透明度，注意它会沿光线累积，见 SetMinOpacity 注释）。
    double minOpacity{0.0};
    bool showHelp{false};
    bool valid{false};
};

void PrintUsage(const char* prog) {
    // 说明：帮助文本一律用 ASCII 英文——控制台代码页不是 UTF-8 时，中文会变成乱码。
    std::cout
            << "Usage: " << prog << " -i <input> [options]\n"
            << "\n"
            << "Parallel volume rendering entry (stage 3 GPU validation / stage 4 CPU\n"
            << "production backend).\n"
            << "\n"
            << "Required:\n"
            << "  -i, --input <file>       Input data: multi-piece .pvd/.vtm/.igcm, or a\n"
            << "                           single .vtr/.vts/.vtu file.\n"
            << "  -f, --field <name>       Field to render (point/cell scalar or vector\n"
            << "                           name). Skips the interactive field prompt on\n"
            << "                           rank 0; required when there is no stdin (srun).\n"
            << "\n"
            << "Options:\n"
            << "  -t, --timestep <n>       PVD timestep (default 0; ignored for non-PVD\n"
            << "                           input). For multi-frame data this is the start\n"
            << "                           frame: matched against the timestep values that\n"
            << "                           actually appear in the file, falling back to the\n"
            << "                           first frame if not found.\n"
            << "  -r, --resample <n>       Resample resolution per piece (default 64, min 2).\n"
            << "      --gpu                GPU ray-casting backend (stage 3 validation;\n"
            << "                           needs OpenGL/GLFW, each rank renders offscreen\n"
            << "                           into a hidden window).\n"
            << "      --cpu                CPU ray-stepping backend (stage 4 production;\n"
            << "                           headless, no OpenGL/GLFW; default).\n"
            << "      --interactive        Interactive window (stage 5, CPU backend only):\n"
            << "                           rank 0 opens a window showing the composited\n"
            << "                           image; left-drag rotates, wheel zooms, colorbar\n"
            << "                           in the lower left, fps shown while dragging.\n"
            << "      --server             Client/server mode (stage 6, CPU backend only):\n"
            << "                           rank 0 listens on a TCP port, receives incremental\n"
            << "                           interaction commands, renders and streams the\n"
            << "                           composited image (cf. MiniPVServer).\n"
            << "      --port <n>           --server listen port (default 11111).\n"
            << "      --direct             Sparse-ROI compositing (default): each rank\n"
            << "                           gathers the bounding box of its non-empty pixels\n"
            << "                           to rank 0, O(sum of ROI areas).\n"
            << "      --binary-swap        binary-swap compositing (cf. IceT\n"
            << "                           icetBSwapCompose): ceil(log2 P) rounds of pairwise\n"
            << "                           half-ROI exchange plus ordered over, work spread\n"
            << "                           over all ranks and no rank-0 hotspot; meant for\n"
            << "                           thousands of ranks. Prefer --direct at ~1k ranks:\n"
            << "                           binary-swap is a globally synchronized blocking\n"
            << "                           P2P wave and can stall interactive frames. If given\n"
            << "                           together with --direct, the last one wins.\n"
            << "      --blockwise          Depth criterion for compositing (default): one\n"
            << "                           depth per rank (that of the super-block center),\n"
            << "                           and rank 0 blends whole images in that global\n"
            << "                           block order.\n"
            << "      --pixelwise          Depth criterion for compositing: each pixel is\n"
            << "                           ordered by its own first-hit depth (the reversed-z\n"
            << "                           value written back by the ray caster), so overlap\n"
            << "                           ordering is correct per pixel. Cost: the payload\n"
            << "                           carries an extra depth plane and rank 0 must\n"
            << "                           depth-sort per pixel. Combines with both --direct\n"
            << "                           and --binary-swap; if given together with\n"
            << "                           --blockwise, the last one wins.\n"
            << "      --hq-step <f>        Voxels per step for the high-quality (mouse-up)\n"
            << "                           setting (default 1.5). The step is in units of the\n"
            << "                           global voxel size and must match on all ranks.\n"
            << "      --lq-step <f>        Voxels per step for the low-quality (dragging)\n"
            << "                           setting (default 4.0).\n"
            << "      --lq-div <n>         Resolution divisor while dragging (default 2,\n"
            << "                           i.e. 1024 -> 512).\n"
            << "      --autoplay-interval <ms>\n"
            << "                           A-key autoplay frame interval in milliseconds\n"
            << "                           (default 2000 = 2 s per frame; --server only).\n"
            << "      --min-opacity <f>    Opacity of the transfer function at the scalar\n"
            << "                           MINIMUM, per unit distance (one resampled voxel).\n"
            << "                           Default 0 = the minimum region is fully transparent\n"
            << "                           (historical behavior); frame centering is handled by\n"
            << "                           the *visible* bbox, not by this knob). -1 = auto:\n"
            << "                           derive it so the accumulated haze over the volume\n"
            << "                           diagonal is ~10% (use this if you want to SEE the\n"
            << "                           whole bounding box). NOTE: opacity accumulates along\n"
            << "                           the ray (1-exp(-f*L)), so a \"small looking\" 0.01\n"
            << "                           over a 3000-voxel data set is already opaque.\n"
            << "  -h, --help               Show this help.\n"
            << "\n"
            << "Examples:\n"
            << "  mpiexec -n 4 " << prog
            << " -i data.pvd -t 0 --resample 64 --cpu\n"
            << "  mpiexec -n 4 " << prog << " -i data.pvd --gpu\n"
            << "  " << prog << " -i data.vts --cpu\n"
            << "  mpiexec -n 4 " << prog
            << " -i data.pvd --resample 64 --cpu --interactive\n"
            << "  mpiexec -n 4 " << prog
            << " -i data.pvd --resample 64 --cpu --server --port 11111\n"
            << "  mpiexec -n 4 " << prog
            << " -i data.pvd --resample 64 --cpu --server --pixelwise\n"
            << std::flush;
}

CliOptions ParseCli(int argc, char** argv) {
    CliOptions opts;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];

        if (a == "-h" || a == "--help") {
            opts.showHelp = true;
            continue;
        }
        if (a == "-i" || a == "--input") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts; // valid = false
            }
            opts.input = argv[++i];
            continue;
        }
        if (a == "-f" || a == "--field") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.field = argv[++i];
            continue;
        }
        if (a == "-t" || a == "--timestep") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.timestep = std::atoi(argv[++i]);
            continue;
        }
        if (a == "-r" || a == "--resample") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.resPerChunk = std::atoi(argv[++i]);
            continue;
        }
        if (a == "--gpu") {
            opts.useGPU = true;
            continue;
        }
        if (a == "--cpu") {
            opts.useGPU = false;
            continue;
        }
        if (a == "--interactive") {
            opts.interactive = true;
            continue;
        }
        if (a == "--server") {
            opts.server = true;
            continue;
        }
        if (a == "--binary-swap") {
            opts.useBinarySwap = true;
            continue;
        }
        if (a == "--direct") {
            opts.useBinarySwap = false;
            continue;
        }
        if (a == "--pixelwise") {
            opts.usePixelwise = true;
            continue;
        }
        if (a == "--blockwise") {
            opts.usePixelwise = false;
            continue;
        }
        if (a == "--port") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.port = std::atoi(argv[++i]);
            continue;
        }
        if (a == "--hq-step") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.hqStepScale = std::atof(argv[++i]);
            continue;
        }
        if (a == "--lq-step") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.lqStepScale = std::atof(argv[++i]);
            continue;
        }
        if (a == "--lq-div") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.lqDivisor = std::atoi(argv[++i]);
            continue;
        }
        if (a == "--autoplay-interval") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.autoPlayIntervalMs = std::atoi(argv[++i]);
            continue;
        }
        if (a == "--min-opacity") {
            if (i + 1 >= argc) {
                std::cerr << "Error: option '" << a << "' requires a value.\n";
                return opts;
            }
            opts.minOpacity = std::atof(argv[++i]);
            continue;
        }

        std::cerr << "Error: unknown option '" << a << "'. Use -h for help.\n";
        return opts; // valid = false
    }

    if (opts.resPerChunk < 2) { opts.resPerChunk = 2; }
    if (!(opts.hqStepScale > 0.0)) { opts.hqStepScale = 1.5; }
    if (!(opts.lqStepScale > 0.0)) { opts.lqStepScale = 4.0; }
    if (opts.lqDivisor < 1) { opts.lqDivisor = 1; }
    // 自动播放间隔：非正值回落默认 2s；下限 10ms，避免误配成忙等。
    if (opts.autoPlayIntervalMs <= 0) { opts.autoPlayIntervalMs = 2000; }
    if (opts.autoPlayIntervalMs < 10) { opts.autoPlayIntervalMs = 10; }
    // 最小值不透明度：<0 = 自动（按包围盒对角线上的目标雾浓度反推）；上限 0.5，
    // 免得误填一个大值把整个包围盒变成实心块。
    if (opts.minOpacity > 0.5) { opts.minOpacity = 0.5; }
    if (!opts.input.empty()) { opts.valid = true; }
    return opts;
}
} // namespace

int main(int argc, char** argv) {
    using namespace iGame;

    const auto tProgram0 = std::chrono::steady_clock::now();
    ParallelContext::Initialize(&argc, &argv);
    auto ctx = ParallelContext::Instance();
    const int rank = ctx->Rank();
    const int size = ctx->Size();

    // iGame 线程池线程数按「本进程实际可用核数」设定（默认 12 在 --cpu-bind=cores 场景
    // 是 12 倍超订）。必须在任何 parallelFor（重采样 / 光线步进 / 砖块构建）之前设置。
    const int usableCores = DetectUsableCoreCount();
    ThreadPool::SetDefaultThreadCount(usableCores);

    const CliOptions cli = ParseCli(argc, argv);

    // 帮助：rank 0 打印后所有 rank 一致退出。
    if (cli.showHelp) {
        if (rank == 0) { PrintUsage(argv[0]); }
        ParallelContext::Finalize();
        return 0;
    }

    // 参数错误：ParseCli 已打印具体错误（未知选项/缺值）；此处补打印用法并退出。
    if (!cli.valid) {
        if (rank == 0) {
            if (cli.input.empty()) {
                std::cerr << "Error: missing required option '-i/--input "
                             "<file>'.\n\n";
            }
            PrintUsage(argv[0]);
        }
        ParallelContext::Finalize();
        return 1;
    }

    const std::string input = cli.input;
    const int timestep = cli.timestep;
    const int resPerChunk = cli.resPerChunk;
    // 渲染后端：默认 --cpu（阶段 4 生产后端）；--gpu 走阶段 3 验证通路。
    const bool useCPU = !cli.useGPU;

    // 输入标识与节点名：启动横幅和加载汇总都要用（相对路径 + 绝对路径各留一份，
    // 便于在作业日志里确认「跑的到底是哪份数据」）。
    const std::string inputName =
            std::filesystem::path(input).filename().string();
    std::string inputAbs = input;
    {
        std::error_code ecAbs;
        const auto absPath = std::filesystem::weakly_canonical(input, ecAbs);
        if (!ecAbs) { inputAbs = absPath.string(); }
    }
    const std::string localNode = LocalHostName();

    // 节点映射：各 rank 上报自己的主机名，rank 0 打印「节点 -> rank 区间」。
    // 这是集合通信，所有 rank 必须调用；放在分发器之前，保证启动日志先于（可能很慢的）
    // 全量分块扫描出现。rankNodes 只在 rank 0 上有效。
    std::map<std::string, std::vector<int>> nodeMap;
    {
        constexpr int kNodeNameBytes = 64;
        std::vector<char> sendNode(kNodeNameBytes, '\0');
        std::strncpy(sendNode.data(), localNode.c_str(), kNodeNameBytes - 1);
        std::vector<char> recvNodes;
        if (rank == 0) {
            recvNodes.assign(static_cast<size_t>(size) * kNodeNameBytes, '\0');
        }
        ctx->Gather(sendNode.data(), rank == 0 ? recvNodes.data() : nullptr,
                    kNodeNameBytes, 0);
        if (rank == 0) {
            for (int r = 0; r < size; ++r) {
                const char* begin =
                        recvNodes.data() + static_cast<size_t>(r) * kNodeNameBytes;
                const char* end = std::find(begin, begin + kNodeNameBytes, '\0');
                nodeMap[std::string(begin, end)].push_back(r);
            }
        }
    }

    // -------------------------------------------------------------------------
    // 启动横幅（rank 0）：程序/构建、输入文件、后端与模式、规模与核心、调度器环境。
    // 只打一次，方便在 server_%j.out 开头直接确认「这份作业跑的什么数据、开了多少进程/核」。
    // -------------------------------------------------------------------------
    if (rank == 0) {
        const std::string mode = cli.server
                                         ? (useCPU ? "server" : "server (ignored: GPU)")
                                         : (cli.interactive
                                                    ? (useCPU ? "interactive"
                                                              : "interactive (ignored: GPU)")
                                                    : "batch");
        std::cout << "[env] ===== iGameVis parallel volume rendering backend =====\n";
        std::cout << "[env] program="
                  << std::filesystem::path(argv[0]).filename().string()
                  << " built=" << __DATE__ << ' ' << __TIME__
                  << " compiler=" << CompilerName()
                  << " arch=" << TargetArchName() << '\n';
        std::cout << "[env] input=" << inputName << " path=" << inputAbs << '\n';
        std::cout << "[env] backend=" << (useCPU ? "CPU" : "GPU")
                  << " mode=" << mode << " timestep=" << timestep
                  << " resample=" << resPerChunk
                  << " composite=" << (cli.useBinarySwap ? "binary-swap" : "direct")
                  << '/' << (cli.usePixelwise ? "pixelwise" : "blockwise") << '\n';
        std::cout << "[env] ranks=" << size << " nodes=" << nodeMap.size()
                  << " cores/rank=" << usableCores
                  << " threads/rank=" << ThreadPool::GetDefaultThreadCount()
                  << " hardware_concurrency=" << std::thread::hardware_concurrency()
                  << " OMP_NUM_THREADS=" << EnvOrDash("OMP_NUM_THREADS") << '\n';
        std::cout << "[env] scheduler: job=" << EnvOrDash("SLURM_JOB_ID")
                  << " slurm_nodes=" << EnvOrDash("SLURM_NNODES")
                  << " slurm_ntasks=" << EnvOrDash("SLURM_NTASKS")
                  << " slurm_cpus/task=" << EnvOrDash("SLURM_CPUS_PER_TASK")
                  << " ompi_size=" << EnvOrDash("OMPI_COMM_WORLD_SIZE")
                  << " ompi_local_rank=" << EnvOrDash("OMPI_COMM_WORLD_LOCAL_RANK")
                  << " pmi_size=" << EnvOrDash("PMI_SIZE") << '\n';
        std::cout << "[env] node of rank 0: " << localNode
                  << " pid=" << CurrentProcessId() << '\n';
        std::cout.flush();
        PrintNodeMap(nodeMap, size);
    }

    // 程序结束提示（rank 0）：--server 是持久服务，正常运行时只会因外部终止而退出，
    // 所以正常路径上「最后一行」由加载汇总（见下）承担；这里补的是会话/批处理结束的时刻。
    auto PrintRunEnd = [&](const char* reason) {
        if (rank != 0) { return; }
        std::cout << "[done] " << reason << " input=" << inputName
                  << " wall=" << FormatSeconds(SecSince(tProgram0,
                                                        std::chrono::steady_clock::now()))
                  << '\n';
        std::cout.flush();
    };

    // 1. 文件级分发（阶段 7，对齐 UnifiedVersion/DataDistribution::ComputeDistribution）：
    //    解析 PVD/目录/单文件 → 各 rank 只扫 part%Size==rank 的分块包围盒 → AllReduce
    //    汇总 → 确定性排序 + 连续切块。每个 rank 只知道自己要读哪些分块文件，**不整读全量**。
    const auto tDist0 = std::chrono::steady_clock::now();
    auto distributor = iGameVolumeDistributor::New();
    if (!distributor->ComputeFileDistribution(input, timestep)) {
        ParallelContext::Finalize();
        return 1;
    }
    const auto tDist1 = std::chrono::steady_clock::now();
    const int nLocalFiles = distributor->GetNumberOfLocalPieceFiles();

    // 打印本 rank 分配到的分块（对标 TestPVolumeRender 的 rank/block/pieces 输出，
    // 确认每个 rank 只读自己那部分分块）。带上节点名与 pid：这就是「这个分块在哪个
    // node 上加载」的直接答案；分块清单本身按 part(文件名) 列出。
    {
        const auto blk = distributor->GetLocalBlock();
        std::ostringstream oss;
        oss << "[load] rank " << rank << " node=" << localNode
            << " pid=" << CurrentProcessId() << " block=(" << blk.ix0 << ".."
            << blk.ix1 << ", " << blk.iy0 << ".." << blk.iy1 << ", " << blk.iz0
            << ".." << blk.iz1 << ") pieces=" << nLocalFiles << ": [";
        for (int i = 0; i < nLocalFiles; ++i) {
            if (i) { oss << ", "; }
            oss << distributor->GetLocalPiecePart(i) << "("
                << std::filesystem::path(distributor->GetLocalPieceFile(i))
                           .filename().string()
                << ")";
        }
        oss << "]\n";
        std::cout << oss.str() << std::flush;
    }

    // 2. 字段选择：优先用 --field 直接指定（srun 批处理无 stdin）；否则 rank0 读自己
    //    第 0 个分块列出可渲染字段并等待输入，随后广播给所有 rank。
    const auto tField0 = std::chrono::steady_clock::now();
    char fieldBuf[1024] = {0};
    if (!cli.field.empty()) {
        std::strncpy(fieldBuf, cli.field.c_str(), sizeof(fieldBuf) - 1);
    } else if (rank == 0) {
        std::vector<std::string> names;
        std::vector<int> comps;
        std::vector<bool> isCell;
        auto probe = FileIO::ReadFile(distributor->GetLocalPieceFile(0));
        if (probe) {
            auto* attrs = probe->GetAttributeSet();
            if (attrs) {
                for (int i = 0; i < static_cast<int>(attrs->GetNumberOfAttributes()); ++i) {
                    auto& a = attrs->GetAttribute(i);
                    if (a.IsNone() || !a.pointer) { continue; }
                    if (a.type != IG_SCALAR && a.type != IG_VECTOR) { continue; }
                    names.push_back(a.pointer->GetName());
                    comps.push_back(a.pointer->GetDimension());
                    isCell.push_back(a.attachmentType == IG_CELL);
                }
            }
        }

        if (names.empty()) {
            std::cerr << "No scalar/vector field found in the data.\n";
        } else {
            for (std::size_t i = 0; i < names.size(); ++i) {
                std::cout << "  [" << i << "] " << names[i] << "  ("
                          << comps[i] << " component(s)"
                          << (isCell[i] ? ", cell)" : ")") << '\n';
            }

            bool ok = false;
            while (!ok) {
                std::cout << "Enter the field to visualize (name or index, Enter to cancel): ";
                std::cout.flush();
                std::string line;
                if (!std::getline(std::cin, line)) { break; }
                const std::size_t s = line.find_first_not_of(" \t\r\n");
                if (s == std::string::npos) { break; }
                const std::size_t e = line.find_last_not_of(" \t\r\n");
                line = line.substr(s, e - s + 1);

                const bool isNumber = !line.empty() &&
                    std::all_of(line.begin(), line.end(),
                                [](unsigned char c) { return std::isdigit(c); });
                if (isNumber) {
                    const int idx = std::atoi(line.c_str());
                    if (idx >= 0 && idx < static_cast<int>(names.size())) {
                        std::strncpy(fieldBuf, names[idx].c_str(), sizeof(fieldBuf) - 1);
                        ok = true;
                    }
                } else {
                    for (const auto& n : names) {
                        if (n == line) {
                            std::strncpy(fieldBuf, n.c_str(), sizeof(fieldBuf) - 1);
                            ok = true;
                            break;
                        }
                    }
                }
                if (!ok) { std::cout << "Invalid field selection, please try again.\n"; }
            }
        }
    }
    ParallelContext::Instance()->Broadcast(fieldBuf, static_cast<int>(sizeof(fieldBuf)), 0);
    if (fieldBuf[0] == '\0') {
        if (rank == 0) { std::cout << "Cancelled, exiting.\n"; }
        ParallelContext::Finalize();
        return 0;
    }
    const std::string selectedField(fieldBuf);
    const auto tField1 = std::chrono::steady_clock::now();

    // 3. 多帧枚举：PVD 会解析出所有时间步（每帧分块数一致、相同 part 空间位置不变）；
    //    目录/单文件只有 1 帧。时间步按文件里实际出现的值记录（不一定从 0 开始），
    //    startFrame 由 -t/--timestep 匹配；匹配不到则回退到 PVD 里的第 1 帧。
    const int numFrames = distributor->GetNumberOfTimesteps();
    int startFrame = distributor->GetFrameIndexForTimestep(timestep);
    if (startFrame < 0) {
        startFrame = 0;
        if (rank == 0) {
            std::cerr << "[frames] timestep " << timestep
                      << " not found in pvd; falling back to the first frame.\n";
        }
    }
    if (rank == 0 && numFrames > 1) {
        std::cerr << "[frames] " << numFrames << " timesteps; start frame = "
                  << startFrame << " (timestep "
                  << distributor->GetTimestep(startFrame) << ")\n";
    }

    // 每个 rank 只读自己分到的分块（对标 DataDistribution：rank 只读 localFiles），
    // 合并成本地 composite 后重采样。多帧时每个 rank 读 numFrames 倍的分块（相同 part、
    // 不同 timestep），重采样产物按帧缓存，切帧只换数据指针——渲染/合成路径不变，帧率不变。
    // 仅当「整个数据集是单块」且该单块是 StructuredMesh 时直接复用（跳过重采样）；
    // 否则（多块，或单块但不是结构化网格）一律重采样。
    std::vector<StructuredMesh::Pointer> volumes(static_cast<size_t>(numFrames));
    std::vector<UnsignedCharArray::Pointer> masks(static_cast<size_t>(numFrames));
    // 只保留**起始帧**的重采样器（后面用它的 ComputeVisibleBounds 圈「看得见」的体素范围）。
    //
    // ⚠️ 不能把每帧的重采样器都留着：Filter 自己持有 m_Input（= 该帧读进来的全部分块），
    // 留着就意味着"每帧的原始分块数据全部常驻内存"（旧代码在每个帧循环结束时析构，
    // 内存随帧数线性增长；1000 rank 上足以把节点内存打满 → rank 被 OOM kill → 整个作业
    // 挂住，表现为"画面卡住 + 客户端再也连不上"）。用完还会把它置空。
    iGameVolumeResampleFilter::Pointer anchorResampler;

    // 重采样目标分辨率（只算一次：各帧空间位置一致，网格完全相同）。
    const auto blk = distributor->GetLocalBlock();
    int bcx = blk.ix1 - blk.ix0 + 1;
    int bcy = blk.iy1 - blk.iy0 + 1;
    int bcz = blk.iz1 - blk.iz0 + 1;
    if (bcx < 1) { bcx = 1; }
    if (bcy < 1) { bcy = 1; }
    if (bcz < 1) { bcz = 1; }
    int tni = resPerChunk * bcx;
    int tnj = resPerChunk * bcy;
    int tnk = resPerChunk * bcz;
    // 护栏：体素总数上限 512^3，避免单 rank 分到过多 chunk 时 OOM。
    const long long kMaxVoxels = 512LL * 512 * 512;
    {
        const long long vox = static_cast<long long>(tni) * tnj * tnk;
        if (vox > kMaxVoxels) {
            const double s = std::cbrt(static_cast<double>(kMaxVoxels) / static_cast<double>(vox));
            tni = std::max(2, static_cast<int>(static_cast<double>(tni) * s));
            tnj = std::max(2, static_cast<int>(static_cast<double>(tnj) * s));
            tnk = std::max(2, static_cast<int>(static_cast<double>(tnk) * s));
            if (rank == 0) {
                std::cerr << "[resample] target clamped to " << tni << 'x' << tnj
                          << 'x' << tnk << " (voxel cap " << kMaxVoxels << ").\n";
            }
        }
    }

    // 加载统计（每个 rank 本地累计；全局汇总见下面的 [load-summary]）。
    //   readSec / resampleSec      本 rank 的实际耗时（用于定位 IO 还是重采样慢）
    //   startFrameDiskBytes        起始帧本 rank 分块在磁盘上的字节数（= 模型大小口径之一）
    //   allFramesDiskBytes         所有帧实际读过的分块字节数（多帧数据的总 IO 量）
    //   startFrameSourceBytes      起始帧源网格常驻内存（GetRealMemorySize 汇总分块）
    double readSec = 0.0;
    double resampleSec = 0.0;
    double startFrameDiskBytes = 0.0;
    double allFramesDiskBytes = 0.0;
    double startFrameSourceBytes = 0.0;

    const auto tLoad0 = std::chrono::steady_clock::now();
    double sourceMinSpacing = 0.0;
    for (int f = 0; f < numFrames; ++f) {
        auto composite = DataObject::New();
        const auto tRead0 = std::chrono::steady_clock::now();
        double frameDiskBytes = 0.0;
        for (int i = 0; i < nLocalFiles; ++i) {
            const std::string& piecePath = distributor->GetLocalPieceFile(i, f);
            // 文件大小（磁盘口径）与 ReadFile 的解析开销分开统计；file_size 只读元数据。
            std::error_code sizeEc;
            const auto fileBytes = std::filesystem::file_size(piecePath, sizeEc);
            if (!sizeEc) { frameDiskBytes += static_cast<double>(fileBytes); }
            auto piece = FileIO::ReadFile(piecePath);
            if (!piece) {
                std::cerr << "[rank " << rank << "] failed to read " << piecePath
                          << '\n';
                ParallelContext::Finalize();
                return 1;
            }
            composite->AddSubDataObject(piece);
        }
        const auto tRead1 = std::chrono::steady_clock::now();
        const double frameReadMs = MsSince(tRead0, tRead1);
        readSec += frameReadMs / 1000.0;
        allFramesDiskBytes += frameDiskBytes;
        if (f == startFrame) {
            startFrameDiskBytes = frameDiskBytes;
            // 注意：非起始帧的 composite 在本轮结束时随重采样器一起释放（见上面的 OOM
            // 注释），所以「源数据常驻内存」只统计起始帧，才是服务进程真实占用。
            startFrameSourceBytes =
                    static_cast<double>(composite->GetRealMemorySize());
        }

        double frameResampleMs = 0.0;
        StructuredMesh::Pointer direct = nullptr;
        if (distributor->GetTotalPieceCount() == 1) {
            auto it = composite->SubDataObjectIteratorBegin();
            if (it != composite->SubDataObjectIteratorEnd()) {
                direct = DynamicCast<StructuredMesh>(it->second);
                if (direct && !selectedField.empty()) {
                    const int idx =
                            direct->GetAttributeSet()->GetAttributeIndex(selectedField);
                    if (idx >= 0) { direct->SetAttributeIndex(idx); }
                }
            }
        }
        if (direct) {
            volumes[static_cast<size_t>(f)] = direct;
        } else {
            const auto tResample0 = std::chrono::steady_clock::now();
            auto resampler = iGameVolumeResampleFilter::New();
            resampler->SetInput(composite);
            resampler->SetFieldName(selectedField);
            resampler->SetTargetDims(tni, tnj, tnk);
            // 多帧共享几何：各帧空间位置一致，复用第 0 帧的点坐标，只重算标量 + mask。
            if (f > 0 && volumes[0]) {
                resampler->SetGeometryTemplate(volumes[0]);
            }
            if (!resampler->Execute()) {
                if (rank == 0) { std::cerr << "Resample failed.\n"; }
                ParallelContext::Finalize();
                return 1;
            }
            const auto tResample1 = std::chrono::steady_clock::now();
            frameResampleMs = MsSince(tResample0, tResample1);
            resampleSec += frameResampleMs / 1000.0;
            volumes[static_cast<size_t>(f)] = resampler->GetStructuredMesh();
            masks[static_cast<size_t>(f)] = resampler->GetValidMask();
            if (f == startFrame) { anchorResampler = resampler; }
            if (f == startFrame) {
                // 诊断只打一次（起始帧），避免多帧刷屏。
                sourceMinSpacing = resampler->GetSourceMinSpacing();
                if (rank == 0) {
                    const double outSp = resampler->GetOutputMinSpacing();
                    std::cerr << "[resample] block=(" << bcx << 'x' << bcy << 'x' << bcz
                              << ") target=" << tni << 'x' << tnj << 'x' << tnk
                              << " outSpacing(min)=" << outSp
                              << " sourceSpacing(min)=" << sourceMinSpacing << '\n';
                    if (sourceMinSpacing > 0.0 && outSp > sourceMinSpacing * 1.05) {
                        std::cerr << "[resample] WARNING: undersampling "
                                  << (outSp / sourceMinSpacing)
                                  << "x (output spacing coarser than source). Raise "
                                     "--resample to >= "
                                  << static_cast<int>(std::ceil(
                                             resPerChunk * outSp / sourceMinSpacing))
                                  << " to keep the source detail (otherwise fine "
                                     "texture aliases into blocky/choppy bands).\n";
                    }
                }
            }
        }

        // 多帧加载进度（rank 0）：长作业里能确认还在推进，以及每帧 IO / 重采样各占多少。
        // 单帧数据不打印（下面每个 rank 的 [load] 行已足够）。
        if (rank == 0 && numFrames > 1) {
            std::cout << "[load] frame " << (f + 1) << '/' << numFrames
                      << " timestep=" << distributor->GetTimestep(f)
                      << " rank0-pieces=" << nLocalFiles
                      << " disk=" << FormatBytes(frameDiskBytes)
                      << " read=" << FormatMs(frameReadMs)
                      << " resample=" << FormatMs(frameResampleMs) << '\n';
            std::cout.flush();
        }
    }
    const auto tLoad1 = std::chrono::steady_clock::now();

    // 7. 全局标量范围：各 rank 局部 min/max -> AllReduce -> 全局一致（对标
    //    VolumeRenderingCommon.h:44-50 的 AllReduce(MIN/MAX)）。
    // 注意：无论本 rank 是否有有效体素都参与 AllReduce，避免某 rank 全无效体素时
    // hasRange=false 导致该 rank 跳过 AllReduce、与其它 rank 集合通信失配（死锁）。
    // 无有效体素时用 ±inf 作"空贡献"，MIN/MAX 归约不影响其它 rank。
    // 全局标量范围取「所有帧」的并集（多帧播放时颜色映射跨帧稳定、colorbar 只需发一次）。
    double localMin = 0.0, localMax = 1.0;
    bool hasLocalRange = false;
    for (int f = 0; f < numFrames; ++f) {
        double mn = 0.0, mx = 1.0;
        if (ComputeLocalScalarRange(volumes[static_cast<size_t>(f)].get(),
                                    masks[static_cast<size_t>(f)].get(), mn, mx)) {
            if (!hasLocalRange) {
                localMin = mn;
                localMax = mx;
                hasLocalRange = true;
            } else {
                localMin = std::min(localMin, mn);
                localMax = std::max(localMax, mx);
            }
        }
    }
    double reduceMin = hasLocalRange
                               ? localMin
                               : std::numeric_limits<double>::infinity();
    double reduceMax = hasLocalRange
                               ? localMax
                               : -std::numeric_limits<double>::infinity();
    double globalMin = 0.0, globalMax = 1.0;
    ctx->AllReduce(&reduceMin, &globalMin, 1, ParallelContext::ReduceOp::Min);
    ctx->AllReduce(&reduceMax, &globalMax, 1, ParallelContext::ReduceOp::Max);
    // 全员都无有效体素时 globalMin=+inf > globalMax=-inf，视为无范围，回退默认。
    const bool hasRange = globalMin < globalMax;
    if (!hasRange) {
        globalMin = 0.0;
        globalMax = 1.0;
    }
    // 诊断：rank0 打印选中字段与全局标量范围，便于对比本地(6块)/超算(19200块)。
    // 若超算上 global 范围被 0.0 污染（min 起跳为 0）或跨度异常，即定位到范围问题。
    if (rank == 0) {
        std::cerr << "[range] field='" << selectedField
                  << "' hasRange=" << hasRange << " global=[" << globalMin
                  << ", " << globalMax << "]\n";
    }

    // 8. 全局包围盒：各 rank 局部超块包围盒 -> AllGather -> 求并集。
    const BoundingBox& localBounds = distributor->GetLocalBlockBounds();
    double localBox[6] = {localBounds.min[0], localBounds.max[0],
                          localBounds.min[1], localBounds.max[1],
                          localBounds.min[2], localBounds.max[2]};
    std::vector<double> allBoxes(static_cast<size_t>(size) * 6);
    ctx->AllGather(localBox, allBoxes.data(), 6);

    BoundingBox globalBounds;
    globalBounds.reset();
    for (int r = 0; r < size; ++r) {
        const double mn[3] = {allBoxes[static_cast<size_t>(r) * 6 + 0],
                              allBoxes[static_cast<size_t>(r) * 6 + 2],
                              allBoxes[static_cast<size_t>(r) * 6 + 4]};
        const double mx[3] = {allBoxes[static_cast<size_t>(r) * 6 + 1],
                              allBoxes[static_cast<size_t>(r) * 6 + 3],
                              allBoxes[static_cast<size_t>(r) * 6 + 5]};
        globalBounds.add(BoundingBox(mn, mx));
    }

    // 8.5 全局体素尺寸（所有 rank 一致）——并行体绘制正确性的关键量。
    //   Beer-Lambert 的 ScalarOpacityUnitDistance 与「全局统一光线步长」都取它：
    //     alphaStep = 1 - exp(-opacity * step / unitDistance)
    //   只有把 unitDistance 设成真实体素尺寸，累计光学厚度才与步长、与「每个 rank 分到
    //   多大超块」无关。否则（unitDistance=0，本工程此前的默认）
    //     alphaStep = opacity  ——累计不透明度 ∝ 采样步数，而自适应步长下每个 rank 的步数
    //   都恰好等于 maxSamples，于是「薄超块」和「厚超块」贡献同样的不透明度：前面的 rank
    //   一饱和就把内部结构全遮住，屏幕上表现为明显的块状明暗台阶（对标参考实现
    //   MiniPVServer.cpp:586-600 / TestPVolumeRender.cpp:504-520 的
    //   SetScalarOpacityUnitDistance(cbrt(spx*spy*spz))）。
    //   取 AllReduce(MIN) 是为了让所有 rank 用同一个值（各 rank 重采样网格间距略有差异）。
    double localVoxel = std::numeric_limits<double>::infinity();
    // 各帧空间位置一致，体素尺寸相同；取起始帧即可。
    if (volumes[static_cast<size_t>(startFrame)]) {
        const BoundingBox& vb = volumes[static_cast<size_t>(startFrame)]->GetBoundingBox();
        igIndex* vd = volumes[static_cast<size_t>(startFrame)]->GetDimensionSize();
        double sp[3];
        bool ok = true;
        for (int a = 0; a < 3; ++a) {
            sp[a] = (vd[a] > 1)
                            ? (vb.max[a] - vb.min[a]) /
                                      static_cast<double>(vd[a] - 1)
                            : 0.0;
            if (!(sp[a] > 0.0)) { ok = false; }
        }
        if (ok) { localVoxel = std::cbrt(sp[0] * sp[1] * sp[2]); }
    }
    double voxelSize = 1.0;
    ctx->AllReduce(&localVoxel, &voxelSize, 1, ParallelContext::ReduceOp::Min);
    if (!std::isfinite(voxelSize) || voxelSize <= 0.0) { voxelSize = 1.0; }
    if (rank == 0) {
        std::cerr << "[voxel] global voxel size (unit distance) = " << voxelSize
                  << "; HQ step = " << voxelSize * cli.hqStepScale
                  << ", LQ step = " << voxelSize * cli.lqStepScale << '\n';
    }

    // 8.6 传输函数在「标量最小值」处的不透明度（底色雾 floor，默认 0 = 关闭）。
    //
    // 不透明度在这里是**每单位距离**的量（Beer-Lambert，unitDistance = 全局体素尺寸），
    // 沿光线会累积成
    //     A = 1 - exp(-floor · L),  L = 路径上有多少个重采样体素
    // 所以：
    //   · floor = 0（默认）⇒ 整个"最小值平台"（本数据集的 101325 环境压力区）完全不可见。
    //     想让「看不见的包围盒」变成看得见，本该改相机而不该改渲染 —— 已由「可见包围盒」
    //     取景解决（见 --server 分支的 [bounds] 日志），画面上不会多出一层蓝底，
    //     ROI / 下行载荷也不会被撑大。
    //   · 凭直觉给 0.01 ⇒ 本数据集对角线约 3000+ 个体素，1-exp(-0.01×3000) ≈ 100%，
    //     整个包围盒变成实心块。
    //   · 真想看一眼整个域时用 --min-opacity -1（自动）：按「对角线累计雾 ≈10%」反推
    //         floor = -ln(1 - 0.10) / L_diag
    //     这个值随数据尺度/--resample 自动变化，不会因为体素变细而糊掉。
    //
    // 参考：VTK 的 vtkVolumeProperty::SetScalarOpacityUnitDistance 就是把"不透明度"
    // 定义在单位距离上、并由 mapper 做 opacity correction（Max 1995 / Roettger 2003）；
    // 参考实现 UnifiedVersion/src/VolumeRenderingCommon.h 用的是 (gMin, 0.0) 起步，
    // 即本参数的默认值 0。
    constexpr double kMinOpacityHazeTarget = 0.10;
    // 可见体素包围盒（相机取景用）的隔点采样步长：2 = 每轴隔 1 个取 1，只查 1/8 的体素。
    // 可见区是连片的大块，隔点采样的包围盒误差 ≤ 1 个体素（代码里再向外扩 stride 个格子，
    // 保证是"略大"的安全包围盒），对 1500~3000 体素量级的取景完全看不出来。
    constexpr int kVisibleBoundsStride = 2;
    auto ResolveMinOpacity = [&]() -> double {
        if (cli.minOpacity >= 0.0) { return cli.minOpacity; }
        const double diag = globalBounds.diag();
        if (!(diag > 0.0) || !(voxelSize > 0.0)) { return 0.0; }
        const double diagVox = diag / voxelSize; // 对角线有多少个重采样体素
        if (!(diagVox > 1.0)) { return 0.0; }
        return -std::log(1.0 - kMinOpacityHazeTarget) / diagVox;
    };
    const double minOpacityValue = ResolveMinOpacity();
    if (rank == 0) {
        const double diag = globalBounds.diag();
        // 三个主轴方向的路径长度（体素数），用于说明"不同视角看到的雾浓淡不同"。
        double axisVox[3] = {0.0, 0.0, 0.0};
        for (int a = 0; a < 3; ++a) {
            axisVox[a] = (globalBounds.max[a] - globalBounds.min[a]) / voxelSize;
        }
        const double diagVox = (voxelSize > 0.0) ? diag / voxelSize : 0.0;
        auto hazeAt = [&](double lVox) {
            return (minOpacityValue > 0.0)
                           ? 100.0 * (1.0 - std::exp(-minOpacityValue * lVox))
                           : 0.0;
        };
        std::cerr << "[tf] min opacity = " << minOpacityValue
                  << " per voxel ("
                  << (cli.minOpacity >= 0.0 ? "disabled/default" : "auto")
                  << "); accumulated haze over the bbox diagonal ("
                  << static_cast<long long>(diagVox) << " vox) = "
                  << hazeAt(diagVox) << "%, over the shortest axis ("
                  << static_cast<long long>(std::max(0.0, std::min({axisVox[0], axisVox[1], axisVox[2]})))
                  << " vox) = "
                  << hazeAt(std::min({axisVox[0], axisVox[1], axisVox[2]}))
                  << "%. 0 disables it (--min-opacity 0).\n";
    }

    // -------------------------------------------------------------------------
    // 加载完成汇总（rank 0）：输入文件、节点/核心、模型规模、内存、耗时。
    //
    // 为什么放在这里（加载 + 全局范围 + 包围盒 + 传输函数都算完，进入渲染 / C/S 服务
    // 循环之前）：
    //   · --server 是持久服务，正常运行时永远走不到 main 末尾，作业日志里能稳定出现的
    //     「最后一段」就是这里；
    //   · 所有 rank 都必须执行下面的 AllReduce / AllGather（集合通信次数一致，否则
    //     死锁），因此这些调用不能包在 if (rank == 0) 里。
    // -------------------------------------------------------------------------
    const bool usedResample = (anchorResampler != nullptr);
    double volumeBytes = 0.0;  // 所有缓存帧的体数据（+ mask）常驻内存
    double totalVoxels = 0.0;  // 起始帧体素总数（所有 rank 之和，代表单帧分辨率）
    double validVoxels = 0.0;  // 起始帧中有效体素（mask != 0）
    // 多帧播放会缓存每一帧的体数据（volumes/masks 各帧都常驻），内存必须按帧累加；
    // 体素数/有效率只统计起始帧，用来说明单帧分辨率与分块覆盖情况。
    for (int f = 0; f < numFrames; ++f) {
        if (volumes[static_cast<size_t>(f)]) {
            volumeBytes += static_cast<double>(
                    volumes[static_cast<size_t>(f)]->GetRealMemorySize());
        }
        if (masks[static_cast<size_t>(f)]) {
            volumeBytes += static_cast<double>(
                    masks[static_cast<size_t>(f)]->GetRealMemorySize());
        }
    }
    if (volumes[static_cast<size_t>(startFrame)]) {
        const igIndex* dim =
                volumes[static_cast<size_t>(startFrame)]->GetDimensionSize();
        if (dim) {
            totalVoxels = static_cast<double>(dim[0]) * static_cast<double>(dim[1]) *
                          static_cast<double>(dim[2]);
        }
    }
    if (masks[static_cast<size_t>(startFrame)]) {
        auto& mask = masks[static_cast<size_t>(startFrame)];
        const IGsize maskCount = mask->GetNumberOfElements();
        const unsigned char* maskData = mask->RawPointer();
        for (IGsize i = 0; i < maskCount; ++i) {
            if (maskData[i] != 0) { validVoxels += 1.0; }
        }
    }
    // 多帧**重采样**共享同一份点坐标（iGameVolumeResampleFilter.cpp 的 SetGeometryTemplate
    // 分支：volumes[f>0] 复用 volumes[0] 的 Points），按帧累加 GetRealMemorySize 会把
    // 这块 12 B/体素的几何重复计 numFrames-1 次，这里扣掉，得到真实常驻内存。
    // 单块直读路径每帧各自 ReadFile，点坐标不共享，不能扣（usedResample 为 false）。
    if (usedResample && numFrames > 1 && volumes[0] &&
        volumes[0]->GetPoints()) {
        volumeBytes -= static_cast<double>(
                               volumes[0]->GetPoints()->GetRealMemorySize()) *
                       static_cast<double>(numFrames - 1);
    }
    const double loadSec = readSec + resampleSec;

    // [0] 起始帧磁盘字节 [1] 所有帧磁盘字节 [2] 源常驻内存 [3] 体数据常驻内存
    // [4] 体素数 [5] 有效体素 [6] 核心数 [7] 本 rank 分块数
    // [8] 读耗时 [9] 重采样耗时
    double localSums[10] = {
            startFrameDiskBytes, allFramesDiskBytes, startFrameSourceBytes,
            volumeBytes, totalVoxels, validVoxels,
            static_cast<double>(usableCores), static_cast<double>(nLocalFiles),
            readSec, resampleSec};
    double globalSums[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ctx->AllReduce(localSums, globalSums, 10, ParallelContext::ReduceOp::Sum);

    double localPiecesMin = nLocalFiles, localPiecesMax = nLocalFiles;
    double globalPiecesMin = 0.0, globalPiecesMax = 0.0;
    ctx->AllReduce(&localPiecesMin, &globalPiecesMin, 1,
                   ParallelContext::ReduceOp::Min);
    ctx->AllReduce(&localPiecesMax, &globalPiecesMax, 1,
                   ParallelContext::ReduceOp::Max);

    double localCoresMin = usableCores, localCoresMax = usableCores;
    double globalCoresMin = 0.0, globalCoresMax = 0.0;
    ctx->AllReduce(&localCoresMin, &globalCoresMin, 1,
                   ParallelContext::ReduceOp::Min);
    ctx->AllReduce(&localCoresMax, &globalCoresMax, 1,
                   ParallelContext::ReduceOp::Max);

    // 各 rank 的总加载耗时（read + resample），用于找慢节点 / 负载不均。
    std::vector<double> allLoadSec(static_cast<size_t>(size), 0.0);
    ctx->AllGather(&loadSec, allLoadSec.data(), 1);

    const auto tSummary = std::chrono::steady_clock::now();
    if (rank == 0) {
        double minLoad = allLoadSec.empty() ? 0.0 : allLoadSec[0];
        double maxLoad = minLoad;
        double sumLoad = 0.0;
        int slowest = 0;
        for (int r = 0; r < size; ++r) {
            const double v = allLoadSec[static_cast<size_t>(r)];
            sumLoad += v;
            if (v < minLoad) { minLoad = v; }
            if (v > maxLoad) {
                maxLoad = v;
                slowest = r;
            }
        }
        const double avgLoad = (size > 0) ? sumLoad / static_cast<double>(size) : 0.0;
        std::string slowestNodeResolved = "?";
        for (const auto& kv : nodeMap) {
            for (int r : kv.second) {
                if (r == slowest) { slowestNodeResolved = kv.first; }
            }
        }
        const double validPct = (globalSums[4] > 0.0)
                                        ? 100.0 * globalSums[5] / globalSums[4]
                                        : 0.0;
        const double throughputMiBs =
                (maxLoad > 0.0) ? (globalSums[1] / (1024.0 * 1024.0)) / maxLoad : 0.0;
        const double distSec = SecSince(tDist0, tDist1);
        const double fieldSec = SecSince(tField0, tField1);
        const double loadWallSec = SecSince(tLoad0, tLoad1);
        const double prepSec = SecSince(tLoad1, tSummary);

        std::cout << "[load-summary] input=" << inputName << " path=" << inputAbs
                  << " field='" << selectedField << "' timestep=" << timestep
                  << " frames=" << numFrames
                  << " pieces=" << distributor->GetTotalPieceCount() << '\n';
        std::cout << "[load-summary] ranks=" << size << " nodes=" << nodeMap.size()
                  << " cores/rank min/avg/max=" << globalCoresMin << '/'
                  << (globalSums[6] / static_cast<double>(size)) << '/'
                  << globalCoresMax << " total-cores="
                  << static_cast<long long>(globalSums[6])
                  << " threads/rank=" << ThreadPool::GetDefaultThreadCount()
                  << " pieces/rank min/avg/max=" << globalPiecesMin << '/'
                  << (globalSums[7] / static_cast<double>(size)) << '/'
                  << globalPiecesMax << '\n';
        std::cout << "[load-summary] dataset on disk: start frame="
                  << FormatBytes(globalSums[0]) << ", all " << numFrames
                  << " frame(s)=" << FormatBytes(globalSums[1]) << '\n';
        if (usedResample) {
            std::cout << "[load-summary] memory: source(start frame, resident)="
                      << FormatBytes(globalSums[2])
                      << ", resampled(all cached frames, volume+mask)="
                      << FormatBytes(globalSums[3])
                      << ", total=" << FormatBytes(globalSums[2] + globalSums[3])
                      << '\n';
            std::cout << "[load-summary] resample: target=" << tni << 'x' << tnj
                      << 'x' << tnk << " per rank, voxels/frame total="
                      << FormatVoxels(globalSums[4]) << ", valid="
                      << FormatVoxels(globalSums[5]) << " (" << std::fixed
                      << std::setprecision(1) << validPct << "%)\n";
        } else {
            // 单块 StructuredMesh 直接复用（每帧各自一份）：源数据就是体数据，不重复计。
            std::cout << "[load-summary] memory: volume(all cached frames) = "
                      << FormatBytes(globalSums[3])
                      << " (single-piece input used directly, no resample)\n";
        }
        std::cout << "[load-summary] load time per rank: total min/avg/max="
                  << FormatSeconds(minLoad) << '/' << FormatSeconds(avgLoad) << '/'
                  << FormatSeconds(maxLoad) << " (slowest rank " << slowest
                  << " on " << slowestNodeResolved << "); avg read="
                  << FormatSeconds(globalSums[8] / static_cast<double>(size))
                  << " avg resample="
                  << FormatSeconds(globalSums[9] / static_cast<double>(size))
                  << " aggregate-read=" << std::fixed << std::setprecision(1)
                  << throughputMiBs << " MiB/s\n";
        std::cout << "[load-summary] startup: distribute=" << FormatSeconds(distSec)
                  << " field=" << FormatSeconds(fieldSec)
                  << " load(all frames)=" << FormatSeconds(loadWallSec)
                  << " prep(range/bounds/tf)=" << FormatSeconds(prepSec)
                  << " total=" << FormatSeconds(SecSince(tProgram0, tSummary))
                  << '\n';
        std::cout << "[load-summary] field range=[" << std::defaultfloat
                  << std::setprecision(6) << globalMin << ", " << globalMax
                  << "] hasRange=" << hasRange
                  << " bbox.diag=" << globalBounds.diag()
                  << " voxelSize=" << voxelSize << '\n';
        std::cout.flush();
    }

    // -------------------------------------------------------------------------
    // CPU 后端（阶段 4）：纯 CPU 光线步进体渲染，无头、不依赖 OpenGL/GLFW。
    // 每个 rank 用自己的超块体素场渲染到 CPU 图像缓冲（RGBA + float 深度），
    // 再交给 iGameCompositePass 做深度有序合成，rank 0 输出合成 PNG。
    // 与下方 GPU 路径共用同一分发/重采样/全局标量范围/全局包围盒/相机视角，
    // 便于逐像素对照。默认仍走 GPU 路径（-n 1 / -n N 行为不变）。
    // -------------------------------------------------------------------------
    if (useCPU) {
        // 6 个主轴视角（与 GPU 路径一致）。
        struct AxisView {
            const char* name;
            double dir[3];
            double up[3];
        };
        const AxisView views[6] = {
                {"+Z", {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}},
                {"-Z", {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}},
                {"+X", {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}},
                {"-X", {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}},
                {"+Y", {0.0, 1.0, 0.0}, {0.0, 0.0, -1.0}},
                {"-Y", {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}},
        };
        const int viewCount = static_cast<int>(sizeof(views) / sizeof(views[0]));

        const Vector3d gc = globalBounds.center();
        const double gcenter[3] = {gc[0], gc[1], gc[2]};
        const double camDist = globalBounds.diag() / 2.0 * 3.0;

        const Vector3d lc = localBounds.center();
        const double blockCenter[3] = {lc[0], lc[1], lc[2]};

        const int width = 1024;
        const int height = 1024;

        std::error_code ec;
        const std::filesystem::path outDir =
                std::filesystem::path(GetExecutableDirectory()) / "out";
        std::filesystem::create_directories(outDir, ec);
        const std::string timestamp = MakeTimestamp();

        // 传输函数：与 Scene 内部 m_VolumeTransferFunction 同口径（Fast 配色 +
        // 不透明度映射）。开启模型不透明度映射对标 scene->SetParallelVolumeRendering(true)。
        auto tf = iGameVolumeTransferFunction::New();
        if (hasRange) { tf->SetScalarRange(globalMin, globalMax); }
        // 显式开启透明度映射：不要从 volume 读回 GetOpacityMappingEnabled()。
        // 重采样产物的 color mapper/激活属性未初始化（GetAttributeIndex()==-1），
        // GetOpacityMappingEnabled() 恒为 false，会把 TF 的透明度映射意外关掉，
        // MapOpacity 恒返回 1.0 → 体渲染退化为表面渲染（本问题根因）。
        tf->SetOpacityMappingEnabled(true);
        // 最小值处的底色雾（见 8.6 的推导与日志）：0 = 最小值区域完全透明（历史行为）。
        tf->SetMinOpacity(static_cast<float>(minOpacityValue));

        auto cpuRayCaster = iGameVolumeRayCastCPU::New();
        if (!cpuRayCaster->SetInput(volumes[static_cast<size_t>(startFrame)])) {
            if (rank == 0) { std::cerr << "CPU ray-caster SetInput failed.\n"; }
            ParallelContext::Finalize();
            return 1;
        }
        cpuRayCaster->SetValidMask(masks[static_cast<size_t>(startFrame)]);
        cpuRayCaster->SetTransferFunction(tf);
        cpuRayCaster->SetMaxSamples(512);
        cpuRayCaster->SetEmptySpaceSkippingEnabled(true);
        // Beer-Lambert 单位距离 = 全局体素尺寸：让「每单位长度的光学厚度」与采样步长无关，
        // 从而与「本 rank 分到多大的超块」无关（消除块状密度台阶，见 8.5 的说明）。
        cpuRayCaster->SetScalarOpacityUnitDistance(voxelSize);

        // 无头相机（纯数学，不依赖 GL 上下文）。
        auto cpuCamera = Camera::New();
        cpuCamera->SetViewPort(width, height);
        const igm::mat4 modelMatrix(1.0f);

        // 阶段 5 交互窗口：rank 0 打开窗口显示「各 rank 离屏渲染 → 合成」的结果，
        // 左键拖动旋转、滚轮缩放、左下角 colorbar、拖动期间显示 fps（仅 CPU 后端）。
        if (cli.interactive) {
            const double radius = globalBounds.diag() / 2.0;
            const int rc = iGameVolInteractive::RunInteractive(
                    cpuRayCaster.get(), cpuCamera.get(), tf.get(), globalMin,
                    globalMax, gcenter, blockCenter, radius, width, height,
                    cli.useBinarySwap, volumes, masks, numFrames,
                    startFrame, selectedField, voxelSize, cli.hqStepScale,
                    cli.lqStepScale, cli.lqDivisor, cli.usePixelwise);
            PrintRunEnd("interactive session ended");
            ParallelContext::Finalize();
            return rc;
        }

        // 阶段 6 C/S 服务端：rank 0 开放端口、接收客户端增量交互命令，渲染 + 合成后
        // 流式回传（对标 MiniPVServer）。仅 CPU 后端；其余 rank 全程无头参与集合通信。
        if (cli.server) {
            const double radius = globalBounds.diag() / 2.0;

            // 每帧的全局包围盒：O 键轨道相机要求「按当前帧的模型包围盒取景」，多帧数据
            // 各帧的几何范围可能不同。各 rank 只上报自己这一帧的局部包围盒，经
            // AllReduce(MIN/MAX) 后所有 rank 得到同一份结果（每帧 2 次小消息集合操作，
            // 只在 --server 分支启动时做一次）。
            std::vector<double> frameCentersXYZ(
                    static_cast<std::size_t>(numFrames) * 3, 0.0);
            std::vector<double> frameRadii(static_cast<std::size_t>(numFrames),
                                           radius);
            for (int f = 0; f < numFrames; ++f) {
                double localMin[3] = {
                        std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::infinity()};
                double localMax[3] = {
                        -std::numeric_limits<double>::infinity(),
                        -std::numeric_limits<double>::infinity(),
                        -std::numeric_limits<double>::infinity()};
                const auto& vol = volumes[static_cast<std::size_t>(f)];
                if (vol) {
                    const BoundingBox& vb = vol->GetBoundingBox();
                    for (int a = 0; a < 3; ++a) {
                        localMin[a] = vb.min[a];
                        localMax[a] = vb.max[a];
                    }
                }
                double gmin[3], gmax[3];
                ctx->AllReduce(localMin, gmin, 3, ParallelContext::ReduceOp::Min);
                ctx->AllReduce(localMax, gmax, 3, ParallelContext::ReduceOp::Max);

                bool ok = true;
                for (int a = 0; a < 3; ++a) {
                    if (!std::isfinite(gmin[a]) || !std::isfinite(gmax[a]) ||
                        gmax[a] < gmin[a]) {
                        ok = false;
                    }
                }
                const std::size_t base = static_cast<std::size_t>(f) * 3;
                if (!ok) {
                    // 该帧在全局都没有有效体：退回全局包围盒（不改变旧行为）。
                    const Vector3d c = globalBounds.center();
                    for (int a = 0; a < 3; ++a) { frameCentersXYZ[base + a] = c[a]; }
                    frameRadii[static_cast<std::size_t>(f)] = radius;
                    continue;
                }
                double ext[3];
                for (int a = 0; a < 3; ++a) {
                    frameCentersXYZ[base + a] = (gmin[a] + gmax[a]) * 0.5;
                    ext[a] = gmax[a] - gmin[a];
                }
                const double r =
                        0.5 * std::sqrt(ext[0] * ext[0] + ext[1] * ext[1] +
                                        ext[2] * ext[2]);
                frameRadii[static_cast<std::size_t>(f)] = (r > 0.0) ? r : radius;
            }
            if (rank == 0 && !frameRadii.empty()) {
                std::cerr << "[server] per-frame bounds: " << numFrames
                          << " frame(s), radius[0]=" << frameRadii.front()
                          << ", radius[last]=" << frameRadii.back()
                          << "; autoplay interval = " << cli.autoPlayIntervalMs
                          << " ms\n";
            }

            // ---- 「可见体素包围盒」：相机取景 / O 轨道 pivot 用它 ----
            //
            // 几何包围盒是分块 bbox 的并集，里面包含"标量恒等于最小值"的平台；默认传输
            // 函数在最小值处不透明度为 0，那块在屏幕上**完全不可见**。拿几何中心当焦点
            // 就等于把镜头对准了一团看不见的包围盒 —— 表现为模型整体偏到一边。
            //
            // 这里改用渲染器的空体素剔除阈值当可见判据（`scalar >= threshold` 且 mask!=0），
            // 圈出"屏幕上真正会出颜色"的体素范围。
            //
            // **只锚定在起始帧算一次**，然后把同一个中心/半径填给所有帧：
            //   · 相机在切帧（N/P、A 自动播放）时完全不动 —— 否则多帧数据的场在演化，
            //     每帧重算 pivot 与取景距离会让画面"跳一下"（闪动），观感很差；
            //   · FOV 本来就固定 45°（kOrbitFixedFovDeg），随帧变动的其实是**距离**
            //     （= 半径 × 系数），所以固定半径 = 固定观感尺寸；
            //   · 顺带把启动开销从「每帧一遍」降到「一遍」。
            //
            // 开销：每 rank 扫一遍自己的体素，每轴隔 kVisibleBoundsStride 个取 1（只查 1/8），
            // 量级 1~2 ms；通信 2 次 AllReduce（各 3 个 double = 24 B/rank），只做一次。
            // 渲染 / 合成 / ROI / 下行载荷全都不受影响（裁剪面仍用几何包围盒）。
            const float visibleThreshold = cpuRayCaster->GetOpacityThreshold();
            std::vector<double> frameFocusCentersXYZ(frameCentersXYZ); // 兜底 = 几何值
            std::vector<double> frameFocusRadii(frameRadii);
            bool focusAnchored = false;
            {
                double localMin[3] = {
                        std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::infinity()};
                double localMax[3] = {
                        -std::numeric_limits<double>::infinity(),
                        -std::numeric_limits<double>::infinity(),
                        -std::numeric_limits<double>::infinity()};
                const auto& rs = anchorResampler;
                const bool hasLocal =
                        rs && rs->ComputeVisibleBounds(visibleThreshold,
                                                       kVisibleBoundsStride,
                                                       localMin, localMax);
                (void)hasLocal; // 没算出来就是 ±inf，AllReduce 后会被下面的 ok 判掉
                double gmin[3], gmax[3];
                ctx->AllReduce(localMin, gmin, 3, ParallelContext::ReduceOp::Min);
                ctx->AllReduce(localMax, gmax, 3, ParallelContext::ReduceOp::Max);
                // 注意：这两个 AllReduce 必须由**所有 rank 无条件**执行（不能按 hasLocal
                // 跳过），否则各 rank 的集合通信次数不一致 → 死锁（不变量 §4.3）。
                bool ok = true;
                for (int a = 0; a < 3; ++a) {
                    if (!std::isfinite(gmin[a]) || !std::isfinite(gmax[a]) ||
                        gmax[a] < gmin[a]) {
                        ok = false;
                    }
                }
                if (ok) {
                    double center[3];
                    double ext[3];
                    for (int a = 0; a < 3; ++a) {
                        center[a] = (gmin[a] + gmax[a]) * 0.5;
                        ext[a] = gmax[a] - gmin[a];
                    }
                    const double r =
                            0.5 * std::sqrt(ext[0] * ext[0] + ext[1] * ext[1] +
                                            ext[2] * ext[2]);
                    // 退化保护：可见区缩成一个点时（r 远小于几何半径）不拿它当取景依据，
                    // 否则相机距离会小到钻进数据里。保留几何兜底值。
                    if (r > radius * 1e-4) {
                        for (int f = 0; f < numFrames; ++f) {
                            const std::size_t base = static_cast<std::size_t>(f) * 3;
                            for (int a = 0; a < 3; ++a) {
                                frameFocusCentersXYZ[base + a] = center[a];
                            }
                            frameFocusRadii[static_cast<std::size_t>(f)] = r;
                        }
                        focusAnchored = true;
                    }
                }
                // 用完立刻释放：不要让它继续持有该帧读进来的原始分块（见上面 anchorResampler
                // 的注释 —— 这就是 1000 rank 上被 OOM 的直接来源）。
                anchorResampler = nullptr;
            }
            if (rank == 0 && numFrames > 0) {
                const std::size_t b0 = static_cast<std::size_t>(startFrame) * 3;
                const double* gc0 = &frameCentersXYZ[b0];
                const double* fc0 = &frameFocusCentersXYZ[b0];
                std::cerr << "[bounds] visible bbox (threshold=" << visibleThreshold
                          << ", stride=" << kVisibleBoundsStride << ") anchored at frame "
                          << startFrame << ": "
                          << (focusAnchored ? "used for all frames (camera is fixed "
                                              "across frame switches)"
                                            : "NOT found -> falling back to the "
                                              "geometry bbox")
                          << '\n';
                std::cerr << "[bounds] anchor focus center = ("
                          << fc0[0] << ", " << fc0[1] << ", " << fc0[2]
                          << "), r=" << frameFocusRadii[static_cast<std::size_t>(startFrame)]
                          << "  vs geometry center = (" << gc0[0] << ", " << gc0[1]
                          << ", " << gc0[2] << "), r="
                          << frameRadii[static_cast<std::size_t>(startFrame)]
                          << "  (delta = " << (fc0[0] - gc0[0]) << ", "
                          << (fc0[1] - gc0[1]) << ", " << (fc0[2] - gc0[2]) << ")\n";
            }

            const int rc = iGamePVServer::RunServer(
                    cpuRayCaster.get(), cpuCamera.get(), tf.get(), globalMin,
                    globalMax, gcenter, blockCenter, radius, width, height,
                    cli.port, cli.useBinarySwap, volumes, masks,
                    numFrames, startFrame, selectedField, voxelSize,
                    cli.hqStepScale, cli.lqStepScale, cli.lqDivisor,
                    cli.usePixelwise, frameCentersXYZ, frameRadii,
                    frameFocusCentersXYZ, frameFocusRadii,
                    cli.autoPlayIntervalMs);
            PrintRunEnd("server stopped");
            ParallelContext::Finalize();
            return rc;
        }

        for (int v = 0; v < viewCount; ++v) {
            // 相机参数：rank0 从全局包围盒 + 视角方向算参数，广播给所有 rank。
            double camPos[3] = {0.0, 0.0, 0.0};
            double camFp[3] = {0.0, 0.0, 0.0};
            double camUp[3] = {0.0, 1.0, 0.0};
            if (rank == 0) {
                for (int d = 0; d < 3; ++d) {
                    camFp[d] = gcenter[d];
                    camPos[d] = gcenter[d] + views[v].dir[d] * camDist;
                    camUp[d] = views[v].up[d];
                }
            }
            ctx->Broadcast(camPos, 3, 0);
            ctx->Broadcast(camFp, 3, 0);
            ctx->Broadcast(camUp, 3, 0);

            cpuCamera->SetPosition(static_cast<float>(camPos[0]),
                                   static_cast<float>(camPos[1]),
                                   static_cast<float>(camPos[2]));
            cpuCamera->SetFocal(static_cast<float>(camFp[0]),
                                static_cast<float>(camFp[1]),
                                static_cast<float>(camFp[2]));
            cpuCamera->SetUp(static_cast<float>(camUp[0]),
                             static_cast<float>(camUp[1]),
                             static_cast<float>(camUp[2]));

            // 复制 Scene::UpdateCameraClippingRange：用全局包围盒计算裁剪范围，
            // 保证所有 rank 投影矩阵一致（深度才能跨进程比较）。
            double front[3] = {camFp[0] - camPos[0], camFp[1] - camPos[1],
                               camFp[2] - camPos[2]};
            const double frontLen = std::sqrt(front[0] * front[0] +
                                              front[1] * front[1] +
                                              front[2] * front[2]);
            if (frontLen > 1e-12) {
                front[0] /= frontLen;
                front[1] /= frontLen;
                front[2] /= frontLen;
            }
            const double toCenter[3] = {gcenter[0] - camPos[0],
                                        gcenter[1] - camPos[1],
                                        gcenter[2] - camPos[2]};
            const double dist = toCenter[0] * front[0] + toCenter[1] * front[1] +
                                toCenter[2] * front[2];
            const double radius = globalBounds.diag() / 2.0;
            double nearPlane = dist - radius;
            double farPlane = dist + radius;
            const double minGap = 0.0001;
            if (nearPlane < minGap * farPlane) { nearPlane = minGap * farPlane; }
            cpuCamera->SetClippingRange(static_cast<float>(nearPlane),
                                        static_cast<float>(farPlane));

            const igm::mat4 view = cpuCamera->GetViewMatrix();
            const igm::mat4 proj = cpuCamera->GetProjectionMatrix();

            std::vector<unsigned char> rgba;
            std::vector<float> depth;
            cpuRayCaster->Render(view, proj, modelMatrix,
                                 igm::uvec2{static_cast<unsigned>(width),
                                            static_cast<unsigned>(height)},
                                 rgba, depth);

            // 每个 rank 保存自己的局部渲染图（透明背景 -> 叠到黑背景成不透明）。
            if (v == 0) {
                auto local = rgba;
                for (std::size_t p = 0; p + 4 <= local.size(); p += 4) {
                    local[p + 3] = 255;
                }
                FlipRGBAVertically(local, width, height);
                const std::string fp =
                        (outDir / (timestamp + "_rank" + std::to_string(rank) +
                                   ".png"))
                                .string();
                if (stbi_write_png(fp.c_str(), width, height, 4, local.data(),
                                   width * 4) == 0) {
                    std::cerr << "[rank " << rank << "] failed to write " << fp
                              << '\n';
                } else {
                    std::cout << "[rank " << rank << "] wrote " << fp << '\n';
                }
            }

            // 分布式合成（深度排序 + over 混合 + MPI_Gather），与 GPU 路径相同。
            // --blockwise（默认）用 iGameCompositePass（超块中心深度，块级有序）；
            // --pixelwise 用 ParallelVolumePixelComposite（逐像素首命中深度排序）。
            auto composite = iGameCompositePass::New();
            iGamePVPixel::PixelCompositePass pixelComposite;
            const std::vector<unsigned char>* frameRGBA = nullptr;
            bool compositeOk = false;
            if (cli.usePixelwise) {
                pixelComposite.SetLocalImage(width, height, rgba, depth);
                pixelComposite.SetBlockDepth(iGameCompositePass::ComputeBlockDepth(
                        blockCenter, camPos, front));
                pixelComposite.SetBackgroundColor(0.0f, 0.0f, 0.0f);
                pixelComposite.SetUseBinarySwapComposite(cli.useBinarySwap);
                compositeOk = pixelComposite.Composite();
                frameRGBA = &pixelComposite.GetResultRGBA();
            } else {
                composite->SetLocalImage(width, height, rgba, depth);
                composite->SetBlockDepth(iGameCompositePass::ComputeBlockDepth(
                        blockCenter, camPos, front));
                composite->SetBackgroundColor(0.0f, 0.0f, 0.0f);
                composite->SetUseBinarySwapComposite(cli.useBinarySwap);
                compositeOk = composite->Composite();
                frameRGBA = &composite->GetResultRGBA();
            }
            if (!compositeOk) {
                if (rank == 0) { std::cerr << "Composite failed.\n"; }
                ParallelContext::Finalize();
                return 1;
            }

            if (rank == 0) {
                auto result = *frameRGBA;
                FlipRGBAVertically(result, width, height);
                const std::string fp =
                        (outDir / (timestamp + "_composited_" + views[v].name +
                                   ".png"))
                                .string();
                if (stbi_write_png(fp.c_str(), width, height, 4, result.data(),
                                   width * 4) == 0) {
                    std::cerr << "[rank 0] failed to write " << fp << '\n';
                } else {
                    std::cout << "[rank 0] wrote composited " << views[v].name
                              << ' ' << width << 'x' << height << " -> " << fp
                              << '\n';
                }
            }
        }

        PrintRunEnd("CPU batch rendering finished");
        ParallelContext::Finalize();
        return 0;
    }

    // 9. 场景 + 相机/传输函数全局一致（GPU 批渲染用起始帧；多帧播放仅 CPU 交互/服务端）。
    auto scene = Scene::New();
    scene->SetBackGround(0, 0, 0);
    scene->SetAxesVisible(false);  // 批渲染不画坐标轴，避免各 rank 图像里的轴被重复合成
    scene->AddModel(volumes[static_cast<size_t>(startFrame)]);
    scene->SetParallelVolumeRendering(true);
    // 裁剪范围用全局包围盒（否则各 rank 投影矩阵不一致，深度无法跨进程比较）。
    scene->SetParallelVolumeClippingBounds(globalBounds);
    if (hasRange) {
        scene->SetParallelVolumeScalarRange(globalMin, globalMax);
    }
    // 透明背景：每个 rank 的帧只含自己的体数据（预乘 alpha + 深度）。
    scene->SetParallelVolumeTransparentBackground(true);

    // 10. 6 个主轴视角方向（dir = 相机相对全局中心的方向；+Z 为默认视角）。
    struct AxisView {
        const char* name;
        double dir[3];
        double up[3];
    };
    const AxisView views[6] = {
            {"+Z", {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}},
            {"-Z", {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}},
            {"+X", {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}},
            {"-X", {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}},
            {"+Y", {0.0, 1.0, 0.0}, {0.0, 0.0, -1.0}},
            {"-Y", {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}},
    };
    const int viewCount = static_cast<int>(sizeof(views) / sizeof(views[0]));

    const Vector3d gc = globalBounds.center();
    const double gcenter[3] = {gc[0], gc[1], gc[2]};
    const double camDist = globalBounds.diag() / 2.0 * 3.0;

    // 本 rank 超块中心（用于每个视角的块深度排序）。
    const Vector3d lc = localBounds.center();
    const double blockCenter[3] = {lc[0], lc[1], lc[2]};

    // 11. 无头离屏渲染（隐藏 GLFW 窗口）。
    const int width = 1024;
    const int height = 1024;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // 无头：创建即隐藏，不闪窗
    auto window = RenderWindow::New();
    window->SetSize(width, height);
    window->SetScene(scene);

    auto camera = scene->GetCamera();

    std::error_code ec;
    const std::filesystem::path outDir =
            std::filesystem::path(GetExecutableDirectory()) / "out";
    std::filesystem::create_directories(outDir, ec);
    const std::string timestamp = MakeTimestamp();

    // 12. 逐个视角渲染 -> 捕获 -> 合成。第一个视角（+Z）同时让每个 rank 保存自己的
    //     局部渲染图（用于查验分发/分区），其余视角仅 rank0 保存合成图。
    for (int v = 0; v < viewCount; ++v) {
        // 相机：rank0 从全局包围盒 + 视角方向算参数，广播给所有 rank（对标
        // TestPVolumeRender.cpp:430-456）。所有 rank 用同一相机 + 同一全局裁剪范围。
        double camPos[3] = {0.0, 0.0, 0.0};
        double camFp[3] = {0.0, 0.0, 0.0};
        double camUp[3] = {0.0, 1.0, 0.0};
        if (rank == 0) {
            for (int d = 0; d < 3; ++d) {
                camFp[d] = gcenter[d];
                camPos[d] = gcenter[d] + views[v].dir[d] * camDist;
                camUp[d] = views[v].up[d];
            }
        }
        ctx->Broadcast(camPos, 3, 0);
        ctx->Broadcast(camFp, 3, 0);
        ctx->Broadcast(camUp, 3, 0);

        camera->SetPosition(static_cast<float>(camPos[0]),
                            static_cast<float>(camPos[1]),
                            static_cast<float>(camPos[2]));
        camera->SetFocal(static_cast<float>(camFp[0]),
                         static_cast<float>(camFp[1]),
                         static_cast<float>(camFp[2]));
        camera->SetUp(static_cast<float>(camUp[0]), static_cast<float>(camUp[1]),
                      static_cast<float>(camUp[2]));

        window->RenderOneFrame();

        // 实际帧缓冲尺寸（与 CaptureParallelVolumeFrame 内部口径一致）。
        const auto vp = scene->GetCamera()->GetScaledViewPort();
        const int fbW = static_cast<int>(vp.x);
        const int fbH = static_cast<int>(vp.y);

        std::vector<unsigned char> rgba;
        std::vector<float> depth;
        scene->CaptureParallelVolumeFrame(rgba, depth);

        // 每个 rank 保存自己的局部渲染图（透明背景 -> 叠到黑背景成不透明）。
        if (v == 0) {
            auto local = rgba;  // 预乘 alpha：rgb 已是叠黑后的最终色，只需把 alpha 置 255
            for (std::size_t p = 0; p + 4 <= local.size(); p += 4) {
                local[p + 3] = 255;
            }
            FlipRGBAVertically(local, fbW, fbH);
            const std::string fp =
                    (outDir / (timestamp + "_rank" + std::to_string(rank) + ".png"))
                            .string();
            if (stbi_write_png(fp.c_str(), fbW, fbH, 4, local.data(),
                               fbW * 4) == 0) {
                std::cerr << "[rank " << rank << "] failed to write " << fp
                          << '\n';
            } else {
                std::cout << "[rank " << rank << "] wrote " << fp << '\n';
            }
        }

        // 分布式合成（深度排序 + over 混合 + MPI_Gather）。
        // --blockwise（默认）用 iGameCompositePass；--pixelwise 用逐像素路径
        // （--gpu 路径的深度来自 Scene::CaptureParallelVolumeFrame 读回的 GL 深度缓冲，
        //   与 CPU 光线步进同为 reversed-z：大 = 近，口径一致）。
        auto composite = iGameCompositePass::New();
        iGamePVPixel::PixelCompositePass pixelComposite;

        double front[3] = {camFp[0] - camPos[0], camFp[1] - camPos[1],
                           camFp[2] - camPos[2]};
        const double frontLen = std::sqrt(front[0] * front[0] +
                                          front[1] * front[1] +
                                          front[2] * front[2]);
        if (frontLen > 1e-12) {
            front[0] /= frontLen;
            front[1] /= frontLen;
            front[2] /= frontLen;
        }
        const double blockDepth = iGameCompositePass::ComputeBlockDepth(
                blockCenter, camPos, front);

        const std::vector<unsigned char>* frameRGBA = nullptr;
        int resultW = 0;
        int resultH = 0;
        bool compositeOk = false;
        if (cli.usePixelwise) {
            pixelComposite.SetLocalImage(fbW, fbH, rgba, depth);
            pixelComposite.SetBlockDepth(blockDepth);
            pixelComposite.SetBackgroundColor(0.0f, 0.0f, 0.0f);
            pixelComposite.SetUseBinarySwapComposite(cli.useBinarySwap);
            compositeOk = pixelComposite.Composite();
            frameRGBA = &pixelComposite.GetResultRGBA();
            resultW = pixelComposite.GetResultWidth();
            resultH = pixelComposite.GetResultHeight();
        } else {
            composite->SetLocalImage(fbW, fbH, rgba, depth);
            composite->SetBlockDepth(blockDepth);
            composite->SetBackgroundColor(0.0f, 0.0f, 0.0f);
            composite->SetUseBinarySwapComposite(cli.useBinarySwap);
            compositeOk = composite->Composite();
            frameRGBA = &composite->GetResultRGBA();
            resultW = composite->GetResultWidth();
            resultH = composite->GetResultHeight();
        }

        if (!compositeOk) {
            if (rank == 0) { std::cerr << "Composite failed.\n"; }
            ParallelContext::Finalize();
            return 1;
        }

        // rank0 输出该视角的合成图。
        if (rank == 0) {
            auto result = *frameRGBA; // 复制，翻转不破坏内部结果
            const int rw = resultW;
            const int rh = resultH;
            FlipRGBAVertically(result, rw, rh);

            const std::string fp =
                    (outDir / (timestamp + "_composited_" + views[v].name +
                               ".png"))
                            .string();
            if (stbi_write_png(fp.c_str(), rw, rh, 4, result.data(),
                               rw * 4) == 0) {
                std::cerr << "[rank 0] failed to write " << fp << '\n';
            } else {
                std::cout << "[rank 0] wrote composited " << views[v].name
                          << ' ' << rw << 'x' << rh << " -> " << fp << '\n';
            }
        }
    }

    PrintRunEnd("GPU batch rendering finished");
    ParallelContext::Finalize();
    return 0;
}
