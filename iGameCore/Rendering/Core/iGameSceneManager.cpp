#include "iGameSceneManager.h"
#include "iGameCommand.h"

IGAME_NAMESPACE_BEGIN

SceneManager::SceneManager() { m_CurrentScene = nullptr; }

SceneManager::~SceneManager() {}

Scene::Pointer SceneManager::GetScene(int id) {
    if (id < 0 || id >= m_Scenes.size()) { return nullptr; }
    return m_Scenes[id];
}

Scene::Pointer SceneManager::NewScene() {
    Scene::Pointer scene = Scene::New();
    m_Scenes.push_back(scene);
    m_CurrentScene = scene;
    return scene;
}

void SceneManager::DeleteScene(int id) {
    if (id < 0 || id >= m_Scenes.size()) { return; }
    m_Scenes[id] = nullptr;
}

void SceneManager::DeleteScene(Scene::Pointer p) {
    if (p == nullptr) { return; }
    for (int i = 0; i < m_Scenes.size(); i++) {
        if (p == m_Scenes[i]) {
            m_Scenes[i] = nullptr;
            return;
        }
    }
}

void SceneManager::MakeCurrentScene(int id) {
    if (id < 0 || id >= m_Scenes.size() || m_Scenes[id] == nullptr) { return; }
    m_CurrentScene = m_Scenes[id];
}

void SceneManager::MakeCurrentScene(Scene::Pointer p) {
    if (p == nullptr) { return; }
    for (int i = 0; i < m_Scenes.size(); i++) {
        if (p == m_Scenes[i]) {
            m_CurrentScene = m_Scenes[i];
            return;
        }
    }
    m_CurrentScene = nullptr;
}
IGAME_NAMESPACE_END