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
#include "iGameDataObject.h"
class igQtModelInformationWidget : public QWidget {
public:
	igQtModelInformationWidget(QWidget* parent = nullptr);

public slots:
	void updateInformationFrame();

private:
	QLabel* createLabel(const QString& text);

	void createPropertyLabel(QFormLayout* formLayout, const QString& name, const QString& value);

	QFrame* createSeparator();

	void CreateDataObjectLayoutInfo(iGame::DataObject::Pointer obj, QFormLayout* formLayout);

private:
	QFrame* informationFrame;
	QVBoxLayout* frameLayout;
};