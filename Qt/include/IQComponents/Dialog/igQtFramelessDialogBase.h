#pragma once

#include <IQCore/igQtExportModule.h>
#include <QDialog>

class QLabel;
class QPushButton;
class QWidget;
class QVBoxLayout;
class QMouseEvent;
class QResizeEvent;

class IG_QT_MODULE_EXPORT igQtFramelessDialogBase : public QDialog {
    Q_OBJECT
public:
    explicit igQtFramelessDialogBase(QWidget* parent = nullptr);
    ~igQtFramelessDialogBase() override = default;

    void setDialogTitle(const QString& title);
    void setContentWidget(QWidget* widget);
    QWidget* contentWidget() const { return m_contentHost; }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void updateRoundedMask();

private:
    QWidget* m_titleBar{nullptr};
    QLabel* m_titleLabel{nullptr};
    QPushButton* m_closeButton{nullptr};
    QWidget* m_contentHost{nullptr};
    QVBoxLayout* m_contentLayout{nullptr};

    bool m_dragging{false};
    QPoint m_dragOffset;
    int m_cornerRadius{10};
};

