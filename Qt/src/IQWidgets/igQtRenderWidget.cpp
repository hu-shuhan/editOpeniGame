//
// Created by m_ky on 2024/4/18.
//

/**
 * @class   iGameQtGLFWWindow
 * @brief   iGameQtGLFWWindow's brief
 */
#include "iGameInteractor.h"
#include "iGameSceneManager.h"

#include <IQWidgets/igQtRenderWidget.h>
#include <QMouseEvent>
#include <iGamePointSet.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <qdebug.h>

igQtRenderWidget::igQtRenderWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, false);

    setMouseTracking(true);
    setMinimumHeight(185);
    setMinimumWidth(320);
}

igQtRenderWidget::~igQtRenderWidget() {
    makeCurrent();
    SceneManager::Pointer sceneManager = SceneManager::Instance();
    sceneManager->DeleteScene(m_Scene);
    m_Scene = nullptr;
    doneCurrent();
}

Scene* igQtRenderWidget::GetScene() { return m_Scene; }

void igQtRenderWidget::AddDataObject(SmartPointer<DataObject> obj) {
    //m_Scene->AddDataObject(obj);
    //Q_EMIT AddDataObjectToModelList(QString::fromStdString(obj->GetName()));
    //update();
}

void igQtRenderWidget::ChangeInteractor(SmartPointer<Interactor> it) {
    m_Interactor = it;
    m_Interactor->Initialize(m_Scene);
    m_Scene->SetInteractor(m_Interactor);
}

void igQtRenderWidget::ChangeInteractorStyle(IGenum style) {

    switch (style) {
        case Interactor::BasicStyle:
            m_Interactor->RequestBasicStyle();
            break;
        case Interactor::SinglePointSelectionStyle: {
            auto* s = m_Scene->GetCurrentModel()->GetSelection();
            auto ps = DynamicCast<PointSet>(
                    m_Scene->GetCurrentModel()->GetDataObject());
            if (ps == nullptr) {
                m_Interactor->RequestBasicStyle();
                return;
            }
            s->SetPoints(ps->GetPoints());
            s->SetModel(m_Scene->GetCurrentModel());
            m_Interactor->RequestPointSelectionStyle(s);
        } break;
        case Interactor::SingleFaceSelectionStyle: {
            auto* s = m_Scene->GetCurrentModel()->GetSelection();
            auto model = m_Scene->GetCurrentModel();
            auto obj = model->GetDataObject();
            Points::Pointer points;
            CellArray::Pointer faces;

            if (DynamicCast<VolumeMesh>(obj)) {
                //auto mesh = DynamicCast<VolumeMesh>(obj)->GetDrawMesh();
                auto mesh = DynamicCast<VolumeMesh>(obj);
                points = mesh->GetPoints();
                faces = mesh->GetFaces();
            } else if (DynamicCast<UnstructuredMesh>(obj)) {
                //auto mesh = DynamicCast<UnstructuredMesh>(obj)->GetDrawMesh();
                auto mesh = DynamicCast<UnstructuredMesh>(obj);
                points = mesh->GetPoints();
                faces = mesh->GetCells();
            } else if (DynamicCast<SurfaceMesh>(obj)) {
                auto mesh = DynamicCast<SurfaceMesh>(obj);
                points = mesh->GetPoints();
                faces = mesh->GetFaces();
            }
            if (points == nullptr || faces == nullptr) {
                m_Interactor->RequestBasicStyle();
                return;
            }
            s->SetPoints(points);
            s->SetCells(faces);
            s->SetModel(model);
            m_Interactor->RequestFaceSelectionStyle(s);
        } break;
        case Interactor::MultiPointSelectionStyle:
            //m_Interactor->RequestPointSelectionStyle(m_Scene->GetCurrentModel()->GetSelection());
            break;
        case Interactor::MultiFaceSelectionStyle:
            //m_Interactor->RequestPointSelectionStyle(m_Scene->GetCurrentModel()->GetSelection());
            break;
        case Interactor::DragPointStyle: {
            auto* s = m_Scene->GetCurrentModel()->GetSelection();
            auto ps = DynamicCast<PointSet>(
                    m_Scene->GetCurrentModel()->GetDataObject());
            if (ps == nullptr) {
                m_Interactor->RequestBasicStyle();
                return;
            }
            s->SetPoints(ps->GetPoints());
            s->SetModel(m_Scene->GetCurrentModel());
            m_Interactor->RequestDragPointStyle(s);
        } break;
        default:
            break;
    }
}

void igQtRenderWidget::initializeGL() {
    // 目前当窗口
    SceneManager::Pointer sceneManager = SceneManager::Instance();
    m_Scene = sceneManager->NewScene();
    m_Scene->SetUpdateFunctor(&igQtRenderWidget::update, this);
    m_Scene->SetMakeCurrentFunctor(&igQtRenderWidget::makeCurrent, this);
    m_Scene->SetDoneCurrentFunctor(&igQtRenderWidget::doneCurrent, this);

    m_Interactor = Interactor::New();
    m_Interactor->Initialize(m_Scene);
    m_Scene->SetInteractor(m_Interactor);
}

void igQtRenderWidget::resizeGL(int w, int h) {
    auto ratio = this->devicePixelRatio();
    m_Scene->Resize(width(), height(), ratio);
}

void igQtRenderWidget::paintGL() { m_Scene->Draw(); }


void igQtRenderWidget::mousePressEvent(QMouseEvent* event) {
    IEvent _event;
    switch (event->button()) {
        case Qt::NoButton:
            _event.button = MouseButton::NoButton;
            break;
        case Qt::LeftButton:
            _event.button = MouseButton::LeftButton;
            break;
        case Qt::RightButton:
            _event.button = MouseButton::RightButton;
            break;
        case Qt::MiddleButton:
            _event.button = MouseButton::MiddleButton;
            break;
        default:
            break;
    }
    _event.type = IEvent::MousePress;
    _event.pos.x = event->pos().x();
    _event.pos.y = event->pos().y();
    m_Interactor->FilterEvent(_event);
    update();
}

void igQtRenderWidget::mouseMoveEvent(QMouseEvent* event) {
    IEvent _event;
    _event.type = IEvent::MouseMove;
    _event.pos.x = event->pos().x();
    _event.pos.y = event->pos().y();
    m_Interactor->FilterEvent(_event);
    update();
}

void igQtRenderWidget::mouseReleaseEvent(QMouseEvent* event) {
    IEvent _event;
    _event.type = IEvent::MouseRelease;
    _event.pos.x = event->pos().x();
    _event.pos.y = event->pos().y();
    m_Interactor->FilterEvent(_event);
    update();
}

void igQtRenderWidget::wheelEvent(QWheelEvent* event) {
    IEvent _event;
    _event.type = IEvent::Wheel;
    _event.delta = event->delta();
    m_Interactor->FilterEvent(_event);
    update();
}