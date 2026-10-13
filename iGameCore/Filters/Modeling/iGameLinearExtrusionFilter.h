#pragma once
#ifndef iGameLinearExtrusionFilter_h
#define iGameLinearExtrusionFilter_h

#include <iGameFilter.h>
#include <iGameVector.h>

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class LinearExtrusionFilter
 * @brief 将多边形数据沿指定规则线性扫描，生成端面和自由边侧裙面的 Filter。
 *
 * 语义尽可能对齐 vtkLinearExtrusionFilter。输入支持 SurfaceMesh，以及包含
 * IG_VERTEX、IG_LINE、IG_POLY_LINE、IG_TRIANGLE、IG_QUAD、IG_POLYGON 的
 * UnstructuredMesh；输出统一为 UnstructuredMesh，以容纳拉伸后的混合单元。
 * 不生成体单元：
 * - Vector：每个点按 ScaleFactor * Vector 平移；
 * - Normal：每个点按其点法向乘以 ScaleFactor 平移；
 * - Point：每个点相对 ExtrusionPoint 按 ScaleFactor 缩放/拉伸。
 *
 * 顶点拉伸为线，线/折线的每段拉伸为四边面，多边形保留可选的两端 cap，并由
 * 自由边生成侧裙面。当前 iGame 没有可持久化的 IG_TRIANGLE_STRIP 容器，因此
 * VTK TriangleStrip 只能在后续数据模型支持后完整复刻；首版不得伪称支持它。
 * Normal 模式需要输入具有可用的点法向量；若没有可用法向量，Execute() 应按
 * vtkLinearExtrusionFilter 的行为退回 Vector 模式。
 */
class LinearExtrusionFilter : public Filter {
public:
    I_OBJECT(LinearExtrusionFilter);
    static Pointer New() { return new LinearExtrusionFilter; }

    /** 与 vtkLinearExtrusionFilter 的三个拉伸模式保持相同数值语义。 */
    enum class ExtrusionType {
        Vector = 1,
        Normal = 2,
        Point = 3,
    };

    bool Execute() override;

    void SetExtrusionType(ExtrusionType type) {
        if (m_ExtrusionType != type) {
            m_ExtrusionType = type;
            Modified();
        }
    }
    ExtrusionType GetExtrusionType() const { return m_ExtrusionType; }

    void SetExtrusionTypeToVectorExtrusion() { SetExtrusionType(ExtrusionType::Vector); }
    void SetExtrusionTypeToNormalExtrusion() { SetExtrusionType(ExtrusionType::Normal); }
    void SetExtrusionTypeToPointExtrusion() { SetExtrusionType(ExtrusionType::Point); }

    void SetCapping(bool value) {
        if (m_Capping != value) {
            m_Capping = value;
            Modified();
        }
    }
    bool GetCapping() const { return m_Capping; }
    void CappingOn() { SetCapping(true); }
    void CappingOff() { SetCapping(false); }

    void SetScaleFactor(double value) {
        if (m_ScaleFactor != value) {
            m_ScaleFactor = value;
            Modified();
        }
    }
    double GetScaleFactor() const { return m_ScaleFactor; }

    /** 仅在 Vector 模式中使用；实际位移为 ScaleFactor * Vector。 */
    void SetVector(const Vector3d& value) {
        if (m_Vector != value) {
            m_Vector = value;
            Modified();
        }
    }
    void SetVector(double x, double y, double z) { SetVector(Vector3d(x, y, z)); }
    const Vector3d& GetVector() const { return m_Vector; }
    void GetVector(double& x, double& y, double& z) const {
        x = m_Vector[0];
        y = m_Vector[1];
        z = m_Vector[2];
    }

    /**
     * 仅在 Point 模式中使用。与 VTK 源码一致，位移为
     * ScaleFactor * (inputPoint - ExtrusionPoint)。
     */
    void SetExtrusionPoint(const Vector3d& value) {
        if (m_ExtrusionPoint != value) {
            m_ExtrusionPoint = value;
            Modified();
        }
    }
    void SetExtrusionPoint(double x, double y, double z) { SetExtrusionPoint(Vector3d(x, y, z)); }
    const Vector3d& GetExtrusionPoint() const { return m_ExtrusionPoint; }
    void GetExtrusionPoint(double& x, double& y, double& z) const {
        x = m_ExtrusionPoint[0];
        y = m_ExtrusionPoint[1];
        z = m_ExtrusionPoint[2];
    }

    const std::string& GetMessage() const { return m_Message; }

protected:
    LinearExtrusionFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~LinearExtrusionFilter() override = default;

private:
    // VTK 默认值：Normal 模式、端面封闭、ScaleFactor=1、Vector=(0,0,1)。
    ExtrusionType m_ExtrusionType{ExtrusionType::Normal};
    bool m_Capping{true};//true 拉伸产生的侧面+原始底面+新的顶面 | false 则丢弃 原始底面
    double m_ScaleFactor{1.0};// 拉伸的倍率
    Vector3d m_Vector{0.0, 0.0, 1.0};
    Vector3d m_ExtrusionPoint{0.0, 0.0, 0.0};

    std::string m_Message;// 记录过滤器在执行过程中产生的非致命性警告、提示或状态说明
};

IGAME_NAMESPACE_END
#endif
