#pragma once
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include "iGameScalarsToColors.h"
#include "iGameDataObject.h"
class igQtColorBarWidget : public QWidget
{
	Q_OBJECT
public:
	explicit igQtColorBarWidget(QWidget* parent = nullptr);

	void setLimitPosition(int, int, int, int);
	std::string formatNumber(float x);

	/**
	 * @brief 设置"当前数据对象"（选中优先语义，由模型树侧传入）。
	 *
	 * 传入多块组的子块时，色条按子块自己的属性名/配色器/范围显示；
	 * 未设置（nullptr）时保持既有行为：回落为当前模型的根对象。
	 */
	void setCurrentDataObject(iGame::DataObject::Pointer obj) { m_OverrideDataObject = obj; }
protected:
    void updateColorBarDrawInfo();
    void initDrawStringStyle();
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* _event)override;
	void mouseMoveEvent(QMouseEvent* _event)override;
	void mouseReleaseEvent(QMouseEvent* _event)override;
	void changeEvent(QEvent* e) override;

signals:
	void PaintFinished();
public slots:

private:
	bool isPressed = false;
	QPoint lastPos;                 //鼠标按下处坐标
	int limitX1, limitX2, limitY1, limitY2; //窗体限制区域
	int limitHeight=0;
	QVector<QColor> colors;// 颜色数组
	QVector<QString>strData;// 数据数组
	QFont font;//字体
	int colorBarLength = 0;
	int colorBarWidth = 0;
	int fontHeight = 0;
	QColor boundColor = Qt::white;
    iGame::ScalarsToColors::Pointer m_ColorMapper;
	std::string m_AttributeName;  // Current attribute/physical field name
	/** "选中优先"的数据对象（模型树选中子块时由外部设置；nullptr = 回落当前模型根对象） */
	iGame::DataObject::Pointer m_OverrideDataObject;
};

