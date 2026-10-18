#include "IQComponents/igQtDataRangeSlider.h"
#include <IQWidgets/igQtRenderWidget.h>
#include <sstream>
#include <iomanip>

namespace
{
	const int SliderBarHeight = 10;
	const int HandleSideLength = SliderBarHeight + 5;

	const int LeftRightMargin = 10;
	const int TopMargin = 4;
}

igQtDataRangeSlider::igQtDataRangeSlider(QWidget* parent) :QWidget(parent) {
	range[0] = 0.0;
	range[1] = 1.0;
	min = 0.0;
	max = 1.0;
	setMouseTracking(true);
	setToolTip(QStringLiteral("拖动两侧圆点调整范围"));
}
void igQtDataRangeSlider::updateMinAndMax(float _min, float _max) {
	min = _min;
	max = _max;
	range[0] = 0.0;
	range[1] = 1.0;
	update();
}
void igQtDataRangeSlider::paintEvent(QPaintEvent* aEvent)
{
	Q_UNUSED(aEvent);
	using Role = igQtRenderWidget::UiRole;
	QPainter painter(this);
	this->sliderBarLength = this->width() - 2 * LeftRightMargin;
	int st = range[0] * sliderBarLength;
	int ed = range[1] * sliderBarLength;
	const QColor trackColor = igQtRenderWidget::uiRole(Role::CardBg);
	const QColor rangeColor = rangeInColor.isValid() ? rangeInColor : igQtRenderWidget::uiRole(Role::Accent);
	// 手柄颜色在绘制时按悬停/拖拽状态取（见下），此处不再预取
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);
	const qreal radius = SliderBarHeight / 2.0;
	const QRectF trackRect(LeftRightMargin, TopMargin, sliderBarLength, SliderBarHeight);
	painter.setBrush(trackColor);
	painter.drawRoundedRect(trackRect, radius, radius);
	if (ed > st) {
		painter.setBrush(rangeColor);
		painter.drawRoundedRect(QRectF(LeftRightMargin + st, TopMargin, ed - st, SliderBarHeight), radius, radius);
	}
	QPen trackPen(igQtRenderWidget::uiRole(Role::Border));
	trackPen.setWidth(1);
	painter.setPen(trackPen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRoundedRect(trackRect.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
	for (int i = 0; i < 2; i++) {
		const bool active = (i == m_HoverHandle || (isPressed && i == PressedHandle));
		QPen pen(active ? igQtRenderWidget::uiRole(Role::Text) : igQtRenderWidget::uiRole(Role::Border));
		pen.setWidth(active ? 2 : 1);
		painter.setPen(pen);
		painter.setBrush(active ? igQtRenderWidget::uiRole(Role::Accent) : igQtRenderWidget::uiRole(Role::Text));
		painter.drawEllipse(getHandleRectWithFloatValue(range[i]));
	}
	float minRange = range[0] * (max - min) + min;
	float maxRange = range[1] * (max - min) + min;
	std::ostringstream oss;
	oss << std::defaultfloat << std::setprecision(4);
	oss << "Range: (" << minRange << ", " << maxRange << ")";
	const QString text = QString::fromStdString(oss.str());
	QFontMetrics fm(painter.font());
	const int availW = this->width() - 2 * LeftRightMargin;
	const QString shown = fm.elidedText(text, Qt::ElideRight, qMax(0, availW));
	QRect textRect(LeftRightMargin, 2 * TopMargin + SliderBarHeight + 6, availW, fm.height());
	painter.setPen(igQtRenderWidget::uiRole(Role::Text));
	painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, shown);

}

QRectF igQtDataRangeSlider::getHandleRectWithFloatValue(float aValue) const
{
	return getHandleRect(aValue * sliderBarLength - HandleSideLength / 2 + LeftRightMargin);
}

QRectF igQtDataRangeSlider::getHandleRect(int aValue) const
{
	return QRect(aValue, TopMargin, HandleSideLength, HandleSideLength);
}

void igQtDataRangeSlider::mousePressEvent(QMouseEvent* aEvent)
{
	this->isPressed = true;
	this->PressedHandle = -1;
	if (aEvent->buttons() & Qt::LeftButton) {
		int  posValue;
		posValue = aEvent->pos().x();
		for (int i = 0; i < 2; i++) {
			if (getHandleRectWithFloatValue(range[i]).contains(aEvent->pos())) {
				if (PressedHandle != -1) {
					if (abs(getHandleRectWithFloatValue(range[i]).center().x() - posValue)
						< abs(getHandleRectWithFloatValue(range[PressedHandle]).center().x() - posValue)) {
						PressedHandle = i;
					}
				}
				else {
					PressedHandle = i;
				}
			}
		}
		//printf("%d\n", PressedHandle);
		if (PressedHandle != -1) {
			mDelta = posValue - getHandleRectWithFloatValue(range[PressedHandle]).center().x();
		}
	}
	m_HoverHandle = PressedHandle;
	update();
}

void igQtDataRangeSlider::mouseMoveEvent(QMouseEvent* aEvent)
{
	if (!(aEvent->buttons() & Qt::LeftButton) || !isPressed || PressedHandle == -1)
	{
		int hover = -1;
		for (int i = 0; i < 2; i++) {
			if (getHandleRectWithFloatValue(range[i]).contains(aEvent->pos())) { hover = i; break; }
		}
		if (hover != m_HoverHandle) { m_HoverHandle = hover; update(); }
		setCursor(hover != -1 ? Qt::SizeHorCursor : Qt::ArrowCursor);
		return;
	}
	{
		int posValue = aEvent->pos().x();
		float value = 0.0;
		if (PressedHandle == 0) {
			value = qBound(0.0, (posValue - mDelta - LeftRightMargin) * 1.0 / sliderBarLength, (double)range[1]);
		}
		else if (PressedHandle == 1) {
			value = qBound((double)range[0], (posValue - mDelta - LeftRightMargin) * 1.0 / sliderBarLength, 1.0);
		}
		range[PressedHandle] = value;

		update();
		Q_EMIT DataRangeChanged(range[0] * (max - min) + min, range[1] * (max - min) + min);
	}
}

void igQtDataRangeSlider::mouseReleaseEvent(QMouseEvent* aEvent)
{
	Q_UNUSED(aEvent);
	this->isPressed = false;
	this->PressedHandle = -1;
	update();
}

void igQtDataRangeSlider::leaveEvent(QEvent* aEvent)
{
	Q_UNUSED(aEvent);
	m_HoverHandle = -1;
	setCursor(Qt::ArrowCursor);
	update();
}