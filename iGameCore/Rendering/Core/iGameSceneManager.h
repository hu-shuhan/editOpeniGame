#pragma once

#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN
class SceneManager : public Object {
public:
    I_OBJECT(SceneManager);
    static Pointer Instance() {
        static Pointer ins = new SceneManager;
        return ins;
    }

    Scene::Pointer NewScene();
    Scene::Pointer GetScene(int id);

    void DeleteScene(int id);
    void DeleteScene(Scene::Pointer p);

    void MakeCurrentScene(int id);
    void MakeCurrentScene(Scene::Pointer p);

    Scene* GetCurrentScene() { return m_CurrentScene; }

protected:
    SceneManager();
    ~SceneManager() override;

    std::vector<Scene::Pointer> m_Scenes;
    Scene* m_CurrentScene;
};

IGAME_NAMESPACE_END
