#ifndef IGQTPREVIEWCOLORAREAWIDGET_H
#define IGQTPREVIEWCOLORAREAWIDGET_H

#include <IQCore/igQtExportModule.h>
#include <QPainter>
#include <QStylePainter>
#include <QWidget>
#include <QColor>
#include <QMouseEvent>
class IG_QT_MODULE_EXPORT  igQtPreviewColorAreaWidget : public QWidget {
	Q_OBJECT

public:
	igQtPreviewColorAreaWidget(QWidget* parent = nullptr);
	~igQtPreviewColorAreaWidget() override;

public:

	void paintEvent(QPaintEvent*) override;
	void resizeEvent(QResizeEvent*) override;
	void paint(QPainter& painter, QRect rect) const;
	void slotSvChanged(QColor);
public slots:
	void setOldColor(const QColor& c);
	void setNewColor(const QColor& c);
signals:
	void  sigSvChanged(QColor);
protected:
	QColor oldColor;
	QColor newColor;
};


#endif //IGQTPREVIEWCOLORAREAWIDGET_H