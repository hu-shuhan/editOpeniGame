#include <QPointer>
#include "IQWidgets/igQtTextureMapToCylinderWidget.h"

// —— iGame 场景相关 ——
#include "iGameScene.h"         // 场景：管理所有模型和渲染
#include "iGameSceneManager.h"  // 场景管理器：获取当前场景（单例）
#include "iGameSmartPointer.h"  // 智能指针定义

// —— Qt 控件 ——
#include <QCheckBox>
#include <QMessageBox>

// ------------------------------------------------------------------
// 构造函数：加载 UI 并绑定信号
// ------------------------------------------------------------------
igQtTextureMapToCylinderWidget::igQtTextureMapToCylinderWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::TextureMapToCylinder) {
    ui->setupUi(this);

    m_Generated = false;
    m_Filter = nullptr;  // Filter 实例延迟到执行时创建

    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtTextureMapToCylinderWidget::Execute);
    connect(ui->cbAutoAxis, &QCheckBox::toggled, this,
            [this](bool) { UpdateAxisControlsEnabled(); });
    UpdateAxisControlsEnabled();
}

// ------------------------------------------------------------------
// UpdateAxisControlsEnabled：自动求轴时不需要手填轴端点
// ------------------------------------------------------------------
void igQtTextureMapToCylinderWidget::UpdateAxisControlsEnabled() {
    const bool manual = !ui->cbAutoAxis->isChecked();
    ui->spP1x->setEnabled(manual);
    ui->spP1y->setEnabled(manual);
    ui->spP1z->setEnabled(manual);
    ui->spP2x->setEnabled(manual);
    ui->spP2y->setEnabled(manual);
    ui->spP2z->setEnabled(manual);
}

// ------------------------------------------------------------------
// ApplyParametersToFilter：把面板上的参数写进 Filter
// ------------------------------------------------------------------
void igQtTextureMapToCylinderWidget::ApplyParametersToFilter() {
    m_Filter->SetAutomaticCylinderGeneration(ui->cbAutoAxis->isChecked());
    m_Filter->SetPreventSeam(ui->cbPreventSeam->isChecked());
    m_Filter->SetPoint1(ui->spP1x->value(), ui->spP1y->value(), ui->spP1z->value());
    m_Filter->SetPoint2(ui->spP2x->value(), ui->spP2y->value(), ui->spP2z->value());
}

// ------------------------------------------------------------------
// SetOriginDataObject：主窗口选中新模型时调用
// ------------------------------------------------------------------
void igQtTextureMapToCylinderWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    m_OriginDataObject = m_d;
    m_Generated = false;
    m_Filter = iGame::TextureMapToCylinderFilter::New();

    // 结果容器：一个空的 UnstructuredMesh，名字 = 原模型名 + "_TCoords"
    m_ResultMesh = iGame::UnstructuredMesh::New();
    if (m_OriginDataObject != nullptr) {
        m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_TCoords");
        m_ResultMesh->SetAttributeSet(m_OriginDataObject->GetAttributeSet());
    }
    // 结果模型被外部删除时重置标记，避免下次误用
    QPointer<igQtTextureMapToCylinderWidget> guard(this);
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [guard]() { if (guard) guard->m_Generated = false; });

    ui->lblInfo->setText(QStringLiteral("已就绪：设置圆柱轴后点「执行」。"));
}

// ------------------------------------------------------------------
// RunFilterOn：对单个输入跑一遍 Filter
// ------------------------------------------------------------------
iGame::UnstructuredMesh::Pointer igQtTextureMapToCylinderWidget::RunFilterOn(
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
void igQtTextureMapToCylinderWidget::Execute() {
    if (!m_Filter) { m_Filter = iGame::TextureMapToCylinderFilter::New(); }
    if (m_OriginDataObject == nullptr) {
        QMessageBox::warning(this, "提示", "请先在模型树中选中一个模型。");
        return;
    }
    ApplyParametersToFilter();

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

    // 记录并临时关闭结果模型的着色，避免刷新过程中的中间状态
    auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
    auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();
    m_ResultMesh->ClearSubDataObject();
    m_ResultMesh->ViewCloudPicture(scene, -1, -1);

    if (m_OriginDataObject->HasSubDataObject()) {
        // —— 多块模型：逐个子对象处理，结果作为子对象挂上 ——
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
            QMessageBox::information(this, "未生成纹理坐标",
                                     QString("当前模型没有处理成功的子对象（%1 个失败）。")
                                         .arg(failedChildren));
            return;
        }
        if (failedChildren > 0) {
            QMessageBox::warning(this, "提示",
                                 QString("有 %1 个子对象处理失败，已跳过。").arg(failedChildren));
        }
    } else {
        // —— 单个网格（最常见）——
        auto out = RunFilterOn(m_OriginDataObject);
        if (out == nullptr) {
            const QString reason = QString::fromStdString(m_Filter->GetMessage());
            QMessageBox::warning(this, "生成失败",
                                 reason.isEmpty() ? QStringLiteral("生成纹理坐标失败，请查看日志。")
                                                  : reason);
            return;
        }
        // 把结果网格的点/单元/属性拷贝进结果容器（属性集里已含新增的纹理坐标数组）
        m_ResultMesh->SetPoints(out->GetPoints());
        m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
        m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
    }

    // 恢复着色状态
    m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);

    // 面板提示：数组名 + 本次实际使用的轴（自动求轴时是算出来的结果）
    double p1[3] = {0, 0, 0};
    double p2[3] = {0, 0, 0};
    m_Filter->GetResolvedAxis(p1, p2);
    ui->lblInfo->setText(QStringLiteral("完成：Point Data 新增 \"%1\"（2 分量）；轴 (%2, %3, %4) → (%5, %6, %7)")
                             .arg(QString::fromStdString(m_Filter->GetTCoordsArrayName()))
                             .arg(p1[0], 0, 'g', 4)
                             .arg(p1[1], 0, 'g', 4)
                             .arg(p1[2], 0, 'g', 4)
                             .arg(p2[0], 0, 'g', 4)
                             .arg(p2[1], 0, 'g', 4)
                             .arg(p2[2], 0, 'g', 4));

    // 通知主窗口：第一次加模型到场景树，之后只刷新
    if (m_Generated) {
        UpdateResultModel(m_ResultMesh);
    } else {
        DrawResultModel(m_ResultMesh);
        m_Generated = true;
    }
}
