/**
 * @class   igQtBasicListOptionDialog
 * @brief   igQtBasicListOptionDialog's brief
 */

#pragma once

#include <IQCore/igQtExportModule.h>
#include <QDialog>
#include <IQCore/igQtFileType.h>

class QListWidget;
class QLabel;
class IG_QT_MODULE_EXPORT igQtBasicListOptionDialog : public  QDialog{
Q_OBJECT
public:
    igQtBasicListOptionDialog(QWidget* par = nullptr);
    void setDialogTitle(const QString& _title);
    void setLabelName(const QString& _labelInfo, const QString& _format = "");
    void setLabelName(const std::string& _labelInfo, const std::string& _format = "");
    virtual int getDialogOutput();
    void setInfoList(const QStringList& _list);
    void setInfoList(const std::vector<QString>& _list);
    void setInfoList(const std::vector<char*>& _list);
    void setInfoList(const std::vector<std::string>& _list);
protected:
    QListWidget* m_ListWidget;
    QLabel* m_IntroduceLabel;

};
