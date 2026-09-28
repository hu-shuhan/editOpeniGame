// ============================================================================
// igQtProbeWidget — 见 igQtProbeWidget.h
// ============================================================================
#include "IQWidgets/igQtProbeWidget.h"

#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtRenderWidget.h>

#include <iGameBoundingBox.h>
#include <iGameFlatArray.h>
#include <iGameModel.h>
#include <iGamePainter3D.h>
#include <iGameScene.h>
#include <iGameType.h>

#include <Probe/iGameProbeFilter.h>

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace iGame;

namespace {

const char* kInputStyle =
        "QLineEdit { background-color: #252526; color: #FFFFFF; "
        "border: 1px solid #3C3C3C; padding: 4px; }";
const char* kButtonStyle =
        "QPushButton { background-color: #252526; color: #CCCCCC; "
        "border: 1px solid #3C3C3C; padding: 6px; }"
        "QPushButton:hover { background-color: #3A3A3A; }"
        "QPushButton:pressed { background-color: #1E1E1E; }";
const char* kLabelStyle = "QLabel { font-size: 13px; color: #C8C8C8; }";
const char* kGroupStyle =
        "QGroupBox { font-size: 14px; color: #C8C8C8; "
        "border: 1px solid #3C3C3C; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; "
        "padding: 0 5px 0 5px; }";
const char* kResultTableStyle =
        "QTableWidget { background-color: #252526; color: #FFFFFF; "
        "border: 1px solid #3C3C3C; gridline-color: #3C3C3C; }"
        "QHeaderView { background-color: #2A2A2A; }"
        "QHeaderView::section { background-color: #2A2A2A; color: #C8C8C8; "
        "border: 1px solid #3C3C3C; padding: 4px; }"
        "QTableCornerButton { background-color: #2A2A2A; }"
        "QTableCornerButton::section { background-color: #2A2A2A; "
        "border: 1px solid #3C3C3C; }"
        "QTableWidget::item { background-color: #252526; color: #FFFFFF; "
        "padding: 4px; }"
        "QTableWidget::item:alternate { background-color: #2A2A2A; }"
        "QTableWidget::item:selected { background-color: #3A3A3A; "
        "color: #FFFFFF; }";

QLineEdit* MakeLineEdit() {
    auto* edit = new QLineEdit;
    edit->setStyleSheet(kInputStyle);
    return edit;
}

// 点到线段的最短距离（屏幕像素空间），用于球体线框拾取。
float DistToSegment(const igm::vec2& p, const igm::vec2& a,
                    const igm::vec2& b) {
    const float abx = b.x - a.x;
    const float aby = b.y - a.y;
    const float len2 = abx * abx + aby * aby;
    if (len2 < 1e-12f) {
        const float dx = p.x - a.x;
        const float dy = p.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    float t = ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2;
    t = std::max(0.0f, std::min(1.0f, t));
    const float px = a.x + t * abx;
    const float py = a.y + t * aby;
    const float dx = p.x - px;
    const float dy = p.y - py;
    return std::sqrt(dx * dx + dy * dy);
}

}  // namespace

igQtProbeWidget::igQtProbeWidget(QWidget* parent) : QWidget(parent) {
    buildUi();
    initConnections();
    m_probeButton->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("请先选择模型"));
}

igQtProbeWidget::~igQtProbeWidget() = default;

void igQtProbeWidget::setContext(
        std::function<iGame::Scene*()> sceneGetter,
        igQtModelDialogWidget* modelTree,
        std::function<void()> requestRender) {
    m_sceneGetter = std::move(sceneGetter);
    m_modelTree = modelTree;
    m_requestRender = std::move(requestRender);
    refreshFromCurrentModel();
}

void igQtProbeWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    auto* description = new QLabel(
            QStringLiteral("在球体内生成查询点，对模型做点定位 + 插值；\n"
                           "输出点集带插值属性与 ValidPointMask（有效=1）。"),
            this);
    description->setWordWrap(true);
    description->setStyleSheet(kLabelStyle);
    rootLayout->addWidget(description);

    auto* paramGroup = new QGroupBox(QStringLiteral("采样参数"), this);
    paramGroup->setStyleSheet(kGroupStyle);
    auto* formLayout = new QFormLayout(paramGroup);
    formLayout->setSpacing(6);

    m_centerX = MakeLineEdit();
    m_centerY = MakeLineEdit();
    m_centerZ = MakeLineEdit();
    m_radius = MakeLineEdit();
    m_count = MakeLineEdit();
    m_count->setText(QStringLiteral("1"));
    m_tolerance = MakeLineEdit();
    m_tolerance->setPlaceholderText(QStringLiteral("留空自动"));

    auto* centerWidget = new QWidget(paramGroup);
    auto* centerLayout = new QHBoxLayout(centerWidget);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(4);
    centerLayout->addWidget(m_centerX);
    centerLayout->addWidget(m_centerY);
    centerLayout->addWidget(m_centerZ);

    formLayout->addRow(QStringLiteral("Center X / Y / Z:"), centerWidget);
    formLayout->addRow(QStringLiteral("Radius:"), m_radius);
    formLayout->addRow(QStringLiteral("点数 N:"), m_count);
    formLayout->addRow(QStringLiteral("容差:"), m_tolerance);
    rootLayout->addWidget(paramGroup);

    m_probeButton = new QPushButton(QStringLiteral("探测"), this);
    m_probeButton->setStyleSheet(kButtonStyle);
    rootLayout->addWidget(m_probeButton);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(kLabelStyle);
    rootLayout->addWidget(m_statusLabel);

    auto* resultGroup = new QGroupBox(QStringLiteral("查询结果"), this);
    resultGroup->setStyleSheet(kGroupStyle);
    auto* resultLayout = new QVBoxLayout(resultGroup);
    resultLayout->setSpacing(4);
    m_resultTable = new QTableWidget(resultGroup);
    m_resultTable->setStyleSheet(kResultTableStyle);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // 隐藏行号表头，避免左上角出现默认白色角按钮；
    // 行号由“点 ID”列承担。
    m_resultTable->verticalHeader()->setVisible(false);
    resultLayout->addWidget(m_resultTable);
    rootLayout->addWidget(resultGroup);

    rootLayout->addStretch();
}

void igQtProbeWidget::initConnections() {
    connect(m_probeButton, &QPushButton::clicked, this,
            &igQtProbeWidget::onProbeClicked);
    for (QLineEdit* edit : {m_centerX, m_centerY, m_centerZ, m_radius}) {
        connect(edit, &QLineEdit::textChanged, this,
                &igQtProbeWidget::onSphereParamsEdited);
    }
}

void igQtProbeWidget::refreshFromCurrentModel() {
    clearOverlays();
    auto* scene = currentScene();
    if (scene == nullptr) {
        m_probeButton->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("请先选择模型"));
        return;
    }
    auto model = scene->GetCurrentModel();
    if (model.IsNull() || model->GetDataObject().IsNull()) {
        m_probeButton->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("请先选择模型"));
        return;
    }
    if (!m_queryPoints.IsNull() &&
        model->GetDataObject().GetPointer() == m_queryPoints.GetPointer()) {
        // 当前选中的是探测输出点集，不能作为探测输入模型
        m_probeButton->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("请先选择模型"));
        return;
    }
    auto data = model->GetDataObject();
    const BoundingBox& bbox = data->GetBoundingBox();
    if (bbox.isNull() || bbox.isEmpty()) {
        m_probeButton->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("当前模型无有效包围盒"));
        return;
    }
    const Vector3d center = bbox.center();
    const double diag = bbox.diag();
    // 中心红十字的固定大小按模型尺度取一次，不随 radius 变化
    m_crossHalfSize = static_cast<float>(diag * 0.03);

    m_updatingParams = true;
    m_centerX->setText(QString::number(center[0]));
    m_centerY->setText(QString::number(center[1]));
    m_centerZ->setText(QString::number(center[2]));
    m_radius->setText(QString::number(diag * 0.1));
    m_count->setText(QStringLiteral("1"));
    m_tolerance->clear();
    m_updatingParams = false;

    m_probeButton->setEnabled(true);
    m_statusLabel->setText(QStringLiteral("参数已按模型包围盒重置，点击[探测]执行"));
    drawSphere(Point(static_cast<float>(center[0]), static_cast<float>(center[1]),
                     static_cast<float>(center[2])),
               static_cast<float>(diag * 0.1));
    if (m_requestRender) m_requestRender();
}

iGame::Scene* igQtProbeWidget::currentScene() const {
    return m_sceneGetter ? m_sceneGetter() : nullptr;
}

void igQtProbeWidget::ensureQueryPointSet() {
    if (!m_queryPoints.IsNull() || m_modelTree == nullptr) return;
    m_queryPoints = PointSet::New();
    m_queryPoints->SetName("Probe 查询点");

    // addDataObjectToModelTree 会把新模型设为当前模型（Scene::AddModel
    // 本身也会改 m_CurrentModelID），这里先记住原当前模型，挂完树后再切回，
    // 避免打断用户当前选中模型。
    auto* scene = currentScene();
    iGame::Model* previousModel =
            scene ? scene->GetCurrentModel().get() : nullptr;
    ModelTreeWidgetItem* previousItem = nullptr;
    if (previousModel) {
        previousItem = m_modelTree->getItemFromObject(
                previousModel->GetDataObject());
    }

    m_modelTree->addDataObjectToModelTree(m_queryPoints, ItemSource::Algorithm);

    if (previousModel) {
        if (previousItem) m_modelTree->setCurrentItem(previousItem);
        // 该重载内部会 scene->SetCurrentModel(previousModel)，
        // 再刷新一次模型信息面板，回到挂树前的状态。
        m_modelTree->updateCurrentModelProperty(previousModel);
        m_modelTree->updateCurrentModelInfo();
    }

    // 挂树后直接打开点显示开关，方便查看探测点
    if (auto* item = m_modelTree->getItemFromObject(m_queryPoints)) {
        if (iGame::Model* model = item->getModel()) {
            model->SetViewPointsSwitch(true);
        }
    }
}

bool igQtProbeWidget::parseParams(Point& center, float& radius, int& count,
                                  double& tolerance,
                                  bool& autoTolerance) const {
    bool okX = false, okY = false, okZ = false, okR = false, okN = false;
    const double x = m_centerX->text().toDouble(&okX);
    const double y = m_centerY->text().toDouble(&okY);
    const double z = m_centerZ->text().toDouble(&okZ);
    const double r = m_radius->text().toDouble(&okR);
    const double n = m_count->text().toInt(&okN);
    if (!okX || !okY || !okZ || !okR || !okN) return false;
    // radius == 0 合法：此时所有查询点都生成在球心位置。
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(r) || r < 0.0 || n < 1.0) return false;

    center = Point(static_cast<float>(x), static_cast<float>(y),
                   static_cast<float>(z));
    radius = static_cast<float>(r);
    count = static_cast<int>(n);

    tolerance = 0.0;
    autoTolerance = true;
    if (!m_tolerance->text().trimmed().isEmpty()) {
        bool okT = false;
        tolerance = m_tolerance->text().toDouble(&okT);
        if (!okT || tolerance < 0.0) return false;
        autoTolerance = false;
    }
    return true;
}

void igQtProbeWidget::onProbeClicked() {
    auto* scene = currentScene();
    if (scene == nullptr || m_modelTree == nullptr) {
        m_statusLabel->setText(QStringLiteral("场景未初始化"));
        return;
    }
    auto model = scene->GetCurrentModel();
    if (model.IsNull() || model->GetDataObject().IsNull()) {
        m_statusLabel->setText(QStringLiteral("请先选择模型"));
        return;
    }

    Point center;
    float radius = 0.0f;
    int count = 1;
    double tolerance = 0.0;
    bool autoTolerance = true;
    if (!parseParams(center, radius, count, tolerance, autoTolerance)) {
        m_statusLabel->setText(
                QStringLiteral("参数无效，请检查 Center / Radius / N / 容差"));
        return;
    }

    // 1. 查询点集：创建一次后一直复用；同一对象作为 filter 的输入 1 与输出 0
    ensureQueryPointSet();
    if (m_queryPoints.IsNull()) {
        m_statusLabel->setText(QStringLiteral("模型树未初始化"));
        return;
    }

    // 2. 原地重生成球体内随机查询点
    ProbeFilter::GenerateSpherePoints(m_queryPoints, center, radius, count);

    // 3. 画线框球（Painter3D 叠加绘制，不进模型树）
    drawSphere(center, radius);

    // 4. 执行探测：模型 + 查询点集，结果原地写回查询点集
    auto filter = ProbeFilter::New();
    filter->SetInput(0, model->GetDataObject());
    filter->SetInput(1, m_queryPoints);
    if (!autoTolerance) filter->SetTolerance(tolerance);
    if (!filter->Execute()) {
        m_statusLabel->setText(QStringLiteral("探测失败"));
        return;
    }

    // 5. 统计有效点
    int validCount = 0;
    if (AttributeSet* attrs = m_queryPoints->GetAttributeSet()) {
        const int maskIndex = attrs->GetAttributeIndex("ValidPointMask");
        if (maskIndex >= 0) {
            auto mask = DynamicCast<IntArray>(
                    attrs->GetAttribute(maskIndex).pointer);
            if (!mask.IsNull()) {
                const IGsize num = mask->GetNumberOfElements();
                for (IGsize i = 0; i < num; ++i) {
                    if (mask->GetValue(i) == 1) ++validCount;
                }
            }
        }
    }

    // 6. 刷新模型树与视图
    m_modelTree->updateItemName(m_queryPoints);
    m_modelTree->updateAllAttriubute(m_queryPoints);
    updateResultTable();
    if (m_requestRender) m_requestRender();

    m_statusLabel->setText(
            QStringLiteral("探测完成：有效 %1 / 共 %2 个点")
                    .arg(validCount)
                    .arg(count));
}

void igQtProbeWidget::onSphereParamsEdited() {
    if (m_updatingParams) return;

    bool okX = false, okY = false, okZ = false, okR = false;
    const double x = m_centerX->text().toDouble(&okX);
    const double y = m_centerY->text().toDouble(&okY);
    const double z = m_centerZ->text().toDouble(&okZ);
    const double r = m_radius->text().toDouble(&okR);
    if (!okX || !okY || !okZ || !okR) return;

    drawSphere(Point(static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(z)),
               r <= 0.0 ? 0.0f : static_cast<float>(r));
    if (m_requestRender) m_requestRender();
}

void igQtProbeWidget::drawSphere(const Point& center, float radius) {
    auto* scene = currentScene();
    if (scene == nullptr) return;
    auto painter = scene->GetPainter3D();
    if (painter.IsNull()) return;

    clearSphere();
    clearCenterCross();
    if (radius > 0.0f) {
        painter->SetPen(255, 180, 0);
        painter->SetPen(1.5f);
        painter->SetBrush(Brush::Style::NoBrush);
        // 网格密度：9 个纬度层（8 条横线）+ 8 条经线
        m_sphereHandle = painter->DrawSphere(center, radius, 9, 8);
    }
    drawCenterCross(center);
}

void igQtProbeWidget::clearSphere() {
    auto* scene = currentScene();
    if (scene != nullptr && m_sphereHandle != 0) {
        auto painter = scene->GetPainter3D();
        if (!painter.IsNull()) {
            painter->Delete(m_sphereHandle);
        }
    }
    m_sphereHandle = 0;
}

void igQtProbeWidget::drawCenterCross(const Point& center) {
    if (m_crossHalfSize <= 0.0f) return;
    auto* scene = currentScene();
    if (scene == nullptr) return;
    auto painter = scene->GetPainter3D();
    if (painter.IsNull()) return;

    painter->SetPen(255, 0, 0);
    painter->SetPen(2.0f);
    const float s = m_crossHalfSize;
    m_crossHandle[0] =
            painter->DrawLine(Point{center[0] - s, center[1], center[2]},
                              Point{center[0] + s, center[1], center[2]});
    m_crossHandle[1] =
            painter->DrawLine(Point{center[0], center[1] - s, center[2]},
                              Point{center[0], center[1] + s, center[2]});
    m_crossHandle[2] =
            painter->DrawLine(Point{center[0], center[1], center[2] - s},
                              Point{center[0], center[1], center[2] + s});
}

void igQtProbeWidget::clearCenterCross() {
    auto* scene = currentScene();
    if (scene != nullptr) {
        auto painter = scene->GetPainter3D();
        if (!painter.IsNull()) {
            for (unsigned int& handle : m_crossHandle) {
                if (handle != 0) painter->Delete(handle);
                handle = 0;
            }
        }
    }
    for (unsigned int& handle : m_crossHandle) handle = 0;
}

void igQtProbeWidget::clearOverlays() {
    clearSphere();
    clearCenterCross();
}

void igQtProbeWidget::updateResultTable() {
    if (m_resultTable == nullptr) return;
    m_resultTable->setSortingEnabled(false);
    m_resultTable->clearContents();
    m_resultTable->setRowCount(0);

    if (m_queryPoints.IsNull()) return;

    auto* data = m_queryPoints.GetPointer();
    auto points = data->GetPoints();
    const int numPoints =
            points ? static_cast<int>(points->GetNumberOfPoints()) : 0;
    if (numPoints <= 0) return;

    struct ResultColumn {
        QString name;
        int attributeIndex{-1};
        int component{0};
    };
    QVector<ResultColumn> columns;

    auto* attributeSet = data->GetAttributeSet();
    if (attributeSet) {
        auto attributes = attributeSet->GetAllAttributes();
        for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
            auto& attribute = attributes->GetElement(i);
            if (attribute.isDeleted || !attribute.pointer ||
                attribute.attachmentType != IG_POINT)
                continue;
            const int dimension = attribute.pointer->GetDimension();
            if (dimension <= 0) continue;

            QString baseName =
                    QString::fromStdString(attribute.pointer->GetName());
            if (baseName.isEmpty()) {
                baseName = QStringLiteral("属性 %1").arg(i);
            }

            if (dimension > 1) {
                columns.push_back(
                        {QStringLiteral("%1（模）").arg(baseName), i, -1});
            }
            for (int component = 0; component < dimension; ++component) {
                columns.push_back(
                        {dimension == 1
                                 ? baseName
                                 : QStringLiteral("%1[%2]")
                                           .arg(baseName)
                                           .arg(component),
                         i, component});
            }
        }
    }

    const int firstPropertyColumn = 4;
    m_resultTable->setColumnCount(firstPropertyColumn + columns.size());
    QStringList headers{QStringLiteral("点 ID"), "X", "Y", "Z"};
    for (const auto& column : columns) headers.push_back(column.name);
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->setRowCount(numPoints);

    for (int row = 0; row < numPoints; ++row) {
        auto* idItem = new QTableWidgetItem;
        idItem->setData(Qt::DisplayRole, row);
        m_resultTable->setItem(row, 0, idItem);

        const Point& point = points->GetPoint(row);
        for (int component = 0; component < 3; ++component) {
            auto* coordinateItem = new QTableWidgetItem;
            coordinateItem->setData(Qt::DisplayRole, point[component]);
            m_resultTable->setItem(row, component + 1, coordinateItem);
        }

        for (int columnIndex = 0; columnIndex < columns.size();
             ++columnIndex) {
            auto* valueItem = new QTableWidgetItem;
            const auto& column = columns[columnIndex];
            if (column.attributeIndex >= 0 && attributeSet) {
                auto& attribute =
                        attributeSet->GetAttribute(column.attributeIndex);
                if (attribute.pointer &&
                    row <
                            static_cast<int>(
                                    attribute.pointer->GetNumberOfElements())) {
                    valueItem->setData(
                            Qt::DisplayRole,
                            attribute.pointer->GetElementValue(
                                    row, column.component));
                } else {
                    valueItem->setText(QStringLiteral("—"));
                }
            } else {
                valueItem->setText(QStringLiteral("—"));
            }
            m_resultTable->setItem(row, firstPropertyColumn + columnIndex,
                                   valueItem);
        }
    }

    m_resultTable->resizeColumnsToContents();
    m_resultTable->setSortingEnabled(true);
}

void igQtProbeWidget::closeEvent(QCloseEvent* event) {
    clearOverlays();
    resetDragState();
    if (m_renderWidget && m_filterInstalled) {
        m_renderWidget->removeEventFilter(this);
        m_filterInstalled = false;
    }
    QWidget::closeEvent(event);
}

void igQtProbeWidget::hideEvent(QHideEvent* event) {
    clearOverlays();
    resetDragState();
    if (m_renderWidget && m_filterInstalled) {
        m_renderWidget->removeEventFilter(this);
        m_filterInstalled = false;
    }
    QWidget::hideEvent(event);
}

void igQtProbeWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 面板显示时挂上事件过滤器，拖球交互才生效；隐藏时在 hideEvent 里卸载。
    if (m_renderWidget && !m_filterInstalled) {
        m_renderWidget->installEventFilter(this);
        m_filterInstalled = true;
    }
}

void igQtProbeWidget::setRenderWidget(igQtRenderWidget* renderWidget) {
    m_renderWidget = renderWidget;
}

bool igQtProbeWidget::eventFilter(QObject* obj, QEvent* event) {
    // 只过滤渲染视图自身的事件；没有球（或面板已隐藏）时完全放行，
    // 不影响旋转/平移/缩放等原有交互。
    if (obj != m_renderWidget) return false;
    // 拖拽中即使球被删（如半径拖到 0）也要继续消费，避免拖拽脱钩。
    if (m_sphereHandle == 0 && m_dragMode == DragMode::None) return false;

    const QEvent::Type type = event->type();
    if (type == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        const QPoint pos = me->pos();
        // 拖拽进行中时新按键也消费掉，避免模式被意外切换。
        if (m_dragging) return true;
        if (me->button() == Qt::LeftButton &&
            isPointOnSphereWireframe(pos, 10.0f)) {
            return beginDrag(pos, DragMode::MoveCenter);
        }
        if (me->button() == Qt::RightButton && isPointOnSphereSurface(pos)) {
            return beginDrag(pos, DragMode::ResizeRadius);
        }
        return false;
    }

    if (type == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        const QPoint pos = me->pos();
        if (m_dragMode != DragMode::None) {
            updateDrag(pos);
            return true; // 拖拽期间持续消费，即使鼠标移出球体。
        }
        updateHoverCursor(pos);
        return false; // 未拖拽时放行，保持原有交互。
    }

    if (type == QEvent::MouseButtonRelease) {
        if (m_dragMode != DragMode::None) {
            endDrag();
            return true;
        }
        return false;
    }

    if (type == QEvent::Leave) {
        if (m_dragMode == DragMode::None && m_renderWidget) {
            m_renderWidget->unsetCursor();
        }
        return false;
    }

    return false;
}

bool igQtProbeWidget::beginDrag(const QPoint& pos, DragMode mode) {
    auto* scene = currentScene();
    if (scene == nullptr) return false;
    auto camera = scene->GetCamera();
    if (camera == nullptr) return false;

    Point center;
    float radius = 0.0f;
    int count = 0;
    double tolerance = 0.0;
    bool autoTolerance = false;
    if (!parseParams(center, radius, count, tolerance, autoTolerance)) {
        return false;
    }

    m_dragMode = mode;
    m_dragging = true;
    m_dragCenter = center;
    m_dragRadius = radius;
    m_lastDragPos = pos;

    const igm::mat4 model = scene->GetModelMatrix();
    m_dragMVP = camera->GetProjectionMatrix() * camera->GetViewMatrix() * model;
    m_dragInvMVP = m_dragMVP.invert();

    // 记录球心 NDC 深度：拖拽固定在按下时的深度平面上移动。
    const igm::vec4 clip =
            m_dragMVP * igm::vec4{center[0], center[1], center[2], 1.0f};
    m_centerNDCZ = clip.z / clip.w;
    return true;
}

void igQtProbeWidget::updateDrag(const QPoint& pos) {
    if (!m_dragging) return;
    if (m_dragMode == DragMode::MoveCenter) {
        const igm::vec3 p = unproject(pos, m_centerNDCZ);
        applySphereParams(Point{static_cast<float>(p.x), static_cast<float>(p.y),
                                static_cast<float>(p.z)},
                          m_dragRadius);
    } else if (m_dragMode == DragMode::ResizeRadius) {
        const igm::vec3 p = unproject(pos, m_centerNDCZ);
        const igm::vec3 c{m_dragCenter[0], m_dragCenter[1], m_dragCenter[2]};
        const igm::vec3 d = p - c;
        const float r =
                std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        applySphereParams(m_dragCenter, r);
    }
    m_lastDragPos = pos;
}

void igQtProbeWidget::endDrag() {
    m_dragMode = DragMode::None;
    m_dragging = false;
}

void igQtProbeWidget::resetDragState() {
    m_dragMode = DragMode::None;
    m_dragging = false;
    if (m_renderWidget) m_renderWidget->unsetCursor();
}

void igQtProbeWidget::applySphereParams(const Point& center, float radius) {
    m_updatingParams = true;
    m_centerX->setText(QString::number(static_cast<double>(center[0])));
    m_centerY->setText(QString::number(static_cast<double>(center[1])));
    m_centerZ->setText(QString::number(static_cast<double>(center[2])));
    m_radius->setText(QString::number(static_cast<double>(radius)));
    m_updatingParams = false;

    drawSphere(center, radius);
    if (m_requestRender) m_requestRender();
}

igm::vec3 igQtProbeWidget::unproject(const QPoint& pos, float ndcZ) const {
    if (m_renderWidget == nullptr) return igm::vec3{};
    const float ndcX = 2.0f * pos.x() / m_renderWidget->width() - 1.0f;
    const float ndcY = 1.0f - 2.0f * pos.y() / m_renderWidget->height();
    igm::vec4 p = m_dragInvMVP * igm::vec4{ndcX, ndcY, ndcZ, 1.0f};
    p /= p.w;
    return igm::vec3{p.x, p.y, p.z};
}

bool igQtProbeWidget::isPointOnSphereWireframe(const QPoint& pos,
                                               float tolerance) const {
    auto* scene = currentScene();
    if (scene == nullptr || m_renderWidget == nullptr) return false;
    auto camera = scene->GetCamera();
    if (camera == nullptr) return false;

    Point center;
    float radius = 0.0f;
    int count = 0;
    double dummyTolerance = 0.0;
    bool autoTolerance = false;
    if (!parseParams(center, radius, count, dummyTolerance, autoTolerance)) {
        return false;
    }
    if (radius <= 0.0f) return false;

    const igm::mat4 mvp = camera->GetProjectionMatrix() *
                          camera->GetViewMatrix() * scene->GetModelMatrix();
    const float w = static_cast<float>(m_renderWidget->width());
    const float h = static_cast<float>(m_renderWidget->height());
    if (w <= 0.0f || h <= 0.0f) return false;

    // 与 drawSphere 相同的网格密度（9 个纬度层 × 8 条经线）。
    const int stackCount = 9;
    const int sectorCount = 8;
    const float sectorStep = 2.0f * IGM_PI / sectorCount;
    const float stackStep = IGM_PI / stackCount;

    // 生成并投影全部线框顶点到屏幕空间。
    std::vector<igm::vec2> screen;
    screen.reserve(static_cast<size_t>((stackCount + 1) * (sectorCount + 1)));
    for (int i = 0; i <= stackCount; ++i) {
        const float stackAngle = IGM_PI / 2.0f - i * stackStep;
        const float xy = radius * std::cos(stackAngle);
        const float z = center[2] + radius * std::sin(stackAngle);
        for (int j = 0; j <= sectorCount; ++j) {
            const float sectorAngle = j * sectorStep;
            igm::vec4 clip =
                    mvp * igm::vec4{center[0] + xy * std::cos(sectorAngle),
                                    center[1] + xy * std::sin(sectorAngle), z,
                                    1.0f};
            clip /= clip.w;
            screen.emplace_back((clip.x + 1.0f) * 0.5f * w,
                                (1.0f - clip.y) * 0.5f * h);
        }
    }

    const auto at = [sectorCount, &screen](int i, int j) -> const igm::vec2& {
        return screen[static_cast<size_t>(i * (sectorCount + 1) + j)];
    };
    const igm::vec2 p{static_cast<float>(pos.x()),
                      static_cast<float>(pos.y())};

    // 竖线（经线）：同一 sector 的相邻纬度层连线。
    for (int i = 0; i < stackCount; ++i) {
        for (int j = 0; j <= sectorCount; ++j) {
            if (DistToSegment(p, at(i, j), at(i + 1, j)) <= tolerance) {
                return true;
            }
        }
    }
    // 横线（纬线）：同一纬度层的相邻 sector 连线（最顶部一圈不画）。
    for (int i = 1; i <= stackCount; ++i) {
        for (int j = 0; j < sectorCount; ++j) {
            if (DistToSegment(p, at(i, j), at(i, j + 1)) <= tolerance) {
                return true;
            }
        }
    }
    return false;
}

bool igQtProbeWidget::isPointOnSphereSurface(const QPoint& pos) const {
    auto* scene = currentScene();
    if (scene == nullptr || m_renderWidget == nullptr) return false;
    auto camera = scene->GetCamera();
    if (camera == nullptr) return false;

    Point center;
    float radius = 0.0f;
    int count = 0;
    double dummyTolerance = 0.0;
    bool autoTolerance = false;
    if (!parseParams(center, radius, count, dummyTolerance, autoTolerance)) {
        return false;
    }
    if (radius <= 0.0f) return false;

    const igm::mat4 mvp = camera->GetProjectionMatrix() *
                          camera->GetViewMatrix() * scene->GetModelMatrix();
    const igm::mat4 invMVP = mvp.invert();
    const float w = static_cast<float>(m_renderWidget->width());
    const float h = static_cast<float>(m_renderWidget->height());
    if (w <= 0.0f || h <= 0.0f) return false;

    const float ndcX = 2.0f * pos.x() / w - 1.0f;
    const float ndcY = 1.0f - 2.0f * pos.y() / h;
    const auto unprojectLocal = [&](float ndcZ) {
        igm::vec4 q = invMVP * igm::vec4{ndcX, ndcY, ndcZ, 1.0f};
        q /= q.w;
        return igm::vec3{q.x, q.y, q.z};
    };

    // 构造模型空间射线，与球面求交（取近根，即前半球）。
    const igm::vec3 nearPoint = unprojectLocal(-1.0f);
    const igm::vec3 farPoint = unprojectLocal(1.0f);
    igm::vec3 dir = farPoint - nearPoint;
    const float dirLen =
            std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (dirLen < 1e-12f) return false;
    dir /= dirLen;

    const igm::vec3 oc =
            nearPoint - igm::vec3{center[0], center[1], center[2]};
    const float b = oc.x * dir.x + oc.y * dir.y + oc.z * dir.z;
    const float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.0f) return false;
    const float t = -b - std::sqrt(disc);
    return t >= 0.0f;
}

void igQtProbeWidget::updateHoverCursor(const QPoint& pos) {
    if (m_renderWidget == nullptr) return;
    if (isPointOnSphereWireframe(pos, 10.0f)) {
        m_renderWidget->setCursor(Qt::SizeAllCursor);
    } else if (isPointOnSphereSurface(pos)) {
        m_renderWidget->setCursor(Qt::SizeHorCursor);
    } else {
        m_renderWidget->unsetCursor();
    }
}
