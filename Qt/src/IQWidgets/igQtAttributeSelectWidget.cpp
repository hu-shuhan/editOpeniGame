#include <IQWidgets/igQtAttributeSelectWidget.h>

#include <IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <iGameAttributeSet.h>
#include <iGameScene.h>
#include <iGameSceneManager.h>
#include "MeshReport/iGameMeshReportGenerator.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QThread>

using namespace iGame;

namespace {
const char* kDarkButtonQss = R"(
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

const char* kCheckBoxQss = R"(
QCheckBox {
    color: rgba(255, 255, 255, 204);
    font-size: 10pt;
    padding: 3px 6px;
}
QCheckBox::indicator {
    width: 14px; height: 14px;
    border: 1px solid rgba(255, 255, 255, 80);
    border-radius: 3px;
    background-color: #2A2A2A;
}
QCheckBox::indicator:checked {
    background-color: #094771;
    border: 1px solid #4FC3F7;
}
QCheckBox:hover {
    background-color: #2F2F2F;
    border-radius: 3px;
}
)";
} // namespace

igQtAttributeSelectWidget::igQtAttributeSelectWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

igQtAttributeSelectWidget::~igQtAttributeSelectWidget() {
    // 窗口关闭时 widget 被销毁：若后台报告线程仍在运行，不能直接销毁 QThread（Qt 会 fatal）。
    // 让线程自然结束后自删（QThread::finished -> deleteLater），期间不触碰任何 widget 成员。
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->requestInterruption();
        m_workerThread->setParent(nullptr);
        m_workerThread = nullptr;
    }
}

void igQtAttributeSelectWidget::setupUI() {
    setStyleSheet("igQtAttributeSelectWidget { background-color: #222222; } "
                  "QLabel { color: rgba(255,255,255,204); font-size: 10pt; } "
                  "QScrollArea { background-color: #1E1E1E; border: 1px solid #3C3C3C; border-radius: 4px; } "
                  "QWidget#attrContainer { background-color: #1E1E1E; }");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    m_statusLabel = new QLabel(QStringLiteral("请先点击\"刷新属性列表\""), this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setMinimumHeight(180);

    m_container = new QWidget();
    m_container->setObjectName("attrContainer");
    m_container->setLayout(new QVBoxLayout());
    m_container->layout()->setContentsMargins(4, 4, 4, 4);
    m_container->layout()->setSpacing(2);
    static_cast<QVBoxLayout*>(m_container->layout())->addStretch();

    m_scrollArea->setWidget(m_container);
    root->addWidget(m_scrollArea, 1);

    m_btnRefresh = new QPushButton(QStringLiteral("刷新属性列表"), this);
    m_btnGenerate = new QPushButton(QStringLiteral("生成报告"), this);
    m_btnGenerate->setEnabled(false);

    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(m_btnRefresh);
    btnRow->addWidget(m_btnGenerate);
    root->addLayout(btnRow);

    const QString btnStyle = QString::fromUtf8(kDarkButtonQss);
    m_btnRefresh->setStyleSheet(btnStyle);
    m_btnGenerate->setStyleSheet(btnStyle);

    connect(m_btnRefresh, &QPushButton::clicked, this, &igQtAttributeSelectWidget::onRefreshClicked);
    connect(m_btnGenerate, &QPushButton::clicked, this, &igQtAttributeSelectWidget::onGenerateClicked);
}

void igQtAttributeSelectWidget::setStatus(const QString& msg) {
    m_statusLabel->setText(msg);
}

void igQtAttributeSelectWidget::setBusy(bool busy) {
    m_busy = busy;
    m_btnRefresh->setEnabled(!busy);
    m_btnGenerate->setEnabled(!busy && !m_checkBoxes.isEmpty());
}

bool igQtAttributeSelectWidget::IsGenerating() const {
    return m_busy;
}

bool igQtAttributeSelectWidget::HasPartId() const {
    return m_hasPartId;
}

void igQtAttributeSelectWidget::SetMaxSelectableCount(int count) {
    m_maxSelectableCount = std::max(1, count);
}

void igQtAttributeSelectWidget::onCheckBoxToggled(bool checked) {
    auto* cb = qobject_cast<QCheckBox*>(sender());
    if (!cb) return;
    if (checked) {
        // 避免重复入队（例如代码触发的 toggle）
        if (!m_selectionOrder.contains(cb)) {
            m_selectionOrder.append(cb);
        }
        // 超过上限：把最早勾选的取消掉（QCheckBox 非互斥，用代码复位避免信号递归）
        while (m_selectionOrder.size() > m_maxSelectableCount) {
            QCheckBox* oldest = m_selectionOrder.takeFirst();
            if (oldest == cb) {
                // 理论不会发生（上面已去重），防御性处理：勾选数超过上限时取最新一个
                m_selectionOrder.append(oldest);
                continue;
            }
            oldest->setChecked(false);
        }
    } else {
        m_selectionOrder.removeOne(cb);
    }
}

void igQtAttributeSelectWidget::RefreshAttributeList() {
    if (m_busy) return;

    m_checkBoxes.clear();
    m_selectionOrder.clear();
    auto* containerLayout = static_cast<QVBoxLayout*>(m_container->layout());
    QLayoutItem* item;
    while ((item = containerLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    containerLayout->addStretch();

    m_hasPartId = false;

    auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
    if (!scene) {
        setStatus(QStringLiteral("场景为空"));
        m_btnGenerate->setEnabled(false);
        return;
    }
    auto model = scene->GetCurrentModel();
    if (!model) {
        setStatus(QStringLiteral("无当前模型，请先加载模型"));
        m_btnGenerate->setEnabled(false);
        return;
    }
    m_dataObj = model->GetDataObject();
    if (!m_dataObj) {
        setStatus(QStringLiteral("模型数据对象为空"));
        m_btnGenerate->setEnabled(false);
        return;
    }

    m_hasPartId = m_dataObj->HasBlockMapping();

    auto attrSet = m_dataObj->GetAttributeSet();
    QStringList pointNames;
    QStringList cellNames;
    const QString partIdName = QStringLiteral("part_id");

    if (attrSet) {
        // part_id 不列入勾选列表：服务端始终分析它（相当于默认选中）
        auto pointList = attrSet->GetAllPointAttributes();
        for (int i = 0; i < pointList->GetNumberOfElements(); i++) {
            auto& attr = pointList->GetElement(i);
            if (attr.IsNone()) continue;
            QString name = QString::fromStdString(attr.pointer->GetName());
            if (name == partIdName) continue;
            pointNames << name;
        }
        auto cellList = attrSet->GetAllCellAttributes();
        for (int i = 0; i < cellList->GetNumberOfElements(); i++) {
            auto& attr = cellList->GetElement(i);
            if (attr.IsNone()) continue;
            QString name = QString::fromStdString(attr.pointer->GetName());
            if (name == partIdName) continue;
            cellNames << name;
        }
    }

    // 移除在 stretch 前残留的空白（takeAt 已清空，重新加）
    while (containerLayout->count() > 0) {
        auto* it = containerLayout->takeAt(0);
        delete it;
    }

    const QString checkboxStyle = QString::fromUtf8(kCheckBoxQss);
    auto addSection = [&](const QString& title, const QStringList& names) {
        if (names.isEmpty()) return;
        auto* titleLabel = new QLabel(title, m_container);
        titleLabel->setStyleSheet("color: rgba(255,255,255,120); font-size: 9pt; padding: 4px 2px 2px 6px;");
        containerLayout->addWidget(titleLabel);
        for (const QString& name : names) {
            auto* cb = new QCheckBox(name, m_container);
            cb->setStyleSheet(checkboxStyle);
            containerLayout->addWidget(cb);
            m_checkBoxes.append({name, cb});
            connect(cb, &QCheckBox::toggled, this, &igQtAttributeSelectWidget::onCheckBoxToggled);
        }
    };
    addSection(QStringLiteral("— 点属性 (Point) —"), pointNames);
    addSection(QStringLiteral("— 单元属性 (Cell) —"), cellNames);

    containerLayout->addStretch();

    if (m_checkBoxes.isEmpty()) {
        setStatus(QStringLiteral("当前模型没有任何属性"));
        m_btnGenerate->setEnabled(false);
        return;
    }

    QString status = QStringLiteral("共 %1 个属性，最多可勾选 %2 个")
                          .arg(m_checkBoxes.size())
                          .arg(m_maxSelectableCount);
    if (m_hasPartId) {
        status += QStringLiteral("（part_id 将自动包含）");
    }
    setStatus(status);
    m_btnGenerate->setEnabled(true);
}

void igQtAttributeSelectWidget::onRefreshClicked() {
    RefreshAttributeList();
}

std::vector<std::string> igQtAttributeSelectWidget::GetSelectedAttributes() const {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    return m_selectedAttributes;
}

void igQtAttributeSelectWidget::onGenerateClicked() {
    if (m_busy) return;

    // 收集勾选的属性
    QStringList selected;
    for (const auto& [name, cb] : m_checkBoxes) {
        if (cb && cb->isChecked()) selected << name;
    }

    if (!m_hasPartId) {
        // 没有 part_id 时仍可生成报告：服务器端按整体（单块）模型分析。
        // 弹窗提示后不中断流程，继续执行后续报告生成。
        igQtShowDarkFramelessMessage(this, QStringLiteral("报告生成"),
                                     QStringLiteral("当前模型没有 part_id 属性，将按整体模型生成报告（无零件级分析）。"));
    }

    if (selected.isEmpty()) {
        setStatus(QStringLiteral("请先勾选至少一个要分析的属性"));
        return;
    }

    if (selected.size() > m_maxSelectableCount) {
        setStatus(QStringLiteral("最多只能勾选 %1 个属性").arg(m_maxSelectableCount));
        return;
    }

    m_savePath = QFileDialog::getSaveFileName(this, QStringLiteral("选择报告保存路径"),
                                              QString(), QStringLiteral("Word 文档 (*.docx)"));
    if (m_savePath.isEmpty()) {
        return;
    }

    // 快照到结果缓存（worker 线程读它），随后在后台线程中运行报告生成
    {
        std::lock_guard<std::mutex> lock(m_resultMutex);
        m_selectedAttributes.clear();
        for (const QString& name : selected) {
            m_selectedAttributes.push_back(name.toStdString());
        }
    }

    if (m_workerThread && m_workerThread->isRunning()) {
        return;
    }

    setBusy(true);
    setStatus(QStringLiteral("正在生成报告（可能需要较长时间），请稍候..."));

    m_workerThread = new QThread(this);
    // 注意：这里不能把 this 作为 connect 的 context 传入 —— 那样 lambda 会被投递到主线程执行，
    // Execute() 整个流程（含等待服务器）都会阻塞 UI。不带 context 时在 QThread 的 run() 线程内直接执行。
    // lambda 按值捕获所有需要的输入；ui 用 QPointer 弱引用，窗口关闭后不再触碰 widget 成员。
    QPointer<igQtAttributeSelectWidget> ui = this;
    connect(m_workerThread, &QThread::started, [ui, input = m_dataObj, savePath = m_savePath, fields = GetSelectedAttributes()]() {
        bool ok = false;
        QString errorMessage;
        if (input) {
            auto generator = MeshReportGenerator::New(savePath.toStdString());
            generator->SetInput(input);
            generator->SetSimplificationRatio(0.2f);
            generator->SetTimeout(15 * 60 * 1000);
            generator->SetSpecifiedFields(fields);
            ok = generator->Execute();
            if (!ok) {
                errorMessage = QString::fromStdString(generator->GetErrorMessage());
            }
        } else {
            errorMessage = QStringLiteral("Input data object is null");
        }
        // 窗口可能已关闭：回调前检查弱引用，避免访问已销毁的 widget
        if (ui) {
            QMetaObject::invokeMethod(ui, "onAsyncFinished", Qt::QueuedConnection,
                                      Q_ARG(bool, ok), Q_ARG(QString, errorMessage));
        }
    });
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    m_workerThread->start();
}

void igQtAttributeSelectWidget::onAsyncFinished(bool success, const QString& message) {
    setBusy(false);
    if (success) {
        setStatus(QStringLiteral("报告生成完成"));
        igQtShowDarkFramelessMessage(this, QStringLiteral("报告生成"),
                                     QStringLiteral("报告生成成功，已保存到：\n%1").arg(m_savePath), true);
    } else {
        setStatus(QStringLiteral("报告生成失败"));
        igQtShowDarkFramelessMessage(this, QStringLiteral("报告生成失败"),
                                     message.isEmpty() ? QStringLiteral("未知错误") : message);
    }
    emit SIGNAL_ReportFinished(success, message);
}
