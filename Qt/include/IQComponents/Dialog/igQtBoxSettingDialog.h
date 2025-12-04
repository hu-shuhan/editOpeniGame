#ifndef IGQTBOXSETTINGDIALOG_H
#define IGQTBOXSETTINGDIALOG_H

#include <QDialog>
#include <QWidget>
#include <iGamePoints.h>
#include <iGameScene.h>

namespace Ui {
class igQtBoxSettingDialog;
}

class igQtBoxSettingDialog : public QWidget {
    Q_OBJECT

public:
    explicit igQtBoxSettingDialog(QWidget* renderWidget, QWidget* parent);
    ~igQtBoxSettingDialog();
    void ReloadBoxMsg();

private:
    void SetBoxChangeCallBackFunc();
    void BoxChangeCallBackFunc();
    void SetBoxUpdateWidgetFunc();
    void BoxUpdateWidgetFunc();

private:
    Ui::igQtBoxSettingDialog *ui;
    bool m_PreventBoxChangeFunc{};
    QWidget* m_RenderWidget{};
    void SetPreventBoxChangeFunc(bool prevent);

private slots:
    void PChanged();
    void RChanged();
    void LChanged();

private:
    void SetBoxNumsToLineEdit();
    void SetLineEditToBoxNums();
    void SetBoxCenter();
    void SetBoxRotation();
    void SetBoxLength();
    void UpdateBoxView();
};

#endif // IGQTBOXSETTINGDIALOG_H
