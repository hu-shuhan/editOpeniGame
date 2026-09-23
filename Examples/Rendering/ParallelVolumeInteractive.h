#pragma once
// ParallelVolumeInteractive.h — 并行体绘制交互窗口（阶段 5）共享实现。
//
// 把「各 rank 离屏渲染 → iGameCompositePass 合成 → rank 0 窗口显示」的交互循环封装成
// 一个可复用函数 RunInteractive(...)，由 ParallelVolumeRendering.cpp 的 --interactive
// 开关调用，避免把显示/交互代码内联到入口文件。
//
// 设计要点：
//   - 后端统一走 CPU 生产路径（iGameVolumeRayCastCPU 无头渲染），只有 rank 0 额外创建
//     GLFW 窗口用于显示合成结果，其余 rank 全程无头；
//   - 每帧 rank 0 广播相机（位置/焦点/上方向）与交互状态，所有 rank 用同一相机 + 同一
//     全局裁剪范围渲染 → 深度有序合成 → rank 0 显示；
//   - 鼠标：左键拖动 = 绕全局中心轨道旋转，滚轮 = 缩放（iGame::Camera）；
//   - 左下角 colorbar（含全局标量范围），拖动旋转期间显示 fps；
//   - 交互 LOD：交互中 maxSamples=128 / pixelStride=2 / screenROI 开，静止恢复
//     512 / 1 / 关（对标 MiniPVServer；语义等价于 iGameInteractorStyle 的
//     StartInteraction()/EndInteraction() 回调）。
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "iGameCamera.h"
#include "iGameCompositePass.h"
#include "iGameParallelContext.h"
#include "iGameRenderWindow.h"
#include "iGameVolumeRayCastCPU.h"
#include "iGameVolumeTransferFunction.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace iGameVolInteractive {

// ---------------------------------------------------------------------------
// 交互状态（仅 rank 0 使用，由 GLFW 回调更新）
// ---------------------------------------------------------------------------
inline bool g_dragging = false;    // 左键是否按下（连续旋转）
inline bool g_interacting = false; // 是否处于交互中（用于 LOD + fps 显示）
inline double g_lastX = 0.0;
inline double g_lastY = 0.0;
inline double g_azimuth = 0.0;     // 相机轨道方位角（弧度）
inline double g_elevation = 0.0;   // 相机轨道仰角（弧度）
inline double g_distance = 1.0;    // 相机到全局中心的距离
inline double g_center[3] = {0.0, 0.0, 0.0}; // 全局包围盒中心（世界坐标）
inline double g_diag = 1.0;        // 全局包围盒对角线（用于缩放/初始距离）

inline void ClampElevation() {
    const double kMaxElev = 1.55; // ~89°，避免 up 向量退化
    g_elevation = std::clamp(g_elevation, -kMaxElev, kMaxElev);
}
inline void ClampDistance() {
    g_distance = std::clamp(g_distance, g_diag * 0.05, g_diag * 20.0);
}

// ---------------------------------------------------------------------------
// 最小 OpenGL 显示辅助（仅 rank 0）：合成图全屏纹理 + colorbar + 5x7 位图字体
// ---------------------------------------------------------------------------
inline GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "[interactive] shader compile error: " << log << "\n";
    }
    return s;
}

inline GLuint LinkProgram(const char* vs, const char* fs) {
    GLuint v = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::cerr << "[interactive] link error: " << log << "\n";
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// 纹理着色器：像素坐标 + UV -> 采样纹理。
inline const char* const kTexVS = R"GLSL(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
uniform vec2 uViewport;
out vec2 vUV;
void main() {
    vec2 ndc = (aPos / uViewport) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
}
)GLSL";
inline const char* const kTexFS = R"GLSL(#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 FragColor;
void main() { FragColor = texture(uTex, vUV); }
)GLSL";

// 纯色着色器：像素坐标 + 颜色 -> 填充色（文字/边框）。
inline const char* const kColVS = R"GLSL(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
uniform vec2 uViewport;
out vec4 vColor;
void main() {
    vec2 ndc = (aPos / uViewport) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = aColor;
}
)GLSL";
inline const char* const kColFS = R"GLSL(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
)GLSL";

inline GLuint g_texProgram = 0;
inline GLuint g_colProgram = 0;
inline GLuint g_texVao = 0, g_texVbo = 0;
inline GLuint g_colVao = 0, g_colVbo = 0;
inline GLuint g_imageTex = 0;
inline GLuint g_colorbarTex = 0;
inline int g_texViewportLoc = -1;
inline int g_colViewportLoc = -1;

inline bool InitDisplay(int width, int height) {
    g_texProgram = LinkProgram(kTexVS, kTexFS);
    g_colProgram = LinkProgram(kColVS, kColFS);
    if (!g_texProgram || !g_colProgram) { return false; }
    g_texViewportLoc = glGetUniformLocation(g_texProgram, "uViewport");
    g_colViewportLoc = glGetUniformLocation(g_colProgram, "uViewport");

    // 纹理四边形 VAO（interleaved [x,y,u,v]）。
    glGenVertexArrays(1, &g_texVao);
    glGenBuffers(1, &g_texVbo);
    glBindVertexArray(g_texVao);
    glBindBuffer(GL_ARRAY_BUFFER, g_texVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    // 纯色四边形 VAO（interleaved [x,y,r,g,b,a]）。
    glGenVertexArrays(1, &g_colVao);
    glGenBuffers(1, &g_colVbo);
    glBindVertexArray(g_colVao);
    glBindBuffer(GL_ARRAY_BUFFER, g_colVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    // 合成图纹理（每帧更新）。
    glGenTextures(1, &g_imageTex);
    glBindTexture(GL_TEXTURE_2D, g_imageTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);

    // colorbar 渐变纹理（由传输函数 LUT 的 RGB 构建）。
    glGenTextures(1, &g_colorbarTex);
    glBindTexture(GL_TEXTURE_2D, g_colorbarTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

inline void UploadImageTexture(int width, int height,
                               const std::vector<unsigned char>& rgba) {
    glBindTexture(GL_TEXTURE_2D, g_imageTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                    GL_UNSIGNED_BYTE, rgba.data());
}

// 从传输函数颜色映射器构建 256x1 的 colorbar 纹理（仅颜色，alpha 恒 1）。
inline void BuildColorbarTexture(iGame::iGameVolumeTransferFunction* tf) {
    std::vector<unsigned char> ramp(256 * 4, 255);
    auto mapper = tf ? tf->GetColorMapper() : nullptr;
    for (int i = 0; i < 256; ++i) {
        const float t = static_cast<float>(i) / 255.0f;
        float rgb[3]{0.5f, 0.5f, 0.5f};
        if (mapper) { mapper->MapColor(t, rgb); }
        auto toU = [](float v) {
            v = std::clamp(v, 0.0f, 1.0f);
            return static_cast<unsigned char>(v * 255.0f + 0.5f);
        };
        ramp[static_cast<size_t>(i) * 4 + 0] = toU(rgb[0]);
        ramp[static_cast<size_t>(i) * 4 + 1] = toU(rgb[1]);
        ramp[static_cast<size_t>(i) * 4 + 2] = toU(rgb[2]);
    }
    glBindTexture(GL_TEXTURE_2D, g_colorbarTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, ramp.data());
}

inline void DrawTexturedQuad(float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1,
                             GLuint tex, float viewW, float viewH) {
    const float verts[24] = {
            x0, y0, u0, v0, x1, y0, u1, v0, x0, y1, u0, v1,
            x1, y0, u1, v0, x1, y1, u1, v1, x0, y1, u0, v1,
    };
    glUseProgram(g_texProgram);
    glUniform2f(g_texViewportLoc, viewW, viewH);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(g_texProgram, "uTex"), 0);
    glBindVertexArray(g_texVao);
    glBindBuffer(GL_ARRAY_BUFFER, g_texVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

inline void EmitColoredQuad(std::vector<float>& v, float x0, float y0, float x1,
                            float y1, float r, float g, float b, float a) {
    const float quad[36] = {
            x0, y0, r, g, b, a, x1, y0, r, g, b, a, x0, y1, r, g, b, a,
            x1, y0, r, g, b, a, x1, y1, r, g, b, a, x0, y1, r, g, b, a,
    };
    v.insert(v.end(), quad, quad + 36);
}

inline void DrawColoredQuads(const std::vector<float>& v, float viewW,
                             float viewH) {
    if (v.empty()) { return; }
    glUseProgram(g_colProgram);
    glUniform2f(g_colViewportLoc, viewW, viewH);
    glBindVertexArray(g_colVao);
    glBindBuffer(GL_ARRAY_BUFFER, g_colVbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(),
                 GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size() / 6));
}

// 5x7 位图字体：每字符 7 字节，bit4(0x10)=最左列，bit0=最右列。
inline const unsigned char* Glyph(char c) {
    static const unsigned char kFont[][7] = {
            {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // '0'
            {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // '1'
            {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // '2'
            {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, // '3'
            {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // '4'
            {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // '5'
            {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // '6'
            {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // '7'
            {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // '8'
            {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // '9'
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}, // '.'
            {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, // '-'
            {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}, // '+'
            {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E}, // 'e'
            {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // 'E'
            {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // 'F'
            {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // 'P'
            {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // 'S'
            {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}, // ':'
            {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // 'R'
            {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // 'T'
            {0x00, 0x00, 0x1B, 0x15, 0x15, 0x15, 0x15}, // 'm'
            {0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E}, // 's'
    };
    static const unsigned char kSpace[7] = {0, 0, 0, 0, 0, 0, 0};

    if (c >= '0' && c <= '9') { return kFont[c - '0']; }
    switch (c) {
        case '.': return kFont[10];
        case '-': return kFont[11];
        case '+': return kFont[12];
        case 'e': return kFont[13];
        case 'E': return kFont[14];
        case 'F': return kFont[15];
        case 'P': return kFont[16];
        case 'S': return kFont[17];
        case ':': return kFont[18];
        case 'R': return kFont[19];
        case 'T': return kFont[20];
        case 'm': return kFont[21];
        case 's': return kFont[22];
        case ' ': return kSpace;
        default: return kSpace;
    }
}

// 在 (x, y)（左下角，像素坐标）绘制一行文本；scale 为每个位点的像素尺寸。
inline void DrawText(const std::string& text, float x, float y, float scale,
                     const float color[3], float viewW, float viewH) {
    std::vector<float> verts;
    float cursorX = x;
    for (char c : text) {
        const unsigned char* rows = Glyph(c);
        for (int r = 0; r < 7; ++r) {
            const float py = y + static_cast<float>(6 - r) * scale;
            for (int col = 0; col < 5; ++col) {
                if (rows[r] & (0x10u >> col)) {
                    const float px = cursorX + static_cast<float>(col) * scale;
                    EmitColoredQuad(verts, px, py, px + scale, py + scale,
                                    color[0], color[1], color[2], 1.0f);
                }
            }
        }
        cursorX += 6.0f * scale;
    }
    DrawColoredQuads(verts, viewW, viewH);
}

inline void FormatNumber(double v, char* buf, size_t n) {
    if (!std::isfinite(v)) {
        std::snprintf(buf, n, "0");
    } else {
        std::snprintf(buf, n, "%.4g", v);
    }
}

// 左下角 colorbar：渐变条 + 白框 + min/mid/max 刻度。
inline void DrawColorBar(double minV, double maxV, float viewW, float viewH) {
    const float x0 = 24.0f;
    const float y0 = 64.0f;
    const float w = 360.0f;
    const float h = 20.0f;

    DrawTexturedQuad(x0, y0, x0 + w, y0 + h, 0.0f, 0.0f, 1.0f, 1.0f,
                     g_colorbarTex, viewW, viewH);

    const float white[3] = {1.0f, 1.0f, 1.0f};
    std::vector<float> border;
    EmitColoredQuad(border, x0, y0, x0 + w, y0 + 1.0f, white[0], white[1],
                    white[2], 1.0f);
    EmitColoredQuad(border, x0, y0 + h - 1.0f, x0 + w, y0 + h, white[0],
                    white[1], white[2], 1.0f);
    EmitColoredQuad(border, x0, y0, x0 + 1.0f, y0 + h, white[0], white[1],
                    white[2], 1.0f);
    EmitColoredQuad(border, x0 + w - 1.0f, y0, x0 + w, y0 + h, white[0],
                    white[1], white[2], 1.0f);
    DrawColoredQuads(border, viewW, viewH);

    char buf[32];
    const float labelY = y0 - 20.0f;
    FormatNumber(minV, buf, sizeof(buf));
    DrawText(buf, x0, labelY, 2.0f, white, viewW, viewH);
    FormatNumber((minV + maxV) * 0.5, buf, sizeof(buf));
    DrawText(buf, x0 + w * 0.5f, labelY, 2.0f, white, viewW, viewH);
    FormatNumber(maxV, buf, sizeof(buf));
    DrawText(buf, x0 + w, labelY, 2.0f, white, viewW, viewH);
}

// ---------------------------------------------------------------------------
// GLFW 鼠标回调（仅 rank 0 安装）：左键旋转、滚轮缩放。
// ---------------------------------------------------------------------------
inline void MouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) { return; }
    if (action == GLFW_PRESS) {
        g_dragging = true;
        g_interacting = true;
    } else if (action == GLFW_RELEASE) {
        g_dragging = false;
        g_interacting = false;
    }
}

inline void CursorPosCallback(GLFWwindow* window, double x, double y) {
    if (!g_dragging) {
        g_lastX = x;
        g_lastY = y;
        return;
    }
    const double dx = x - g_lastX;
    const double dy = y - g_lastY;
    g_lastX = x;
    g_lastY = y;
    g_azimuth -= dx * 0.006;   // 弧度/像素
    g_elevation += dy * 0.006;
    ClampElevation();
}

inline void ScrollCallback(GLFWwindow*, double, double yoffset) {
    g_distance *= (yoffset > 0.0) ? 0.9 : 1.1;
    ClampDistance();
}

// ---------------------------------------------------------------------------
// 交互渲染循环（所有 rank 共同调用）。
// ---------------------------------------------------------------------------
inline int RunInteractive(iGame::iGameVolumeRayCastCPU* rayCaster,
                          iGame::Camera* camera,
                          iGame::iGameVolumeTransferFunction* tf,
                          double globalMin, double globalMax,
                          const double gcenter[3],
                          const double blockCenter[3], double radius, int width,
                          int height) {
    auto ctx = iGame::ParallelContext::Instance();
    const int rank = ctx->Rank();

    g_center[0] = gcenter[0];
    g_center[1] = gcenter[1];
    g_center[2] = gcenter[2];
    g_diag = radius * 2.0;
    g_distance = radius * 3.0;

    // rank 0 打开交互窗口（仅显示用；其余 rank 全程无头）。
    iGame::RenderWindow::Pointer window = nullptr;
    GLFWwindow* rawWindow = nullptr;
    if (rank == 0) {
        window = iGame::RenderWindow::New();
        window->SetSize(width, height);
        window->SetTitle("Parallel Volume Rendering - Interactive (rank 0)");
        rawWindow = window->GetRawWindowPtr();
        if (!rawWindow) {
            std::cerr << "[rank 0] failed to create window.\n";
            return 1;
        }
        glfwMakeContextCurrent(rawWindow);
        if (!gladLoadGL()) {
            std::cerr << "[rank 0] gladLoadGL failed.\n";
            return 1;
        }
        if (!InitDisplay(width, height)) {
            std::cerr << "[rank 0] display init failed.\n";
            return 1;
        }
        BuildColorbarTexture(tf);

        glfwSetMouseButtonCallback(rawWindow, MouseButtonCallback);
        glfwSetCursorPosCallback(rawWindow, CursorPosCallback);
        glfwSetScrollCallback(rawWindow, ScrollCallback);

        std::cout << "[rank 0] scalar range [" << globalMin << ", "
                  << globalMax << "]\n";
        std::cout << "[rank 0] interactive: left-drag = rotate, wheel = zoom; "
                     "fps shown while rotating.\n";
        std::cout.flush();
    }

    const igm::mat4 modelMatrix(1.0f);
    std::chrono::steady_clock::time_point lastFrame =
            std::chrono::steady_clock::now();
    double smoothedFps = 0.0;

    while (true) {
        int shouldClose = 0;
        int interactive = 0;
        double camPos[3] = {0.0, 0.0, 0.0};
        double camFp[3] = {0.0, 0.0, 0.0};
        double camUp[3] = {0.0, 1.0, 0.0};

        if (rank == 0) {
            glfwPollEvents();
            shouldClose = glfwWindowShouldClose(rawWindow) ? 1 : 0;
            interactive = g_interacting ? 1 : 0;

            const double elev = g_elevation;
            const double dir[3] = {
                    std::cos(elev) * std::sin(g_azimuth),
                    std::sin(elev),
                    std::cos(elev) * std::cos(g_azimuth),
            };
            for (int d = 0; d < 3; ++d) {
                camFp[d] = gcenter[d];
                camPos[d] = gcenter[d] + dir[d] * g_distance;
            }
        }

        ctx->Broadcast(&shouldClose, 1, 0);
        if (shouldClose) { break; }
        ctx->Broadcast(&interactive, 1, 0);
        ctx->Broadcast(camPos, 3, 0);
        ctx->Broadcast(camFp, 3, 0);
        ctx->Broadcast(camUp, 3, 0);

        // 所有 rank 更新相机（位置/焦点/上方向 + 全局裁剪范围）。
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
        double nearPlane = dist - radius;
        double farPlane = dist + radius;
        const double minGap = 0.0001;
        if (nearPlane < minGap * farPlane) { nearPlane = minGap * farPlane; }
        camera->SetClippingRange(static_cast<float>(nearPlane),
                                 static_cast<float>(farPlane));

        // 交互 LOD（对标 MiniPVServer：交互中低采样 + 大步进 + ROI）。
        // 采样率砍到 1/4（512→128）、像素步进 2。步长用自动推导（按**各自分块对角线** /
        // maxSamples），保证所有 rank 在交互时用同样的相对采样率与大步长（固定全局步长会
        // 让小分块只有 1~2 个采样、大分块仍采满 128 步，出现不一致）。
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
        rayCaster->Render(view, proj, modelMatrix,
                          igm::uvec2{static_cast<unsigned>(width),
                                     static_cast<unsigned>(height)},
                          rgba, depth);

        // 分布式合成。
        auto composite = iGame::iGameCompositePass::New();
        composite->SetLocalImage(width, height, rgba, depth);
        composite->SetBlockDepth(iGame::iGameCompositePass::ComputeBlockDepth(
                blockCenter, camPos, front));
        composite->SetBackgroundColor(0.0f, 0.0f, 0.0f);
        if (!composite->Composite()) {
            if (rank == 0) { std::cerr << "Composite failed.\n"; }
            break;
        }

        // rank 0 显示合成结果 + colorbar +（交互中）fps。
        if (rank == 0) {
            int fbW = width, fbH = height;
            glfwGetFramebufferSize(rawWindow, &fbW, &fbH);
            const auto now = std::chrono::steady_clock::now();
            const double dtMs = std::chrono::duration<double, std::milli>(
                                        now - lastFrame)
                                        .count();
            lastFrame = now;
            const double fps = dtMs > 1e-6 ? 1000.0 / dtMs : 0.0;
            smoothedFps = smoothedFps <= 0.0 ? fps
                                             : smoothedFps * 0.9 + fps * 0.1;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fbW, fbH);
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            UploadImageTexture(width, height, composite->GetResultRGBA());
            DrawTexturedQuad(0.0f, 0.0f, static_cast<float>(fbW),
                             static_cast<float>(fbH), 0.0f, 0.0f, 1.0f, 1.0f,
                             g_imageTex, static_cast<float>(fbW),
                             static_cast<float>(fbH));

            DrawColorBar(globalMin, globalMax, static_cast<float>(fbW),
                         static_cast<float>(fbH));

            if (g_interacting) {
                char fpsBuf[64];
                std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", smoothedFps);
                const float white[3] = {1.0f, 1.0f, 1.0f};
                DrawText(fpsBuf, 24.0f, static_cast<float>(fbH) - 40.0f, 2.0f,
                         white, static_cast<float>(fbW),
                         static_cast<float>(fbH));
            }

            glfwSwapBuffers(rawWindow);
        }
    }

    return 0;
}

} // namespace iGameVolInteractive
