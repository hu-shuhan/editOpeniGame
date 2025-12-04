#ifndef VolumeMeshMetrics_h
#define VolumeMeshMetrics_h

#include "Eigen/Dense"
#include "Eigen/Eigenvalues"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <algorithm>
#include <cmath>
#include <vector>


IGAME_NAMESPACE_BEGIN
class VolumeMeshMetricsFilter : public Filter {
public:
    I_OBJECT(VolumeMeshMetricsFilter);
    static Pointer New() { return new VolumeMeshMetricsFilter; }

    enum VolumeMetric {
        INVALID = -1,
        TET_EDGE_RATIO,         // 边长比
        TET_VOLUME,             // 体积
        TET_ASPECT_RATIO,       // 纵横比
        TET_JACOBIAN,           // 雅可比
        TET_COLLAPSE_RATIO,     // 塌陷率
        TET_VOL_SKEW,           // 体积歪斜度
        TET_MIN_ANGLE,          // 最小内角
        TET_EQUIANGLE_SKEWNESS, // 等角斜率
        TET_INRADIUS,           // 内切球半径
        TET_CIRCUMRADIUS,       // 外接球半径
        TET_VOL_ASPECT_RATIO,   // 体长宽比

        //六面体
        HEX_VOLUME,                // 体积
        HEX_TAPER,                 // 锥度
        HEX_JACOBIAN,              // 雅可比
        HEX_EDGE_RATIO,            // 长宽比
        HEX_MAX_EDGE_RATIO,        // 最大长宽比
        HEX_SKEW,                  // 歪斜度
        HEX_STRETCH,               // 伸展度
        HEX_DIAGONAL,              // 对角线比值
        HEX_RELATIVE_SIZE_SQUARED, // 相对大小平方

        // 新增六面体指标
        HEX_MIN_SCALED_JACOBIAN, // 最小标量雅可比
        HEX_AVG_SCALED_JACOBIAN, // 平均标量雅可比

        // 新增四面体指标
        TET_ASPECT_RATIO_ALT, // 替代纵横比计算方法
        TET_VOLUME_ALT,       // 四面体体积（替代方法）
        HEX_VOLUME_ALT,       // 六面体体积（替代方法）

    };

    void SetVolumeMetric(VolumeMetric mode) { this->m_Metric = mode; }
    VolumeMetric GetVolumeMetric() { return this->m_Metric; }

    bool Execute() override;

protected:
    VolumeMeshMetricsFilter();
    ~VolumeMeshMetricsFilter();
    VolumeMetric m_Metric = VolumeMetric::TET_EDGE_RATIO; //质量指标类型

    //四面体
    double ComputeMetric(igIndex vNum, igIndex* vhs);
    static std::vector<double> GetMinAndMaxLenOfCell(const std::vector<iGame::Point>& points);
    static double GetAreaOfFace(iGame::Point v1, iGame::Point v2, iGame::Point v3);
    static double ComputeTetEdgeRatio(const std::vector<iGame::Point>& points);
    static double ComputeTetVolume(const std::vector<iGame::Point>& points);
    static std::vector<double> GetAreaOfCell(const std::vector<iGame::Point>& points);
    static double GetInradiusOfCell(const std::vector<iGame::Point>& points);
    static iGame::Point GetOuterCircle(const std::vector<iGame::Point>& points);
    static double GetCircumradiusOfCell(const std::vector<iGame::Point>& points);
    static double GetAspectRatioOfCell(const std::vector<iGame::Point>& points);
    static double GetJacobianOfCell(const std::vector<iGame::Point>& points);
    static double GetCollapseRatioOfVertex(iGame::Point v1, iGame::Point v2, iGame::Point v3, double volume);
    static double GetCollapseRatioOfCell(const std::vector<iGame::Point>& points);
    static double GetVolSkewOfCell(const std::vector<iGame::Point>& points);
    static double GetSkewnessOfVertex(iGame::Point v0, iGame::Point v1, iGame::Point v2);
    static double GetSkewnessOfFace(const std::vector<iGame::Point>& points);
    static double GetInternalAnglesOfVertex(iGame::Point v0, iGame::Point v1, iGame::Point v2);
    static std::vector<double> GetInternalAnglesOfFace(const std::vector<iGame::Point>& points);
    static std::vector<std::vector<double>> GetInternalAnglesOfCell(const std::vector<iGame::Point>& points);
    static double GetMinInternalAnglesOfCell(const std::vector<iGame::Point>& points);
    static double GetHighOfVertex(iGame::Point v0, iGame::Point v1, iGame::Point v2, iGame::Point v3);
    static double GetVolAspectRatioOfCell(const std::vector<iGame::Point>& points);
    static double GetEquiangleSkewnessOfCell(const std::vector<iGame::Point>& points);

    // 六面体
    static double ComputeHexVolume(const std::vector<iGame::Point>& points);
    static double ComputeHexTaper(const std::vector<iGame::Point>& points);
    static double ComputeHexJacobian(const std::vector<iGame::Point>& points);
    static double ComputeHexEdgeRatio(const std::vector<iGame::Point>& points);
    static double ComputeHexMaxEdgeRatio(const std::vector<iGame::Point>& points);
    static double ComputeHexSkew(const std::vector<iGame::Point>& points);
    static double ComputeHexStretch(const std::vector<iGame::Point>& points);
    static double ComputeHexDiagonal(const std::vector<iGame::Point>& points);
    static double ComputeHexRelativeSizeSquared(const std::vector<iGame::Point>& points, float average_volume);

    static double ComputeHexMinScaledJacobian(const std::vector<iGame::Point>& points);
    static double ComputeHexAvgScaledJacobian(const std::vector<iGame::Point>& points);
    static double ComputeTetAspectRatioAlt(const std::vector<iGame::Point>& points);
    static double ComputeTetVolumeAlt(const std::vector<iGame::Point>& points);
    static double ComputeHexVolumeAlt(const std::vector<iGame::Point>& points);

    CellArray::Pointer m_Cells = nullptr;
    Points::Pointer m_Points = nullptr;
    static constexpr double PI = 3.14159265358979323846;
};

IGAME_NAMESPACE_END
#endif
