//
// Created by Sumzeek on 9/9/2024.
//

#ifndef OPENIGAME_INTERACTOR_H
#define OPENIGAME_INTERACTOR_H

#include "iGameScene.h"
#include "iGameInteractorStyle.h"
#include "iGameBasicStyle.h"
#include "iGameSingleSelectionStyle.h"
#include "iGameMultiSelectionStyle.h"
#include "iGameSingleDragStyle.h"
#include "iGameSlicingStyle.h"

IGAME_NAMESPACE_BEGIN

class Interactor : public Object {
public:
    I_OBJECT(Interactor);
    static Pointer New() { return new Interactor; }

    void Initialize(Scene::Pointer scene) { 
        if (scene) {
            m_Scene = scene;
            m_Camera = m_Scene->m_Camera; 
            CreateDefaultStyle();
        }
    }

    void CreateDefaultStyle() { 
        BasicStyle::Pointer style = BasicStyle::New();
        style->Initialize(this);
        m_Internal = style;
    }

    void FilterEvent(IEvent _event) {
        if (!m_Internal) {
            CreateDefaultStyle();
        }
        m_Internal->FilterEvent(_event);
    }

    enum Style {
        BasicStyle = 0, 
        SinglePointSelectionStyle,
        SingleFaceSelectionStyle,
        MultiPointSelectionStyle,
        MultiFaceSelectionStyle,
        DragPointStyle,
        SlicingStyle,
    };

    void RequestBasicStyle() {
        //InitModel();
        m_Internal = BasicStyle::New();
        m_Internal->Initialize(this);
    }

    void RequestDragPointStyle(Selection* s) {
        if (!s) 
            return;
        //InitModel();
        auto act = SingleDragStyle::New();
        act->SetSelectedType(SelectionStyle::SelectedType::SelectPoint);
        act->Initialize(this, s);
        m_Internal = act;
    }

    void RequestPointSelectionStyle(Selection* s) {
        if (!s) return;
        //InitModel();
        auto act = SingleSelectionStyle::New();
        act->SetSelectedType(SelectionStyle::SelectedType::SelectPoint);
        act->Initialize(this, s);
        m_Internal = act;
    }

    void RequestFaceSelectionStyle(Selection* s) {
        if (!s) return;
        //InitModel();
        auto act = SingleSelectionStyle::New();
        act->SetSelectedType(SelectionStyle::SelectedType::SelectCell);
        act->Initialize(this, s);
        m_Internal = act;
    }

    void LoadSelectionStyleRequired(Selection* s) {
        if (!m_Internal) {
            return;
        }
        SelectionStyle::Pointer act;
        if ((act = DynamicCast<SelectionStyle>(m_Internal)) = nullptr) {
            return;
        }
        act->Initialize(this, s);
    }

    void RequestSlicingStyle() {
        auto act = SlicingStyle::New();
        //InitModel();
        act->Initialize(this);
        m_Internal = act;
    }


    float GetWidth() const { return m_Camera->GetViewPort().x; }
    float GetHeight() const { return m_Camera->GetViewPort().y; }
    igm::mat4 GetMVP() const {
        return m_Scene->CameraData().proj_view * m_Scene->ObjectData().model;
    }
    //Model* GetModel() { return m_Model.get(); }
    Scene* GetScene() { return m_Scene.get(); }
    Camera* GetCamera() { return m_Camera.get(); }
    
    template<typename Functor, typename... Args>
    void SetCallBack(Functor&& functor, Args&&... args) {
        this->m_CallBack = std::bind(
                std::forward<Functor>(functor), std::forward<Args>(args)...,
                std::placeholders::_1, std::placeholders::_2);
    }

    void RequestSignal(InteractorStyle::Signal signal, void* callData) {
        if (m_CallBack) { m_CallBack(signal, callData); }
    }

    void SetDataObject(DataObject::Pointer obj) { m_DataObject = obj; }
    DataObject::Pointer GetDataObject() { return m_DataObject; }
    void SetPainter(Painter3D::Pointer p) { m_Painter = p; }
    Painter3D::Pointer GetPainter() { return m_Painter; }

protected:
    Interactor() = default;
    ~Interactor() override = default;

    //void InitModel() {
    //    if (m_Scene) { 
    //        m_Model = m_Scene->GetCurrentModel();
    //    }
    //}

    std::function<void(InteractorStyle::Signal, void*)> m_CallBack;

    InteractorStyle::Pointer m_Internal{};
    Scene::Pointer m_Scene{};
    Camera::Pointer m_Camera{};
    Painter3D::Pointer m_Painter{};
    DataObject::Pointer m_DataObject{};
};

IGAME_NAMESPACE_END

#endif //OPENIGAME_INTERACTOR_H
