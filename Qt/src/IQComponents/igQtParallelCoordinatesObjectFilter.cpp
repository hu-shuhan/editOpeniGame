#include <IQComponents/igQtParallelCoordinatesObjectFilter.h>
#include "ui_igQtParallelCoordinatesObjectFilter.h"


igQtParallelCoordinatesObjectFilter::igQtParallelCoordinatesObjectFilter(int number, double maxValue, double minValue,
                                                                         const std::string& variableName,
                                                                         QWidget* parent)
    : m_Number(number), QWidget(parent), ui(new Ui::igQtParallelCoordinatesObjectFilter) {
    ui->setupUi(this);
    ui->maxSpinBox->setMaximum(maxValue);
    ui->maxSpinBox->setMinimum(minValue);
    ui->maxSpinBox->setValue(maxValue);
    ui->minSpinBox->setMaximum(maxValue);
    ui->minSpinBox->setMinimum(minValue);
    ui->minSpinBox->setValue(minValue);
    ui->variableName->setText(variableName.c_str());
    connect(ui->maxSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &igQtParallelCoordinatesObjectFilter::MaxSpinBoxChanged);
    connect(ui->minSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &igQtParallelCoordinatesObjectFilter::MinSpinBoxChanged);
}

igQtParallelCoordinatesObjectFilter::~igQtParallelCoordinatesObjectFilter() {
    delete ui; }

int igQtParallelCoordinatesObjectFilter::GetNumber() const { return m_Number; }

void igQtParallelCoordinatesObjectFilter::MaxSpinBoxChanged(double value) {
    ui->minSpinBox->setMaximum(value);
    emit ChangeMaxValue(m_Number, value);
}

void igQtParallelCoordinatesObjectFilter::MinSpinBoxChanged(double value) {
    ui->maxSpinBox->setMinimum(value);
    emit ChangeMinValue(m_Number, value);
}
