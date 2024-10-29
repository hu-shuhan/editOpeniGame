/**
 * @class   igQtScalarViewWidget
 * @brief   igQtScalarViewWidget's brief
 */

#pragma once
#include <ui_ScalarView.h>
#include <ui_SetCustomScaleRange.h>
#include <QDockWidget>
#include <iGameScalarsToColors.h>
class igQtScalarViewWidget : public QWidget {

	Q_OBJECT

public:
	igQtScalarViewWidget(QWidget* parent = nullptr);


public slots:
	void getScalarsName();
	void showScalarItem();
	void showScalarView();
	void editColorBar();
	void rescaleRange();
	void setCustomScaleRange();
	void showCustomScaleRangeWidget();
	void isShowColorLegend();
    void updateDrawStyle();
	void loadScalarData();
	void initScalarRange();
    void initScalarInfo();
    int getCurrentSelectedScalarIdx();
signals:
	void updateCurrentModelColor();
	void changeColorBarShow();
	void ChangeShowColorManager();
	void UpdateRenderWidget();
protected:

private:
	Ui::ScalarView* ui;
    iGame::ScalarsToColors::Pointer m_ColorMapper;
    iGame::ScalarsToColors::Pointer m_TmpColorMapper = iGame::ScalarsToColors::New();
	QWidget* SetCustomScaleRangeWidget{ nullptr };
	Ui::SetCustomScaleRange* SetCustomScaleRangeUi{ nullptr };
	std::map<std::string, int >scalarInfo;
	int scalarDimension{ -1 };
    int currentSelectedScalarIdx{ -1 };
	std::string scalarName = {"" };
	float scalarMin = 0.0, scalarMax = 1.0;
};
