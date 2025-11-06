#ifndef IGAME_MESH_SIMPLIFIER_H
#define IGAME_MESH_SIMPLIFIER_H

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"
#include "iGameTriangulation.h"

IGAME_NAMESPACE_BEGIN
class MeshSimplifier : public Filter {
public:
    I_OBJECT(MeshSimplifier);
    static Pointer New() { return new MeshSimplifier; }

    bool Execute() override;

    // Reduce to the target number. 
    // target: [0,1]
    void SetTargetReduction(float target) { this->TargetReduction = std::min(1.0f, std::max(0.0f, target)); }
    void SetTargetFaceCount(int target) { this->TargetFaceCount = target; }

protected:
    MeshSimplifier();
    ~MeshSimplifier() override = default;

    float TargetReduction = 0.5;
    float TargetFaceCount = 0;
};
IGAME_NAMESPACE_END
#endif