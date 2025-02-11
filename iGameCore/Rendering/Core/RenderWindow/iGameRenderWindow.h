/**
 * @class   iGameRenderWindow
 * @brief   iGameRenderWindow's brief
 */

#pragma once

#include <iGameInteractorStyle.h>
#include <iGameObject.h>

class GLFWwindow;
IGAME_NAMESPACE_BEGIN

class Scene;
class Interactor;
class RenderWindow : public Object {
public:
    I_OBJECT(RenderWindow)

    static Pointer New() { return new RenderWindow; }

    void SetScene(Scene* scene);

    void SetInteractor(Interactor* _interactor);

    void Show();

    void SetSize(int width, int height);

    void SetTitle(const char* title);

    void SetTitle(const std::string& title);

    Scene* GetScene();
    /*Gets a window pointer to the original GLFW third-party library for custom actions
     * (currently only used for multi-window management)*/
    GLFWwindow* GetRawWindowPtr();

protected:
    GLFWwindow* m_window{nullptr};
    std::string m_title{"iGameVis - GLFW_OpenGL"};

    Scene* m_scene{nullptr};
    Interactor* m_Interactor{nullptr};
    IEvent m_Event;

    int m_window_width{800}, m_window_height{600};

protected:
    void ResizeScene();

private:
protected:
    RenderWindow();
    ~RenderWindow();
};
IGAME_NAMESPACE_END
