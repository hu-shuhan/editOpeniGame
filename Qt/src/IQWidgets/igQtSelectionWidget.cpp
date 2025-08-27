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

void igQtSelectionWidget::PreventSignalSend(bool prevent) { m_PreventSignalSend = prevent; }

void igQtSelectionWidget::SetDefaultSelectionButton() { ui->NONE_SELECTION->click(); }

void igQtSelectionWidget::SelectionStationNone(bool checked) {
    if (!checked) return;
    m_SelectionStation = SelectionStation::NONE_SELECTION;
    if (m_PreventSignalSend) return;
    emit SetSelectionStation(SelectionStation::NONE_SELECTION);
}

void igQtSelectionWidget::SelectionStationPoint(bool checked) {
    if (!checked) return;
    m_SelectionStation = SelectionStation::POINT_SELECTION;
    if (m_PreventSignalSend) return;
    emit SetSelectionStation(SelectionStation::POINT_SELECTION);
}

void igQtSelectionWidget::SelectionStationCell(bool checked) {
    if (!checked) return;
    m_SelectionStation = SelectionStation::CELL_SELECTION;
    if (m_PreventSignalSend) return;
    emit SetSelectionStation(SelectionStation::CELL_SELECTION);
}

void igQtSelectionWidget::SelectionSelect(bool checked) {
    if (!checked) return;
    m_Select_Or_UnSelect = true;
    if (m_PreventSignalSend) return;
    emit SetSelectOrUnSelect(m_Select_Or_UnSelect);
}

void igQtSelectionWidget::SelectionUnSelect(bool checked) {
    if (!checked) return;
    m_Select_Or_UnSelect = false;
    if (m_PreventSignalSend) return;
    emit SetSelectOrUnSelect(m_Select_Or_UnSelect);
}

void igQtSelectionWidget::SelectionRadiusSpinBox(double radius) {
    m_SelectionRadius = radius;
    if (m_PreventSignalSend) return;
    emit SetSelectionRadius(radius);
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
