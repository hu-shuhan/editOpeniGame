#pragma once

#include <IQCore/igQtExportModule.h>
#include <QDialog>

class QLabel;
class QPushButton;
class QWidget;
class QVBoxLayout;
class QMouseEvent;
class QPaintEvent;
class QEvent;

/**
 * 无边框对话框：深色标题栏、最小化 / 最大化 / 关闭，边缘与原生一致的缩放（Windows 下 WM_NCHITTEST）。
 * 用于需要独立窗体装饰且保留系统缩放行为的工具窗口（如变量相关性分析）。
 */
class IG_QT_MODULE_EXPORT igQtChromeFramelessDialog : public QDialog {
    Q_OBJECT

public:
    explicit igQtChromeFramelessDialog(QWidget* parent = nullptr);
    ~igQtChromeFramelessDialog() override = default;

    void setDialogTitle(const QString& title);
    void setContentWidget(QWidget* widget);
    QWidget* contentHost() const { return m_contentHost; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#endif

private:
    void updateFrameMarginsForWindowState();
    void updateMaximizeButtonIcon();
    void toggleMaximizeRestore();
    bool isOnCaptionButton(const QPoint& dialogPos) const;
#if !defined(Q_OS_WIN)
    Qt::Edges hitTestEdges(const QPoint& pos) const;
    static Qt::CursorShape cursorForEdges(Qt::Edges edges);
#endif

    QWidget* m_titleBar{nullptr};
    QLabel* m_titleLabel{nullptr};
    QPushButton* m_minimizeButton{nullptr};
    QPushButton* m_maximizeButton{nullptr};
    QPushButton* m_closeButton{nullptr};
    QWidget* m_contentHost{nullptr};
    QVBoxLayout* m_contentLayout{nullptr};
    QVBoxLayout* m_rootLayout{nullptr};

    QRect m_normalGeometry;
    bool m_dragging{false};
    QPoint m_dragOffset;
};
