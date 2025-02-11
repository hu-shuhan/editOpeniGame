#include "iGameSceneManager.h"
#include "iGameCommand.h"
#include <format>
#include <iGameRenderingLogger.h>

IGAME_NAMESPACE_BEGIN

SceneManager::SceneManager() {
    Logger::SetLogFile("Rendering_Log.txt");
    Logger::SetLogLevel(Logger::LogLevel::Info);
    m_CurrentScene = nullptr;
}

SceneManager::~SceneManager() {}

SmartPointer<Scene> SceneManager::GetScene(int id) {
    if (id < 0 || id >= m_Scenes.size()) { return nullptr; }
    return m_Scenes[id];
}

SmartPointer<Scene> SceneManager::NewScene() {
    SmartPointer<Scene> scene = Scene::New();

    m_CurrentScene = scene;
    m_CurrentScene->SetName(std::format("Scene{}", m_Scenes.size()));
    m_Scenes.push_back(std::move(scene));
    return m_CurrentScene;
}

void SceneManager::DeleteScene(int id) {
    if (id < 0 || id >= m_Scenes.size()) { return; }
    m_Scenes[id] = nullptr;
}

void SceneManager::DeleteScene(SmartPointer<Scene> p) {
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

void SceneManager::MakeCurrentScene(SmartPointer<Scene> p) {
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