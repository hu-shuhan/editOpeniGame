//
// Created by Sumzeek on 9/9/2024.
//

#ifndef IGAMEVIS_INTERACTOR_H
#define IGAMEVIS_INTERACTOR_H

#include "iGameBasicStyle.h"
#include "iGameInteractorStyle.h"
#include "iGameMultiSelectionStyle.h"
#include "iGameSingleDragStyle.h"
#include "iGameSingleSelectionStyle.h"
#include "iGameSlicingStyle.h"
#include "iGameStreamLineStyle.h"

IGAME_NAMESPACE_BEGIN

class Interactor : public Object {
public:
    I_OBJECT(Interactor);
    static Pointer New() { return new Interactor; }

    // 交互风格
    enum Style {
        BasicStyle = 0,           // 基础
        SinglePointSelectionStyle,// 点选
        SingleFaceSelectionStyle, // 面选
        MultiPointSelectionStyle, // 多个点选
        MultiFaceSelectionStyle,  // 多个面选
        DragPointStyle,           // 点拖动
        SlicingStyle,             // 切片
        StreamLine,               // 流形的线
    };

    /**
     * @brief 初始化，需要绑定一个场景
     * @param scene 渲染坐标轴的目标场景
     */
    void Initialize(SmartPointer<Scene> scene);

    /**
     * @brief 创建一个默认的交互风格
     */
    void CreateDefaultStyle();

    /**
     * @brief 响应事件，传入一个事件
     * @param event 事件
     */
    void FilterEvent(IEvent event);

    /**
     * @brief 切换成基础风格类型交互器
     */
    void RequestBasicStyle();

    /**
     * @brief 切换成点拖选风格类型交互器
     * @param Selection s 事件响应后将会通知的对象
     */
    void RequestDragPointStyle(SmartPointer<Selection> s);

    /**
     * @brief 切换成点选风格类型交互器
     * @param Selection s 事件响应后将会通知的对象
     */
    void RequestPointSelectionStyle(SmartPointer<Selection> s);

    /**
     * @brief 切换成面选风格类型交互器
     * @param Selection s 事件响应后将会通知的对象
     */
    void RequestFaceSelectionStyle(SmartPointer<Selection> s);

    /**
     * @brief 将s绑定到交互器上
     * @param Selection s 事件响应后将会通知的对象
     */
    void LoadSelectionStyleRequired(SmartPointer<Selection> s);

    /**
     * @brief 切换成切片风格类型交互器
     */
    void RequestSlicingStyle();

     /**
     * @brief 切换成流形的线类型交互器
     * @param Selection s 事件响应后将会通知的对象
     */
    void RequestStreamLineStyle(SmartPointer<Selection> s);

    bool IsBasicStyle() const;

    /**
     * @brief 获取场景的一些信息
     */
    float GetWidth() const;
    float GetHeight() const;
    igm::mat4 GetMVP() const;

    Scene* GetScene();
    Camera* GetCamera();

    template<typename Functor, typename... Args>
    void SetCallBack(Functor&& functor, Args&&... args) {
        this->m_CallBack = std::bind(
                std::forward<Functor>(functor), std::forward<Args>(args)...,
                std::placeholders::_1, std::placeholders::_2);
    }

    void RequestSignal(InteractorStyle::Signal signal, void* callData);

    void SetDataObject(SmartPointer<DataObject> obj);
    SmartPointer<DataObject> GetDataObject();
    void SetPainter3D(SmartPointer<Painter3D> p);
    SmartPointer<Painter3D> GetPainter3D();

protected:
    Interactor();
    ~Interactor() override;

    std::function<void(InteractorStyle::Signal, void*)> m_CallBack;

    bool is_Base;
    SmartPointer<InteractorStyle> m_Internal;
    SmartPointer<Scene> m_Scene;
    SmartPointer<Camera> m_Camera;
    SmartPointer<Painter3D> m_Painter3D;
    SmartPointer<DataObject> m_DataObject;
};

IGAME_NAMESPACE_END

#endif //IGAMEVIS_INTERACTOR_H
