#include "IQWidgets/igQtCountCellVerticesWidget.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>

igQtCountCellVerticesWidget::igQtCountCellVerticesWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::CountCellVertices) {
    ui->setupUi(this);

    m_Filter = iGame::CountCellVerticesFilter::New();

    // 按钮 → 槽
    connect(ui->btnExecute, &QPushButton::clicked, this, &igQtCountCellVerticesWidget::ExecuteCount);
    connect(ui->btnExportCSV, &QPushButton::clicked, this, &igQtCountCellVerticesWidget::ExportCSV);
    connect(ui->btnPrevPage, &QPushButton::clicked, this, &igQtCountCellVerticesWidget::PrevPage);
    connect(ui->btnNextPage, &QPushButton::clicked, this, &igQtCountCellVerticesWidget::NextPage);

    // 翻页按钮初始禁用（执行后按数据量启用）
    ui->btnPrevPage->setEnabled(false);
    ui->btnNextPage->setEnabled(false);

    // 表格初始化：两列表头 + 拉伸
    QStringList headers;
    headers << QStringLiteral("单元编号") << QStringLiteral("顶点数");
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);   // 隐藏行号列，更紧凑
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 只读表格

    // 深色主题适配：程序全局是深色样式，表格必须显式配深底浅字，
    // 否则交替行会出现"白底白字"看不清（QTableWidget 默认底色是白的）。
    ui->tableWidget->setStyleSheet(R"(
        QTableWidget {
            background-color: #2b2b2b;
            alternate-background-color: #3a3a3a;
            color: #e8e8e8;
            gridline-color: #4a4a4a;
            selection-background-color: #4a6fa5;
            selection-color: #ffffff;
        }
        QHeaderView::section {
            background-color: #3a3a3a;
            color: #e8e8e8;
            border: none;
            padding: 4px;
        }
        QTableCornerButton::section { background-color: #3a3a3a; }
    )");
    ui->tableWidget->setAlternatingRowColors(true);
}

void igQtCountCellVerticesWidget::SetOriginDataObject(iGame::DataObject::Pointer obj) {
    this->m_OriginDataObject = obj;
    m_Counts = nullptr;       // 换了模型，旧统计结果作废
    m_ResultMesh = nullptr;   // 旧结果节点作废
    m_Generated = false;
    m_currentPage = 0;
}

// ------------------------------------------------------------------
// 按名字 + 挂载位置（IG_CELL）从属性集里找 cell_vertex_count 数组
// ------------------------------------------------------------------
iGame::ArrayObject::Pointer
igQtCountCellVerticesWidget::FindCountArray(iGame::DataObject::Pointer obj) {
    if (!obj) { return nullptr; }
    auto attrs = obj->GetAttributeSet();
    if (!attrs) { return nullptr; }

    // 遍历属性集，按名字 + 挂载位置匹配
    auto all = attrs->GetAllAttributes();
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || !attr.pointer) { continue; }
        if (attr.attachmentType != IG_CELL) { continue; }
        if (std::string(attr.pointer->GetName()) == "cell_vertex_count") {
            return attr.pointer;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------------
// 「执行」：跑 Filter → 取独立输出 → 填表格 → 通知主窗口加模型树
// ------------------------------------------------------------------
void igQtCountCellVerticesWidget::ExecuteCount() {
    if (!m_OriginDataObject) {
        QMessageBox::warning(this, "提示", "请先选中一个模型。");
        return;
    }

    // 标准 Filter 调用：New() → SetInput → Execute
    m_Filter->SetInput(m_OriginDataObject);
    if (!m_Filter->Execute()) {
        const QString reason = QString::fromStdString(m_Filter->GetMessage());
        QMessageBox::critical(this, "执行失败",
                              reason.isEmpty()
                                  ? QStringLiteral("CountCellVerticesFilter 执行失败，请查看日志。")
                                  : reason);
        return;
    }

    // 结果来自独立输出节点（不是原模型）
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(m_Filter->GetOutput());
    if (!out) {
        QMessageBox::critical(this, "执行失败", "输出结果不是有效的网格。");
        return;
    }

    // 复用结果容器：场景里始终是同一个结果对象，重复执行时数据被正确覆盖刷新，
    // 不会在场景里累积多个结果模型（避免多模型叠加渲染导致的卡顿）
    if (!m_ResultMesh) {
        m_ResultMesh = iGame::UnstructuredMesh::New();
        m_ResultMesh->SetName(m_OriginDataObject->GetName() + "_VertexCount");
    }
    m_ResultMesh->SetPoints(out->GetPoints());
    m_ResultMesh->SetCells(out->GetCells(), out->GetCellTypes());
    m_ResultMesh->SetAttributeSet(out->GetAttributeSet());
    m_ResultMesh->ForceReConvertToDrawableData();  // 结果几何/属性已更新，重建渲染数据

    m_Counts = FindCountArray(m_ResultMesh);
    if (!m_Counts) {
        // 理论上"执行成功必有数组"，这里只是兜底
        QMessageBox::critical(this, "执行失败", "输出网格中未找到 cell_vertex_count 属性数组。");
        return;
    }

    m_currentPage = 0;  // 每次执行回到第一页
    ShowPage();

    // 通知主窗口：首次加入模型树（并隐藏原模型），之后只刷新
    if (m_Generated) {
        emit UpdateCountModel(m_ResultMesh);
    } else {
        emit DrawCountModel(m_ResultMesh);
        m_Generated = true;
    }
}

// ------------------------------------------------------------------
// 按当前页填充表格 + 更新页码 / 摘要 / 翻页按钮状态
// ------------------------------------------------------------------
void igQtCountCellVerticesWidget::ShowPage() {
    if (!m_Counts) {
        ui->tableWidget->setRowCount(0);
        ui->lblPageInfo->setText(QStringLiteral("第 0 / 0 页"));
        ui->btnPrevPage->setEnabled(false);
        ui->btnNextPage->setEnabled(false);
        return;
    }

    const IGsize n = m_Counts->GetNumberOfValues();                // 数组长度 = 单元数
    const int pageCount = n == 0 ? 0 : static_cast<int>((n + kPageSize - 1) / kPageSize);
    if (m_currentPage < 0) { m_currentPage = 0; }
    if (pageCount > 0 && m_currentPage >= pageCount) { m_currentPage = pageCount - 1; }
    if (pageCount == 0) { m_currentPage = 0; }

    const IGsize start = static_cast<IGsize>(m_currentPage) * kPageSize;
    const IGsize end = (start + kPageSize < n) ? (start + kPageSize) : n;
    const IGsize shown = end - start;

    // 统计顶点数范围（min~max）—— 全量统计，与当前页无关
    IGsize minV = 0;
    IGsize maxV = 0;
    if (n > 0) {
        minV = maxV = static_cast<IGsize>(m_Counts->GetValue(0));
        for (IGsize i = 1; i < n; ++i) {
            const IGsize v = static_cast<IGsize>(m_Counts->GetValue(i));
            if (v < minV) { minV = v; }
            if (v > maxV) { maxV = v; }
        }
    }

    // 只填当前页的行
    ui->tableWidget->setUpdatesEnabled(false);
    ui->tableWidget->setRowCount(static_cast<int>(shown));
    for (IGsize i = 0; i < shown; ++i) {
        const IGsize cellId = start + i;
        const IGsize v = static_cast<IGsize>(m_Counts->GetValue(cellId));

        auto* idItem = new QTableWidgetItem(QString::number(static_cast<long long>(cellId)));
        auto* cntItem = new QTableWidgetItem(QString::number(static_cast<long long>(v)));
        idItem->setTextAlignment(Qt::AlignCenter);
        cntItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(static_cast<int>(i), 0, idItem);
        ui->tableWidget->setItem(static_cast<int>(i), 1, cntItem);
    }
    ui->tableWidget->setUpdatesEnabled(true);

    // 摘要
    QString summary;
    if (n == 0) {
        summary = QStringLiteral("该模型没有单元（0 个），未生成顶点统计数据。");
    } else if (pageCount > 1) {
        summary = QStringLiteral("共 %1 个单元，顶点数范围 %2 ~ %3（共 %4 页，用下方按钮翻页，或「导出CSV」看全量）")
                      .arg(static_cast<long long>(n))
                      .arg(static_cast<long long>(minV))
                      .arg(static_cast<long long>(maxV))
                      .arg(pageCount);
    } else {
        summary = QStringLiteral("共 %1 个单元，顶点数范围 %2 ~ %3")
                      .arg(static_cast<long long>(n))
                      .arg(static_cast<long long>(minV))
                      .arg(static_cast<long long>(maxV));
    }
    ui->lblSummary->setText(summary);

    // 页码与翻页按钮
    ui->lblPageInfo->setText(pageCount == 0
                                 ? QStringLiteral("第 0 / 0 页")
                                 : QStringLiteral("第 %1 / %2 页").arg(m_currentPage + 1).arg(pageCount));
    ui->btnPrevPage->setEnabled(m_currentPage > 0);
    ui->btnNextPage->setEnabled(pageCount > 0 && m_currentPage + 1 < pageCount);
}

void igQtCountCellVerticesWidget::PrevPage() {
    if (m_currentPage > 0) {
        --m_currentPage;
        ShowPage();
    }
}

void igQtCountCellVerticesWidget::NextPage() {
    const IGsize n = m_Counts ? m_Counts->GetNumberOfValues() : 0;
    const int pageCount = n == 0 ? 0 : static_cast<int>((n + kPageSize - 1) / kPageSize);
    if (m_currentPage + 1 < pageCount) {
        ++m_currentPage;
        ShowPage();
    }
}

// ------------------------------------------------------------------
// 「导出CSV」：把完整统计数据存成 .csv 文件（不截断，含全部单元）
// ------------------------------------------------------------------
void igQtCountCellVerticesWidget::ExportCSV() {
    if (!m_Counts || m_Counts->GetNumberOfValues() == 0) {
        QMessageBox::warning(this, "提示", "请先点击「执行」生成统计数据。");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "导出统计结果为 CSV", "cell_vertex_count.csv", "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) { return; }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法写入文件: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << "cell_id,vertex_count\n";   // 表头
    IGsize n = m_Counts->GetNumberOfValues();
    for (IGsize i = 0; i < n; ++i) {
        out << i << ',' << static_cast<long long>(m_Counts->GetValue(i)) << '\n';
    }
    file.close();

    QMessageBox::information(this, "导出成功", "已导出到:\n" + filePath);
}
