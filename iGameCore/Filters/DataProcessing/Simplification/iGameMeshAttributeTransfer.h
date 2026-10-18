#pragma once
#include "iGameSurfaceMesh.h"
#include <string>

IGAME_NAMESPACE_BEGIN
// Post-process geometry only, without changing either mesh's topology.
// Point data uses closest-surface barycentric interpolation; cell data is
// piecewise constant and sampled at the destination face center.
bool TransferMeshAttributes(SurfaceMesh* source, SurfaceMesh* destination, std::string& error);
IGAME_NAMESPACE_END
