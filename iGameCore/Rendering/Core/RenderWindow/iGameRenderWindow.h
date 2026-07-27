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

    void RenderOneFrame();

    void Show();

    void SetSize(int width, int height);

    void SetTitle(const char* title);

    void SetTitle(const std::string& title);

    Scene* GetScene();
    /*Gets a window pointer to the original GLFW third-party library for custom actions
     * (currently only used for multi-window management)*/
    GLFWwindow* GetRawWindowPtr();

protected:
    RenderWindow();
    ~RenderWindow() override;

    void ResizeScene();

    GLFWwindow* m_Window;
    std::string m_Title;

    Scene* m_Scene;
    Interactor* m_Interactor;
    IEvent m_Event;

    int m_WindowWidth;
    int m_WindowHeight;
};
IGAME_NAMESPACE_END
