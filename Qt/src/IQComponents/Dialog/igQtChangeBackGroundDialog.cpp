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

    m_R = (int)(RGB.x * 255), m_G = (int)(RGB.y * 255), m_B = (int)(RGB.z * 255);
    m_Red_LineEdit =   new QLineEdit(QString::number(m_R),this);
    m_Green_LineEdit = new QLineEdit(QString::number(m_G),this);
    m_Blue_LineEdit =  new QLineEdit(QString::number(m_B),this);

    // 创建一个正则表达式，匹配 0~255 的数字
    QRegExp regExp("^(0|[1-9]\\d?|1\\d{2}|2[0-4]\\d|25[0-5])$");

    // 创建一个 QRegExpValidator，使用正则表达式
    QRegExpValidator *validator = new QRegExpValidator(regExp, this);
    m_Red_LineEdit->setValidator(validator);
    m_Green_LineEdit->setValidator(validator);
    m_Blue_LineEdit->setValidator(validator);

    hlay->addWidget(new QLabel("R:", this)),  hlay->addWidget(m_Red_LineEdit);
    hlay->addWidget(new QLabel("G:", this)),  hlay->addWidget(m_Green_LineEdit);
    hlay->addWidget(new QLabel("B:", this)),  hlay->addWidget(m_Blue_LineEdit);
    auto *okButton = new QPushButton("OK", this);
    auto *EditButton = new QPushButton("自定义调色板", this);



//    QHBoxLayout* hlay_color = new QHBoxLayout(this);
//    hlay_color->addWidget(new QLabel("范围: [0, 255]"));

//    QWidget* widget_display_color = new QWidget(this);
//    widget_display_color->setFixedSize(40, 20); // 设置固定大小
//    widget_display_color->setAutoFillBackground(true); // 允许自动填充背景
//    QPalette palette;
//    palette.setColor(QPalette::Window, QColor(R, G, B));
//    widget_display_color->setPalette(palette);
//    hlay_color->addWidget(new QLabel("当前颜色： "));
//    hlay_color->addWidget(widget_display_color);
//    hlay_color->setStretch(2, 1);
//    layout->addLayout(hlay_color);

    layout->addWidget(new QLabel("范围: [0, 255]"));
    layout->addLayout(hlay);
    layout->addWidget(EditButton);
    layout->addWidget(okButton);
    connect(okButton, &QPushButton::clicked, this, &igQtChangeBackGroundDialog::accept);
    connect(EditButton, &QPushButton::clicked, this, [&](){
        QColor color = QColorDialog::getColor(QColor(m_R, m_G, m_B), this, "");
        if(color != Qt::black) {
            m_Red_LineEdit->setText(QString::number(color.red()));
            m_Green_LineEdit->setText(QString::number(color.green()));
            m_Blue_LineEdit->setText(QString::number(color.blue()));
        }
    });

}

std::vector<int> igQtChangeBackGroundDialog::getInput() {
    m_R = m_Red_LineEdit->text().toInt(), m_G = m_Green_LineEdit->text().toInt(), m_B = m_Blue_LineEdit->text().toInt();
    return {m_R, m_G, m_B};
}