/**
 * @class   iGameRenderWindow
 * @brief   iGameRenderWindow's brief
 */

#pragma once

#include <iGameObject.h>
#include <iGameInteractorStyle.h>

class GLFWwindow;
IGAME_NAMESPACE_BEGIN

class Scene;
class Interactor;
class RenderWindow : public Object{
public:
    I_OBJECT(RenderWindow)

    static Pointer New(){return new RenderWindow;}

    void setScene(Scene* scene);

    void setInteractor(Interactor* _interactor);

    void show();

    void setSize(int width, int height);

    void setTitle(const char* title);

    void setTitle(const std::string& title);

    Scene* getScene();

    GLFWwindow* getRawWindowPtr();
protected:
    GLFWwindow* m_window{nullptr};
    std::string m_title{"OpeniGame - GLFW_OpenGL"};

    Scene* m_scene {nullptr};
    Interactor* m_Interactor{nullptr};
    IEvent m_Event;

    int m_window_width{800}, m_window_height{600};
protected:
    void resizeScene();

private:

protected:
    RenderWindow();
    ~RenderWindow();
};
IGAME_NAMESPACE_END
