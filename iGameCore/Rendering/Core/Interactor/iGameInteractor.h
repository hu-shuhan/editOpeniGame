//
// Created by Sumzeek on 9/9/2024.
//

#ifndef OPENIGAME_INTERACTOR_H
#define OPENIGAME_INTERACTOR_H

#include "iGameBasicStyle.h"
#include "iGameInteractorStyle.h"
#include "iGameMultiSelectionStyle.h"
#include "iGameSingleDragStyle.h"
#include "iGameSingleSelectionStyle.h"
#include "iGameSlicingStyle.h"

IGAME_NAMESPACE_BEGIN

class Interactor : public Object {
public:
    I_OBJECT(Interactor);
    static Pointer New() { return new Interactor; }

    enum Style {
        BasicStyle = 0,
        SinglePointSelectionStyle,
        SingleFaceSelectionStyle,
        MultiPointSelectionStyle,
        MultiFaceSelectionStyle,
        DragPointStyle,
        SlicingStyle,
    };

    void Initialize(SmartPointer<Scene> scene);

    void CreateDefaultStyle();

    void FilterEvent(IEvent event);

    void RequestBasicStyle();

    void RequestDragPointStyle(SmartPointer<Selection> s);

    void RequestPointSelectionStyle(SmartPointer<Selection> s);

    void RequestFaceSelectionStyle(SmartPointer<Selection> s);

    void LoadSelectionStyleRequired(SmartPointer<Selection> s);

    void RequestSlicingStyle();

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
    bool IsBase() const;

protected:
    Interactor();
    ~Interactor();

    std::function<void(InteractorStyle::Signal, void*)> m_CallBack;

    bool is_Base;
    SmartPointer<InteractorStyle> m_Internal;
    SmartPointer<Scene> m_Scene;
    SmartPointer<Camera> m_Camera;
    SmartPointer<Painter3D> m_Painter3D;
    SmartPointer<DataObject> m_DataObject;
};

IGAME_NAMESPACE_END

#endif //OPENIGAME_INTERACTOR_H
