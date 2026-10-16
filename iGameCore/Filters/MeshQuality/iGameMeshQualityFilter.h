#ifndef iGameMeshQualityFilter_h
#define iGameMeshQualityFilter_h

#include "iGameFilter.h"
#include "MeshMetrics/iGameSurfaceMeshMetricsFilter.h"
#include "MeshMetrics/iGameVolumeMeshMetricsFilter.h"

IGAME_NAMESPACE_BEGIN


class MeshQualityFilter : public Filter {
public:

    I_OBJECT(MeshQualityFilter);
    static Pointer New() {return new MeshQualityFilter;}
    bool Execute() override;

    //triangle
    // 设置三角形单元的质量指标
    void SetTriangleMetric(SurfaceMeshMetricsFilter::SurfaceMetric metric) {
        m_TriangleMetric = metric;
    }
    // 获取三角形单元的质量指标
    SurfaceMeshMetricsFilter::SurfaceMetric GetTriangleMetric() const {
        return m_TriangleMetric;
    }

    // Qua
    // 设置四边形单元的质量指标
    void SetQuadMetric(SurfaceMeshMetricsFilter::SurfaceMetric metric) {
        m_QuadMetric = metric;
    }
    //获取四边形单元的质量指标
    SurfaceMeshMetricsFilter::SurfaceMetric GetQuadMetric() const {
        return m_QuadMetric;
    }

    // Tet
    // 设置四面体单元的质量指标
    void SetTetMetric(VolumeMeshMetricsFilter::VolumeMetric metric) {
        m_TetMetric = metric;
    }
    // 获取四面体单元的质量指标
    VolumeMeshMetricsFilter::VolumeMetric GetTetMetric() const {
        return m_TetMetric;
    }

    // Hex
    // 设置六面体单元的质量指标
    void SetHexMetric(VolumeMeshMetricsFilter::VolumeMetric metric) {
        m_HexMetric = metric;
    }
    // 获取六面体单元的质量指标
    VolumeMeshMetricsFilter::VolumeMetric GetHexMetric() const {
        return m_HexMetric;
    }

    double GetMinimum() const {return m_Minimum;}
    double GetMaximum() const {return m_Maximum;}
    double GetAverage() const {return m_Average;}
    igIndex GetNumberOfCells() const {return m_NumberOfCells;}


protected:

    MeshQualityFilter();
    ~MeshQualityFilter() override = default;

    SurfaceMeshMetricsFilter::SurfaceMetric m_TriangleMetric =SurfaceMeshMetricsFilter::SurfaceMetric::FACE_AREA;
    SurfaceMeshMetricsFilter::SurfaceMetric m_QuadMetric =SurfaceMeshMetricsFilter::SurfaceMetric::FACE_AREA;
    VolumeMeshMetricsFilter::VolumeMetric m_TetMetric =VolumeMeshMetricsFilter::VolumeMetric::TET_EDGE_RATIO;
    VolumeMeshMetricsFilter::VolumeMetric m_HexMetric =VolumeMeshMetricsFilter::VolumeMetric::HEX_EDGE_RATIO;

    double m_Minimum = 0.0;
    double m_Maximum = 0.0;
    double m_Average = 0.0;
    igIndex m_NumberOfCells = 0;
};

IGAME_NAMESPACE_END

#endif
