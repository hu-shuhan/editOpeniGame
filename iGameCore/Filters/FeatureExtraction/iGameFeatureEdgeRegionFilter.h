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

protected:
    FeatureEdgeRegionFilter() {
        this->SetNumberOfInputs(2);
        this->SetNumberOfOutputs(1);
    }
};
IGAME_NAMESPACE_END
#endif
