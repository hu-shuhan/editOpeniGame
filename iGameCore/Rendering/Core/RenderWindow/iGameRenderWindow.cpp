/**
 * @class   iGameRenderWindow
 * @brief   iGameRenderWindow's brief
 */

#include "iGameRenderWindow.h"

#include "iGameMultiRenderWindowManager.h"
#include "iGameScene.h"
#include "iGameInteractor.h"

#include <GLFW/glfw3.h>

IGAME_NAMESPACE_BEGIN

iGame::RenderWindow::RenderWindow() {
    /* init glfw */
    glfwInit();
    /* set glfw version */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    /* set glfw to core profile */
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   m_window = glfwCreateWindow(m_window_width, m_window_height, m_title.c_str(), NULL, NULL);
    /* set main window */
    glfwMakeContextCurrent(m_window);
    /* set user pointer to use object in GLFW recall function. */
    glfwSetWindowUserPointer(m_window, this);
    /* set framebuffer recall */
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto* this_window = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
        if(!this_window) return;
        this_window->resizeScene();
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
    glfwDestroyWindow(m_window);
}

void iGame::RenderWindow::show() {
    while (!glfwWindowShouldClose(m_window))
    {
        glfwMakeContextCurrent(m_window);
        /* Render here */
        if(m_scene) m_scene->Draw();
        /* Swap front and back buffers */
        glfwSwapBuffers(m_window);
        /* Poll for and process events */
        glfwPollEvents();
    }

}

void iGame::RenderWindow::setScene(iGame::Scene *_scene) {
    glfwMakeContextCurrent(m_window);
    m_scene = _scene;
    m_scene->Init();
    resizeScene();
}

void iGame::RenderWindow::setInteractor(iGame::Interactor *_interactor) {
    m_Interactor = _interactor;
    m_Event = IEvent();
}



void iGame::RenderWindow::resizeScene() {
    glfwMakeContextCurrent(m_window);
    glfwGetWindowSize(m_window, &m_window_width, &m_window_height);
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(m_window, &frameBufferWidth, &frameBufferHeight);
    if(m_scene == nullptr) return ;
    int pixelRatio = frameBufferWidth / m_window_width;
    m_scene->Resize(m_window_width, m_window_height, pixelRatio);
}

void iGame::RenderWindow::setSize(int width, int height) {
    glfwSetWindowSize(m_window, width, height);
    resizeScene();
}

void iGame::RenderWindow::setTitle(const char *title) {
    m_title = title;
    glfwSetWindowTitle(m_window, m_title.c_str());
}

void iGame::RenderWindow::setTitle(const std::string &title) {
    m_title = title;
    glfwSetWindowTitle(m_window, m_title.c_str());
}

Scene *RenderWindow::getScene() {
    return m_scene;
}

GLFWwindow *RenderWindow::getRawWindowPtr() {
    return m_window;
}

IGAME_NAMESPACE_END