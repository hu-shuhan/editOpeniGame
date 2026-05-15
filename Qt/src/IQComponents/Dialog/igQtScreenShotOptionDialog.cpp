#include "IQComponents/Dialog/igQtScreenShotOptionDialog.h"

#include <QFormLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

igQtScreenShotOptionDialog::igQtScreenShotOptionDialog(QWidget* parent) : igQtChromeFramelessDialog(parent) {
    setMinimumSize(460, 260);
    resize(520, 280);
    setMaximizeEnabled(false);

    auto* body = new QWidget(this);
    body->setAttribute(Qt::WA_StyledBackground, true);
    body->setStyleSheet(
        "QWidget { background-color: transparent; color: #EAEAEA; }"
        "QLabel { color: #D8D8D8; }"
        "QLineEdit { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 4px; border-radius: 3px; }"
        "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3A3A3A; }"
        "QPushButton:pressed { background-color: #252526; }");

    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(14, 10, 14, 14);
    layout->setSpacing(12);

    m_WidthLineEdit = new QLineEdit(QStringLiteral("1920"), body);
    m_HeightLineEdit = new QLineEdit(QStringLiteral("1080"), body);
    m_WidthLineEdit->setValidator(new QIntValidator(1, 9999, m_WidthLineEdit));
    m_HeightLineEdit->setValidator(new QIntValidator(1, 9999, m_HeightLineEdit));
    m_WidthLineEdit->setMinimumWidth(160);
    m_HeightLineEdit->setMinimumWidth(160);

    auto* widthLabel = new QLabel(QStringLiteral("width :"), body);
    auto* heightLabel = new QLabel(QStringLiteral("height :"), body);
    widthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    heightLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* form = new QFormLayout();
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setContentsMargins(0, 0, 0, 0);
    form->addRow(widthLabel, m_WidthLineEdit);
    form->addRow(heightLabel, m_HeightLineEdit);

    layout->addLayout(form);

    auto* okButton = new QPushButton(QStringLiteral("OK"), body);
    layout->addWidget(okButton, 0, Qt::AlignRight);

    connect(okButton, &QPushButton::clicked, this, &igQtScreenShotOptionDialog::accept);

    setContentWidget(body);
}

std::pair<int, int> igQtScreenShotOptionDialog::getInput() {
    return {m_WidthLineEdit->text().toInt(), m_HeightLineEdit->text().toInt()};
}
