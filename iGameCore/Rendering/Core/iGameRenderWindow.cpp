/**
 * @class   iGameRenderWindow
 * @brief   iGameRenderWindow's brief
 */

#include "iGameRenderWindow.h"

#include "iGameScene.h"
#include "iGameInteractor.h"

#include<GLFW/glfw3.h>
iGame::RenderWindow::RenderWindow() {
    //初始化glfw
    glfwInit();
    //设定glfw版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    //设定glfw为核心状态
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_window_width, m_window_height, "OpeniGame #GLFW_OpenGL# 1", NULL, NULL);
    //  设定主窗口
    glfwMakeContextCurrent(m_window);
    // 设置对象指针，方便在回调中访问
    glfwSetWindowUserPointer(m_window, this);
    // 设置 framebuffer 回调
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto* this_window = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if(!this_window) return;
        // 调用渲染器的 resize 方法
        if(this_window->m_scene != nullptr) this_window->resizeScene();
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int height){
        auto* this_window = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if(this_window == nullptr || this_window->m_Interactor == nullptr) return;
        IEvent event;
        switch (action){
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

    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset){
        auto* this_window = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if(this_window == nullptr || this_window->m_Interactor == nullptr) return;
        auto M_Event = this_window->m_Event;
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        M_Event.pos.x = static_cast<float>(xpos);
        M_Event.pos.y = static_cast<float>(ypos);
        M_Event.type = IEvent::MouseMove;
        this_window->m_Interactor->FilterEvent(M_Event);
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset){
        auto* this_window = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if(this_window == nullptr || this_window->m_Interactor == nullptr) return;
        auto M_Event = this_window->m_Event;
        M_Event.type = IEvent::Wheel;
        M_Event.delta = 120 * yoffset;
        this_window->m_Interactor->FilterEvent(M_Event);
    });
}

iGame::RenderWindow::~RenderWindow() {

}

void iGame::RenderWindow::show() {
    while (!glfwWindowShouldClose(m_window))
    {
        /* Render here */
        if(m_scene) m_scene->Draw();
        /* Swap front and back buffers */
        glfwSwapBuffers(m_window);
        /* Poll for and process events */
        glfwPollEvents();
    }

}

void iGame::RenderWindow::setScene(iGame::Scene *_scene) {
    m_scene = _scene;
    resizeScene();
}

void iGame::RenderWindow::setInteractor(iGame::Interactor *_interactor) {
    m_Interactor = _interactor;
    m_Event = IEvent();

}



void iGame::RenderWindow::resizeScene() {
    glfwGetWindowSize(m_window, &m_window_width, &m_window_height);
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(m_window, &frameBufferWidth, &frameBufferHeight);
    float divicePixelRatio = (float)frameBufferWidth / m_window_width;
    m_scene->Resize(m_window_width, m_window_height, divicePixelRatio);
}
