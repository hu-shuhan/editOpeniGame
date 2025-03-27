//
// Created by m_ky on 2025/3/12.
//

/**
 * @class   igQtBasicListOptionDialog
 * @brief   igQtBasicListOptionDialog's brief
 */

#include "IQComponents/Dialog/igQtBasicListOptionDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>


igQtBasicListOptionDialog::igQtBasicListOptionDialog(QWidget *par): QDialog(par) {
    this->setWindowTitle("Basic List Widget Option Dialog");
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_IntroduceLabel = new QLabel(this);
    m_ListWidget = new QListWidget(this);

    QHBoxLayout* hlay_buttons = new QHBoxLayout(this);
    auto* okButton = new QPushButton("OK", this);
    auto* cancelButton = new QPushButton("Cancel", this);
    hlay_buttons->addWidget(okButton);
    hlay_buttons->addWidget(cancelButton);
    layout->addLayout(hlay_buttons);
    connect(okButton, &QPushButton::clicked, this,
            &igQtBasicListOptionDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this,
            &igQtBasicListOptionDialog::reject);


    layout->addWidget(m_IntroduceLabel);
    layout->addWidget(m_ListWidget);
    layout->addLayout(hlay_buttons);
}

void igQtBasicListOptionDialog::setDialogTitle(const QString &_title) {
    this->setWindowTitle(_title);
}
void igQtBasicListOptionDialog::setLabelName(const std::string &_labelInfo, const std::string &_format) {
    if(_format != ""){
        m_IntroduceLabel->setText(QString::asprintf(_format.c_str(), _labelInfo.c_str()));
    } else {
        m_IntroduceLabel->setText(QString(_labelInfo.c_str()));
    }
}
void igQtBasicListOptionDialog::setLabelName(const QString &_labelInfo, const QString &_format) {
    if(_format != ""){
        m_IntroduceLabel->setText(QString::asprintf(_format.toStdString().c_str(), _labelInfo.toStdString().c_str()));
    } else {
        m_IntroduceLabel->setText(_labelInfo);
    }
}


int igQtBasicListOptionDialog::getDialogOutput() {
    return m_ListWidget->currentRow();
}

void igQtBasicListOptionDialog::setInfoList(const QStringList &_list) {
    m_ListWidget->clear();
    for(auto& itName: _list){
        m_ListWidget->addItem(itName);
    }
}

void igQtBasicListOptionDialog::setInfoList(const std::vector<std::string> &_list) {
    m_ListWidget->clear();
    for(auto& itName: _list){
        m_ListWidget->addItem(QString(itName.c_str()));
    }
}

void igQtBasicListOptionDialog::setInfoList(const std::vector<QString> &_list) {
    m_ListWidget->clear();
    for(auto& itName: _list){
        m_ListWidget->addItem(itName);
    }
}

void igQtBasicListOptionDialog::setInfoList(const std::vector<char *> &_list) {
    m_ListWidget->clear();
    for(auto& itName: _list){
        m_ListWidget->addItem(itName);
    }
}