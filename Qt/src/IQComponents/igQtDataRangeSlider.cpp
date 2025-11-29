#include "IQComponents/igQtDataRangeSlider.h"
namespace
{
	const int SliderBarHeight = 10;
	const int HandleSideLength = SliderBarHeight + 1;

	const int LeftRightMargin = 10;
	const int TopMargin = 2;
}

igQtDataRangeSlider::igQtDataRangeSlider(QWidget* parent) :QWidget(parent) {
	range[0] = 0.0;
	range[1] = 1.0;
	min = 0.0;
	max = 1.0;
	rangeInColor.setRgbF(0.0, 0.8, 0.8);
	rangeOutColor.setRgbF(0.9, 0.9, 0.9);
	setMouseTracking(true);
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
	QPainter painter(this);
	this->sliderBarLength = this->width() - 2 * LeftRightMargin;
	int st = range[0] * sliderBarLength;
	int ed = range[1] * sliderBarLength;
	for (int i = 0; i < sliderBarLength; i++) {
		if (i >= st && i <= ed) {
			QRect rect(LeftRightMargin + i, TopMargin - 1, 1, SliderBarHeight + 2);
			painter.fillRect(rect, rangeInColor);
		}
		else {
			QRect rect(LeftRightMargin + i, TopMargin, 1, SliderBarHeight);
			painter.fillRect(rect, rangeOutColor);
		}
	}
	QPen pen;
	painter.setRenderHint(QPainter::Antialiasing);
	QBrush handleBrush(QColor(0xFA, 0xFA, 0xFA));
	painter.setBrush(handleBrush);

	for (int i = 0; i < 2; i++) {
		pen.setColor(Qt::darkGray);
		pen.setWidth(static_cast<int>(0.5));
		painter.setPen(pen);
		QRectF HandleRect = getHandleRectWithFloatValue(range[i]);
		painter.drawRoundedRect(HandleRect, 5, 5);
	}

	std::string str;
	float minRange = range[0] * (max - min) + min;
	float maxRange = range[1] * (max - min) + min;
	QFont font = *(new QFont);
	font.setFamily("Microsoft YaHei");
	font.setPointSize(12);
	QFontMetrics fm(font);
	str = "Range:(" + std::to_string(minRange) + ", " + std::to_string(maxRange) + ")\n";
	QRect rect = fm.boundingRect(QString::fromStdString(str));
	int textWidth = rect.width();
	int textHeight = rect.height();
	QRect textRect(LeftRightMargin, 2 * TopMargin + SliderBarHeight + 5, 2 * textWidth, textHeight);
	painter.drawText(textRect, QString::fromStdString(str));

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

}

void igQtDataRangeSlider::mouseMoveEvent(QMouseEvent* aEvent)
{
	if (aEvent->buttons() & Qt::LeftButton && isPressed && PressedHandle != -1)
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
}