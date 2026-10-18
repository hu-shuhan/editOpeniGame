#pragma once
#include "iGameMeshSimplificationUtil.h"
#include <vector>

// Physical point attributes are normalized before entering this function.
// Outputs: normal sharpness, joint attribute variation, and their equal blend.
void CalculateMeshVertexImportance(std::vector<FVector>& vertices,
    std::vector<int>& triangles, const std::vector<std::vector<float>>& attributes,
    bool enhanced, std::vector<double>& sharpness,
    std::vector<double>& attributeFeature, std::vector<double>& importance);

namespace iGame { class SurfaceMesh; }
// Input must be a validated triangle mesh with matching vertex/attribute tuples.
// Normalize physical point attributes as in the paper workflow. All working
// arrays are local; the mesh and its AttributeSet are never modified.
void CalculateSurfaceMeshVertexImportance(iGame::SurfaceMesh* mesh,
    std::vector<FVector>& vertices, std::vector<int>& triangles,
    bool enhanced, std::vector<double>& importance);
