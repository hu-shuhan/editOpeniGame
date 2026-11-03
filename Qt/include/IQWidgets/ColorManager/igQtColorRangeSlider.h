#pragma once
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include "iGameColorMap.h"
#include "iGameDataObject.h"
//namespace iGame {
//	class iGameColorMap;
//}
class igQtColorRangeSlider : public QWidget
{
	Q_OBJECT

public:
	igQtColorRangeSlider(QWidget* Parent = nullptr);

	/**
	 * @brief 设置"当前数据对象"（选中优先语义，由色条管理面板传入）。
	 *
	 * 传入多块组的子块时，色条编辑的绑定/写回都作用在【子块】自己的 ColorMapper；
	 * 未设置（nullptr）时保持既有行为：回落为当前模型的根对象。
	 */
	void setCurrentDataObject(iGame::DataObject::Pointer obj) { m_OverrideDataObject = obj; }

	QSize minimumSizeHint() const override;

	// 从当前场景模型同步色条；无模型/无 ColorMapper 时返回 false（仍会初始化默认临时色条供编辑）
	bool InitColorRangeSlider();
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
	/** 将临时色条写回模型 ColorMapper，并 Modified；失败返回 false */
	bool updataManagerColorBarWithMyCorlorBar();
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
	/** "选中优先"的数据对象（多块组内选中子块时由外部设置；nullptr = 回落当前模型根对象） */
	iGame::DataObject::Pointer m_OverrideDataObject;
};

