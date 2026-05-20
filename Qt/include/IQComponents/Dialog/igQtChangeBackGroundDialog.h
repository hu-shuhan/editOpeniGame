/**
 * @class   igQtChangeBackGroundDialog
 * @brief   igQtChangeBackGroundDialog's brief
 */

#pragma once

#include <IQCore/igQtExportModule.h>
#include <QDialog>
#include <QPoint>


namespace Ui {
class igQtChangeBackGroundDialog;
}

class QMouseEvent;
class QLineEdit ;
class QListWidget ;
class IG_QT_MODULE_EXPORT igQtChangeBackGroundDialog : public QDialog{
Q_OBJECT
public:
    igQtChangeBackGroundDialog(QWidget *parent = nullptr);
    ~igQtChangeBackGroundDialog() override;

    std::vector<int > getInput();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

protected:
    Ui::igQtChangeBackGroundDialog* ui;
    int m_R, m_G, m_B;
    QListWidget* m_ListWidget;
    QLineEdit* m_Red_LineEdit, *m_Green_LineEdit, *m_Blue_LineEdit;
    bool m_dragging{false};
    QPoint m_dragOffset;
    int m_cornerRadius{8};

private:
    void updateRoundedMask();
};