//
// Created by m_ky on 2024/10/15.
//

/**
 * @class   igQtDeformationWidget
 * @brief   igQtDeformationWidget's brief
 */

#include "IQWidgets/igQtDeformationWidget.h"
#include <iGameSceneManager.h>
#include <iGameStreamingData.h>
#include <Deformation/iGameStressDeformationFilter.h>

igQtDeformationWidget::igQtDeformationWidget(QWidget *par)
    : QWidget(par), ui(new Ui::Deformation){

    ui->setupUi(this);

    HideNonUniform();
    ui->lineEdit_Uniform_val->setEnabled(false);
    connect(ui->radioButton_autoCompute, &QRadioButton::toggled, this, [&](bool checked){
        if(checked){
            ShowUniform();
            HideNonUniform();
            ui->lineEdit_Uniform_val->setEnabled(false);
        }
//        else {
//            if(ui->radioButton_Nonuniform->isChecked()) {
//                HideUniform();
//                ShowNonUniform();
//            }
//            else {
//                HideNonUniform();
//                ShowUniform();
//                ui->lineEdit_Uniform_val->setEnabled(true);
//            }
//        }
    });
    connect(ui->radioButton_Uniform, &QRadioButton::toggled, this, [&](bool checked){
       if(checked){
           ShowUniform();
           ui->lineEdit_Uniform_val->setEnabled(true);
           HideNonUniform();
       }
//       else if(ui->radioButton_Nonuniform->isChecked()){
//           ShowNonUniform();
//           HideUniform();
//       }
    });

    connect(ui->radioButton_Nonuniform, &QRadioButton::toggled, this, [&](bool checked) {
        if(checked){
            ShowNonUniform();
            HideUniform();
        }
    });

    connect(ui->checkBox_enableOffset, &QCheckBox::toggled, this, [&](bool checked){
        auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
        dataObject->GetDeformationData()->m_enable_dsf = checked;
        if(checked){
//            this->CalculateCurrentDSF(checked);
            int attri_idx = ui->comboBox_Deformation_vector->currentIndex();
            dataObject->GetDeformationData()->m_deformation_attribute_name = ui->comboBox_Deformation_vector->currentText().toStdString();
            if(ui->radioButton_Uniform->isChecked() || ui->radioButton_autoCompute->isChecked()){
                dataObject->GetDeformationData()->SetScaleFactors(ui->lineEdit_Uniform_val->text().toFloat());
            } else{
                dataObject->GetDeformationData()->SetScaleFactorX(ui->lineEdit_Nonuniform_x->text().toFloat());
                dataObject->GetDeformationData()->SetScaleFactorY(ui->lineEdit_Nonuniform_y->text().toFloat());
                dataObject->GetDeformationData()->SetScaleFactorZ(ui->lineEdit_Nonuniform_z->text().toFloat());
            }

        }

    });

    /* if lineEdit update. update the corresponding dsf. */
    connect(ui->lineEdit_Uniform_val, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()){
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            dataObject->GetDeformationData()->SetScaleFactors(ui->lineEdit_Uniform_val->text().toFloat());
        }
    });
    connect(ui->lineEdit_Nonuniform_x, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()){
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            dataObject->GetDeformationData()->SetScaleFactorX(ui->lineEdit_Nonuniform_x->text().toFloat());
        }
    });
    connect(ui->lineEdit_Nonuniform_y, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()) {
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            dataObject->GetDeformationData()->SetScaleFactorY(ui->lineEdit_Nonuniform_y->text().toFloat());
        }
    });
    connect(ui->lineEdit_Nonuniform_z, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()) {
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            dataObject->GetDeformationData()->SetScaleFactorZ(ui->lineEdit_Nonuniform_z->text().toFloat());
        }
    });

    connect(ui->pushButton_exec, &QPushButton::clicked, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()) {
            iGame::StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            deformFilter->SetInput(dataObject);
            if(!deformFilter->Execute()) std::cout << " error \n";
        }
    });
}

igQtDeformationWidget::~igQtDeformationWidget() {

}

void igQtDeformationWidget::updateInfo() {
    using namespace iGame;
    /*Update combobox info.*/
    auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
    dataObject->GetAttributeSet();
    m_Scalar_num = dataObject->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();

    ui->comboBox_Deformation_vector->clear();
    for (int i = 0; i < m_Scalar_num; i++) {
        auto& data = dataObject->GetAttributeSet()->GetAttribute(i);
        ui->comboBox_Deformation_vector->addItem(QString(data.pointer->GetName().c_str()));
    }
    /*Update lineEdit info.*/
    ui->lineEdit_Uniform_val->setText("1.0");
    ui->lineEdit_Nonuniform_x->setText("1.0");
    ui->lineEdit_Nonuniform_y->setText("1.0");
    ui->lineEdit_Nonuniform_z->setText("1.0");
}


void igQtDeformationWidget::CalculateCurrentDSF(bool enabled) {
    auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
    int attri_idx = ui->comboBox_Deformation_vector->currentIndex();
    dataObject->GetDeformationData()->m_enable_dsf = enabled;
    dataObject->GetDeformationData()->m_deformation_attribute_name = enabled;
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





