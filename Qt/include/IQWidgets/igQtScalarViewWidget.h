/**
 * @class   igQtScalarViewWidget
 * @brief   igQtScalarViewWidget's brief
 */

#pragma once
#include <ui_ScalarView.h>
#include <ui_SetCustomScaleRange.h>
#include <QDockWidget>
#include <iGameScalarsToColors.h>
#include <iGameDataObject.h>
class igQtScalarViewWidget : public QWidget {

	Q_OBJECT

public:
	igQtScalarViewWidget(QWidget* parent = nullptr);

	/**
	 * @brief 设置"当前数据对象"（选中优先语义，由模型树侧传入）。
	 *
	 * 传入多块组的子块时，色条按子块自己的属性/配色器显示与调范围；
	 * 未设置（nullptr）时保持既有行为：回落为当前模型的根对象。
	 */
	void setCurrentDataObject(iGame::DataObject::Pointer obj) { m_OverrideDataObject = obj; }


public slots:
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
	void changeEvent(QEvent* e) override;

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
	/** "选中优先"的数据对象（模型树选中子块时由外部设置；nullptr = 回落当前模型根对象） */
	iGame::DataObject::Pointer m_OverrideDataObject;
};
