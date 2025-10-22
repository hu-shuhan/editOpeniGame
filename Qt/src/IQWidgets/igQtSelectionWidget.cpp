#include <IQWidgets/igQtSelectionWidget.h>

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

double igQtSelectionWidget::GetSelectionRadius() const { return m_SelectionRadius; }

bool igQtSelectionWidget::GetSelectionShow() const { return m_SelectionShow; }

bool igQtSelectionWidget::GetSelectOrUnSelect() const { return m_Select_Or_UnSelect; }

int igQtSelectionWidget::GetVariableIndex() const { return m_VariableIndex; }

double igQtSelectionWidget::GetVariableRange() const { return m_VariableRange; }

void igQtSelectionWidget::SetVariableNames(const std::vector<std::string>& variableNames) {
    PreventSignalSend(true);
    ui->variableChoose->clear();
    ui->variableChoose->addItem("Null");
    for (auto& name: variableNames) { ui->variableChoose->addItem(name.c_str()); }
    m_VariableIndex = -1;
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
    m_Select_Or_UnSelect = true;
    if (m_PreventSignalSend) return;
    emit SetSelectionStateChanged();
}

void igQtSelectionWidget::SelectionUnSelect(bool checked) {
    if (!checked) return;
    m_Select_Or_UnSelect = false;
    if (m_PreventSignalSend) return;
    emit SetSelectionStateChanged();
}

void igQtSelectionWidget::SelectionRadiusSpinBox(double radius) {
    m_SelectionRadius = radius;
    if (m_PreventSignalSend) return;
    emit SetSelectionStateChanged();
}

void igQtSelectionWidget::SelectionVariableIndex(int index) {
    m_VariableIndex = std::max<int>(-1, index - 1);
    if (m_PreventSignalSend) return;
    emit SetSelectionStateChanged();
}

void igQtSelectionWidget::SelectionVariableAutoCheck(bool checked) {
    if (checked) {
        m_VariableRange = -1;
        ui->theRange->setEnabled(false);
    } else {
        m_VariableRange = ui->theRange->value();
        ui->theRange->setEnabled(true);
    }
    if (m_PreventSignalSend) return;
    emit SetSelectionStateChanged();
}

void igQtSelectionWidget::SelectionVariableRange(double range) {
    m_VariableRange = range;
    if (m_PreventSignalSend) return;
    emit SetSelectionStateChanged();
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
