#ifndef IGQTHCOLORAREAWIDGET_H
#define IGQTHCOLORAREAWIDGET_H

#include <IQCore/igQtExportModule.h>
#include <QPainter>
#include <QWidget>
#include <QColor>
#include <QMouseEvent>
class IG_QT_MODULE_EXPORT  igQtHColorAreaWidget : public QWidget {
	Q_OBJECT

public:
	igQtHColorAreaWidget(QWidget* parent = nullptr);
	~igQtHColorAreaWidget() override;

public:
	void setHue(int h);
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
	void createHuePixmap();

signals:
	void sigHueChanged(int m_iHue);
protected:
	double doubleHue;
	int intHue;
	int colorHeight;
	int colorWidth;
	int topMargin = 10;
	QPixmap huePixmap;
};


#endif //IGQTHCOLORAREAWIDGET_H