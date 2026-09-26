#include "IQComponents/Dialog/igQtScreenShotOptionDialog.h"

#include <IQWidgets/igQtRenderWidget.h>
#include <QEvent>
#include <QFormLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QString bodyThemeQss() {
    return QStringLiteral(
                   "QWidget { background-color: transparent; color: %1; }"
                   "QLabel { color: %1; }"
                   "QLineEdit { background-color: %2; color: %1; border: 1px solid %3; padding: 4px; border-radius: 3px; }"
                   "QLineEdit:focus { border: 1px solid %4; }"
                   "QPushButton { background-color: %2; color: %1; border: 1px solid %3; padding: 6px 12px; border-radius: 4px; }"
                   "QPushButton:hover { background-color: %5; }"
                   "QPushButton:pressed { background-color: %6; }")
            .arg(igQtRenderWidget::uiRoleCss(igQtRenderWidget::UiRole::Text),
                 igQtRenderWidget::uiRoleCss(igQtRenderWidget::UiRole::PanelBg2),
                 igQtRenderWidget::uiRoleCss(igQtRenderWidget::UiRole::Border),
                 igQtRenderWidget::uiRoleCss(igQtRenderWidget::UiRole::Accent),
                 igQtRenderWidget::uiRoleCss(igQtRenderWidget::UiRole::HoverBg),
                 igQtRenderWidget::uiRoleCss(igQtRenderWidget::UiRole::SelectionBg));
}
}

igQtScreenShotOptionDialog::igQtScreenShotOptionDialog(QWidget* parent) : igQtChromeFramelessDialog(parent) {
    setMinimumSize(460, 260);
    resize(520, 280);
    setMaximizeEnabled(false);

    auto* body = new QWidget(this);
    m_body = body;
    body->setAttribute(Qt::WA_StyledBackground, true);
    body->setStyleSheet(bodyThemeQss());

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

void igQtScreenShotOptionDialog::changeEvent(QEvent* e) {
    if (e && e->type() == QEvent::StyleChange && m_body) {
        m_body->setStyleSheet(bodyThemeQss());
    }
    igQtChromeFramelessDialog::changeEvent(e);
}
