
//
// Created by m_ky on 2024/11/5.
//

/**
 * @class   iGameRenderWindowManager
 * @brief   iGameRenderWindowManager's brief
 */

#include "iGameMultiRenderWindowManager.h"

#include "iGameRenderWindow.h"
#include "iGameScene.h"

#include <GLFW/glfw3.h>
IGAME_NAMESPACE_BEGIN

MultiRenderWindowManager::MultiRenderWindowManager() {}

MultiRenderWindowManager::~MultiRenderWindowManager() {}

void MultiRenderWindowManager::ShowAllRegisterWindow() {
    //    bool shouldAllWindowClose = false;
    //    while (!shouldAllWindowClose)
    //    {
    //        for(GLFWwindow* window_ptr : m_GLFW_WindowPointPool){
    //            /* set current Context */
    //            glfwMakeContextCurrent(window_ptr);
    //            /* If window has Scene, render here */
    //            auto* this_window = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window_ptr));
    //            if(this_window != nullptr && this_window->GetScene() != nullptr){
    //                this_window->GetScene()->Draw();
    //            }
    //            /* Swap front and back buffers */
    //            glfwSwapBuffers(window_ptr);
    //
    //            if(!shouldAllWindowClose) shouldAllWindowClose = glfwWindowShouldClose(window_ptr);
    //        }
    //        /* Poll for and process events */
    //        glfwPollEvents();
    //    }

    while (!m_GLFWWindowPointerPool.empty()) {
        /* check all window should be closed */
        for (auto it = m_GLFWWindowPointerPool.begin();
             it != m_GLFWWindowPointerPool.end();) {
            GLFWwindow* window_ptr = *it;
            if (glfwWindowShouldClose(window_ptr)) {
                glfwDestroyWindow(window_ptr);          // 销毁窗口
                it = m_GLFWWindowPointerPool.erase(it); // 从列表中移除
            } else {
                /* set current Context */
                glfwMakeContextCurrent(window_ptr);
                auto* this_window = static_cast<RenderWindow*>(
                        glfwGetWindowUserPointer(window_ptr));
                if (this_window != nullptr &&
                    this_window->GetScene() != nullptr) {
                    this_window->GetScene()->Draw();
                }
                /* Swap front and back buffers */
                glfwSwapBuffers(window_ptr);
                ++it;
            }
        }

        /* Process all window events */
        glfwPollEvents();
    }

    /* Terminate and clean glfw cache. */
    glfwTerminate();
}

void MultiRenderWindowManager::CreateWindow(int width, int height,
                                            const char* title,
                                            GLFWmonitor* monitor,
                                            GLFWwindow* share) {
    auto* window_ptr = glfwCreateWindow(width, height, title, monitor, share);
    this->Register(window_ptr);
}

void MultiRenderWindowManager::Register(GLFWwindow* _win_ptr) {
    m_GLFWWindowPointerPool.emplace_back(_win_ptr);
}

void iGame::MultiRenderWindowManager::Register(iGame::RenderWindow* _win_ptr) {
    m_GLFWWindowPointerPool.emplace_back(_win_ptr->GetRawWindowPtr());
}

IGAME_NAMESPACE_END
