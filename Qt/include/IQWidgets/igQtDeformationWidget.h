/**
 * @class   igQtDeformationWidget
 * @brief   igQtDeformationWidget's brief
 */

#pragma once

#include <ui_Deformation.h>
class igQtDeformationWidget : public QWidget{
    Q_OBJECT

public:
    igQtDeformationWidget(QWidget* par = nullptr);
    ~igQtDeformationWidget();
private:

    Ui::Deformation* ui;

    uint32_t m_Scalar_num{0};
public:
    void updateInfo();

private slots:
    void CalculateCurrentDSF(bool enabled);

private:
    void HideUniform();
    void HideNonUniform();
    void ShowNonUniform();
    void ShowUniform();

};
