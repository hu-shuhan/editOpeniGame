#include "IQWidgets/igQtResampleToLineWidget.h"

#include "IQComponents/igQtModelDialogWidget.h"
#include "iGameInteractor.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"

#include <QApplication>
#include <QHideEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QShowEvent>
#include <QSignalBlocker>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

igQtResampleToLine::igQtResampleToLine(igQtModelDialogWidget* modelTreeWidget, QWidget* parent)
    : QWidget(parent), ui(new Ui::ResampleToLineWidget) {
    ui->setupUi(this);

    // 主窗口会传入真正的模型树控件；为空时再从顶层窗口对象树中查找
    m_ModelTreeWidget = (modelTreeWidget != nullptr) ? modelTreeWidget : ResolveModelTreeWidget();

    // 执行
    connect(ui->pushButton, &QPushButton::clicked, this, [this]() {
        this->UpdateLine();
        this->ResampleToLine();
    });

    // 端点输入框
    QLineEdit* endpointEdits[] = {ui->point1_x, ui->point1_y, ui->point1_z, ui->point2_x, ui->point2_y, ui->point2_z};
    for (QLineEdit* edit : endpointEdits) {
        if (edit == nullptr) { continue; }
        connect(edit, &QLineEdit::textChanged, this, [this]() { this->UpdateLine(); });
    }

    // X / Y / Z 轴快捷按钮
    connect(ui->pushButton_AxisX, &QPushButton::clicked, this, [this]() { this->SetAxis(0); });
    connect(ui->pushButton_AxisY, &QPushButton::clicked, this, [this]() { this->SetAxis(1); });
    connect(ui->pushButton_AxisZ, &QPushButton::clicked, this, [this]() { this->SetAxis(2); });

    // 输入校验
    const QRegularExpression rxFloat("-?\\d*\\.?\\d+");
    for (QLineEdit* edit : endpointEdits) {
        if (edit == nullptr) { continue; }
        edit->setValidator(new QRegularExpressionValidator(rxFloat, this));
    }
    const QRegularExpression rxInt("^[0-9]+$");
    ui->resolution->setValidator(new QRegularExpressionValidator(rxInt, this));
    // 容差允许小数：相对模型包围盒对角线的单元判定容差
    ui->lineEdit_8->setValidator(new QRegularExpressionValidator(rxFloat, this));

    ui->resolution->setText(QString::number(resolution));
    // 容差留空或填 0 表示「自动容差」（包围盒对角线 × 1e-6）
    ui->lineEdit_8->setPlaceholderText(QStringLiteral("自动"));
    ui->lineEdit_8->setText(QString());

    m_Selection = GetSelection();
    SyncLineWidgets();
}

igQtResampleToLine::~igQtResampleToLine() {
    if (m_OriginDataObject && m_OriginObserverTag) {
        m_OriginDataObject->RemoveObserver(m_OriginObserverTag);
        m_OriginObserverTag = 0;
    }
    if (m_ResultMesh && m_ResultObserverTag) {
        m_ResultMesh->RemoveObserver(m_ResultObserverTag);
        m_ResultObserverTag = 0;
    }
    ClearPreviewHandles();
}

void igQtResampleToLine::SetModelTreeWidget(igQtModelDialogWidget* modelTreeWidget) {
    if (modelTreeWidget != nullptr) { m_ModelTreeWidget = modelTreeWidget; }
}

igQtModelDialogWidget* igQtResampleToLine::ResolveModelTreeWidget() {
    if (m_ModelTreeWidget != nullptr) { return m_ModelTreeWidget; }

    // 1) 优先在自己的顶层窗口里查找
    QWidget* top = this->window();
    if (top != nullptr) {
        if (auto* widget = top->findChild<igQtModelDialogWidget*>()) { return widget; }
    }
    // 2) 回退：遍历应用程序的所有顶层窗口（浮动 Dock 等情况）
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (QWidget* level : topLevels) {
        if (level == nullptr) { continue; }
        if (auto* widget = level->findChild<igQtModelDialogWidget*>()) { return widget; }
    }
    return nullptr;
}

/* ------------------------------------------------------------------ */
/* 面板显示/隐藏：接管交互风格                                          */
/* ------------------------------------------------------------------ */
bool igQtResampleToLine::ActivateInteractorStyle() {
    if (m_Activating) { return false; }

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) { return false; }

    auto model = scene->GetCurrentModel();
    if (model == nullptr) { return false; }

    auto dataObject = model->GetDataObject();
    if (dataObject == nullptr) { return false; }

    m_Activating = true;

    // 面板跟随当前模型：默认端点由模型包围盒给出；
    // 执行后当前模型会切换为本面板的结果折线，此时保留原有输入对象。
    const bool isOwnResult = (m_ResultMesh != nullptr) && (dataObject.GetPointer() == m_ResultMesh.GetPointer());
    if (!isOwnResult &&
        (m_OriginDataObject == nullptr || dataObject.GetPointer() != m_OriginDataObject.GetPointer())) {
        SetOriginDataObject(dataObject);
    }

    bool ok = false;
    if (m_OriginDataObject != nullptr) {
        auto interactor = scene->GetInteractor();
        if (interactor != nullptr) {
            if (m_Selection == nullptr) { m_Selection = GetSelection(); }

            // 交互风格需要数据对象与画笔：用于在场景中绘制可拖动的起点/终点
            interactor->SetDataObject(m_OriginDataObject);
            interactor->SetPainter3D(model->GetPainter3D());
            interactor->RequestResampleToLineStyle(m_Selection);
            m_StyleActive = true;

            // 交互风格会自己绘制端点/线段，去掉控件绘制的重复线段
            ClearPreviewHandles();
            ok = true;
        }
    }

    m_Activating = false;

    if (ok) { RefreshLine(); }
    return ok;
}

void igQtResampleToLine::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    ActivateInteractorStyle();
}

void igQtResampleToLine::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);

    m_StyleActive = false;
    ClearPreviewHandles();

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene == nullptr) { return; }

    auto interactor = scene->GetInteractor();
    if (interactor == nullptr) { return; }

    if (!interactor->IsBasicStyle()) { interactor->RequestBasicStyle(); }
}

/* ------------------------------------------------------------------ */
/* 端点设置与同步                                                      */
/* ------------------------------------------------------------------ */
void igQtResampleToLine::SetLine(float o[3], float t[3]) {
    m_orig[0] = o[0];
    m_orig[1] = o[1];
    m_orig[2] = o[2];
    m_target[0] = t[0];
    m_target[1] = t[1];
    m_target[2] = t[2];

    SyncLineWidgets();
    RefreshLine();
}

void igQtResampleToLine::SetLine(iGame::Vector3d orig, iGame::Vector3d target) {
    float o[3], t[3];
    o[0] = static_cast<float>(orig[0]);
    o[1] = static_cast<float>(orig[1]);
    o[2] = static_cast<float>(orig[2]);
    t[0] = static_cast<float>(target[0]);
    t[1] = static_cast<float>(target[1]);
    t[2] = static_cast<float>(target[2]);
    SetLine(o, t);
}

void igQtResampleToLine::SyncLineWidgets() {
    QSignalBlocker blocker1(ui->point1_x);
    QSignalBlocker blocker2(ui->point1_y);
    QSignalBlocker blocker3(ui->point1_z);
    QSignalBlocker blocker4(ui->point2_x);
    QSignalBlocker blocker5(ui->point2_y);
    QSignalBlocker blocker6(ui->point2_z);

    ui->point1_x->setText(QString::number(m_orig[0]));
    ui->point1_y->setText(QString::number(m_orig[1]));
    ui->point1_z->setText(QString::number(m_orig[2]));
    ui->point2_x->setText(QString::number(m_target[0]));
    ui->point2_y->setText(QString::number(m_target[1]));
    ui->point2_z->setText(QString::number(m_target[2]));
}

void igQtResampleToLine::UpdateLine() {
    m_orig[0] = ui->point1_x->text().toFloat();
    m_orig[1] = ui->point1_y->text().toFloat();
    m_orig[2] = ui->point1_z->text().toFloat();
    m_target[0] = ui->point2_x->text().toFloat();
    m_target[1] = ui->point2_y->text().toFloat();
    m_target[2] = ui->point2_z->text().toFloat();

    bool ok = false;
    const int res = ui->resolution->text().toInt(&ok);
    if (ok && res >= 2) { resolution = res; }

    // 容差：留空 / 0 → 自动（过滤器内部按包围盒对角线计算）
    const double tolerance = ui->lineEdit_8->text().toDouble(&ok);
    m_Tolerance = (ok && tolerance > 0.0) ? tolerance : 0.0;

    RefreshLine();
}

void igQtResampleToLine::RefreshLine() {
    if (m_Selection == nullptr) { m_Selection = GetSelection(); }

    if (m_Selection != nullptr) {
        m_Selection->Orig = iGame::Vector3d(m_orig[0], m_orig[1], m_orig[2]);
        m_Selection->Target = iGame::Vector3d(m_target[0], m_target[1], m_target[2]);
        // 反向驱动交互风格刷新场景中的起点/终点/线段
        m_Selection->UpdateLine();
    }

    if (!m_StyleActive) { DrawLine(m_orig, m_target); }
}

void igQtResampleToLine::DrawLine(float o[3], float t[3]) {
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = (scene != nullptr) ? scene->GetCurrentModel() : nullptr;
    if (model == nullptr) { return; }

    auto painter = model->GetPainter3D();
    if (painter == nullptr) { return; }

    ClearPreviewHandles();

    const iGame::Point p0(o[0], o[1], o[2]);
    const iGame::Point p1(t[0], t[1], t[2]);

    painter->SetPen(2);
    painter->SetPen(Color::White);
    m_PreviewHandles.push_back(painter->DrawLine(p0, p1));

    painter->SetPen(16);
    painter->SetPen(Color::Green);
    m_PreviewHandles.push_back(painter->DrawPoint(p0));
    painter->SetPen(Color::Red);
    m_PreviewHandles.push_back(painter->DrawPoint(p1));

    painter->Modified();
}

void igQtResampleToLine::ClearPreviewHandles() {
    if (m_PreviewHandles.empty()) { return; }

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto model = (scene != nullptr) ? scene->GetCurrentModel() : nullptr;
    if (model != nullptr) {
        auto painter = model->GetPainter3D();
        if (painter != nullptr) {
            for (IGuint handle : m_PreviewHandles) { painter->Delete(handle); }
            painter->Modified();
        }
    }
    m_PreviewHandles.clear();
}

/* ------------------------------------------------------------------ */
/* X / Y / Z 轴快捷按钮与包围盒默认端点                                 */
/* ------------------------------------------------------------------ */
void igQtResampleToLine::SetAxis(int axis) {
    if (axis < 0 || axis > 2) { return; }

    if (m_OriginDataObject == nullptr) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto model = (scene != nullptr) ? scene->GetCurrentModel() : nullptr;
        if (model != nullptr) { SetOriginDataObject(model->GetDataObject()); }
    }
    if (m_OriginDataObject == nullptr) { return; }

    const auto& bbox = m_OriginDataObject->GetBoundingBox();
    const iGame::Vector3d center = bbox.center();
    const iGame::Vector3d extent = bbox.max - bbox.min;
    const double diag = std::max(bbox.diag(), 1e-12);

    double length = extent[axis];
    if (length < 0.25 * diag) { length = 0.5 * diag; } // 平面模型某一维长度为 0 时的兜底

    float o[3] = {static_cast<float>(center[0]), static_cast<float>(center[1]), static_cast<float>(center[2])};
    float t[3] = {static_cast<float>(center[0]), static_cast<float>(center[1]), static_cast<float>(center[2])};
    o[axis] = static_cast<float>(o[axis] - 0.5 * length);
    t[axis] = static_cast<float>(t[axis] + 0.5 * length);

    SetLine(o, t);

    if (!m_StyleActive) { ActivateInteractorStyle(); }
}

void igQtResampleToLine::ResetLineFromBoundingBox() {
    if (m_OriginDataObject == nullptr) { return; }

    const auto& bbox = m_OriginDataObject->GetBoundingBox();
    const iGame::Vector3d extent = bbox.max - bbox.min;

    // 取包围盒最长的一条轴作为默认采样方向
    int axis = 0;
    if (extent[1] > extent[axis]) { axis = 1; }
    if (extent[2] > extent[axis]) { axis = 2; }

    SetAxis(axis);
}

/* ------------------------------------------------------------------ */
/* 数据对象与结果                                                      */
/* ------------------------------------------------------------------ */
void igQtResampleToLine::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    if (m_OriginDataObject && m_OriginObserverTag) {
        m_OriginDataObject->RemoveObserver(m_OriginObserverTag);
        m_OriginObserverTag = 0;
    }
    if (m_ResultMesh && m_ResultObserverTag) {
        m_ResultMesh->RemoveObserver(m_ResultObserverTag);
        m_ResultObserverTag = 0;
    }

    m_OriginDataObject = m_d;
    m_ResultMesh = iGame::SurfaceMesh::New();
    m_ResultMesh->SetName("ResampleToLine");

    if (m_d == nullptr) { return; }

    // 原始模型被删除
    m_OriginObserverTag = m_d->AddObserver(iGame::Command::DeleteEvent, [this]() -> void {
        if (m_ResultMesh && m_ResultObserverTag) {
            m_ResultMesh->RemoveObserver(m_ResultObserverTag);
            m_ResultObserverTag = 0;
        }
        m_OriginObserverTag = 0;
        this->m_OriginDataObject = nullptr;
        this->m_ResultMesh = nullptr;
        if (this->parentWidget() != nullptr) { this->parentWidget()->hide(); }
        emit ResetInteractor();
    });

    // 结果折线被删除
    m_ResultObserverTag = m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [this]() -> void {
        if (m_OriginDataObject && m_OriginObserverTag) {
            m_OriginDataObject->RemoveObserver(m_OriginObserverTag);
            m_OriginObserverTag = 0;
        }
        m_ResultObserverTag = 0;
        this->m_OriginDataObject = nullptr;
        this->m_ResultMesh = nullptr;
        if (this->parentWidget() != nullptr) { this->parentWidget()->hide(); }
        emit ResetInteractor();
    });

    // 默认端点根据模型包围盒设置
    ResetLineFromBoundingBox();
    RefreshLine();
}

void igQtResampleToLine::UpdateOriginDataObject(iGame::DataObject::Pointer _origin_ptr) {
    m_OriginDataObject = _origin_ptr;
}

void igQtResampleToLine::BindCurrentModel() {
    // 与 showEvent 行为一致：绑定当前模型（模型变化时按包围盒重新给出默认端点）并激活交互风格
    ActivateInteractorStyle();
}

iGame::LineSelection::Pointer igQtResampleToLine::GetSelection() {
    if (m_Selection == nullptr) {
        m_Selection = iGame::LineSelection::New();
        m_Selection->SetSelectionCallBackEvent(
                [this](IGenum itemType, const std::vector<igIndex>& ids, iGame::Selection::Operate ope) {
                    if (itemType != IG_CHANGE) { return; }
                    if (m_Selection == nullptr) { return; }
                    // 交互拖动后把端点回填到面板（SetLine 内部阻塞信号，不会递归）
                    SetLine(m_Selection->Orig, m_Selection->Target);
                },
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }
    return m_Selection;
}

void igQtResampleToLine::ResampleToLine() {
    // 若面板此前没有绑定模型（例如先打开面板后加载模型），这里补上绑定与交互风格
    if (!m_StyleActive) { ActivateInteractorStyle(); }

    UpdateLine();

    if (m_OriginDataObject == nullptr) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto model = (scene != nullptr) ? scene->GetCurrentModel() : nullptr;
        if (model != nullptr) { SetOriginDataObject(model->GetDataObject()); }
    }
    if (m_OriginDataObject == nullptr) { return; }

    iGame::ResampleToLine::Pointer filter = iGame::ResampleToLine::New();
    filter->SetInput(m_OriginDataObject);
    filter->setOrigTarget(m_orig, m_target, resolution);
    filter->SetTolerance(m_Tolerance);

    if (!filter->Execute()) { return; }

    // 输出 1 为真正的折线数据（SurfaceMesh：点 + 边）
    iGame::SurfaceMesh::Pointer poly = filter->GetPolyLine();
    if (poly == nullptr) { poly = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput(1)); }
    if (poly == nullptr) { return; }

    poly->SetName("ResampleToLine");

    if (m_ModelTreeWidget == nullptr) { m_ModelTreeWidget = ResolveModelTreeWidget(); }
    if (m_ModelTreeWidget != nullptr) { m_ModelTreeWidget->addDataObjectToModelTree(poly, Algorithm); }

    m_ResultMesh = poly;
    m_ResultMesh->ConvertToDrawableData();

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene != nullptr) { scene->Update(); }
}
