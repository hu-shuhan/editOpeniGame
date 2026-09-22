#ifndef IGAMEVIS_IVOLUMETRANSFERFUNCTION_H
#define IGAMEVIS_IVOLUMETRANSFERFUNCTION_H

#include "iGameObject.h"
#include "iGameScalarsToColors.h"
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameVolumeTransferFunction
 * @brief 分段颜色 + 不透明度传输函数，烘焙成 1D RGBA LUT（GPU/CPU 后端共用）。
 *
 * @details
 *  颜色复用 iGameVis 现有的 ScalarsToColors（ColorMap 控制点 + 范围）；
 *  不透明度使用独立的分段线性控制点。两者统一在标量归一化域 [0,1] 上求值，
 *  BakeLUT 把 [0,1] -> RGBA8 的映射烘焙成一张一维表，供着色器/CPU 光线步进直接查表。
 *  本类不依赖 OpenGL，产出的 LUT 缓冲由渲染器负责上传。
 */
class iGameVolumeTransferFunction : public Object {
public:
    I_OBJECT(iGameVolumeTransferFunction);
    static Pointer New() { return new iGameVolumeTransferFunction; }

    struct OpacityPoint {
        float value;   ///< 归一化标量位置 [0,1]
        float opacity; ///< 不透明度 [0,1]
    };

    /** 设置标量数据范围（用于 LUT 索引归一化的上下界）。 */
    void SetScalarRange(double minValue, double maxValue);
    double GetScalarMin() const { return m_ScalarMin; }
    double GetScalarMax() const { return m_ScalarMax; }

    /** 设置/获取颜色映射器（复用 ColorMap 的控制点与范围）。 */
    void SetColorMapper(ScalarsToColors::Pointer mapper);
    ScalarsToColors::Pointer GetColorMapper() const { return m_ColorMapper; }

    /** 是否按标量映射不透明度；关闭时使用固定不透明度。 */
    void SetOpacityMappingEnabled(bool enabled);
    bool GetOpacityMappingEnabled() const { return m_OpacityMappingEnabled; }

    /** 清空不透明度控制点。 */
    void ClearOpacityPoints();
    /** 添加一个不透明度控制点（value 为归一化 [0,1]）。 */
    void AddOpacityPoint(float value, float opacity);
    /** 使用默认三段不透明度（低值 0 -> 中值 0.2 -> 高值 1.0）。 */
    void SetDefaultOpacityPoints();

    /** 分段线性求值归一化 value 的不透明度。 */
    float MapOpacity(float value) const;

    /** 设置 LUT 分辨率（默认 1024）。 */
    void SetLUTResolution(int resolution);
    int GetLUTResolution() const { return m_LUTResolution; }

    /**
     * 烘焙 RGBA8 LUT。返回 resolution*4 个字节，每个像素依次为 R,G,B,A。
     * 颜色来自 ColorMapper（归一化域 [0,1]），不透明度来自 MapOpacity。
     */
    std::vector<unsigned char> BakeLUT() const;

protected:
    iGameVolumeTransferFunction();
    ~iGameVolumeTransferFunction() override = default;

private:
    double m_ScalarMin{0.0};
    double m_ScalarMax{1.0};
    ScalarsToColors::Pointer m_ColorMapper{nullptr};
    bool m_OpacityMappingEnabled{true};
    std::vector<OpacityPoint> m_OpacityPoints;
    int m_LUTResolution{1024};
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_IVOLUMETRANSFERFUNCTION_H
