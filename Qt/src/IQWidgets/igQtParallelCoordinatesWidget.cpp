#include "iGameSceneManager.h"
#include <IQWidgets/igQtParallelCoordinatesWidget.h>
#include <iomanip>
#include <iostream>
#include <tuple>
#include <QRgb>
#include <climits>
#include <cmath>
#include <algorithm>
#include <IQComponents/Dialog/igQtParallelCoordinatesSortVariableDialog.h>
#include <unordered_set>
#include <random>
#include <iGameThreadPool.h>
#include <thread>
#include <QElapsedTimer>

/**
 * @class   igQtParallelCoordinatesWidget
 * @brief   igQtParallelCoordinatesWidget's brief
 */

using namespace std;

static constexpr int leftSpace = 5, rightSpace = 5, topSpace = 5, bottomSpace = 5;
static constexpr int stringSize = 10;
static constexpr int eachInterval = 2;

static double CalculateValueByPos(int pos, int minPos, int maxPos, double minValue, double maxValue) {
    if (minPos == maxPos) return (minValue + maxValue) / 2;
    if (minValue == maxValue) return minValue;
    return (pos - minPos) * (maxValue - minValue) / (maxPos - minPos) + minValue;
}

static inline QColor GetQColorFromTuple(const tuple<int, int, int>& rgb, int alpha) {
    return QColor(get<0>(rgb), get<1>(rgb), get<2>(rgb), alpha);
}

static int DistanceSquared(const std::tuple<int, int, int>& color1, const std::tuple<int, int, int>& color2) {
    int r1 = std::get<0>(color1);
    int g1 = std::get<1>(color1);
    int b1 = std::get<2>(color1);
    int r2 = std::get<0>(color2);
    int g2 = std::get<1>(color2);
    int b2 = std::get<2>(color2);
    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;
    return dr * dr + dg * dg + db * db;
}

static int MinDistanceSquared(const std::tuple<int, int, int>& bg,
                              const std::vector<std::tuple<int, int, int>>& colors) {
    int minDistSq = std::numeric_limits<int>::max();
    for (const auto& obj: colors) {
        int distSq = DistanceSquared(bg, obj);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            if (minDistSq == 0) break;
        }
    }
    return minDistSq;
}

static std::tuple<int, int, int> CalculateBackgroundColor(const std::vector<std::tuple<int, int, int>>& colors) {
    if (colors.empty()) { return std::make_tuple(255, 255, 255); }

    const int step1 = 16;
    int bestMinDistSq = -1;
    std::tuple<int, int, int> bestBg;

    for (int r = 0; r <= 255; r += step1) {
        for (int g = 0; g <= 255; g += step1) {
            for (int b = 0; b <= 255; b += step1) {
                auto bg = std::make_tuple(r, g, b);
                int minDistSq = MinDistanceSquared(bg, colors);
                if (minDistSq > bestMinDistSq) {
                    bestMinDistSq = minDistSq;
                    bestBg = bg;
                }
            }
        }
    }

    int r0 = std::get<0>(bestBg);
    int g0 = std::get<1>(bestBg);
    int b0 = std::get<2>(bestBg);
    int r_start = std::max(0, r0 - step1);
    int r_end = std::min(255, r0 + step1);
    int g_start = std::max(0, g0 - step1);
    int g_end = std::min(255, g0 + step1);
    int b_start = std::max(0, b0 - step1);
    int b_end = std::min(255, b0 + step1);

    bestMinDistSq = -1;
    for (int r = r_start; r <= r_end; ++r) {
        for (int g = g_start; g <= g_end; ++g) {
            for (int b = b_start; b <= b_end; ++b) {
                auto bg = std::make_tuple(r, g, b);
                int minDistSq = MinDistanceSquared(bg, colors);
                if (minDistSq > bestMinDistSq) {
                    bestMinDistSq = minDistSq;
                    bestBg = bg;
                }
            }
        }
    }

    return bestBg;
}

static std::tuple<int, int, int> CalculateBackgroundColor(FloatArray::Pointer colorBar) {
    std::vector<std::tuple<int, int, int>> colors;
    for (int i = 0; i < colorBar->GetNumberOfElements(); i++) {
        colors.push_back({colorBar->GetElementValue(i, 0) * 255, colorBar->GetElementValue(i, 1) * 255,
                          colorBar->GetElementValue(i, 2) * 255});
    }
    return ParallelCoordinatesData::ChangeSaturation(CalculateBackgroundColor(colors), 85);
}

static void CalculateMidLinkLineXs(int lineNum, int minPos, int maxPos, std::vector<int>& poss) {
    if (lineNum <= 1) return;
    if (maxPos <= minPos) return;
    int interval = (maxPos - minPos) / lineNum;
    int halfInterval = interval / 2;
    for (int i = 0; i < lineNum; i++) {
        int nowMidLine = minPos + i * interval + halfInterval;
        poss.push_back(nowMidLine);
    }
}

igQtParallelCoordinatesWidget::igQtParallelCoordinatesWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ParallelCoordinatesView) {
    ui->setupUi(this);
    connect(ui->choosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtParallelCoordinatesWidget::ChoosedAlphaSliderChanged);
    connect(ui->choosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,  
           &igQtParallelCoordinatesWidget::ChoosedAlphaSpinBoxChanged);
    connect(ui->unChoosedAlphaSlider, &QSlider::valueChanged, this,
            &igQtParallelCoordinatesWidget::UnChoosedAlphaSliderChanged);
    connect(ui->unChoosedAlphaSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtParallelCoordinatesWidget::UnChoosedAlphaSpinBoxChanged);
    connect(ui->choosedLightSlider, &QSlider::valueChanged, this,
            &igQtParallelCoordinatesWidget::ChoosedLightSliderChanged);
    connect(ui->choosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtParallelCoordinatesWidget::ChoosedLightSpinBoxChanged);
    connect(ui->unChoosedLightSlider, &QSlider::valueChanged, this,
            &igQtParallelCoordinatesWidget::UnChoosedLightSliderChanged);
    connect(ui->unChoosedLightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &igQtParallelCoordinatesWidget::UnChoosedLightSpinBoxChanged);
    connect(ui->dataChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtParallelCoordinatesWidget::DataChooseChanged);
    connect(ui->colorChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtParallelCoordinatesWidget::ColorChooseChanged);
    connect(ui->refreshData, &QPushButton::clicked, this, &igQtParallelCoordinatesWidget::RefreshData);
    connect(ui->variableSort, &QPushButton::clicked, this, &igQtParallelCoordinatesWidget::SetVariableSort);
    connect(this, &igQtParallelCoordinatesWidget::SIGNAL_WaitImageLoading, this,
            &igQtParallelCoordinatesWidget::WaitImageLoading);
    connect(this, &igQtParallelCoordinatesWidget::SIGNAL_CompleteImageLoading, this,
            &igQtParallelCoordinatesWidget::CompleteImageLoading);
    connect(ui->rangeChoose, &QCheckBox::clicked, this, &igQtParallelCoordinatesWidget::RangeChooseButtonClicked);
    setMouseTracking(true);
    ui->ParallelCoordinatesDrawView->installEventFilter(this);
    ui->ParallelCoordinatesDrawView->setMouseTracking(true);
}

igQtParallelCoordinatesWidget::~igQtParallelCoordinatesWidget() {}

void igQtParallelCoordinatesWidget::RangeChooseObj(const QRect& chooseRange, const QRect& frameRange,
                                                   std::vector<igIndex>& ids, IGenum& type) {
    //init
    type = {};

    //set
    QRect overLapRange = frameRange.intersected(chooseRange);
    if (overLapRange.right() <= overLapRange.left() || overLapRange.bottom() <= overLapRange.top()) return;
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    type = Data->GetDataType();
    if (Data->GetVariableSort().size() <= 0) return;

    //range
    std::map<int, std::pair<double, double>> variableMinMaxValues;
    if (Data->GetVariableSort().size() == 1) {
        auto& variableIndex = Data->GetVariableSort().front();
        auto minValue = CalculateValueByPos(overLapRange.bottom(), frameRange.bottom(), frameRange.top(),
                                            Data->GetMinValueInVariables()[variableIndex],
                                            Data->GetMaxValueInVariables()[variableIndex]);
        auto maxValue = CalculateValueByPos(overLapRange.top(), frameRange.bottom(), frameRange.top(),
                                            Data->GetMinValueInVariables()[variableIndex],
                                            Data->GetMaxValueInVariables()[variableIndex]);
        variableMinMaxValues[variableIndex] = {minValue, maxValue};
    } else {
        std::vector<int> midLines;
        CalculateMidLinkLineXs(Data->GetVariableSort().size(), frameRange.left() + rightSpace,
                               frameRange.right() - leftSpace, midLines);
        for (int i = 0; i < Data->GetVariableSort().size(); i++) {
            auto& midLine = midLines[i];
            if (midLine < overLapRange.left() || overLapRange.right() < midLine) continue;
            auto& variableId = Data->GetVariableSort()[i];
            auto minValue = CalculateValueByPos(overLapRange.bottom(), frameRange.bottom(), frameRange.top(),
                                                Data->GetMinValueInVariables()[variableId],
                                                Data->GetMaxValueInVariables()[variableId]);
            auto maxValue = CalculateValueByPos(overLapRange.top(), frameRange.bottom(), frameRange.top(),
                                                Data->GetMinValueInVariables()[variableId],
                                                Data->GetMaxValueInVariables()[variableId]);
            variableMinMaxValues[variableId] = {minValue, maxValue};
        }
    }

    //choose
    if (variableMinMaxValues.size() == 0) return;
    ids = Data->FiltInRangeIds(variableMinMaxValues);
}

void igQtParallelCoordinatesWidget::EndRangeChoose() {
    if (!m_RangeChoosing) return;
    m_RangeChoosing = false;
    if (!m_RangeChooseOn) return;
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    QRect chooseRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint);
    std::vector<QRect> variableMaxFontPoints;
    std::vector<QRect> variableMinFontPoints;
    std::vector<QRect> variableNameFontPoints;
    QRect drawImageArea;
    QRect background;
    bool drawAble = GetDrawFramePoints(Data->GetVariableSort().size(), variableMaxFontPoints, variableMinFontPoints,
                                       variableNameFontPoints, drawImageArea, background);
    if (!drawAble) return;
    std::vector<igIndex> ids;
    IGenum type{};
    RangeChooseObj(chooseRect, drawImageArea, ids, type);
    auto events = Selection::GenerateEvents(ids, type, Selection::Event::Add, m_Mesh, m_Model->GetPainter3D().get());
    m_Model->GetSelection()->SelectionCallBackEvent(events);
    update();
}

void igQtParallelCoordinatesWidget::StartRangeChoose(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    QRect drawFrame;
    GetDrawWidgetRect(drawFrame);
    if (!drawFrame.contains(pos)) return;
    m_RangeChoosing = true;
    m_RangeChooseStartPoint = pos;
    m_RangeChooseEndPoint = pos;
}

void igQtParallelCoordinatesWidget::MoveRangeChooseEndPoint(const QPoint& pos) {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QRect drawFrame;
    GetDrawWidgetRect(drawFrame);
    int x = max(drawFrame.left(), min(pos.x(), drawFrame.right()));
    int y = max(drawFrame.top(), min(pos.y(), drawFrame.bottom()));
    m_RangeChooseEndPoint = {x, y};
    update();
}

void igQtParallelCoordinatesWidget::DrawRangeChooseRect() {
    if (!m_RangeChooseOn) return;
    if (!m_RangeChoosing) return;
    QPainter painter(this);
    painter.setPen(QPen(QColorConstants::DarkMagenta, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(m_RangeChooseStartPoint, m_RangeChooseEndPoint));
}

void igQtParallelCoordinatesWidget::RangeChooseButtonClicked(bool checked) { m_RangeChooseOn = checked; }

void igQtParallelCoordinatesWidget::mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);
    StartRangeChoose(event->pos());
}

void igQtParallelCoordinatesWidget::mouseReleaseEvent(QMouseEvent* event) {
    QWidget::mouseReleaseEvent(event);
    EndRangeChoose();
}

void igQtParallelCoordinatesWidget::paintEvent(QPaintEvent* QPE) {
    if (m_ParallelCoordinatesDatas.size() == 0 || m_CurrentModelDataIndex == -1) return;
    DrawParallelCoordinates();
    DrawRangeChooseRect();
}

void igQtParallelCoordinatesWidget::mouseMoveEvent(QMouseEvent* event) {
    handleMouseMove(event->pos());
    QWidget::mouseMoveEvent(event);
}

bool igQtParallelCoordinatesWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        QPoint parentPos = static_cast<QWidget*>(watched)->mapTo(this, mouseEvent->pos());
        handleMouseMove(parentPos);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void igQtParallelCoordinatesWidget::handleMouseMove(const QPoint& pos) {
    static QElapsedTimer timer;
    if (!timer.isValid() || timer.elapsed() >= 50) {
        MoveRangeChooseEndPoint(pos);
        timer.start();
    }
}

void igQtParallelCoordinatesWidget::SetParallelCoordinates(Model::Pointer model) {
    m_Model = model;
    m_Mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    SetSelectionCallback();
    SetClearSelectionCallback();
    UpdateData();
    SetUpdateLinkImage();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::UpdateData() {
    GenerateModelDatas();
    SetDataChoosedComboBox();
    SetColorComboBox();
    SetUiData();
}

void igQtParallelCoordinatesWidget::UpdateColor() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    UpdateChoosedColor();
    UpdateUnChoosedColor();
}

void igQtParallelCoordinatesWidget::UpdateChoosedColor() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    auto colorMap = m_Mesh->GetColorMapper();
    Data->SetChoosedObjectColor(ParallelCoordinatesData::GenerateObjectColors(
            m_ColorVariableIndex, Data->GetChoosedObjectIds(), Data, Data->GetMaxValueInVariables(),
            Data->GetMinValueInVariables(), Data->GetChoosedLight(), colorMap));
}

void igQtParallelCoordinatesWidget::UpdateUnChoosedColor() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    auto colorMap = m_Mesh->GetColorMapper();
    Data->SetObjectColor(ParallelCoordinatesData::GenerateObjectColors(
            m_ColorVariableIndex, Data->GetKeyObjectIds(), Data, Data->GetMaxValueInVariables(),
            Data->GetMinValueInVariables(), Data->GetUnChoosedLight(), colorMap));
}

void igQtParallelCoordinatesWidget::UpdateBackgroundColor() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex ||
        m_ColorVariableIndex < 0 ||
        m_ParallelCoordinatesDatas[m_CurrentModelDataIndex]->GetVariableNum() <= m_ColorVariableIndex) {
        m_BackgroundColor = {255, 255, 255};
        return;
    }
    auto colorMap = m_Mesh->GetColorMapper();
    auto colorBar = colorMap->GetColorBar();
    m_BackgroundColor = CalculateBackgroundColor(colorBar);
}

void igQtParallelCoordinatesWidget::UpdateChoosedData(const std::vector<Selection::Event>& _events) {
    for (auto& Data: m_ParallelCoordinatesDatas) {
        for (auto& e: _events) {
            switch (e.type) {
                case Selection::Event::Type::PickPoint:
                    if (Data->GetDataType() != IG_POINT) break;
                    if (e.operate == Selection::Event::Operate::Add) Data->AddChoosedObjectId(e.pickId);
                    else if (e.operate == Selection::Event::Operate::Remove)
                        Data->RemoveChoosedObjectId(e.pickId);
                    break;
                case Selection::Event::Type::PickFace:
                    if (Data->GetDataType() != IG_CELL) break;
                    if (e.operate == Selection::Event::Operate::Add) Data->AddChoosedObjectId(e.pickId);
                    else if (e.operate == Selection::Event::Operate::Remove)
                        Data->RemoveChoosedObjectId(e.pickId);
                    break;
                default:
                    break;
            }
        }
        Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(
                Data->GetVariableNum(), Data->GetChoosedObjectIds(), Data));
    }
}

void igQtParallelCoordinatesWidget::ClearChoosedData() {
    for (auto& Data: m_ParallelCoordinatesDatas) {
        Data->ClearChoosedObjectIds();
        Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(
                Data->GetVariableNum(), Data->GetChoosedObjectIds(), Data));
    }
}

void igQtParallelCoordinatesWidget::SetUiData() {
    ClearObjectFilters();
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    ui->choosedAlphaSlider->setValue(Data->GetChoosedAlpha());
    ui->choosedAlphaSpinBox->setValue(Data->GetChoosedAlpha());
    ui->unChoosedAlphaSlider->setValue(Data->GetUnChoosedAlpha());
    ui->unChoosedAlphaSpinBox->setValue(Data->GetUnChoosedAlpha());
    ui->choosedLightSlider->setValue(Data->GetChoosedLight());
    ui->choosedLightSpinBox->setValue(Data->GetChoosedLight());
    ui->unChoosedLightSlider->setValue(Data->GetUnChoosedLight());
    ui->unChoosedLightSpinBox->setValue(Data->GetUnChoosedLight());
    SetObjectFilters(Data->GetVariableSort(), Data->GetVariableName(), Data->GetFilterMaxValue(),
                     Data->GetFilterMinValue());
}

void igQtParallelCoordinatesWidget::SetDataChoosedComboBox() {
    ui->dataChoose->clear();
    if (m_ParallelCoordinatesDatas.size() == 0) {
        ui->dataChoose->hide();
        return;
    }
    ui->dataChoose->show();
    for (auto& Data: m_ParallelCoordinatesDatas) { ui->dataChoose->addItem(Data->GetDataTypeName().c_str()); }
}

void igQtParallelCoordinatesWidget::SetColorComboBox() {
    ui->colorChoose->clear();
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) {
        ui->colorChoose->hide();
        return;
    }
    ui->colorChoose->show();
    ui->colorChoose->addItem("default");
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    for (auto& variableName: Data->GetVariableName()) { ui->colorChoose->addItem(variableName.c_str()); }
}

std::vector<double> igQtParallelCoordinatesWidget::GetObjectData(IGenum dataType, int objId) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    std::vector<double> objData;
    for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
        auto& attr = attrs->GetElement(attrIndex);
        if (attr.attachmentType != dataType) continue;
        if (attr.pointer->GetDimension() > 1) { objData.push_back(attr.pointer->GetElementValue(objId, -1)); }
        for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
            objData.push_back(attr.pointer->GetElementValue(objId, dimensionIndex));
        }
    }
    return objData;
}

void igQtParallelCoordinatesWidget::GenerateModelDatas() {
    m_ParallelCoordinatesDatas.clear();
    m_CurrentModelDataIndex = -1;
    auto pointData = GeneratePointData();
    if (pointData.IsNotNull()) {
        m_ParallelCoordinatesDatas.push_back(pointData);
    }
    auto cellData = GenerateCellData();
    if (cellData.IsNotNull()) {
        m_ParallelCoordinatesDatas.push_back(cellData);
    }
    if (m_ParallelCoordinatesDatas.size() != 0) m_CurrentModelDataIndex = 0;
}

ParallelCoordinatesData::Pointer igQtParallelCoordinatesWidget::GeneratePointData() { return GenerateData(IG_POINT); }

ParallelCoordinatesData::Pointer igQtParallelCoordinatesWidget::GenerateCellData() { return GenerateData(IG_CELL); }

ParallelCoordinatesData::Pointer igQtParallelCoordinatesWidget::GenerateData(IGenum dataType) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    auto& selectedItems = m_Model->GetSelection()->GetSelectedItems();
    int objNum{};
    if (dataType == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    return ParallelCoordinatesData::New(attrs, dataType, selectedItems, objNum);
}

void igQtParallelCoordinatesWidget::SetSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtParallelCoordinatesWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1);
}

void igQtParallelCoordinatesWidget::SelectionCallbackEvent(const std::vector<Selection::Event>& _events) {
    UpdateChoosedData(_events);
    UpdateChoosedColor();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::SetClearSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetClearSelectionCallBackEvent(&igQtParallelCoordinatesWidget::ClearSelectionCallback, this);
}

void igQtParallelCoordinatesWidget::ClearSelectionCallback() {
    ClearChoosedData();
    UpdateChoosedColor();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::SetObjectFilters(const std::vector<int>& variableSort,
                                                     const std::vector<std::string>& variableName,
                                                     const std::vector<double>& filterMaxValue,
                                                     const std::vector<double>& filterMinValue) {
    for (auto& variableIndex: variableSort) {
        igQtParallelCoordinatesObjectFilter* pcObjFilter = new igQtParallelCoordinatesObjectFilter(
                variableIndex, filterMaxValue[variableIndex], filterMinValue[variableIndex],
                variableName[variableIndex], this);
        connect(pcObjFilter, &igQtParallelCoordinatesObjectFilter::ChangeMaxValue, this,
                &igQtParallelCoordinatesWidget::FilterMaxValueChanged);
        connect(pcObjFilter, &igQtParallelCoordinatesObjectFilter::ChangeMinValue, this,
                &igQtParallelCoordinatesWidget::FilterMinValueChanged);
        m_PcObjFilters.push_back(pcObjFilter);
        ui->coreHorizontalLayout->addWidget(pcObjFilter);
    }
}

void igQtParallelCoordinatesWidget::ClearObjectFilters() {
    for (auto& pcObjFilter: m_PcObjFilters) { pcObjFilter->deleteLater(); }
    m_PcObjFilters.clear();
}

void igQtParallelCoordinatesWidget::FilterMaxValueChanged(int number, double value) {
    if (m_CurrentModelDataIndex == -1) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    Data->FilterMaxValue()[number] = value;
    SetUpdateLinkImage();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::FilterMinValueChanged(int number, double value) {
    if (m_CurrentModelDataIndex == -1) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    Data->FilterMinValue()[number] = value;
    SetUpdateLinkImage();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::DataChooseChanged(int choosedIndex) {
    if (m_CurrentModelDataIndex == choosedIndex) return;
    m_CurrentModelDataIndex = choosedIndex;
    SetColorComboBox();
    SetUiData();
    SetUpdateLinkImage();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::ColorChooseChanged(int choosedIndex) {
    m_ColorVariableIndex = choosedIndex - 1;
    UpdateColor();
    UpdateBackgroundColor();
    SetUpdateLinkImage();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::RefreshData() {
    emit SIGNAL_RefreshDataClicked();
    //auto choosedDataIndex = m_CurrentModelDataIndex;
    //UpdateData();
    //UpdateColor();
    //UpdateBackgroundColor();
    //SetUpdateLinkImage();
    //UpdatingChoosedLinkImage();
    //update();
    //if (choosedDataIndex < m_ParallelCoordinatesDatas.size()) m_CurrentModelDataIndex = choosedDataIndex;
}

void igQtParallelCoordinatesWidget::SetVariableSort() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    igQtParallelCoordinatesSortVariableDialog* sortDialog = new igQtParallelCoordinatesSortVariableDialog(
            Data->GetVariableNum(), Data->GetVariableName(), Data->GetVariableSort(), this);
    connect(sortDialog, &igQtParallelCoordinatesSortVariableDialog::ReturnSort, this,
            &igQtParallelCoordinatesWidget::GetVariableSortFromDialog);
    sortDialog->exec();
}

void igQtParallelCoordinatesWidget::GetVariableSortFromDialog(const std::vector<int>& choosedSort) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    for (auto& choosedIndex: choosedSort) {
        if (Data->GetVariableNum() <= choosedIndex) return;
    }
    Data->SetVariableSort(choosedSort);
    ClearObjectFilters();
    SetObjectFilters(Data->GetVariableSort(), Data->GetVariableName(), Data->GetFilterMaxValue(),
                     Data->GetFilterMinValue());
    SetUpdateLinkImage();
    UpdatingChoosedLinkImage();
    update();
}

void igQtParallelCoordinatesWidget::WaitImageLoading() {
    setDisabled(true);
    m_ImageLoading = true;
    update();
}

void igQtParallelCoordinatesWidget::CompleteImageLoading() {
    setDisabled(false);
    m_ImageLoading = false;
    update();
}

void igQtParallelCoordinatesWidget::DrawParallelCoordinates() {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    std::vector<QRect> variableMaxFontPoints;
    std::vector<QRect> variableMinFontPoints;
    std::vector<QRect> variableNameFontPoints;
    QRect drawImageArea;
    QRect background;
    bool drawAble = GetDrawFramePoints(Data->GetVariableSort().size(), variableMaxFontPoints, variableMinFontPoints,
                                       variableNameFontPoints, drawImageArea, background);
    if (!drawAble) return;
    DrawBackground(background);
    DrawStrs(variableMaxFontPoints, variableMinFontPoints, variableNameFontPoints);
    DrawLinkImage(drawImageArea);
    //DrawLinks(linkTopPoints, linkBottomPoints);
    //DrawLinks(linkTopPoints, linkBottomPoints, QColor(255, 0, 0, Data->GetChoosedAlpha()),
    //          QColor(0, 0, 255, Data->GetUnChoosedAlpha()));
}

int igQtParallelCoordinatesWidget::GetLinePointLocation(int top, int bottom, double currentValue, double maxValue,
                                                        double minValue) {
    if (maxValue == minValue) return (top + bottom) / 2;
    return (currentValue - minValue) * (top - bottom) / (maxValue - minValue) + bottom;
}

void igQtParallelCoordinatesWidget::DrawBackground(const QRect& range) {
    QPainter painter(this);
    QBrush brush;
    brush.setColor(QColor(get<0>(m_BackgroundColor), get<1>(m_BackgroundColor), get<2>(m_BackgroundColor)));
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.drawRect(range);
}

void igQtParallelCoordinatesWidget::DrawStrs(std::vector<QRect>& variableMaxFontPoints,
                                             std::vector<QRect>& variableMinFontPoints,
                                             std::vector<QRect>& variableNameFontPoints) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    QPainter painter(this);
    QPen pen;
    pen.setWidth(10);
    QFont font;
    font.setPointSize(8);
    painter.setPen(pen);
    painter.setFont(font);
    for (int i = 0; i < Data->GetVariableSort().size(); i++) {
        int variableIndex = Data->GetVariableSort()[i];
        painter.drawText(variableNameFontPoints[i], Qt::AlignCenter,
                         QString(Data->GetVariableName()[variableIndex].c_str()));
        painter.drawText(variableMaxFontPoints[i], Qt::AlignCenter,
                         QString::number(Data->GetMaxValueInVariables()[variableIndex]));
        painter.drawText(variableMinFontPoints[i], Qt::AlignCenter,
                         QString::number(Data->GetMinValueInVariables()[variableIndex]));
    }
}

void igQtParallelCoordinatesWidget::DrawLinkImage(QRect& linkImageArea) {
    QPainter painter(this);
    if (m_ImageLoading) {
        QPen pen;
        pen.setWidth(10);
        QFont font;
        font.setPointSize(8);
        painter.setPen(pen);
        painter.setFont(font);
        painter.drawText(linkImageArea, Qt::AlignCenter, "Loading...");
    } else {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // 设置平滑缩放
        {
            std::lock_guard lg(m_LinkImageMutex);
            painter.drawImage(linkImageArea, m_LinkImage);
        }
        painter.drawImage(linkImageArea, m_ChoosedLinkImage);
    }
}

bool igQtParallelCoordinatesWidget::IsChoosedObj(IGenum dataType, int objId) {
    auto selection = m_Model->GetSelection();
    auto& selectedItems = selection->GetSelectedItems();
    if (dataType == IG_POINT) {
        if (selectedItems.count(Selection::Event::Type::PickPoint) == 0) return false;
        if (selectedItems.at(Selection::Event::Type::PickPoint).count(objId) == 0) return false;
        return true;
    } else {
        if (selectedItems.count(Selection::Event::Type::PickFace) == 0) return false;
        if (selectedItems.at(Selection::Event::Type::PickFace).count(objId) == 0) return false;
        return true;
    }
}

void igQtParallelCoordinatesWidget::SetUpdateLinkImage() {
    std::thread updateLinkImage([&]() {
        emit SIGNAL_WaitImageLoading();
        UpdatingLinkImage();
        emit SIGNAL_CompleteImageLoading();
    });
    updateLinkImage.detach();
}

static constexpr int defaultW = 1920, defaultH = 1080;

void igQtParallelCoordinatesWidget::UpdatingLinkImage() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    std::vector<QPoint> linkTopPoints;
    std::vector<QPoint> linkBottomPoints;
    int objNum = Data->GetKeyObjectIds().size();
    int w = max(1000, defaultW / max(objNum / 1000, 1));
    int h = min<int>(1000, max(50, defaultH / max(objNum / 1000, 1)) * Data->GetVariableSort().size());
    auto image = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    bool drawAble = CalculateLinkPointInImage(Data->GetVariableSort().size(), image.width(), image.height(),
                                              linkTopPoints, linkBottomPoints);
    if (!drawAble) return;
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&image);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    GenerateDrawLinksImage(linkTopPoints, linkBottomPoints, painter);
    {
        std::lock_guard lg(m_LinkImageMutex);
        m_LinkImage = image;
    }
}

void igQtParallelCoordinatesWidget::UpdatingChoosedLinkImage() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    std::vector<QPoint> linkTopPoints;
    std::vector<QPoint> linkBottomPoints;
    int objNum = Data->GetChoosedObjectIds().size();
    int w = max(1000, defaultW / max(objNum / 1000, 1));
    int h = min<int>(1000, max(50, defaultH / max(objNum / 1000, 1)) * Data->GetVariableSort().size());
    m_ChoosedLinkImage = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    m_ChoosedLinkImage.fill(Qt::transparent);
    bool drawAble = CalculateLinkPointInImage(Data->GetVariableSort().size(), m_ChoosedLinkImage.width(),
                                              m_ChoosedLinkImage.height(), linkTopPoints, linkBottomPoints);
    if (!drawAble) return;
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(&m_ChoosedLinkImage);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    GenerateChoosedDrawLinksImage(linkTopPoints, linkBottomPoints, painter);
}

bool igQtParallelCoordinatesWidget::CalculateLinkPointInImage(int variableSortSize, int imageW, int imageH,
                                                              std::vector<QPoint>& linkTopPoints,
                                                              std::vector<QPoint>& linkBottomPoints) {
    if (variableSortSize <= 0) return false;
    if (variableSortSize == 1) {
        linkTopPoints.push_back(QPoint(0, 0));
        linkTopPoints.push_back(QPoint(imageW, 0));
        linkBottomPoints.push_back(QPoint(0, imageH));
        linkBottomPoints.push_back(QPoint(imageW, imageH));
        return true;
    }
    int interval = imageW / variableSortSize;
    for (int i = 0; i < variableSortSize; i++) {
        int halfInterval = interval / 2;
        int nowMidLine = i * interval + halfInterval;
        linkTopPoints.push_back(QPoint(nowMidLine, 0));
        linkBottomPoints.push_back(QPoint(nowMidLine, imageH));
    }
    return true;
}

void igQtParallelCoordinatesWidget::GenerateDrawLinkImage(int leftLine, int rightLine, int top, int bottom,
                                                          double leftValue, double rightValue, double leftMaxValue,
                                                          double leftMinValue, double rightMaxValue,
                                                          double rightMinValue,
                                                          const std::shared_ptr<QPainter>& painter) {
    int leftPoint = GetLinePointLocation(top, bottom, leftValue, leftMaxValue, leftMinValue);
    int rightPoint = GetLinePointLocation(top, bottom, rightValue, rightMaxValue, rightMinValue);
    painter->drawLine(leftLine, leftPoint, rightLine, rightPoint);
}

void igQtParallelCoordinatesWidget::GenerateDrawLinksImage(std::vector<QPoint>& linkTopPoints,
                                                           std::vector<QPoint>& linkBottomPoints,
                                                           const std::shared_ptr<QPainter>& painter) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    auto& drawSort = Data->GetObjectDrawSorts()[std::max(0, m_ColorVariableIndex)];
    if (Data->GetVariableSort().size() == 1) {
        int variableIndex = Data->GetVariableSort().front();
        for (auto& objId: drawSort) {
            auto objIdx = Data->GetKeyObjectIdToIndexMap().at(objId);
            if (Data->NotInFilterValueRange(objId)) continue;
            painter->setPen(
                    QPen(GetQColorFromTuple(Data->GetObjectColor(false, objIdx), Data->GetUnChoosedAlpha()), 1));
            GenerateDrawLinkImage(
                    linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                    linkBottomPoints.front().y(), Data->GetObjectData(objId, variableIndex),
                    Data->GetObjectData(objId, variableIndex), Data->GetMaxValueInVariables()[variableIndex],
                    Data->GetMinValueInVariables()[variableIndex], Data->GetMaxValueInVariables()[variableIndex],
                    Data->GetMinValueInVariables()[variableIndex], painter);
        }
        return;
    }
    for (auto& objId: drawSort) {
        auto objIdx = Data->GetKeyObjectIdToIndexMap().at(objId);
        if (Data->NotInFilterValueRange(objId)) continue;
        painter->setPen(QPen(GetQColorFromTuple(Data->GetObjectColor(false, objIdx), Data->GetUnChoosedAlpha()), 1));
        for (int sortIndex = 0; sortIndex < Data->GetVariableSort().size() - 1; sortIndex++) {
            int variableIndexA = Data->GetVariableSort()[sortIndex];
            int variableIndexB = Data->GetVariableSort()[sortIndex + 1];
            GenerateDrawLinkImage(
                    linkTopPoints[sortIndex].x(), linkTopPoints[sortIndex + 1].x(), linkTopPoints[sortIndex].y(),
                    linkBottomPoints[sortIndex].y(), Data->GetObjectData(objId, variableIndexA),
                    Data->GetObjectData(objId, variableIndexB),
                    Data->GetMaxValueInVariables()[variableIndexA], Data->GetMinValueInVariables()[variableIndexA],
                    Data->GetMaxValueInVariables()[variableIndexB], Data->GetMinValueInVariables()[variableIndexB],
                    painter);
        }
    }
}

void igQtParallelCoordinatesWidget::GenerateChoosedDrawLinksImage(std::vector<QPoint>& linkTopPoints,
                                                                  std::vector<QPoint>& linkBottomPoints,
                                                                  const std::shared_ptr<QPainter>& painter) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    auto& choosedDrawSort = Data->GetChoosedObjDrawSorts()[std::max(0, m_ColorVariableIndex)];
    if (Data->GetVariableSort().size() == 1) {
        int variableIndex = Data->GetVariableSort().front();
        for (auto& objId: choosedDrawSort) {
            if (Data->NotInFilterValueRange(objId)) continue;
            painter->setPen(QPen(GetQColorFromTuple(Data->GetObjectColor(true, objId), Data->GetChoosedAlpha()), 1));
            GenerateDrawLinkImage(
                    linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                    linkBottomPoints.front().y(), Data->GetObjectData(objId, variableIndex),
                    Data->GetObjectData(objId, variableIndex), Data->GetMaxValueInVariables()[variableIndex],
                    Data->GetMinValueInVariables()[variableIndex], Data->GetMaxValueInVariables()[variableIndex],
                    Data->GetMinValueInVariables()[variableIndex], painter);
        }
        return;
    }
    for (auto& objId: choosedDrawSort) {
        if (Data->NotInFilterValueRange(objId)) continue;
        painter->setPen(QPen(GetQColorFromTuple(Data->GetObjectColor(true, objId), Data->GetChoosedAlpha()), 1));
        for (int sortIndex = 0; sortIndex < Data->GetVariableSort().size() - 1; sortIndex++) {
            int variableIndexA = Data->GetVariableSort()[sortIndex];
            int variableIndexB = Data->GetVariableSort()[sortIndex + 1];
            GenerateDrawLinkImage(
                    linkTopPoints[sortIndex].x(), linkTopPoints[sortIndex + 1].x(), linkTopPoints[sortIndex].y(),
                    linkBottomPoints[sortIndex].y(), Data->GetObjectData(objId, variableIndexA),
                    Data->GetObjectData(objId, variableIndexB), Data->GetMaxValueInVariables()[variableIndexA],
                    Data->GetMinValueInVariables()[variableIndexA], Data->GetMaxValueInVariables()[variableIndexB],
                    Data->GetMinValueInVariables()[variableIndexB], painter);
        }
    }
}

void igQtParallelCoordinatesWidget::ChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    UpdatingChoosedLinkImage();
    this->update();
    if (ui->choosedAlphaSpinBox->value() != value) ui->choosedAlphaSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedAlphaSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    SetUpdateLinkImage();
    this->update();
    if (ui->unChoosedAlphaSpinBox->value() != value) ui->unChoosedAlphaSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    UpdatingChoosedLinkImage();
    this->update();
    if (ui->choosedAlphaSlider->value() != value) ui->choosedAlphaSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedAlphaSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    SetUpdateLinkImage();
    this->update();
    if (ui->unChoosedAlphaSlider->value() != value) ui->unChoosedAlphaSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    Data->SetChoosedDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(value));
    UpdateChoosedColor();
    UpdatingChoosedLinkImage();
    this->update();
    if (ui->choosedLightSpinBox->value() != value) ui->choosedLightSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedLightSliderChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    Data->SetDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(value));
    UpdateUnChoosedColor();
    SetUpdateLinkImage();
    this->update();
    if (ui->unChoosedLightSpinBox->value() != value) ui->unChoosedLightSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    Data->SetChoosedDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(value));
    UpdateChoosedColor();
    UpdatingChoosedLinkImage();
    this->update();
    if (ui->choosedLightSlider->value() != value) ui->choosedLightSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedLightSpinBoxChanged(int value) {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    Data->SetDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(value));
    UpdateUnChoosedColor();
    SetUpdateLinkImage();
    this->update();
    if (ui->unChoosedLightSlider->value() != value) ui->unChoosedLightSlider->setValue(value);
}

bool igQtParallelCoordinatesWidget::GetDrawFramePoints(int variableSortSize, std::vector<QRect>& variableMaxFontPoints,
                                                       std::vector<QRect>& variableMinFontPoints,
                                                       std::vector<QRect>& variableNameFontPoints, QRect& linkImageArea,
                                                       QRect& background) {
    if (variableSortSize < 1) return false;
    QPoint startPoint(ui->ParallelCoordinatesDrawView->x() + leftSpace,
                      ui->ParallelCoordinatesDrawView->y() + topSpace),
            endPoint(ui->ParallelCoordinatesDrawView->x() + ui->ParallelCoordinatesDrawView->size().width() -
                             rightSpace,
                     ui->ParallelCoordinatesDrawView->y() + ui->ParallelCoordinatesDrawView->size().height() -
                             bottomSpace);
    background =
            QRect(ui->ParallelCoordinatesDrawView->x(), ui->ParallelCoordinatesDrawView->y(),
                  ui->ParallelCoordinatesDrawView->size().width(), ui->ParallelCoordinatesDrawView->size().height());
    if (endPoint.x() <= startPoint.x() || endPoint.y() - stringSize * 3 - eachInterval * 3 <= startPoint.y())
        return false;
    int useableWidth = endPoint.x() - startPoint.x();
    int useableHeight = endPoint.y() - startPoint.y();
    int maxFontStartTop = startPoint.y();
    int maxFontEndBottom = maxFontStartTop + stringSize;
    int variableStartTop = maxFontEndBottom + eachInterval;
    int nameFontEndBottom = endPoint.y();
    int nameFontStartTop = nameFontEndBottom - stringSize;
    int minFontEndBottom = nameFontStartTop - eachInterval;
    int minFontStartTop = minFontEndBottom - stringSize;
    int variableEndBottom = minFontStartTop - eachInterval;
    linkImageArea = QRect(QPoint(startPoint.x(), variableStartTop), QPoint(endPoint.x(), variableEndBottom));
    if (variableSortSize == 1) {
        int variableInterval = useableWidth;
        variableMaxFontPoints.push_back(QRect(
                QPoint(0, maxFontStartTop), QPoint(ui->ParallelCoordinatesDrawView->size().width(), maxFontEndBottom)));
        variableMinFontPoints.push_back(QRect(
                QPoint(0, minFontStartTop), QPoint(ui->ParallelCoordinatesDrawView->size().width(), minFontEndBottom)));
        variableNameFontPoints.push_back(
                QRect(QPoint(0, nameFontStartTop),
                      QPoint(ui->ParallelCoordinatesDrawView->size().width(), nameFontEndBottom)));
        return true;
    }
    //Interval of midline in each variable
    int variableInterval = useableWidth / variableSortSize;
    for (int i = 0; i < variableSortSize; i++) {
        int halfInterval = variableInterval / 2;
        int nowMidLine = startPoint.x() + i * variableInterval + halfInterval;
        variableMaxFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, maxFontStartTop),
                                              QPoint(nowMidLine + halfInterval, maxFontEndBottom)));
        variableMinFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, minFontStartTop),
                                              QPoint(nowMidLine + halfInterval, minFontEndBottom)));
        variableNameFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, nameFontStartTop),
                                               QPoint(nowMidLine + halfInterval, nameFontEndBottom)));
    }
    return true;
}

void igQtParallelCoordinatesWidget::GetDrawWidgetRect(QRect& frame) {
    QRect drawWidgetRect = ui->ParallelCoordinatesDrawView->rect();
    drawWidgetRect.moveTo(ui->ParallelCoordinatesDrawView->mapTo(this, QPoint(0, 0)));
    frame = drawWidgetRect;
}