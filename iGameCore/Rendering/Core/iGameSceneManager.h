/**
 * @class    SceneManager
 * @brief    SceneManager类用于管理游戏中的多个场景。
 *
 * SceneManager提供了创建、获取、删除和切换场景的功能，并维护一个当前活动场景。
 * 该类采用单例模式（Singleton）设计，保证全局仅有一个SceneManager实例。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameScene.h"

IGAME_NAMESPACE_BEGIN

class SceneManager : public Object {
public:
    I_OBJECT(SceneManager);

    /**
     * @brief 获取SceneManager的单例实例。
     * @return SceneManager单例指针。
     */
    static Pointer Instance() {
        static Pointer ins = new SceneManager;
        return ins;
    }

    /**
     * @brief 创建一个新场景并返回其指针。
     * @return 新创建场景的智能指针。
     */
    Scene::Pointer NewScene();

    /**
     * @brief 根据场景ID获取对应的场景。
     * @param id 场景ID。
     * @return 场景的智能指针。如果不存在对应ID的场景，则返回nullptr。
     */
    Scene::Pointer GetScene(int id);

    /**
     * @brief 删除指定ID的场景。
     * @param id 场景ID。
     */
    void DeleteScene(int id);

    /**
     * @brief 删除指定的场景。
     * @param p 场景的智能指针。
     */
    void DeleteScene(Scene::Pointer p);

    /**
     * @brief 将指定ID的场景设为当前活动场景。
     * @param id 场景ID。
     */
    void MakeCurrentScene(int id);

    /**
     * @brief 将指定的场景设为当前活动场景。
     * @param p 场景的智能指针。
     */
    void MakeCurrentScene(Scene::Pointer p);

    /**
     * @brief 获取当前活动的场景。
     * @return 当前活动场景的原始指针。
     */
    Scene* GetCurrentScene() { return m_CurrentScene; }

protected:
    SceneManager();
    ~SceneManager() override;

    std::vector<Scene::Pointer> m_Scenes;
    Scene* m_CurrentScene;
};

IGAME_NAMESPACE_END
