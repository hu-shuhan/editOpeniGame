/**
 * @class   iGamePerlinNoise
 * @brief   在输入数据集的每个点上采样 3D Perlin 噪声，输出一个独立的深拷贝网格，
 *          并为其点属性新增单分量标量数组（默认名 "PerlinNoise"）。
 *
 *          几何与拓扑保持不变，原模型不受影响。
 *
 *          采样公式：
 *              xd[i] = x[i] * Frequency[i] - Phase[i] * 2
 *              value = PerlinNoise(xd) * Amplitude
 *          其中 PerlinNoise 为 Greg Ward 在 Graphics Gems II 中的实现
 *          （整数格点随机值 + 三线性 / hermite 插值）。
 */

#pragma once

#include "iGameFilter.h"
#include "iGameDataObject.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class PerlinNoiseFilter : public Filter {
public:
    I_OBJECT(PerlinNoiseFilter)
    static Pointer New() { return new PerlinNoiseFilter; }

    bool Execute() override;

    /// 噪声振幅（默认 1.0）
    void SetAmplitude(double amplitude);
    double GetAmplitude() const;

    /// 噪声频率（默认 (1,1,1)）
    void SetFrequency(double x, double y, double z);
    void SetFrequency(const double frequency[3]);
    void GetFrequency(double frequency[3]) const;

    /// 噪声相位（默认 (0,0,0)）
    void SetPhase(double x, double y, double z);
    void SetPhase(const double phase[3]);
    void GetPhase(double phase[3]) const;

    /// 输出标量数组名（默认 "PerlinNoise"）
    void SetScalarArrayName(const std::string& name);
    const std::string& GetScalarArrayName() const;

protected:
    PerlinNoiseFilter();
    ~PerlinNoiseFilter() override = default;

    double m_Amplitude{1.0};
    double m_Frequency[3]{1.0, 1.0, 1.0};
    double m_Phase[3]{0.0, 0.0, 0.0};
    std::string m_ScalarArrayName{"PerlinNoise"};
};

/**
 * @brief 单独的 Perlin 噪声求值，供单元测试与其它场景复用。
 */
double EvaluatePerlinNoise(const double x[3], const double frequency[3],
                           const double phase[3], double amplitude);

IGAME_NAMESPACE_END
