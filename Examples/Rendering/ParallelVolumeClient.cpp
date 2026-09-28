// ParallelVolumeClient.cpp — 并行体绘制 C/S 前端（阶段 6）。
//
// 对标 UnifiedVersion 的 MiniPVClient.cpp，但不依赖 Qt/VTK：本程序用 GLFW + glad +
// iGame::RenderWindow 复刻阶段 5 的交互窗口，连接服务端（ParallelVolumeServer.h 里的
// RunServer，跑在超算计算节点）后，接收合成帧并显示，同时把鼠标交互（左键拖动旋转、
// 滚轮缩放）以增量命令（INTERACT）回传给服务端。
//
// 用法：
//   testParallelVolumeClient [host] [port]
//     默认 host=127.0.0.1、port=11111。跨机连接时 host 填服务端所在节点/登录节点，
//     必要时用 ssh -L 端口转发把远程端口映射到本地。
//
// 【本次修复的核心问题：网络 I/O 与显示循环耦合】
//   旧实现把「收帧」放在 GLFW 显示循环里，而且每轮只读一次 8192 字节：
//       while (!close) { glfwPollEvents(); sendPending(); RecvAvailable(8192B); draw(); swap(); }
//   于是「接收带宽 = 8192 字节 × 显示循环频率」。在软件 GL / X11 转发环境下显示循环只有
//   2~3 Hz，接收带宽被限到 ~20 KB/s——服务端一帧（合成图 ROI 经 zlib 后几十~几百 KB）
//   要好几秒才能读完，实测客户端 frames 只有 0.4 Hz；更糟的是服务端仍按命令速率出帧，
//   帧在 TCP 缓冲里无界堆积，客户端测到的 RTT 单调增长到几万毫秒（松手后要「追帧」很久）。
//
//   修复：把网络收发整体搬到独立的 I/O 线程——
//     * I/O 线程每轮用 select + recv 循环「把 socket 里可读的数据全部读完」（全量排空），
//       接收速率不再受显示循环限制；
//     * I/O 线程里一次把积压的交互命令合并发出去，命令时延也不再受显示循环限制；
//     * 一批里解析出多帧时只保留最新一帧（旧帧直接丢），保证显示的是最新状态；
//     * 主线程只负责 glfwPollEvents + 上屏，帧到达即取最新帧显示。
//   这与服务端的「发送队列积压则丢帧」（ParallelVolumeServer.h kMaxSendQueueBytes）
//   形成完整背压闭环。
//
// 说明：
//   - 客户端只做「显示 + 输入」，不做任何渲染/合成，也不依赖 MPI；
//   - 左下角 colorbar（含全局标量范围）由服务端握手时下发的 ramp 重建；
//   - 拖动旋转期间显示 fps（本地接收帧率）与 RTT（命令 seq 往返）；
//   - 复用阶段 5（ParallelVolumeInteractive.h）的 OpenGL 显示辅助代码（纹理四边形、
//     colorbar、5x7 位图字体），保证前后端观感一致。
// 注意：协议头必须先包含（其内部先于 windows.h 引入 winsock2），再包含 glad/windows.h。
#include "ParallelVolumeProtocol.h"

#include "iGameRenderWindow.h"
#include "ParallelVolumeInteractive.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <zlib.h>

using namespace iGameVolInteractive; // 复用阶段 5 的显示辅助（InitDisplay/DrawColorBar/...）

namespace {

// ---------------------------------------------------------------------------
// 交互状态
// ---------------------------------------------------------------------------
// 仅主线程访问（GLFW 回调 + 主循环）。
bool g_cliDragging = false;
double g_cliLastX = 0.0;
double g_cliLastY = 0.0;
std::atomic<bool> g_cliInteracting{false}; // I/O 线程发命令时要读

// 增量交互累积量：主线程（GLFW 回调）写，I/O 线程读并清零 —— 由 g_ioMutex 保护。
std::mutex g_ioMutex;
double g_cliPendingAzim = 0.0; // 待发送的轨道方位角增量（弧度）
double g_cliPendingElev = 0.0; // 待发送的仰角增量（弧度）
double g_cliPendingZoom = 1.0; // 待发送的距离缩放因子（累积）
bool g_cliModeDirty = false;   // 交互状态变化需立刻发送一帧
int g_cliPendingFrameStep = 0; // 待发送的切帧步进（N=+1 / P=-1，累积）

// RTT 测量：seq -> 发送时刻（服务端在回帧里原样带回 seq）。由 g_ioMutex 保护。
std::uint32_t g_cliSeq = 0;
std::map<std::uint32_t, std::chrono::steady_clock::time_point> g_sendTime;

// I/O 线程与主线程之间的共享状态。
std::atomic<bool> g_ioStop{false};
std::atomic<bool> g_serverGone{false};
std::atomic<double> g_rttMs{-1.0};   // 最近一次 RTT（毫秒）；-1 表示尚未测量
std::atomic<std::uint64_t> g_rxFrames{0};    // I/O 线程收到的完整帧数
std::atomic<std::uint64_t> g_rxDropped{0};   // 因「只保留最新帧」被丢弃的帧数
std::atomic<std::uint64_t> g_rxBytes{0};     // 收到的 payload 总字节数
std::mutex g_frameMutex;
iGamePVNet::Frame g_latestFrame;   // I/O 线程写，主线程取
bool g_latestFrameReady = false;
std::uint64_t g_latestFrameId = 0; // 单调递增，主线程用来判断是否是新帧

// 显示循环四段耗时累加器（仅主线程访问），每 5s 打印一次后清零。
double g_diagPollMs = 0.0;   // glfwPollEvents
double g_diagUploadMs = 0.0; // 纹理上传（仅新帧）
double g_diagDrawMs = 0.0;   // 绘制（quad + colorbar + 文字），不含换缓冲
double g_diagSwapMs = 0.0;   // glfwSwapBuffers（等 vblank / X11 传输）

// 当前 g_imageTex 的尺寸（帧分辨率变化时重新分配纹理）。仅主线程访问。
int g_imgTexW = 0;
int g_imgTexH = 0;

// 用服务端下发的 colorbar ramp 重建渐变纹理（仅颜色，alpha 恒 1）。
void BuildColorbarTextureFromRamp(const unsigned char* ramp) {
    glBindTexture(GL_TEXTURE_2D, g_colorbarTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iGamePVNet::kColorbarSize, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, ramp);
}

// 帧分辨率变化时（重）分配合成图纹理（服务端拖动档会降分辨率，1024 -> 512）。
void EnsureImageTexture(int w, int h) {
    if (w == g_imgTexW && h == g_imgTexH) { return; }
    glBindTexture(GL_TEXTURE_2D, g_imageTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    g_imgTexW = w;
    g_imgTexH = h;
}

// 鼠标回调：左键拖动 = 旋转，滚轮 = 缩放（增量口径与阶段 5 的 0.006 rad/px 一致）。
void ClientMouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) { return; }
    if (action == GLFW_PRESS) {
        g_cliDragging = true;
        g_cliInteracting.store(true);
        std::lock_guard<std::mutex> lk(g_ioMutex);
        g_cliModeDirty = true;
    } else if (action == GLFW_RELEASE) {
        g_cliDragging = false;
        g_cliInteracting.store(false);
        std::lock_guard<std::mutex> lk(g_ioMutex);
        g_cliModeDirty = true;
    }
}

void ClientCursorPosCallback(GLFWwindow*, double x, double y) {
    if (!g_cliDragging) {
        g_cliLastX = x;
        g_cliLastY = y;
        return;
    }
    const double dx = x - g_cliLastX;
    const double dy = y - g_cliLastY;
    g_cliLastX = x;
    g_cliLastY = y;
    std::lock_guard<std::mutex> lk(g_ioMutex);
    g_cliPendingAzim += -dx * 0.006; // 阶段 5：azimuth -= dx*0.006
    g_cliPendingElev += dy * 0.006;  // 阶段 5：elevation += dy*0.006
}

void ClientScrollCallback(GLFWwindow*, double, double yoffset) {
    std::lock_guard<std::mutex> lk(g_ioMutex);
    g_cliPendingZoom *= (yoffset > 0.0) ? 0.9 : 1.1;
}

// 键盘回调：N = 下一帧，P = 上一帧（多帧播放，越界由服务端回绕）。
void ClientKeyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) { return; }
    std::lock_guard<std::mutex> lk(g_ioMutex);
    if (key == GLFW_KEY_N) { g_cliPendingFrameStep += 1; }
    else if (key == GLFW_KEY_P) { g_cliPendingFrameStep -= 1; }
}

// 发送当前累积的 INTERACT 命令（带单调递增 seq 用于 RTT 测量），成功后清零 pending。
// 调用方必须持有 g_ioMutex。
bool SendInteractLocked(PVSocket s) {
    const std::uint32_t seq = ++g_cliSeq;
    const std::string cmd = iGamePVNet::MakeInteractCommand(
            seq, g_cliPendingAzim, g_cliPendingElev, g_cliPendingZoom,
            g_cliInteracting.load() ? 1 : 0);
    if (!iGamePVNet::SendAll(s, cmd.data(), cmd.size())) { return false; }
    g_sendTime[seq] = std::chrono::steady_clock::now();
    if (g_sendTime.size() > 1024) { g_sendTime.clear(); } // 防泄漏
    g_cliPendingAzim = 0.0;
    g_cliPendingElev = 0.0;
    g_cliPendingZoom = 1.0;
    g_cliModeDirty = false;
    return true;
}

// 把一篇 payload 还原成整帧 RGBA（ROI 之外保持黑背景）。返回 false 表示解码失败。
bool DecodeFrameRGBA(iGamePVNet::Frame& f) {
    std::vector<unsigned char> sub;
    if (f.roiW > 0 && f.roiH > 0) {
        if (f.codec == iGamePVNet::kCodecZlib) {
            sub.resize(static_cast<std::size_t>(f.roiW) * f.roiH * 4);
            uLongf destLen = static_cast<uLongf>(sub.size());
            const int zr = uncompress(sub.data(), &destLen, f.payload.data(),
                                      static_cast<uLong>(f.payload.size()));
            if (zr != Z_OK || destLen != sub.size()) { return false; }
        } else {
            sub = std::move(f.payload); // raw RGBA8（roiW*roiH*4）
        }
    }
    f.rgba.assign(static_cast<std::size_t>(f.width) * f.height * 4, 0);
    for (int y = 0; y < f.roiH; ++y) {
        const std::size_t dstBase =
                (static_cast<std::size_t>(f.roiY + y) * f.width + f.roiX) * 4;
        const std::size_t srcBase = static_cast<std::size_t>(y) * f.roiW * 4;
        std::memcpy(f.rgba.data() + dstBase, sub.data() + srcBase,
                    static_cast<std::size_t>(f.roiW) * 4);
    }
    return true;
}

// ---------------------------------------------------------------------------
// 网络 I/O 线程：独占 socket 的收发，与显示循环完全解耦。
//
// 关键点（修复「RTT 涨到万毫秒」）：
//   1) 全量排空：select + recv 循环一直读到无数据可读为止，而不是每轮只读 8KB；
//   2) 立即发命令：鼠标一动就把积压的增量合并成一条 INTERACT 发出去，不等显示循环；
//   3) 只留最新帧：一次排空里解析出多帧时，只保留最后一张（旧帧丢弃并计数），
//      这样即使服务端出帧快于显示，客户端显示的也永远是最新状态，不会「追帧」。
// ---------------------------------------------------------------------------
void IoThreadMain(PVSocket sock) {
    std::vector<char> rxBuf;
    rxBuf.reserve(1 << 20);

    while (!g_ioStop.load()) {
        // ---- 1) 发送积压的交互命令（合并成一条）----
        {
            std::lock_guard<std::mutex> lk(g_ioMutex);
            const bool hasPending =
                    std::abs(g_cliPendingAzim) > 1e-12 ||
                    std::abs(g_cliPendingElev) > 1e-12 ||
                    std::abs(g_cliPendingZoom - 1.0) > 1e-12 || g_cliModeDirty;
            if (hasPending && !SendInteractLocked(sock)) {
                g_serverGone.store(true);
                break;
            }
        }

        // ---- 1b) 发送切帧命令（N/P 累积的步进，每次一条 ±1）----
        {
            std::lock_guard<std::mutex> lk(g_ioMutex);
            while (g_cliPendingFrameStep != 0) {
                const int step = g_cliPendingFrameStep > 0 ? 1 : -1;
                g_cliPendingFrameStep -= step;
                const std::string cmd = iGamePVNet::MakeFrameStepCommand(step);
                if (!iGamePVNet::SendAll(sock, cmd.data(), cmd.size())) {
                    g_serverGone.store(true);
                    break;
                }
            }
        }

        // ---- 2) 等可读（1ms 超时，保证命令发送的时延上限）----
        {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            timeval tv{0, 1000};
            const int ready = select(static_cast<int>(sock) + 1, &rfds, nullptr,
                                     nullptr, &tv);
            if (ready < 0) {
                if (iGamePVNet::LastError() == PV_EINTR) { continue; }
                g_serverGone.store(true);
                break;
            }
            if (ready == 0) { continue; } // 超时：回顶部检查待发命令
        }

        // ---- 3) 全量排空：一直读到无数据 ----
        bool closed = false;
        for (;;) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            timeval tv{0, 0};
            const int ready = select(static_cast<int>(sock) + 1, &rfds, nullptr,
                                     nullptr, &tv);
            if (ready < 0) {
                if (iGamePVNet::LastError() == PV_EINTR) { continue; }
                closed = true;
                break;
            }
            if (ready == 0) { break; } // 当前已无更多可读数据

            char tmp[65536];
#ifdef _WIN32
            const int n = recv(sock, tmp, sizeof(tmp), 0);
#else
            const ssize_t n = recv(sock, tmp, sizeof(tmp), 0);
#endif
            if (n == 0) { closed = true; break; }
            if (n == PV_SOCKET_ERROR) {
                if (iGamePVNet::LastError() == PV_EINTR) { continue; }
                if (iGamePVNet::LastError() == PV_EWOULDBLOCK) { break; }
                closed = true;
                break;
            }
            rxBuf.insert(rxBuf.end(), tmp, tmp + n);
            g_rxBytes.fetch_add(static_cast<std::uint64_t>(n));
        }
        if (closed) {
            g_serverGone.store(true);
            break;
        }

        // ---- 4) 解析：一次排空里只保留最新一帧 ----
        int parsedFrames = 0;
        bool gotFrame = false;
        iGamePVNet::Frame newest;
        std::size_t off = 0;
        while (off < rxBuf.size()) {
            iGamePVNet::Frame f;
            std::size_t consumed = 0;
            const int rc = iGamePVNet::ParseFrame(rxBuf, off, f, consumed);
            if (rc == 1) { break; }               // 缓冲不足，等更多数据
            if (rc == -1) { off += 1; continue; } // 失步：丢 1 字节重对齐
            off += consumed;
            ++parsedFrames;
            newest = std::move(f);
            gotFrame = true;
        }
        if (off > 0) { rxBuf.erase(rxBuf.begin(), rxBuf.begin() + off); }
        // 防御：异常情况下不让缓冲无限增长。
        if (rxBuf.size() > (256u << 20)) { rxBuf.clear(); }
        // 本批里除了最后留下的那一张，其余都是被「只显示最新帧」策略丢掉的。
        if (parsedFrames > 1) {
            g_rxDropped.fetch_add(static_cast<std::uint64_t>(parsedFrames - 1));
        }

        if (!gotFrame) { continue; }

        if (!DecodeFrameRGBA(newest)) {
            // 解码失败：丢弃本帧，保留上一帧。
            continue;
        }

        g_rxFrames.fetch_add(1);
        if (newest.seq != 0) {
            std::lock_guard<std::mutex> lk(g_ioMutex);
            const auto it = g_sendTime.find(newest.seq);
            if (it != g_sendTime.end()) {
                g_rttMs.store(std::chrono::duration<double, std::milli>(
                                      std::chrono::steady_clock::now() -
                                      it->second)
                                      .count());
                g_sendTime.erase(it);
            }
        }
        {
            std::lock_guard<std::mutex> lk(g_frameMutex);
            g_latestFrame = std::move(newest);
            g_latestFrameReady = true;
            ++g_latestFrameId;
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    const std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    const int port = (argc > 2) ? std::atoi(argv[2]) : 11111;

    if (!iGamePVNet::Startup()) {
        std::cerr << "[client] socket startup failed\n";
        return 1;
    }

    // 主机名解析 + 连接：支持超算计算节点主机名（如 "cn78021"），也支持数字 IP
    // （如 "127.0.0.1"）。对标 MiniPVClient 的 QTcpSocket::connectToHost：内部用
    // getaddrinfo 解析主机名（inet_pton 只能解析数字 IP，无法解析 cn 节点名）。
    PVSocket sock = iGamePVNet::ConnectTo(host.c_str(), port);
    if (sock == PV_INVALID_SOCKET) {
        std::cerr << "[client] connect failed: " << host << ':' << port << '\n';
        iGamePVNet::Cleanup();
        return 1;
    }
    // 禁用 Nagle：降低交互延迟。
    int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
#ifdef _WIN32
               reinterpret_cast<const char*>(&one),
#else
               &one,
#endif
               sizeof(one));

    // 握手：接收标量范围 + colorbar ramp + 渲染分辨率。
    iGamePVNet::Metadata meta;
    if (!iGamePVNet::RecvMetadata(sock, meta)) {
        std::cerr << "[client] metadata recv failed\n";
        iGamePVNet::CloseSocket(sock);
        iGamePVNet::Cleanup();
        return 1;
    }
    std::cout << "[client] connected: " << meta.width << 'x' << meta.height
              << ", scalar range [" << meta.scalarMin << ", " << meta.scalarMax
              << "]\n";

    // 交互窗口（复用阶段 5 的显示端）。
    auto window = iGame::RenderWindow::New();
    window->SetSize(meta.width, meta.height);
    window->SetTitle("Parallel Volume Rendering - Client");
    GLFWwindow* raw = window->GetRawWindowPtr();
    if (!raw) {
        std::cerr << "[client] failed to create window.\n";
        iGamePVNet::CloseSocket(sock);
        iGamePVNet::Cleanup();
        return 1;
    }
    glfwMakeContextCurrent(raw);
    // 开 vsync：把客户端显示循环限到显示器刷新率（软件 GL 下实际会更低）。
    // 注意：网络收发已经搬到 I/O 线程，显示循环慢不会再拖慢收帧（这正是本次修复的点）。
    glfwSwapInterval(1);
    if (!gladLoadGL()) {
        std::cerr << "[client] gladLoadGL failed.\n";
        iGamePVNet::CloseSocket(sock);
        iGamePVNet::Cleanup();
        return 1;
    }
    // 插桩：打印 GL 后端。llvmpipe/softpipe/Mesa ⇒ 软件路径；NVIDIA/Quadro ⇒ 硬件路径。
    // 客户端走 RenderWindow + gladLoadGL，不经过 Scene::InitOpenGL，因此这里单独打。
    {
        const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const char* renderer =
                reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* version =
                reinterpret_cast<const char*>(glGetString(GL_VERSION));
        std::cerr << "[client] GL vendor=" << (vendor ? vendor : "?")
                  << " renderer=" << (renderer ? renderer : "?")
                  << " version=" << (version ? version : "?") << '\n';
    }
    if (!InitDisplay(meta.width, meta.height)) {
        std::cerr << "[client] display init failed.\n";
        iGamePVNet::CloseSocket(sock);
        iGamePVNet::Cleanup();
        return 1;
    }
    BuildColorbarTextureFromRamp(meta.colorbar);
    g_imgTexW = meta.width;
    g_imgTexH = meta.height;

    glfwSetMouseButtonCallback(raw, ClientMouseButtonCallback);
    glfwSetCursorPosCallback(raw, ClientCursorPosCallback);
    glfwSetScrollCallback(raw, ClientScrollCallback);
    glfwSetKeyCallback(raw, ClientKeyCallback);

    // 启动网络 I/O 线程（独占 socket 收发），主线程只做显示。
    std::thread ioThread(IoThreadMain, sock);

    std::cout << "[client] left-drag = rotate, wheel = zoom; fps/RTT shown while "
                 "dragging.\n";
    std::cout.flush();

    bool hasFrame = false;
    iGamePVNet::Frame displayFrame;
    std::uint64_t lastFrameId = 0;
    auto lastNewFrameTime = std::chrono::steady_clock::now();
    double smoothedFps = 0.0;

    while (!glfwWindowShouldClose(raw)) {
        // glfwPollEvents 决定鼠标事件的采样率，也就决定 INTERACT 命令的发送频率
        // （光标回调只在 poll 时触发）——它是整个交互闭环的节奏上限，因此单独计时。
        const auto tPoll0 = std::chrono::steady_clock::now();
        glfwPollEvents();
        g_diagPollMs += std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - tPoll0)
                                .count();

        // 取 I/O 线程送来的最新帧（没有新帧就不动纹理，只重画上一帧）。
        bool gotNewFrame = false;
        {
            std::lock_guard<std::mutex> lk(g_frameMutex);
            if (g_latestFrameReady && g_latestFrameId != lastFrameId) {
                displayFrame = std::move(g_latestFrame);
                g_latestFrameReady = false;
                lastFrameId = g_latestFrameId;
                gotNewFrame = true;
                hasFrame = true;
            }
        }

        if (gotNewFrame) {
            const auto now = std::chrono::steady_clock::now();
            const double dtMs = std::chrono::duration<double, std::milli>(
                                        now - lastNewFrameTime)
                                        .count();
            lastNewFrameTime = now;
            const double fps = dtMs > 1e-6 ? 1000.0 / dtMs : 0.0;
            smoothedFps = smoothedFps <= 0.0 ? fps
                                             : smoothedFps * 0.9 + fps * 0.1;
        }

        // 渲染显示。
        int fbW = meta.width, fbH = meta.height;
        glfwGetFramebufferSize(raw, &fbW, &fbH);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fbW, fbH);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 只在新帧到达时上传纹理（4MB 上传是客户端循环的最大开销）。
        const auto tUpload0 = std::chrono::steady_clock::now();
        if (gotNewFrame) {
            EnsureImageTexture(displayFrame.width, displayFrame.height);
            UploadImageTexture(displayFrame.width, displayFrame.height,
                               displayFrame.rgba);
        }
        g_diagUploadMs += std::chrono::duration<double, std::milli>(
                                  std::chrono::steady_clock::now() - tUpload0)
                                  .count();

        // 绘制（quad + colorbar + 文字）单独计时，与换缓冲（swap）分开：
        // 软件 GL 下 draw 耗时 ∝ 光栅化量，swap 耗时 ∝ 等 vblank 或 X11 传输。
        const auto tDraw0 = std::chrono::steady_clock::now();
        if (hasFrame) {
            DrawTexturedQuad(0.0f, 0.0f, static_cast<float>(fbW),
                             static_cast<float>(fbH), 0.0f, 0.0f, 1.0f, 1.0f,
                             g_imageTex, static_cast<float>(fbW),
                             static_cast<float>(fbH));
        }

        DrawColorBar(meta.scalarMin, meta.scalarMax, static_cast<float>(fbW),
                     static_cast<float>(fbH));

        const float white[3] = {1.0f, 1.0f, 1.0f};

        // 字段名（放在 colorbar 附近，其刻度下方）。
        if (meta.fieldName[0] != '\0') {
            DrawText(("Field: " + std::string(meta.fieldName)).c_str(), 24.0f,
                     24.0f, 2.0f, white, static_cast<float>(fbW),
                     static_cast<float>(fbH));
        }

        // 多帧播放：当前帧号 + 操作说明（英文，常驻显示）。
        if (meta.numFrames > 1) {
            char frameBuf[64];
            std::snprintf(frameBuf, sizeof(frameBuf), "Frame %d/%d",
                          displayFrame.frameIndex + 1, meta.numFrames);
            DrawText(frameBuf, 24.0f, static_cast<float>(fbH) - 112.0f, 2.0f,
                     white, static_cast<float>(fbW),
                     static_cast<float>(fbH));
            DrawText("N-Next Frame, P-Previous Frame", 24.0f,
                     static_cast<float>(fbH) - 88.0f, 2.0f, white,
                     static_cast<float>(fbW), static_cast<float>(fbH));
        }

        if (g_cliInteracting.load()) {
            // RTT 显示在 FPS 上方（黄色，毫秒）。
            const double rtt = g_rttMs.load();
            if (rtt >= 0.0) {
                char rttBuf[64];
                std::snprintf(rttBuf, sizeof(rttBuf), "RTT: %.0f ms", rtt);
                const float yellow[3] = {1.0f, 1.0f, 0.0f};
                DrawText(rttBuf, 24.0f, static_cast<float>(fbH) - 64.0f, 2.0f,
                         yellow, static_cast<float>(fbW),
                         static_cast<float>(fbH));
            }
            char fpsBuf[64];
            std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", smoothedFps);
            const float white[3] = {1.0f, 1.0f, 1.0f};
            DrawText(fpsBuf, 24.0f, static_cast<float>(fbH) - 40.0f, 2.0f, white,
                     static_cast<float>(fbW), static_cast<float>(fbH));
        }
        g_diagDrawMs += std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - tDraw0)
                                .count();

        const auto tSwap0 = std::chrono::steady_clock::now();
        glfwSwapBuffers(raw);
        g_diagSwapMs += std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - tSwap0)
                                .count();
        if (g_serverGone.load()) { break; }

        // 诊断：每 5s 打印显示循环速率、到达帧率、丢弃帧数、收字节数、RTT，
        // 以及显示循环各段耗时的单次平均值（poll=事件, upload=纹理上传, draw=绘制,
        // swap=换缓冲），外加窗口面积与最近一帧 ROI 面积（判断 draw+swap ∝ 窗口面积
        // 还是 ∝ ROI，见设计文档 §10.1/§10.10）。
        // 判读方法：
        //   display 明显低于 60Hz 且 swap 占掉大半、draw 很小 → 瓶颈是等 vblank 或
        //     X11 传输（换缓冲），此时服务端再怎么降分辨率也没用；
        //   display 明显低于 60Hz 且 draw 占掉大半 → 软件 GL 光栅化按窗口面积走，
        //     缩小窗口最有效；
        //   display ≈ 60Hz 而 rx ≈ 服务端出帧率 → 网络/服务端才是节奏来源。
        {
            static int iter = 0;
            static auto t0 = std::chrono::steady_clock::now();
            static std::uint64_t lastFrames = 0;
            static std::uint64_t lastBytes = 0;
            static std::uint64_t lastDropped = 0;
            ++iter;
            const auto now = std::chrono::steady_clock::now();
            const double sec = std::chrono::duration<double>(now - t0).count();
            if (sec >= 5.0) {
                const std::uint64_t fr = g_rxFrames.load();
                const std::uint64_t by = g_rxBytes.load();
                const std::uint64_t dr = g_rxDropped.load();
                const double n = iter > 0 ? static_cast<double>(iter) : 1.0;
                std::cerr << "[client] display=" << (iter / sec)
                          << "Hz rx=" << ((fr - lastFrames) / sec)
                          << "Hz dropped=" << (dr - lastDropped)
                          << " rxKB/s=" << ((by - lastBytes) / sec / 1024.0)
                          << " rtt=" << g_rttMs.load() << "ms"
                          << " win=" << fbW << 'x' << fbH
                          << " roi=" << displayFrame.roiW << 'x'
                          << displayFrame.roiH
                          << " | ms/iter[poll=" << (g_diagPollMs / n)
                          << " upload=" << (g_diagUploadMs / n)
                          << " draw=" << (g_diagDrawMs / n)
                          << " swap=" << (g_diagSwapMs / n) << "]\n";
                iter = 0;
                lastFrames = fr;
                lastBytes = by;
                lastDropped = dr;
                g_diagPollMs = 0.0;
                g_diagUploadMs = 0.0;
                g_diagDrawMs = 0.0;
                g_diagSwapMs = 0.0;
                t0 = now;
            }
        }
    }

    // 停止 I/O 线程，再尽力通知服务端退出。
    g_ioStop.store(true);
    if (ioThread.joinable()) { ioThread.join(); }
    if (!g_serverGone.load()) {
        const char exitCmd[] = "EXIT\n";
        iGamePVNet::SendAll(sock, exitCmd, sizeof(exitCmd) - 1);
    }
    iGamePVNet::CloseSocket(sock);
    iGamePVNet::Cleanup();
    return 0;
}
