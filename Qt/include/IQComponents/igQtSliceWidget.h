/**
 * @class   igQtSliceWidget
 * @brief   igQtSliceWidget's brief
 */

#pragma once

//#include <ui.h>
#include <IQCore/igQtExportModule.h>

#include <ui_Slice.h>
#include <QWidget>

class IG_QT_MODULE_EXPORT igQtSliceWidget : public QWidget{
public:
    igQtSliceWidget(QWidget* parent = nullptr);
    ~igQtSliceWidget() override;

private:
    Ui::Form* ui;
};

