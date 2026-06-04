#include "IQComponents/Dialog/igQtDarkFramelessMessage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace {
QString iconHostStyleSheet(bool useInformationIcon) {
    if (useInformationIcon) {
        return QStringLiteral(
            "QLabel#DarkMessageIconHost {"
            "  background-color: rgba(56, 96, 132, 0.35);"
            "  border: 1px solid rgba(88, 140, 180, 0.55);"
            "  border-radius: 8px;"
            "  padding: 6px;"
            "}");
    }
    return QStringLiteral(
        "QLabel#DarkMessageIconHost {"
        "  background-color: rgba(132, 96, 48, 0.30);"
        "  border: 1px solid rgba(180, 130, 70, 0.50);"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "}");
}
} // namespace

igQtDarkFramelessMessageDialog::igQtDarkFramelessMessageDialog(QWidget* parent, const QString& title,
                                                               const QString& text, bool useInformationIcon)
    : igQtChromeFramelessDialog(parent) {
    setModal(true);
    setDialogTitle(title);
    setMinimumSize(400, 180);
    resize(460, 200);
    setMaximizeEnabled(false);

    auto* body = new QWidget(this);
    body->setAttribute(Qt::WA_StyledBackground, true);
    body->setStyleSheet(
        "QWidget { background-color: transparent; color: #EAEAEA; }"
        "QLabel#DarkMessageText { color: #C9C9C9; background: transparent; }"
        "QPushButton { background-color: #5A6066; color: #ECECEC; border: 1px solid #747C84;"
        "  padding: 6px 16px; border-radius: 4px; min-width: 64px; }"
        "QPushButton:hover { background-color: #666D74; }"
        "QPushButton:pressed { background-color: #4A5056; }");

    auto* root = new QVBoxLayout(body);
    root->setContentsMargins(14, 10, 14, 14);
    root->setSpacing(14);

    auto* row = new QHBoxLayout();
    row->setSpacing(14);

    const QStyle::StandardPixmap which =
            useInformationIcon ? QStyle::SP_MessageBoxInformation : QStyle::SP_MessageBoxWarning;
    auto* iconHost = new QLabel(body);
    iconHost->setObjectName(QStringLiteral("DarkMessageIconHost"));
    iconHost->setStyleSheet(iconHostStyleSheet(useInformationIcon));
    iconHost->setPixmap(style()->standardIcon(which).pixmap(32, 32));
    iconHost->setAlignment(Qt::AlignCenter);
    iconHost->setFixedSize(48, 48);

    auto* textLabel = new QLabel(text, body);
    textLabel->setObjectName(QStringLiteral("DarkMessageText"));
    textLabel->setWordWrap(true);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    row->addWidget(iconHost, 0, Qt::AlignTop);
    row->addWidget(textLabel, 1);
    root->addLayout(row);

    auto* okButton = new QPushButton(QStringLiteral("确定"), body);
    root->addWidget(okButton, 0, Qt::AlignRight);

    connect(okButton, &QPushButton::clicked, this, &igQtDarkFramelessMessageDialog::accept);

    setContentWidget(body);
}

void igQtShowDarkFramelessMessage(QWidget* parent, const QString& title, const QString& text,
                                  bool useInformationIcon) {
    igQtDarkFramelessMessageDialog dialog(parent, title, text, useInformationIcon);
    dialog.exec();
}
