#include <IQWidgets/igQtRoundedCornerHelper.h>

#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>

namespace {

const char* const kOverlayObjectName = "RoundedCornerOverlay";
const char* const kOverlaySyncObjectName = "RoundedCornerOverlaySync";

void paintCornerWedge(QPainter& painter, const QRectF& cornerBox, qreal startAngle, const QColor& coverColor) {
    QPainterPath disc;
    disc.moveTo(cornerBox.center());
    disc.arcTo(cornerBox, startAngle, 90.0);
    disc.closeSubpath();

    QPainterPath wedge;
    wedge.addRect(cornerBox);
    painter.fillPath(wedge.subtracted(disc), coverColor);
}

class OverlaySync final : public QObject {
public:
    OverlaySync(QWidget* target, QWidget* overlay) : QObject(target), m_target(target), m_overlay(overlay) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        Q_UNUSED(watched);
        if (m_target && m_overlay) {
            switch (event->type()) {
                case QEvent::Resize:
                case QEvent::Show:
                case QEvent::LayoutRequest:
                case QEvent::StyleChange:
                    m_overlay->setGeometry(m_target->rect());
                    m_overlay->raise();
                    break;
                default:
                    break;
            }
        }
        return false;
    }

private:
    QPointer<QWidget> m_target;
    QPointer<QWidget> m_overlay;
};

}

igQtRoundedCornerOverlay::igQtRoundedCornerOverlay(QWidget* parent)
    : QWidget(parent), m_radius{0.0, 0.0, 0.0, 0.0}, m_coverColor(0x1E, 0x1E, 0x1E) {
    setObjectName(QString::fromLatin1(kOverlayObjectName));
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::NoFocus);
}

void igQtRoundedCornerOverlay::setUniformRadius(qreal radius) {
    setCornerRadii(radius, radius, radius, radius);
}

void igQtRoundedCornerOverlay::setCornerRadii(qreal topLeft, qreal topRight, qreal bottomRight, qreal bottomLeft) {
    m_radius[0] = qMax<qreal>(0.0, topLeft);
    m_radius[1] = qMax<qreal>(0.0, topRight);
    m_radius[2] = qMax<qreal>(0.0, bottomRight);
    m_radius[3] = qMax<qreal>(0.0, bottomLeft);
    update();
}

void igQtRoundedCornerOverlay::setCoverColor(const QColor& color) {
    m_coverColor = color;
    update();
}

void igQtRoundedCornerOverlay::paintEvent(QPaintEvent*) {
    const qreal w = width();
    const qreal h = height();
    if (w <= 0.0 || h <= 0.0) return;

    const qreal maxRadius = qMin(w, h) / 2.0;
    bool any = false;
    for (int i = 0; i < 4; ++i) {
        m_radius[i] = qMin(m_radius[i], maxRadius);
        if (m_radius[i] > 0.0) any = true;
    }
    if (!any) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    const qreal startAngles[4] = {90.0, 0.0, 270.0, 180.0};
    const bool isTop[4] = {true, true, false, false};
    const bool isLeft[4] = {true, false, false, true};

    for (int i = 0; i < 4; ++i) {
        const qreal r = m_radius[i];
        if (r <= 0.0) continue;
        const QRectF box(isLeft[i] ? 0.0 : w - 2.0 * r, isTop[i] ? 0.0 : h - 2.0 * r, 2.0 * r, 2.0 * r);
        paintCornerWedge(painter, box, startAngles[i], m_coverColor);
    }
}

void igQtAttachRoundedCorners(QWidget* target, qreal topLeft, qreal topRight, qreal bottomRight, qreal bottomLeft,
                              const QColor& coverColor) {
    if (!target) return;

    auto* overlay = target->findChild<igQtRoundedCornerOverlay*>(QString::fromLatin1(kOverlayObjectName),
                                                                Qt::FindDirectChildrenOnly);
    if (!overlay) {
        overlay = new igQtRoundedCornerOverlay(target);
        auto* sync = new OverlaySync(target, overlay);
        sync->setObjectName(QString::fromLatin1(kOverlaySyncObjectName));
        target->installEventFilter(sync);
    }

    overlay->setCornerRadii(topLeft, topRight, bottomRight, bottomLeft);
    overlay->setCoverColor(coverColor);
    overlay->setGeometry(target->rect());
    overlay->show();
    overlay->raise();
}

void igQtAttachRoundedCorners(QWidget* target, qreal radius, const QColor& coverColor) {
    igQtAttachRoundedCorners(target, radius, radius, radius, radius, coverColor);
}

void igQtRefreshRoundedCorners(QWidget* target, const QColor& coverColor) {
    if (!target) return;
    auto* overlay = target->findChild<igQtRoundedCornerOverlay*>(QString::fromLatin1(kOverlayObjectName),
                                                                Qt::FindDirectChildrenOnly);
    if (overlay) overlay->setCoverColor(coverColor);
}

void igQtDetachRoundedCorners(QWidget* target) {
    if (!target) return;
    if (auto* overlay = target->findChild<igQtRoundedCornerOverlay*>(QString::fromLatin1(kOverlayObjectName),
                                                                    Qt::FindDirectChildrenOnly)) {
        overlay->hide();
        overlay->deleteLater();
    }
    if (auto* sync = target->findChild<QObject*>(QString::fromLatin1(kOverlaySyncObjectName),
                                                 Qt::FindDirectChildrenOnly)) {
        sync->deleteLater();
    }
}
