#pragma once
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include "iGameScalarsToColors.h"
class igQtColorBarWidget : public QWidget
{
	Q_OBJECT
public:
	explicit igQtColorBarWidget(QWidget* parent = nullptr);

	void setLimitPosition(int, int, int, int);
	std::string formatNumber(float x);
protected:
    void updateColorBarDrawInfo();
    void initDrawStringStyle();
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* _event)override;
	void mouseMoveEvent(QMouseEvent* _event)override;
	void mouseReleaseEvent(QMouseEvent* _event)override;

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
};

