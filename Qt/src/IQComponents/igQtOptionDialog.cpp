#include "IQComponents/igQtOptionDialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>


igQtOptionDialog::igQtOptionDialog(QWidget *parent) : QDialog(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout* hlay_0 = new QHBoxLayout();
    QHBoxLayout* hlay_1 = new QHBoxLayout();
    m_WidthLineEdit = new QLineEdit(this);
    m_HeightLineEdit = new QLineEdit(this);
    QLabel* width_label  = new QLabel("width", this);
    QLabel* height_label = new QLabel("height", this);

    hlay_0->addWidget(width_label),  hlay_0->addWidget(m_WidthLineEdit);
    hlay_1->addWidget(height_label), hlay_1->addWidget(m_HeightLineEdit);

    layout->addLayout(hlay_0), layout->addLayout(hlay_1);
    auto *okButton = new QPushButton("OK", this);
    layout->addWidget(okButton);

    connect(okButton, &QPushButton::clicked, this, &igQtOptionDialog::accept);
}

std::pair<int, int> igQtOptionDialog::getInput() {
    return {m_WidthLineEdit->text().toInt(), m_HeightLineEdit->text().toInt()};
}
