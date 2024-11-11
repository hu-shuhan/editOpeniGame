#pragma once
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include "iGameColorMap.h"
//namespace iGame {
//	class iGameColorMap;
//}
class igQtColorRangeSlider : public QWidget
{
	Q_OBJECT

public:
	igQtColorRangeSlider(QWidget* Parent = nullptr);

	QSize minimumSizeHint() const override;

	//	Obtain Color Bar from the scene, then init slider
	void InitColorRangeSlider();
	//  Obtain Color Bar from the input, then update slider
	void UpdateSliderWithColorBar(iGame::ColorMap::Pointer);

protected:
	void updateSliderDrawInfo();

	void paintEvent(QPaintEvent* aEvent) override;
	void mousePressEvent(QMouseEvent* aEvent) override;
	void mouseMoveEvent(QMouseEvent* aEvent) override;
	void mouseReleaseEvent(QMouseEvent* aEvent) override;

	QRectF getHandleRect(int handle)const;
	QRectF getHandleRectWithFloatValue(float handle)const;

signals:
	void rangeChanges();
	void MouseReleased(QColor);
public slots:
	void updateColorInIndex(QColor);
	void updataManagerColorBarWithMyCorlorBar();
	void changeColorBarWithDefaultMode(int);
private:
	int colorBarLength;
	int colorBarHeight;
	QVector<QColor> drawColors;// 颜色数组
    iGame::ColorMap::Pointer m_ColorMapper{nullptr};
    iGame::ColorMap::Pointer m_TmpColorMapper{nullptr};
	int PressedHandle = -1;
	bool isPressed = false;
	int mDelta;
	bool RealUpdate = false;
};

