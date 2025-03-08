#include "iGameInteractor.h"
#include "iGameBasicStyle.h"
#include "iGameScene.h"
#include "iGameSingleDragStyle.h"
#include "iGameSingleSelectionStyle.h"

IGAME_NAMESPACE_BEGIN

Interactor::Interactor() {
    is_Base = true;
    m_Internal = nullptr;
    m_Scene = nullptr;
    m_Camera = nullptr;
    m_Painter3D = nullptr;
    m_DataObject = nullptr;
}

Interactor::~Interactor() {}

void Interactor::Initialize(SmartPointer<Scene> scene) {
    if (scene) {
        m_Scene = scene;
        m_Camera = m_Scene->m_Camera;
        CreateDefaultStyle();
    }
}

void Interactor::CreateDefaultStyle() {
    auto style = BasicStyle::New();
    style->Initialize(this);
    m_Internal = style;
}

void Interactor::FilterEvent(IEvent event) {
    if (m_Scene == nullptr) return;
    if (!m_Internal) { CreateDefaultStyle(); }
    m_Internal->FilterEvent(event);
}

void Interactor::RequestBasicStyle() {
    //InitModel();
    m_Internal = BasicStyle::New();
    m_Internal->Initialize(this);
    is_Base = true;
}

void Interactor::RequestDragPointStyle(SmartPointer<Selection> s) {
    if (!s) return;
    //InitModel();
    auto act = SingleDragStyle::New();
    act->SetSelectedType(SelectionStyle::SelectedType::SelectPoint);
    act->Initialize(this, s);
    m_Internal = act;
    is_Base = false;
}

void Interactor::RequestPointSelectionStyle(SmartPointer<Selection> s) {
    if (!s) return;
    //InitModel();
    auto act = SingleSelectionStyle::New();
    act->SetSelectedType(SelectionStyle::SelectedType::SelectPoint);
    act->Initialize(this, s);
    m_Internal = act;
    is_Base = false;
}

void Interactor::RequestFaceSelectionStyle(SmartPointer<Selection> s) {
    if (!s) return;
    //InitModel();
    auto act = SingleSelectionStyle::New();
    act->SetSelectedType(SelectionStyle::SelectedType::SelectCell);
    act->Initialize(this, s);
    m_Internal = act;
    is_Base = false;
}

void Interactor::LoadSelectionStyleRequired(SmartPointer<Selection> s) {
    if (!m_Internal) { return; }
    SmartPointer<SelectionStyle> act;
    if ((act = DynamicCast<SelectionStyle>(m_Internal)) = nullptr) { return; }
    act->Initialize(this, s);
}

void Interactor::RequestSlicingStyle() {
    auto act = SlicingStyle::New();
    //InitModel();
    act->Initialize(this);
    m_Internal = act;
    is_Base = false;
}

void Interactor::RequestStreamLineStyle(SmartPointer<Selection> s) {
    if (!s) return;
    //InitModel();
    auto act = StreamLineStyle::New();
    act->Initialize(this, s);
    m_Internal = act;
    is_Base = false;
}

float Interactor::GetWidth() const { return m_Camera->GetViewPort().x; }

float Interactor::GetHeight() const { return m_Camera->GetViewPort().y; }

igm::mat4 Interactor::GetMVP() const {
    return m_Scene->m_Camera->GetProjectionMatrix() *
           m_Scene->m_Camera->GetViewMatrix() * m_Scene->m_ModelMatrix;
}

Scene* Interactor::GetScene() { return m_Scene.get(); }

Camera* Interactor::GetCamera() { return m_Camera.get(); }

void Interactor::RequestSignal(InteractorStyle::Signal signal, void* callData) {
    if (m_CallBack) { m_CallBack(signal, callData); }
}

void Interactor::SetDataObject(SmartPointer<DataObject> obj) {
    m_DataObject = obj;
}

SmartPointer<DataObject> Interactor::GetDataObject() { return m_DataObject; }

void Interactor::SetPainter3D(SmartPointer<Painter3D> p) { m_Painter3D = p; }

SmartPointer<Painter3D> Interactor::GetPainter3D() { return m_Painter3D; }

bool Interactor::IsBasicStyle() const { return is_Base; }

IGAME_NAMESPACE_END
