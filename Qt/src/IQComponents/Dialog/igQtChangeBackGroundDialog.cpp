//
// Created by m_ky on 2025/2/15.
//

/**
 * @class   igQtChangeBackGroundDialog
 * @brief   igQtChangeBackGroundDialog's brief
 */


#include "IQComponents/Dialog/igQtChangeBackGroundDialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QIntValidator>
#include <QListWidget>
#include <QColorDialog>

#include <iGameSceneManager.h>
igQtChangeBackGroundDialog::igQtChangeBackGroundDialog(QWidget *parent) : QDialog(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout* hlay = new QHBoxLayout();

    igm::vec3 RGB = iGame::SceneManager::Instance()->GetCurrentScene()->GetBackGround();

    int R = (int)(RGB.x * 255), G = (int)(RGB.y * 255), B = (int)(RGB.z * 255);
    m_Red_LineEdit =   new QLineEdit(QString::number(R),this);
    m_Green_LineEdit = new QLineEdit(QString::number(G),this);
    m_Blue_LineEdit =  new QLineEdit(QString::number(B),this);

    // 创建一个正则表达式，匹配 0~255 的数字
    QRegExp regExp("^(0|[1-9]\\d?|1\\d{2}|2[0-4]\\d|25[0-5])$");

    // 创建一个 QRegExpValidator，使用正则表达式
    QRegExpValidator *validator = new QRegExpValidator(regExp, this);
    m_Red_LineEdit->setValidator(validator);
    m_Green_LineEdit->setValidator(validator);
    m_Blue_LineEdit->setValidator(validator);

    QLabel* tip_label = new QLabel("范围: [0, 255]");
    QLabel* red_label  = new QLabel("R:", this);
    QLabel* green_label  = new QLabel("G:", this);
    QLabel* blue_label  = new QLabel("B:", this);
    hlay->addWidget(red_label),  hlay->addWidget(m_Red_LineEdit);
    hlay->addWidget(green_label),  hlay->addWidget(m_Green_LineEdit);
    hlay->addWidget(blue_label),  hlay->addWidget(m_Blue_LineEdit);
    auto *okButton = new QPushButton("OK", this);
    auto *EditButton = new QPushButton("自定义调色板", this);
    layout->addWidget(tip_label);
    layout->addLayout(hlay);
    layout->addWidget(EditButton);
    layout->addWidget(okButton);
    connect(okButton, &QPushButton::clicked, this, &igQtChangeBackGroundDialog::accept);
    connect(EditButton, &QPushButton::clicked, this, [&](){
        QColor color = QColorDialog::getColor(QColor(R, G, B), this, "");
        m_Red_LineEdit->setText(QString::number(color.red()));
        m_Green_LineEdit->setText(QString::number(color.green()));
        m_Blue_LineEdit->setText(QString::number(color.blue()));
    });

}

std::vector<int> igQtChangeBackGroundDialog::getInput() {
    return {m_Red_LineEdit->text().toInt(), m_Green_LineEdit->text().toInt(), m_Blue_LineEdit->text().toInt()};
}