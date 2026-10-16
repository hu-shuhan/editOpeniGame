#include "IQWidgets/igQtExtractComponentWidget.h"
#include "iGameFilterIncludes.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <QWheelEvent>
#include <set>

igQtExtractComponentWidget::igQtExtractComponentWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ExtractComponentWidget) {
    ui->setupUi(this);
    ui->comboBox_InputArray->installEventFilter(this);
    connect(ui->btnApply, &QPushButton::clicked, this, &igQtExtractComponentWidget::Apply);
    connect(ui->comboBox_InputArray, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        UpdateComponentOptions();
    });
}

// 输入数组滚轮增强：多项时手动循环切换选项（并联动分量），单项/空时吞掉防止面板滚动
bool igQtExtractComponentWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == ui->comboBox_InputArray && event->type() == QEvent::Wheel) {
        const int count = ui->comboBox_InputArray->count();
        if (count <= 1) return true;
        auto* wheel = static_cast<QWheelEvent*>(event);
        const int cur = ui->comboBox_InputArray->currentIndex();
        int next = cur + (wheel->angleDelta().y() > 0 ? -1 : 1);
        next = (next % count + count) % count;
        ui->comboBox_InputArray->setCurrentIndex(next);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void igQtExtractComponentWidget::SetOriginDataObject(iGame::DataObject::Pointer data) {
    m_OriginDataObject = data;
    m_Generated = false;
    m_ResultDataObject = nullptr;
    InitInputArrayList();
    UpdateComponentOptions();
}

void igQtExtractComponentWidget::InitInputArrayList() {
    ui->comboBox_InputArray->clear();
    m_InputArrayNames.clear();
    m_InputArrayAttachments.clear();
    if (m_OriginDataObject == nullptr) return;
    auto attrSet = m_OriginDataObject->GetAttributeSet();
    if (attrSet == nullptr) return;
    // List all data arrays with their attachment type, so same-named Point/Cell arrays
    // (e.g. Velocity on both PointData and CellData) can be told apart in the combo.
    auto all = attrSet->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.IsNone()) continue;
        const QString name = QString::fromStdString(attr.pointer->GetName());
        const QString label = attr.attachmentType == IG_CELL
                                      ? name + QStringLiteral(" (Cell)")
                                      : name + QStringLiteral(" (Point)");
        ui->comboBox_InputArray->addItem(label);
        m_InputArrayNames.push_back(name);
        m_InputArrayAttachments.push_back(static_cast<int>(attr.attachmentType));
    }
    // Defaults to the first array (input array is optional).
}

int igQtExtractComponentWidget::GetSelectedInputDimension() {
    if (m_OriginDataObject == nullptr) return 0;
    const int idx = ui->comboBox_InputArray->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(m_InputArrayNames.size())) return 0;
    auto attrSet = m_OriginDataObject->GetAttributeSet();
    if (attrSet == nullptr) return 0;
    // Look up by raw name + attachment type to disambiguate same-named arrays.
    auto all = attrSet->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& candidate = all->GetElement(i);
        if (candidate.IsNone()) continue;
        if (candidate.pointer->GetName() != m_InputArrayNames[idx].toStdString()) continue;
        if (candidate.attachmentType != m_InputArrayAttachments[idx]) continue;
        return candidate.pointer->GetDimension();
    }
    return 0;
}

// 按输入数组维度动态生成分量选项：1 维禁用；2 维 X/Y；3 维 X/Y/Z；更高维用数字 1..N
void igQtExtractComponentWidget::UpdateComponentOptions() {
    ui->comboBox_Component->clear();
    const int dim = GetSelectedInputDimension();
    if (dim <= 1) {
        ui->comboBox_Component->setEnabled(false);
        return;
    }
    ui->comboBox_Component->setEnabled(true);
    if (dim == 2) {
        ui->comboBox_Component->addItem(QStringLiteral("X"));
        ui->comboBox_Component->addItem(QStringLiteral("Y"));
    } else if (dim == 3) {
        ui->comboBox_Component->addItem(QStringLiteral("X"));
        ui->comboBox_Component->addItem(QStringLiteral("Y"));
        ui->comboBox_Component->addItem(QStringLiteral("Z"));
    } else {
        for (int i = 1; i <= dim; ++i) {
            ui->comboBox_Component->addItem(QString::number(i));
        }
    }
}

void igQtExtractComponentWidget::Apply() {
    if (m_OriginDataObject == nullptr) return;

    if (!ui->comboBox_Component->isEnabled() || ui->comboBox_Component->count() == 0) {
        Q_EMIT ApplyFailed(QStringLiteral("当前输入数组维度不足，无可提取的分量"));
        return;
    }

    const int inputIndex = ui->comboBox_InputArray->currentIndex();
    if (inputIndex < 0 || inputIndex >= static_cast<int>(m_InputArrayNames.size())) {
        Q_EMIT ApplyFailed(QStringLiteral("Please select an input array"));
        return;
    }
    const std::string inputName = m_InputArrayNames[inputIndex].toStdString();
    const IGenum inputAttachment = static_cast<IGenum>(m_InputArrayAttachments[inputIndex]);
    std::string outputName = ui->lineEdit_OutputName->text().toStdString();
    if (outputName.empty()) outputName = "Result";
    const int component = ui->comboBox_Component->currentIndex();

    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(m_OriginDataObject);
    filter->SetInputArrayName(inputName);
    filter->SetInputAttachmentType(inputAttachment);
    filter->SetOutputArrayName(outputName);
    filter->SetComponent(component);
    if (!filter->Execute()) {
        Q_EMIT ApplyFailed(QString::fromStdString(filter->GetMessage()));
        return;
    }
    auto result = filter->GetOutput();
    if (result == nullptr) {
        Q_EMIT ApplyFailed(QStringLiteral("提取分量执行失败"));
        return;
    }

    if (!m_Generated) {
        // 首次执行：按全局序列命名（方案 Y）并新增模型树节点
        result->SetName(UniqueResultName(m_OriginDataObject->GetName()));
        m_ResultDataObject = result;
        // 场景/模型树删除结果时重置：同一输入再次执行可重新生成节点
        m_ResultDataObject->AddObserver(iGame::Command::DeleteEvent, [this]() -> void { m_Generated = false; });
        m_Generated = true;
        Q_EMIT DrawExtractComponentModel(m_ResultDataObject);
    } else {
        // 再次执行：修改上次 filter 结果（不生成新节点），更新同一结果对象内容
        RebuildResultObject(result);
        Q_EMIT UpdateExtractComponentModel(m_ResultDataObject);
    }
}

// 提取分量结果节点命名（方案 Y）：输入名_ExtractComponent_序号，
// 序号 = 该 filter 类型在场景中的全局序列（与输入名无关），复用最小空缺（删除后序号可复用）
std::string igQtExtractComponentWidget::UniqueResultName(const std::string& inputName) {
    std::set<int> used;
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene != nullptr) {
        auto modelList = scene->GetModelList();
        if (modelList != nullptr) {
            const std::string marker = "_ExtractComponent_";
            for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
                if (it->second == nullptr || it->second->GetDataObject() == nullptr) continue;
                const std::string& name = it->second->GetDataObject()->GetName();
                const size_t pos = name.rfind(marker);
                if (pos == std::string::npos) continue;
                const std::string suffix = name.substr(pos + marker.size());
                if (suffix.empty() || suffix.find_first_not_of("0123456789") != std::string::npos) continue;
                used.insert(std::stoi(suffix));
            }
        }
    }
    int n = 1;
    while (used.count(n) != 0) ++n;
    return inputName + "_ExtractComponent_" + std::to_string(n);
}

// 把新结果的内容（几何共享 + 属性集 + 名字）搬运到已挂载的结果对象上
void igQtExtractComponentWidget::RebuildResultObject(iGame::DataObject::Pointer fresh) {
    auto src = iGame::DynamicCast<iGame::UnstructuredMesh>(fresh);
    auto dst = iGame::DynamicCast<iGame::UnstructuredMesh>(m_ResultDataObject);
    if (src != nullptr && dst != nullptr) {
        dst->SetPoints(src->GetPoints());
        dst->SetCells(src->GetCells(), iGame::UnsignedIntArray::Pointer(src->GetCellTypes()));
        dst->SetAttributeSet(src->GetAttributeSet());
        return;
    }
    auto ssrc = iGame::DynamicCast<iGame::SurfaceMesh>(fresh);
    auto sdst = iGame::DynamicCast<iGame::SurfaceMesh>(m_ResultDataObject);
    if (ssrc != nullptr && sdst != nullptr) {
        sdst->SetPoints(ssrc->GetPoints());
        sdst->SetFaces(ssrc->GetFaces());
        sdst->SetAttributeSet(ssrc->GetAttributeSet());
        return;
    }
    m_ResultDataObject = fresh;
}
