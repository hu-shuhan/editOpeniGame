//
// Created by m_ky on 2024/10/15.
//

/**
 * @class   igQtDeformationWidget
 * @brief   igQtDeformationWidget's brief
 */

#include "IQWidgets/igQtDeformationWidget.h"

igQtDeformationWidget::igQtDeformationWidget(QWidget *par)
    : QWidget(par), ui(new Ui::Deformation){

    ui->setupUi(this);

    HideNonUniform();
    HideUniform();
    connect(ui->radioButton_autoCompute, &QRadioButton::toggled, this, [&](bool checked){
        if(checked){
            HideNonUniform();
            HideUniform();
        } else {
            if(ui->radioButton_Nonuniform->isChecked()) {
                HideUniform();
                ShowNonUniform();
            }
            else {
                HideNonUniform();
                ShowUniform();
            }
        }
    });
    connect(ui->radioButton_Uniform, &QRadioButton::toggled, this, [&](bool checked){
       if(checked){
           ShowUniform();
           HideNonUniform();
       }else if(ui->radioButton_Nonuniform->isChecked()){
           ShowNonUniform();
           HideUniform();
       }
    });
}

igQtDeformationWidget::~igQtDeformationWidget() {

}

void igQtDeformationWidget::HideUniform() {
    ui->lineEdit_Uniform_val->hide();
    ui->label_Uniform->hide();
}

void igQtDeformationWidget::HideNonUniform() {
    ui->lineEdit_Nonuniform_x->hide();
    ui->lineEdit_Nonuniform_y->hide();
    ui->lineEdit_Nonuniform_z->hide();
    ui->label_x->hide();
    ui->label_y->hide();
    ui->label_z->hide();
}

void igQtDeformationWidget::ShowUniform() {
    ui->lineEdit_Uniform_val->show();
    ui->label_Uniform->show();
}

void igQtDeformationWidget::ShowNonUniform() {
    ui->lineEdit_Nonuniform_x->show();
    ui->lineEdit_Nonuniform_y->show();
    ui->lineEdit_Nonuniform_z->show();
    ui->label_x->show();
    ui->label_y->show();
    ui->label_z->show();
}
