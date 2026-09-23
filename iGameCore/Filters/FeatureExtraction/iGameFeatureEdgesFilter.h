#ifndef iGameFeatureEdgesFilter_h
#define iGameFeatureEdgesFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <array>

IGAME_NAMESPACE_BEGIN

class FeatureEdgesFilter : public Filter {
public:
    I_OBJECT(FeatureEdgesFilter);

    static Pointer New() {
        return new FeatureEdgesFilter;
    }

    void SetFeatureAngle(double angle) {
        m_FeatureAngle =
            std::clamp(angle, 0.0, 180.0);
    }

    double GetFeatureAngle() const {
        return m_FeatureAngle;
    }

    void SetBoundaryEdges(bool value) {
        m_BoundaryEdges = value;
    }

    void SetFeatureEdges(bool value) {
        m_FeatureEdges = value;
    }

    void SetNonManifoldEdges(bool value) {
        m_NonManifoldEdges = value;
    }

    void SetManifoldEdges(bool value) {
        m_ManifoldEdges = value;
    }

    bool Execute() override;

private:
    enum EdgeType {
        BOUNDARY_EDGE = 0,
        FEATURE_EDGE = 1,
        NON_MANIFOLD_EDGE = 2,
        MANIFOLD_EDGE = 3
    };

    static std::array<double, 3>
        ComputeFaceNormal(Face* face);

protected:
    FeatureEdgesFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }

    ~FeatureEdgesFilter() override = default;

private:
    double m_FeatureAngle{
        30.0
    };

    bool m_BoundaryEdges{
        true
    };

    bool m_FeatureEdges{
        true
    };

    bool m_NonManifoldEdges{
        true
    };

    bool m_ManifoldEdges{
        false
    };
};

IGAME_NAMESPACE_END

#endif