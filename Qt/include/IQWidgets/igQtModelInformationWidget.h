#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QFrame>
#include <QGridLayout>
#include "iGameDataObject.h"
class igQtModelInformationWidget : public QWidget {
public:
	igQtModelInformationWidget(QWidget* parent = nullptr);

public slots:
	void updateInformationFrame();

private:
	QLabel* createLabel(const QString& text);

	QLabel* createPropertyLabel(const QString& name, const QString& value);

	QFrame* createSeparator();

	void CreateDataObjectLayoutInfo(iGame::DataObject::Pointer obj);

private:
	QFrame* informationFrame;
	QVBoxLayout* frameLayout;
};