#ifndef iGameExtractEnclosedPointsFilter_h
#define iGameExtractEnclosedPointsFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class   ExtractEnclosedPointsFilter
 * @brief   提取封闭表面内部的点（参考 vtkExtractEnclosedPoints）
 *
 * 两个输入：
 *   Input 0 : 待判定的点集（PointSet）
 *   Input 1 : 封闭表面（SurfaceMesh，必须封闭流形）
 *
 * 输出 0：内部点及其点属性（InsideOut 可反选）。
 * 输出 1：GenerateOutliers 开启时，提供未选中的点及其点属性。
 *
 * 警告：表面必须封闭且流形。若 CheckSurface 为 true 会先检查；
 *       若为 false 且表面不封闭，结果未定义。
 */
class ExtractEnclosedPointsFilter : public Filter {
public:
    I_OBJECT(ExtractEnclosedPointsFilter);
    static Pointer New() { return new ExtractEnclosedPointsFilter; }

    bool Execute() override;

    // 是否检查表面封闭性。若开启且不封闭，执行失败。
    void SetCheckSurface(bool v) { m_CheckSurface = v; }
    bool GetCheckSurface() const { return m_CheckSurface; }

    // 相对于表面包围盒对角线长度的求交容差，与 ParaView 一致。
    void SetTolerance(double t) { m_Tolerance = t; }
    double GetTolerance() const { return m_Tolerance; }

    // false（默认）：输出内部点；true：输出外部点。
    void SetInsideOut(bool v) { m_InsideOut = v; }
    bool GetInsideOut() const { return m_InsideOut; }
    // 与 ParaView GenerateOutliers 一致：第二输出提供未选中的点。
    void SetGenerateOutliers(bool v) { m_GenerateOutliers = v; }
    bool GetGenerateOutliers() const { return m_GenerateOutliers; }

protected:
    ExtractEnclosedPointsFilter() {
        this->SetNumberOfInputs(2);
        this->SetNumberOfOutputs(2);
    }
    ~ExtractEnclosedPointsFilter() override = default;

    bool m_CheckSurface{true};
    double m_Tolerance{0.0001};
    bool m_InsideOut{false};
    bool m_GenerateOutliers{false};
};

IGAME_NAMESPACE_END
#endif
