#if defined(FFMPEG_ENABLE)
#include "IQComponents/Dialog/igQtVideoOptionDialog.h"


#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QIntValidator>

igQtVideoOptionDialog::igQtVideoOptionDialog(QWidget *parent) : QDialog(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    auto* validator = new QIntValidator(
            1, 9999, this);
    auto* LineValidator =
            new QRegExpValidator(QRegExp("^[0-9]*\\.?[0-9]*$"), this);

    QHBoxLayout* hlay_0 = new QHBoxLayout();
    m_Width_LineEdit = new QLineEdit("1920",this);
    m_Width_LineEdit->setValidator(validator);
    QLabel* width_label  = new QLabel("width :", this);
    hlay_0->addWidget(width_label);
    hlay_0->addWidget(m_Width_LineEdit);

    QHBoxLayout* hlay_1 = new QHBoxLayout();
    m_Height_LineEdit = new QLineEdit("1080",this);
    m_Height_LineEdit->setValidator(validator);
    QLabel* height_label = new QLabel("height :", this);
    hlay_1->addWidget(height_label);
    hlay_1->addWidget(m_Height_LineEdit);


    QHBoxLayout* hlay_2 = new QHBoxLayout();
    m_frameRate_LineEdit = new QLineEdit("1",this);
    m_frameRate_LineEdit->setValidator(LineValidator);

    QLabel* frameRate_label = new QLabel("Frame Rate :", this);
    hlay_2->addWidget(frameRate_label);
    hlay_2->addWidget(m_frameRate_LineEdit);

    QHBoxLayout* hlay_3 = new QHBoxLayout();
    m_bitRate_LineEdit = new QLineEdit("4000000",this);// 400kbps
    m_bitRate_LineEdit->setValidator(LineValidator);
    QLabel* bitRate_label = new QLabel("Bit rate :", this);
    hlay_3->addWidget(bitRate_label);
    hlay_3->addWidget(m_bitRate_LineEdit);

    layout->addLayout(hlay_0);
    layout->addLayout(hlay_1);
    layout->addLayout(hlay_2);
    layout->addLayout(hlay_3);

    auto *okButton = new QPushButton("OK", this);
    layout->addWidget(okButton);

    connect(okButton, &QPushButton::clicked, this, &igQtVideoOptionDialog::accept);
}

iGame::VideoInputInfo igQtVideoOptionDialog::getInput() {
    iGame::VideoInputInfo res;
    res.width = m_Width_LineEdit->text().toInt();
    res.height = m_Height_LineEdit->text().toInt();
    res.frame_rate = m_frameRate_LineEdit->text().toInt();
    res.bit_rate = m_bitRate_LineEdit->text().toInt();
    return res;
}
#endif