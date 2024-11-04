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

protected:
    GLFWwindow* m_window;

    Scene* m_scene;
    Interactor* m_Interactor;
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
