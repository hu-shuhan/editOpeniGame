#ifndef IGQTBASICCOLOITEM_H
#define IGQTBASICCOLOITEM_H

#include <QLabel>
#include <IQCore/igQtExportModule.h>
#include <QPainter>
#include <QColor>
class IG_QT_MODULE_EXPORT  igQtBasicColorItem : public QLabel {
	Q_OBJECT

public:
	igQtBasicColorItem(const QColor& color, QWidget* parent = nullptr);
	~igQtBasicColorItem() override;

public:
	QColor getColor();
	void setColor(const QColor& c);
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
signals:
	void sigItemClicked(QColor);
protected:
	QColor myColor;
	bool myMouseIn;
};


#endif //IGQTBASICCOLOITEM_H