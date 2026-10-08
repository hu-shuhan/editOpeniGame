// ParallelVolumeRendering.cpp — 并行体绘制入口（阶段 3 分布式合成 + 阶段 4 CPU 后端 +
//   阶段 5 交互窗口 + 阶段 6 C/S 服务端）
//
// CLI（命名参数，默认 CPU 后端）：
//   <program> -i <input> [-t <timestep>] [--resample <res>] [--gpu|--cpu]
//             [--interactive] [--server [--port <n>]] [-f <field>]
//             [--direct|--binary-swap] [--blockwise|--pixelwise]
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
#include <iostream>
#include <limits>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#if defined(__linux__)
#  include <sched.h>   // sched_getaffinity：拿到本进程真正可用的核数（srun --cpu-bind 后）
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
            << "                           half-image exchange plus ordered over, work spread\n"
            << "                           over all ranks and no rank-0 hotspot; meant for\n"
            << "                           thousands of ranks. If given together with\n"
            << "                           --direct, the last one wins.\n"
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

        std::cerr << "Error: unknown option '" << a << "'. Use -h for help.\n";
        return opts; // valid = false
    }

    if (opts.resPerChunk < 2) { opts.resPerChunk = 2; }
    if (!(opts.hqStepScale > 0.0)) { opts.hqStepScale = 1.5; }
    if (!(opts.lqStepScale > 0.0)) { opts.lqStepScale = 4.0; }
    if (opts.lqDivisor < 1) { opts.lqDivisor = 1; }
    if (!opts.input.empty()) { opts.valid = true; }
    return opts;
}
} // namespace

int main(int argc, char** argv) {
    using namespace iGame;

    ParallelContext::Initialize(&argc, &argv);
    auto ctx = ParallelContext::Instance();
    const int rank = ctx->Rank();
    const int size = ctx->Size();

    // iGame 线程池线程数按「本进程实际可用核数」设定（默认 12 在 --cpu-bind=cores 场景
    // 是 12 倍超订）。必须在任何 parallelFor（重采样 / 光线步进 / 砖块构建）之前设置。
    {
        const int cores = DetectUsableCoreCount();
        ThreadPool::SetDefaultThreadCount(cores);
        if (rank == 0) {
            std::cerr << "[threads] iGame ThreadPool default thread count = "
                      << cores << " (usable cores = " << cores << ")\n";
        }
    }

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

    // 1. 文件级分发（阶段 7，对齐 UnifiedVersion/DataDistribution::ComputeDistribution）：
    //    解析 PVD/目录/单文件 → 各 rank 只扫 part%Size==rank 的分块包围盒 → AllReduce
    //    汇总 → 确定性排序 + 连续切块。每个 rank 只知道自己要读哪些分块文件，**不整读全量**。
    auto distributor = iGameVolumeDistributor::New();
    if (!distributor->ComputeFileDistribution(input, timestep)) {
        ParallelContext::Finalize();
        return 1;
    }
    const int nLocalFiles = distributor->GetNumberOfLocalPieceFiles();

    // 打印本 rank 分配到的分块（对标 TestPVolumeRender 的 rank/block/pieces 输出，
    // 确认每个 rank 只读自己那部分分块）。
    {
        const auto blk = distributor->GetLocalBlock();
        std::ostringstream oss;
        oss << "[rank " << rank << "] block=(" << blk.ix0 << ".." << blk.ix1 << ", "
            << blk.iy0 << ".." << blk.iy1 << ", " << blk.iz0 << ".." << blk.iz1
            << "), pieces=" << nLocalFiles << ": [";
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

    double sourceMinSpacing = 0.0;
    for (int f = 0; f < numFrames; ++f) {
        auto composite = DataObject::New();
        for (int i = 0; i < nLocalFiles; ++i) {
            auto piece = FileIO::ReadFile(distributor->GetLocalPieceFile(i, f));
            if (!piece) {
                std::cerr << "[rank " << rank << "] failed to read "
                          << distributor->GetLocalPieceFile(i, f) << '\n';
                ParallelContext::Finalize();
                return 1;
            }
            composite->AddSubDataObject(piece);
        }

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
            volumes[static_cast<size_t>(f)] = resampler->GetStructuredMesh();
            masks[static_cast<size_t>(f)] = resampler->GetValidMask();
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
    }

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
            ParallelContext::Finalize();
            return rc;
        }

        // 阶段 6 C/S 服务端：rank 0 开放端口、接收客户端增量交互命令，渲染 + 合成后
        // 流式回传（对标 MiniPVServer）。仅 CPU 后端；其余 rank 全程无头参与集合通信。
        if (cli.server) {
            const double radius = globalBounds.diag() / 2.0;
            const int rc = iGamePVServer::RunServer(
                    cpuRayCaster.get(), cpuCamera.get(), tf.get(), globalMin,
                    globalMax, gcenter, blockCenter, radius, width, height,
                    cli.port, cli.useBinarySwap, volumes, masks,
                    numFrames, startFrame, selectedField, voxelSize,
                    cli.hqStepScale, cli.lqStepScale, cli.lqDivisor,
                    cli.usePixelwise);
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

    ParallelContext::Finalize();
    return 0;
}
