#include "IQComponents/Dialog/igQtScreenShotOptionDialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QIntValidator>

igQtScreenShotOptionDialog::igQtScreenShotOptionDialog(QWidget *parent) : QDialog(parent) {
    setStyleSheet(
            "QDialog { background-color: #1E1E1E; color: #EAEAEA; }"
            "QLabel { color: #D8D8D8; }"
            "QLineEdit { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 4px; }"
            "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 6px 12px; }"
            "QPushButton:hover { background-color: #3A3A3A; }"
            "QPushButton:pressed { background-color: #252526; }");

    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout* hlay_0 = new QHBoxLayout();
    QHBoxLayout* hlay_1 = new QHBoxLayout();
    m_WidthLineEdit = new QLineEdit("1920",this);
    m_HeightLineEdit = new QLineEdit("1080",this);
    auto* validator = new QIntValidator(
            1, 9999, this);
    m_WidthLineEdit->setValidator(validator);
    m_HeightLineEdit->setValidator(validator);

    QLabel* width_label  = new QLabel("width :", this);
    QLabel* height_label = new QLabel("height :", this);

    hlay_0->addWidget(width_label),  hlay_0->addWidget(m_WidthLineEdit);
    hlay_1->addWidget(height_label), hlay_1->addWidget(m_HeightLineEdit);

    layout->addLayout(hlay_0), layout->addLayout(hlay_1);
    auto *okButton = new QPushButton("OK", this);
    layout->addWidget(okButton);

    connect(okButton, &QPushButton::clicked, this, &igQtScreenShotOptionDialog::accept);
}

std::pair<int, int> igQtScreenShotOptionDialog::getInput() {
    return {m_WidthLineEdit->text().toInt(), m_HeightLineEdit->text().toInt()};
}