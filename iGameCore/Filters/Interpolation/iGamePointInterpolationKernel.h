#ifndef iGamePointInterpolationKernel_h
#define iGamePointInterpolationKernel_h

#include "iGameMacro.h"

#include <algorithm>
#include <cmath>

IGAME_NAMESPACE_BEGIN

// 与 ParaView vtkPointInterpolator 对应的：核类型 / 邻域形态 / 空点策略。
enum class PointKernelType {
    Voronoi = 0,
    Gaussian = 1,
    Shepard = 2,
    Linear = 3,
};

enum class PointKernelFootprint {
    Radius = 0,
    NClosest = 1,
};

enum class PointNullPointsStrategy {
    MaskPoints = 0,
    NullValue = 1,
    ClosestPoint = 2,
};

inline constexpr double PointKernelExactHitToleranceSq = 1e-12;

// Gaussian: w = exp(-(sharpness * r / radius)^2)，r 为距离。
inline double PointGaussianWeight(double distSq, double radius, double sharpness) {
    if (radius <= 0.0) return 0.0;
    const double factor = sharpness / radius;
    return std::exp(-(factor * factor) * distSq);
}

// Shepard(IDW): w = 1 / r^power。
inline double PointShepardWeight(double distSq, double power) {
    if (distSq <= 0.0) return 0.0;
    return 1.0 / std::pow(std::sqrt(distSq), power);
}

IGAME_NAMESPACE_END

#endif
