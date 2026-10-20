#pragma once
#ifndef iGameRibbonFilter_h
#define iGameRibbonFilter_h

#include <iGameFilter.h>
#include <iGameVector.h>

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class RibbonFilter
 * @brief 把线/折线扫成有向带状面的 Filter，语义对齐 vtkRibbonFilter。
 *
 * 输入：UnstructuredMesh（IG_LINE / IG_POLY_LINE 单元），或带显式边数组的
 * SurfaceMesh（legacy VTK POLYDATA 的 LINES 段）。其他单元类型不参与生成，
 * 只在 GetMessage() 里记录被忽略的数量与类型；一条线都没有则失败。
 *
 * 每条折线独立处理（不同折线共享点也不会互相污染）：
 * - 逐点求平均切向 s = normalize((sPrev + sNext) / 2)，得到转角处自然倒角；
 * - w = normalize(cross(s, n))，nP = normalize(cross(w, s))，n 为该点的线法向；
 * - 带宽方向 v = w * cos(Angle) + nP * sin(Angle)，两侧点 p∓ = p ∓ Width * v，
 *   Width 是半宽（与 vtkRibbonFilter 一致），带宽恒为 2 * Width；
 * - 每条折线输出 2 * npts 个点（偶数索引是减号侧，与 VTK 输出顺序一致），
 *   第 i 段输出四边形 (p-ᵢ, p+ᵢ, p+ᵢ₊₁, p-ᵢ₊₁)。
 *
 * 与 VTK 的差异（使用说明里同样写明）：
 * - VTK 每条折线输出 1 个 triangle strip，iGame 没有可持久化的 strip 容器，
 *   因此每段输出 1 个 IG_QUAD（输出面数 = Σ(npts - 1)，与 VTK 的 cell 数不同）；
 * - VTK 的滑动法向（UseDefaultNormal 关且输入没有点法向时自动计算）尚未实现：
 *   此时回退到 DefaultNormal 并在 GetMessage() 中说明；
 * - UseDefaultNormal 关且输入带 IG_NORMAL 点属性时按 VTK 使用输入的点法向；
 * - 不实现 VaryWidth / WidthFactor / GenerateTCoords / TextureLength；
 * - Width <= 0 明确失败（VTK 会 clamp 成 0 从而产出零宽退化面）；
 * - 折线中途退化时整条跳过，不留下未被单元引用的孤立点（VTK 会留下）。
 */
class RibbonFilter : public Filter {
public:
    I_OBJECT(RibbonFilter);
    static Pointer New() { return new RibbonFilter; }

    bool Execute() override;

    /** 带状面的半宽，默认 0.5（与 vtkRibbonFilter 构造函数一致）；必须 > 0。 */
    void SetWidth(double value) {
        if (m_Width != value) {
            m_Width = value;
            Modified();
        }
    }
    double GetWidth() const { return m_Width; }

    /** 相对线法向的偏转角（度），默认 0；与 vtkSetClampMacro(Angle, 0, 360) 一致地夹到 [0, 360]。 */
    void SetAngle(double degrees) {
        double clamped = degrees;
        if (clamped < 0.0) { clamped = 0.0; }
        if (clamped > 360.0) { clamped = 360.0; }
        if (m_Angle != clamped) {
            m_Angle = clamped;
            Modified();
        }
    }
    double GetAngle() const { return m_Angle; }

    /** 输入没有可用点法向时使用的默认法向，默认 (0, 0, 1)；必须非零。 */
    void SetDefaultNormal(const Vector3d& value) {
        if (m_DefaultNormal != value) {
            m_DefaultNormal = value;
            Modified();
        }
    }
    void SetDefaultNormal(double x, double y, double z) { SetDefaultNormal(Vector3d(x, y, z)); }
    const Vector3d& GetDefaultNormal() const { return m_DefaultNormal; }
    void GetDefaultNormal(double& x, double& y, double& z) const {
        x = m_DefaultNormal[0];
        y = m_DefaultNormal[1];
        z = m_DefaultNormal[2];
    }

    /**
     * 是否强制使用 DefaultNormal。
     * 默认 true（与 VTK 的字段默认值不同：VTK 默认关，然后按折线自动算滑动法向；
     * iGame 首版没有滑动法向，因此默认用显式法向，行为确定、可复现）。
     */
    void SetUseDefaultNormal(bool value) {
        if (m_UseDefaultNormal != value) {
            m_UseDefaultNormal = value;
            Modified();
        }
    }
    bool GetUseDefaultNormal() const { return m_UseDefaultNormal; }
    void UseDefaultNormalOn() { SetUseDefaultNormal(true); }
    void UseDefaultNormalOff() { SetUseDefaultNormal(false); }

    const std::string& GetMessage() const { return m_Message; }

protected:
    RibbonFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~RibbonFilter() override = default;

private:
    double m_Width{0.5};                            // 半宽
    double m_Angle{0.0};                            // 偏转角（度）
    Vector3d m_DefaultNormal{0.0, 0.0, 1.0};        // 默认线法向
    bool m_UseDefaultNormal{true};                  // 是否强制使用默认法向

    std::string m_Message;// 非致命提示与失败原因（与线性拉伸同一机制）
};

IGAME_NAMESPACE_END
#endif
