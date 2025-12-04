#include "ui_igQtBoxSettingDialog.h"
#include <IQComponents/Dialog/igQtBoxSettingDialog.h>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <iGameDynamicBox.h>
#include <iGameBasicStyle.h>
#include <iGameBoxStyle.h>
#include <iGameInteractor.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <QWidget>
#include <functional>
#include <iGameSelectionParameter.h>

static iGame::Interactor::Pointer GetInteractor(igQtModelDrawWidget* rendererWidget) {
    if (rendererWidget == nullptr) return nullptr;
    auto scene = rendererWidget->GetScene();
    if (scene == nullptr) return nullptr;
    auto interactor = scene->GetInteractor();
    if (interactor == nullptr) return nullptr;
    return interactor;
}

static iGame::BoxStyle::Pointer GetBoxStyle(igQtModelDrawWidget* rendererWidget) {
    if (rendererWidget == nullptr) return nullptr;
    auto scene = rendererWidget->GetScene();
    if (scene == nullptr) return nullptr;
    auto interactor = scene->GetInteractor();
    if (interactor == nullptr) return nullptr;
    if (!interactor->HaveSpecialInteractor("SelectBox")) return nullptr;
    auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
    if (basicStyle == nullptr) return nullptr;
    auto boxStyle = iGame::DynamicCast<iGame::BoxStyle>(basicStyle);
    if (boxStyle == nullptr) return nullptr;
    return boxStyle;
}

static iGame::DynamicBox::Pointer GetDynamicBox(igQtModelDrawWidget* rendererWidget) {
    if (rendererWidget == nullptr) return nullptr;
    auto scene = rendererWidget->GetScene();
    if (scene == nullptr) return nullptr;
    auto interactor = scene->GetInteractor();
    if (interactor == nullptr) return nullptr;
    if (!interactor->HaveSpecialInteractor("SelectBox")) return nullptr;
    auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
    if (basicStyle == nullptr) return nullptr;
    auto boxStyle = iGame::DynamicCast<iGame::BoxStyle>(basicStyle);
    if (boxStyle == nullptr) return nullptr;
    auto dynamicBox = boxStyle->GetBox();
    if (dynamicBox == nullptr) return nullptr;
    return dynamicBox;
}

igQtBoxSettingDialog::igQtBoxSettingDialog(QWidget* renderWidget, QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtBoxSettingDialog) {
    ui->setupUi(this);
    m_RenderWidget = renderWidget;
    //############ Set ViewAble ############
    ui->comfirm->hide();
    ui->cancel->hide();

    //############ Set Num QRegularExpression ############
    QRegularExpression rx("\\d{1,5}(\\.\\d{1,9})?");
    QRegularExpression rxn("-?\\d{1,5}(\\.\\d{1,9})?");
    ui->px->setValidator(new QRegularExpressionValidator(rxn, this));
    ui->py->setValidator(new QRegularExpressionValidator(rxn, this));
    ui->pz->setValidator(new QRegularExpressionValidator(rxn, this));
    ui->rx->setValidator(new QRegularExpressionValidator(rxn, this));
    ui->ry->setValidator(new QRegularExpressionValidator(rxn, this));
    ui->rz->setValidator(new QRegularExpressionValidator(rxn, this));
    ui->lx->setValidator(new QRegularExpressionValidator(rx, this));
    ui->ly->setValidator(new QRegularExpressionValidator(rx, this));
    ui->lz->setValidator(new QRegularExpressionValidator(rx, this));

    //############ Set Num ############
    SetBoxNumsToLineEdit();

    //############ Set BoxChangeCallBackFunc ############
    SetBoxChangeCallBackFunc();
    SetBoxUpdateWidgetFunc();

    //############ Set connect ############
    connect(ui->px, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::PChanged);
    connect(ui->py, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::PChanged);
    connect(ui->pz, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::PChanged);

    connect(ui->rx, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::RChanged);
    connect(ui->ry, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::RChanged);
    connect(ui->rz, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::RChanged);

    connect(ui->lx, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::LChanged);
    connect(ui->ly, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::LChanged);
    connect(ui->lz, &QLineEdit::textChanged, this, &igQtBoxSettingDialog::LChanged);
}

igQtBoxSettingDialog::~igQtBoxSettingDialog() {
    auto boxStyle = GetBoxStyle(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (boxStyle != nullptr) {
        boxStyle->RemovePointMoveCallBack("igQtBoxSettingDialog");
        boxStyle->RemoveUpdateWidgetFunc();
    }
    delete ui;
}

void igQtBoxSettingDialog::ReloadBoxMsg() {
    SetPreventBoxChangeFunc(true);
    SetBoxNumsToLineEdit();
    SetBoxChangeCallBackFunc();
    SetBoxUpdateWidgetFunc();
    SetPreventBoxChangeFunc(false);
}

void igQtBoxSettingDialog::SetBoxChangeCallBackFunc() {
    auto boxStyle = GetBoxStyle(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (boxStyle == nullptr) return;
    boxStyle->_SetPointMoveCallBack("igQtBoxSettingDialog",
                                    std::bind(&igQtBoxSettingDialog::BoxChangeCallBackFunc, this));
}

void igQtBoxSettingDialog::BoxChangeCallBackFunc() {
    SetPreventBoxChangeFunc(true);
    SetBoxNumsToLineEdit();
    SetPreventBoxChangeFunc(false);
}

void igQtBoxSettingDialog::SetBoxUpdateWidgetFunc() {
    auto boxStyle = GetBoxStyle(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (boxStyle == nullptr) return;
    boxStyle->SetUpdateWidgetFunc(std::bind(&igQtBoxSettingDialog::BoxUpdateWidgetFunc, this));
}

void igQtBoxSettingDialog::BoxUpdateWidgetFunc() {
    if (m_RenderWidget) m_RenderWidget->update();
    this->hide();
}

void igQtBoxSettingDialog::SetPreventBoxChangeFunc(bool prevent) { m_PreventBoxChangeFunc = prevent; }

void igQtBoxSettingDialog::PChanged() {
    if (m_PreventBoxChangeFunc) return;
    SetBoxCenter();
    UpdateBoxView();
}

void igQtBoxSettingDialog::RChanged() {
    if (m_PreventBoxChangeFunc) return;
    SetBoxRotation();
    UpdateBoxView();
}

void igQtBoxSettingDialog::LChanged() {
    if (m_PreventBoxChangeFunc) return;
    SetBoxLength();
    UpdateBoxView();
}

void igQtBoxSettingDialog::SetBoxNumsToLineEdit() {
    auto dynamicBox = GetDynamicBox(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (dynamicBox == nullptr) return;
    auto& center = dynamicBox->GetMidPoint();
    ui->px->setText(QString::number(center[0]));
    ui->py->setText(QString::number(center[1]));
    ui->pz->setText(QString::number(center[2]));
    auto& rotation = dynamicBox->GetRotation();
    ui->rx->setText(QString::number(rotation[0]));
    ui->ry->setText(QString::number(rotation[1]));
    ui->rz->setText(QString::number(rotation[2]));
    auto& length = dynamicBox->GetLength();
    ui->lx->setText(QString::number(length[0]));
    ui->ly->setText(QString::number(length[1]));
    ui->lz->setText(QString::number(length[2]));
}

void igQtBoxSettingDialog::SetLineEditToBoxNums() {
    auto dynamicBox = GetDynamicBox(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (dynamicBox == nullptr) return;
    dynamicBox->MovePosition(ui->px->text().toDouble(), ui->py->text().toDouble(), ui->pz->text().toDouble());
    dynamicBox->SetRotation(ui->rx->text().toDouble(), ui->ry->text().toDouble(), ui->rz->text().toDouble());
    dynamicBox->SetLength(ui->lx->text().toDouble(), ui->ly->text().toDouble(), ui->lz->text().toDouble());
}

void igQtBoxSettingDialog::SetBoxCenter() {
    auto dynamicBox = GetDynamicBox(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (dynamicBox == nullptr) return;
    dynamicBox->MovePosition(ui->px->text().toDouble(), ui->py->text().toDouble(), ui->pz->text().toDouble());
}

void igQtBoxSettingDialog::SetBoxRotation() {
    auto dynamicBox = GetDynamicBox(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (dynamicBox == nullptr) return;
    dynamicBox->SetRotation(ui->rx->text().toDouble(), ui->ry->text().toDouble(), ui->rz->text().toDouble());
}

void igQtBoxSettingDialog::SetBoxLength() {
    auto dynamicBox = GetDynamicBox(dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget));
    if (dynamicBox == nullptr) return;
    dynamicBox->SetLength(ui->lx->text().toDouble(), ui->ly->text().toDouble(), ui->lz->text().toDouble());
}

void igQtBoxSettingDialog::UpdateBoxView() {
    auto rendererWidget = dynamic_cast<igQtModelDrawWidget*>(m_RenderWidget);
    auto boxStyle = GetBoxStyle(rendererWidget);
    if (boxStyle == nullptr) return;
    boxStyle->ClearDraw();
    boxStyle->ToDraw();
    rendererWidget->update();
}