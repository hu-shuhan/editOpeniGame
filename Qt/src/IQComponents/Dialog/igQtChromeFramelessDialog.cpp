#include "IQComponents/Dialog/igQtChromeFramelessDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <windowsx.h>
#endif

namespace {
constexpr int kCaptionHeight = 34;
constexpr int kResizeBorder = 5;
constexpr int kPanelCornerRadius = 10;
constexpr int kRootMarginNormal = 3;
// 半透明外壳（略透底，最大化时略更不透明以保证可读）
constexpr int kShellFillAlphaNormal = 236;
constexpr int kShellFillAlphaMaximized = 252;
constexpr int kShellBorderAlpha = 125;
} // namespace

igQtChromeFramelessDialog::igQtChromeFramelessDialog(QWidget* parent) : QDialog(parent) {
    setObjectName(QStringLiteral("ChromeToolFramelessDialog"));
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);
    setModal(false);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    if (parentWidget()) setWindowIcon(parentWidget()->windowIcon());

    setStyleSheet(
        "QDialog#ChromeToolFramelessDialog { background: transparent; border: none; }"
        "QWidget#ChromeCaptionBar { background: transparent; border: none; }"
        "QLabel#ChromeCaptionTitle { background: transparent; color: rgba(220, 222, 228, 0.88); font-size: 13px; font-weight: 500; padding-left: 10px; }"
        "QWidget#ChromeCaptionBar QPushButton { border: none; padding: 0 10px; color: rgba(220, 222, 228, 0.9); }"
        "QWidget#ChromeCaptionBar QPushButton:hover { background-color: rgba(255, 255, 255, 0.07); }"
        "QPushButton#ChromeMinimizeButton, QPushButton#ChromeMaximizeButton {"
        "  padding: 0; min-width: 46px; max-width: 46px; min-height: 30px; max-height: 30px;"
        "  background: transparent;"
        "}"
        "QPushButton#ChromeMinimizeButton:hover, QPushButton#ChromeMaximizeButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.1);"
        "}"
        "QPushButton#ChromeMinimizeButton:pressed, QPushButton#ChromeMaximizeButton:pressed {"
        "  background-color: rgba(255, 255, 255, 0.16);"
        "}"
        "QPushButton#ChromeCloseButton {"
        "  padding: 0; min-width: 46px; max-width: 46px; min-height: 30px; max-height: 30px;"
        "  background: transparent; font-size: 18px; color: rgba(230, 230, 235, 0.92);"
        "}"
        "QPushButton#ChromeCloseButton:hover { background-color: rgba(232, 17, 35, 0.88); color: rgba(255, 255, 255, 0.95); }"
        "QPushButton#ChromeCloseButton:pressed { background-color: rgba(197, 15, 31, 0.9); color: rgba(255, 255, 255, 0.95); }"
        "QWidget#ChromeContentHost { background: transparent; border: none; }");

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(kRootMarginNormal, kRootMarginNormal, kRootMarginNormal, kRootMarginNormal);
    m_rootLayout->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName(QStringLiteral("ChromeCaptionBar"));
    m_titleBar->setFixedHeight(kCaptionHeight);
    auto* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    m_titleLabel = new QLabel(windowTitle(), m_titleBar);
    m_titleLabel->setObjectName(QStringLiteral("ChromeCaptionTitle"));
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_minimizeButton = new QPushButton(m_titleBar);
    m_minimizeButton->setObjectName(QStringLiteral("ChromeMinimizeButton"));
    m_minimizeButton->setIcon(QIcon(QStringLiteral(":/Ticon/Icons/window_minimize_white.svg")));
    m_minimizeButton->setIconSize(QSize(12, 12));
    m_minimizeButton->setCursor(Qt::PointingHandCursor);
    m_minimizeButton->setFlat(true);

    m_maximizeButton = new QPushButton(m_titleBar);
    m_maximizeButton->setObjectName(QStringLiteral("ChromeMaximizeButton"));
    m_maximizeButton->setIconSize(QSize(12, 12));
    m_maximizeButton->setCursor(Qt::PointingHandCursor);
    m_maximizeButton->setFlat(true);

    m_closeButton = new QPushButton(QStringLiteral("\u00D7"), m_titleBar);
    m_closeButton->setObjectName(QStringLiteral("ChromeCloseButton"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFlat(true);

    titleLayout->addWidget(m_titleLabel, 1);
    titleLayout->addWidget(m_minimizeButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    titleLayout->addWidget(m_maximizeButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    titleLayout->addWidget(m_closeButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    m_contentHost = new QWidget(this);
    m_contentHost->setObjectName(QStringLiteral("ChromeContentHost"));
    m_contentLayout = new QVBoxLayout(m_contentHost);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(0);

    m_rootLayout->addWidget(m_titleBar);
    m_rootLayout->addWidget(m_contentHost, 1);

    connect(m_minimizeButton, &QPushButton::clicked, this, &igQtChromeFramelessDialog::showMinimized);
    connect(m_maximizeButton, &QPushButton::clicked, this, [this]() { toggleMaximizeRestore(); });
    connect(m_closeButton, &QPushButton::clicked, this, &igQtChromeFramelessDialog::close);

    setMinimumSize(480, 320);
    updateMaximizeButtonIcon();
    updateFrameMarginsForWindowState();
}

void igQtChromeFramelessDialog::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRect r = rect().adjusted(1, 1, -1, -1);
    const int fillA = m_opaqueShell ? 255 : (isMaximized() ? kShellFillAlphaMaximized : kShellFillAlphaNormal);
    const QColor fill(30, 30, 30, fillA);
    const QColor border(90, 92, 98, kShellBorderAlpha);
    if (isMaximized()) {
        painter.fillRect(rect(), fill);
        return;
    }
    painter.setPen(QPen(border, 2));
    painter.setBrush(fill);
    painter.drawRoundedRect(r, kPanelCornerRadius, kPanelCornerRadius);
}

void igQtChromeFramelessDialog::updateFrameMarginsForWindowState() {
    if (!m_rootLayout) return;
    if (isMaximized())
        m_rootLayout->setContentsMargins(0, 0, 0, 0);
    else
        m_rootLayout->setContentsMargins(kRootMarginNormal, kRootMarginNormal, kRootMarginNormal, kRootMarginNormal);
}

void igQtChromeFramelessDialog::setDialogTitle(const QString& title) {
    setWindowTitle(title);
    if (m_titleLabel) m_titleLabel->setText(title);
}

void igQtChromeFramelessDialog::setContentWidget(QWidget* widget) {
    if (!m_contentLayout || !widget) return;
    if (widget->parentWidget() != m_contentHost) widget->setParent(m_contentHost);
    m_contentLayout->addWidget(widget, 1);
}

void igQtChromeFramelessDialog::setMaximizeEnabled(bool enabled) {
    m_maximizeEnabled = enabled;
    if (m_maximizeButton) {
        m_maximizeButton->setVisible(enabled);
        m_maximizeButton->setEnabled(enabled);
    }
    if (!enabled && isMaximized()) {
        showNormal();
        if (m_normalGeometry.isValid() && m_normalGeometry.width() >= minimumWidth() &&
            m_normalGeometry.height() >= minimumHeight()) {
            setGeometry(m_normalGeometry);
        }
        updateMaximizeButtonIcon();
        updateFrameMarginsForWindowState();
    }
}

void igQtChromeFramelessDialog::setOpaqueShell(bool opaque) {
    m_opaqueShell = opaque;
    update();
}

bool igQtChromeFramelessDialog::isOnCaptionButton(const QPoint& dialogPos) const {
    if (!m_titleBar || !m_minimizeButton || !m_closeButton) return false;
    const QPoint inTitle = m_titleBar->mapFrom(this, dialogPos);
    if (m_minimizeButton->geometry().contains(inTitle) || m_closeButton->geometry().contains(inTitle)) return true;
    if (m_maximizeEnabled && m_maximizeButton && m_maximizeButton->isVisible() &&
        m_maximizeButton->geometry().contains(inTitle)) {
        return true;
    }
    return false;
}

void igQtChromeFramelessDialog::mousePressEvent(QMouseEvent* event) {
#if !defined(Q_OS_WIN)
    if (event->button() == Qt::LeftButton && !isMaximized()) {
        const Qt::Edges edges = hitTestEdges(event->pos());
        if (edges != Qt::Edges()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            if (QWindow* wh = windowHandle()) {
                wh->startSystemResize(edges);
                event->accept();
                return;
            }
#else
            beginManualResize(edges, event->globalPos());
            event->accept();
            return;
#endif
        }
    }
#endif
    if (event->button() == Qt::LeftButton && m_titleBar && m_titleBar->geometry().contains(event->pos())) {
        if (!isOnCaptionButton(event->pos())) {
            if (isMaximized()) {
                const qreal ratioX =
                        qBound<qreal>(0.0, static_cast<qreal>(event->pos().x()) / qMax(1, width()), 1.0);
                showNormal();
                const int newX = event->globalPos().x() - static_cast<int>(width() * ratioX);
                const int newY = event->globalPos().y() - m_titleBar->height() / 2;
                m_dragOffset = event->globalPos() - QPoint(newX, newY);
                move(newX, newY);
            } else {
                m_dragging = true;
                m_dragOffset = event->globalPos() - frameGeometry().topLeft();
            }
            event->accept();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}

void igQtChromeFramelessDialog::mouseMoveEvent(QMouseEvent* event) {
#if !defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        performManualResize(event->globalPos());
        event->accept();
        return;
    }
#endif
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
#if !defined(Q_OS_WIN)
    if (!isMaximized()) {
        const Qt::Edges e = hitTestEdges(event->pos());
        setCursor(QCursor(cursorForEdges(e)));
    } else {
        unsetCursor();
    }
#endif
    QDialog::mouseMoveEvent(event);
}

void igQtChromeFramelessDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
#if !defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        m_resizing = false;
#endif
    }
    QDialog::mouseReleaseEvent(event);
}

void igQtChromeFramelessDialog::mouseDoubleClickEvent(QMouseEvent* event) {
    if (m_maximizeEnabled && event->button() == Qt::LeftButton && m_titleBar &&
        m_titleBar->geometry().contains(event->pos()) && !isOnCaptionButton(event->pos())) {
        toggleMaximizeRestore();
        event->accept();
        return;
    }
    QDialog::mouseDoubleClickEvent(event);
}

void igQtChromeFramelessDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButtonIcon();
        updateFrameMarginsForWindowState();
    }
    QDialog::changeEvent(event);
}

void igQtChromeFramelessDialog::leaveEvent(QEvent* event) {
#if !defined(Q_OS_WIN)
    unsetCursor();
#endif
    QDialog::leaveEvent(event);
}

void igQtChromeFramelessDialog::updateMaximizeButtonIcon() {
    if (!m_maximizeButton) return;
    m_maximizeButton->setIcon(QIcon(isMaximized() ? QStringLiteral(":/Ticon/Icons/window_restore_white.svg")
                                                    : QStringLiteral(":/Ticon/Icons/window_maximize_white.svg")));
    m_maximizeButton->setIconSize(isMaximized() ? QSize(15, 15) : QSize(12, 12));
    m_maximizeButton->setText(QString());
}

void igQtChromeFramelessDialog::toggleMaximizeRestore() {
    if (!m_maximizeEnabled) return;
    if (isMaximized()) {
        showNormal();
        if (m_normalGeometry.isValid() && m_normalGeometry.width() >= minimumWidth() &&
            m_normalGeometry.height() >= minimumHeight()) {
            setGeometry(m_normalGeometry);
        }
        updateMaximizeButtonIcon();
        updateFrameMarginsForWindowState();
        return;
    }
    m_normalGeometry = geometry();
    showMaximized();
    updateMaximizeButtonIcon();
    updateFrameMarginsForWindowState();
}

#if !defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
void igQtChromeFramelessDialog::beginManualResize(Qt::Edges edges, const QPoint& globalPos) {
    m_resizing = true;
    m_resizeEdges = edges;
    m_resizeStartGlobalPos = globalPos;
    m_resizeStartGeometry = frameGeometry();
}

void igQtChromeFramelessDialog::performManualResize(const QPoint& globalPos) {
    const QPoint delta = globalPos - m_resizeStartGlobalPos;
    QRect geo = m_resizeStartGeometry;

    if (m_resizeEdges & Qt::LeftEdge) {
        int newWidth = m_resizeStartGeometry.width() - delta.x();
        int newLeft = m_resizeStartGeometry.left() + delta.x();
        if (newWidth < minimumWidth()) {
            newLeft = m_resizeStartGeometry.right() - minimumWidth() + 1;
            newWidth = minimumWidth();
        }
        geo.setLeft(newLeft);
        geo.setWidth(newWidth);
    } else if (m_resizeEdges & Qt::RightEdge) {
        int newWidth = m_resizeStartGeometry.width() + delta.x();
        if (newWidth < minimumWidth()) newWidth = minimumWidth();
        geo.setWidth(newWidth);
    }

    if (m_resizeEdges & Qt::TopEdge) {
        int newHeight = m_resizeStartGeometry.height() - delta.y();
        int newTop = m_resizeStartGeometry.top() + delta.y();
        if (newHeight < minimumHeight()) {
            newTop = m_resizeStartGeometry.bottom() - minimumHeight() + 1;
            newHeight = minimumHeight();
        }
        geo.setTop(newTop);
        geo.setHeight(newHeight);
    } else if (m_resizeEdges & Qt::BottomEdge) {
        int newHeight = m_resizeStartGeometry.height() + delta.y();
        if (newHeight < minimumHeight()) newHeight = minimumHeight();
        geo.setHeight(newHeight);
    }

    setGeometry(geo);
}
#endif

#if !defined(Q_OS_WIN)
Qt::Edges igQtChromeFramelessDialog::hitTestEdges(const QPoint& pos) const {
    if (minimumSize() == maximumSize()) {
        return {};
    }
    Qt::Edges e = {};
    if (pos.x() <= kResizeBorder) e |= Qt::LeftEdge;
    if (pos.x() >= width() - kResizeBorder) e |= Qt::RightEdge;
    if (pos.y() <= kResizeBorder) e |= Qt::TopEdge;
    if (pos.y() >= height() - kResizeBorder) e |= Qt::BottomEdge;
    return e;
}

Qt::CursorShape igQtChromeFramelessDialog::cursorForEdges(Qt::Edges edges) {
    const bool h = (edges & Qt::LeftEdge) || (edges & Qt::RightEdge);
    const bool v = (edges & Qt::TopEdge) || (edges & Qt::BottomEdge);
    if (h && v) {
        const bool left = (edges & Qt::LeftEdge);
        const bool top = (edges & Qt::TopEdge);
        if ((left && top) || (!left && !top)) return Qt::SizeFDiagCursor;
        return Qt::SizeBDiagCursor;
    }
    if (h) return Qt::SizeHorCursor;
    if (v) return Qt::SizeVerCursor;
    return Qt::ArrowCursor;
}
#endif

#if defined(Q_OS_WIN)
bool igQtChromeFramelessDialog::nativeEvent(const QByteArray& eventType, void* message, long* result) {
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST && !isMaximized()) {
            const int bw = qMax(1, qRound(static_cast<qreal>(kResizeBorder) * devicePixelRatioF()));
            const int gx = GET_X_LPARAM(msg->lParam);
            const int gy = GET_Y_LPARAM(msg->lParam);
            const QPoint gp(gx, gy);
            const QPoint lp = mapFromGlobal(gp);
            if (!rect().contains(lp)) return QDialog::nativeEvent(eventType, message, result);

            const int x = lp.x();
            const int y = lp.y();
            const int w = width();
            const int h = height();
            const bool fixedSize = minimumSize() == maximumSize();

            if (!fixedSize && x < bw && y < bw) {
                *result = HTTOPLEFT;
                return true;
            }
            if (!fixedSize && x >= w - bw && y < bw) {
                *result = HTTOPRIGHT;
                return true;
            }
            if (!fixedSize && x < bw && y >= h - bw) {
                *result = HTBOTTOMLEFT;
                return true;
            }
            if (!fixedSize && x >= w - bw && y >= h - bw) {
                *result = HTBOTTOMRIGHT;
                return true;
            }
            if (!fixedSize && y < bw && x >= bw && x < w - bw) {
                *result = HTTOP;
                return true;
            }
            if (!fixedSize && y >= h - bw && x >= bw && x < w - bw) {
                *result = HTBOTTOM;
                return true;
            }
            if (!fixedSize && x < bw && y >= bw && y < h - bw) {
                *result = HTLEFT;
                return true;
            }
            if (!fixedSize && x >= w - bw && y >= bw && y < h - bw) {
                *result = HTRIGHT;
                return true;
            }

            if (m_titleBar && m_titleBar->geometry().contains(lp)) {
                if (!isOnCaptionButton(lp)) {
                    *result = HTCAPTION;
                    return true;
                }
                *result = HTCLIENT;
                return true;
            }
        }
    }
    return QDialog::nativeEvent(eventType, message, result);
}
#endif
