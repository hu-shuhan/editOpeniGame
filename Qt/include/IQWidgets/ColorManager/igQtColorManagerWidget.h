#ifndef IGQTCOLORMANAGERWIDGET_H
#define IGQTCOLORMANAGERWIDGET_H

#include <IQCore/igQtExportModule.h>
#include <IQWidgets/ColorManager/igQtBasicColorAreaWidget.h>
#include <IQWidgets/ColorManager/igQtSVColorAreaWidget.h>
#include <IQWidgets/ColorManager/igQtHColorAreaWidget.h>
#include <IQWidgets/ColorManager/igQtPreviewColorAreaWidget.h>
#include <IQWidgets/ColorManager/igQtCustomColorAreaWidget.h>
#include <IQWidgets/ColorManager/igQtColorRangeSlider.h>

#include <QLabel>
#include <QWidget>
#include <QColor>
#include <QPushButton>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPaintEvent>
#include <ui_ColorManager.h>

class IG_QT_MODULE_EXPORT igQtColorManagerWidget : public QWidget {
	Q_OBJECT

public:
	igQtColorManagerWidget(QWidget* parent = nullptr);
	~igQtColorManagerWidget() override;

	/** 从当前场景模型同步色条；无模型时返回 false。 */
	bool syncFromCurrentModel();

public slots:
	void resetColorRange();
	void slotColorItemSel(QColor);
	void slotUpdateEditData(QColor);
	void slotEditChanged(QString);
	void slotEditFinished();
	void slotAddCustomColor();
	void slotOkBtnClicked();
	void slotCancelBtnClicked();
	void slotValueChangedH(int);
	void slotValueChangedS(int);
	void slotValueChangedV(int);
	void slotValueChangedR(int);
	void slotValueChangedG(int);
	void slotValueChangedB(int);
	void slotSetTmpHSVToColor();
	void slotSetTmpRGBToColor();
	bool setCustomColorFromStr();
	void updataManagerColorBar();
	void changeColorMapMode();
	void slotControlPointColor(QColor);

signals:
	void UpdateColorBarFinished();

protected:
	void paintEvent(QPaintEvent* event) override;
	void syncSpinBoxesFromColor(const QColor& c);
	void applyColorBarToModel();

	Ui::ColorManager* ui{nullptr};
	QColor myColor;
	QString customColorStr;
	QColor customColor;
	float tmpRgb[3] = {0, 0, 0};
	float tmpHsv[3] = {0, 0, 0};
	bool m_SyncingSpinBoxes{false};
};

#endif // IGQTCOLORMANAGERWIDGET_H
