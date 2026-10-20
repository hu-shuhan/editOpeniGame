#ifndef iGameCellMeshMetrics_h
#define iGameCellMeshMetrics_h

#include "iGameFilter.h"
#include "iGameCellQualityMath.h"

IGAME_NAMESPACE_BEGIN
class CellMeshMetricsFilter : public Filter {
public:
    I_OBJECT(CellMeshMetricsFilter);
    static Pointer New() { return new CellMeshMetricsFilter; }

    /// 统一的质量评估指标（对齐 ParaView Cell Quality 的交互）：
    /// 用户只选一个指标，底层按每个单元的【真实单元类型】分发计算，
    /// 该指标对某类单元不适用时填入"不支持值"（默认 -1.0）。
    ///
    /// 指标清单严格对齐 ParaView 6.1.1 Cell Quality 的 20 个 Quality Measure，
    /// 每项对应 Verdict 的一个或多个函数（按单元类型分发）。
    enum CellQualityMetric {
        // —— ParaView Cell Quality 的 20 个标准指标 ——
        QUALITY_AREA,                // Area                 quad_area / tri_area
        QUALITY_ASPECT_RATIO,        // Aspect Ratio         quad_/tri_/tet_aspect_ratio
        QUALITY_CONDITION,           // Condition            quad_/tri_/hex_/tet_condition
        QUALITY_DISTORTION,          // Distortion           quad_/tri_/hex_/tet_distortion
        QUALITY_EDGE_RATIO,          // Edge Ratio           quad_/tri_/hex_/tet_edge_ratio
        QUALITY_EQUIANGLE_SKEW,      // Equiangle Skew       quad_/tri_/hex_/tet_equiangle_skew
        QUALITY_JACOBIAN,            // Jacobian             quad_/hex_/tet_jacobian
        QUALITY_MIN_ANGLE,           // Minimum Angle        quad_/tri_/tet_minimum_angle
        QUALITY_ODDY,                // Oddy                 quad_oddy / hex_oddy
        QUALITY_RADIUS_RATIO,        // Radius Ratio         quad_/tri_/tet_radius_ratio
        QUALITY_RELATIVE_SIZE_SQUARED, // Relative Size Squared  hex_relative_size_squared（需平均体积）
        QUALITY_SCALED_JACOBIAN,     // Scaled Jacobian      quad_/tri_/hex_/tet_scaled_jacobian
        QUALITY_SHAPE,               // Shape                quad_/tri_/hex_/tet_shape
        QUALITY_SHAPE_AND_SIZE,      // Shape and Size       hex_shape_and_size（需平均体积）
        QUALITY_SHEAR,               // Shear                quad_shear / hex_shear
        QUALITY_SHEAR_AND_SIZE,      // Shear and Size       hex_shear_and_size（需平均体积）
        QUALITY_SKEW,                // Skew                 quad_skew / hex_skew
        QUALITY_STRETCH,             // Stretch              quad_stretch / hex_stretch
        QUALITY_TAPER,               // Taper                quad_taper / hex_taper
        QUALITY_WARPAGE,             // Warpage              quad_warpage

        // —— 以下为内部/兼容保留项，不再出现在 UI 下拉框中 ——
        QUALITY_VOLUME,         // （保留：四面体/六面体体积，ParaView Cell Quality 无此项）
        QUALITY_DIAGONAL,       // （保留：六面体对角线比值）
        QUALITY_MAX_EDGE_RATIO, // （保留：六面体最大长宽比）
        QUALITY_COLLAPSE_RATIO  // （保留：四面体塌陷率）
    };

    bool Execute() override;
    bool Execute(DataObject::Pointer);
    bool Execute(DataObject::Pointer, DataObject::Pointer&);

    // 设置统一评估指标
    void setMetric(CellQualityMetric metric) { m_QualityMetric = metric; }
    CellQualityMetric getMetric() const { return m_QualityMetric; }

    // 设置不支持/无效值（默认 -1.0）
    void setUnsupportedValue(double val) { m_UnsupportedValue = val; }
    double getUnsupportedValue() const { return m_UnsupportedValue; }

    // 统计结果查询接口（供 Qt 弹窗与自动化测试断言）
    int GetSupportedCount() const { return m_SupportedCount; }
    int GetUnsupportedCount() const { return m_UnsupportedCount; }
    int GetSkippedBlockCount() const { return m_SkippedBlockCount; }

protected:
    DataObject::Pointer input;
    DataObject::Pointer output;
    CellMeshMetricsFilter();
    ~CellMeshMetricsFilter() override = default;

    bool ComputeCellMetrics(DataObject::Pointer, DataObject::Pointer&);

    CellQualityMetric m_QualityMetric = QUALITY_EDGE_RATIO;
    // 不支持的 cell 返回值
    double m_UnsupportedValue = -1.0;

    int m_SupportedCount = 0;
    int m_UnsupportedCount = 0;
    int m_SkippedBlockCount = 0;
};

IGAME_NAMESPACE_END

#endif // !iGameCellMeshMetrics_h
