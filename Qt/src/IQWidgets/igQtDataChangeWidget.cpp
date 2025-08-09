#include "ui_igQtDataChangeWidget.h"
#include <IQWidgets/igQtDataChangeWidget.h>
#include <QElapsedTimer>
#include <utility>

using namespace std;

static constexpr int SATURATION = 160;
static constexpr int defaultW = 600, defaultH = 600;
static constexpr double boundaryRatio = 0.05;
static constexpr double edBoundryRatio = 1.0 - boundaryRatio;
static constexpr int FULL_ALPHA = 255;
static constexpr int POINT_SIZE = 6;
static constexpr int COLOR_WIDGET_SIZE = 15;
static constexpr int MIN_H = 0, MAX_H = 360, MIN_S = 100, MAX_S = 255;

static enum VariableSite : int { variableColor = 0, choosedVariableColor, checkButton };

static inline QColor GetQColorFromTuple(const tuple<int, int, int>& rgb, int alpha) {
    return QColor(get<0>(rgb), get<1>(rgb), get<2>(rgb), alpha);
}

static QRect InsetRectByBoundaryRatio(const QRect& rect, double boundaryRatio) {
    int width = rect.width();
    int height = rect.height();

    double insetX = static_cast<double>(width) * boundaryRatio;
    double insetY = static_cast<double>(height) * boundaryRatio;

    int insetXInt = static_cast<int>(std::round(insetX));
    int insetYInt = static_cast<int>(std::round(insetY));

    int newLeft = rect.left() + insetXInt;
    int newTop = rect.top() + insetYInt;

    int newWidth = std::max(0, width - 2 * insetXInt);
    int newHeight = std::max(0, height - 2 * insetYInt);

    return QRect(newLeft, newTop, newWidth, newHeight);
}

static double CalculateValueByPos(double pos, double minPos, double maxPos, double minValue, double maxValue) {
    if (minPos == maxPos) return (minValue + maxValue) / 2;
    if (minValue == maxValue) return minValue;
    return (pos - minPos) * (maxValue - minValue) / (maxPos - minPos) + minValue;
}

static Point CalculateDistancePoint(const Point& startPoint, const Point& endPoint, double distance) {
    double length = (startPoint - endPoint).length();
    float X = CalculateValueByPos(distance, 0, length, startPoint[0], endPoint[0]);
    float Y = CalculateValueByPos(distance, 0, length, startPoint[1], endPoint[1]);
    float Z = CalculateValueByPos(distance, 0, length, startPoint[2], endPoint[2]);
    return {X, Y, Z};
}

static inline int CalculateSite(double data, double maxData, double minData, int start, int end) {
    if (start == end) return start;
    if (minData == maxData) return (start + end) / 2;
    return ((double) start * (maxData - data) - (double) end * (minData - data)) / (maxData - minData);
}

static inline pair<int, int> CalculatePointSite(double yVariableData, double xVariableData,
                                                double yVariableMaxData, double yVariableMinData,
                                                double xVariableMaxData, double xVariableMinData,
                                                const QRect& drawFrame) {
    int ySite = CalculateSite(yVariableData, yVariableMaxData, yVariableMinData, drawFrame.bottom(),
                                 drawFrame.top());
    int xSite =
            CalculateSite(xVariableData, xVariableMaxData, xVariableMinData, drawFrame.left(), drawFrame.right());
    return {xSite, ySite};
}

igQtDataChangeWidget_CheckBox::igQtDataChangeWidget_CheckBox(QWidget* parent) : QCheckBox(parent) {}

igQtDataChangeWidget_ColorWidget::igQtDataChangeWidget_ColorWidget(QWidget* parent) : QWidget(parent) {
    this->setMinimumSize(COLOR_WIDGET_SIZE, COLOR_WIDGET_SIZE);
    this->setMaximumSize(COLOR_WIDGET_SIZE, COLOR_WIDGET_SIZE);
}

void igQtDataChangeWidget_ColorWidget::paintEvent(QPaintEvent* QPE) {
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::Black, 1));
    painter.setBrush(GetQColorFromTuple(m_Color, FULL_ALPHA));
    painter.drawRect(this->rect());
}

igQtDataChangeWidget::igQtDataChangeWidget(QWidget* parent) : QWidget(parent), ui(new Ui::igQtDataChangeWidget) {
    ui->setupUi(this);
    m_RadialStyle = RadialStyle::New();
    connect(ui->choosedLightSlider, &QSlider::valueChanged, this, &igQtDataChangeWidget::ChoosedLightSliderChanged);
    connect(ui->choosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtDataChangeWidget::ChoosedLightSpinBoxChanged);
    connect(ui->unChoosedLightSlider, &QSlider::valueChanged, this, &igQtDataChangeWidget::UnChoosedLightSliderChanged);
    connect(ui->unChoosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtDataChangeWidget::UnChoosedLightSpinBoxChanged);
    connect(ui->dataTypeChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtDataChangeWidget::DataChooseChanged);
    connect(ui->refreshData, &QPushButton::clicked, this, &igQtDataChangeWidget::RefreshData);
    connect(ui->dataGetTool, &QCheckBox::clicked, this, &igQtDataChangeWidget::DataGetToolClicked);
    connect(ui->draw, &QPushButton::clicked, this, &igQtDataChangeWidget::TempSlot_SetRadialData);
    connect(ui->rangeChoose, &QCheckBox::clicked, this, &igQtDataChangeWidget::RangeChooseButtonClicked);
    setMouseTracking(true);
    ui->drawWidget->installEventFilter(this);
    ui->drawWidget->setMouseTracking(true);
}

igQtDataChangeWidget::~igQtDataChangeWidget() { delete ui; }

void igQtDataChangeWidget::InitRadialStyle(SmartPointer<Interactor> interactor) {
    m_RadialStyle->Initialize(interactor);
}

RadialStyle::Pointer igQtDataChangeWidget::GetRadialStyle() { return m_RadialStyle; }

void igQtDataChangeWidget::SetModel(Model::Pointer model) {
    m_Model = model;
    m_Mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    SetRadialPoint();
    /*ShowRadial(true);*/
    SetRadialPointMoveCallBack();
    DrawRadial();
    SetSelectionCallback();
    SetClearSelectionCallback();
    GenerateDataChangeDatas();
    SetDataTypeChoose();
    SetUiData();
    ClearVariableChoose();
    ResetVariableButton();
    ResetVariableImage();
    GenerateBackgroundColor();
}

void igQtDataChangeWidget::SetInteractorName(const std::string& name) { m_RadialIntName = name; }

const std::string& igQtDataChangeWidget::GetInteractorName() { return m_RadialIntName; }

void igQtDataChangeWidget::RangeChooseObj(const QRect& chooseRange, const QRect& frameRange, std::vector<igIndex>& ids,
                                          IGenum& type) {
    //init
    type = {};

    //set
    QRect overLapRange = frameRange.intersected(chooseRange);
    if (overLapRange.right() <= overLapRange.left() || overLapRange.bottom() <= overLapRange.top()) return;
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    type = Data->GetDataType();

    //range
    double minDistance = CalculateValueByPos(overLapRange.left(), frameRange.left(), frameRange.right(),
                                             Data->GetMinDistance(), Data->GetMaxDistance());
    double maxDistance = CalculateValueByPos(overLapRange.right(), frameRange.left(), frameRange.right(),
                                             Data->GetMinDistance(), Data->GetMaxDistance());
    double minValue = CalculateValueByPos(overLapRange.bottom(), frameRange.bottom(), frameRange.top(),
                                          Data->GetMinValue(), Data->GetMaxValue());
    double maxValue = CalculateValueByPos(overLapRange.top(), frameRange.bottom(), frameRange.top(),
                                          Data->GetMinValue(), Data->GetMaxValue());

    //choose
    auto& objIds = Data->GetObjIndexs();
    for (auto& objId_: objIds) {
        auto& objId = objId_.first;
        auto& objIndex = objId_.second;
        auto& objDistance = Data->GetObjDistance()[objIndex];
        auto& objValues = Data->GetObjectDatas()[objIndex];
        if (objDistance < minDistance || maxDistance < objDistance) continue;
        for (int variableIndex = 0; variableIndex < Data->GetVariableNum(); variableIndex++) {
            if (!m_VariableShow[variableIndex]) continue;
            if (minValue <= objValues[variableIndex] && objValues[variableIndex] <= maxValue) {
                ids.push_back(objId);
                break;
            }
        }
    }
}

void igQtDataChangeWidget::EndRangeChoose() {
    if (!m_RangeChoosing) return;
    m_RangeChoosing = false;
    if (!m_RangeChooseOn) return;
    QRect chooseRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint);
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    std::vector<igIndex> ids;
    IGenum type{};
    RangeChooseObj(chooseRect, smallDrawFrame, ids, type);
    auto events = Selection::GenerateEvents(ids, type, Selection::Event::Add, m_Mesh->GetPoints().get(),
                                            m_Mesh->GetCells().get(), m_Model->GetPainter3D().get());
    m_Model->GetSelection()->SelectionCallBackEvent(events);
    update();
}

void igQtDataChangeWidget::StartRangeChoose(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    if (!bigDrawFrame.contains(pos)) return;
    m_RangeChoosing = true;
    m_RangeChooseStartPoint = pos;
    m_RangeChooseEndPoint = pos;
}

void igQtDataChangeWidget::MoveRangeChooseEndPoint(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    int x = max(bigDrawFrame.left(), min(pos.x(), bigDrawFrame.right()));
    int y = max(bigDrawFrame.top(), min(pos.y(), bigDrawFrame.bottom()));
    m_RangeChooseEndPoint = {x, y};
    update();
}

void igQtDataChangeWidget::DrawRangeChooseRect() {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::DarkMagenta, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint));
}

void igQtDataChangeWidget::RangeChooseButtonClicked(bool checked) { m_RangeChooseOn = checked; }

void igQtDataChangeWidget::mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);
    StartRangeChoose(event->pos());
}

void igQtDataChangeWidget::mouseReleaseEvent(QMouseEvent* event) {
    QWidget::mouseReleaseEvent(event);
    EndRangeChoose();
}

void igQtDataChangeWidget::paintEvent(QPaintEvent* QPE) {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    Draw();
    DrawRangeChooseRect();
}

void igQtDataChangeWidget::mouseMoveEvent(QMouseEvent* event) {
    handleMouseMove(event->pos());
    QWidget::mouseMoveEvent(event);
}

bool igQtDataChangeWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        QPoint parentPos = static_cast<QWidget*>(watched)->mapTo(this, mouseEvent->pos());
        handleMouseMove(parentPos);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void igQtDataChangeWidget::handleMouseMove(const QPoint& pos) {
    static QElapsedTimer timer;
    if (!timer.isValid() || timer.elapsed() >= 50) {
        ClearPosLabel();
        SetPosLabel(pos.x(), pos.y());
        MoveRangeChooseEndPoint(pos);
        timer.start();
    }
}

void igQtDataChangeWidget::ClearPosLabel() {
    ui->value_data->setText("");
    ui->value_x->setText("");
    ui->value_y->setText("");
    ui->value_z->setText("");
}

void igQtDataChangeWidget::SetPosLabel(int x, int y) {
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    if (!smallDrawFrame.contains(x, y)) return;
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    double value = CalculateValueByPos(y, smallDrawFrame.bottom(), smallDrawFrame.top(), Data->GetMinValue(),
                                       Data->GetMaxValue());
    ui->value_data->setNum(value);
    double distance = CalculateValueByPos(x, smallDrawFrame.left(), smallDrawFrame.right(), Data->GetMinDistance(),
                                          Data->GetMaxDistance());
    auto point = CalculateDistancePoint(m_RadialStyle->GetStartPoint(), m_RadialStyle->GetEndPoint(), distance);
    ui->value_x->setNum(point[0]);
    ui->value_y->setNum(point[1]);
    ui->value_z->setNum(point[2]);
}

void igQtDataChangeWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    emit Hided();
}

void igQtDataChangeWidget::SetRadialPoint() {
    auto& box = m_Mesh->GetBoundingBox();
    auto dir = (box.max - box.min) * 0.1;
    m_RadialStyle->SetPoint(box.min - dir, box.max + dir);
}

void igQtDataChangeWidget::ShowRadial(bool show) { m_RadialStyle->SetShow(show); }

void igQtDataChangeWidget::DrawRadial() {
    m_RadialStyle->ClearDrawRadial();
    m_RadialStyle->DrawRadial();
}

void igQtDataChangeWidget::GenerateBackgroundColor() { m_BackgroundColor = {255, 255, 255}; }

void igQtDataChangeWidget::SetUiData() {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    _SetLightUi(Data->GetUnChoosedLight(), Data->GetChoosedLight());
    _ClearVariableButton();
    _SetVariableButton(Data->GetVariableNum(), Data->GetVariableName());
    _ClearVariableColorWidget();
    _SetVariableColorWidget(Data->GetVariableNum(), Data->GetVariableColor());
    _ClearChoosedVariableColorWidget();
    _SetChoosedVariableColorWidget(Data->GetVariableNum(), Data->GetChoosedVariableColor());
}

void igQtDataChangeWidget::SetDataTypeChoose() {
    ui->dataTypeChoose->clear();
    if (m_DataChangeDatas.size() == 0) {
        ui->dataTypeChoose->hide();
        return;
    }
    ui->dataTypeChoose->show();
    for (auto& Data: m_DataChangeDatas) { ui->dataTypeChoose->addItem(Data->GetDataTypeName().c_str()); }
}

void igQtDataChangeWidget::ClearVariableChoose() {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    m_VariableShow = std::vector<bool>(Data->GetVariableNum(), false);
}

void igQtDataChangeWidget::ResetVariableButton() {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    _ClearVariableButton();
    _SetVariableButton(Data->GetVariableNum(), Data->GetVariableName());
    _ClearVariableColorWidget();
    _SetVariableColorWidget(Data->GetVariableNum(), Data->GetVariableColor());
    _ClearChoosedVariableColorWidget();
    _SetChoosedVariableColorWidget(Data->GetVariableNum(), Data->GetChoosedVariableColor());
}

void igQtDataChangeWidget::ResetVariableImage() {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    _ResetVariableImage(Data->GetVariableNum());
}

void igQtDataChangeWidget::GenerateVariableImage() {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    _GenerateVariableImage(m_VariableShow, Data);
}

void igQtDataChangeWidget::GenerateChoosedVariableImage() {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    _GenerateChoosedVariableImage(m_VariableShow, Data);
}

void igQtDataChangeWidget::UpdateChoosedData(const std::vector<Selection::Event>& _events) {
    for (auto& Data: m_DataChangeDatas) {
        for (auto& e: _events) {
            switch (e.type) {
                case Selection::Event::Type::PickPoint:
                    if (Data->GetDataType() != IG_POINT) break;
                    _TryUpdateChoosedPointData(Data, e.pickId, e.operate);
                    break;
                case Selection::Event::Type::PickFace:
                    if (Data->GetDataType() != IG_CELL) break;
                    _TryUpdateChoosedCellData(Data, e.pickId, e.operate);
                    break;
                default:
                    break;
            }
        }
    }
}

void igQtDataChangeWidget::ClearChoosedData() {
    for (auto& Data: m_DataChangeDatas) Data->ClearChoosedObjIds();
}

void igQtDataChangeWidget::GenerateDataChangeDatas() {
    m_DataChangeDatas.clear();
    m_CurrentModelDataIndex = -1;
    auto pointData = _GenerateDataChangeDatas(IG_POINT);
    if (pointData.IsNotNull()) { m_DataChangeDatas.push_back(pointData); }
    auto cellData = _GenerateDataChangeDatas(IG_CELL);
    if (cellData.IsNotNull()) { m_DataChangeDatas.push_back(cellData); }
    if (m_DataChangeDatas.size() != 0) m_CurrentModelDataIndex = 0;
}

void igQtDataChangeWidget::SetRadialData() {
    for (auto& Data: m_DataChangeDatas) { _SetRadialData(Data); }
}

void igQtDataChangeWidget::Draw() {
    QRect bigDrawFrame, smallDrawFrame;
    _CalculatePaintDrawFrame(bigDrawFrame, smallDrawFrame);
    _DrawBackground(bigDrawFrame);
    _DrawCoordinateRect(smallDrawFrame);
    _DrawImages(bigDrawFrame);
}

DataChangeData::Pointer igQtDataChangeWidget::_GenerateDataChangeDatas(IGenum dataType) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    auto colorMap = m_Mesh->GetColorMapper();
    auto& selectedItems = m_Model->GetSelection()->GetSelectedItems();
    int objNum{};
    if (dataType == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    auto variableNames = DataChangeData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return DataChangeData::Pointer();
    auto Data = DataChangeData::New();
    Data->SetVariableNum(variableNum);
    Data->SetVariableName(variableNames);
    auto variableIndex = DataChangeData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto [minValue, maxValue] = DataChangeData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    Data->SetDataType(dataType);
    Data->SetDataTypeName(DataChangeData::GenerateDataTypeName(dataType));
    //auto hue = DataChangeData::GenerateVariableHue(variableNum);
    //Data->SetVariableHue(hue);
    auto hs = DataChangeData::GenerateHS(variableNum, MIN_H, MAX_H, MIN_S, MAX_S);
    Data->SetVariableHS(hs);
    //auto variableColor = DataChangeData::GenerateVariableColor(hue, SATURATION, Data->GetUnChoosedLight());
    auto variableColor = DataChangeData::GenerateVariableColor(hs, Data->GetUnChoosedLight());
    Data->SetVariableColor(variableColor);
    //auto choosedVariableColor = DataChangeData::GenerateVariableColor(hue, SATURATION, Data->GetChoosedLight());
    auto choosedVariableColor = DataChangeData::GenerateVariableColor(hs, Data->GetChoosedLight());
    Data->SetChoosedVariableColor(choosedVariableColor);
    return Data;
}

void igQtDataChangeWidget::_SetRadialData(DataChangeData::Pointer Data) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    auto colorMap = m_Mesh->GetColorMapper();
    auto& selectedItems = m_Model->GetSelection()->GetSelectedItems();
    int objNum{};
    if (Data->GetDataType() == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    auto& startPoint = m_RadialStyle->GetStartPoint();
    auto& endPoint = m_RadialStyle->GetEndPoint();
    auto objIndexs = DataChangeData::GenerateObjIndex(startPoint, endPoint, m_Mesh->GetPoints(), m_Mesh->GetCells(),
                                                      m_Mesh, Data->GetDataType());
    Data->SetObjIndexs(objIndexs);
    auto objData = DataChangeData::GenerateObjectDatas(attrs, Data->GetDataType(), objIndexs);
    Data->SetObjectDatas(objData);
    auto choosedObjIds = DataChangeData::GenerateChoosedObjIds(selectedItems, Data->GetDataType(), objIndexs);
    Data->SetChoosedObjIds(choosedObjIds);
    std::vector<double> objDistance;
    if (Data->GetDataType() == IG_POINT) {
        objDistance = DataChangeData::GenerateObjDistance(startPoint, objIndexs, m_Mesh->GetPoints());
    } else {
        objDistance =
                DataChangeData::GenerateObjDistance(startPoint, objIndexs, m_Mesh->GetCells(), m_Mesh->GetPoints());
    }
    Data->SetObjDistance(objDistance);
    auto objDrawSort = DataChangeData::GenerateObjDrawSort(objDistance, objIndexs);
    Data->SetObjDrawSort(objDrawSort);
    auto maxDistance = DataChangeData::GenerateObjMaxDistance(objDrawSort, objDistance, objIndexs);
    Data->SetMaxDistance(maxDistance);
    auto minDistance = DataChangeData::GenerateObjMinDistance(objDrawSort, objDistance, objIndexs);
    Data->SetMinDistance(minDistance);
}

void igQtDataChangeWidget::_ClearVariableButton() {
    for (auto& checkBox: m_VariableCheckBoxs) { checkBox->deleteLater(); }
    m_VariableCheckBoxs.clear();
}

void igQtDataChangeWidget::_SetVariableButton(int variableNum, const std::vector<std::string>& variableName) {
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        auto checkButton = new igQtDataChangeWidget_CheckBox(this);
        checkButton->setText(variableName[variableIndex].c_str());
        checkButton->m_VariableIndex = variableIndex;
        connect(checkButton, &igQtDataChangeWidget_CheckBox::clicked, this,
                &igQtDataChangeWidget::VariableCheckButtonClicked);
        m_VariableCheckBoxs.push_back(checkButton);
        ui->variable->addWidget(checkButton, variableIndex, VariableSite::checkButton);
    }
}

void igQtDataChangeWidget::_ClearVariableColorWidget() {
    for (auto& colorWidget: m_VariableColorWidgets) { colorWidget->deleteLater(); }
    m_VariableColorWidgets.clear();
}

void igQtDataChangeWidget::_SetVariableColorWidget(int variableNum,
                                                   const std::vector<std::tuple<int, int, int>>& variableColor) {
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        auto colorWidget = new igQtDataChangeWidget_ColorWidget(this);
        colorWidget->m_Color = variableColor[variableIndex];
        m_VariableColorWidgets.push_back(colorWidget);
        ui->variable->addWidget(colorWidget, variableIndex, VariableSite::variableColor);
    }
}

void igQtDataChangeWidget::_SetVariableColorWidgetColor(int variableNum,
                                                        const std::vector<std::tuple<int, int, int>>& variableColor) {
    if (m_VariableColorWidgets.size() != variableNum) return;
    if (variableColor.size() != variableNum) return;
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        auto colorWidget = m_VariableColorWidgets[variableIndex];
        colorWidget->m_Color = variableColor[variableIndex];
        colorWidget->update();
    }
}

void igQtDataChangeWidget::_ClearChoosedVariableColorWidget() {
    for (auto& colorWidget: m_ChoosedVariableColorWidgets) { colorWidget->deleteLater(); }
    m_ChoosedVariableColorWidgets.clear();
}

void igQtDataChangeWidget::_SetChoosedVariableColorWidget(int variableNum,
                                                          const std::vector<std::tuple<int, int, int>>& variableColor) {
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        auto colorWidget = new igQtDataChangeWidget_ColorWidget(this);
        colorWidget->m_Color = variableColor[variableIndex];
        m_ChoosedVariableColorWidgets.push_back(colorWidget);
        ui->variable->addWidget(colorWidget, variableIndex, VariableSite::choosedVariableColor);
    }
}

void igQtDataChangeWidget::_SetChoosedVariableColorWidgetColor(
        int variableNum, const std::vector<std::tuple<int, int, int>>& variableColor) {
    if (m_ChoosedVariableColorWidgets.size() != variableNum) return;
    if (variableColor.size() != variableNum) return;
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        auto colorWidget = m_ChoosedVariableColorWidgets[variableIndex];
        colorWidget->m_Color = variableColor[variableIndex];
        colorWidget->update();
    }
}

void igQtDataChangeWidget::_GenerateVariableImage(int variableIndex, DataChangeData::Pointer Data) {
    if (m_VariableShow[variableIndex] == false) return;
    auto minValue = Data->GetMinValue();
    auto maxValue = Data->GetMaxValue();
    m_VariableImages[variableIndex] = _DrawVariableImage(
            minValue, maxValue, Data->GetMinDistance(), Data->GetMaxDistance(), Data->GetObjDrawSort(), variableIndex,
            Data->GetObjDistance(), Data->GetObjectDatas(), Data->GetVariableColor()[variableIndex], FULL_ALPHA,
            Data->GetChoosedObjIds(), Data->GetObjIndexs());
}

void igQtDataChangeWidget::_GenerateChoosedVariableImage(int variableIndex, DataChangeData::Pointer Data) {
    if (m_VariableShow[variableIndex] == false) return;
    auto minValue = Data->GetMinValue();
    auto maxValue = Data->GetMaxValue();
    m_ChoosedVariableImages[variableIndex] = _DrawChoosedVariableImage(
            minValue, maxValue, Data->GetMinDistance(), Data->GetMaxDistance(), Data->GetObjDrawSort(), variableIndex,
            Data->GetObjDistance(), Data->GetObjectDatas(), Data->GetChoosedVariableColor()[variableIndex], FULL_ALPHA,
            Data->GetChoosedObjIds(), Data->GetObjIndexs());
}

void igQtDataChangeWidget::_GenerateVariableImage(const std::vector<bool>& variableShow, DataChangeData::Pointer Data) {
    for (int variableIndex = 0; variableIndex < Data->GetVariableNum(); variableIndex++) {
        _GenerateVariableImage(variableIndex, Data);
    }
}

void igQtDataChangeWidget::_GenerateChoosedVariableImage(const std::vector<bool>& variableShow,
                                                         DataChangeData::Pointer Data) {
    for (int variableIndex = 0; variableIndex < Data->GetVariableNum(); variableIndex++) {
        _GenerateChoosedVariableImage(variableIndex, Data);
    }
}

void igQtDataChangeWidget::_ResetVariableImage(int variableNum) {
    m_VariableImages = std::vector<QImage>(variableNum, QImage());
    m_ChoosedVariableImages = std::vector<QImage>(variableNum, QImage());
}

void igQtDataChangeWidget::_SetLightUi(int unchoosedLight, int choosedLight) {
    ui->unChoosedLightSlider->setValue(unchoosedLight);
    ui->unChoosedLightSpinBox->setValue(unchoosedLight);
    ui->choosedLightSlider->setValue(choosedLight);
    ui->choosedLightSpinBox->setValue(choosedLight);
}

void igQtDataChangeWidget::_TryUpdateChoosedPointData(DataChangeData::Pointer Data, int id,
                                                      Selection::Event::Operate ope) {
    if (Data.IsNull() || Data->GetDataType() != IG_POINT) return;
    auto& objIds = Data->GetObjIndexs();
    if (objIds.count(id) == 0) return;
    if (ope == Selection::Event::Operate::Add) {
        Data->AddChoosedObjId(id);
    } else if (ope == Selection::Event::Operate::Remove) {
        Data->RemoveChoosedObjId(id);
    }
}

void igQtDataChangeWidget::_TryUpdateChoosedCellData(DataChangeData::Pointer Data, int id,
                                                     Selection::Event::Operate ope) {
    if (Data.IsNull() || Data->GetDataType() != IG_CELL) return;
    auto& objIds = Data->GetObjIndexs();
    if (objIds.count(id) == 0) return;
    if (ope == Selection::Event::Operate::Add) {
        Data->AddChoosedObjId(id);
    } else if (ope == Selection::Event::Operate::Remove) {
        Data->RemoveChoosedObjId(id);
    }
}

void igQtDataChangeWidget::_CalculateDrawFrame(int w, int h, QRect& drawFrame) {
    drawFrame = InsetRectByBoundaryRatio(QRect(0, 0, w, h), boundaryRatio);
}

void igQtDataChangeWidget::_CalculatePaintDrawFrame(QRect& bigDrawFrame, QRect& smallDrawFrame) {
    QRect drawWidgetRect = ui->drawWidget->rect();
    drawWidgetRect.moveTo(ui->drawWidget->mapTo(this, QPoint(0, 0)));
    bigDrawFrame = drawWidgetRect;
    smallDrawFrame = InsetRectByBoundaryRatio(drawWidgetRect, boundaryRatio);
}

QImage igQtDataChangeWidget::_DrawVariableImage(double minValue, double maxValue, double minDistance,
                                                double maxDistance, const std::vector<int>& objDrawSort,
                                                int variableIndex, const std::vector<double>& objDistance,
                                                const std::vector<std::vector<double>>& objData,
                                                const std::tuple<int, int, int>& color, int alpha,
                                                const std::set<int> choosedObjIds,
                                                const std::map<int, int>& objIndexs) {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return {};
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    int objNum = Data->GetObjIndexs().size();
    //int w = max(1000, defaultW / max(objNum / 1000, 1));
    //int h = max(1000, defaultH / max(objNum / 1000, 1));
    auto w = defaultW;
    auto h = defaultH;
    QRect drawFrame;
    _CalculateDrawFrame(w, h, drawFrame);
    QImage re = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    re.fill(Qt::transparent);
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&re);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    auto pen = QPen(QColor(GetQColorFromTuple(color, alpha)), POINT_SIZE);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    //Draw
    for (auto& objId: objDrawSort) {
        if (choosedObjIds.count(objId) != 0) continue;
        auto objIndex = objIndexs.at(objId);
        _DrawPoint(minValue, maxValue, minDistance, maxDistance, objData[objIndex][variableIndex],
                   objDistance[objIndex], painter, drawFrame);
    }
    return re;
}

QImage igQtDataChangeWidget::_DrawChoosedVariableImage(double minValue, double maxValue, double minDistance,
                                                       double maxDistance, const std::vector<int>& objDrawSort,
                                                       int variableIndex, const std::vector<double>& objDistance,
                                                       const std::vector<std::vector<double>>& objData,
                                                       const std::tuple<int, int, int>& color, int alpha,
                                                       const std::set<int> choosedObjIds,
                                                       const std::map<int, int>& objIndexs) {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return {};
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    int objNum = Data->GetObjIndexs().size();
    //int w = max(1000, defaultW / max(objNum / 1000, 1));
    //int h = max(1000, defaultH / max(objNum / 1000, 1));
    auto w = defaultW;
    auto h = defaultH;
    QRect drawFrame;
    _CalculateDrawFrame(w, h, drawFrame);
    QImage re = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    re.fill(Qt::transparent);
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&re);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    auto pen = QPen(QColor(GetQColorFromTuple(color, alpha)), POINT_SIZE);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    //Draw
    for (auto& objId: objDrawSort) {
        if (choosedObjIds.count(objId) == 0) continue;
        auto objIndex = objIndexs.at(objId);
        _DrawPoint(minValue, maxValue, minDistance, maxDistance, objData[objIndex][variableIndex],
                   objDistance[objIndex], painter, drawFrame);
    }
    return re;
}

void igQtDataChangeWidget::_DrawPoint(double minValue, double maxValue, double minDistance, double maxDistance,
                                      double value, double distance, std::shared_ptr<QPainter> painter,
                                      const QRect& drawFrame) {
    auto [x, y] = CalculatePointSite(value, distance, maxValue, minValue, maxDistance, minDistance, drawFrame);
    painter->drawPoint(x, y);
}

void igQtDataChangeWidget::_DrawBackground(const QRect& range) {
    QPainter painter(this);
    QBrush brush;
    brush.setColor(QColor(get<0>(m_BackgroundColor), get<1>(m_BackgroundColor), get<2>(m_BackgroundColor)));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.drawRect(range);
}

void igQtDataChangeWidget::_DrawCoordinateRect(const QRect& range) {
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::Black, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(range);
}

void igQtDataChangeWidget::_DrawImages(const QRect& range) {
    QPainter painter(this);
    for (int variableIndex = 0; variableIndex < m_VariableShow.size(); variableIndex++) {
        if (!m_VariableShow[variableIndex]) continue;
        painter.drawImage(range, m_VariableImages[variableIndex]);
    }
    for (int variableIndex = 0; variableIndex < m_VariableShow.size(); variableIndex++) {
        if (!m_VariableShow[variableIndex]) continue;
        painter.drawImage(range, m_ChoosedVariableImages[variableIndex]);
    }
}

void igQtDataChangeWidget::SetSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtDataChangeWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1);
}

void igQtDataChangeWidget::SetClearSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetClearSelectionCallBackEvent(&igQtDataChangeWidget::ClearSelectionCallback, this);
}

void igQtDataChangeWidget::SetRadialPointMoveCallBack() {
    m_RadialStyle->SetPointMoveCallBack(std::bind(&igQtDataChangeWidget::RadialPointMoveCallBack, this));
}

void igQtDataChangeWidget::SelectionCallbackEvent(const std::vector<Selection::Event>& _events) {
    UpdateChoosedData(_events);
    GenerateChoosedVariableImage();
    update();
}

void igQtDataChangeWidget::ClearSelectionCallback() {
    ClearChoosedData();
    GenerateChoosedVariableImage();
    update();
}

void igQtDataChangeWidget::RadialPointMoveCallBack() {
    static QElapsedTimer timer;
    if (!timer.isValid() || timer.elapsed() >= 50) {
        DrawRadial();
        timer.start();
    }
}

void igQtDataChangeWidget::ChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    auto colors = DataChangeData::GenerateVariableColor(Data->GetVariableHS(), Data->GetChoosedLight());//GetVariableHue(), SATURATION
    Data->SetChoosedVariableColor(colors);
    _SetChoosedVariableColorWidgetColor(Data->GetVariableNum(), colors);
    _GenerateChoosedVariableImage(m_VariableShow, Data);
    this->update();
    if (ui->choosedLightSpinBox->value() != value) ui->choosedLightSpinBox->setValue(value);
}

void igQtDataChangeWidget::UnChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    auto colors = DataChangeData::GenerateVariableColor(Data->GetVariableHS(), Data->GetUnChoosedLight());//GetVariableHue(), SATURATION
    Data->SetVariableColor(colors);
    _SetVariableColorWidgetColor(Data->GetVariableNum(), colors);
    _GenerateVariableImage(m_VariableShow, Data);
    this->update();
    if (ui->unChoosedLightSpinBox->value() != value) ui->unChoosedLightSpinBox->setValue(value);
}

void igQtDataChangeWidget::ChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    auto colors = DataChangeData::GenerateVariableColor(Data->GetVariableHS(), Data->GetChoosedLight());//GetVariableHue(), SATURATION
    Data->SetChoosedVariableColor(colors);
    _SetChoosedVariableColorWidgetColor(Data->GetVariableNum(), colors);
    _GenerateChoosedVariableImage(m_VariableShow, Data);
    this->update();
    if (ui->choosedLightSlider->value() != value) ui->choosedLightSlider->setValue(value);
}

void igQtDataChangeWidget::UnChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    auto colors = DataChangeData::GenerateVariableColor(Data->GetVariableHS(), Data->GetUnChoosedLight());//GetVariableHue(), SATURATION
    Data->SetVariableColor(colors);
    _SetVariableColorWidgetColor(Data->GetVariableNum(), colors);
    _GenerateVariableImage(m_VariableShow, Data);
    this->update();
    if (ui->unChoosedLightSlider->value() != value) ui->unChoosedLightSlider->setValue(value);
}

void igQtDataChangeWidget::VariableCheckButtonClicked(bool checked) {
    igQtDataChangeWidget_CheckBox* theSender = qobject_cast<igQtDataChangeWidget_CheckBox*>(sender());
    int variableIndex = theSender->m_VariableIndex;
    m_VariableShow[variableIndex] = checked;
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    auto [minValue, maxValue] = DataChangeData::GenerateObjMinMaxValue(Data->GetObjectDatas(), m_VariableShow);
    Data->SetMinValue(minValue);
    Data->SetMaxValue(maxValue);
    _GenerateVariableImage(m_VariableShow, Data);
    _GenerateChoosedVariableImage(m_VariableShow, Data);
    update();
}

void igQtDataChangeWidget::DataChooseChanged(int choosedIndex) {
    if (m_CurrentModelDataIndex == choosedIndex) return;
    m_CurrentModelDataIndex = choosedIndex;
    SetUiData();
    ResetVariableImage();
    update();
}

void igQtDataChangeWidget::RefreshData() {
    GenerateDataChangeDatas();
    SetDataTypeChoose();
    SetUiData();
    ClearVariableChoose();
    ResetVariableButton();
    ResetVariableImage();
    GenerateBackgroundColor();
}

void igQtDataChangeWidget::DataGetToolClicked(bool checked) { ShowRadial(checked); }

void igQtDataChangeWidget::TempSlot_SetRadialData() {
    if (m_CurrentModelDataIndex < 0 || m_DataChangeDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_DataChangeDatas[m_CurrentModelDataIndex];
    _SetRadialData(Data);
    _ResetVariableImage(Data->GetVariableNum());
    _GenerateVariableImage(m_VariableShow, Data);
    _GenerateChoosedVariableImage(m_VariableShow, Data);
    update();
}