/**
 * @class   iGameRenderWindow
 * @brief   iGameRenderWindow's brief
 */

#include "iGameRenderWindow.h"

#include "iGameInteractor.h"
#include "iGameMultiRenderWindowManager.h"
#include "iGameScene.h"

#include <GLFW/glfw3.h>
#include <iostream>
#ifdef __EMSCRIPTEN__
    #include <emscripten/html5_webgl.h>
#endif

IGAME_NAMESPACE_BEGIN

iGame::RenderWindow::RenderWindow() {
    m_Window = nullptr;
    m_Title = "iGameVis - GLFW_OpenGL";

    m_Scene = nullptr;
    m_Interactor = nullptr;

    m_WindowWidth = 800;
    m_WindowHeight = 600;

    /* init glfw */
    if (!glfwInit()) { std::cout << "INIT GLFW ERROR\n"; }
    // 设置 GLFW 错误回调
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    });
    /* set glfw version */
#ifdef IGAME_OPENGL_VERSION_330
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#elif IGAME_OPENGL_VERSION_GLES2
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif IGAME_OPENGL_VERSION_460
    #ifdef IGAME_PLATFORM_WINDOWS
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    #elif IGAME_PLATFORM_LINUX
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    #endif
#endif
    /* set glfw to core profile */
#ifndef IGAME_OPENGL_VERSION_GLES2
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, m_Title.c_str(),
                                NULL, NULL);
    if (m_Window == nullptr) {
        std::cout << "GLFW NULLPTR\n";
        return;
    }
    /* set main window */
    glfwMakeContextCurrent(m_Window);
#ifdef __EMSCRIPTEN__
    emscripten_webgl_enable_extension(emscripten_webgl_get_current_context(),
                                      "OES_vertex_array_object");
    emscripten_webgl_enable_extension(emscripten_webgl_get_current_context(),
                                      "OES_element_index_uint");
#endif
    /* set user pointer to use object in GLFW recall function. */
    glfwSetWindowUserPointer(m_Window, this);
    /* set framebuffer recall */
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width,
                                                int height) {
        auto* this_window =
                static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if (!this_window) return;
        this_window->ResizeScene();
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button,
                                            int action, int height) {
        auto* this_window =
                static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if (this_window == nullptr || this_window->m_Interactor == nullptr)
            return;
        IEvent event;
        switch (action) {
            case GLFW_PRESS:
                event.type = IEvent::MousePress;
                break;
            case GLFW_RELEASE:
                event.type = IEvent::MouseRelease;
                break;
            default:
                break;
        }
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                event.button = MouseButton::LeftButton;
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                event.button = MouseButton::MiddleButton;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                event.button = MouseButton::RightButton;
                break;
        }
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        event.pos.x = static_cast<float>(xpos);
        event.pos.y = static_cast<float>(ypos);

        this_window->m_Interactor->FilterEvent(event);
    });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xoffset,
                                          double yoffset) {
        auto* this_window =
                static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if (this_window == nullptr || this_window->m_Interactor == nullptr)
            return;
        auto M_Event = this_window->m_Event;
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        M_Event.pos.x = static_cast<float>(xpos);
        M_Event.pos.y = static_cast<float>(ypos);
        M_Event.type = IEvent::MouseMove;
        this_window->m_Interactor->FilterEvent(M_Event);
    });

    glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset,
                                       double yoffset) {
        auto* this_window =
                static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if (this_window == nullptr || this_window->m_Interactor == nullptr)
            return;
        auto M_Event = this_window->m_Event;
        M_Event.type = IEvent::Wheel;
        M_Event.delta = 120 * yoffset;
        this_window->m_Interactor->FilterEvent(M_Event);
    });
}

iGame::RenderWindow::~RenderWindow() {
    std::cout << "[iGameDestroy] RenderWindow::~RenderWindow this=" << this << " window=" << m_Window << '\n';
    if (m_Window != nullptr) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    m_Scene = nullptr;
    m_Interactor = nullptr;
}

void iGame::RenderWindow::RenderOneFrame() {
    if (m_Window == nullptr) return;
    glfwMakeContextCurrent(m_Window);
    if (m_Scene) { m_Scene->Draw(); }
    glfwSwapBuffers(m_Window);
    glfwPollEvents();
}

void iGame::RenderWindow::Show() {
    if (m_Window == nullptr) return;
    while (!glfwWindowShouldClose(m_Window)) { RenderOneFrame(); }
}

void iGame::RenderWindow::SetScene(iGame::Scene* _scene) {
    m_Scene = _scene;
    if (m_Window == nullptr || m_Scene == nullptr) return;
    glfwMakeContextCurrent(m_Window);
    m_Scene->Initialize();
    ResizeScene();
}

void iGame::RenderWindow::SetInteractor(iGame::Interactor* _interactor) {
    m_Interactor = _interactor;
    m_Event = IEvent();
}


void iGame::RenderWindow::ResizeScene() {
    if (m_Window == nullptr) return;
    glfwMakeContextCurrent(m_Window);
    glfwGetWindowSize(m_Window, &m_WindowWidth, &m_WindowHeight);
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(m_Window, &frameBufferWidth, &frameBufferHeight);
    if (m_Scene == nullptr || m_WindowWidth == 0) return;
    int pixelRatio = frameBufferWidth / m_WindowWidth;
    m_Scene->Resize(m_WindowWidth, m_WindowHeight, pixelRatio);
}

void iGame::RenderWindow::SetSize(int width, int height) {
    if (m_Window == nullptr) return;
    glfwSetWindowSize(m_Window, width, height);
    ResizeScene();
}

void iGame::RenderWindow::SetTitle(const char* title) {
    if (m_Window == nullptr) return;
    m_Title = title;
    glfwSetWindowTitle(m_Window, m_Title.c_str());
}

void iGame::RenderWindow::SetTitle(const std::string& title) {
    if (m_Window == nullptr) return;
    m_Title = title;
    glfwSetWindowTitle(m_Window, m_Title.c_str());
}

Scene* RenderWindow::GetScene() { return m_Scene; }

GLFWwindow* RenderWindow::GetRawWindowPtr() { return m_Window; }

IGAME_NAMESPACE_END
