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

/**
 * @class   igQtParallelCoordinatesWidget
 * @brief   igQtParallelCoordinatesWidget's brief
 */

using namespace std;

const float EPSILON = 1e-6f;

static void rgbToHsb(float r, float g, float b, float& h, float& s, float& bVal) {
    float max_c = std::max({r, g, b});
    float min_c = std::min({r, g, b});
    float delta = max_c - min_c;

    bVal = max_c;

    if (max_c != 0.0f) {
        s = delta / max_c;
    } else {
        s = 0.0f;
        h = 0.0f;
        return;
    }

    if (delta == 0.0f) {
        h = 0.0f;
        return;
    }

    if (max_c == r) {
        h = 60.0f * std::fmod((g - b) / delta, 6.0f);
    } else if (max_c == g) {
        h = 60.0f * ((b - r) / delta + 2.0f);
    } else { // max_c == b
        h = 60.0f * ((r - g) / delta + 4.0f);
    }
}

static void hsbToRgb(float h, float s, float bVal, float& r, float& g, float& b) {
    float c = bVal * s;
    float x = c * (1.0f - std::fabs(std::fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = bVal - c;

    float r1, g1, b1;

    if (h >= 0.0f - EPSILON && h < 60.0f + EPSILON) {
        r1 = c + m;
        g1 = x + m;
        b1 = 0.0f + m;
    } else if (h >= 60.0f - EPSILON && h < 120.0f + EPSILON) {
        r1 = x + m;
        g1 = c + m;
        b1 = 0.0f + m;
    } else if (h >= 120.0f - EPSILON && h < 180.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = c + m;
        b1 = x + m;
    } else if (h >= 180.0f - EPSILON && h < 240.0f + EPSILON) {
        r1 = 0.0f + m;
        g1 = x + m;
        b1 = c + m;
    } else if (h >= 240.0f - EPSILON && h < 300.0f + EPSILON) {
        r1 = x + m;
        g1 = 0.0f + m;
        b1 = c + m;
    } else {
        r1 = c + m;
        g1 = 0.0f + m;
        b1 = x + m;
    }

    r = r1;
    g = g1;
    b = b1;
}

static void ChangeRgbBrightness(float& r, float& g, float& b, float targetBrightness) {
    float h, s, currentB;

    rgbToHsb(r, g, b, h, s, currentB);

    if (std::fabs(currentB - targetBrightness) < EPSILON) { return; }

    float newB = std::max(0.0f, std::min(1.0f, targetBrightness));

    hsbToRgb(h, s, newB, r, g, b);
}

static tuple<int, int, int> ChangeBrightness(float r, float g, float b, int brightness) {
    float targetBrightness = (float) brightness / 255.0;
    ChangeRgbBrightness(r, g, b, targetBrightness);
    int ir = r * 255;
    int ig = g * 255;
    int ib = b * 255;
    return {ir, ig, ib};
}

static tuple<int, int, int> GetRedColor(int brightNess) {
    QColor red = QColor::fromHsv(0, 255, brightNess);
    int r{}, g{}, b{};
    red.getRgb(&r, &g, &b);
    return {r, g, b};
}

static inline QColor GetQColorFromTuple(const tuple<int, int, int>& rgb, int alpha) {
    return QColor(get<0>(rgb), get<1>(rgb), get<2>(rgb), alpha);
}

static vector<tuple<int, int, int>> GenerateObjColors(int variableIndex, const vector<vector<double>>& objDatas,
                                                      const vector<double>& maxValues, const vector<double>& minValues,
                                                      int brightness, ScalarsToColors::Pointer colorMap) {
    vector<tuple<int, int, int>> re;
    if (variableIndex < 0 || objDatas.empty() || objDatas.front().size() <= variableIndex) return re;
    float shift = 0 - minValues[variableIndex];
    float scale = 1.0 / (maxValues[variableIndex] - minValues[variableIndex]);
    float rgb[3]{};
    for (auto& objData: objDatas) {
        auto& value = objData[variableIndex];
        colorMap->GetColor(value, rgb, shift, scale);
        re.push_back(ChangeBrightness(rgb[0], rgb[1], rgb[2], brightness));
    }
    return re;
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
    return CalculateBackgroundColor(colors);
}

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
    connect(ui->colorChoose, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &igQtParallelCoordinatesWidget::ColorChooseChanged);
    connect(ui->refreshData, &QPushButton::clicked, this, &igQtParallelCoordinatesWidget::RefreshData);
    connect(ui->variableSort, &QPushButton::clicked, this, &igQtParallelCoordinatesWidget::SetVariableSort);
}
igQtParallelCoordinatesWidget::~igQtParallelCoordinatesWidget() {}

void igQtParallelCoordinatesWidget::paintEvent(QPaintEvent* QPE) {
    if (m_ParallelCoordinatesDatas.size() == 0 || m_CurrentModelDataIndex == -1) return;
    DrawParallelCoordinates();
}

void igQtParallelCoordinatesWidget::SetParallelCoordinates(Model::Pointer model) {
    m_Model = model;
    m_Mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(m_Model->GetDataObject());
    SetSelectionCallback();
    UpdateData();
    update();
}

void igQtParallelCoordinatesWidget::UpdateData() {
    GenerateModelDatas();
    SetDataChoosedComboBox();
    SetColorComboBox();
    LoadCurrentData();
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
    Data->SetObjectChoosedColor(GenerateObjColors(m_ColorVariableIndex, Data->GetObjectDatas(),
                                                  Data->GetMaxValueInVariables(), Data->GetMinValueInVariables(),
                                                  Data->GetChoosedLight(), colorMap));
}

void igQtParallelCoordinatesWidget::UpdateUnChoosedColor() {
    if (m_CurrentModelDataIndex < 0 || m_ParallelCoordinatesDatas.size() <= m_CurrentModelDataIndex) return;
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    auto colorMap = m_Mesh->GetColorMapper();
    Data->SetObjectUnChoosedColor(GenerateObjColors(m_ColorVariableIndex, Data->GetObjectDatas(),
                                                    Data->GetMaxValueInVariables(), Data->GetMinValueInVariables(),
                                                    Data->GetUnChoosedLight(), colorMap));
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
    parallelCoordinatesData->SetVariableSort(GetDefaultVariableSort(variableNum));
    parallelCoordinatesData->SetVariableName(variableNames);
    auto objDatas = GetObjectDatas(dataType);
    parallelCoordinatesData->SetObjectData(objDatas);
    parallelCoordinatesData->SetObjectDrawSorts(GetObjectDrawSorts(variableNum, objDatas));
    parallelCoordinatesData->SetDefaultChoosedColor(GetRedColor(parallelCoordinatesData->GetChoosedLight()));
    parallelCoordinatesData->SetDefaultUnChoosedColor(GetRedColor(parallelCoordinatesData->GetUnChoosedLight()));
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
            ss << attr.pointer->GetName() << "_" << j;
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

std::vector<std::vector<int>>
igQtParallelCoordinatesWidget::GetObjectDrawSorts(int variableNum,
                                                  const std::vector<std::vector<double>>& objcetValues) {
    std::vector<std::vector<int>> re(variableNum, std::vector<int>(objcetValues.size(), 0));
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        for (int objIndex = 0; objIndex < objcetValues.size(); objIndex++) { re[variableIndex][objIndex] = objIndex; }
    }
    for (int variableIndex = 0; variableIndex < variableNum; variableIndex++) {
        std::sort(re[variableIndex].begin(), re[variableIndex].end(), [&](int objIdA, int objIdB) {
            return objcetValues[objIdA][variableIndex] < objcetValues[objIdB][variableIndex];
        });
    }
    return re;
}

std::vector<int> igQtParallelCoordinatesWidget::GetDefaultVariableSort(int variableNum) {
    std::vector<int> re(variableNum);
    for (int i = 0; i < variableNum; i++) re[i] = i;
    return re;
}

void igQtParallelCoordinatesWidget::SetSelectionCallback() {
    auto selection = m_Model->GetSelection();
    selection->SetSelectionCallBackEvent(&igQtParallelCoordinatesWidget::SelectionCallbackEvent, this,
                                         std::placeholders::_1);
}

void igQtParallelCoordinatesWidget::SelectionCallbackEvent(const std::vector<Selection::Event>& _events) {
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
    SetColorComboBox();
    LoadCurrentData();
    update();
}

void igQtParallelCoordinatesWidget::ColorChooseChanged(int choosedIndex) {
    m_ColorVariableIndex = choosedIndex - 1;
    UpdateColor();
    UpdateBackgroundColor();
    update();
}

void igQtParallelCoordinatesWidget::RefreshData() {
    auto choosedDataIndex = m_CurrentModelDataIndex;
    UpdateData();
    UpdateColor();
    UpdateBackgroundColor();
    update();
    if (choosedDataIndex < m_ParallelCoordinatesDatas.size()) m_CurrentModelDataIndex = choosedDataIndex;
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
    update();
}

void igQtParallelCoordinatesWidget::DrawParallelCoordinates() {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    std::vector<QRect> variableMaxFontPoints;
    std::vector<QRect> variableMinFontPoints;
    std::vector<QRect> variableNameFontPoints;
    std::vector<QPoint> linkTopPoints;
    std::vector<QPoint> linkBottomPoints;
    QRect background;
    bool drawAble = GetDrawFramePoints(Data->GetVariableSort().size(), variableMaxFontPoints, variableMinFontPoints,
                                       variableNameFontPoints, linkTopPoints, linkBottomPoints, background);
    if (!drawAble) return;
    DrawBackground(background);
    DrawStrs(variableMaxFontPoints, variableMinFontPoints, variableNameFontPoints);
    DrawLinks(linkTopPoints, linkBottomPoints);
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

void igQtParallelCoordinatesWidget::DrawLinks(std::vector<QPoint>& linkTopPoints,
                                              std::vector<QPoint>& linkBottomPoints) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    auto& objDatas = Data->GetObjectDatas();
    std::vector<int> choosedObjIndexs;
    std::shared_ptr<QPainter> painter = make_shared<QPainter>(this);
    auto& drawSort = Data->GetObjectDrawSorts()[std::max(0, m_ColorVariableIndex)];
    if (Data->GetVariableSort().size() == 1) {
        int variableIndex = Data->GetVariableSort().front();
        for (auto& objIndex: drawSort) {
            auto& objData = objDatas[objIndex];
            if (ShoultBeFilted(objData)) continue;
            if (IsChoosedObj(Data->GetDataType(), objIndex)) {
                choosedObjIndexs.push_back(objIndex);
                continue;
            }
            painter->setPen(QPen(GetQColorFromTuple(Data->GetObjColor(false, objIndex), Data->GetUnChoosedAlpha()), 1));
            DrawLink(linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                     linkBottomPoints.front().y(), objData[variableIndex], objData[variableIndex],
                     Data->GetMaxValueInVariables()[variableIndex], Data->GetMinValueInVariables()[variableIndex],
                     Data->GetMaxValueInVariables()[variableIndex], Data->GetMinValueInVariables()[variableIndex],
                     painter);
        }
        for (auto& objIndex: choosedObjIndexs) {
            auto& objData = objDatas[objIndex];
            painter->setPen(QPen(GetQColorFromTuple(Data->GetObjColor(true, objIndex), Data->GetChoosedAlpha()), 1));
            DrawLink(linkTopPoints.front().x(), linkTopPoints.back().x(), linkTopPoints.front().y(),
                     linkBottomPoints.front().y(), objData[variableIndex], objData[variableIndex],
                     Data->GetMaxValueInVariables()[variableIndex], Data->GetMinValueInVariables()[variableIndex],
                     Data->GetMaxValueInVariables()[variableIndex], Data->GetMinValueInVariables()[variableIndex],
                     painter);
        }
        return;
    }
    for (auto& objIndex: drawSort) {
        auto& objData = objDatas[objIndex];
        if (ShoultBeFilted(objData)) continue;
        if (IsChoosedObj(Data->GetDataType(), objIndex)) {
            choosedObjIndexs.push_back(objIndex);
            continue;
        }
        painter->setPen(QPen(GetQColorFromTuple(Data->GetObjColor(false, objIndex), Data->GetUnChoosedAlpha()), 1));
        for (int sortIndex = 0; sortIndex < Data->GetVariableSort().size() - 1; sortIndex++) {
            int variableIndexA = Data->GetVariableSort()[sortIndex];
            int variableIndexB = Data->GetVariableSort()[sortIndex + 1];
            DrawLink(linkTopPoints[sortIndex].x(), linkTopPoints[sortIndex + 1].x(), linkTopPoints[sortIndex].y(),
                     linkBottomPoints[sortIndex].y(), objData[variableIndexA], objData[variableIndexB],
                     Data->GetMaxValueInVariables()[variableIndexA], Data->GetMinValueInVariables()[variableIndexA],
                     Data->GetMaxValueInVariables()[variableIndexB], Data->GetMinValueInVariables()[variableIndexB],
                     painter);
        }
    }
    for (auto& objIndex: choosedObjIndexs) {
        auto& objData = objDatas[objIndex];
        painter->setPen(QPen(GetQColorFromTuple(Data->GetObjColor(true, objIndex), Data->GetChoosedAlpha()), 1));
        for (int sortIndex = 0; sortIndex < Data->GetVariableSort().size() - 1; sortIndex++) {
            int variableIndexA = Data->GetVariableSort()[sortIndex];
            int variableIndexB = Data->GetVariableSort()[sortIndex + 1];
            DrawLink(linkTopPoints[sortIndex].x(), linkTopPoints[sortIndex + 1].x(), linkTopPoints[sortIndex].y(),
                     linkBottomPoints[sortIndex].y(), objData[variableIndexA], objData[variableIndexB],
                     Data->GetMaxValueInVariables()[variableIndexA], Data->GetMinValueInVariables()[variableIndexA],
                     Data->GetMaxValueInVariables()[variableIndexB], Data->GetMinValueInVariables()[variableIndexB],
                     painter);
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
                                             double rightMaxValue, double rightMinValue,
                                             const std::shared_ptr<QPainter>& painter) {
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
    Data->SetDefaultChoosedColor(GetRedColor(value));
    UpdateChoosedColor();
    this->update();
    if (ui->choosedLightSpinBox->value() != value) ui->choosedLightSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedLightSliderChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    Data->SetDefaultUnChoosedColor(GetRedColor(value));
    UpdateUnChoosedColor();
    this->update();
    if (ui->unChoosedLightSpinBox->value() != value) ui->unChoosedLightSpinBox->setValue(value);
}

void igQtParallelCoordinatesWidget::ChoosedLightSpinBoxChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetChoosedLight() == value) return;
    Data->SetChoosedLight(value);
    Data->SetDefaultChoosedColor(GetRedColor(value));
    UpdateChoosedColor();
    this->update();
    if (ui->choosedLightSlider->value() != value) ui->choosedLightSlider->setValue(value);
}

void igQtParallelCoordinatesWidget::UnChoosedLightSpinBoxChanged(int value) {
    auto& Data = m_ParallelCoordinatesDatas[m_CurrentModelDataIndex];
    if (Data->GetUnChoosedLight() == value) return;
    Data->SetUnChoosedLight(value);
    Data->SetDefaultUnChoosedColor(GetRedColor(value));
    UpdateUnChoosedColor();
    this->update();
    if (ui->unChoosedLightSlider->value() != value) ui->unChoosedLightSlider->setValue(value);
}

bool igQtParallelCoordinatesWidget::GetDrawFramePoints(int variableSortSize, std::vector<QRect>& variableMaxFontPoints,
                                                       std::vector<QRect>& variableMinFontPoints,
                                                       std::vector<QRect>& variableNameFontPoints,
                                                       std::vector<QPoint>& linkTopPoints,
                                                       std::vector<QPoint>& linkBottomPoints, QRect& background) {
    constexpr int leftSpace = 5, rightSpace = 5, topSpace = 5, bottomSpace = 5;
    constexpr int stringSize = 10;
    constexpr int eachInterval = 2;
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
    if (variableSortSize == 1) {
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
    int variableInterval = useableWidth / variableSortSize;
    for (int i = 0; i < variableSortSize; i++) {
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
