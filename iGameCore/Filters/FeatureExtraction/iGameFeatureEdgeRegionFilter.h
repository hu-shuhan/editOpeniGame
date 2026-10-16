#ifndef iGameFeatureEdgeRegion_h
#define iGameFeatureEdgeRegion_h
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class FeatureEdgeRegionFilter : public Filter {
public:
    I_OBJECT(FeatureEdgeRegionFilter);
    static Pointer New() { return new FeatureEdgeRegionFilter; }

    bool Execute() override;

    void SetFeatureAngle(float angle) { m_featureAngle = angle; }

protected:
    FeatureEdgeRegionFilter() {
        this->SetNumberOfInputs(1);
        this->SetNumberOfOutputs(1);
    }
    float m_featureAngle = 0.0f;
};
IGAME_NAMESPACE_END
#endif
