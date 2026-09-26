/**
 * @file     igQtRoundedCornerHelper.h
 * @brief    抗锯齿圆角工具（子控件圆角统一方案）
 */

#pragma once

#include <IQCore/igQtExportModule.h>
#include <QColor>
#include <QWidget>

class IG_QT_MODULE_EXPORT igQtRoundedCornerOverlay : public QWidget {
    Q_OBJECT
public:
    explicit igQtRoundedCornerOverlay(QWidget* parent = nullptr);

    void setUniformRadius(qreal radius);
    void setCornerRadii(qreal topLeft, qreal topRight, qreal bottomRight, qreal bottomLeft);
    void setCoverColor(const QColor& color);

    QColor coverColor() const { return m_coverColor; }
    qreal cornerRadius(int corner) const { return m_radius[qBound(0, corner, 3)]; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qreal m_radius[4];
    QColor m_coverColor;
};

IG_QT_MODULE_EXPORT void igQtAttachRoundedCorners(QWidget* target, qreal radius, const QColor& coverColor);
IG_QT_MODULE_EXPORT void igQtAttachRoundedCorners(QWidget* target, qreal topLeft, qreal topRight, qreal bottomRight,
                                                 qreal bottomLeft, const QColor& coverColor);
IG_QT_MODULE_EXPORT void igQtRefreshRoundedCorners(QWidget* target, const QColor& coverColor);
IG_QT_MODULE_EXPORT void igQtDetachRoundedCorners(QWidget* target);
