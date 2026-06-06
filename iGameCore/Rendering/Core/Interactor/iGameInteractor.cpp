#include "iGameInteractor.h"
#include "iGameBasicStyle.h"
#include "iGameScene.h"
#include "iGameSingleDragStyle.h"
#include "iGameSingleSelectionStyle.h"

#include <iostream>

IGAME_NAMESPACE_BEGIN

Interactor::Interactor() {
    is_Base = true;
    m_Internal = nullptr;
    m_Scene = nullptr;
    m_Camera = nullptr;
    m_Painter3D = nullptr;
    m_DataObject = nullptr;
}

Interactor::~Interactor() {
    std::cout << "[iGameDestroy] Interactor::~Interactor this=" << this << '\n';
}

void Interactor::Finalize() {
    std::cout << "[iGameDestroy] Interactor::Finalize this=" << this << '\n';
    m_Internal = nullptr;
    m_SpecialInternals.clear();
    m_DataObject = nullptr;
    m_Painter3D = nullptr;
    m_Camera = nullptr;
    m_Scene = nullptr;
}

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
    if (!m_Internal) {
        std::cout << "FilterEvent: Creating default style" << std::endl;
        CreateDefaultStyle();
    }
    //先考虑特殊交互器，再考虑普通交互器
    //请勿改变这个顺序，否则会导致Selection的Box出错
    for (auto& specialInternal: m_SpecialInternals)
        specialInternal.second->FilterEvent(event);
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

void Interactor::RequestPickCenterStyle(SmartPointer<Selection> s) {
    if (!s) return;

    // 创建并初始化中心点选择交互样式
    auto act = SingleSelectionStyle::New();

    // 设置为点选择类型（只需要选择单个点作为中心）
    act->SetSelectedType(SelectionStyle::SelectedType::SelectPoint);

    // 添加特殊标志表示这是中心点选择模式
    /*act->SetIsCenterPickMode(true);*/

    // 初始化交互样式并设置为当前活动交互器
    act->Initialize(this, s);
    m_Internal = act;
    is_Base = false;
}

// 在Interactor.cpp中添加实现
void Interactor::RequestDragCenterStyle(SmartPointer<Selection> s) {
    if (!m_Scene) return;

    // 创建并设置拖拽交互器
    auto act = DragCenterStyle::New();
    act->Initialize(this);
    auto model = m_Scene->GetCenterAxesModel();
    act->SetAxesModel(model);
    // 设置为当前交互器
    m_Internal = act;
    is_Base = false;
}

void Interactor::LoadSelectionStyleRequired(SmartPointer<Selection> s) {
    if (!m_Internal) { return; }
    SmartPointer<SelectionStyle> act;
    if ((act = DynamicCast<SelectionStyle>(m_Internal)) == nullptr) { return; }
    act->Initialize(this, s);
}

//void Interactor::RequestSlicingStyle() {
//    auto act = SlicingStyle::New();
//    //InitModel();
//    act->Initialize(this);
//    m_Internal = act;
//    is_Base = false;
//}
void Interactor::RequestSlicingStyle(SmartPointer<Selection> s) {
    auto act = SlicingStyle::New();
    //InitModel();
    act->Initialize(this, s);
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

std::string Interactor::_SetSpecialInteractor(
        const std::string& interactorName,
        SmartPointer<InteractorStyle> interactorStyle) {
    if (interactorStyle.IsNull()) return interactorName;
    m_SpecialInternals[interactorName] = interactorStyle;
    return interactorName;
}

void Interactor::RemoveSepcialInteractor(const std::string& interactorName) {
    m_SpecialInternals.erase(interactorName);
}

float Interactor::GetWidth() const { return m_Camera->GetViewPort().x; }

float Interactor::GetHeight() const { return m_Camera->GetViewPort().y; }

igm::mat4 Interactor::GetMVP() const {
    return m_Scene->m_Camera->GetProjectionMatrix() *
           m_Scene->m_Camera->GetViewMatrix() * m_Scene->m_ModelMatrix;
}

Scene* Interactor::GetScene() { return m_Scene.get(); }

Camera* Interactor::GetCamera() { return m_Camera.get(); }

void Interactor::SetDataObject(SmartPointer<DataObject> obj) {
    m_DataObject = obj;
}

SmartPointer<DataObject> Interactor::GetDataObject() { return m_DataObject; }

void Interactor::SetPainter3D(SmartPointer<Painter3D> p) { m_Painter3D = p; }

SmartPointer<Painter3D> Interactor::GetPainter3D() { return m_Painter3D; }

SmartPointer<InteractorStyle>
Interactor::GetSpecialInteractor(const std::string& interactorName) {
    if (m_SpecialInternals.count(interactorName) == 0) return nullptr;
    return m_SpecialInternals.at(interactorName);
}

bool Interactor::HaveSpecialInteractor(const std::string& interactorName) {
    return m_SpecialInternals.count(interactorName) != 0;
}

bool Interactor::IsBasicStyle() const { return is_Base; }

IGAME_NAMESPACE_END
