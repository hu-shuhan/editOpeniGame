#include <IQWidgets/igQtSelectionWidget.h>
#include <iGameSelectionParameter.h>
igQtSelectionWidget::igQtSelectionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SelectionView)
{
    ui->setupUi(this);
    connect(ui->NONE_SELECTION, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionStationNone);
    connect(ui->POINT_SELECTION, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionStationPoint);
    connect(ui->CELL_SELECTION, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionStationCell);
    connect(ui->Select, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionSelect);
    connect(ui->UnSelect, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionUnSelect);
    connect(ui->RadiusSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &igQtSelectionWidget::SelectionRadiusSpinBox);
    connect(ui->variableChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtSelectionWidget::SelectionVariableIndex);
    connect(ui->autoRange, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionVariableAutoCheck);
    connect(ui->theRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &igQtSelectionWidget::SelectionVariableRange);
    connect(ui->noneSelectionState, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionStateShow);
    connect(ui->clearSelectionState, &QPushButton::clicked, this, &igQtSelectionWidget::ClearSelectionState);
    //ui->noneSelectionState->hide();
}

igQtSelectionWidget::~igQtSelectionWidget()
{
    delete ui; }

const SelectionStation& igQtSelectionWidget::GetSelectionStation() const { return m_SelectionStation; }

bool igQtSelectionWidget::GetSelectionShow() const { return m_SelectionShow; }

void igQtSelectionWidget::SetVariableNames(const std::vector<std::string>& variableNames) {
    PreventSignalSend(true);
    ui->variableChoose->clear();
    ui->variableChoose->addItem("Null");
    for (auto& name: variableNames) { ui->variableChoose->addItem(name.c_str()); }
    iGame::SelectionParameter::Instance().SetSelectVariableIndex(-1);
    PreventSignalSend(false);
}

void igQtSelectionWidget::PreventSignalSend(bool prevent) { m_PreventSignalSend = prevent; }

void igQtSelectionWidget::SetDefaultSelectionButton() {
    ui->NONE_SELECTION->click();
    SetVariableNames({});
}

void igQtSelectionWidget::SelectionStationNone(bool checked) {
    if (!checked) return;
    m_SelectionStation = SelectionStation::NONE_SELECTION;
    if (m_PreventSignalSend) return;
    emit Signal_SetSelectionStationChanged();
}

void igQtSelectionWidget::SelectionStationPoint(bool checked) {
    if (!checked) return;
    m_SelectionStation = SelectionStation::POINT_SELECTION;
    if (m_PreventSignalSend) return;
    emit Signal_SetSelectionStationChanged();
}

void igQtSelectionWidget::SelectionStationCell(bool checked) {
    if (!checked) return;
    m_SelectionStation = SelectionStation::CELL_SELECTION;
    if (m_PreventSignalSend) return;
    emit Signal_SetSelectionStationChanged();
}

void igQtSelectionWidget::SelectionSelect(bool checked) {
    if (!checked) return;
    iGame::SelectionParameter::Instance().SetSelectOrUnSelect(true);
}

void igQtSelectionWidget::SelectionUnSelect(bool checked) {
    if (!checked) return;
    iGame::SelectionParameter::Instance().SetSelectOrUnSelect(false);
}

void igQtSelectionWidget::SelectionRadiusSpinBox(double radius) {
    iGame::SelectionParameter::Instance().SetSelectionRadius(radius);
}

void igQtSelectionWidget::SelectionVariableIndex(int index) {
    iGame::SelectionParameter::Instance().SetSelectVariableIndex(std::max<int>(-1, index - 1));
}

void igQtSelectionWidget::SelectionVariableAutoCheck(bool checked) {
    if (checked) {
        iGame::SelectionParameter::Instance().SetSelectVariableRange(-1);
        ui->theRange->setEnabled(false);
    } else {
        iGame::SelectionParameter::Instance().SetSelectVariableRange(ui->theRange->value());
        ui->theRange->setEnabled(true);
    }
}

void igQtSelectionWidget::SelectionVariableRange(double range) {
    iGame::SelectionParameter::Instance().SetSelectVariableRange(range);
}

void igQtSelectionWidget::ClearSelectionState() {
    if (m_PreventSignalSend) return;
    emit SetClearSelection();
}

void igQtSelectionWidget::SelectionStateShow(bool unShow) {
    auto show = !unShow;
    m_SelectionShow = show;
    if (m_PreventSignalSend) return;
    emit SetSelectionShow(show);
}

void igQtSelectionWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    if (m_PreventSignalSend) return;
    emit Hided();
}
