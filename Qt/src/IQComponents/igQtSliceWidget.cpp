//
// Created by m_ky on 2024/10/12.
//

/**
 * @class   igQtSliceWidget
 * @brief   igQtSliceWidget's brief
 */

#include "IQComponents/igQtSliceWidget.h"

igQtSliceWidget::igQtSliceWidget(QWidget* parent) : QWidget(parent), ui(new Ui::Form){
    ui->setupUi(this);
}

igQtSliceWidget::~igQtSliceWidget() {

}
