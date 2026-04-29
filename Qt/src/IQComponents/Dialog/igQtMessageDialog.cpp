#include "IQComponents/Dialog/igQtMessageDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

igQtMessageDialog::igQtMessageDialog(QWidget* parent) : igQtFramelessDialogBase(parent) {
    resize(420, 220);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(8, 8, 8, 8);
    bodyLayout->setSpacing(12);

    m_messageLabel = new QLabel(body);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_messageLabel->setStyleSheet("QLabel { color: #D0D0D0; font-size: 14px; }");

    auto* buttonsRow = new QHBoxLayout();
    buttonsRow->addStretch();

    m_okButton = new QPushButton(QStringLiteral("确定"), body);
    m_okButton->setCursor(Qt::PointingHandCursor);
    m_okButton->setMinimumSize(88, 30);
    m_okButton->setStyleSheet(
        "QPushButton { background-color: #2D2D30; color: #E0E0E0; border: 1px solid #3C3C3C; border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background-color: #3A3A3D; }"
        "QPushButton:pressed { background-color: #45454A; }");
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonsRow->addWidget(m_okButton);

    bodyLayout->addWidget(m_messageLabel, 1);
    bodyLayout->addLayout(buttonsRow);

    setContentWidget(body);
}

void igQtMessageDialog::setMessage(const QString& text) {
    if (m_messageLabel) m_messageLabel->setText(text);
}

void igQtMessageDialog::information(QWidget* parent, const QString& title, const QString& text) {
    igQtMessageDialog dialog(parent);
    dialog.setDialogTitle(title);
    dialog.setMessage(text);
    dialog.exec();
}

