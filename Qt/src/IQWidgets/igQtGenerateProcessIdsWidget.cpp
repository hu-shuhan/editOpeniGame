#include "IQWidgets/igQtGenerateProcessIdsWidget.h"
#include "iGameFilterIncludes.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <QTimer>
#include <set>

igQtGenerateProcessIdsWidget::igQtGenerateProcessIdsWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::GenerateProcessIdsWidget) {
    ui->setupUi(this);
    connect(ui->btnApply, &QPushButton::clicked, this, &igQtGenerateProcessIdsWidget::Apply);
}

void igQtGenerateProcessIdsWidget::SetOriginDataObject(iGame::DataObject::Pointer data) {
    m_OriginDataObject = data;
    m_Generated = false;
    m_ResultDataObject = nullptr;
}

void igQtGenerateProcessIdsWidget::Apply() {
    if (m_OriginDataObject == nullptr) {
        Q_EMIT ApplyFailed(QStringLiteral("请先选择一个模型。"));
        return;
    }

    // Apply 按钮点击反馈：变暗一秒再恢复（与提取分量面板一致）
    const QString normalStyle = ui->btnApply->styleSheet();
    ui->btnApply->setEnabled(false);
    ui->btnApply->setStyleSheet(QStringLiteral("QPushButton { background-color: #3A3A3D; color: #808080; }"));
    QTimer::singleShot(1000, this, [this, normalStyle]() {
        ui->btnApply->setEnabled(true);
        ui->btnApply->setStyleSheet(normalStyle);
    });

    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(m_OriginDataObject);
    filter->SetGeneratePointData(ui->checkBox_PointData->isChecked());
    filter->SetGenerateCellData(ui->checkBox_CellData->isChecked());
    if (!filter->Execute()) {
        Q_EMIT ApplyFailed(QString::fromStdString(filter->GetMessage()));
        return;
    }
    auto result = filter->GetOutput();
    if (result == nullptr) {
        Q_EMIT ApplyFailed(QStringLiteral("生成进程ID执行失败"));
        return;
    }

    if (!m_Generated) {
        // 首次执行：按全局序列命名（输入名_ProcessIds_序号）并新增模型树节点
        result->SetName(UniqueResultName(m_OriginDataObject->GetName()));
        m_ResultDataObject = result;
        // 场景/模型树删除结果时重置：同一输入再次执行可重新生成节点
        m_ResultDataObject->AddObserver(iGame::Command::DeleteEvent, [this]() -> void { m_Generated = false; });
        m_Generated = true;
        Q_EMIT DrawProcessIdsModel(m_ResultDataObject);
    } else {
        // 再次执行：修改上次结果（不生成新节点），更新同一结果对象内容
        RebuildResultObject(result);
        Q_EMIT UpdateProcessIdsModel(m_ResultDataObject);
    }
}

// 结果节点命名：输入名_ProcessIds_序号，序号 = 该 filter 类型在场景中的全局序列，
// 复用最小空缺（删除后序号可复用）
std::string igQtGenerateProcessIdsWidget::UniqueResultName(const std::string& inputName) {
    std::set<int> used;
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (scene != nullptr) {
        auto modelList = scene->GetModelList();
        if (modelList != nullptr) {
            const std::string marker = "_ProcessIds_";
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
    return inputName + "_ProcessIds_" + std::to_string(n);
}

// 把新结果的内容（几何共享 + 属性集 + 名字）搬运到已挂载的结果对象上
void igQtGenerateProcessIdsWidget::RebuildResultObject(iGame::DataObject::Pointer fresh) {
    auto src = iGame::DynamicCast<iGame::UnstructuredMesh>(fresh);
    auto dst = iGame::DynamicCast<iGame::UnstructuredMesh>(m_ResultDataObject);
    if (src != nullptr && dst != nullptr) {
        dst->SetPoints(src->GetPoints());
        dst->SetCells(src->GetCells(), iGame::UnsignedIntArray::Pointer(src->GetCellTypes()));
        dst->SetAttributeSet(src->GetAttributeSet());
        return;
    }
    auto vsrc = iGame::DynamicCast<iGame::VolumeMesh>(fresh);
    auto vdst = iGame::DynamicCast<iGame::VolumeMesh>(m_ResultDataObject);
    if (vsrc != nullptr && vdst != nullptr) {
        vdst->SetPoints(vsrc->GetPoints());
        vdst->SetFaces(vsrc->GetFaces());
        vdst->SetVolumes(vsrc->GetVolumes());
        vdst->SetAttributeSet(vsrc->GetAttributeSet());
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
