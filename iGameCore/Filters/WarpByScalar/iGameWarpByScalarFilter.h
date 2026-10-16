#ifndef iGameWarpByScalarFilter_h
#define iGameWarpByScalarFilter_h

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class WarpByScalar : public Filter {
public:
    I_OBJECT(WarpByScalar);
    static Pointer New() { return new WarpByScalar; }

    /// 用于变形的点标量数组名。
    void SetScalarsArrayName(const std::string& name);
    const std::string& GetScalarsArrayName() const { return m_ScalarsArrayName; }

    /// 缩放系数。
    void SetScaleFactor(double scale);
    double GetScaleFactor() const { return m_ScaleFactor; }

    /// 是否强制使用用户指定的法向。
    void SetUseNormal(bool use);
    bool GetUseNormal() const { return m_UseNormal; }

    /// 用户指定的法向（。
    void SetNormal(double nx, double ny, double nz);
    const double* GetNormal() const { return m_Normal; }

    /// XY 平面模式。
    void SetXYPlane(bool xyPlane);
    bool GetXYPlane() const { return m_XYPlane; }

    bool Execute() override;

protected:
    WarpByScalar();
    ~WarpByScalar() override = default;

private:
    std::string m_ScalarsArrayName{};
    double m_ScaleFactor{1.0};
    bool m_UseNormal{false};
    double m_Normal[3]{0.0, 0.0, 1.0};
    bool m_XYPlane{false};
};

IGAME_NAMESPACE_END
#endif
