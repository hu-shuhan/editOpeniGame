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

#include <QRegularExpression>
#include <QRegularExpressionValidator>
igQtDeformationWidget::~igQtDeformationWidget() {

}

igQtDeformationWidget::igQtDeformationWidget(QWidget *par)
    : QWidget(par), ui(new Ui::Deformation){

    ui->setupUi(this);

    HideNonUniform();
    ui->lineEdit_Uniform_val->setEnabled(false);
    connect(ui->radioButton_autoCompute, &QRadioButton::toggled, this, [&](bool checked){
        if(checked){
            CalculateCurrentDSF();
            ShowUniform();
            HideNonUniform();
            ui->lineEdit_Uniform_val->setEnabled(false);
        }else {
            if(iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel() == nullptr){
                IGAME_WARN("Deformation Widget : Current Model is Empty");
                return ;
            }
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            dataObject->GetDeformationData()->SetAutoCompute(false);
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
        if(iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel() == nullptr){
            IGAME_WARN("Deformation Widget : Current Model is Empty");
            return ;
        }
        auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
        dataObject->GetDeformationData()->SetEnableDeformation(checked);
        if(checked){
            if(ui->radioButton_Uniform->isChecked() || ui->radioButton_autoCompute->isChecked()){
                dataObject->GetDeformationData()->SetScaleFactors(ui->lineEdit_Uniform_val->text().toFloat());
            } else{
                dataObject->GetDeformationData()->SetScaleFactorX(ui->lineEdit_Nonuniform_x->text().toFloat());
                dataObject->GetDeformationData()->SetScaleFactorY(ui->lineEdit_Nonuniform_y->text().toFloat());
                dataObject->GetDeformationData()->SetScaleFactorZ(ui->lineEdit_Nonuniform_z->text().toFloat());
            }

        } else {
            dataObject->GetDeformationData()->SetScaleFactors(0.f);
            iGame::StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
            deformFilter->SetInput(dataObject);
            if(!deformFilter->Execute()) std::cout << " error \n";
            iGame::SceneManager::Instance()->GetCurrentScene()->Update();
        }

    });

    /* if lineEdit update. update the corresponding dsf. */
    connect(ui->lineEdit_Uniform_val, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()){
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            if(dataObject == nullptr){
                IGAME_WARN("Deformation Widget : Current Model is Empty");
                return ;
            }
            dataObject->GetDeformationData()->SetScaleFactors(ui->lineEdit_Uniform_val->text().toFloat());
        }
    });
    connect(ui->lineEdit_Nonuniform_x, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()){
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            if(dataObject == nullptr) return;
            dataObject->GetDeformationData()->SetScaleFactorX(ui->lineEdit_Nonuniform_x->text().toFloat());
        }
    });
    connect(ui->lineEdit_Nonuniform_y, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()) {
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            if(dataObject == nullptr) return;
            dataObject->GetDeformationData()->SetScaleFactorY(ui->lineEdit_Nonuniform_y->text().toFloat());
        }
    });
    connect(ui->lineEdit_Nonuniform_z, &QLineEdit::editingFinished, this, [&]{
        if(ui->checkBox_enableOffset->isChecked()) {
            auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
            if(dataObject == nullptr) return;
            dataObject->GetDeformationData()->SetScaleFactorZ(ui->lineEdit_Nonuniform_z->text().toFloat());
        }
    });

    connect(ui->pushButton_exec, &QPushButton::clicked, this, [&]{
        iGame::StressDeformationFilter::Pointer deformFilter = iGame::StressDeformationFilter::New();
        if(iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel() == nullptr) return;
        auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
        deformFilter->SetInput(dataObject);
        if(!deformFilter->Execute()) std::cout << " error \n";
        iGame::SceneManager::Instance()->GetCurrentScene()->Update();
    });
    QRegularExpression  rx("-?\\d*\\.?\\d+");
    ui->lineEdit_Uniform_val->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_Nonuniform_x->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_Nonuniform_y->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lineEdit_Nonuniform_z->setValidator(new QRegularExpressionValidator(rx, this));
}

void igQtDeformationWidget::updateInfo() {
    using namespace iGame;
    /*Update combobox info.*/
    auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
    if(dataObject) m_Scalar_num = dataObject->GetAttributeSet()->GetAllAttributes()->GetNumberOfElements();

    /*Update lineEdit info.*/
    ui->lineEdit_Uniform_val->setText("0.0");
    ui->lineEdit_Nonuniform_x->setText("0.0");
    ui->lineEdit_Nonuniform_y->setText("0.0");
    ui->lineEdit_Nonuniform_z->setText("0.0");
    ui->checkBox_enableOffset->setChecked(false);


    ui->comboBox_Deformation_vector->blockSignals(true);
    ui->comboBox_Deformation_vector->clear();
    for (int i = 0; i < m_Scalar_num; i++) {
        auto& data = dataObject->GetAttributeSet()->GetAttribute(i);
        if(data.pointer->GetDimension() < 2) continue;
        ui->comboBox_Deformation_vector->addItem(QString(data.pointer->GetName().c_str()));
    }
    ui->comboBox_Deformation_vector->blockSignals(false);
    ui->radioButton_autoCompute->setChecked(true);
}


void igQtDeformationWidget::CalculateCurrentDSF() {
    auto dataObject = iGame::SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
    if(dataObject == nullptr) return;

    if(dataObject != nullptr){
        dataObject->GetDeformationData()->SetAttributeName(ui->comboBox_Deformation_vector->currentText().toStdString());
        iGame::StressDeformationFilter::Pointer p = iGame::StressDeformationFilter::New();
        p->SetInput(dataObject);
        p->CalculateIdealDSF();
    }
    QString val = QString::number(dataObject->GetDeformationData()->GetScaleFactorX(), 'f', 5);
    std::string a = val.toStdString();
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





