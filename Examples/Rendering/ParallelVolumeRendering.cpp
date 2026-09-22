// ParallelVolumeRendering.cpp — 并行体绘制入口（阶段 3：分布式合成 + MPI 批渲染）
//
// CLI 对标 UnifiedVersion 的 win/TestPVolumeRenderWin.cpp：
//   <program> <input> [timestep] [resPerChunk]
//     argv[1] input       输入数据（.pvd/.vtm/.igcm 多分块，或 .vtr/.vts/.vtu 单块）
//     argv[2] timestep    PVD 时间步（默认 0；非 PVD 忽略）
//     argv[3] resPerChunk 每块重采样分辨率（默认 64）
// 启动后 rank0 列出该数据可渲染的字段（点/单元标量、向量），提示按名称或编号选择，
// 随后把所选字段广播给所有 rank。
//
// 阶段 3 流程（对标 UnifiedVersion 的 TestPVolumeRender.cpp）：
//   多分块 → iGameVolumeDistributor 分发 → iGameVolumeResampleFilter 重采样 →
//   全局标量范围 AllReduce + 相机参数 Broadcast（保证各 rank 传输函数/投影矩阵一致）→
//   每个 rank 无头离屏渲染自己超块（透明背景 + 预乘 alpha + 深度）→
//   iGameCompositePass 深度有序合成 → rank 0 输出合成 PNG。
//
// 输出（写入 <exe>/out/<时间戳>_*）：
//   - 每个 rank 1 张自己的局部渲染图（_rank<rank>.png，+Z 视角，用于查验分发/分区）；
//   - rank 0 输出 6 个主轴视角（±X/±Y/±Z）的合成图（_composited_<axis>.png，用于查验
//     各分块是否按深度正确合成为一张完整体）。
//
// 进程模型：所有 rank（含 rank 0）都用隐藏 GLFW 窗口离屏渲染自己分到的超块
// （GPU 验证通路的 OffscreenContext = 隐藏窗口，Windows 下配软件 GL / Mesa llvmpipe
// 即可无显示器运行）。-n 1 与 -n N 走同一代码路径，合成结果可直接逐像素对比。
#include "iGameFileIO.h"
#include "iGameParallelContext.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameCompositePass.h"
#include "VolumeMeshAlgorithm/iGameVolumeDistributor.h"
#include "VolumeMeshAlgorithm/iGameVolumeResampleFilter.h"
#include "iGameResourcePath.h"

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
#include <string>
#include <system_error>
#include <vector>

#include <GLFW/glfw3.h>

// 单头文件 PNG 编码器（自包含，含 DEFLATE）。仅本示例用于输出最终合成图。
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../ThirdParty/glfw-3.4/deps/stb_image_write.h"

namespace {
// 本进程写入 PNG 的时间戳（秒级）；同一批合成图用同一时间戳。
std::string MakeTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&tm, &t);
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
bool ComputeLocalScalarRange(iGame::StructuredMesh* mesh, double& mn,
                             double& mx) {
    bool isCell = false;
    auto arr = PickScalarField(mesh, isCell);
    if (!arr) { return false; }

    const IGsize n = arr->GetNumberOfElements();
    if (n <= 0) { return false; }

    bool first = true;
    for (IGsize i = 0; i < n; ++i) {
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
} // namespace

int main(int argc, char** argv) {
    using namespace iGame;

    ParallelContext::Initialize(&argc, &argv);
    auto ctx = ParallelContext::Instance();
    const int rank = ctx->Rank();
    const int size = ctx->Size();

    if (argc < 2) {
        if (rank == 0) {
            std::cerr << "Usage: " << argv[0]
                      << " <input> [timestep] [resPerChunk]\n";
        }
        ParallelContext::Finalize();
        return 1;
    }

    const std::string input = argv[1];
    const int timestep = (argc > 2) ? std::atoi(argv[2]) : 0;
    int resPerChunk = (argc > 3) ? std::atoi(argv[3]) : 64;
    if (resPerChunk < 2) { resPerChunk = 2; }

    // 1. 读入数据（当前各 rank 读同一份；阶段 5 再做 per-rank 读取优化）。
    auto root = FileIO::ReadFile(input);
    if (!root) {
        if (rank == 0) { std::cerr << "Read ERROR: " << input << '\n'; }
        ParallelContext::Finalize();
        return 1;
    }

    // 2. PVD 时间步选择（非 PVD / 无时间帧时忽略）。
    if (timestep > 0) {
        auto frames = root->PeekTimeFrames();
        if (frames && static_cast<int>(frames->GetTimeNum()) > timestep) {
            root->UpdateAnimation(timestep);
        }
    }

    // 3. 收集分块（先只收集，用于列字段；稍后再做分发）。
    auto distributor = iGameVolumeDistributor::New();
    distributor->SetInput(root);

    // 4. 字段选择：rank0 列出可渲染字段并等待输入，随后广播给所有 rank。
    char fieldBuf[1024] = {0};
    if (rank == 0) {
        std::vector<std::string> names;
        std::vector<int> comps;
        std::vector<bool> isCell;
        auto piece = distributor->GetPiece(0);
        if (piece) {
            auto* attrs = piece->GetAttributeSet();
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

    // 5. 空间分发：每个分块恰好归属一个 rank。
    if (!distributor->ComputeDistribution()) {
        if (rank == 0) {
            std::cerr << "Distribution failed: process count > piece count.\n";
        }
        ParallelContext::Finalize();
        return 1;
    }

    // 6. 得到本 rank 的规则体素场：单块结构化网格直接复用，否则重采样。
    StructuredMesh::Pointer volume = nullptr;
    if (distributor->GetNumberOfLocalPieces() == 1) {
        volume = DynamicCast<StructuredMesh>(distributor->GetLocalPiece(0));
        if (volume && !selectedField.empty()) {
            const int idx = volume->GetAttributeSet()->GetAttributeIndex(selectedField);
            if (idx >= 0) { volume->SetAttributeIndex(idx); }
        }
    }
    if (!volume) {
        auto resampler = iGameVolumeResampleFilter::New();
        resampler->SetInput(distributor->GetLocalComposite());
        resampler->SetFieldName(selectedField);
        resampler->SetTargetDims(resPerChunk, resPerChunk, resPerChunk);
        if (!resampler->Execute()) {
            if (rank == 0) { std::cerr << "Resample failed.\n"; }
            ParallelContext::Finalize();
            return 1;
        }
        volume = resampler->GetStructuredMesh();
    }

    // 7. 全局标量范围：各 rank 局部 min/max -> AllReduce -> 全局一致（对标
    //    VolumeRenderingCommon.h:44-50 的 AllReduce(MIN/MAX)）。
    double localMin = 0.0, localMax = 1.0;
    const bool hasRange = ComputeLocalScalarRange(volume.get(), localMin, localMax);
    double globalMin = localMin, globalMax = localMax;
    if (hasRange) {
        ctx->AllReduce(&localMin, &globalMin, 1, ParallelContext::ReduceOp::Min);
        ctx->AllReduce(&localMax, &globalMax, 1, ParallelContext::ReduceOp::Max);
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

    // 9. 场景 + 相机/传输函数全局一致。
    auto scene = Scene::New();
    scene->SetBackGround(0, 0, 0);
    scene->SetAxesVisible(false);  // 批渲染不画坐标轴，避免各 rank 图像里的轴被重复合成
    scene->AddModel(volume);
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
        auto composite = iGameCompositePass::New();
        composite->SetLocalImage(fbW, fbH, rgba, depth);

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
        composite->SetBlockDepth(iGameCompositePass::ComputeBlockDepth(
                blockCenter, camPos, front));
        composite->SetBackgroundColor(0.0f, 0.0f, 0.0f);

        if (!composite->Composite()) {
            if (rank == 0) { std::cerr << "Composite failed.\n"; }
            ParallelContext::Finalize();
            return 1;
        }

        // rank0 输出该视角的合成图。
        if (rank == 0) {
            auto result = composite->GetResultRGBA(); // 复制，翻转不破坏内部结果
            const int rw = composite->GetResultWidth();
            const int rh = composite->GetResultHeight();
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
