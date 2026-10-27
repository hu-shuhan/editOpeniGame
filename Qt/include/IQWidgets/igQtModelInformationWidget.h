#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include "iGameDataObject.h"
class igQtModelInformationWidget : public QWidget {
public:
	igQtModelInformationWidget(QWidget* parent = nullptr);

	// 指定信息框显示的数据对象（由 igQtModelDialogWidget 传入"当前选中对象"：
	// 选中多块子块时即子块本身；为空时回落为当前模型的根 DataObject，保持原有行为）。
	void setOverrideDataObject(iGame::DataObject::Pointer obj) { m_OverrideDataObject = obj; }

public slots:
	void updateInformationFrame();

private:
	QLabel* createLabel(const QString& text);

	void createPropertyLabel(QFormLayout* formLayout, const QString& name, const QString& value);

	QFrame* createSeparator();

	void CreateDataObjectLayoutInfo(iGame::DataObject::Pointer obj, QFormLayout* formLayout);

protected:
	void changeEvent(QEvent* e) override;

private:
	QScrollArea* scrollArea;
	QFrame* informationFrame;
	QVBoxLayout* frameLayout;
	int m_tableRow{0};
	iGame::DataObject::Pointer m_OverrideDataObject{nullptr};
};