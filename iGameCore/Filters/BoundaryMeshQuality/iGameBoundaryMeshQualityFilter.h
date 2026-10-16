#ifndef iGameBoundaryMeshQualityFilter_h
#define iGameBoundaryMeshQualityFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

class BoundaryMeshQualityFilter : public Filter {
public:
    I_OBJECT(BoundaryMeshQualityFilter);
    static Pointer New() { return new BoundaryMeshQualityFilter; }

    // 边界网格质量评估指标枚举（模仿 ParaView 的 BoundaryMeshQuality）
    enum BoundaryMetric {
        INVALID = -1,
        DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER, // 体单元中心到其边界面中心的距离
        DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE,  // 体单元中心到其边界面所在平面的垂直距离
        ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR // 边界面的法线方向与体单元中心到面中心向量之间的夹角
    };

    void SetBoundaryMetric(BoundaryMetric mode) { this->m_Metric = mode; }
    BoundaryMetric GetBoundaryMetric() { return this->m_Metric; }

    bool Execute() override;
    std::string GetMessage() const { return m_Message; }

protected:
    BoundaryMeshQualityFilter();
    ~BoundaryMeshQualityFilter() override;

    BoundaryMetric m_Metric = BoundaryMetric::DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER;

    // 输入数据
    VolumeMesh::Pointer m_VolumeMesh{};

    // 工具函数
    static Point ComputeCellCenter(Volume* cell);
    static double AngleInDegrees(const Vector3f& a, const Vector3f& b);
    static Vector3f ComputeFaceNormalNewell(const igIndex* facePids, int facePcnt,
                                            const VolumeMesh* mesh);
    double ComputeMetricForBoundaryFace(igIndex faceId);
    bool IsDegenerateFace(igIndex faceId) const;
    bool IsDegenerateVolume(Volume* vol) const;

    std::string m_Message{"Not Volume Mesh!"};
    static constexpr double PI = 3.14159265358979323846;
};

IGAME_NAMESPACE_END
#endif
