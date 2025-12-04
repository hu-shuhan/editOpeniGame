#ifndef IGQTBOXSETTINGDIALOG_H
#define IGQTBOXSETTINGDIALOG_H

#include <QDialog>
#include <iGamePoints.h>
#include <iGameScene.h>

namespace Ui {
class igQtBoxSettingDialog;
}

class igQtBoxSettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit igQtBoxSettingDialog(QWidget* parent);
    ~igQtBoxSettingDialog();
    void ReloadBoxMsg();

private:
    void SetBoxChangeCallBackFunc();
    void BoxChangeCallBackFunc();

private:
    Ui::igQtBoxSettingDialog *ui;
    bool m_PreventBoxChangeFunc{};
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
