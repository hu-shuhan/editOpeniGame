#include <IQWidgets/igQtAxisAlignedReflectionWidget.h>
#include <ui_igQtAxisAlignedReflection.h>

#include <QComboBox>
#include <QDockWidget>
#include <QPushButton>

igQtAxisAlignedReflectionWidget::igQtAxisAlignedReflectionWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::igQtAxisAlignedReflection) {
    ui->setupUi(this);

    connect(ui->comboBox_Plane, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateCenterEnabled(); });
    connect(ui->pushButton_Apply, &QPushButton::clicked,
            this, &igQtAxisAlignedReflectionWidget::applyRequested);

    resetParameters();
}

igQtAxisAlignedReflectionWidget::~igQtAxisAlignedReflectionWidget() {
    delete ui;
}

QDockWidget* igQtAxisAlignedReflectionWidget::createDockWidget(QWidget* parent) {
    auto* dock = new QDockWidget(QStringLiteral("反射"), parent);
    dock->setObjectName(QStringLiteral("dockWidget_AxisAlignedReflection"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setFeatures(QDockWidget::DockWidgetClosable);
    dock->setWidget(new igQtAxisAlignedReflectionWidget(dock));
    return dock;
}

iGame::AxisAlignedReflectionFilter::Plane igQtAxisAlignedReflectionWidget::plane() const {
    using Plane = iGame::AxisAlignedReflectionFilter::Plane;
    switch (ui->comboBox_Plane->currentIndex()) {
    case 0: return Plane::XMin;
    case 1: return Plane::YMin;
    case 2: return Plane::ZMin;
    case 3: return Plane::XMax;
    case 4: return Plane::YMax;
    case 5: return Plane::ZMax;
    case 6: return Plane::X;
    case 7: return Plane::Y;
    case 8: return Plane::Z;
    default: return Plane::XMin;
    }
}

double igQtAxisAlignedReflectionWidget::center() const {
    return ui->doubleSpinBox_Center->value();
}

bool igQtAxisAlignedReflectionWidget::copyInput() const {
    return ui->checkBox_CopyInput->isChecked();
}

bool igQtAxisAlignedReflectionWidget::flipAllInputArrays() const {
    return ui->checkBox_FlipAllInputArrays->isChecked();
}

void igQtAxisAlignedReflectionWidget::resetParameters() {
    ui->comboBox_Plane->setCurrentIndex(0);
    ui->doubleSpinBox_Center->setValue(0.0);
    ui->checkBox_CopyInput->setChecked(true);
    ui->checkBox_FlipAllInputArrays->setChecked(true);
    updateCenterEnabled();
}

void igQtAxisAlignedReflectionWidget::updateCenterEnabled() {
    ui->doubleSpinBox_Center->setEnabled(ui->comboBox_Plane->currentIndex() >= 6);
}
