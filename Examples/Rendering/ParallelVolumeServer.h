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
// 动画交互（服务端定时驱动，客户端只发一条开关命令）：
//   - ORBIT 1/0（O 键 Toggle Orbit Camera）：把镜头对准被渲染模型（按当前帧包围盒重新
//     取景，FOV 固定在 45°、取景距离一次算好且轨道中不缩放），随后以模型中心为轴做
//     「双轴转台」运动：方位角连续旋转，俯仰角在 ±30° 之间用三角波缓慢扫掠（端点短缓动）；
//   - AUTOPLAY 1/0（A 键 Toggle Auto Play）：按 --autoplay-interval（默认 2s/帧）自动
//     切帧，方向取最近一次 N/P 的方向（从未按过则默认向后）；播放中收到 N/P 会自动关闭。
// 这两条路径都不依赖客户端继续发命令，因此动画速度不会被客户端显示循环（X11 转发下
// 可能只有 2~3Hz）拖慢；服务端等命令时用 select 超时，动画开启就不会阻塞在 recv 上。
//
// 持久服务：一个客户端断开 / EXIT / 发送失败后，服务端回到 accept 等下一个客户端，
// 不会退出整个 MPI 作业；只有被外部终止（scancel / Ctrl+C）才会停止。
//
// ---------------------------------------------------------------------------
// 本次修复（对标 UnifiedVersion/MiniPVServer，触发场景：19200 分块 / 1000 rank）
// ---------------------------------------------------------------------------
// BUG A：每帧合成把 P 张 1024x1024 全图（4MB/张）汇聚到 rank 0，rank 0 再做
//        O(P x 像素) 的逐像素块序 over。1000 rank 时汇聚量 4GB 量级、rank 0 单帧
//        合成几百 ms 起。
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
#include "ParallelVolumePixelComposite.h" // --pixelwise 逐像素合成（pvr 模块内）

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
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

// ---------------------------------------------------------------------------
// O 键自动轨道相机（Toggle Orbit Camera）与 A 键自动播放（Toggle Auto Play）
// ---------------------------------------------------------------------------
// 两者都由服务端定时推进（而不是靠客户端按显示循环发命令）：
//   - 客户端只发一条 ORBIT 1/0 或 AUTOPLAY 1/0，之后即使不再发任何命令，
//     服务端也会持续出帧；这样动画速度与客户端显示循环（X11 转发下可能只有 2~3Hz）解耦。
//   - 服务端等待命令时用 select 超时，动画开启就不会阻塞在 recv 上。
inline constexpr int kOrbitIntervalMs = 40;       // 轨道相机最高出帧间隔（25fps 上限）
inline constexpr int kDefaultAutoPlayIntervalMs = 2000; // A 键自动播放默认换帧间隔（2s）
inline constexpr float kOrbitFixedFovDeg = 45.0f; // 固定垂直 FOV（不随交互变化）
inline constexpr double kOrbitFitMargin = 1.06;    // 取景余量（模型外接球外留一点边）
// O 键轨道相机 = 双轴转台（Two-axis Turntable / Azimuth-Elevation Camera）：
//   - 方位角连续旋转（一圈约 18s）；
//   - 俯仰角在 ±30° 之间缓慢上下扫掠，用「三角波 + 端点短缓动」而不是正弦：
//     中段匀速、到端点用很短的加减速折返，避免正弦在端点自然减速后长时间「停在」极限角。
// 周期/摆幅/缓动时长固定在代码里（不开放参数）。
inline constexpr double kOrbitAzimDegPerSec = 20.0;       // 方位角连续旋转速度（度/秒）
inline constexpr double kOrbitElevMaxDeg = 30.0;          // 俯仰扫掠的极限角（±度）
inline constexpr double kOrbitElevHalfPeriodSec = 8.0;    // 俯仰从 -A 到 +A 的时间（秒）
inline constexpr double kOrbitElevRampSec = 0.7;          // 俯仰折返处的短缓动时长（秒）

// 一个会话「静默」多久之后，允许新连接的客户端接管它（秒）。
//
// 场景：客户端进程卡死（例如 X11 转发下 glfwSwapBuffers 卡住）或被 kill -9 以外的原因
// 弄成半死状态时，TCP 连接仍然健康（keepalive 探活会被对端内核 ACK），服务端会一直阻塞
// 在 recv 里，永远回不到 accept —— 用户重启客户端后表现为"再也连不上"。
// 策略：只有当 (a) 监听 socket 上确实有新连接在等，且 (b) 当前会话已经静默超过该秒数时，
// 才丢弃旧会话。这样"用户只是走开了一会儿"的空闲会话不会被误踢。
inline constexpr double kStaleSessionTakeoverSec = 5.0;

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
                     int width, int height, int port, bool useBinarySwap,
                     const std::vector<iGame::StructuredMesh::Pointer>& volumes,
                     const std::vector<iGame::UnsignedCharArray::Pointer>& masks,
                     int numFrames, int startFrame,
                     const std::string& fieldName,
                     double voxelSize = 0.0, double hqStepScale = 1.5,
                     double lqStepScale = 4.0, int lqDivisor = 2,
                     bool usePixelwise = false,
                     const std::vector<double>& frameCentersXYZ = {},
                     const std::vector<double>& frameRadii = {},
                     const std::vector<double>& frameFocusCentersXYZ = {},
                     const std::vector<double>& frameFocusRadii = {},
                     int autoPlayIntervalMs = kDefaultAutoPlayIntervalMs) {
    auto ctx = iGame::ParallelContext::Instance();
    const int rank = ctx->Rank();

    // 多帧播放：当前帧序号（0..numFrames-1）。所有 rank 保持一致（由 frameStep 广播驱动）。
    int curFrame = startFrame;

    // 每帧的全局包围盒（由 ParallelVolumeRendering.cpp 的 --server 分支用 AllReduce 汇总
    // 后传入）。数组为空时退回全局包围盒，因此其它调用方/单帧数据的行为与旧版一致。
    //
    // 两套数组：
    //   frameCentersXYZ/frameRadii       —— **几何**包围盒（分块 bbox 并集）
    //   frameFocusCentersXYZ/frameRadii  —— **可见**包围盒（`scalar >= 空体素剔除阈值`
    //                                        的体素范围，即屏幕上真正看得见的那块）
    // 取景（相机焦点、轨道 pivot、取景/缩放距离）用可见包围盒；近远裁剪面与采样步数
    // 护栏用几何包围盒。原因见下面 focusCenter/focusRadius 的注释。
    auto frameCenter = [&](int f) -> const double* {
        const std::size_t base = static_cast<std::size_t>(f) * 3;
        if (f >= 0 && base + 3 <= frameCentersXYZ.size() &&
            static_cast<std::size_t>(f) < frameRadii.size()) {
            return &frameCentersXYZ[base];
        }
        return gcenter;
    };
    auto frameRadius = [&](int f) -> double {
        if (f >= 0 && static_cast<std::size_t>(f) < frameRadii.size() &&
            frameRadii[static_cast<std::size_t>(f)] > 0.0) {
            return frameRadii[static_cast<std::size_t>(f)];
        }
        return radius;
    };
    // ---- 取景用的「可见包围盒」（与上面几何包围盒并列，二者用途不同）----
    //   · frameCenter/frameRadius（几何）：整块数据的范围。用于近远裁剪面与采样步数护栏
    //     —— 必须覆盖所有 rank 的超块，否则会裁掉数据或把 maxSamples 算小。
    //   · focusCenter/focusRadius（可见）：`scalar >= 空体素剔除阈值` 的体素范围，也就是
    //     屏幕上真正看得见的那一块。用于相机焦点、O 轨道 pivot 与取景距离。
    //     几何包围盒会把"标量恒等于最小值"的透明平台算进去，拿它当焦点就会出现
    //     「模型偏到一边」——这正是本组参数的由来。
    //   数组为空时回退几何值（旧行为），因此其它调用方/单帧数据的表现不变。
    auto focusCenter = [&](int f) -> const double* {
        const std::size_t base = static_cast<std::size_t>(f) * 3;
        if (f >= 0 && base + 3 <= frameFocusCentersXYZ.size() &&
            static_cast<std::size_t>(f) < frameFocusRadii.size()) {
            return &frameFocusCentersXYZ[base];
        }
        return frameCenter(f);
    };
    auto focusRadius = [&](int f) -> double {
        if (f >= 0 && static_cast<std::size_t>(f) < frameFocusRadii.size() &&
            frameFocusRadii[static_cast<std::size_t>(f)] > 0.0) {
            return frameFocusRadii[static_cast<std::size_t>(f)];
        }
        return frameRadius(f);
    };
    // 采样步数护栏取所有帧里最大的包围球半径（切帧不改变步长上限）。
    double maxFrameRadius = radius;
    for (double r : frameRadii) {
        if (r > maxFrameRadius) { maxFrameRadius = r; }
    }

    // 相机轨道状态（仅 rank 0 维护；其余 rank 只接收广播后的绝对相机参数）。
    double azimuth = 0.0;      // 方位角（弧度）
    double elevation = 0.0;    // 仰角（弧度）
    // 取景距离按「可见包围盒」的半径（看得见的内容填满画面，而不是整块几何）。
    double distance = focusRadius(startFrame) * 3.0;

    // O 键 / A 键动画状态（仅 rank 0 读写；每 accept 一个新客户端时重置）。
    bool orbitActive = false;    // 自动轨道相机开
    bool autoPlayActive = false; // 自动播放开
    int autoPlayDir = 1;         // 最近一次 N/P 的方向（+1=NEXT，-1=PREV）；未按过默认 +1
    // 双轴转台状态：方位角连续累加；俯仰角用「半周期时间 + 方向」描述的三角波
    // （orbitElevation 是滤波/缓动后的当前仰角）。*Offset 是轨道期间用户拖动叠加的偏移
    // （允许用户边看动画边手动微调朝向，不破坏「模型中心 + 固定距离」的约束）。
    double orbitAzimuth = 0.0;
    double orbitElevTime = kOrbitElevHalfPeriodSec * 0.5; // 从 0° 开始向上扫掠
    int orbitElevDir = 1;     // +1 = 向上，-1 = 向下
    double orbitElevation = 0.0;
    double orbitAzimOffset = 0.0;
    double orbitElevOffset = 0.0;
    std::chrono::steady_clock::time_point orbitLastTick;
    std::chrono::steady_clock::time_point orbitNextTick;
    std::chrono::steady_clock::time_point autoNextTick;
    // A 键换帧间隔（毫秒）：由 --autoplay-interval 传入，默认 kDefaultAutoPlayIntervalMs。
    const int autoPlayMs = std::max(10, autoPlayIntervalMs);

    // 持久相机参数（每帧由 rank 0 计算并广播）。
    double camPos[3] = {0.0, 0.0, 0.0};
    double camFp[3] = {0.0, 0.0, 0.0};
    double camUp[3] = {0.0, 1.0, 0.0};

    // 触发本帧的 INTERACT 命令序号（rank 0 维护，随帧回传给客户端测 RTT）。
    std::uint32_t frameSeq = 0;
    // 本会话最后一次收到客户端命令的时刻（rank 0 维护）。
    // 用途：判断一个会话是不是"客户端已经卡死/断掉但 TCP 还没报错"的静默会话 ——
    // 有新的客户端在 backlog 里等着时，允许它接管（见 kStaleSessionTakeoverSec）。
    std::chrono::steady_clock::time_point sessionLastCmdTime =
            std::chrono::steady_clock::now();

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
                    // backlog 从 1 提到 8：旧值 1 时，上一个会话的 TIME_WAIT / 半死连接
                    // 可能让新连接进不来（SYN 被丢），表现为"断联后再也连不上"。
                    listen(listenSock, 8);
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
    distance = focusRadius(startFrame) * 3.0;
    frameSeq = 0;
    curFrame = startFrame;
    orbitActive = false;    // O 键状态每个会话重新开始
    autoPlayActive = false; // A 键状态每个会话重新开始
    autoPlayDir = 1;        // 本会话还没按过 N/P → 默认向后播放
    orbitAzimuth = 0.0;   // 方位角从正面开始连续旋转
    orbitElevTime = kOrbitElevHalfPeriodSec * 0.5; // 仰角从 0° 开始向上扫掠
    orbitElevDir = 1;
    orbitElevation = 0.0;
    orbitAzimOffset = 0.0;
    orbitElevOffset = 0.0;
    // 新客户端回到起始帧：把光线步进器输入切回起始帧（上一客户端可能切到过别的帧）。
    if (numFrames > 1) {
        rayCaster->SetInput(volumes[static_cast<size_t>(startFrame)]);
        rayCaster->SetValidMask(masks[static_cast<size_t>(startFrame)]);
    }
    netbuf.clear();
    clientSock = PV_INVALID_SOCKET;
    // 新会话的静默计时从这里开始（见 kStaleSessionTakeoverSec）。
    sessionLastCmdTime = std::chrono::steady_clock::now();

    // 接受客户端（rank 0 阻塞）并发送握手元数据。
    //
    // accept 的瞬时错误（EINTR / ECONNABORTED / EMFILE / ENFILE / ENOBUFS...）**不能**
    // 直接让整个 MPI 作业退出：旧行为是「accept 一失败就 serverStatus=0 → break → 退出
    // 服务循环」，于是客户端断联一次之后只要撞上一次瞬时错误，服务就永久消失（前端再也
    // 连不上）。这里改成有界重试（最多 5s），只有持续失败才退出，并把 errno 打出来。
    serverStatus = 1;
    if (rank == 0) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int alen = sizeof(clientAddr);
#else
        socklen_t alen = sizeof(clientAddr);
#endif
        clientSock = PV_INVALID_SOCKET;
        for (int attempt = 0; attempt < 50; ++attempt) {
            alen = static_cast<decltype(alen)>(sizeof(clientAddr));
            clientSock = accept(listenSock,
                                reinterpret_cast<sockaddr*>(&clientAddr), &alen);
            if (clientSock != PV_INVALID_SOCKET) { break; }
            const int e = iGamePVNet::LastError();
            std::cerr << "[server] accept error (errno=" << e << ", attempt "
                      << (attempt + 1) << "/50), retrying...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (clientSock == PV_INVALID_SOCKET) {
            std::cerr << "[server] accept failed repeatedly; shutting the service "
                         "loop down (this ends the MPI job)\n";
            serverStatus = 0;
        } else {
            std::cout << "[server] client connected\n";
            std::cout.flush();

            // 交互 socket 调优：关 Nagle + 开 keepalive + send 超时（见
            // iGamePVNet::ConfigureInteractiveSocket 的注释：这是「断联后无法重连」与
            // 「两侧互相 send 卡死」的根本防护）。
            iGamePVNet::ConfigureInteractiveSocket(clientSock);

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
        const double n =
                std::ceil(2.0 * maxFrameRadius / static_cast<double>(step)) + 16.0;
        return static_cast<int>(std::clamp(n, 8.0, 8192.0));
    };
    const int hqMaxSamples = hasGlobalStep ? sampleCap(hqStep) : 512;
    const int lqMaxSamples = hasGlobalStep ? sampleCap(lqStep) : 128;

    // O 键轨道相机的固定取景距离：把「当前帧模型包围球」完整放进视锥所需的最小距离
    // （d = R / sin(半视场角)），再留 kOrbitFitMargin 的边。
    // 关键点：
    //   - FOV 固定在 kOrbitFixedFovDeg（垂直 45°），水平半角由宽高比推出，取二者更紧的一个
    //     —— 任意方位角下模型都不会出画；
    //   - 距离在一次开启时算好并保持，轨道过程中不缩放（滚轮被忽略）；
    //   - 半径用「可见包围盒」（focusRadius）：看得见的内容填满画面。用几何半径的话，
    //     透明平台会把半径撑大，模型在画面里显得又小又偏。
    auto FitOrbitDistance = [&](int f) -> double {
        const double fr = focusRadius(f);
        const double aspect = static_cast<double>(std::max(1, hqW)) /
                              static_cast<double>(std::max(1, hqH));
        const double halfV = 0.5 * static_cast<double>(kOrbitFixedFovDeg) *
                             (3.14159265358979323846 / 180.0);
        const double halfH = std::atan(std::tan(halfV) * aspect);
        const double half = std::min(halfV, halfH);
        const double s = std::sin(std::max(half, 1e-6));
        const double d = fr / s * kOrbitFitMargin;
        return std::max(d, fr * 1.001); // 保证近裁剪面落在模型之前
    };

    // 俯仰角推进：三角波 + 端点短缓动。
    // 每个半周期 H 内从 -A 扫到 +A（或反向），速度曲线是「加速 R → 匀速 → 减速 R」的梯形：
    // 中段匀速，折返处只有 R 秒的短缓动；不会像正弦那样在极限角附近自然减速、长时间停住。
    auto AdvanceOrbitElevation = [&](double dt) {
        const double kPi = 3.14159265358979323846;
        const double A = kOrbitElevMaxDeg * (kPi / 180.0);
        const double H = std::max(kOrbitElevHalfPeriodSec, 1e-3);
        const double R = std::clamp(kOrbitElevRampSec, 1e-3, H * 0.25);
        const double v = 2.0 * A / (H - R); // 匀速段角速度
        const double a = v / R;             // 缓动段角加速度
        orbitElevTime += dt;
        while (orbitElevTime >= H) {
            orbitElevTime -= H;
            orbitElevDir = -orbitElevDir;
        }
        const double t = orbitElevTime;
        double swept = 0.0; // 本半周期已扫过的角度（0..2A）
        if (t < R) {
            swept = 0.5 * a * t * t;
        } else if (t < H - R) {
            swept = 0.5 * v * R + v * (t - R);
        } else {
            const double remain = H - t;
            swept = 2.0 * A - 0.5 * a * remain * remain;
        }
        const double e = (orbitElevDir > 0) ? (-A + swept) : (A - swept);
        orbitElevation = std::clamp(e, -1.5, 1.5);
    };

    auto renderFrame = [&](int interactive) -> bool {
        // 交互档真的降分辨率（对标 MiniPVServer kW_LQ=256 / kW_HQ=800）：只做像素步进
        // 不减少网络载荷与合成量，而降分辨率对「渲染 + 合成 + 传输」三者同时生效。
        const int fw = interactive ? lqW : hqW;
        const int fh = interactive ? lqH : hqH;
        camera->SetViewPort(fw, fh);
        // 固定 FOV（所有 rank 一致；O 键取景距离就是按这个 FOV 算的）。
        camera->SetFov(kOrbitFixedFovDeg);

        // 当前帧的**几何**包围盒：只用于近远裁剪面（必须覆盖所有 rank 的超块，
        // 否则会裁掉数据）。相机焦点不在这里 —— 它由上面按「可见包围盒」广播的 camFp 决定。
        const double* fc = frameCenter(curFrame);
        const double fr = frameRadius(curFrame);

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
        const double toCenter[3] = {fc[0] - camPos[0], fc[1] - camPos[1],
                                    fc[2] - camPos[2]};
        const double dist = toCenter[0] * front[0] + toCenter[1] * front[1] +
                            toCenter[2] * front[2];
        double nearPlane = dist - fr;
        double farPlane = dist + fr;
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
        // valid_pixels_viewport）。--binary-swap 时改走 binary-swap 合成（对标 IceT
        // icetBSwapCompose，通信与合成摊到所有 rank）。
        // --pixelwise 则换用「逐像素首命中深度排序」的合成路径（ParallelVolumePixelComposite，
        // 实现放在 pvr 模块内，不改 iGameCore）；与上面两个策略正交组合。
        auto composite = iGame::iGameCompositePass::New();
        iGamePVPixel::PixelCompositePass pixelComposite;
        int resROIX = 0;
        int resROIY = 0;
        int resROIW = 0;
        int resROIH = 0;
        const std::vector<unsigned char>* resROIRGBA = nullptr;
        const double blockDepth = iGame::iGameCompositePass::ComputeBlockDepth(
                blockCenter, camPos, front);
        const auto tComposite0 = std::chrono::steady_clock::now();
        bool compositeOk = false;
        if (usePixelwise) {
            pixelComposite.SetLocalImage(fw, fh, rgba, depth);
            pixelComposite.SetBlockDepth(blockDepth);
            pixelComposite.SetBackgroundColor(0.0f, 0.0f, 0.0f);
            pixelComposite.SetUseBinarySwapComposite(useBinarySwap);
            // C/S 帧只发「预乘 alpha + 透明背景」的 ROI，客户端叠到自己的渐变背景上。
            pixelComposite.SetTransparentROI(true);
            compositeOk = pixelComposite.Composite();
            resROIX = pixelComposite.GetResultROIX();
            resROIY = pixelComposite.GetResultROIY();
            resROIW = pixelComposite.GetResultROIW();
            resROIH = pixelComposite.GetResultROIH();
            resROIRGBA = &pixelComposite.GetResultROIRGBA();
        } else {
            composite->SetLocalImage(fw, fh, rgba, depth);
            composite->SetBlockDepth(blockDepth);
            composite->SetBackgroundColor(0.0f, 0.0f, 0.0f);
            composite->SetUseBinarySwapComposite(useBinarySwap);
            // C/S 帧只发「预乘 alpha + 透明背景」的 ROI，客户端叠到自己的渐变背景上。
            composite->SetTransparentROI(true);
            compositeOk = composite->Composite();
            resROIX = composite->GetResultROIX();
            resROIY = composite->GetResultROIY();
            resROIW = composite->GetResultROIW();
            resROIH = composite->GetResultROIH();
            resROIRGBA = &composite->GetResultROIRGBA();
        }
        const auto tComposite1 = std::chrono::steady_clock::now();
        if (!compositeOk) {
            if (rank == 0) { std::cerr << "[server] composite failed\n"; }
            return false;
        }

        // rank 0 发送合成图；其余 rank 无事可做。
        if (rank == 0) {
            const auto tSend0 = std::chrono::steady_clock::now();

            // 1) ROI 由合成器直接给出（稀疏路径在合成时就知道了），无需再扫一遍全图。
            //    全图路径（binary-swap）下 Composite() 收尾时也已扫过一次，语义一致。
            const int rx0 = resROIX;
            const int ry0 = resROIY;
            const int roiW = resROIW;
            const int roiH = resROIH;
            const std::vector<unsigned char>& roi = *resROIRGBA;

            // 2) 交互档背压：TCP 发送队列积压超过阈值说明客户端消费不过来，此时继续发
            //    只会让延迟无界增长（「松手后追帧」）。宁可丢帧（返回 true，客户端会话
            //    保持），等队列排空后再发最新帧。（对标 MiniPVServer.cpp:921-932 的
            //    ioctl(TIOCOUTQ) 丢帧策略；仅 Linux 可用，其它平台跳过。）
            bool queueBacklogged = false;
#if defined(__linux__)
            // 拖动档与 O/A 动画都可能是「服务端出帧快于客户端消费」，同样要丢帧背压：
            // 动画期间即使不是低清档，只要发送队列积压就丢弃本帧，避免 RTT 无界增长。
            if (interactive || orbitActive || autoPlayActive) {
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
    // 焦点用「可见包围盒」中心（看得见的内容才在画面正中）；几何中心会把不可见的
    // 标量最小值平台算进去，也正是"模型偏到一边"的成因。
    if (rank == 0) {
        const double* fc0 = focusCenter(curFrame);
        for (int d = 0; d < 3; ++d) {
            camFp[d] = fc0[d];
            camPos[d] = fc0[d] + (d == 2 ? distance : 0.0);
            camUp[d] = (d == 1 ? 1.0 : 0.0);
        }
    }
    ctx->Broadcast(camPos, 3, 0);
    ctx->Broadcast(camFp, 3, 0);
    ctx->Broadcast(camUp, 3, 0);
    // 诊断：两套包围盒都打出来 —— 两者的中心差就是"几何中心 ≠ 看得见的中心"的那段偏移。
    if (rank == 0) {
        const double* pc = frameCenter(curFrame);
        const double* fp = focusCenter(curFrame);
        std::cerr << "[server] camera pivot (frame " << curFrame << ") = ("
                  << fp[0] << ", " << fp[1] << ", " << fp[2]
                  << "), radius=" << focusRadius(curFrame) << "  [visible]\n";
        std::cerr << "[server] geometry center = (" << pc[0] << ", " << pc[1]
                  << ", " << pc[2] << "), radius=" << frameRadius(curFrame)
                  << "  [clipping only]\n";
    }
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
        int frameStep = 0;      // 本批 NEXT/PREV 累积的帧步进（+1/-1）

        if (rank == 0) {
            // 0) 等命令。**空闲时也不再无限阻塞**（旧代码 waitMs = -1）：改成最多等 1s，
            //    这样本轮超时后还能做两件事：
            //      (a) 看看监听 socket 上是否有新客户端在 backlog 里等着接管一个已经
            //          **静默**的旧会话（客户端进程卡死但 TCP 还活着时，这是唯一能
            //          自己恢复的路径，否则服务端永远回不到 accept）；
            //      (b) 让 keepalive / TCP_USER_TIMEOUT 有机会把死连接报上来。
            //    动画期间仍按动画节拍等（O/A 的节奏与客户端显示循环无关）。
            int waitMs = 1000;
            if (orbitActive || autoPlayActive) {
                const auto now = std::chrono::steady_clock::now();
                const auto toMs = [](std::chrono::steady_clock::duration d) {
                    return std::chrono::duration_cast<std::chrono::milliseconds>(d)
                            .count();
                };
                long long remainMs = 60000;
                if (orbitActive) {
                    remainMs = std::min<long long>(remainMs,
                                                   toMs(orbitNextTick - now));
                }
                if (autoPlayActive) {
                    remainMs = std::min<long long>(remainMs,
                                                   toMs(autoNextTick - now));
                }
                waitMs = static_cast<int>(
                        std::clamp<long long>(remainMs, 0, 60000));
            }
            bool timedOut = false;
            if (clientAlive && netbuf.find('\n') == std::string::npos &&
                waitMs >= 0) {
                const int ready = iGamePVNet::WaitReadable(clientSock, waitMs);
                if (ready < 0) {
                    std::cerr << "[server] client disconnected\n";
                    clientAlive = false;
                } else if (ready == 0) {
                    timedOut = true; // 到点：本轮没有新命令，交给动画推进
                }
            }
            // 1) 阻塞读到至少一行命令。
            if (clientAlive && !timedOut) {
                while (netbuf.find('\n') == std::string::npos) {
                    std::string line;
                    if (!iGamePVNet::RecvLine(clientSock, line)) {
                        std::cerr << "[server] client disconnected\n";
                        clientAlive = false;
                        break;
                    }
                    netbuf += line;
                }
            }
            // 2) 非阻塞排空，尽量合并更多命令。
            if (clientAlive) {
                iGamePVNet::DrainNonBlock(clientSock, netbuf);
                // 3) 解析所有完整行，合并成一次相机更新 + 一次渲染。
                std::string line;
                while (clientAlive && iGamePVNet::PopLine(netbuf, line)) {
                    sessionLastCmdTime = std::chrono::steady_clock::now();
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
                    // O 键：自动轨道相机（Toggle Orbit Camera）。
                    if (op == "ORBIT") {
                        int on = 0;
                        if (!(iss >> on)) { continue; }
                        const bool want = (on != 0);
                        if (want != orbitActive) {
                            orbitActive = want;
                            if (orbitActive) {
                                // 「自动将镜头对准被渲染模型」：复位到正面视角，焦点=当前帧
                                // 包围盒中心，距离=按固定 FOV 算好的取景距离（此后不再缩放）；
                                // 转台从「正面、仰角 0°、向上扫掠」开始。
                                azimuth = 0.0;
                                elevation = 0.0;
                                orbitAzimuth = 0.0;
                                orbitElevTime = kOrbitElevHalfPeriodSec * 0.5;
                                orbitElevDir = 1;
                                orbitElevation = 0.0;
                                orbitAzimOffset = 0.0;
                                orbitElevOffset = 0.0;
                                distance = FitOrbitDistance(curFrame);
                                orbitLastTick = std::chrono::steady_clock::now();
                                orbitNextTick = orbitLastTick +
                                                std::chrono::milliseconds(
                                                        kOrbitIntervalMs);
                                cameraChanged = true;
                                const double* oc = focusCenter(curFrame);
                                std::cerr << "[server] orbit camera on (two-axis turntable, azimuth "
                                          << kOrbitAzimDegPerSec << " deg/s, elevation +/-"
                                          << kOrbitElevMaxDeg << " deg triangle, fixed fov "
                                          << kOrbitFixedFovDeg << " deg, pivot=(" << oc[0]
                                          << ", " << oc[1] << ", " << oc[2]
                                          << "), radius=" << focusRadius(curFrame)
                                          << ", distance=" << distance << ")\n";
                            } else {
                                std::cerr << "[server] orbit camera off\n";
                            }
                            needRender = true;
                        }
                        continue;
                    }
                    // A 键：自动播放（Toggle Auto Play）。方向取最近一次 N/P，默认向后。
                    if (op == "AUTOPLAY") {
                        int on = 0;
                        if (!(iss >> on)) { continue; }
                        const bool want = (on != 0) && (numFrames > 1);
                        if (want != autoPlayActive) {
                            autoPlayActive = want;
                            if (autoPlayActive) {
                                autoNextTick = std::chrono::steady_clock::now() +
                                               std::chrono::milliseconds(autoPlayMs);
                                std::cerr << "[server] auto play on (direction="
                                          << (autoPlayDir > 0 ? "next" : "prev")
                                          << ", interval=" << autoPlayMs
                                          << "ms)\n";
                            } else {
                                std::cerr << "[server] auto play off\n";
                            }
                        }
                        continue;
                    }
                    if (op == "NEXT") {
                        frameStep += 1;
                        autoPlayDir = 1;
                        autoPlayActive = false; // 播放中按 N/P：自动 toggle off
                        needRender = true;
                        continue;
                    }
                    if (op == "PREV") {
                        frameStep -= 1;
                        autoPlayDir = -1;
                        autoPlayActive = false;
                        needRender = true;
                        continue;
                    }
                    std::cerr << "[server] unknown cmd: " << line << '\n';
                }
            }

            // 4) 动画推进：转台按「距上一轮的真实时间」推进（与帧率无关）——方位角连续
            //    旋转、俯仰角按三角波扫掠；自动播放到点推进一帧（不追补多帧）。
            const auto now = std::chrono::steady_clock::now();
            if (orbitActive && now >= orbitNextTick) {
                const double dt = std::chrono::duration<double>(now - orbitLastTick)
                                          .count();
                orbitLastTick = now;
                orbitNextTick = now + std::chrono::milliseconds(kOrbitIntervalMs);
                const double dts = std::clamp(dt, 0.0, 0.5);
                const double kTwoPi = 6.28318530717958647692;
                orbitAzimuth =
                        std::fmod(orbitAzimuth +
                                          kOrbitAzimDegPerSec * (kTwoPi / 360.0) * dts,
                                  kTwoPi);
                AdvanceOrbitElevation(dts);
                cameraChanged = true;
                needRender = true;
            }
            if (autoPlayActive && now >= autoNextTick) {
                frameStep += autoPlayDir;
                autoNextTick = now + std::chrono::milliseconds(autoPlayMs);
                needRender = true;
            }

            // 4.5) 静默会话接管：本轮没有任何命令、也没有动画在跑，而当前会话已经静默
            //      超过 kStaleSessionTakeoverSec，且监听 socket 上有新连接在等 —— 说明旧
            //      客户端八成已经卡死（TCP 还活着，keepalive 检测不到）。此时丢弃旧会话，
            //      让服务端回到 accept，新客户端立刻就能连上（"断联后无法重连"的真正解药）。
            if (clientAlive && !needRender && !orbitActive && !autoPlayActive) {
                const double idleSec =
                        std::chrono::duration<double>(now - sessionLastCmdTime).count();
                if (idleSec > kStaleSessionTakeoverSec) {
                    fd_set lfds;
                    FD_ZERO(&lfds);
                    FD_SET(listenSock, &lfds);
                    timeval zero{0, 0};
                    const int pending = select(static_cast<int>(listenSock) + 1, &lfds,
                                               nullptr, nullptr, &zero);
                    if (pending > 0) {
                        std::cerr << "[server] idle session silent for " << idleSec
                                  << "s and a new client is waiting -> dropping the "
                                     "stale session so the new one can connect\n";
                        clientAlive = false;
                    }
                }
            }

            // O 键轨道动画统一按「拖动档」渲染。**必须**在广播 interactiveMode 之前设置：
            // 所有 rank 的渲染分辨率必须一致（interactiveMode 是唯一广播的 LOD 开关）；
            // 若只在 rank 0 用 orbitActive 本地改 interactive，rank 0 会渲染 512²、其余
            // rank 仍渲染 1024²，合成时尺寸不一致（--pixelwise 路径甚至会越界写 → 崩溃）。
            if (orbitActive) { interactiveMode = 1; }
        }

        // 广播合并后的命令标志（注意：Broadcast 只接受非 const 指针，这里用非 const int）。
        int needRenderFlag = needRender ? 1 : 0;
        int cameraChangedFlag = cameraChanged ? 1 : 0;
        ctx->Broadcast(&needRenderFlag, 1, 0);
        ctx->Broadcast(&cameraChangedFlag, 1, 0);
        ctx->Broadcast(&interactiveMode, 1, 0);
        ctx->Broadcast(&frameStep, 1, 0);

        if (!needRenderFlag) { continue; }

        // LOD 档位：完全由广播后的 interactiveMode 决定（拖动 / O 轨道动画 → 低清档），
        // 保证所有 rank 用同一分辨率（否则合成尺寸不一致）。
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
            if (orbitActive) {
                // 转台动画期间：拖动只作为整体偏移，不直接改方位/仰角。
                orbitAzimOffset += accAzim;
                orbitElevOffset =
                        std::clamp(orbitElevOffset + accElev, -0.8, 0.8);
            } else {
                azimuth += accAzim;
                elevation += accElev;
                elevation = std::clamp(elevation, -1.55, 1.55);
                // 普通模式：滚轮缩放照旧。轨道相机期间锁定取景距离（禁止缩放）。
                // 缩放范围同样按「可见包围盒」半径算，才与初始取景距离同一量纲。
                const double diag = focusRadius(curFrame) * 2.0;
                distance *= accZoom;
                distance = std::clamp(distance, diag * 0.05, diag * 20.0);
            }
        }
        if (rank == 0) {
            if (orbitActive) {
                // 双轴转台：方位角连续旋转、俯仰角三角波扫掠（均已按时间推进）。
                // 距离每次按「当前帧」的可见包围盒重算，保证切帧/任意角度下看得见的内容都在视锥内。
                const double kTwoPi = 6.28318530717958647692;
                azimuth =
                        std::fmod(orbitAzimuth + orbitAzimOffset, kTwoPi);
                elevation = std::clamp(orbitElevation + orbitElevOffset, -1.5, 1.5);
                distance = FitOrbitDistance(curFrame);
            }
            // 焦点 = 可见包围盒中心（所有视角/轨道都绕它转）。
            const double* fc = focusCenter(curFrame);
            const double dir[3] = {
                    std::cos(elevation) * std::sin(azimuth),
                    std::sin(elevation),
                    std::cos(elevation) * std::cos(azimuth),
            };
            for (int d = 0; d < 3; ++d) {
                camFp[d] = fc[d];
                camPos[d] = fc[d] + dir[d] * distance;
                camUp[d] = (d == 1 ? 1.0 : 0.0);
            }
        }
        ctx->Broadcast(camPos, 3, 0);
        ctx->Broadcast(camFp, 3, 0);
        ctx->Broadcast(camUp, 3, 0);

        // 发送失败（客户端断开）不在此处直接 break，而是标记 clientAlive=false，
        // 由下一轮顶部的 Broadcast 统一通知所有 rank 退出，避免失步死锁。
        if (!renderFrame(interactive)) {
            // 打一行原因：区分「客户端真的走了」还是「send 超时/TCP 判定失效」
            // （后者在超算上意味着 keepalive / SO_SNDTIMEO 生效，是预期行为）。
            std::cerr << "[server] frame send failed -> ending this session "
                         "(peer gone or send timeout)\n";
            clientAlive = false;
        }
    }

    // 客户端断开/EXIT/发送失败：关闭本会话 socket，回到 accept 等下一个客户端（不退出服务）。
    if (rank == 0) {
        iGamePVNet::CloseSocket(clientSock);
        clientSock = PV_INVALID_SOCKET;
        std::cout << "[server] client session ended; waiting for next client...\n";
        std::cout.flush();
        // 只有看到这一行，才说明服务端**真的**回到了 accept；若断联后日志停在上一条
        // [server] 帧行而没有这一行，说明服务端卡在读/写里（keepalive 尚未生效或还没到点）。
    }
    } // 持久服务循环

    if (rank == 0) {
        iGamePVNet::CloseSocket(listenSock);
    }
    return 0;
}

} // namespace iGamePVServer
