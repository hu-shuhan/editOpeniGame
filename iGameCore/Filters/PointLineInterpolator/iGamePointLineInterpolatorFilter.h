#ifndef iGamePointLineInterpolatorFilter_h
#define iGamePointLineInterpolatorFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class PointLineInterpolatorFilter
 * @brief Interpolates input point-data arrays onto a parameterized line.
 *
 * The filter follows ParaView's Point Line Interpolator model: the input
 * points form the interpolation source and a line defined by Point1, Point2,
 * and Resolution provides the output geometry. Resolution is the number of
 * line segments, therefore the output contains Resolution + 1 points.
 */
class PointLineInterpolatorFilter : public Filter {
public:
    I_OBJECT(PointLineInterpolatorFilter);
    static Pointer New() { return new PointLineInterpolatorFilter; }

    enum KernelType { VORONOI = 0, GAUSSIAN = 1, SHEPARD = 2 };
    enum KernelFootprint { RADIUS = 0, N_CLOSEST = 1 };
    enum NullPointsStrategy { MASK_POINTS = 0, NULL_VALUE = 1, CLOSEST_POINT = 2 };

    bool Execute() override;

    void SetPoint1(const Point& point) { m_Point1 = point; }
    const Point& GetPoint1() const noexcept { return m_Point1; }

    void SetPoint2(const Point& point) { m_Point2 = point; }
    const Point& GetPoint2() const noexcept { return m_Point2; }

    void SetResolution(int resolution) { m_Resolution = resolution; }
    int GetResolution() const noexcept { return m_Resolution; }

    void SetKernelType(KernelType type) { m_KernelType = type; }
    KernelType GetKernelType() const noexcept { return m_KernelType; }

    void SetKernelFootprint(KernelFootprint footprint) { m_KernelFootprint = footprint; }
    KernelFootprint GetKernelFootprint() const noexcept { return m_KernelFootprint; }

    void SetRadius(double radius) { m_Radius = radius; }
    double GetRadius() const noexcept { return m_Radius; }

    void SetNumberOfPoints(int numberOfPoints) { m_NumberOfPoints = numberOfPoints; }
    int GetNumberOfPoints() const noexcept { return m_NumberOfPoints; }

    void SetSharpness(double sharpness) { m_Sharpness = sharpness; }
    double GetSharpness() const noexcept { return m_Sharpness; }

    void SetPowerParameter(double power) { m_PowerParameter = power; }
    double GetPowerParameter() const noexcept { return m_PowerParameter; }

    void SetNullPointsStrategy(NullPointsStrategy strategy) { m_NullPointsStrategy = strategy; }
    NullPointsStrategy GetNullPointsStrategy() const noexcept { return m_NullPointsStrategy; }

    void SetNullValue(double value) { m_NullValue = value; }
    double GetNullValue() const noexcept { return m_NullValue; }

    void SetValidPointsMaskArrayName(const std::string& name);
    const std::string& GetValidPointsMaskArrayName() const noexcept { return m_ValidPointsMaskArrayName; }

    UnstructuredMesh::Pointer GetLineOutput() const { return m_Output; }

protected:
    PointLineInterpolatorFilter();
    ~PointLineInterpolatorFilter() override = default;

private:
    Point m_Point1{-0.5, 0.0, 0.0};
    Point m_Point2{0.5, 0.0, 0.0};
    int m_Resolution{100};
    KernelType m_KernelType{VORONOI};
    KernelFootprint m_KernelFootprint{RADIUS};
    double m_Radius{1.0};
    int m_NumberOfPoints{8};
    double m_Sharpness{2.0};
    double m_PowerParameter{2.0};
    NullPointsStrategy m_NullPointsStrategy{CLOSEST_POINT};
    double m_NullValue{0.0};
    std::string m_ValidPointsMaskArrayName{"vtkValidPointMask"};
    UnstructuredMesh::Pointer m_Output{};
};

IGAME_NAMESPACE_END

#endif
