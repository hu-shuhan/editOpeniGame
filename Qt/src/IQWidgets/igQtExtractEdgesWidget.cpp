#include "IQWidgets/igQtExtractEdgesWidget.h"

// —— iGame 场景相关 ——
#include "iGameScene.h"         // 场景：管理所有模型和渲染
#include "iGameSceneManager.h"  // 场景管理器：获取当前场景（单例）
#include "iGameSmartPointer.h"  // 智能指针定义

// —— Qt 对话框 ——
#include <QFileDialog>   // 文件选择对话框（导出时选保存路径）
#include <QMessageBox>   // 消息弹窗（提示/成功/失败）

// ------------------------------------------------------------------
// 构造函数：加载 UI 并绑定按钮信号
// ------------------------------------------------------------------
igQtExtractEdgesWidget::igQtExtractEdgesWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ExtractEdges) {
    ui->setupUi(this);  // 把 ExtractEdges.ui 的内容装载到本控件

    m_Generated = false;   // 初始还没生成过结果
    m_Extracter = nullptr; // Filter 实例延迟到 SetOriginDataObject 时创建

    // 把界面上的两个按钮点击事件，连接到本类的槽函数：
    // 点「执行」→ ExtractEdges()；点「导出边为 VTK」→ ExportEdges()
    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtExtractEdgesWidget::ExtractEdges);
    connect(ui->btnExport, &QPushButton::clicked, this, &igQtExtractEdgesWidget::ExportEdges);
}

// ------------------------------------------------------------------
// SetOriginDataObject：主窗口选中新模型时调用，记录输入并准备结果容器
// ------------------------------------------------------------------
void igQtExtractEdgesWidget::SetOriginDataObject(iGame::DataObject::Pointer m_d) {
    this->m_OriginDataObject = m_d;  // 记录当前选中的输入模型

    m_Generated = false;                     // 重置标记（换模型了，之前的边作废）
    m_Extracter = iGame::ExtractEdgesFilter::New();  // 新建一个提取器实例

    // 新建"结果容器"：一个空的 UnstructuredMesh，用来装提取出来的边
    m_ResultMesh = iGame::UnstructuredMesh::New();

    // 结果模型的名字 = 原模型名 + "_Edges"（场景树里一眼认出）
    m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_Edges");

    // 让结果继承输入模型的属性集（这样边可以按原模型的属性着色）
    m_ResultMesh->SetAttributeSet(m_OriginDataObject->GetAttributeSet());

    // 监听"删除事件"：如果结果模型被外部删掉，重置标记，避免下次误用
    m_ResultMesh->AddObserver(iGame::Command::DeleteEvent, [&]() -> void { m_Generated = false; });
}

// ------------------------------------------------------------------
// ExtractEdges：「执行」按钮 —— 跑一遍提取算法并通知主窗口显示
// ------------------------------------------------------------------
void igQtExtractEdgesWidget::ExtractEdges() {
    // 保险：如果还没有 Filter 实例就创建一个
    if (!m_Extracter) { m_Extracter = iGame::ExtractEdgesFilter::New(); }

    // 取当前场景（刷新视图用）
    // SceneManager::Instance()：场景管理器单例；GetCurrentScene()：返回当前激活的场景
    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

    // 记录结果模型当前的"激活属性"状态（执行期间会临时重置，最后再恢复）
    // GetAttributeIndex()：当前用于着色的属性编号（-1 表示无色）
    // GetAttributeDimension()：该属性的维度（标量=1，向量=3）
    auto oldAttributeIndex = m_ResultMesh->GetAttributeIndex();
    auto oldAttributeDimension = m_ResultMesh->GetAttributeDimension();

    // 清掉上次的子对象，并临时把结果模型切到"无色模式"（渲染时避免干扰）
    // ClearSubDataObject()：删除结果模型下挂的所有子对象（多块网格的旧结果）
    // ViewCloudPicture(scene, -1, -1)：切换显示模式；index=-1 表示关闭属性着色（单色）
    m_ResultMesh->ClearSubDataObject();
    m_ResultMesh->ViewCloudPicture(scene, -1, -1);

    if (m_OriginDataObject->HasSubDataObject()) {
        // —— 情况 A：输入模型带子对象（多块网格）——
        // 每个子对象单独提取，提取结果作为子对象挂到结果模型下
        int failedChildren = 0;
        int addedChildren = 0;
        for (auto it = m_OriginDataObject->SubDataObjectIteratorBegin();
             it != m_OriginDataObject->SubDataObjectIteratorEnd(); it++) {
            auto childObject = it->second;          // 子对象指针（map 的 value）
            if (childObject == nullptr) { continue; }
            m_Extracter->SetInput(childObject);     // 喂入一个子对象
            if (!m_Extracter->Execute()) {          // 检查返回值，失败不伪装成成功
                ++failedChildren;
                continue;
            }
            auto childMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(m_Extracter->GetOutput());
            if (childMesh != nullptr && childMesh->GetNumberOfCells() > 0) {
                m_ResultMesh->AddSubDataObject(childMesh);
                ++addedChildren;
            } else {
                ++failedChildren;                    // 0 条边也算"没有结果"
            }
        }
        if (addedChildren == 0) {
            // 全部子对象都没有可提取的边：不创建结果节点
            QMessageBox::information(
                this, "未提取到边",
                QString("当前模型没有可提取的边（%1 个子对象被跳过）。").arg(failedChildren));
            return;
        }
        if (failedChildren > 0) {
            QMessageBox::warning(this, "提示",
                                 QString("有 %1 个子对象未能提取边，已跳过。").arg(failedChildren));
        }
    } else {
        // —— 情况 B：普通单个网格（最常见）——
        m_Extracter->SetInput(m_OriginDataObject);  // 喂入整个模型
        if (!m_Extracter->Execute()) {              // 检查返回值，失败不伪装成成功
            const QString reason = QString::fromStdString(m_Extracter->GetMessage());
            QMessageBox::warning(this, "提取失败",
                                 reason.isEmpty() ? QStringLiteral("边提取失败，请查看日志。") : reason);
            return;
        }
        auto out = m_Extracter->GetEdgesMesh();     // 拿到边网格（便捷类型转换）
        if (out != nullptr && out->GetNumberOfCells() > 0) {
            // 把边网格的点/单元/属性拷贝到结果容器里：
            // SetPoints()：设置点坐标数组；SetCells(conn, types)：设置单元连接+类型；
            // SetAttributeSet()：设置属性集（Point Data 深拷贝保留 + Cell Data 按来源单元重映射）
            m_ResultMesh->SetPoints(out->GetPoints());
            m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
            m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
        } else {
            // 0 条边：不创建结果节点，明确提示（含跳过的单元统计）
            const QString msg = QString::fromStdString(m_Extracter->GetMessage());
            QMessageBox::information(this, "未提取到边",
                                     msg.isEmpty() ? QStringLiteral("当前模型没有可提取的边。") : msg);
            return;
        }
    }

    // 恢复之前的属性显示状态，并刷新视图
    // ViewCloudPicture(scene, index, dim)：按保存的属性编号/维度恢复着色
    m_ResultMesh->ViewCloudPicture(scene, oldAttributeIndex, oldAttributeDimension);

    // 通知主窗口把结果显示到场景里：
    //   第一次 → DrawEdgesModel（加入模型树）；之后 → UpdateEdgesModel（只刷新）
    if (m_Generated) {
        UpdateEdgesModel(m_ResultMesh);
    } else {
        DrawEdgesModel(m_ResultMesh);
        m_Generated = true;
    }
}

// ------------------------------------------------------------------
// ExportEdges：「导出边为 VTK」按钮 —— 把提取结果写成 .vtk 文件
// ------------------------------------------------------------------
void igQtExtractEdgesWidget::ExportEdges() {
    // 前置检查：必须已经执行过提取，否则没有东西可导出
    if (!m_Generated || !m_ResultMesh) {
        QMessageBox::warning(this, "提示", "请先点击「执行」提取边，再进行导出。");
        return;
    }

    // 弹出"另存为"对话框，默认文件名 = 结果模型名 + .vtk
    QString defaultName = QString::fromStdString(m_ResultMesh->GetName()) + ".vtk";
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出边模型为 VTK", defaultName, "VTK 文件 (*.vtk)");
    if (filePath.isEmpty()) { return; }  // 用户点了取消

    // 复用框架的 ExportEdgesFilter（内部组合 VTKWriter），严格按 Filter 规范调用：
    //   New() → SetInput → SetFilePath → Execute()
    auto exporter = iGame::ExportEdgesFilter::New();
    exporter->SetInput(m_ResultMesh);                      // 要导出的边网格
    exporter->SetFilePath(filePath.toStdString());         // 保存路径
    if (exporter->Execute()) {                             // 执行导出
        QMessageBox::information(this, "导出成功", "边模型已导出到:\n" + filePath);
    } else {
        QMessageBox::critical(this, "导出失败", "导出失败，请查看控制台日志。");
    }
}
