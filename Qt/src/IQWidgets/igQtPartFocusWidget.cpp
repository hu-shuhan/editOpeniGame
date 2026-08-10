#include <IQWidgets/igQtPartFocusWidget.h>

#include <iGameBoundingBox.h>
#include <iGameBoxStyle.h>
#include <iGameDataObject.h>
#include <iGameScene.h>
#include <iGameSelectionParameter.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <QHBoxLayout>
#include <QScrollArea>

using namespace iGame;

igQtPartFocusWidget::igQtPartFocusWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void igQtPartFocusWidget::SetScene(iGame::Scene* scene, QWidget* rendererWidget) {
    m_scene = scene;
    m_rendererWidget = rendererWidget;
}

static const char* kDarkButtonQss = R"(
QPushButton {
    background-color: #2A2A2A;
    color: rgba(255, 255, 255, 204);
    border: 1px solid rgba(255, 255, 255, 20);
    border-radius: 4px;
    padding: 5px 12px;
    min-height: 24px;
    font-size: 10pt;
}
QPushButton:hover {
    background-color: #2F2F2F;
    border: 1px solid rgba(255, 255, 255, 32);
}
QPushButton:pressed {
    background-color: #252526;
    border: 1px solid rgba(255, 255, 255, 26);
    padding: 6px 13px 4px 11px;
}
QPushButton:disabled {
    background-color: #222222;
    color: rgba(255, 255, 255, 80);
    border: 1px solid rgba(255, 255, 255, 10);
}
)";

void igQtPartFocusWidget::setupUI() {
    setStyleSheet("igQtPartFocusWidget { background-color: #222222; } "
                  "QLabel { color: rgba(255,255,255,204); font-size: 10pt; } "
                  "QScrollArea { background-color: #1E1E1E; border: 1px solid #3C3C3C; border-radius: 4px; } "
                  "QWidget#partContainer { background-color: #1E1E1E; } "
                  "QCheckBox { color: rgba(255,255,255,204); font-size: 10pt; padding: 3px 6px; } "
                  "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid rgba(255,255,255,80); "
                  "                       border-radius: 3px; background-color: #2A2A2A; } "
                  "QCheckBox::indicator:checked { background-color: #094771; border: 1px solid #4FC3F7; } "
                  "QCheckBox:hover { background-color: #2F2F2F; border-radius: 3px; }");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // 状态标签
    m_statusLabel = new QLabel(QStringLiteral("请先点击\"刷新列表\""), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    // 滚动区域 + 容器（动态填充 QCheckBox）
    m_partScrollArea = new QScrollArea(this);
    m_partScrollArea->setWidgetResizable(true);
    m_partScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_partScrollArea->setMinimumHeight(180);

    m_partContainer = new QWidget();
    m_partContainer->setObjectName("partContainer");
    m_partContainer->setLayout(new QVBoxLayout());
    m_partContainer->layout()->setContentsMargins(4, 4, 4, 4);
    m_partContainer->layout()->setSpacing(2);
    static_cast<QVBoxLayout*>(m_partContainer->layout())->addStretch();

    m_partScrollArea->setWidget(m_partContainer);
    root->addWidget(m_partScrollArea, 1);

    // 刷新按钮
    m_btnRefresh = new QPushButton(QStringLiteral("刷新列表"), this);
    root->addWidget(m_btnRefresh);

    // 操作按钮行
    auto* btnRow = new QHBoxLayout();
    m_btnFocusCamera = new QPushButton(QStringLiteral("聚焦视角"), this);
    m_btnSetBox      = new QPushButton(QStringLiteral("设置选择框"), this);
    m_btnFocusBoth   = new QPushButton(QStringLiteral("两者同时"), this);
    btnRow->addWidget(m_btnFocusCamera);
    btnRow->addWidget(m_btnSetBox);
    btnRow->addWidget(m_btnFocusBoth);
    root->addLayout(btnRow);

    // 初始禁用操作按钮
    m_btnFocusCamera->setEnabled(false);
    m_btnSetBox->setEnabled(false);
    m_btnFocusBoth->setEnabled(false);

    // 应用深色主题样式
    const QString btnStyle = QString::fromUtf8(kDarkButtonQss);
    m_btnRefresh->setStyleSheet(btnStyle);
    m_btnFocusCamera->setStyleSheet(btnStyle);
    m_btnSetBox->setStyleSheet(btnStyle);
    m_btnFocusBoth->setStyleSheet(btnStyle);

    connect(m_btnRefresh,     &QPushButton::clicked, this, &igQtPartFocusWidget::RefreshPartList);
    connect(m_btnFocusCamera, &QPushButton::clicked, this, &igQtPartFocusWidget::onFocusCamera);
    connect(m_btnSetBox,      &QPushButton::clicked, this, &igQtPartFocusWidget::onSetSelectionBox);
    connect(m_btnFocusBoth,   &QPushButton::clicked, this, &igQtPartFocusWidget::onFocusBoth);
}

void igQtPartFocusWidget::setStatus(const QString& msg) {
    m_statusLabel->setText(msg);
}

std::vector<int> igQtPartFocusWidget::GetSelectedPartIds() const {
    std::vector<int> result;
    for (const auto& [pid, cb] : m_partCheckBoxes) {
        if (cb && cb->isChecked()) result.push_back(pid);
    }
    return result;
}

void igQtPartFocusWidget::RefreshPartList() {
    // 清空旧的 checkbox
    m_partCheckBoxes.clear();
    auto* containerLayout = static_cast<QVBoxLayout*>(m_partContainer->layout());
    QLayoutItem* item;
    while ((item = containerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    containerLayout->addStretch();

    m_btnFocusCamera->setEnabled(false);
    m_btnSetBox->setEnabled(false);
    m_btnFocusBoth->setEnabled(false);

    if (!m_scene) {
        setStatus(QStringLiteral("场景未设置"));
        return;
    }

    auto model = m_scene->GetCurrentModel();
    if (!model) {
        setStatus(QStringLiteral("无当前模型，请先加载模型"));
        return;
    }

    auto dataObj = model->GetDataObject();
    if (!dataObj) {
        setStatus(QStringLiteral("模型数据对象为空"));
        return;
    }

    if (!dataObj->HasBlockMapping()) {
        setStatus(QStringLiteral("当前模型无零件分割数据\n请先执行「零件分割 (PartSegmentation)」"));
        return;
    }

    auto* partIdArray = dataObj->GetBlockMapping();
    if (!partIdArray || partIdArray->GetNumberOfValues() == 0) {
        setStatus(QStringLiteral("零件分割数据为空"));
        return;
    }

    // 统计每个 part 的面数
    std::map<int, int> partFaceCount;
    const IGsize n = partIdArray->GetNumberOfValues();
    for (IGsize i = 0; i < n; ++i) {
        int pid = static_cast<int>(partIdArray->GetValue(i));
        if (pid >= 0) partFaceCount[pid]++;
    }

    if (partFaceCount.empty()) {
        setStatus(QStringLiteral("未找到有效零件（所有 part_id 均为 -1）"));
        return;
    }

    // 在 stretch 前插入 checkbox（takeAt 清空了 stretch，重新加）
    // 先移除刚才加的 stretch，再填 checkbox，最后加回 stretch
    while (containerLayout->count() > 0) {
        auto* it = containerLayout->takeAt(0);
        delete it;
    }

    const QString cbStyle =
        "QCheckBox { color: rgba(255,255,255,204); font-size: 10pt; padding: 3px 6px; } "
        "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid rgba(255,255,255,80); "
        "                       border-radius: 3px; background-color: #2A2A2A; } "
        "QCheckBox::indicator:checked { background-color: #094771; border: 1px solid #4FC3F7; } "
        "QCheckBox:hover { background-color: #2F2F2F; border-radius: 3px; }";

    for (auto& [pid, faceCount] : partFaceCount) {
        QString label = QString("Part %1  (%2 Cells)").arg(pid).arg(faceCount);
        auto* cb = new QCheckBox(label, m_partContainer);
        cb->setStyleSheet(cbStyle);
        containerLayout->addWidget(cb);
        m_partCheckBoxes.append({pid, cb});
    }
    containerLayout->addStretch();

    setStatus(QString(QStringLiteral("共 %1 个零件，可勾选后操作")).arg(partFaceCount.size()));
    m_btnFocusCamera->setEnabled(true);
    m_btnSetBox->setEnabled(true);
    m_btnFocusBoth->setEnabled(true);
}

bool igQtPartFocusWidget::computeBoundingBoxForSelected(iGame::BoundingBox& outBBox) const {
    if (!m_scene) return false;

    auto model = m_scene->GetCurrentModel();
    if (!model) return false;

    auto dataObj = model->GetDataObject();
    if (!dataObj || !dataObj->HasBlockMapping()) return false;

    auto* partIdArray = dataObj->GetBlockMapping();
    if (!partIdArray) return false;

    // 收集勾选的 part id
    std::set<int> selectedParts;
    for (const auto& [pid, cb] : m_partCheckBoxes) {
        if (cb && cb->isChecked()) selectedParts.insert(pid);
    }
    if (selectedParts.empty()) return false;

    // 根据 DataObject 类型分支处理
    IGenum objType = dataObj->GetDataObjectType();

    outBBox.setNull();
    bool found = false;
    const IGsize elementCount = partIdArray->GetNumberOfValues();

    switch (objType) {
        case IG_SURFACE_MESH: {
            SurfaceMesh::Pointer surfMesh = DynamicCast<SurfaceMesh>(dataObj);
            for (IGsize elemId = 0; elemId < elementCount; ++elemId) {
                int pid = static_cast<int>(partIdArray->GetValue(elemId));
                if (selectedParts.find(pid) == selectedParts.end()) continue;
                auto* face = surfMesh->GetFace(elemId);
                if (!face) continue;
                for (IGsize vi = 0; vi < face->GetNumberOfPoints(); ++vi) {
                    outBBox.add(surfMesh->GetPoint(face->GetPointId(vi)));
                    found = true;
                }
            }
            break;
        }
        case IG_UNSTRUCTURED_MESH: {
            UnstructuredMesh::Pointer unstrMesh = DynamicCast<UnstructuredMesh>(dataObj);
            for (IGsize elemId = 0; elemId < elementCount; ++elemId) {
                int pid = static_cast<int>(partIdArray->GetValue(elemId));
                if (selectedParts.find(pid) == selectedParts.end()) continue;
                const igIndex* ptIds = nullptr;
                int nPts = unstrMesh->GetCellPointIds(elemId, ptIds);
                for (int vi = 0; vi < nPts; ++vi) {
                    outBBox.add(unstrMesh->GetPoint(ptIds[vi]));
                    found = true;
                }
            }
            break;
        }
        case IG_VOLUME_MESH: {
            VolumeMesh::Pointer volMesh = DynamicCast<VolumeMesh>(dataObj);
            igIndex ptIds[64];
            for (IGsize elemId = 0; elemId < elementCount; ++elemId) {
                int pid = static_cast<int>(partIdArray->GetValue(elemId));
                if (selectedParts.find(pid) == selectedParts.end()) continue;
                int nPts = volMesh->GetVolumePointIds(elemId, ptIds);
                for (int vi = 0; vi < nPts; ++vi) {
                    outBBox.add(volMesh->GetPoint(ptIds[vi]));
                    found = true;
                }
            }
            break;
        }
        default: {
            return false;
        } break;
    }

    return found && !outBBox.isNull();
}

void igQtPartFocusWidget::onFocusCamera() {
    if (!m_scene || !m_rendererWidget) return;

    BoundingBox bbox;
    if (!computeBoundingBoxForSelected(bbox)) {
        setStatus(QStringLiteral("请先在列表中勾选至少一个零件"));
        return;
    }

    m_scene->ResetCameraView(bbox);
    m_rendererWidget->update();
    emit SIGNAL_FocusApplied();
}

void igQtPartFocusWidget::onSetSelectionBox() {
    if (!m_scene || !m_rendererWidget) return;

    BoundingBox bbox;
    if (!computeBoundingBoxForSelected(bbox)) {
        setStatus(QStringLiteral("请先在列表中勾选至少一个零件"));
        return;
    }

    auto interactor = m_scene->GetInteractor();
    if (!interactor) return;

    Point pMin(bbox.min[0], bbox.min[1], bbox.min[2]);
    Point pMax(bbox.max[0], bbox.max[1], bbox.max[2]);

    if (interactor->HaveSpecialInteractor("SelectBox")) {
        auto boxStyle = DynamicCast<BoxStyle>(interactor->GetSpecialInteractor("SelectBox"));
        if (boxStyle) boxStyle->InitBox(pMin, pMax);
    } else {
        auto boxStyle = BoxStyle::New();
        boxStyle->Initialize(interactor);
        boxStyle->InitBox(pMin, pMax);
        interactor->_SetSpecialInteractor("SelectBox", boxStyle);
    }

    SelectionParameter::Instance().SetHaveBox(true);
    m_rendererWidget->update();
    emit SIGNAL_FocusApplied();
}

void igQtPartFocusWidget::onFocusBoth() {
    onFocusCamera();
    onSetSelectionBox();
}
