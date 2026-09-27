#pragma once
// ParallelVolumeServer.h — 并行体绘制 C/S 服务端（阶段 6）渲染循环。
//
// 对标 UnifiedVersion 的 MiniPVServer.cpp：rank 0 开放 TCP 端口，等待前端
// （ParallelVolumeClient.cpp）连接；连接后 rank 0 接收客户端的增量式交互命令
// （INTERACT <dAzimRad> <dElevRad> <zoomFactor> <interactiveFlag>），把所有命令合并
// 后广播给各 rank，各 rank 用同一相机 + 同一全局裁剪范围做 CPU 光线步进渲染，再经
// iGameCompositePass 深度有序合成，最后 rank 0 把合成图（裁非空 ROI + zlib 压缩）流式
// 发给客户端。
//
// 与 RunInteractive（阶段 5）的区别仅在于「rank 0 的显示端」被替换成「socket 发送」，
// 相机轨道 / LOD / 渲染 / 合成逻辑完全一致，便于逐像素对照。
//
// 持久服务：一个客户端断开 / EXIT / 发送失败后，服务端回到 accept 等下一个客户端，
// 不会退出整个 MPI 作业；只有被外部终止（scancel / Ctrl+C）才会停止。
//
// ---------------------------------------------------------------------------
// 本次修复（对标 UnifiedVersion/MiniPVServer，触发场景：19200 分块 / 1000 rank）
// ---------------------------------------------------------------------------
// BUG A：每帧合成把 P 张 1024x1024 全图（4MB/张）汇聚到 rank 0，rank 0 再做
//        O(P x 像素) 的逐像素块序 over。1000 rank 时汇聚量 4GB 量级、rank 0 单帧
//        合成几百 ms 起（实测 --tree 档也有 ~150ms）。
//        触发条件：rank 数越多越严重；每个 rank 的体数据在屏幕上只占很小一片
//        （19200 块 / 1000 rank 时约 20x20 像素），却要按整屏传输。
//        修复：合成器默认走「稀疏 ROI 合成」——只上报/汇聚非空像素外接矩形，
//        rank 0 按超块深度序逐 ROI 矩形 over。见 iGameCompositePass::CompositeSparse。
//
// BUG B：不透明度用「每步 alpha 直接相乘」（m_UnitDistance <= 0），累计光学厚度
//        ∝ 采样步数；而自适应步长下每个 rank 的步数恰好都等于 maxSamples，
//        于是薄超块和厚超块贡献同样的不透明度 —— 前面的 rank 一饱和就把内部结构
//        遮住，画面上表现为明显的块状明暗台阶 / 顶面纹理发白发糊。
//        触发条件：任何多 rank 拆分（每个 rank 分到的超块尺寸不同）都会触发。
//        修复：SetScalarOpacityUnitDistance(全局体素尺寸) 打开 Beer-Lambert，
//        并把光线步长改为「全局统一步长 = 体素尺寸 x 档位系数」，使密度只由真实
//        路径长度决定，且相邻超块的采样点在 t 轴上严格接续（无接缝）。
//
// BUG C：拖动档只做了像素步进（stride=2），帧仍是 1024x1024 全分辨率，渲染、
//        合成、传输三者的量一点没降。
//        修复：拖动档真的降分辨率（默认 1024 -> 512），对标 MiniPVServer 的
//        800x800 / 256x256 两档。
//
// BUG D：没有背压。服务端出帧快于客户端消费时，帧在 TCP 发送队列里无界堆积，
//        客户端测到的 RTT 单调增长到几万 ms（「松手后追帧」）。
//        修复：交互档发帧前用 ioctl(TIOCOUTQ) 查发送队列，超过 512KB 就丢帧
//        （对标 MiniPVServer.cpp:921-932，仅 Linux）。
//
// 修复提交号：待提交。
//
// 注意：必须先包含本协议头（其内部先于 windows.h 引入 winsock2），再包含可能引入
// windows.h / glad 的头，避免 Windows 下 winsock.h 与 winsock2.h 冲突。
#include "ParallelVolumeProtocol.h"

#include "iGameCamera.h"
#include "iGameCompositePass.h"
#include "iGameParallelContext.h"
#include "iGameVolumeRayCastCPU.h"
#include "iGameVolumeTransferFunction.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <zlib.h>

#if defined(__linux__)
#  include <sys/ioctl.h>   // TIOCOUTQ：查询 TCP 发送队列积压字节数（交互时丢帧用）
#  include <linux/sockios.h>
#endif

namespace iGamePVServer {

// 交互档 TCP 发送队列积压阈值（字节）：超过它就丢弃本帧（不编码/不发送），
// 避免「服务端出帧比客户端消费快 → 队列无界增长 → RTT 涨到几十秒」。
// 对标 UnifiedVersion/MiniPVServer.cpp:266 的 kMaxSendQueueBytes = 512KB。
inline constexpr int kMaxSendQueueBytes = 512 * 1024;

// 从传输函数颜色映射器构建 256 像素 RGBA8 colorbar ramp（仅颜色，alpha 恒 255）。
// 与服务端握手时发给客户端，供其重建左下角 colorbar 渐变纹理。
inline void BuildColorbarRamp(iGame::iGameVolumeTransferFunction* tf,
                              unsigned char ramp[iGamePVNet::kColorbarBytes]) {
    auto mapper = tf ? tf->GetColorMapper() : nullptr;
    for (int i = 0; i < iGamePVNet::kColorbarSize; ++i) {
        const float t = static_cast<float>(i) /
                        static_cast<float>(iGamePVNet::kColorbarSize - 1);
        float rgb[3]{0.5f, 0.5f, 0.5f};
        if (mapper) { mapper->MapColor(t, rgb); }
        auto toU = [](float v) {
            v = std::clamp(v, 0.0f, 1.0f);
            return static_cast<unsigned char>(v * 255.0f + 0.5f);
        };
        ramp[i * 4 + 0] = toU(rgb[0]);
        ramp[i * 4 + 1] = toU(rgb[1]);
        ramp[i * 4 + 2] = toU(rgb[2]);
        ramp[i * 4 + 3] = 255;
    }
}

// ---------------------------------------------------------------------------
// 服务端交互渲染循环（所有 rank 共同调用，与 RunInteractive 并列）。
// 持久服务：一个客户端断开/EXIT 后回到 accept 等下一个客户端，直到被外部终止
// （scancel / Ctrl+C，即手动关闭 MPI 作业）；客户端断开不会让后端退出。
//
// LOD 两档（对标 UnifiedVersion/MiniPVServer.cpp:254-266 与 830-857）：
//   拖动中：分辨率 width/lqDivisor、步长 voxelSize*lqStepScale
//   松手后：分辨率 width×height、步长 voxelSize*hqStepScale
// 之所以要「真的降分辨率」而不只是像素步进：像素步进只减少光线数，帧仍是全分辨率，
// 网络载荷与合成量一点没少（参考实现 800x800 → 256x256，这里 1024x1024 → 512x512）。
//
// 返回 0=正常退出（仅监听失败 / accept 失败），非 0=失败。
// ---------------------------------------------------------------------------
inline int RunServer(iGame::iGameVolumeRayCastCPU* rayCaster,
                     iGame::Camera* camera,
                     iGame::iGameVolumeTransferFunction* tf,
                     double globalMin, double globalMax,
                     const double gcenter[3],
                     const double blockCenter[3], double radius,
                     int width, int height, int port, bool useTree,
                     bool useRadixK,
                     const std::vector<iGame::StructuredMesh::Pointer>& volumes,
                     const std::vector<iGame::UnsignedCharArray::Pointer>& masks,
                     int numFrames, int startFrame,
                     const std::string& fieldName,
                     double voxelSize = 0.0, double hqStepScale = 1.5,
                     double lqStepScale = 4.0, int lqDivisor = 2) {
    auto ctx = iGame::ParallelContext::Instance();
    const int rank = ctx->Rank();

    // 相机轨道状态（仅 rank 0 维护；其余 rank 只接收广播后的绝对相机参数）。
    double azimuth = 0.0;      // 方位角（弧度）
    double elevation = 0.0;    // 仰角（弧度）
    const double diag = radius * 2.0;
    double distance = radius * 3.0;

    // 持久相机参数（每帧由 rank 0 计算并广播）。
    double camPos[3] = {0.0, 0.0, 0.0};
    double camFp[3] = {0.0, 0.0, 0.0};
    double camUp[3] = {0.0, 1.0, 0.0};

    // 触发本帧的 INTERACT 命令序号（rank 0 维护，随帧回传给客户端测 RTT）。
    std::uint32_t frameSeq = 0;

    // 多帧播放：当前帧序号（0..numFrames-1）。所有 rank 保持一致（由 frameStep 广播驱动）。
    int curFrame = startFrame;

    // ---------- socket 监听 / 接受（仅 rank 0） ----------
    // 注意：任何 rank 0 侧的 socket 失败都必须通过 Broadcast 通知所有 rank 一起退出，
    // 否则其它 rank 会阻塞在后续集合通信上造成死锁。
    PVSocket listenSock = PV_INVALID_SOCKET;
    PVSocket clientSock = PV_INVALID_SOCKET;
    int serverStatus = 1;
    if (rank == 0) {
        if (!iGamePVNet::Startup()) {
            std::cerr << "[server] socket startup failed\n";
            serverStatus = 0;
        } else {
            listenSock = socket(AF_INET, SOCK_STREAM, 0);
            if (listenSock == PV_INVALID_SOCKET) {
                std::cerr << "[server] socket() failed\n";
                serverStatus = 0;
            } else {
                int one = 1;
                setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR,
#ifdef _WIN32
                           reinterpret_cast<const char*>(&one),
#else
                           &one,
#endif
                           sizeof(one));

                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = htonl(INADDR_ANY);
                addr.sin_port = htons(static_cast<unsigned short>(port));
                if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr),
                         sizeof(addr)) != 0) {
                    std::cerr << "[server] bind port " << port << " failed\n";
                    serverStatus = 0;
                } else {
                    listen(listenSock, 1);
                    std::cout << "[server] rank 0 listening on port " << port
                              << '\n';
                    std::cout.flush();
                }
            }
        }
    }
    ctx->Broadcast(&serverStatus, 1, 0);
    if (!serverStatus) {
        if (rank == 0) { iGamePVNet::CloseSocket(listenSock); }
        return 1;
    }

    // ---------- 持久服务循环：一个客户端断开后回到 accept 等下一个，直到被外部终止 ----------
    // 手动关闭 = scancel / Ctrl+C（外部终止 MPI 作业），不需要客户端发任何停止命令。
    // 所有 rank 必须在此循环里保持集合通信步调一致：rank 0 阻塞在 accept 时，其余 rank
    // 阻塞在随后的 Broadcast 上等待，不会死锁（与首次 accept 同一模式）。
    // 注：为保持最小 diff，循环体内沿用原缩进（与 `for` 同级），结构以本注释与函数末尾的
    // `} // 持久服务循环` 为准，不影响编译。
    std::string netbuf;
    for (;;) {
    // 每个新客户端从默认 +Z 视角、全新交互状态开始（不继承上一个客户端的轨道/帧序号）。
    azimuth = 0.0;
    elevation = 0.0;
    distance = radius * 3.0;
    frameSeq = 0;
    curFrame = startFrame;
    // 新客户端回到起始帧：把光线步进器输入切回起始帧（上一客户端可能切到过别的帧）。
    if (numFrames > 1) {
        rayCaster->SetInput(volumes[static_cast<size_t>(startFrame)]);
        rayCaster->SetValidMask(masks[static_cast<size_t>(startFrame)]);
    }
    netbuf.clear();
    clientSock = PV_INVALID_SOCKET;

    // 接受客户端（rank 0 阻塞）并发送握手元数据；accept 失败则广播一致退出服务循环。
    serverStatus = 1;
    if (rank == 0) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int alen = sizeof(clientAddr);
#else
        socklen_t alen = sizeof(clientAddr);
#endif
        clientSock = accept(listenSock,
                            reinterpret_cast<sockaddr*>(&clientAddr), &alen);
        if (clientSock == PV_INVALID_SOCKET) {
            std::cerr << "[server] accept failed\n";
            serverStatus = 0;
        } else {
            std::cout << "[server] client connected\n";
            std::cout.flush();

            // 禁用 Nagle：降低交互延迟（对标 MiniPVServer）。
            int one = 1;
            setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY,
#ifdef _WIN32
                       reinterpret_cast<const char*>(&one),
#else
                       &one,
#endif
                       sizeof(one));

            // 握手：发送标量范围 + colorbar ramp + 渲染分辨率 + 帧数 + 字段名。
            iGamePVNet::Metadata meta;
            meta.width = width;
            meta.height = height;
            meta.numFrames = numFrames;
            meta.scalarMin = globalMin;
            meta.scalarMax = globalMax;
            BuildColorbarRamp(tf, meta.colorbar);
            std::memset(meta.fieldName, 0, sizeof(meta.fieldName));
            if (!fieldName.empty()) {
                std::strncpy(meta.fieldName, fieldName.c_str(),
                             sizeof(meta.fieldName) - 1);
            }
            if (!iGamePVNet::SendMetadata(clientSock, meta)) {
                std::cerr << "[server] send metadata failed\n";
                serverStatus = 0;
            }
        }
    }
    ctx->Broadcast(&serverStatus, 1, 0);
    if (!serverStatus) {
        if (rank == 0) { iGamePVNet::CloseSocket(clientSock); }
        break;   // 退出服务循环（listenSock 在函数末尾统一关闭）
    }

    // ---------- 渲染一帧（所有 rank）并把结果发给客户端（rank 0） ----------
    const igm::mat4 modelMatrix(1.0f);

    // LOD 两档参数（所有 rank 用同样入参算出同样结果，不需要额外广播）。
    const int hqW = width;
    const int hqH = height;
    const int lqW = std::max(128, width / std::max(1, lqDivisor));
    const int lqH = std::max(128, height / std::max(1, lqDivisor));
    // 全局统一步长：voxelSize 是「所有 rank 一致的全局体素尺寸」（由 main 里 AllReduce
    // 得到）。步长以体素为单位，因此与「本 rank 超块的弦长」无关——配合 Beer-Lambert
    // 的 unitDistance，光学厚度只由真实路径长度决定，块边界不会出现密度台阶。
    // 另：步长全局一致 ⇒ 各 rank 的采样点落在同一条「以近裁剪面为原点的 t 栅格」上，
    // 超块之间严格接续，不会出现缝隙/重复（见 iGameVolumeRayCastCPU::RayCastPixel）。
    const bool hasGlobalStep = (voxelSize > 0.0);
    const float hqStep = hasGlobalStep
                                 ? static_cast<float>(voxelSize * hqStepScale)
                                 : 0.0f;
    const float lqStep = hasGlobalStep
                                 ? static_cast<float>(voxelSize * lqStepScale)
                                 : 0.0f;
    // maxSamples 在「全局统一步长」下只作安全护栏：真正迭代次数由本超块弦长决定
    // （步长固定后，薄超块只跑几十步就出块）。上限取全局对角线/步长 + 余量。
    auto sampleCap = [&](float step) -> int {
        if (!(step > 0.0f)) { return 512; }
        const double n = std::ceil(2.0 * radius / static_cast<double>(step)) + 16.0;
        return static_cast<int>(std::clamp(n, 8.0, 8192.0));
    };
    const int hqMaxSamples = hasGlobalStep ? sampleCap(hqStep) : 512;
    const int lqMaxSamples = hasGlobalStep ? sampleCap(lqStep) : 128;

    auto renderFrame = [&](int interactive) -> bool {
        // 交互档真的降分辨率（对标 MiniPVServer kW_LQ=256 / kW_HQ=800）：只做像素步进
        // 不减少网络载荷与合成量，而降分辨率对「渲染 + 合成 + 传输」三者同时生效。
        const int fw = interactive ? lqW : hqW;
        const int fh = interactive ? lqH : hqH;
        camera->SetViewPort(fw, fh);

        // 所有 rank 用同一相机（位置/焦点/上方向 + 全局裁剪范围）。
        camera->SetPosition(static_cast<float>(camPos[0]),
                            static_cast<float>(camPos[1]),
                            static_cast<float>(camPos[2]));
        camera->SetFocal(static_cast<float>(camFp[0]),
                         static_cast<float>(camFp[1]),
                         static_cast<float>(camFp[2]));
        camera->SetUp(static_cast<float>(camUp[0]),
                      static_cast<float>(camUp[1]),
                      static_cast<float>(camUp[2]));

        double front[3] = {camFp[0] - camPos[0], camFp[1] - camPos[1],
                           camFp[2] - camPos[2]};
        const double frontLen =
                std::sqrt(front[0] * front[0] + front[1] * front[1] +
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
        double nearPlane = dist - radius;
        double farPlane = dist + radius;
        const double minGap = 0.0001;
        if (nearPlane < minGap * farPlane) { nearPlane = minGap * farPlane; }
        camera->SetClippingRange(static_cast<float>(nearPlane),
                                 static_cast<float>(farPlane));

        rayCaster->SetMaxSamples(interactive ? lqMaxSamples : hqMaxSamples);
        rayCaster->SetPixelStride(1);
        rayCaster->SetUseScreenROI(true);
        // 全局统一步长（voxelSize * scale）；voxelSize 不可用时退回历史自适应步长。
        rayCaster->SetStepSize(interactive ? lqStep : hqStep);

        // 各 rank 无头渲染自己的超块。
        const igm::mat4 view = camera->GetViewMatrix();
        const igm::mat4 proj = camera->GetProjectionMatrix();
        std::vector<unsigned char> rgba;
        std::vector<float> depth;
        const auto tRender0 = std::chrono::steady_clock::now();
        rayCaster->Render(view, proj, modelMatrix,
                          igm::uvec2{static_cast<unsigned>(fw),
                                     static_cast<unsigned>(fh)},
                          rgba, depth);
        const auto tRender1 = std::chrono::steady_clock::now();

        // 分布式深度有序合成。默认走「稀疏 ROI 合成」：每个 rank 只上报自己非空像素的
        // 外接矩形，汇聚量与 rank 0 工作量都和 rank 数基本解耦（对标 IceT
        // valid_pixels_viewport）。--tree 时改走并行树合成；--radix-k 时改走 radix-k
        // 合成（对标 IceT icetRadixkCompose，通信与合成摊到所有 rank）。
        auto composite = iGame::iGameCompositePass::New();
        composite->SetLocalImage(fw, fh, rgba, depth);
        composite->SetBlockDepth(iGame::iGameCompositePass::ComputeBlockDepth(
                blockCenter, camPos, front));
        composite->SetBackgroundColor(0.0f, 0.0f, 0.0f);
        composite->SetUseTreeComposite(useTree);
        composite->SetUseRadixKComposite(useRadixK);
        const auto tComposite0 = std::chrono::steady_clock::now();
        const bool compositeOk = composite->Composite();
        const auto tComposite1 = std::chrono::steady_clock::now();
        if (!compositeOk) {
            if (rank == 0) { std::cerr << "[server] composite failed\n"; }
            return false;
        }

        // rank 0 发送合成图；其余 rank 无事可做。
        if (rank == 0) {
            const auto tSend0 = std::chrono::steady_clock::now();

            // 1) ROI 由合成器直接给出（稀疏路径在合成时就知道了），无需再扫一遍全图。
            //    全图路径（--tree）下 Composite() 收尾时也已扫过一次，语义一致。
            const int rx0 = composite->GetResultROIX();
            const int ry0 = composite->GetResultROIY();
            const int roiW = composite->GetResultROIW();
            const int roiH = composite->GetResultROIH();
            const std::vector<unsigned char>& roi =
                    composite->GetResultROIRGBA();

            // 2) 交互档背压：TCP 发送队列积压超过阈值说明客户端消费不过来，此时继续发
            //    只会让延迟无界增长（「松手后追帧」）。宁可丢帧（返回 true，客户端会话
            //    保持），等队列排空后再发最新帧。（对标 MiniPVServer.cpp:921-932 的
            //    ioctl(TIOCOUTQ) 丢帧策略；仅 Linux 可用，其它平台跳过。）
            bool queueBacklogged = false;
#if defined(__linux__)
            if (interactive) {
                int outq = 0;
                if (ioctl(clientSock, TIOCOUTQ, &outq) == 0 &&
                    outq > kMaxSendQueueBytes) {
                    queueBacklogged = true;
                }
            }
#endif
            if (queueBacklogged) {
                auto ms = [](auto a, auto b) {
                    return std::chrono::duration<double, std::milli>(b - a)
                            .count();
                };
                std::cerr << "[server] render=" << ms(tRender0, tRender1)
                          << "ms composite=" << ms(tComposite0, tComposite1)
                          << "ms send=DROPPED (send queue backlogged)\n";
                return true;
            }

            // 3) 压缩 ROI（压缩后更小才用 codec=1，否则 codec=0 raw ROI）。
            const std::int32_t rawSize = static_cast<std::int32_t>(roi.size());
            bool sent = false;
            if (rawSize <= 0) {
                sent = iGamePVNet::SendFramePayload(clientSock, fw, fh,
                        iGamePVNet::kCodecRawRGBA, rx0, ry0, roiW, roiH,
                        nullptr, 0, frameSeq, curFrame);
            } else {
                std::vector<unsigned char> comp;
                uLongf compLen = compressBound(static_cast<uLong>(rawSize));
                comp.resize(static_cast<std::size_t>(compLen));
                if (compress2(comp.data(), &compLen, roi.data(),
                              static_cast<uLong>(rawSize), Z_BEST_SPEED) == Z_OK &&
                    static_cast<std::int32_t>(compLen) < rawSize) {
                    sent = iGamePVNet::SendFramePayload(
                            clientSock, fw, fh, iGamePVNet::kCodecZlib,
                            rx0, ry0, roiW, roiH, comp.data(),
                            static_cast<std::int32_t>(compLen), frameSeq,
                            curFrame);
                } else {
                    sent = iGamePVNet::SendFramePayload(
                            clientSock, fw, fh, iGamePVNet::kCodecRawRGBA,
                            rx0, ry0, roiW, roiH, roi.data(), rawSize, frameSeq,
                            curFrame);
                }
            }

            const auto tSend1 = std::chrono::steady_clock::now();
            auto ms = [](auto a, auto b) {
                return std::chrono::duration<double, std::milli>(b - a).count();
            };
            std::cerr << "[server] " << fw << 'x' << fh
                      << " render=" << ms(tRender0, tRender1)
                      << "ms composite=" << ms(tComposite0, tComposite1)
                      << "ms send=" << ms(tSend0, tSend1)
                      << "ms roi=" << roiW << 'x' << roiH
                      << " bytes=" << rawSize << '\n';
            return sent;
        }
        return true;
    };

    // ---------- 初始帧：默认 +Z 视角，保证客户端一连上就能看到体数据 ----------
    if (rank == 0) {
        for (int d = 0; d < 3; ++d) {
            camFp[d] = gcenter[d];
            camPos[d] = gcenter[d] + (d == 2 ? distance : 0.0);
            camUp[d] = (d == 1 ? 1.0 : 0.0);
        }
    }
    ctx->Broadcast(camPos, 3, 0);
    ctx->Broadcast(camFp, 3, 0);
    ctx->Broadcast(camUp, 3, 0);
    // clientAlive 仅在 rank 0 更新；其余 rank 通过主循环顶部的 Broadcast(runningFlag)
    // 保持一致，避免某条 send 失败后 rank 0 提前退循环造成其它 rank 集合通信死锁。
    bool clientAlive = renderFrame(0);

    // ---------- 会话循环：读命令 -> 广播 -> 渲染 -> 发送（每 accept 一个客户端进入一次） ----------
    while (true) {
        // 广播运行状态（rank 0 决定是否继续）；所有 rank 在此同步，随后一致退出本会话。
        int runningFlag = clientAlive ? 1 : 0;
        ctx->Broadcast(&runningFlag, 1, 0);
        if (!runningFlag) { break; }

        bool needRender = false;
        bool cameraChanged = false;
        double accAzim = 0.0, accElev = 0.0, accZoom = 1.0;
        int interactiveMode = 0;
        int frameStep = 0; // 本批 NEXT/PREV 累积的帧步进（+1/-1）

        if (rank == 0) {
            // 1) 阻塞读到至少一行命令。
            while (netbuf.find('\n') == std::string::npos) {
                std::string line;
                if (!iGamePVNet::RecvLine(clientSock, line)) {
                    std::cerr << "[server] client disconnected\n";
                    clientAlive = false;
                    break;
                }
                netbuf += line;
            }
            // 2) 非阻塞排空，尽量合并更多命令。
            if (clientAlive) {
                iGamePVNet::DrainNonBlock(clientSock, netbuf);
                // 3) 解析所有完整行，合并成一次相机更新 + 一次渲染。
                std::string line;
                while (clientAlive && iGamePVNet::PopLine(netbuf, line)) {
                    if (line.empty()) { continue; }
                    std::istringstream iss(line);
                    std::string op;
                    iss >> op;
                    if (op.empty()) { continue; }
                    if (op == "EXIT") {
                        clientAlive = false;
                        break;
                    }
                    if (op == "INTERACT") {
                        std::uint32_t seq = 0;
                        double a = 0.0, e = 0.0, z = 1.0;
                        int mode = 0;
                        if (iGamePVNet::ParseInteractCommand(line, seq, a, e, z,
                                                             mode)) {
                            if (mode == 0 || mode == 1) { interactiveMode = mode; }
                            if (std::abs(a) > 1e-12 || std::abs(e) > 1e-12 ||
                                std::abs(z - 1.0) > 1e-12) {
                                accAzim += a;
                                accElev += e;
                                accZoom *= z;
                                cameraChanged = true;
                            }
                            needRender = true;
                            // 用本批里最后一条命令的 seq 作为回帧序号（对标 MiniPVServer）。
                            frameSeq = seq;
                        }
                        continue;
                    }
                    if (op == "NEXT") {
                        frameStep += 1;
                        needRender = true;
                        continue;
                    }
                    if (op == "PREV") {
                        frameStep -= 1;
                        needRender = true;
                        continue;
                    }
                    std::cerr << "[server] unknown cmd: " << line << '\n';
                }
            }
        }

        // 广播合并后的命令标志（注意：Broadcast 只接受非 const 指针，这里用非 const int）。
        int needRenderFlag = needRender ? 1 : 0;
        int cameraChangedFlag = cameraChanged ? 1 : 0;
        ctx->Broadcast(&needRenderFlag, 1, 0);
        ctx->Broadcast(&cameraChangedFlag, 1, 0);
        ctx->Broadcast(&interactiveMode, 1, 0);
        ctx->Broadcast(&frameStep, 1, 0);

        if (!needRenderFlag) { continue; }

        const int interactive = (interactiveMode != 0) ? 1 : 0;

        // 多帧播放切帧：NEXT/PREV 越界回绕。所有 rank 用同一 frameStep + 同一
        // numFrames，因此 curFrame 全程一致；切帧只换数据指针（SetInput 重提标量场），
        // 渲染/合成路径不变，帧率不受影响。
        if (frameStep != 0 && numFrames > 1) {
            curFrame = (curFrame + frameStep) % numFrames;
            if (curFrame < 0) { curFrame += numFrames; }
            rayCaster->SetInput(volumes[static_cast<size_t>(curFrame)]);
            rayCaster->SetValidMask(masks[static_cast<size_t>(curFrame)]);
        }

        // rank 0 累积轨道增量并计算绝对相机参数。
        if (rank == 0 && cameraChangedFlag) {
            azimuth += accAzim;
            elevation += accElev;
            elevation = std::clamp(elevation, -1.55, 1.55);
            distance *= accZoom;
            distance = std::clamp(distance, diag * 0.05, diag * 20.0);
        }
        if (rank == 0) {
            const double dir[3] = {
                    std::cos(elevation) * std::sin(azimuth),
                    std::sin(elevation),
                    std::cos(elevation) * std::cos(azimuth),
            };
            for (int d = 0; d < 3; ++d) {
                camFp[d] = gcenter[d];
                camPos[d] = gcenter[d] + dir[d] * distance;
                camUp[d] = (d == 1 ? 1.0 : 0.0);
            }
        }
        ctx->Broadcast(camPos, 3, 0);
        ctx->Broadcast(camFp, 3, 0);
        ctx->Broadcast(camUp, 3, 0);

        // 发送失败（客户端断开）不在此处直接 break，而是标记 clientAlive=false，
        // 由下一轮顶部的 Broadcast 统一通知所有 rank 退出，避免失步死锁。
        if (!renderFrame(interactive)) { clientAlive = false; }
    }

    // 客户端断开/EXIT/发送失败：关闭本会话 socket，回到 accept 等下一个客户端（不退出服务）。
    if (rank == 0) {
        iGamePVNet::CloseSocket(clientSock);
        clientSock = PV_INVALID_SOCKET;
        std::cout << "[server] client session ended; waiting for next client...\n";
        std::cout.flush();
    }
    } // 持久服务循环

    if (rank == 0) {
        iGamePVNet::CloseSocket(listenSock);
    }
    return 0;
}

} // namespace iGamePVServer
