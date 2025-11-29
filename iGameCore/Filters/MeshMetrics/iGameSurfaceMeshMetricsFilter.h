#ifndef SurfaceMeshMetrics_h
#define SurfaceMeshMetrics_h


#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN
class SurfaceMeshMetricsFilter : public Filter {
public:
    I_OBJECT(SurfaceMeshMetricsFilter);
    static Pointer New() { return new SurfaceMeshMetricsFilter; }

    enum SurfaceMetric {
        //通用指标
        FACE_AREA,    // 三角形面积
        MAX_ANGLE,    // 最大内角
        MIN_ANGLE,    // 最小内角
        JACOBIAN,     // 雅可比
        ASPECT_RATIO, // 纵横比
        EDGE_RATIO,   // 边长比

        // 四边形特有指标
        WARPAGE, // 翘曲度
        TAPER,   // 锥度
        SKEW,    // 歪斜度

        ANGLE_QUALITY,  // 角度质量 (0-1)
        MESH_VOLUME,    // 封闭网格体积
        FACE_MIN_ANGLE, // 面片最小角度(度)
        FACE_MAX_ANGLE, // 面片最大角度(度)

        MESH_QUALITY, // 网格质量

        // 新增项
        FACE_MIN_ANGLE_QUALITY, // 面最小角质量 (0-1) - MeshMath.h
        MESH_VOLUME_SURFACE,    // 表面网格体积 - MeshMath.h
    };

    void SetSurfaceMetric(SurfaceMetric mode) { this->m_Metric = mode; }
    SurfaceMetric GetSurfaceMetric() { return this->m_Metric; }

    bool Execute() override;

protected:
    SurfaceMeshMetricsFilter();
    ~SurfaceMeshMetricsFilter();
    SurfaceMetric m_Metric = SurfaceMetric::FACE_AREA; //质量指标类型

    double ComputeMetric(igIndex vNum, igIndex* vhs);

    // 通用辅助方法
    static double GetInternalAnglesOfVertex(iGame::Point v0, iGame::Point v1, iGame::Point v2);

    // 三角形
    static double ComputeTriangleArea(const std::vector<iGame::Point>& points);
    static double ComputeTriangleMaxAngle(const std::vector<iGame::Point>& points);
    static double ComputeTriangleMinAngle(const std::vector<iGame::Point>& points);
    static double ComputeTriangleJacobian(const std::vector<iGame::Point>& points);
    static double ComputeTriangleAspectRatio(const std::vector<iGame::Point>& points);
    static double ComputeTriangleEdgeRatio(const std::vector<iGame::Point>& points);
    static double ComputeTriangleMeshQuality(const std::vector<iGame::Point>& points);

    //四边形
    static double ComputeQuadArea(const std::vector<iGame::Point>& points);
    static double ComputeQuadMaxAngle(const std::vector<iGame::Point>& points);
    static double ComputeQuadMinAngle(const std::vector<iGame::Point>& points);
    static double ComputeQuadJacobian(const std::vector<iGame::Point>& points);
    static double ComputeQuadAspectRatio(const std::vector<iGame::Point>& points);
    static double ComputeQuadEdgeRatio(const std::vector<iGame::Point>& points);
    static double ComputeQuadWarpage(const std::vector<iGame::Point>& points);
    static double ComputeQuadTaper(const std::vector<iGame::Point>& points);
    static double ComputeQuadSkew(const std::vector<iGame::Point>& points);

    static double ComputeFaceMinAngleQuality(const std::vector<iGame::Point>& points);
    static double ComputeSurfaceVolume(const std::vector<iGame::Point>& points);
    static double ComputeTotalSurfaceVolume(iGame::CellArray::Pointer faces, iGame::Points::Pointer points);

    iGame::CellArray::Pointer m_Faces = nullptr; //存储表面网格的面信息
    iGame::Points::Pointer m_Points = nullptr;   //存储表面网格的点信息
    static constexpr double PI = 3.14159265358979323846;
};

IGAME_NAMESPACE_END
#endif
