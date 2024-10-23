/**
 * @class   igQtSplineOptionDialog
 * @brief   igQtSplineOptionDialog's brief
 */

#pragma once

#include <IQCore/igQtExportModule.h>
#include <QDialog>
#include <IQCore/igQtFileType.h>

class QListWidget;
class QLabel;
class IG_QT_MODULE_EXPORT igQtSplineOptionDialog : public QDialog{
Q_OBJECT
public:
    igQtSplineOptionDialog(QWidget* par = nullptr);
public:

    void setFileName(const QString& fileName);

    SplineType getDialogOutput();
protected:
    QListWidget* m_ListWidget;
    QLabel* m_IntroduceLabel;
};
