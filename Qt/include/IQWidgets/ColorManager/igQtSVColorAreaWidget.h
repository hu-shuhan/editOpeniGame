#ifndef IGQTSVCOLORAREAWIDGET_H
#define IGQTSVCOLORAREAWIDGET_H

#include <IQCore/igQtExportModule.h>
#include <QPainter>
#include <QWidget>
#include <QColor>
#include<QMouseEvent>
class IG_QT_MODULE_EXPORT  igQtSVColorAreaWidget : public QWidget {
	Q_OBJECT

public:
	igQtSVColorAreaWidget(QWidget* parent = nullptr);
	~igQtSVColorAreaWidget() override;

public:
	void setHue(int h);
	void setSaturation(int s);
	void setBrightness(int v);
	void setHsv(int h, int s, int v);
	void setColor(const QColor& c);
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	void createVPixmap();
	void createSVPixmap();
	void updateSVPixmap();
	void slotHueChanged(int h);


signals:
	void  sigSvChanged(QColor);

protected:

	int myHue;
	int mySaturation;
	int myBrightness;
	int myAreaWidth;
	QPixmap myVPixmap;
	QPixmap mySVPixmap;
	QColor myColor;
};


#endif //IGQTSVCOLORAREAWIDGET_H