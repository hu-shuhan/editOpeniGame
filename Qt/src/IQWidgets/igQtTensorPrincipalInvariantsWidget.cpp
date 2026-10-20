#include <QPointer>
#include "IQWidgets/igQtTensorPrincipalInvariantsWidget.h"

// —— iGame 场景相关 ——
#include "iGameAttributeSet.h"
#include "iGameScene.h"
#include "iGameSceneManager.h"
#include "iGameSmartPointer.h"

// —— Qt 控件 ——
#include <QComboBox>
#include <QMessageBox>
#include <QStringList>

// ------------------------------------------------------------------
// 构造函数：加载 UI 并绑定信号
// ------------------------------------------------------------------
igQtTensorPrincipalInvariantsWidget::igQtTensorPrincipalInvariantsWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::TensorPrincipalInvariants) {
    ui->setupUi(this);

    m_Generated = false;
    m_Filter = nullptr;

    connect(ui->btnExecute, &QPushButton::clicked, this,
            &igQtTensorPrincipalInvariantsWidget::Execute);
}

// ------------------------------------------------------------------
// ReloadTensorArrays：只列出"能当张量"的数组
// ------------------------------------------------------------------
void igQtTensorPrincipalInvariantsWidget::ReloadTensorArrays() {
    ui->cbTensorArray->clear();
    if (m_OriginDataObject == nullptr) { return; }
    auto attrs = m_OriginDataObject->GetAttributeSet();
    if (attrs == nullptr) { return; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return; }

    int found = 0;
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT && attr.attachmentType != IG_CELL) { continue; }
        const int dim = attr.pointer->GetDimension();
        if (!iGame::TensorPrincipalInvariantsFilter::IsSymmetricTensor(dim, attr.type)) {
            continue;
        }
        const bool isPoint = (attr.attachmentType == IG_POINT);
        const QString name = QString::fromStdString(attr.pointer->GetName());
        const QString text =
            QStringLiteral("%1  [%2, %3 分量]")
                .arg(name, isPoint ? QStringLiteral("点数据") : QStringLiteral("单元数据"))
                .arg(dim);
        // 用 QStringList 同时携带"数组名 + 挂载位置"，执行时解析
        ui->cbTensorArray->addItem(text,
                                   QVariant(QStringList{name, isPoint ? "point" : "cell"}));
        ++found;
    }
    if (found == 0) {
        ui->cbTensorArray->addItem(QStringLiteral("（当前模型没有可用作张量的数组）"),
                                   QVariant(QStringList{}));
        ui->lblInfo->setText(
            QStringLiteral("提示：需要 6 分量数组（XX,YY,ZZ,XY,YZ,XZ），或标记为 IG_TENSOR 的 3 分量数组。"));
    } else {
        ui->lblInfo->setText(QStringLiteral("共找到 %1 个张量数组，选一个后点「执行」。").arg(found));
    }
}

// ------------------------------------------------------------------
// SetOriginDataObject：主窗口选中新模型时调用
// ------------------------------------------------------------------
void igQtTensorPrincipalInvariantsWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    m_OriginDataObject = m_d;
    m_Generated = false;
    m_Filter = iGame::TensorPrincipalInvariantsFilter::New();

    m_ResultMesh = iGame::UnstructuredMesh::New();
    if (m_OriginDataObject != nullptr) {
        m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_PrincipalInvariants");
        m_ResultMesh->SetAttributeSet(m_OriginDataObject->GetAttributeSet());
    }
    QPointer<igQtTensorPrincipalInvariantsWidget> guard(this);
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [guard]() { if (guard) guard->m_Generated = false; });

    ReloadTensorArrays();
}

// ------------------------------------------------------------------
// ApplyParametersToFilter：从下拉框读数组名与挂载位置
// ------------------------------------------------------------------
void igQtTensorPrincipalInvariantsWidget::ApplyParametersToFilter() {
    const QStringList data = ui->cbTensorArray->currentData().toStringList();
    if (data.size() != 2) {
        m_Filter->SetTensorArrayName("");
        return;
    }
    m_Filter->SetTensorArrayName(data.at(0).toStdString());
    m_Filter->SetArrayAttachment(data.at(1) == QStringLiteral("cell") ? IG_CELL : IG_POINT);
    m_Filter->SetScaleVectors(ui->cbScaleVectors->isChecked());
}

// ------------------------------------------------------------------
// RunFilterOn：对单个输入跑一遍 Filter
// ------------------------------------------------------------------
iGame::UnstructuredMesh::Pointer igQtTensorPrincipalInvariantsWidget::RunFilterOn(
    iGame::DataObject::Pointer input) {
    if (input == nullptr) { return nullptr; }
    m_Filter->SetInput(input);
    if (!m_Filter->Execute()) { return nullptr; }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(m_Filter->GetOutput());
    if (out == nullptr || out->GetNumberOfPoints() == 0) { return nullptr; }
    return out;
}

// ------------------------------------------------------------------
// Execute：「执行」按钮
// ------------------------------------------------------------------
void igQtTensorPrincipalInvariantsWidget::Execute() {
    if (!m_Filter) { m_Filter = iGame::TensorPrincipalInvariantsFilter::New(); }
    if (m_OriginDataObject == nullptr) {
        QMessageBox::warning(this, "提示", "请先在模型树中选中一个模型。");
        return;
    }
    ApplyParametersToFilter();
    if (m_Filter->GetTensorArrayName().empty()) {
        QMessageBox::warning(this, "提示", "请先在下拉框里选择一个张量数组。");
        return;
    }

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
    auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
    m_ResultMesh->ClearSubDataObject();
    m_ResultMesh->ViewCloudPicture(scene, -1, -1);

    if (m_OriginDataObject->HasSubDataObject()) {
        int failedChildren = 0;
        int addedChildren = 0;
        for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
             it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
            auto childMesh = RunFilterOn(it->second);
            if (childMesh == nullptr) {
                ++failedChildren;
                continue;
            }
            m_ResultMesh->AddSubDataObject(childMesh);
            ++addedChildren;
        }
        if (addedChildren == 0) {
            QMessageBox::information(this, "未生成结果",
                                     QString("当前模型没有处理成功的子对象（%1 个失败）。")
                                         .arg(failedChildren));
            return;
        }
        if (failedChildren > 0) {
            QMessageBox::warning(this, "提示",
                                 QString("有 %1 个子对象处理失败，已跳过。").arg(failedChildren));
        }
    } else {
        auto out = RunFilterOn(m_OriginDataObject);
        if (out == nullptr) {
            const QString reason = QString::fromStdString(m_Filter->GetMessage());
            QMessageBox::warning(this, "执行失败",
                                 reason.isEmpty() ? QStringLiteral("计算主值失败，请查看日志。")
                                                  : reason);
            return;
        }
        m_ResultMesh->SetPoints(out->GetPoints());
        m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
        m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
    }

    m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);

    // 面板提示：本次写出的 6 个数组名（前 3 个是主方向，后 3 个是主值）
    QString info = QStringLiteral("完成，新增 6 个数组：");
    for (const auto& name : m_Filter->GetOutputArrayNames()) {
        info += QStringLiteral("\n  ") + QString::fromStdString(name);
    }
    if (!m_Filter->GetMessage().empty()) {
        info += QStringLiteral("\n") + QString::fromStdString(m_Filter->GetMessage());
    }
    ui->lblInfo->setText(info);

    if (m_Generated) {
        UpdateResultModel(m_ResultMesh);
    } else {
        DrawResultModel(m_ResultMesh);
        m_Generated = true;
    }
}
