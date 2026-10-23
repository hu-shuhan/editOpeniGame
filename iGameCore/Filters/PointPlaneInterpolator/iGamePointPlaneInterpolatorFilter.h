#ifndef iGamePointPlaneInterpolatorFilter_h
#define iGamePointPlaneInterpolatorFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"
#include <string>

IGAME_NAMESPACE_BEGIN

class PointPlaneInterpolatorFilter : public Filter {
public:
    I_OBJECT(PointPlaneInterpolatorFilter);
    static Pointer New() { return new PointPlaneInterpolatorFilter; }

    // 与 ParaView 一致的内核和策略枚举
    enum KernelType { VORONOI = 0, GAUSSIAN = 1, SHEPARD = 2 };
    enum KernelFootprint { RADIUS = 0, N_CLOSEST = 1 };
    enum NullPointsStrategy { MASK_POINTS = 0, NULL_VALUE = 1, CLOSEST_POINT = 2 };

    bool Execute() override;

    // 平面参数设置
    void SetOrigin(const Point& p) { m_Origin = p; }
    const Point& GetOrigin() const noexcept { return m_Origin; }

    void SetNormal(const Point& n) { m_Normal = n; }
    const Point& GetNormal() const noexcept { return m_Normal; }

    void SetResolution(int res) { m_Resolution = res; }
    int GetResolution() const noexcept { return m_Resolution; }

    // 插值参数设置
    void SetKernelType(KernelType type) { m_KernelType = type; }
    void SetKernelFootprint(KernelFootprint footprint) { m_KernelFootprint = footprint; }
    void SetRadius(double radius) { m_Radius = radius; }
    void SetNumberOfPoints(int n) { m_NumberOfPoints = n; }
    void SetSharpness(double s) { m_Sharpness = s; }
    void SetPowerParameter(double p) { m_PowerParameter = p; }
    void SetNullPointsStrategy(NullPointsStrategy strategy) { m_NullPointsStrategy = strategy; }
    void SetNullValue(double value) { m_NullValue = value; }
    void SetValidPointsMaskArrayName(const std::string& name) { m_ValidPointsMaskArrayName = name; }

protected:
    PointPlaneInterpolatorFilter();
    ~PointPlaneInterpolatorFilter() override = default;

private:
    Point m_Origin{0.0, 0.0, 0.0};
    Point m_Normal{0.0, 0.0, 1.0};
    int m_Resolution{100};

    KernelType m_KernelType{VORONOI};
    KernelFootprint m_KernelFootprint{RADIUS};
    double m_Radius{100.0};
    int m_NumberOfPoints{8};
    double m_Sharpness{2.0};
    double m_PowerParameter{2.0};
    NullPointsStrategy m_NullPointsStrategy{MASK_POINTS}; // 默认掩码策略
    double m_NullValue{0.0};
    std::string m_ValidPointsMaskArrayName{"vtkValidPointMask"};
};

IGAME_NAMESPACE_END
#endif