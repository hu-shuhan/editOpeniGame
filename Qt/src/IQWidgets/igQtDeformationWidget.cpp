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

igQtDeformationWidget::~igQtDeformationWidget() {

}

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
    connect(ui->comboBox_Deformation_vector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &igQtDeformationWidget::CalculateCurrentDSF);
    connect(ui->radioButton_Nonuniform, &QRadioButton::toggled, this, [&](bool checked) {
        if(checked){
            ShowNonUniform();
            HideUniform();
        }
    });

    connect(ui->checkBox_enableOffset, &QCheckBox::toggled, this, [&](bool checked){
        auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
        if(dataObject == nullptr) return;
        dataObject->GetDeformationData()->m_enable_dsf = checked;
        if(checked){
            if(ui->radioButton_Uniform->isChecked() || ui->radioButton_autoCompute->isChecked()){
                dataObject->GetDeformationData()->SetScaleFactors(ui->lineEdit_Uniform_val->text().toFloat());
            } else{
                dataObject->GetDeformationData()->SetScaleFactorX(ui->lineEdit_Nonuniform_x->text().toFloat());
                dataObject->GetDeformationData()->SetScaleFactorY(ui->lineEdit_Nonuniform_y->text().toFloat());
                dataObject->GetDeformationData()->SetScaleFactorZ(ui->lineEdit_Nonuniform_z->text().toFloat());
            }

        }else {
            dataObject->GetDeformationData()->SetScaleFactors(0.f);
            iGame::StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
            deformFilter->SetInput(dataObject);
            if(!deformFilter->Execute()) std::cout << " error \n";
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
        iGame::StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
        auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
        deformFilter->SetInput(dataObject);
        if(!deformFilter->Execute()) std::cout << " error \n";
    });

}

void igQtDeformationWidget::updateInfo() {
    using namespace iGame;
    /*Update combobox info.*/
    auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
    m_Scalar_num = dataObject->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();

    ui->checkBox_enableOffset->setChecked(false);
    ui->comboBox_Deformation_vector->clear();
    for (int i = 0; i < m_Scalar_num; i++) {
        auto& data = dataObject->GetAttributeSet()->GetAttribute(i);
        ui->comboBox_Deformation_vector->addItem(QString(data.pointer->GetName().c_str()));
    }
    /*Update lineEdit info.*/
    ui->lineEdit_Uniform_val->setText("0.0");
    ui->lineEdit_Nonuniform_x->setText("0.0");
    ui->lineEdit_Nonuniform_y->setText("0.0");
    ui->lineEdit_Nonuniform_z->setText("0.0");
}


void igQtDeformationWidget::CalculateCurrentDSF() {
    auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
    if(dataObject != nullptr && ui->comboBox_Deformation_vector->count() > 1){
        dataObject->GetDeformationData()->m_deformation_attribute_name = ui->comboBox_Deformation_vector->currentText().toStdString();
        iGame::StressDeformationFilter::Pointer p = iGame::StressDeformationFilter::New();
        p->SetInput(dataObject);
        p->CalculateIdealDSF();
    }
    QString val = QString::number(dataObject->GetDeformationData()->m_deformation_scale_factor_x, 'f', 5);
    ui->lineEdit_Uniform_val->setText(val);
    ui->lineEdit_Nonuniform_x->setText(val);
    ui->lineEdit_Nonuniform_y->setText(val);
    ui->lineEdit_Nonuniform_z->setText(val);

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





