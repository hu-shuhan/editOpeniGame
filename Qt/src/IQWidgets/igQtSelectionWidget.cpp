#include <IQWidgets/igQtSelectionWidget.h>
#include <iGameSelectionParameter.h>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
igQtSelectionWidget::igQtSelectionWidget(QWidget* parent) : QWidget(parent), ui(new Ui::SelectionView) {
    ui->setupUi(this);
    connect(ui->NONE_SELECTION, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionStationNone);
    connect(ui->POINT_SELECTION, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionStationPoint);
    connect(ui->CELL_SELECTION, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionStationCell);
    connect(ui->Select, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionSelect);
    connect(ui->UnSelect, &QRadioButton::clicked, this, &igQtSelectionWidget::SelectionUnSelect);
    //############ RadiusSpinBox ############
    QRegularExpression rx("\\d*\\.?\\d+");
    ui->RadiusSpinBox->setValidator(new QRegularExpressionValidator(rx, this));
    connect(ui->RadiusSpinBox, &QLineEdit::textChanged, this, &igQtSelectionWidget::SelectionRadiusSpinBox);

    connect(ui->variableChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtSelectionWidget::SelectionVariableIndex);
    connect(ui->autoRange, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionVariableAutoCheck);
    ui->autoRange->hide();

    //############ skipUnSeeAbleCell ############
    connect(ui->skipUnSeeAbleCell, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionSkipUnSeeAbleCell);
    ui->skipUnSeeAbleCell->setChecked(true);
    iGame::SelectionParameter::Instance().SetSelectIgnoreUnSeeAbleCells(true);

    connect(ui->onlySelectSeeAbleCells, &QCheckBox::clicked, this,
            &igQtSelectionWidget::SelectionOnlySelectSeeAbleCells);
    //ui->onlySelectSeeAbleCells->setChecked(true);
    //iGame::SelectionParameter::Instance().SetSelectOnlySelectSeeAbleCells(true);
    ui->onlySelectSeeAbleCells->hide();

    connect(ui->expdRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &igQtSelectionWidget::SelectionExpdRate);
    ui->expdRate->hide();
    connect(ui->expdRateSlid, &QAbstractSlider::valueChanged, this, &igQtSelectionWidget::SelectionExpdRateSlid);
    SetExpdRateSlidToolTip(ui->expdRateSlid->value());
    connect(ui->noneSelectionState, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectItemShow);
    connect(ui->noneSelectBox, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectBoxShow);
    ui->noneSelectBox->hide();
    connect(ui->clearSelectionState, &QPushButton::clicked, this, &igQtSelectionWidget::ClearSelectionState);
    ui->drawBoundBox->hide();
    //############ BOX ############
    connect(ui->clearBox, &QPushButton::clicked, this, &igQtSelectionWidget::ClearBox);
    connect(ui->settingBox, &QPushButton::clicked, this, &igQtSelectionWidget::BoxSettingDialog);
    //ui->settingBox->hide();
    connect(ui->useBox, &QPushButton::clicked, this, &igQtSelectionWidget::UseBox);
    //############ SELECT MODE ############
    //############ HIDE R MODE ############
    connect(ui->radiusMode, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionRadiusMode);
    connect(ui->ctMode, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionCtMode);
    connect(ui->radiusBoxMode, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionRadiusBoxMode);
    connect(ui->ctBoxMode, &QCheckBox::clicked, this, &igQtSelectionWidget::SelectionCtBoxMode);
    HideSelectionTypeUi();
    HideAllSelectModeUi();
    ShowCtUi();
    ui->radiusMode->hide();
    ui->radiusBoxMode->hide();
    ui->ctMode->setChecked(true);
    iGame::SelectionParameter::Instance().SetSelectMode(iGame::SelectionParameter::SelectMode::CT_MODE);
    //############ Pre Load ############
    connect(ui->preLoadModelMsg, &QPushButton::clicked, this, &igQtSelectionWidget::PreLoadModelMsg);
    ui->preLoadModelMsg->hide();
    //############ R ############
    auto radius = 0.5;
    ui->RadiusSpinBox->setText(QString::number(radius));
    iGame::SelectionParameter::Instance().SetSelectionRadius(radius);
    //############ ATTENTION ############
    SetNoAttention();
}

igQtSelectionWidget::~igQtSelectionWidget() { delete ui; }

bool igQtSelectionWidget::GetSelectItemShow() const { return m_SelectItemShow; }

bool igQtSelectionWidget::GetSelectBoxShow() const { return m_SelectBoxShow; }

void igQtSelectionWidget::SetVariableNames(const std::vector<std::string>& variableNames) {
    PreventSignalSend(true);
    ui->variableChoose->clear();
    ui->variableChoose->addItem("⨀无所选变量");
    for (auto& name: variableNames) { ui->variableChoose->addItem(name.c_str()); }
    iGame::SelectionParameter::Instance().SetSelectVariableIndex(-1);
    PreventSignalSend(false);
}

void igQtSelectionWidget::PreventSignalSend(bool prevent) { m_PreventSignalSend = prevent; }

void igQtSelectionWidget::SetDefaultSelectionButton() {
    ui->NONE_SELECTION->setChecked(false);
    iGame::SelectionParameter::Instance().SetInSelection(false);
    SetVariableNames({});
}

void igQtSelectionWidget::SetInitBoxSettingDialog(QWidget* renderWidget) {
    if (m_BoxSettingDialog == nullptr) {
        m_BoxSettingDialog = new igQtBoxSettingDialog(renderWidget);
    } else {
        m_BoxSettingDialog->ReloadBoxMsg();
    }
    m_BoxSettingDialog->show();
}

void igQtSelectionWidget::SetNoAttention() {
    ui->attention->setText("");
    ui->attention->hide();
}

void igQtSelectionWidget::SetPointAttention() {
    ui->attention->setText(QString("●当前模型无 <font color='red'><b>点</b></font> 数据"));
    ui->attention->show();
}

void igQtSelectionWidget::SetCellAttention() {
    ui->attention->setText(QString("●当前模型无 <font color='red'><b>面/体</b></font> 数据"));
    ui->attention->show();
}

void igQtSelectionWidget::SetAllAttention() {
    ui->attention->setText(
            QString("●当前模型无 <font color='red'><b>点</b></font> 、 <font color='red'><b>面/体</b></font> 数据"));
    ui->attention->show();
}

void igQtSelectionWidget::SelectionStationNone(bool checked) {
    iGame::SelectionParameter::Instance().SetInSelection(checked);
    if (!checked) {
        HideSelectionTypeUi();
    } else {
        ShowSelectionTypeUi();
    }
    //iGame::SelectionParameter::Instance().SetSelectionStation(iGame::SelectionParameter::SelectionStation::NONE_SELECTION);
    //if (!checked) return;
    if (m_PreventSignalSend) return;
    emit Signal_SetSelectionStationChanged();
}

void igQtSelectionWidget::SelectionStationPoint(bool checked) {
    if (!checked) return;
    if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
        iGame::SelectionParameter::SelectionStation::POINT_SELECTION)
        return;
    iGame::SelectionParameter::Instance().SetSelectionStation(iGame::SelectionParameter::SelectionStation::POINT_SELECTION);
    if (m_PreventSignalSend) return;
    emit Signal_SetSelectionStationChanged();
}

void igQtSelectionWidget::SelectionStationCell(bool checked) {
    if (!checked) return;
    if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
        iGame::SelectionParameter::SelectionStation::CELL_SELECTION)
        return;
    iGame::SelectionParameter::Instance().SetSelectionStation(iGame::SelectionParameter::SelectionStation::CELL_SELECTION);
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

void igQtSelectionWidget::SelectionRadiusMode(bool checked) {
    if (!checked) return;
    iGame::SelectionParameter::Instance().SetSelectMode(iGame::SelectionParameter::SelectMode::RADIUS_MODE);
    HideAllSelectModeUi();
    ShowRadiusUi();
}

void igQtSelectionWidget::SelectionCtMode(bool checked) {
    if (!checked) return;
    iGame::SelectionParameter::Instance().SetSelectMode(iGame::SelectionParameter::SelectMode::CT_MODE);
    HideAllSelectModeUi();
    ShowCtUi();
}

void igQtSelectionWidget::SelectionRadiusBoxMode(bool checked) {
    if (!checked) return;
    iGame::SelectionParameter::Instance().SetSelectMode(iGame::SelectionParameter::SelectMode::RADIUS_BOX_MODE);
    HideAllSelectModeUi();
    ShowRadiusUi();
    ShowBoxUi();
}

void igQtSelectionWidget::SelectionCtBoxMode(bool checked) {
    if (!checked) return;
    iGame::SelectionParameter::Instance().SetSelectMode(iGame::SelectionParameter::SelectMode::CT_BOX_MODE);
    HideAllSelectModeUi();
    ShowCtUi();
    ShowBoxUi();
}

void igQtSelectionWidget::SelectionRadiusSpinBox(const QString& radius) {
    iGame::SelectionParameter::Instance().SetSelectionRadius(radius.toDouble());
}

void igQtSelectionWidget::SelectionVariableIndex(int index) {
    iGame::SelectionParameter::Instance().SetSelectVariableIndex(std::max<int>(-1, index - 1));
}

void igQtSelectionWidget::SelectionVariableAutoCheck(bool checked) {
    iGame::SelectionParameter::Instance().SetAutoSelect(checked);
    ui->variableChoose->setEnabled(checked);
    ui->expdRate->setEnabled(checked);
    ui->expdRateSlid->setEnabled(checked);
}

void igQtSelectionWidget::SelectionExpdRate(double rate) {
    iGame::SelectionParameter::Instance().SetAutoSelectExpdRate(rate);
}

void igQtSelectionWidget::SelectionExpdRateSlid(int rate) {
    iGame::SelectionParameter::Instance().SetAutoSelectExpdRate((double) rate / 100.0);
    SetExpdRateSlidToolTip(rate);
}

void igQtSelectionWidget::SelectionSkipUnSeeAbleCell(bool checked) {
    iGame::SelectionParameter::Instance().SetSelectIgnoreUnSeeAbleCells(checked);
}

void igQtSelectionWidget::SelectionOnlySelectSeeAbleCells(bool checked) {
    iGame::SelectionParameter::Instance().SetSelectOnlySelectSeeAbleCells(checked);
}

void igQtSelectionWidget::ClearSelectionState() {
    if (m_PreventSignalSend) return;
    emit SetClearSelection();
}

void igQtSelectionWidget::SelectItemShow(bool unShow) {
    auto show = !unShow;
    m_SelectItemShow = show;
    if (m_PreventSignalSend) return;
    emit SetSelectItemShow(show);
}

void igQtSelectionWidget::SelectBoxShow(bool unShow) {
    auto show = !unShow;
    m_SelectBoxShow = show;
    if (m_PreventSignalSend) return;
    emit SetSelectBoxShow(show);
}

void igQtSelectionWidget::ClearBox() {
    if (m_PreventSignalSend) return;
    if (m_BoxSettingDialog) m_BoxSettingDialog->hide();
    emit SetClearBox();
}

void igQtSelectionWidget::UseBox() {
    if (m_PreventSignalSend) return;
    emit SetUseBox();
}

void igQtSelectionWidget::BoxSettingDialog() { emit SetBoxSettingDialog(); }

void igQtSelectionWidget::PreLoadModelMsg() {
    if (m_PreventSignalSend) return;
    emit SetPreLoadModelMsg();
}

void igQtSelectionWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    if (m_PreventSignalSend) return;
    if (m_BoxSettingDialog) m_BoxSettingDialog->hide();
    emit Hided();
}

void igQtSelectionWidget::SetExpdRateSlidToolTip(int value) {
    //ui->expdRateSlid->setToolTip(QString("取值比例：") + QString::number(value) + QString("%"));
    ui->expdRateSlidTxt->setText(QString::number(value) + QString("%"));
}

void igQtSelectionWidget::HideAllSelectModeUi() {
    ui->radiusLabel->hide();
    ui->vcLabel->hide();
    ui->erLabel->hide();
    ui->RadiusSpinBox->hide();
    ui->variableChoose->hide();
    ui->expdRateSlid->hide();
    ui->expdRateSlidTxt->hide();
    ui->boxLabel->hide();
    ui->clearBox->hide();
    ui->settingBox->hide();
    ui->useBox->hide();
    //ui->attention->hide();
}

void igQtSelectionWidget::HideSelectionTypeUi() {
    ui->opeTypeLabel->hide();
    ui->POINT_SELECTION->hide();
    ui->CELL_SELECTION->hide();
}

void igQtSelectionWidget::ShowSelectionTypeUi() {
    ui->opeTypeLabel->show();
    ui->POINT_SELECTION->show();
    ui->CELL_SELECTION->show();
}

void igQtSelectionWidget::ShowRadiusUi() {
    ui->radiusLabel->show();
    ui->RadiusSpinBox->show();
}

void igQtSelectionWidget::ShowCtUi() {
    ui->radiusLabel->show();
    ui->vcLabel->show();
    //ui->erLabel->show();
    ui->RadiusSpinBox->show();
    ui->variableChoose->show();
    //ui->expdRateSlid->show();
    //ui->expdRateSlidTxt->show();
}

void igQtSelectionWidget::ShowBoxUi() {
    ui->boxLabel->show();
    ui->clearBox->show();
    ui->settingBox->show();
    ui->useBox->show();
}