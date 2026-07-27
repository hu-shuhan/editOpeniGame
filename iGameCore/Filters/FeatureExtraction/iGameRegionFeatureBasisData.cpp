#include "iGameRegionFeatureBasisData.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

IGAME_NAMESPACE_BEGIN

void RegionFeatureBasisData::SetValues(std::vector<double> values) {
    m_Values = std::move(values);
    m_Histogram.clear();

    if (m_Values.empty()) {
        m_MinValue = 0.0;
        m_MaxValue = 0.0;
        return;
    }

    auto minMax = std::minmax_element(m_Values.begin(), m_Values.end());
    m_MinValue = *minMax.first;
    m_MaxValue = *minMax.second;
}

double RegionFeatureBasisData::ValueForPercent(int percent) const {
    if (m_Values.empty()) return 0.0;
    const int boundedPercent = std::clamp(percent, 0, 100);
    const double t = static_cast<double>(boundedPercent) / 100.0;
    return m_MinValue + (m_MaxValue - m_MinValue) * t;
}

void RegionFeatureBasisData::BuildHistogram(int binCount) {
    m_Histogram.clear();
    if (binCount <= 0 || m_Values.empty()) return;

    m_Histogram.assign(static_cast<std::size_t>(binCount), 0.0);
    if (m_MinValue == m_MaxValue) {
        m_Histogram[static_cast<std::size_t>(binCount / 2)] = 1.0;
        return;
    }

    const double range = m_MaxValue - m_MinValue;
    for (double value : m_Values) {
        if (!std::isfinite(value)) continue;
        double scaled = (value - m_MinValue) / range;
        int bin = static_cast<int>(scaled * static_cast<double>(binCount));
        bin = std::clamp(bin, 0, binCount - 1);
        m_Histogram[static_cast<std::size_t>(bin)] += 1.0;
    }

    const auto maxIt = std::max_element(m_Histogram.begin(), m_Histogram.end());
    if (maxIt == m_Histogram.end() || *maxIt <= std::numeric_limits<double>::epsilon()) return;
    for (double& density : m_Histogram) density /= *maxIt;
}

std::vector<igIndex> RegionFeatureBasisData::PickValueRange(double minValue, double maxValue) const {
    if (minValue > maxValue) std::swap(minValue, maxValue);

    std::vector<igIndex> ids;
    ids.reserve(m_Values.size());
    for (std::size_t i = 0; i < m_Values.size(); ++i) {
        const double value = m_Values[i];
        if (value >= minValue && value <= maxValue) ids.push_back(static_cast<igIndex>(i));
    }
    return ids;
}

std::vector<igIndex> RegionFeatureBasisData::PickPercentRange(int lowerPercent, int upperPercent) const {
    const double lowerValue = ValueForPercent(lowerPercent);
    const double upperValue = ValueForPercent(upperPercent);
    return PickValueRange(lowerValue, upperValue);
}

IGAME_NAMESPACE_END
