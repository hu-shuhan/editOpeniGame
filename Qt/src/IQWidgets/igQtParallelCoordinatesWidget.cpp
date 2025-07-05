#include "iGameSceneManager.h"
#include <IQWidgets/igQtParallelCoordinatesWidget.h>
#include <iomanip>
#include <iostream>
/**
 * @class   igQtParallelCoordinatesWidget
 * @brief   igQtParallelCoordinatesWidget's brief
 */
using namespace std;
igQtParallelCoordinatesWidget::igQtParallelCoordinatesWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::ParallelCoordinatesView) {
    ui->setupUi(this);
    m_SpaceItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
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
}
igQtParallelCoordinatesWidget::~igQtParallelCoordinatesWidget() {}

void igQtParallelCoordinatesWidget::paintEvent(QPaintEvent* QPE) {
    if (m_ParallelCoordinatesDatas.size() == 0 || m_CurrentModelDataIndex == -1) return;
    DrawParallelCoordinates();
}

void igQtParallelCoordinatesWidget::SetParallelCoordinates(Model::Pointer model) {
    m_Model = model;
    m_Mesh = DynamicCast<UnstructuredMesh>(m_Model->GetDataObject());
    SetSelect();
    UpdataData();
}

void igQtParallelCoordinatesWidget::UpdataData() {
    GenerateModelDatas();
    SetComboBox();
    LoadCurrentData();
    update();
}

void igQtParallelCoordinatesWidget::LoadCurrentData() {
    ClearObjectFilters();
    if (m_CurrentModelDataIndex == -1) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    ui->choosedAlphaSlider->setValue(Data->GetChoosedAlpha());
    ui->choosedAlphaSpinBox->setValue(Data->GetChoosedAlpha());
    ui->unChoosedAlphaSlider->setValue(Data->GetUnChoosedAlpha());
    ui->unChoosedAlphaSpinBox->setValue(Data->GetUnChoosedAlpha());
    ui->choosedLightSlider->setValue(Data->GetChoosedLight());
    ui->choosedLightSpinBox->setValue(Data->GetChoosedLight());
    ui->unChoosedLightSlider->setValue(Data->GetUnChoosedLight());
    ui->unChoosedLightSpinBox->setValue(Data->GetUnChoosedLight());
    SetObjectFilters(Data->GetVariableNum(), Data->GetVariableName(), Data->GetFilterMaxValue(),
                     Data->GetFilterMinValue());
}

void igQtParallelCoordinatesWidget::SetComboBox() {
    ui->dataChoose->clear();
    if (m_ParallelCoordinatesDatas.size() == 0) {
        ui->dataChoose->hide();
        return;
    }
    ui->dataChoose->show();
    for (auto& Data: m_ParallelCoordinatesDatas) { ui->dataChoose->addItem(Data->GetDataTypeName().c_str()); }
}

void igQtParallelCoordinatesWidget::GenerateModelDatas() {
    m_ParallelCoordinatesDatas.clear();
    m_CurrentModelDataIndex = -1;
    auto pointData = GeneratePointData();
    if (pointData.IsNotNull()) {
        pointData->SetDataTypeName("Point");
        m_ParallelCoordinatesDatas.push_back(pointData);
    }
    auto cellData = GenerateCellData();
    if (cellData.IsNotNull()) {
        cellData->SetDataTypeName("Cell");
        m_ParallelCoordinatesDatas.push_back(cellData);
    }
    if (m_ParallelCoordinatesDatas.size() != 0) m_CurrentModelDataIndex = 0;
}

ParallelCoordinatesData::Pointer igQtParallelCoordinatesWidget::GeneratePointData() { return GenerateData(IG_POINT); }

ParallelCoordinatesData::Pointer igQtParallelCoordinatesWidget::GenerateCellData() { return GenerateData(IG_CELL); }

ParallelCoordinatesData::Pointer igQtParallelCoordinatesWidget::GenerateData(IGenum dataType) {
    auto variableNames = GetVariableNames(dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return ParallelCoordinatesData::Pointer();
    auto parallelCoordinatesData = ParallelCoordinatesData::New(variableNum);
    parallelCoordinatesData->SetVariableName(variableNames);
    auto objDatas = GetObjectDatas(dataType);
    parallelCoordinatesData->SetObjectData(objDatas);
    auto [minValue, maxValue] = GetMinMaxData(dataType, variableNum);
    parallelCoordinatesData->SetMinValueInVariables(minValue);
    parallelCoordinatesData->SetMaxValueInVariables(maxValue);
    parallelCoordinatesData->SetFilterMinValue(minValue);
    parallelCoordinatesData->SetFilterMaxValue(maxValue);
    parallelCoordinatesData->SetDataType(dataType);
    return parallelCoordinatesData;
}

std::vector<std::string> igQtParallelCoordinatesWidget::GetVariableNames(IGenum dataType) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    //Get the name of variables
    vector<string> variableNames;
    for (int i = 0; i < attrs->Size(); i++) {
        auto& attr = attrs->GetElement(i);
        if (attr.attachmentType != dataType) continue;
        if (attr.pointer->GetDimension() == 1) {
            variableNames.push_back(attr.pointer->GetName());
            continue;
        }
        for (int j = 1; j <= attr.pointer->GetDimension(); j++) {
            stringstream ss;
            ss << attr.pointer->GetName() << j;
            variableNames.push_back(ss.str());
        }
    }
    return variableNames;
}

std::vector<std::vector<double>> igQtParallelCoordinatesWidget::GetObjectDatas(IGenum dataType) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    int objNum{};
    if (dataType == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    std::vector<std::vector<double>> objDatas;
    for (int objIndex = 0; objIndex < objNum; objIndex++) {
        std::vector<double> objData;
        for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
            auto& attr = attrs->GetElement(attrIndex);
            if (attr.attachmentType != dataType) continue;
            for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
                objData.push_back(attr.pointer->GetElementValue(objIndex, dimensionIndex));
            }
        }
        objDatas.push_back(objData);
    }
    return objDatas;
}

std::pair<std::vector<double>, std::vector<double>> igQtParallelCoordinatesWidget::GetMinMaxData(IGenum dataType,
                                                                                                 int variableNum) {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    int objNum{};
    if (dataType == IG_POINT) objNum = m_Mesh->GetNumberOfPoints();
    else
        objNum = m_Mesh->GetNumberOfCells();
    std::vector<double> minData;
    std::vector<double> maxData;
    for (int objIndex = 0; objIndex < objNum; objIndex++) {
        if (minData.empty() || maxData.empty()) {
            for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
                auto& attr = attrs->GetElement(attrIndex);
                if (attr.attachmentType != dataType) continue;
                for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
                    minData.push_back(attr.pointer->GetElementValue(objIndex, dimensionIndex));
                    maxData.push_back(attr.pointer->GetElementValue(objIndex, dimensionIndex));
                }
            }
        } else {
            int variableIndex = 0;
            for (int attrIndex = 0; attrIndex < attrs->Size(); attrIndex++) {
                auto& attr = attrs->GetElement(attrIndex);
                if (attr.attachmentType != dataType) continue;
                for (int dimensionIndex = 0; dimensionIndex < attr.pointer->GetDimension(); dimensionIndex++) {
                    minData[variableIndex] =
                            std::min(minData[variableIndex], attr.pointer->GetElementValue(objIndex, dimensionIndex));
                    maxData[variableIndex] =
                            std::max(maxData[variableIndex], attr.pointer->GetElementValue(objIndex, dimensionIndex));
                    variableIndex++;
                }
            }
        }
    }
    return {minData, maxData};
}

void igQtParallelCoordinatesWidget::SetSelect() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtParallelCoordinatesWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1);
}

void igQtParallelCoordinatesWidget::SelectionCallbackEvent(const std::vector<Selection::Event>& _events) {
    update();
}

void igQtParallelCoordinatesWidget::SetObjectFilters(int variableNum, const std::vector<std::string>& variableName,
                                                     const std::vector<double>& filterMaxValue,
                                                     const std::vector<double>& filterMinValue) {
    for (int i = 0; i < variableNum; i++) {
        igQtParallelCoordinatesObjectFilter* pcObjFilter =
                new igQtParallelCoordinatesObjectFilter(i, filterMaxValue[i], filterMinValue[i], variableName[i], this);
        connect(pcObjFilter, &igQtParallelCoordinatesObjectFilter::ChangeMaxValue, this,
                &igQtParallelCoordinatesWidget::FilterMaxValueChanged);
        connect(pcObjFilter, &igQtParallelCoordinatesObjectFilter::ChangeMinValue, this,
                &igQtParallelCoordinatesWidget::FilterMinValueChanged);
        m_PcObjFilters.push_back(pcObjFilter);
        ui->coreHorizontalLayout->addWidget(pcObjFilter);
    }
    ui->coreHorizontalLayout->addItem(m_SpaceItem);
}

void igQtParallelCoordinatesWidget::ClearObjectFilters() {
    for (auto& pcObjFilter: m_PcObjFilters) { pcObjFilter->deleteLater(); }
    m_PcObjFilters.clear();
    ui->coreHorizontalLayout->removeItem(m_SpaceItem);
}

bool igQtParallelCoordinatesWidget::ShoultBeFilted(const std::vector<double>& objData) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    for (int i = 0; i < objData.size(); i++) {
        if (objData[i] < Data->GetFilterMinValue()[i] || Data->GetFilterMaxValue()[i] < objData[i]) return true;
    }
    return false;
}

void igQtParallelCoordinatesWidget::FilterMaxValueChanged(int number, double value) {
    if (m_CurrentModelDataIndex == -1) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    Data->FilterMaxValue()[number] = value;
    update();
}

void igQtParallelCoordinatesWidget::FilterMinValueChanged(int number, double value) {
    if (m_CurrentModelDataIndex == -1) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    Data->FilterMinValue()[number] = value;
    update();
}

void igQtParallelCoordinatesWidget::DataChooseChanged(int choosedIndex) {
    if (m_CurrentModelDataIndex == choosedIndex) return;
    m_CurrentModelDataIndex = choosedIndex;
    LoadCurrentData();
    update();
}

void igQtParallelCoordinatesWidget::RefreshData() {
    auto choosedDataIndex = m_CurrentModelDataIndex;
    UpdataData();
    if (choosedDataIndex < m_ParallelCoordinatesDatas.size()) m_CurrentModelDataIndex = choosedDataIndex;
}

void igQtParallelCoordinatesWidget::DrawParallelCoordinates() {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    std::vector<QRect> variableMaxFontPoints;
    std::vector<QRect> variableMinFontPoints;
    std::vector<QRect> variableNameFontPoints;
    std::vector<QPoint> linkTopPoints;
    std::vector<QPoint> linkBottomPoints;
    QRect background;
    bool drawAble = GetDrawFramePoints(Data->GetVariableNum(), variableMaxFontPoints, variableMinFontPoints,
                                       variableNameFontPoints, linkTopPoints, linkBottomPoints, background);
    if (!drawAble) return;
    DrawBackground(background);
    DrawStrs(variableMaxFontPoints, variableMinFontPoints, variableNameFontPoints);
    QColor lightRed = QColor::fromHsv(0, 255, Data->GetChoosedLight(), Data->GetChoosedAlpha());
    QColor darkRed = QColor::fromHsv(0, 255, Data->GetUnChoosedLight(), Data->GetUnChoosedAlpha());
    DrawLinks(linkTopPoints, linkBottomPoints, lightRed, darkRed);
    //DrawLinks(linkTopPoints, linkBottomPoints, QColor(255, 0, 0, Data->GetChoosedAlpha()),
    //          QColor(0, 0, 255, Data->GetUnChoosedAlpha()));
}

std::shared_ptr<QPainter> igQtParallelCoordinatesWidget::GetLinePainter(const QColor& color) {
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(this);
    painter->setPen(QPen(color, 1));
    return painter;
}

int igQtParallelCoordinatesWidget::GetLinePointLocation(int top, int bottom, double currentValue, double maxValue,
                                                        double minValue) {
    if (maxValue == minValue) return (top + bottom) / 2;
    return (currentValue - minValue) * (top - bottom) / (maxValue - minValue) + bottom;
}

void igQtParallelCoordinatesWidget::DrawBackground(const QRect& range) {
    QPainter painter(this);
    QBrush brush;
    brush.setColor(QColor(255, 255, 255));
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
    for (int i = 0; i < Data->GetVariableName().size(); i++) {
        painter.drawText(variableNameFontPoints[i], Qt::AlignCenter, QString(Data->GetVariableName()[i].c_str()));
    }
    for (int i = 0; i < Data->GetMaxValueInVariables().size(); i++) {
        painter.drawText(variableMaxFontPoints[i], Qt::AlignCenter, QString::number(Data->GetMaxValueInVariables()[i]));
    }
    for (int i = 0; i < Data->GetMinValueInVariables().size(); i++) {
        painter.drawText(variableMinFontPoints[i], Qt::AlignCenter, QString::number(Data->GetMinValueInVariables()[i]));
    }
}

void igQtParallelCoordinatesWidget::DrawLinks(std::vector<QPoint>& linkTopPoints,
                                              std::vector<QPoint>& linkBottomPoints, const QColor& choosedColor,
                                              const QColor& unChoosedColor) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    auto& objDatas = Data->GetObjectDatas();
    std::vector<int> choosedObjIndexs;
    if (Data->GetVariableNum() == 1) {
        for (int objIndex = 0; objIndex < objDatas.size(); objIndex++) {
            auto& objData = objDatas[objIndex];
            if (ShoultBeFilted(objData)) continue;
            if (IsChoosedObj(Data->GetDataType(), objIndex)) {
                choosedObjIndexs.push_back(objIndex);
                continue;
            }
            DrawLink(linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                     linkBottomPoints.front().y(), objData.front(), objData.back(),
                     Data->GetMaxValueInVariables().front(), Data->GetMinValueInVariables().front(),
                     Data->GetMaxValueInVariables().back(), Data->GetMinValueInVariables().back(), unChoosedColor);
        }
        for (auto& objIndex: choosedObjIndexs) {
            auto& objData = objDatas[objIndex];
            DrawLink(linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                     linkBottomPoints.front().y(), objData.front(), objData.back(),
                     Data->GetMaxValueInVariables().front(), Data->GetMinValueInVariables().front(),
                     Data->GetMaxValueInVariables().back(), Data->GetMinValueInVariables().back(), choosedColor);
        }
        return;
    }
    for (int objIndex = 0; objIndex < objDatas.size(); objIndex++) {
        auto& objData = objDatas[objIndex];
        if (ShoultBeFilted(objData)) continue;
        if (IsChoosedObj(Data->GetDataType(), objIndex)) {
            choosedObjIndexs.push_back(objIndex);
            continue;
        }
        for (int variableIndex = 0; variableIndex < objData.size() - 1; variableIndex++) {
            DrawLink(linkTopPoints[variableIndex].x(), linkTopPoints[variableIndex + 1].x(),
                     linkTopPoints[variableIndex].y(), linkBottomPoints[variableIndex].y(), objData[variableIndex],
                     objData[variableIndex + 1], Data->GetMaxValueInVariables()[variableIndex],
                     Data->GetMinValueInVariables()[variableIndex], Data->GetMaxValueInVariables()[variableIndex + 1],
                     Data->GetMinValueInVariables()[variableIndex + 1], unChoosedColor);
        }
    }
    for (auto& objIndex: choosedObjIndexs) {
        auto& objData = objDatas[objIndex];
        for (int variableIndex = 0; variableIndex < objData.size() - 1; variableIndex++) {
            DrawLink(linkTopPoints[variableIndex].x(), linkTopPoints[variableIndex + 1].x(),
                     linkTopPoints[variableIndex].y(), linkBottomPoints[variableIndex].y(), objData[variableIndex],
                     objData[variableIndex + 1], Data->GetMaxValueInVariables()[variableIndex],
                     Data->GetMinValueInVariables()[variableIndex], Data->GetMaxValueInVariables()[variableIndex + 1],
                     Data->GetMinValueInVariables()[variableIndex + 1], choosedColor);
        }
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

void igQtParallelCoordinatesWidget::DrawLink(int leftLine, int rightLine, int top, int bottom, double leftValue,
                                             double rightValue, double leftMaxValue, double leftMinValue,
                                             double rightMaxValue, double rightMinValue, const QColor& color) {
    auto painter = GetLinePainter(color);
    int leftPoint = GetLinePointLocation(top, bottom, leftValue, leftMaxValue, leftMinValue);
    int rightPoint = GetLinePointLocation(top, bottom, rightValue, rightMaxValue, rightMinValue);
    painter->drawLine(leftLine, leftPoint, rightLine, rightPoint);
}

void igQtParallelCoordinatesWidget::ChoosedAlphaSliderChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    this->update();
    if (ui->choosedAlphaSpinBox->value() != value) ui->choosedAlphaSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedAlphaSliderChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    this->update();
    if (ui->unChoosedAlphaSpinBox->value() != value) ui->unChoosedAlphaSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedAlphaSpinBoxChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedAlpha() == value) return;
    Data->SetChoosedAlpha(value);
    this->update();
    if (ui->choosedAlphaSlider->value() != value) ui->choosedAlphaSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedAlphaSpinBoxChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedAlpha() == value) return;
    Data->SetUnChoosedAlpha(value);
    this->update();
    if (ui->unChoosedAlphaSlider->value() != value) ui->unChoosedAlphaSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedLightSliderChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    this->update();
    if (ui->choosedLightSpinBox->value() != value) ui->choosedLightSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedLightSliderChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    this->update();
    if (ui->unChoosedLightSpinBox->value() != value) ui->unChoosedLightSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedLightSpinBoxChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    this->update();
    if (ui->choosedLightSlider->value() != value) ui->choosedLightSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedLightSpinBoxChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    this->update();
    if (ui->unChoosedLightSlider->value() != value) ui->unChoosedLightSlider->setValue(value);
}

bool igQtParallelCoordinatesWidget::GetDrawFramePoints(int variableNum, std::vector<QRect>& variableMaxFontPoints,
                                                       std::vector<QRect>& variableMinFontPoints,
                                                       std::vector<QRect>& variableNameFontPoints,
                                                       std::vector<QPoint>& linkTopPoints,
                                                       std::vector<QPoint>& linkBottomPoints, QRect& background) {
    constexpr int leftSpace = 5, rightSpace = 5, topSpace = 5, bottomSpace = 5;
    constexpr int stringSize = 10;
    constexpr int eachInterval = 2;
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
    if (variableNum < 1) return false;
    if (variableNum == 1) {
        int variableInterval = useableWidth;
        linkTopPoints.push_back(QPoint(startPoint.x(), variableStartTop));
        linkTopPoints.push_back(QPoint(endPoint.x(), variableStartTop));
        linkBottomPoints.push_back(QPoint(startPoint.x(), variableEndBottom));
        linkBottomPoints.push_back(QPoint(endPoint.x(), variableEndBottom));
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
    int variableInterval = useableWidth / variableNum;
    for (int i = 0; i < variableNum; i++) {
        int halfInterval = variableInterval / 2;
        int nowMidLine = startPoint.x() + i * variableInterval + halfInterval;
        linkTopPoints.push_back(QPoint(nowMidLine, variableStartTop));
        linkBottomPoints.push_back(QPoint(nowMidLine, variableEndBottom));
        variableMaxFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, maxFontStartTop),
                                              QPoint(nowMidLine + halfInterval, maxFontEndBottom)));
        variableMinFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, minFontStartTop),
                                              QPoint(nowMidLine + halfInterval, minFontEndBottom)));
        variableNameFontPoints.push_back(QRect(QPoint(nowMidLine - halfInterval, nameFontStartTop),
                                               QPoint(nowMidLine + halfInterval, nameFontEndBottom)));
    }
    return true;
}
