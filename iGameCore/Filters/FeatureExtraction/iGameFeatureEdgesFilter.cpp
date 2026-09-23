#include "iGameFeatureEdgesFilter.h"

#include "iGameAttributeSet.h"

#include <cmath>
#include <iostream>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEpsilon = 1e-12;

double VectorLength(const std::array<double, 3>& vector) {
    return std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
}

double ComputeNormalDot(const std::array<double, 3>& normal1, const std::array<double, 3>& normal2) {
    const double dot = normal1[0] * normal2[0] + normal1[1] * normal2[1] + normal1[2] * normal2[2];
    return std::clamp(dot, -1.0, 1.0);
}

std::array<double, 3> ComputeSurfaceFaceNormal(SurfaceMesh::Pointer surfaceMesh, IGsize faceId) {
    std::array<double, 3> normal{0.0, 0.0, 0.0};
    if (surfaceMesh == nullptr) { return normal; }

    igIndex pointIds[IGAME_CELL_MAX_SIZE]{};
    const int numberOfPoints = surfaceMesh->GetFacePointIds(faceId, pointIds);
    if (numberOfPoints < 3) { return normal; }

    for (int i = 0; i < numberOfPoints; ++i) {
        const Point& point1 = surfaceMesh->GetPoint(pointIds[i]);
        const Point& point2 = surfaceMesh->GetPoint(pointIds[(i + 1) % numberOfPoints]);

        normal[0] += (point1[1] - point2[1]) * (point1[2] + point2[2]);
        normal[1] += (point1[2] - point2[2]) * (point1[0] + point2[0]);
        normal[2] += (point1[0] - point2[0]) * (point1[1] + point2[1]);
    }

    const double length = VectorLength(normal);
    if (length < kEpsilon) { return {0.0, 0.0, 0.0}; }

    normal[0] /= length;
    normal[1] /= length;
    normal[2] /= length;
    return normal;
}

} // namespace

std::array<double, 3> FeatureEdgesFilter::ComputeFaceNormal(Face* face) {
    if (face == nullptr) { return {0.0, 0.0, 0.0}; }

    const Vector3f normal = face->GetNormal();
    std::array<double, 3> result{
            static_cast<double>(normal[0]),
            static_cast<double>(normal[1]),
            static_cast<double>(normal[2])};

    const double length = VectorLength(result);
    if (length < kEpsilon) { return {0.0, 0.0, 0.0}; }

    result[0] /= length;
    result[1] /= length;
    result[2] /= length;
    return result;
}

bool FeatureEdgesFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        std::cerr << "FeatureEdgesFilter: input is null." << std::endl;
        return false;
    }

    auto surfaceMesh = DynamicCast<SurfaceMesh>(input);
    if (surfaceMesh == nullptr) {
        if (DynamicCast<UnstructuredMesh>(input) != nullptr) {
            std::cerr << "FeatureEdgesFilter: extract a surface mesh before applying this filter." << std::endl;
        } else {
            std::cerr << "FeatureEdgesFilter: input must be a SurfaceMesh." << std::endl;
        }
        return false;
    }

    if (surfaceMesh->GetNumberOfPoints() == 0 || surfaceMesh->GetNumberOfFaces() == 0) {
        std::cerr << "FeatureEdgesFilter: input surface mesh is empty." << std::endl;
        return false;
    }

    surfaceMesh->BuildEdges();
    surfaceMesh->BuildFaceEdgeLinks();

    const IGsize numberOfEdges = surfaceMesh->GetNumberOfEdges();
    if (numberOfEdges == 0) {
        std::cerr << "FeatureEdgesFilter: surface mesh has no edges." << std::endl;
        return false;
    }

    const double featureCosine = std::cos(m_FeatureAngle * kPi / 180.0);

    auto output = UnstructuredMesh::New();
    output->SetName(surfaceMesh->GetName() + "_feature_edges");
    output->SetPoints(surfaceMesh->GetPoints());

    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();

    auto edgeTypes = FloatArray::New();
    edgeTypes->SetName("Edge Types");
    edgeTypes->SetDimension(1);
    edgeTypes->Reserve(numberOfEdges);

    auto edgeIds = UnsignedIntArray::New();
    edgeIds->SetName("Edge Ids");
    edgeIds->SetDimension(1);
    edgeIds->Reserve(numberOfEdges);

    IGsize boundaryEdgeCount = 0;
    IGsize featureEdgeCount = 0;
    IGsize nonManifoldEdgeCount = 0;
    IGsize manifoldEdgeCount = 0;
    IGsize outputEdgeCount = 0;

    for (IGsize edgeId = 0; edgeId < numberOfEdges; ++edgeId) {
        SurfaceMesh::ReturnContainer neighborFaces;
        const bool success = surfaceMesh->GetEdgeToNeighborFaces(edgeId, neighborFaces);
        if (!success || neighborFaces.size() == 0) { continue; }

        bool shouldOutput = false;
        int edgeTypeValue = -1;

        if (neighborFaces.size() == 1) {
            if (m_BoundaryEdges) {
                shouldOutput = true;
                edgeTypeValue = BOUNDARY_EDGE;
                ++boundaryEdgeCount;
            }
        } else if (neighborFaces.size() >= 3) {
            if (m_NonManifoldEdges) {
                shouldOutput = true;
                edgeTypeValue = NON_MANIFOLD_EDGE;
                ++nonManifoldEdgeCount;
            }
        } else if (neighborFaces.size() == 2) {
            const auto normal1 = ComputeSurfaceFaceNormal(surfaceMesh, neighborFaces[0]);
            const auto normal2 = ComputeSurfaceFaceNormal(surfaceMesh, neighborFaces[1]);
            if (VectorLength(normal1) > kEpsilon && VectorLength(normal2) > kEpsilon) {
                const double normalDot = ComputeNormalDot(normal1, normal2);
                if (m_FeatureEdges && normalDot <= featureCosine) {
                    shouldOutput = true;
                    edgeTypeValue = FEATURE_EDGE;
                    ++featureEdgeCount;
                } else if (m_ManifoldEdges) {
                    shouldOutput = true;
                    edgeTypeValue = MANIFOLD_EDGE;
                    ++manifoldEdgeCount;
                }
            }
        }

        if (!shouldOutput || edgeTypeValue < 0) { continue; }

        igIndex pointIds[2]{};
        const int pointCount = surfaceMesh->GetEdgePointIds(edgeId, pointIds);
        if (pointCount != 2) { continue; }

        outputCells->AddCellId2(pointIds[0], pointIds[1]);
        outputTypes->AddValue(IG_LINE);
        edgeTypes->AddValue(static_cast<float>(edgeTypeValue));
        edgeIds->AddValue(static_cast<unsigned int>(edgeId));
        ++outputEdgeCount;
    }

    if (outputEdgeCount == 0) {
        std::cerr << "FeatureEdgesFilter: no edges matched the selected criteria." << std::endl;
        return false;
    }

    output->SetCells(outputCells, outputTypes);
    auto attributeSet = output->GetAttributeSet();
    attributeSet->AddScalar(IG_CELL, edgeTypes);
    attributeSet->AddScalar(IG_CELL, edgeIds);
    SetOutput(output);

    std::cout << "Boundary edges: " << boundaryEdgeCount << std::endl;
    std::cout << "Feature edges: " << featureEdgeCount << std::endl;
    std::cout << "Non-manifold edges: " << nonManifoldEdgeCount << std::endl;
    std::cout << "Manifold edges: " << manifoldEdgeCount << std::endl;
    std::cout << "Output edges: " << outputEdgeCount << std::endl;

    return true;
}

IGAME_NAMESPACE_END
