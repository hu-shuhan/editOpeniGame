#include "iGameHistogramPicker.h"
#include <iGameCtxPresObjData.h>
IGAME_NAMESPACE_BEGIN
static double RoadNumPer = 0.35;
static double GetPer = 0.2;
static double HalfGetPer = GetPer / 2;

static int CalculateBoxIndexByValue(int boxNum, double value, double maxValue, double minValue) {
    if (maxValue == minValue) { return boxNum / 2; }
    double binWidth = (maxValue - minValue) / boxNum;

    int index = static_cast<int>((value - minValue) / binWidth);

    if (index < 0) return 0;
    if (index >= boxNum) return boxNum - 1;

    return index;
}

static std::pair<double, double> CalculateMinMaxValueByBoxIndex(int boxNum, int boxIndex, double maxValue,
                                                                double minValue) {
    if (maxValue == minValue) return {minValue, maxValue};
    double binWidth = (maxValue - minValue) / boxNum;
    double leftValue = minValue + binWidth * boxIndex;
    double rightValue = leftValue + binWidth;
    return {leftValue, rightValue};
}

const std::vector<double>& HistogramPicker::GetDensity() { return m_Density; }

HistogramPicker::HistogramPicker(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                 const std::pair<int, int>& variableIndex, int objNum, int boxNum, int maxKeyObjNum,
                                 double minValue, double maxValue) {
    auto keyObjIds = CtxPresObjData_Main::GenerateKeyObjectIds(objNum, maxKeyObjNum);
    m_MinValue = minValue;
    m_MaxValue = maxValue;
    m_Density = GenerateDensity(attrs, variableIndex, keyObjIds, boxNum, minValue, maxValue);
    m_RoadNum = boxNum * RoadNumPer;
}

std::pair<double, double> HistogramPicker::CalculateMinMaxValueToPick(double value) {
    int mainBoxIndex = CalculateBoxIndexByValue(m_Density.size(), value, m_MaxValue, m_MinValue);
    int leftIndex{mainBoxIndex}, rightIndex{mainBoxIndex};
    double accumulatedLeftPer{};
    int leftRoad = m_RoadNum;
    double accumulatedRightPer{};
    int rightRoad = m_RoadNum;

    while (leftRoad > 0 && accumulatedLeftPer <= HalfGetPer && leftIndex - 1 >= 0) {
        leftIndex--;
        leftRoad--;
        auto& currentDensity = m_Density[leftIndex];
        if (accumulatedLeftPer + currentDensity > HalfGetPer) break;
        accumulatedLeftPer += currentDensity;
    }
    while (rightRoad > 0 && accumulatedRightPer <= HalfGetPer && rightIndex + 1 < m_Density.size()) {
        rightIndex++;
        rightRoad--;
        auto& currentDensity = m_Density[rightIndex];
        if (accumulatedRightPer + currentDensity > HalfGetPer) break;
        accumulatedRightPer += currentDensity;
    }

    double leftValue = CalculateMinMaxValueByBoxIndex(m_Density.size(), leftIndex, m_MaxValue, m_MinValue).first;
    double rightValue = CalculateMinMaxValueByBoxIndex(m_Density.size(), rightIndex, m_MaxValue, m_MinValue).second;
    return {leftValue, rightValue};
}

std::vector<double> HistogramPicker::GenerateDensity(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                     const std::pair<int, int>& variableIndex,
                                                     const std::vector<int>& objIds, int boxNum, double minValue,
                                                     double maxValue) {
    std::vector<double> re(boxNum, 0);
    if (objIds.empty()) return re;
    for (int i = 0; i < objIds.size(); i++) {
        auto& objId = objIds[i];
        auto objValue = CtxPresObjData_Main::GenerateObjData(objId, attrs, variableIndex);
        int boxIndex = CalculateBoxIndexByValue(boxNum, objValue, maxValue, minValue);
        re[boxIndex]++;
    }
    for (auto& v: re) { v /= objIds.size(); }
    return re;
}
IGAME_NAMESPACE_END