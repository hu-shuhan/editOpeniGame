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
// 说明：
//   - 客户端只做「显示 + 输入」，不做任何渲染/合成，也不依赖 MPI；
//   - 左下角 colorbar（含全局标量范围）由服务端握手时下发的 ramp 重建；
//   - 拖动旋转期间显示 fps（本地接收帧率）；
//   - 复用阶段 5（ParallelVolumeInteractive.h）的 OpenGL 显示辅助代码（纹理四边形、
//     colorbar、5x7 位图字体），保证前后端观感一致。
// 注意：协议头必须先包含（其内部先于 windows.h 引入 winsock2），再包含 glad/windows.h。
#include "ParallelVolumeProtocol.h"

#include "iGameRenderWindow.h"
#include "ParallelVolumeInteractive.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <zlib.h>

using namespace iGameVolInteractive; // 复用阶段 5 的显示辅助（InitDisplay/DrawColorBar/...）

namespace {

// ---------------------------------------------------------------------------
// 客户端交互状态（与阶段 5 命名区分，避免和 iGameVolInteractive 里的全局变量冲突）。
// ---------------------------------------------------------------------------
bool g_cliDragging = false;       // 左键是否按下
bool g_cliInteracting = false;    // 是否拖动中（用于 LOD + fps）
double g_cliLastX = 0.0;
double g_cliLastY = 0.0;
double g_cliPendingAzim = 0.0;    // 待发送的轨道方位角增量（弧度）
double g_cliPendingElev = 0.0;    // 待发送的仰角增量（弧度）
double g_cliPendingZoom = 1.0;    // 待发送的距离缩放因子（累积）
bool g_cliModeDirty = false;      // 交互状态变化需立刻发送一帧

// 当前 g_imageTex 的尺寸（帧分辨率变化时重新分配纹理）。
int g_imgTexW = 0;
int g_imgTexH = 0;

// RTT 测量：seq -> 发送时刻（服务端在回帧里原样带回 seq）。
std::uint32_t g_cliSeq = 0;
std::map<std::uint32_t, std::chrono::steady_clock::time_point> g_sendTime;
double g_rttMs = -1.0; // 最近一次 RTT（毫秒）；-1 表示尚未测量

// 用服务端下发的 colorbar ramp 重建渐变纹理（仅颜色，alpha 恒 1）。
void BuildColorbarTextureFromRamp(const unsigned char* ramp) {
    glBindTexture(GL_TEXTURE_2D, g_colorbarTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iGamePVNet::kColorbarSize, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, ramp);
}

// 帧分辨率变化时（重）分配合成图纹理。
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
        g_cliInteracting = true;
        g_cliModeDirty = true;
    } else if (action == GLFW_RELEASE) {
        g_cliDragging = false;
        g_cliInteracting = false;
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
    g_cliPendingAzim += -dx * 0.006; // 阶段 5：azimuth -= dx*0.006
    g_cliPendingElev += dy * 0.006;  // 阶段 5：elevation += dy*0.006
}

void ClientScrollCallback(GLFWwindow*, double, double yoffset) {
    g_cliPendingZoom *= (yoffset > 0.0) ? 0.9 : 1.1;
}

// 非阻塞地把 socket 里当前可读的数据读入 buf；返回 false 表示断开。
bool RecvAvailable(PVSocket s, std::vector<char>& buf) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    timeval tv{0, 0};
    const int ready = select(static_cast<int>(s) + 1, &rfds, nullptr, nullptr,
                             &tv);
    if (ready <= 0) { return true; }
    char tmp[8192];
#ifdef _WIN32
    const int n = recv(s, tmp, sizeof(tmp), 0);
#else
    const ssize_t n = recv(s, tmp, sizeof(tmp), 0);
#endif
    if (n > 0) {
        buf.insert(buf.end(), tmp, tmp + n);
        return true;
    }
    if (n == 0) { return false; }
    if (iGamePVNet::LastError() == PV_EINTR) { return true; }
    return false;
}

// 发送当前累积的 INTERACT 命令（带单调递增 seq 用于 RTT 测量），成功后清零 pending。
bool SendInteract(PVSocket s, int interactiveFlag) {
    const std::uint32_t seq = ++g_cliSeq;
    const std::string cmd = iGamePVNet::MakeInteractCommand(
            seq, g_cliPendingAzim, g_cliPendingElev, g_cliPendingZoom,
            interactiveFlag);
    if (!iGamePVNet::SendAll(s, cmd.data(), cmd.size())) { return false; }
    g_sendTime[seq] = std::chrono::steady_clock::now();
    if (g_sendTime.size() > 1024) { g_sendTime.clear(); } // 防泄漏
    g_cliPendingAzim = 0.0;
    g_cliPendingElev = 0.0;
    g_cliPendingZoom = 1.0;
    g_cliModeDirty = false;
    return true;
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
    if (!gladLoadGL()) {
        std::cerr << "[client] gladLoadGL failed.\n";
        iGamePVNet::CloseSocket(sock);
        iGamePVNet::Cleanup();
        return 1;
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

    std::cout << "[client] left-drag = rotate, wheel = zoom; fps shown while "
                 "dragging.\n";
    std::cout.flush();

    std::vector<char> rxBuf;
    bool hasFrame = false;
    iGamePVNet::Frame lastFrame;
    auto lastNewFrameTime = std::chrono::steady_clock::now();
    double smoothedFps = 0.0;

    while (!glfwWindowShouldClose(raw)) {
        glfwPollEvents();

        // 有 pending 交互时发送 INTERACT（含交互状态切换）。
        const bool hasPending = std::abs(g_cliPendingAzim) > 1e-12 ||
                                std::abs(g_cliPendingElev) > 1e-12 ||
                                std::abs(g_cliPendingZoom - 1.0) > 1e-12 ||
                                g_cliModeDirty;
        if (hasPending) {
            if (!SendInteract(sock, g_cliInteracting ? 1 : 0)) {
                std::cerr << "[client] send failed; exiting.\n";
                break;
            }
        }

        // 接收帧：一次 readyRead 可能含多帧，只显示最新一帧（丢弃旧帧）。
        if (!RecvAvailable(sock, rxBuf)) {
            std::cerr << "[client] server disconnected.\n";
            break;
        }
        bool gotNewFrame = false;
        std::size_t off = 0;
        while (off < rxBuf.size()) {
            iGamePVNet::Frame f;
            std::size_t consumed = 0;
            const int rc = iGamePVNet::ParseFrame(rxBuf, off, f, consumed);
            if (rc == 1) { break; }      // 缓冲不足，等更多数据
            if (rc == -1) { off += 1; continue; } // 失步，丢 1 字节重对齐
            // rc == 0：把 payload 解码为 rgba（codec=1 为 zlib 压缩，codec=0 为 raw）。
            if (f.codec == iGamePVNet::kCodecZlib) {
                std::vector<unsigned char> dec(
                        static_cast<std::size_t>(f.width) * f.height * 4);
                uLongf destLen = static_cast<uLongf>(dec.size());
                const int zr = uncompress(dec.data(), &destLen, f.payload.data(),
                                          static_cast<uLong>(f.payload.size()));
                if (zr != Z_OK || destLen != dec.size()) {
                    off += consumed;   // 解压失败：丢弃本帧，保留上一帧
                    continue;
                }
                f.rgba = std::move(dec);
            } else {
                f.rgba = std::move(f.payload);
            }
            off += consumed;
            lastFrame = std::move(f);
            hasFrame = true;
            gotNewFrame = true;
        }
        if (off > 0) { rxBuf.erase(rxBuf.begin(), rxBuf.begin() + off); }

        // 只有“真的收到新帧”才更新 fps / rtt。否则测的是客户端循环率（vsync 60Hz），
        // 而不是服务端实际出帧率。
        if (gotNewFrame) {
            const auto now = std::chrono::steady_clock::now();
            const double dtMs = std::chrono::duration<double, std::milli>(
                                        now - lastNewFrameTime)
                                        .count();
            lastNewFrameTime = now;
            const double fps = dtMs > 1e-6 ? 1000.0 / dtMs : 0.0;
            smoothedFps = smoothedFps <= 0.0 ? fps
                                             : smoothedFps * 0.9 + fps * 0.1;

            if (lastFrame.seq != 0) {
                const auto it = g_sendTime.find(lastFrame.seq);
                if (it != g_sendTime.end()) {
                    g_rttMs = std::chrono::duration<double, std::milli>(
                                      now - it->second)
                                      .count();
                    g_sendTime.erase(it);
                }
            }
        }

        // 渲染显示。
        int fbW = meta.width, fbH = meta.height;
        glfwGetFramebufferSize(raw, &fbW, &fbH);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fbW, fbH);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (hasFrame) {
            EnsureImageTexture(lastFrame.width, lastFrame.height);
            UploadImageTexture(lastFrame.width, lastFrame.height, lastFrame.rgba);

            DrawTexturedQuad(0.0f, 0.0f, static_cast<float>(fbW),
                             static_cast<float>(fbH), 0.0f, 0.0f, 1.0f, 1.0f,
                             g_imageTex, static_cast<float>(fbW),
                             static_cast<float>(fbH));
        }

        DrawColorBar(meta.scalarMin, meta.scalarMax, static_cast<float>(fbW),
                     static_cast<float>(fbH));

        if (g_cliInteracting) {
            // RTT 显示在 FPS 上方（黄色，毫秒）。
            if (g_rttMs >= 0.0) {
                char rttBuf[64];
                std::snprintf(rttBuf, sizeof(rttBuf), "RTT: %.0f ms", g_rttMs);
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

        glfwSwapBuffers(raw);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 尽力通知服务端退出。
    const char exitCmd[] = "EXIT\n";
    iGamePVNet::SendAll(sock, exitCmd, sizeof(exitCmd) - 1);
    iGamePVNet::CloseSocket(sock);
    iGamePVNet::Cleanup();
    return 0;
}
