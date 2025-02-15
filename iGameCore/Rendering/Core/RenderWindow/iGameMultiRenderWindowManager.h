//
// Created by m_ky on 2024/11/5.
//

/**
 * @class   iGameRenderWindowManager
 * @brief   iGameRenderWindowManager's brief
 */
#pragma once
#include "iGameObject.h"

class GLFWwindow;
class GLFWmonitor;
IGAME_NAMESPACE_BEGIN

class RenderWindow;
class MultiRenderWindowManager : public Object {
public:
    I_OBJECT(MultiRenderWindowManager)

    static MultiRenderWindowManager* Instance() {
        static MultiRenderWindowManager manager = MultiRenderWindowManager();
        return &manager;
    }

    void CreateWindow(int width, int height, const char* title,
                      GLFWmonitor* monitor = nullptr,
                      GLFWwindow* share = nullptr);

    void ShowAllRegisterWindow();

    /**
     * @brief 在多窗口管理中注册该窗口
     */
    void Register(GLFWwindow* _win_ptr);
    /**
     * @brief 在多窗口管理中注册该窗口
     */
    void Register(RenderWindow* _win_ptr);
protected:
    std::vector<GLFWwindow* > m_GLFW_WindowPointerPool;


protected:
    MultiRenderWindowManager() = default;
    ~MultiRenderWindowManager() = default;


};

IGAME_NAMESPACE_END
