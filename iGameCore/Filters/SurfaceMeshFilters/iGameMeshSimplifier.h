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

    void SetTargetReduction(double target) { this->TargetReduction = target; }
    void SetTargetFaceCount(int target) { this->TargetFaceCount = target; }

protected:
    MeshSimplifier();
    ~MeshSimplifier() override = default;

    float TargetReduction = 0.5;
    float TargetFaceCount = 0;
};
IGAME_NAMESPACE_END
#endif