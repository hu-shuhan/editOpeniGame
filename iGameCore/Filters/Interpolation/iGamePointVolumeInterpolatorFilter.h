#ifndef iGamePointVolumeInterpolatorFilter_h
#define iGamePointVolumeInterpolatorFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGamePointInterpolationKernel.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

// 点体积插值（Point Volume Interpolator）：
// 把输入点云/数据集上的点属性，按核函数（Voronoi / Shepard / Gaussian）
// 插值到一个规则体网格（StructuredMesh）的格点上。
// 语义对齐 ParaView 的 "Point Volume Interpolator"（vtkPointInterpolator）。
class PointVolumeInterpolatorFilter : public Filter {
public:
    I_OBJECT(PointVolumeInterpolatorFilter);
    static Pointer New() { return new PointVolumeInterpolatorFilter; }

    bool Execute() override;

    void SetKernelType(PointKernelType type) { m_KernelType = type; }
    PointKernelType GetKernelType() const { return m_KernelType; }

    void SetKernelFootprint(PointKernelFootprint footprint) { m_KernelFootprint = footprint; }
    PointKernelFootprint GetKernelFootprint() const { return m_KernelFootprint; }

    void SetRadius(double radius) { m_Radius = radius; }
    double GetRadius() const { return m_Radius; }

    void SetNumberOfPoints(int numberOfPoints) { m_NumberOfPoints = numberOfPoints; }
    int GetNumberOfPoints() const { return m_NumberOfPoints; }

    void SetSharpness(double sharpness) { m_Sharpness = sharpness; }
    double GetSharpness() const { return m_Sharpness; }

    void SetPowerParameter(double power) { m_PowerParameter = power; }
    double GetPowerParameter() const { return m_PowerParameter; }

    void SetNullPointsStrategy(PointNullPointsStrategy strategy) { m_NullPointsStrategy = strategy; }
    PointNullPointsStrategy GetNullPointsStrategy() const { return m_NullPointsStrategy; }

    void SetNullValue(double value) { m_NullValue = value; }
    double GetNullValue() const { return m_NullValue; }

    // 要插值的点属性名列表；为空表示插值全部 POINT 数组。
    void SetInterpolateArrayNames(const std::vector<std::string>& names) {
        m_InterpolateArrayNames = names;
    }
    const std::vector<std::string>& GetInterpolateArrayNames() const {
        return m_InterpolateArrayNames;
    }

    void SetUseInputBounds(bool use) { m_UseInputBounds = use; }
    bool GetUseInputBounds() const { return m_UseInputBounds; }

    void SetSamplingBounds(const double bounds[6]);
    void SetSamplingBounds(double x0, double x1, double y0, double y1, double z0, double z1);
    double* GetSamplingBounds() { return m_SamplingBounds; }
    void GetSamplingBounds(double bounds[6]) const;

    void SetResolution(int i, int j, int k);
    void SetResolution(int dims[3]);
    int* GetResolution() { return m_Resolution; }
    void GetResolution(int dims[3]) const;

    // 输出规则体网格的有效点掩码数组名（对齐 ParaView / vtkProbeFilter 语义）。
    static constexpr const char* ValidPointsMaskName = "vtkValidPointMask";

    std::string GetMessage() const { return m_Message; }

protected:
    PointVolumeInterpolatorFilter();
    ~PointVolumeInterpolatorFilter() override = default;

private:
    PointKernelType m_KernelType{PointKernelType::Voronoi};
    PointKernelFootprint m_KernelFootprint{PointKernelFootprint::Radius};
    double m_Radius{1.0};
    int m_NumberOfPoints{8};
    double m_Sharpness{2.0};
    double m_PowerParameter{2.0};
    PointNullPointsStrategy m_NullPointsStrategy{PointNullPointsStrategy::NullValue};
    double m_NullValue{0.0};

    std::vector<std::string> m_InterpolateArrayNames;

    bool m_UseInputBounds{true};
    double m_SamplingBounds[6]{0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
    int m_Resolution[3]{64, 64, 64};

    std::string m_Message;
};

IGAME_NAMESPACE_END

#endif
