#pragma once

#include <IQCore/igQtExportModule.h>
#include <iGameDataObject.h>

#include <QCheckBox>
#include <QLabel>
#include <QPointer>
#include <QPair>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include <mutex>
#include <string>
#include <vector>

/**
 * igQtAttributeSelectWidget
 * 报告生成面板：列出当前模型的 point/cell 属性，勾选要分析的属性后，
 * 选择报告保存路径并异步（后台线程）生成报告。
 *
 * 与零件聚焦弹窗保持一致的深色主题，由 igQtMainWindow 包裹在
 * igQtChromeFramelessDialog 中显示。
 */
class IG_QT_MODULE_EXPORT igQtAttributeSelectWidget : public QWidget {
    Q_OBJECT

public:
    explicit igQtAttributeSelectWidget(QWidget* parent = nullptr);
    ~igQtAttributeSelectWidget() override;

    // 刷新属性列表（读取当前模型数据对象的 point/cell 属性）
    void RefreshAttributeList();

    // 返回当前勾选的属性名列表（内部线程安全地访问）
    std::vector<std::string> GetSelectedAttributes() const;

    bool HasPartId() const;

    // 限制最多可勾选的数量；1 表示只能勾选一个（默认在 igQtMainWindow 中设置为 1）
    void SetMaxSelectableCount(int count);

    bool IsGenerating() const;

signals:
    // 报告生成完成（成功或失败都发，message 描述结果）
    void SIGNAL_ReportFinished(bool success, QString message);

private slots:
    void onRefreshClicked();
    void onGenerateClicked();
    void onConfigClicked();
    void onAsyncFinished(bool success, const QString& message);
    void onCheckBoxToggled(bool checked);

private:
    void setupUI();
    void setStatus(const QString& msg);
    void setBusy(bool busy);

    iGame::DataObject::Pointer m_dataObj{nullptr};

    QLabel*       m_statusLabel{nullptr};
    QScrollArea*  m_scrollArea{nullptr};
    QWidget*      m_container{nullptr};
    QVector<QPair<QString, QCheckBox*>> m_checkBoxes;
    // 勾选顺序队列（最早勾选的在最前）：勾选数超过 m_maxSelectableCount 时，最早勾选的被自动取消
    QVector<QCheckBox*> m_selectionOrder;

    // 后台线程与结果缓存（worker 线程只写这两个成员，UI 线程在读）
    mutable std::mutex m_resultMutex;
    std::vector<std::string> m_selectedAttributes;
    QString m_savePath;
    bool m_hasPartId{false};
    QPointer<QThread> m_workerThread;
    bool m_busy{false};

    // 报告生成服务器的目标地址（可在面板中配置，持久化到 QSettings）
    QString m_serverHost;
    int m_serverPort{8766};

    QPushButton* m_configBtn{nullptr};
    QPushButton* m_btnRefresh{nullptr};
    QPushButton* m_btnGenerate{nullptr};
    int m_maxSelectableCount{1};
};
