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
#include <QOpenGLFunctions>//

igQtRenderWidget::igQtRenderWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, false);

    setMouseTracking(true);
    setMinimumHeight(185);
    setMinimumWidth(320);
}

igQtRenderWidget::~igQtRenderWidget() {
    makeCurrent();
    iGame::SceneManager::Pointer sceneManager = iGame::SceneManager::Instance();
    sceneManager->DeleteScene(m_Scene);
    m_Scene = nullptr;
    doneCurrent();
}

iGame::Scene* igQtRenderWidget::GetScene() { return m_Scene; }

void igQtRenderWidget::AddDataObject(iGame::SmartPointer<iGame::DataObject> obj) {
    //m_Scene->AddDataObject(obj);
    //Q_EMIT AddDataObjectToModelList(QString::fromStdString(obj->GetName()));
    //update();
}

void igQtRenderWidget::ChangeInteractor(iGame::SmartPointer<iGame::Interactor> it) {
    m_Interactor = it;
    m_Interactor->Initialize(m_Scene);
    m_Scene->SetInteractor(m_Interactor);
}

void igQtRenderWidget::ChangeInteractorStyle(IGenum style, double interactorRadius, bool selectOrUnSelect) {
    if (!m_Scene || !m_Scene->GetCurrentModel()) { return; }
    switch (style) {
        case iGame::Interactor::BasicStyle:
            m_Interactor->RequestBasicStyle();
            break;
        case iGame::Interactor::SinglePointSelectionStyle: {
            auto obj = m_Scene->GetCurrentModel()->GetDataObject();
            if (obj->HasSubDataObject()) {
                auto s = m_Scene->GetCurrentModel()->GetSelection();
                s->SetModel(m_Scene->GetCurrentModel());
                m_Interactor->SetDataObject(obj);
                m_Interactor->SetPainter3D(m_Scene->GetCurrentModel()->GetPainter3D());
                m_Interactor->RequestPointSelectionStyle(s, interactorRadius, selectOrUnSelect);

            } else {
                auto s = m_Scene->GetCurrentModel()->GetSelection();
                auto ps = DynamicCast<iGame::PointSet>(m_Scene->GetCurrentModel()->GetDataObject());
                if (ps == nullptr) {
                    m_Interactor->RequestBasicStyle();
                    return;
                }
                s->SetPoints(ps->GetPoints());
                s->SetModel(m_Scene->GetCurrentModel());
                m_Interactor->SetDataObject(ps);
                m_Interactor->SetPainter3D(m_Scene->GetCurrentModel()->GetPainter3D());
                m_Interactor->RequestPointSelectionStyle(s, interactorRadius, selectOrUnSelect);
            }
        } break;
        case iGame::Interactor::SingleFaceSelectionStyle: {
            auto s = m_Scene->GetCurrentModel()->GetSelection();
            auto model = m_Scene->GetCurrentModel();
            auto obj = model->GetDataObject();
            iGame::Points::Pointer points;
            iGame::CellArray::Pointer faces;

            if (DynamicCast<iGame::VolumeMesh>(obj)) {
                //auto mesh = DynamicCast<VolumeMesh>(obj)->GetDrawMesh();
                auto mesh = DynamicCast<iGame::VolumeMesh>(obj);
                points = mesh->GetPoints();
                faces = mesh->GetFaces();
            } else if (DynamicCast<iGame::UnstructuredMesh>(obj)) {
                //auto mesh = DynamicCast<UnstructuredMesh>(obj)->GetDrawMesh();
                auto mesh = DynamicCast<iGame::UnstructuredMesh>(obj);
                points = mesh->GetPoints();
                faces = mesh->GetCells();
            } else if (DynamicCast<iGame::SurfaceMesh>(obj)) {
                auto mesh = DynamicCast<iGame::SurfaceMesh>(obj);
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
            m_Interactor->SetDataObject(obj);
            m_Interactor->SetPainter3D(m_Scene->GetCurrentModel()->GetPainter3D());
            m_Interactor->RequestFaceSelectionStyle(s, interactorRadius, selectOrUnSelect);
        } break;
        case iGame::Interactor::MultiPointSelectionStyle:
            //m_Interactor->RequestPointSelectionStyle(m_Scene->GetCurrentModel()->GetSelection());
            break;
        case iGame::Interactor::MultiFaceSelectionStyle:
            //m_Interactor->RequestPointSelectionStyle(m_Scene->GetCurrentModel()->GetSelection());
            break;
        case iGame::Interactor::DragPointStyle: {
            auto s = m_Scene->GetCurrentModel()->GetSelection();
            auto ps = DynamicCast<iGame::PointSet>(m_Scene->GetCurrentModel()->GetDataObject());
            if (ps == nullptr) {
                m_Interactor->RequestBasicStyle();
                return;
            }
            s->SetPoints(ps->GetPoints());
            s->SetModel(m_Scene->GetCurrentModel());
            m_Interactor->SetDataObject(ps);
            m_Interactor->SetPainter3D(m_Scene->GetCurrentModel()->GetPainter3D());
            m_Interactor->RequestDragPointStyle(s);
            
        } break;
        //case iGame::Interactor::PickCenterStyle: {
        //    //点选中心
        //    // 标记
        //    this->setProperty("isPickingCenter", true);
        //    setCursor(Qt::CrossCursor);

        //    // 1. 获取当前模型的Selection对象
        //    auto s = m_Scene->GetCurrentModel()->GetSelection();
        //    auto model = m_Scene->GetCurrentModel();
        //    auto obj = model->GetDataObject();
        //    // 2. 提取模型的顶点数据（Points）
        //    iGame::Points::Pointer points;
        //    if (DynamicCast<iGame::VolumeMesh>(obj)) {
        //        points = DynamicCast<iGame::VolumeMesh>(obj)->GetPoints();
        //    } else if (DynamicCast<iGame::UnstructuredMesh>(obj)) {
        //        points = DynamicCast<iGame::UnstructuredMesh>(obj)->GetPoints();
        //    } else if (DynamicCast<iGame::SurfaceMesh>(obj)) {
        //        points = DynamicCast<iGame::SurfaceMesh>(obj)->GetPoints();
        //    }
        //    // 3. 数据有效性检查
        //    if (!points) {
        //        m_Interactor->RequestBasicStyle();
        //        return;
        //    }

        //    s->SetPoints(points);
        //    s->SetModel(model);
        //    m_Interactor->SetDataObject(obj);
        //    m_Interactor->SetPainter3D(model->GetPainter3D());
        //    m_Interactor->RequestPickCenterStyle(s); // 需要实现这个方法
        //    
        //} break;
        case iGame::Interactor::DragCenterStyle: {
            this->setProperty("isDragingCenter", true);
            setCursor(Qt::SizeAllCursor); // 设置拖拽光标

            // 直接激活拖拽交互器，无需Selection对象
            
            m_Interactor->RequestDragCenterStyle(nullptr); 
            /*this->setProperty("isDragingCenter", true);
            auto s = m_Scene->GetCurrentModel()->GetSelection();
            m_Interactor->RequestDragCenterStyle(s);*/
        } break;
        default:
            break;
    }
}

iGame::Interactor* igQtRenderWidget::getInteractor() { return m_Interactor.get(); }

void igQtRenderWidget::initializeGL() {
    //    qDebug() <<"Init GL start";
    // 目前当窗口
    iGame::SceneManager::Pointer sceneManager = iGame::SceneManager::Instance();
    m_Scene = sceneManager->NewScene();
    m_Scene->Initialize();
    m_Scene->SetUpdateFunctor(&igQtRenderWidget::update, this);
    m_Scene->SetMakeCurrentFunctor(&igQtRenderWidget::makeCurrent, this);
    m_Scene->SetDoneCurrentFunctor(&igQtRenderWidget::doneCurrent, this);

    m_Interactor = iGame::Interactor::New();
    m_Interactor->Initialize(m_Scene);
    m_Scene->SetInteractor(m_Interactor);
    //    qDebug() <<"Init GL end";
}

void igQtRenderWidget::resizeGL(int w, int h) {
    auto ratio = this->devicePixelRatio();
    m_Scene->Resize(width(), height(), ratio);
}

void igQtRenderWidget::paintGL() {
    //    qDebug() <<"Paint start";
    m_Scene->Draw();
    //    qDebug() <<"Paint end";
}


void igQtRenderWidget::mousePressEvent(QMouseEvent* event) {

    //if (this->property("isPickingCenter").toBool() && event->button() == Qt::LeftButton) {
    //    // 获取当前中心点的深度
    //    float currentDepth = m_Scene->GetRotationCenterDepth();
    //    std::cout << "depth " << currentDepth << std::endl;

    //    // 获取点击位置的世界坐标
    //    igm::vec3 worldPos = GetWorldPositionFromDepth(event->pos(), currentDepth);
    //    /*QPoint eventPos = event->pos();
    //    igm::vec2 screenPos = igm::vec2(eventPos.x(), eventPos.y());
    //    igm::vec3 worldPos = m_Scene->ScreenToWorld(screenPos, currentDepth);*/
    //    // 设置新的中心点
    //    m_Scene->SetRotationCenter(worldPos);
    //    update();
    //    return;
    //}

    iGame::IEvent _event;
    switch (event->button()) {
        case Qt::NoButton:
            _event.button = iGame::MouseButton::NoButton;
            break;
        case Qt::LeftButton:
            _event.button = iGame::MouseButton::LeftButton;
            break;
        case Qt::RightButton:
            _event.button = iGame::MouseButton::RightButton;
            break;
        case Qt::MiddleButton:
            _event.button = iGame::MouseButton::MiddleButton;
            break;
        default:
            break;
    }
    _event.type = iGame::IEvent::MousePress;
    _event.pos.x = event->pos().x();
    _event.pos.y = event->pos().y();
    m_Interactor->FilterEvent(_event);
    update();
}

void igQtRenderWidget::mouseMoveEvent(QMouseEvent* event) {


    iGame::IEvent _event;
    _event.type = iGame::IEvent::MouseMove;
    _event.pos.x = event->pos().x();
    _event.pos.y = event->pos().y();
    m_Interactor->FilterEvent(_event);
    update();
}

void igQtRenderWidget::mouseReleaseEvent(QMouseEvent* event) {
    iGame::IEvent _event;
    _event.type = iGame::IEvent::MouseRelease;
    _event.pos.x = event->pos().x();
    _event.pos.y = event->pos().y();
    m_Interactor->FilterEvent(_event);
    update();
}

void igQtRenderWidget::wheelEvent(QWheelEvent* event) {
    iGame::IEvent _event;
    _event.type = iGame::IEvent::Wheel;
    _event.delta = event->delta();
    m_Interactor->FilterEvent(_event);
    update();
}

// 根据深度值获取世界坐标
igm::vec3 igQtRenderWidget::GetWorldPositionFromDepth(const QPoint& screenPos, float depth) {
    // 将屏幕坐标转换为标准化设备坐标
    float x = (2.0f * screenPos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / height();

    // 获取投影和视图矩阵
    igm::mat4 projection = m_Scene->GetCamera()->GetProjectionMatrix();
    igm::mat4 view = m_Scene->GetCamera()->GetViewMatrix();

    // 计算逆矩阵
    igm::mat4 invVP = (projection * view).invert();

    // 创建近平面和远平面点
    igm::vec4 nearPoint(x, y, -1.0f, 1.0f);
    igm::vec4 farPoint(x, y, 1.0f, 1.0f);

    // 转换为世界坐标
    igm::vec4 nearResult = invVP * nearPoint;
    igm::vec4 farResult = invVP * farPoint;
    nearResult /= nearResult.w;
    farResult /= farResult.w;

    // 计算射线方向
    igm::vec3 rayDir = igm::vec3(farResult) - igm::vec3(nearResult);
    rayDir = rayDir.normalize();

    // 计算交点（世界坐标）
    igm::vec3 worldPos = igm::vec3(m_Scene->GetCamera()->GetPosition()) + rayDir * depth;

    // 如果启用模型变换，则应用当前模型矩阵的逆变换 
    //if (applyModelMatrix) {
        igm::mat4 invModelMatrix = m_Scene->GetModelMatrix().invert();
            //m_ModelMatrix.invert();
        igm::vec4 transformedPos = invModelMatrix * igm::vec4(worldPos, 1.0f);
        worldPos = igm::vec3(transformedPos);
    //}

    // 根据深度计算交点
    return worldPos;
}


