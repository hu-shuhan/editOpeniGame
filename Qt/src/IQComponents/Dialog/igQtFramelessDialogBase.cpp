#include "IQComponents/Dialog/igQtFramelessDialogBase.h"

#include <QBitmap>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

igQtFramelessDialogBase::igQtFramelessDialogBase(QWidget* parent) : QDialog(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setStyleSheet(
        "QDialog { background-color: #1F1F1F; border: 1px solid #3C3C3C; }"
        "QWidget#framelessTitleBar { background-color: #2D2D30; border-bottom: 1px solid #3C3C3C; }"
        "QLabel#framelessTitleLabel { color: #E0E0E0; font-size: 14px; font-weight: 500; padding-left: 8px; }"
        "QPushButton#framelessCloseButton {"
        "  min-width: 28px; max-width: 28px; min-height: 24px; max-height: 24px;"
        "  background-color: transparent; color: #E0E0E0; border: none; font-size: 15px;"
        "}"
        "QPushButton#framelessCloseButton:hover { background-color: #C42B1C; }"
        "QPushButton#framelessCloseButton:pressed { background-color: #A2261A; }"
        "QWidget#framelessContentHost { background-color: #1F1F1F; border: none; }");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("framelessTitleBar");
    m_titleBar->setFixedHeight(34);
    auto* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(6, 4, 6, 4);
    titleLayout->setSpacing(6);

    m_titleLabel = new QLabel(windowTitle(), m_titleBar);
    m_titleLabel->setObjectName("framelessTitleLabel");
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_closeButton = new QPushButton(QStringLiteral("x"), m_titleBar);
    m_closeButton->setObjectName("framelessCloseButton");
    m_closeButton->setCursor(Qt::PointingHandCursor);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addWidget(m_closeButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    m_contentHost = new QWidget(this);
    m_contentHost->setObjectName("framelessContentHost");
    m_contentLayout = new QVBoxLayout(m_contentHost);
    m_contentLayout->setContentsMargins(12, 12, 12, 12);
    m_contentLayout->setSpacing(10);

    rootLayout->addWidget(m_titleBar);
    rootLayout->addWidget(m_contentHost);
    updateRoundedMask();
}

void igQtFramelessDialogBase::setDialogTitle(const QString& title) {
    setWindowTitle(title);
    if (m_titleLabel) m_titleLabel->setText(title);
}

void igQtFramelessDialogBase::setContentWidget(QWidget* widget) {
    if (!m_contentLayout || !widget) return;
    if (widget->parentWidget() != m_contentHost) widget->setParent(m_contentHost);
    m_contentLayout->addWidget(widget);
}

void igQtFramelessDialogBase::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_titleBar && m_titleBar->geometry().contains(event->pos())) {
        if (!m_closeButton->geometry().contains(m_titleBar->mapFrom(this, event->pos()))) {
            m_dragging = true;
            m_dragOffset = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}

void igQtFramelessDialogBase::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void igQtFramelessDialogBase::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

void igQtFramelessDialogBase::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    updateRoundedMask();
}

void igQtFramelessDialogBase::updateRoundedMask() {
    if (width() <= 0 || height() <= 0) return;
    QBitmap mask(size());
    mask.fill(Qt::color0);
    {
        QPainter painter(&mask);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::color1);
        painter.drawRoundedRect(mask.rect().adjusted(0, 0, -1, -1), m_cornerRadius, m_cornerRadius);
    }
    setMask(mask);
}

