#pragma once
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

namespace iGame {
	class iGameColorMap;
}
class igQtDataRangeSlider : public QWidget
{
	Q_OBJECT

public:
	igQtDataRangeSlider(QWidget* Parent = nullptr);

	void initRange();
	void getRange(float& _min, float& _max) {
		_min = min;
		_max = max;
	}
	void setDrawColor(QColor c1, QColor c2) { this->rangeInColor = c1; this->rangeOutColor = c2; };
	void setDrawInColor(QColor _c) { this->rangeInColor = _c; };
	void setDrawOutColor(QColor _c) { this->rangeOutColor = _c; };
protected:
	void updateColorBarDrawInfo();

	void paintEvent(QPaintEvent* aEvent) override;
	void mousePressEvent(QMouseEvent* aEvent) override;
	void mouseMoveEvent(QMouseEvent* aEvent) override;
	void mouseReleaseEvent(QMouseEvent* aEvent) override;

	QRectF getHandleRect(int handle)const;
	QRectF getHandleRectWithFloatValue(float handle)const;

signals:
	void DataRangeChanged(double, double);
public slots:
	void updateMinAndMax(float, float);

private:
	int sliderBarLength = 0;
	float min = 0.0, max = 0.0;
	double mDelta = 0.0;
	float range[2] = { 0.0,1.0 };
	bool isPressed = false;
	int PressedHandle;
	QColor rangeInColor;
	QColor rangeOutColor;
};

