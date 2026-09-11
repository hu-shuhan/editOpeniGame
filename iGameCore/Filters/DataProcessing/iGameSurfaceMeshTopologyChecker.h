#ifndef iGameSurfaceMeshTopologyChecker_h
#define iGameSurfaceMeshTopologyChecker_h

#include "iGameSurfaceMesh.h"

#include <vector>

IGAME_NAMESPACE_BEGIN

struct SurfaceMeshTopologyReport {
    IGsize pointCount{0};
    IGsize edgeCount{0};
    IGsize faceCount{0};

    std::vector<igIndex> nonTriangleFaceIds;
    std::vector<igIndex> invalidIndexFaceIds;
    std::vector<igIndex> degenerateFaceIds;
    std::vector<igIndex> zeroAreaFaceIds;
    std::vector<igIndex> duplicateFaceIds;
    std::vector<igIndex> invalidEdgeIds;
    std::vector<igIndex> isolatedEdgeIds;
    std::vector<igIndex> nonManifoldEdgeIds;
    std::vector<igIndex> invalidAdjacencyEdgeIds;

    bool IsValid() const;
};

class SurfaceMeshTopologyChecker {
public:
    static SurfaceMeshTopologyReport Check(const SurfaceMesh::Pointer& mesh);
};

IGAME_NAMESPACE_END
#endif
