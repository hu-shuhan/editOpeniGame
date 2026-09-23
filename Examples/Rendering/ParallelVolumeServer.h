#pragma once
// ParallelVolumeServer.h — 并行体绘制 C/S 服务端（阶段 6）渲染循环。
//
// 对标 UnifiedVersion 的 MiniPVServer.cpp：rank 0 开放 TCP 端口，等待前端
// （ParallelVolumeClient.cpp）连接；连接后 rank 0 接收客户端的增量式交互命令
// （INTERACT <dAzimRad> <dElevRad> <zoomFactor> <interactiveFlag>），把所有命令合并
// 后广播给各 rank，各 rank 用同一相机 + 同一全局裁剪范围做 CPU 光线步进渲染，再经
// iGameCompositePass 深度有序合成，最后 rank 0 把合成图（raw RGBA8）流式发给客户端。
//
// 与 RunInteractive（阶段 5）的区别仅在于「rank 0 的显示端」被替换成「socket 发送」，
// 相机轨道 / LOD / 渲染 / 合成逻辑完全一致，便于逐像素对照。
//
// 设计要点（参考 MiniPVServer.cpp:125-132 的流式服务端）：
//   - 阻塞读到至少一行命令后，非阻塞排空 socket，把积压的多条命令合并成一次相机
//     更新 + 一次渲染，避免命令堆积导致交互延迟越来越大；
//   - 拖动时（interactiveFlag=1）降采样 + 大步进（LOD），松手恢复高清（对标
//     MiniPVServer.cpp:845-847）；
//   - 除 rank 0 外其余 rank 全程无头、只参与集合通信与渲染。
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
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <zlib.h>

namespace iGamePVServer {

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
// 返回 0=正常退出（客户端 EXIT/断开），非 0=失败。
// ---------------------------------------------------------------------------
inline int RunServer(iGame::iGameVolumeRayCastCPU* rayCaster,
                     iGame::Camera* camera,
                     iGame::iGameVolumeTransferFunction* tf,
                     double globalMin, double globalMax,
                     const double gcenter[3],
                     const double blockCenter[3], double radius,
                     int width, int height, int port) {
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

    // 接受客户端（rank 0 阻塞）并发送握手元数据；失败则广播退出。
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

            // 握手：发送标量范围 + colorbar ramp + 渲染分辨率。
            iGamePVNet::Metadata meta;
            meta.width = width;
            meta.height = height;
            meta.scalarMin = globalMin;
            meta.scalarMax = globalMax;
            BuildColorbarRamp(tf, meta.colorbar);
            if (!iGamePVNet::SendMetadata(clientSock, meta)) {
                std::cerr << "[server] send metadata failed\n";
                serverStatus = 0;
            }
        }
    }
    ctx->Broadcast(&serverStatus, 1, 0);
    if (!serverStatus) {
        if (rank == 0) {
            iGamePVNet::CloseSocket(clientSock);
            iGamePVNet::CloseSocket(listenSock);
        }
        return 1;
    }

    // ---------- 渲染一帧（所有 rank）并把结果发给客户端（rank 0） ----------
    const igm::mat4 modelMatrix(1.0f);
    auto renderFrame = [&](int interactive) -> bool {
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

        // 交互 LOD（对标 MiniPVServer.cpp:845-847）：交互中采样率砍到 1/4（512→128）、
        // 像素步进 2（1/4 像素）。步长用自动推导（SetStepSize(0)：按**各自分块对角线** /
        // maxSamples），保证所有 rank 在交互时用同样的相对采样率与大步长——若用固定全局
        // 对角线步长，会导致小分块只有 1~2 个采样而大分块仍采满 128 步，出现「只有小分块
        // rank 变低清、大分块 rank 保持原样」的不一致。
        if (interactive) {
            rayCaster->SetMaxSamples(128);
            rayCaster->SetPixelStride(2);
            rayCaster->SetUseScreenROI(true);
            rayCaster->SetStepSize(0.0f); // 自动步长：各自分块对角线 / 128
        } else {
            rayCaster->SetMaxSamples(512);
            rayCaster->SetPixelStride(1);
            rayCaster->SetUseScreenROI(false);
            rayCaster->SetStepSize(0.0f); // 自动步长：各自分块对角线 / 512
        }

        // 各 rank 无头渲染自己的超块。
        const igm::mat4 view = camera->GetViewMatrix();
        const igm::mat4 proj = camera->GetProjectionMatrix();
        std::vector<unsigned char> rgba;
        std::vector<float> depth;
        const auto tRender0 = std::chrono::steady_clock::now();
        rayCaster->Render(view, proj, modelMatrix,
                          igm::uvec2{static_cast<unsigned>(width),
                                     static_cast<unsigned>(height)},
                          rgba, depth);
        const auto tRender1 = std::chrono::steady_clock::now();

        // 分布式深度有序合成。
        auto composite = iGame::iGameCompositePass::New();
        composite->SetLocalImage(width, height, rgba, depth);
        composite->SetBlockDepth(iGame::iGameCompositePass::ComputeBlockDepth(
                blockCenter, camPos, front));
        composite->SetBackgroundColor(0.0f, 0.0f, 0.0f);
        const auto tComposite0 = std::chrono::steady_clock::now();
        const bool compositeOk = composite->Composite();
        const auto tComposite1 = std::chrono::steady_clock::now();
        if (!compositeOk) {
            if (rank == 0) { std::cerr << "[server] composite failed\n"; }
            return false;
        }

        // rank 0 发送合成图（zlib 压缩优先，压缩后更小才用压缩帧）；其余 rank 无事可做。
        if (rank == 0) {
            const auto& result = composite->GetResultRGBA();
            const auto tSend0 = std::chrono::steady_clock::now();
            const std::int32_t rawSize = width * height * 4;
            std::vector<unsigned char> comp;
            bool sent = false;
            // compressBound 给出最坏上界；压缩后更小才用 codec=1，否则退回 raw（codec=0）。
            uLongf compLen = compressBound(static_cast<uLong>(rawSize));
            comp.resize(static_cast<std::size_t>(compLen));
            if (compress2(comp.data(), &compLen, result.data(),
                          static_cast<uLong>(rawSize), Z_BEST_SPEED) == Z_OK &&
                static_cast<std::int32_t>(compLen) < rawSize) {
                sent = iGamePVNet::SendFramePayload(
                        clientSock, width, height, iGamePVNet::kCodecZlib,
                        comp.data(), static_cast<std::int32_t>(compLen), frameSeq);
            } else {
                sent = iGamePVNet::SendFrame(clientSock, width, height,
                                             result.data(), frameSeq);
            }
            const auto tSend1 = std::chrono::steady_clock::now();
            auto ms = [](auto a, auto b) {
                return std::chrono::duration<double, std::milli>(b - a).count();
            };
            std::cerr << "[server] render=" << ms(tRender0, tRender1)
                      << "ms composite=" << ms(tComposite0, tComposite1)
                      << "ms send=" << ms(tSend0, tSend1) << "ms\n";
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

    // ---------- 主循环：读命令 -> 广播 -> 渲染 -> 发送 ----------
    std::string netbuf;
    while (true) {
        // 广播运行状态（rank 0 决定是否继续）；所有 rank 在此同步，随后一致退出。
        int runningFlag = clientAlive ? 1 : 0;
        ctx->Broadcast(&runningFlag, 1, 0);
        if (!runningFlag) { break; }

        bool needRender = false;
        bool cameraChanged = false;
        double accAzim = 0.0, accElev = 0.0, accZoom = 1.0;
        int interactiveMode = 0;

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

        if (!needRenderFlag) { continue; }

        const int interactive = (interactiveMode != 0) ? 1 : 0;

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

    if (rank == 0) {
        iGamePVNet::CloseSocket(clientSock);
        iGamePVNet::CloseSocket(listenSock);
    }
    return 0;
}

} // namespace iGamePVServer
