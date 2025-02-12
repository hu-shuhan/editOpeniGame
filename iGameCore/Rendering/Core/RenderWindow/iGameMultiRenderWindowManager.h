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

    void Register(GLFWwindow* _win_ptr);

    void ShowAllRegisterWindow();

protected:
    MultiRenderWindowManager();
    ~MultiRenderWindowManager() override;

    void Register(RenderWindow* _win_ptr);

    std::vector<GLFWwindow*> m_GLFW_WindowPointerPool;
};

IGAME_NAMESPACE_END
