#ifndef OPENIGAME_SCENCE_MANAGER_H
#define OPENIGAME_SCENCE_MANAGER_H

#include "iGameScene.h"
#include "iGameCommand.h"

IGAME_NAMESPACE_BEGIN
class SceneManager : public Object {
public:
    I_OBJECT(SceneManager);
    static Pointer Instance() {
        static Pointer ins = new SceneManager;
        return ins;
    }

    Scene::Pointer GetScene(int id);
    Scene::Pointer NewScene();
//    Scene::Pointer NewScene(void*(*proc) ){
//        Scene::Pointer scene = Scene::New();
//        scene->m_proc = proc;
//        scene->init();
//        m_Scenes.push_back(scene);
//        m_CurrentScene = scene;
//        return scene;
//    };
    void DeleteScene(int id);
    void DeleteScene(Scene::Pointer p);

    void MakeCurrentScene(int id);
    void MakeCurrentScene(Scene::Pointer p);

    Scene* GetCurrentScene() { return m_CurrentScene;}
    
protected:
    SceneManager();
    ~SceneManager() override;

    std::vector<Scene::Pointer> m_Scenes;
    Scene* m_CurrentScene{ nullptr };
};

//class SceneObserver : public Command {
//public:
//    I_OBJECT(SceneObserver);
//    static Pointer New() { return new SceneObserver; }
//
//    void Execute(Object* caller, unsigned long eventId, void* callData) override
//    {
//        switch (eventId)
//        {
//        case Command::ChangeSceneEvent:
//            ChangeScene(caller, eventId, callData);
//            break;
//        case Command::ChangeScalarNameEvent:
//            {
//                //std::vector<Actor::ScalarItem>* name = static_cast<std::vector<Actor::ScalarItem>*>(callData);
//                //ChangeScalarName(*name);
//            }
//            break;
//        default:
//            break;
//        }
//    };
//
//    void ChangeScene(Object* caller, unsigned long eventId, void* callData)
//    {
//        m_CurrentScene = m_SceneManager->GetCurrentScene();
//        m_CurrentScene->AddObserver(Command::ChangeScalarNameEvent, this);
//    }
//
//    virtual void ChangeScalarName()
//    {
//
//    }
//
//protected:
//    SceneObserver()
//    {
//        m_SceneManager = SceneManager::Instance();
//        m_SceneManager->AddObserver(Command::ChangeSceneEvent, this);
//    }
//    ~SceneObserver() override = default;
//
//    Scene* m_CurrentScene;
//    SceneManager::Pointer m_SceneManager;
//};

IGAME_NAMESPACE_END

#endif // OPENIGAME_SCENCE_H
