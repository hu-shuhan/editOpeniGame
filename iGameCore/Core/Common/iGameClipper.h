#ifndef iGameClipper_h
#define iGameClipper_h

#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN

struct VirtualClippingSolid {
public:
    VirtualClippingSolid() {}
    virtual ~VirtualClippingSolid() {}
    virtual bool IsVisible(const double* position) const = 0;
    bool m_Use{false};
    bool m_Flip{false};
};

struct ClippingSolid_Box : public VirtualClippingSolid {
    double m_Bmin[3]{-DBL_MAX, -DBL_MAX, -DBL_MAX},
            m_Bmax[3]{DBL_MAX, DBL_MAX, DBL_MAX};
    bool IsVisible(const double* position) const override {
        if (m_Use) {
            if (position[0] >= m_Bmin[0] && position[0] <= m_Bmax[0] &&
                position[1] >= m_Bmin[1] && position[1] <= m_Bmax[1] &&
                position[2] >= m_Bmin[2] && position[2] <= m_Bmax[2]) {
                return !m_Flip;
            } else {
                return m_Flip;
            }
        } else {
            return true;
        }
    }
};

struct ClippingSolid_Plane : public VirtualClippingSolid {
    double m_Origin[3]{0, 0, 0}, m_Normal[3]{1, 0, 0};
    bool IsVisible(const double* position) const override {
        if (m_Use) {
            if (((position[0] - m_Origin[0]) * m_Normal[0] +
                 (position[1] - m_Origin[1]) * m_Normal[1] +
                 (position[2] - m_Origin[2]) * m_Normal[2]) >= 0.) {
                return !m_Flip;
            } else {
                return m_Flip;
            }
        } else {
            return true;
        }
    }
};

class iGameClipper : public Object {
public:
    I_OBJECT(iGameClipper);
    static Pointer New() { return new iGameClipper; }

    ClippingSolid_Box m_Box;
    ClippingSolid_Plane m_Plane;

    bool IsVisible(const double* position) const {
        if (!m_Box.IsVisible(position)) {
            return false;
        } else if (!m_Plane.IsVisible(position)) {
            return false;
        } else {
            return true;
        }
    }

    bool IsVisible(const float* position) const {
        double p[3];
        p[0] = position[0];
        p[1] = position[1];
        p[2] = position[2];
        return IsVisible(p);
    }

    void DisableAll() {
        this->Modified();
        m_Box.m_Use = false;
        m_Plane.m_Use = false;
    }
    bool IsAllDisable() const { return !m_Box.m_Use && !m_Plane.m_Use; }

protected:
    iGameClipper() {}
    ~iGameClipper() override {}
};
IGAME_NAMESPACE_END

#endif
