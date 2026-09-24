#include "iGameVolumeTransferFunction.h"

#include <algorithm>

IGAME_NAMESPACE_BEGIN

iGameVolumeTransferFunction::iGameVolumeTransferFunction() {
    SetDefaultOpacityPoints();
    // 默认配色用 Fast（ParaView 彩虹，迁移自 UnifiedVersion ColorMapManager），
    // 体渲染观感优于 ColorMap 默认的蓝白红；如需自定义可再调 SetColorMapper 覆盖。
    m_ColorMapper = ScalarsToColors::New();
    m_ColorMapper->InitColorBarWithFastType();
}

void iGameVolumeTransferFunction::SetScalarRange(double minValue,
                                                 double maxValue) {
    if (maxValue <= minValue) { maxValue = minValue + 1e-6; }
    m_ScalarMin = minValue;
    m_ScalarMax = maxValue;
}

void iGameVolumeTransferFunction::SetColorMapper(
        ScalarsToColors::Pointer mapper) {
    m_ColorMapper = mapper;
}

void iGameVolumeTransferFunction::SetOpacityMappingEnabled(bool enabled) {
    m_OpacityMappingEnabled = enabled;
}

void iGameVolumeTransferFunction::ClearOpacityPoints() {
    m_OpacityPoints.clear();
}

void iGameVolumeTransferFunction::AddOpacityPoint(float value, float opacity) {
    value = std::clamp(value, 0.0f, 1.0f);
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    m_OpacityPoints.push_back(OpacityPoint{value, opacity});
    std::sort(m_OpacityPoints.begin(), m_OpacityPoints.end(),
              [](const OpacityPoint& a, const OpacityPoint& b) {
                  return a.value < b.value;
              });
}

void iGameVolumeTransferFunction::SetDefaultOpacityPoints() {
    m_OpacityPoints.clear();
    // 对标 VolumeRenderingCommon.h：低值 0 -> 中值 0.2 -> 高值 1.0。
    // 中间控制点在 PVR_REF 里是 gMin + 0.6*(gMax-gMin)（归一化 0.6），这里保持一致。
    m_OpacityPoints.push_back(OpacityPoint{0.0f, 0.0f});
    m_OpacityPoints.push_back(OpacityPoint{0.6f, 0.2f});
    m_OpacityPoints.push_back(OpacityPoint{1.0f, 1.0f});
}

float iGameVolumeTransferFunction::MapOpacity(float value) const {
    if (!m_OpacityMappingEnabled) { return 1.0f; }
    value = std::clamp(value, 0.0f, 1.0f);
    if (m_OpacityPoints.empty()) { return 1.0f; }
    if (value <= m_OpacityPoints.front().value) {
        return m_OpacityPoints.front().opacity;
    }
    if (value >= m_OpacityPoints.back().value) {
        return m_OpacityPoints.back().opacity;
    }
    for (size_t i = 1; i < m_OpacityPoints.size(); ++i) {
        const auto& a = m_OpacityPoints[i - 1];
        const auto& b = m_OpacityPoints[i];
        if (value <= b.value) {
            const float span = b.value - a.value;
            if (span <= 1e-8f) { return b.opacity; }
            const float t = (value - a.value) / span;
            return a.opacity * (1.0f - t) + b.opacity * t;
        }
    }
    return m_OpacityPoints.back().opacity;
}

void iGameVolumeTransferFunction::SetLUTResolution(int resolution) {
    if (resolution < 2) { resolution = 2; }
    m_LUTResolution = resolution;
}

std::vector<unsigned char> iGameVolumeTransferFunction::BakeLUT() const {
    std::vector<unsigned char> lut(static_cast<size_t>(m_LUTResolution) * 4);
    const int res = m_LUTResolution;

    for (int i = 0; i < res; ++i) {
        const float t = res <= 1 ? 0.0f
                                 : static_cast<float>(i) /
                                           static_cast<float>(res - 1);

        float rgb[3]{0.5f, 0.5f, 0.5f};
        if (m_ColorMapper) {
            // ColorMap::MapColor 期望归一化 [0,1] 输入。
            m_ColorMapper->MapColor(t, rgb);
        }

        const float alpha = MapOpacity(t);

        auto toUChar = [](float v) {
            const float c = std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f;
            return static_cast<unsigned char>(c);
        };

        const size_t base = static_cast<size_t>(i) * 4;
        lut[base + 0] = toUChar(rgb[0]);
        lut[base + 1] = toUChar(rgb[1]);
        lut[base + 2] = toUChar(rgb[2]);
        lut[base + 3] = toUChar(alpha);
    }
    return lut;
}

IGAME_NAMESPACE_END
