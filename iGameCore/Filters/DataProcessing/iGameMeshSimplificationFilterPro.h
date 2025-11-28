#ifndef IGAME_MESH_SIMPLIFIER_H
#define IGAME_MESH_SIMPLIFIER_H

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"
#include "iGameMeshTriangulationFilter.h"

IGAME_NAMESPACE_BEGIN
class MeshSimplificationFilterPro : public Filter {
public:
    I_OBJECT(MeshSimplificationFilterPro);
    static Pointer New() { return new MeshSimplificationFilterPro; }

    bool Execute() override;

    // Reduce to the target number. 
    // target: [0,1]
    void SetTargetReduction(float target = 0.5f) { this->TargetReduction = std::min(1.0f, std::max(0.0f, target)); }
    // If TargetFaceCount is not 0, TargetFaceCount is used first
    void SetTargetFaceCount(int target = 0) { this->TargetFaceCount = target; }

    void SetPreserveBoundary(bool flag = true) { this->PreserveBoundary = flag; }

    // true: 使用原来的顶点数据
    void SetFreeze(bool flag = false) { this->Freeze = flag; }

    // true: 使用原来的顶点数据
    void SetTransformToCellData(bool flag = false) { this->Transform = flag; }

protected:
    MeshSimplificationFilterPro();
    ~MeshSimplificationFilterPro() override = default;

    float TargetReduction = 0.5;
    float TargetFaceCount = 0;
    bool PreserveBoundary = true;
    bool Freeze = false;
    bool Transform = false;
};
IGAME_NAMESPACE_END
#endif