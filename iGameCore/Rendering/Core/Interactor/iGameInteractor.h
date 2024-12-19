//
// Created by Sumzeek on 9/9/2024.
//

#ifndef OPENIGAME_INTERACTOR_H
#define OPENIGAME_INTERACTOR_H

#include "iGameBasicStyle.h"
#include "iGameInteractorStyle.h"
#include "iGameMultiSelectionStyle.h"
#include "iGameScene.h"
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

    void Initialize(Scene::Pointer scene);

    void CreateDefaultStyle();

    void FilterEvent(IEvent _event);

    void RequestBasicStyle();

    void RequestDragPointStyle(Selection* s);

    void RequestPointSelectionStyle(Selection* s);

    void RequestFaceSelectionStyle(Selection* s);

    void LoadSelectionStyleRequired(Selection* s);

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

    void SetDataObject(DataObject::Pointer obj);
    DataObject::Pointer GetDataObject();
    void SetPainter(Painter3D::Pointer p);
    Painter3D::Pointer GetPainter();
    bool IsBase() const;

protected:
    Interactor() = default;
    ~Interactor() override = default;

    std::function<void(InteractorStyle::Signal, void*)> m_CallBack;

    bool is_Base{true};
    InteractorStyle::Pointer m_Internal{};
    Scene::Pointer m_Scene{};
    Camera::Pointer m_Camera{};
    Painter3D::Pointer m_Painter{};
    DataObject::Pointer m_DataObject{};
};

IGAME_NAMESPACE_END

#endif //OPENIGAME_INTERACTOR_H
