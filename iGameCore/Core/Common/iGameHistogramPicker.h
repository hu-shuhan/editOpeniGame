#pragma once
#include <iGameMacro.h>
#include <utility>
#include <vector>
#include <iGameAttributeSet.h>
IGAME_NAMESPACE_BEGIN
class HistogramPicker {
public:
    HistogramPicker() = default;
    HistogramPicker(ElementArray<AttributeSet::Attribute>::Pointer attrs, const std::pair<int, int>& variableIndex,
                    int objNum, int boxNum, int maxKeyObjNum, double minValue, double maxValue);

public:
    const std::vector<double>& GetDensity();
    std::pair<double, double> CalculateMinMaxValueToPick(double value);

private:
    double m_MinValue{};
    double m_MaxValue{};
    std::vector<double> m_Density;
    int m_RoadNum{};
    int m_SmallRoadNum{};

public:
    static std::vector<double> GenerateDensity(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                               const std::pair<int, int>& variableIndex, const std::vector<int>& objIds,
                                               int boxNum, double minValue, double maxValue);
};

IGAME_NAMESPACE_END