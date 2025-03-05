/**
 * @class   igQtChangeBackGroundDialog
 * @brief   igQtChangeBackGroundDialog's brief
 */

#pragma once

#include <IQCore/igQtExportModule.h>
#include <QDialog>


class QLineEdit ;
class QListWidget ;
class IG_QT_MODULE_EXPORT igQtChangeBackGroundDialog : public QDialog{
Q_OBJECT
public:
    igQtChangeBackGroundDialog(QWidget *parent = nullptr);

    std::vector<int > getInput();

protected:
    int m_R, m_G, m_B;
    QListWidget* m_ListWidget;
    QLineEdit* m_Red_LineEdit, *m_Green_LineEdit, *m_Blue_LineEdit;
};