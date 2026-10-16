#ifndef iGameAxisAlignedReflectionFilter_h
#define iGameAxisAlignedReflectionFilter_h

#include "iGameFilter.h"

IGAME_NAMESPACE_BEGIN

class AxisAlignedReflectionFilter : public Filter {
public:
    I_OBJECT(AxisAlignedReflectionFilter);
    static Pointer New() { return new AxisAlignedReflectionFilter; }

    enum class Plane {
        XMin,
        YMin,
        ZMin,
        XMax,
        YMax,
        ZMax,
        X,
        Y,
        Z
    };

    bool Execute() override;

    void SetPlane(Plane plane) { m_Plane = plane; }
    Plane GetPlane() const { return m_Plane; }

    void SetCenter(double center) { m_Center = center; }
    double GetCenter() const { return m_Center; }

    void SetCopyInput(bool value) { m_CopyInput = value; }
    bool GetCopyInput() const { return m_CopyInput; }

    void SetFlipAllInputArrays(bool value) { m_FlipAllInputArrays = value; }
    bool GetFlipAllInputArrays() const { return m_FlipAllInputArrays; }

protected:
    AxisAlignedReflectionFilter();
    ~AxisAlignedReflectionFilter() override = default;

private:
    Plane m_Plane{Plane::XMin};
    double m_Center{0.0};
    bool m_CopyInput{true};
    bool m_FlipAllInputArrays{true};
};

IGAME_NAMESPACE_END

#endif
