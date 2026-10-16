#include "iGameMeshTriangulationFilter.h"
#include "iGamePolygonTriangulator.h"

IGAME_NAMESPACE_BEGIN

bool MeshTriangulationFilter::Execute() {

    mesh = nullptr;
    SetOutput(nullptr);
    auto input = GetInput(0);
    if (!input) return false;
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            mesh = DynamicCast<SurfaceMesh>(input);
            break;
        case IG_UNSTRUCTURED_MESH:
            mesh = DynamicCast<UnstructuredMesh>(input)->TransferToSurfaceMesh();
            break;
        default:
            break;
    }
    if (mesh == nullptr) { return false; }

    {
        bool isTriangle = true;
        igIndex face[IGAME_CELL_MAX_SIZE]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
            int size = mesh->GetFacePointIds(i, face);
            if (size != 3) {
                isTriangle = false;
                break;
            }
        }

        if (isTriangle) {
            SetOutput(mesh);
            return true;
        }
    }

    auto Attrs = mesh->GetAttributeSet();
    std::vector<int> FaceIdMap;
    CellArray::Pointer Faces = CellArray::New();
    Points::Pointer Points = mesh->GetPoints();
    // Reserve the complete input boundary before choosing any new diagonal.
    // This also protects triangles processed later from overlapping polygons.
    detail::PolygonEdges occupiedEdges;
    for (IGsize faceId = 0; faceId < mesh->GetNumberOfFaces(); ++faceId) {
        const igIndex* ids = nullptr;
        const int count = mesh->GetFaces()->GetCellIds(faceId, ids);
        if (count < 3) return false;
        for (int j = 0; j < count; ++j) {
            if (ids[j] < 0 || ids[j] >= mesh->GetNumberOfPoints()) return false;
            occupiedEdges.emplace(ids[j], ids[(j + 1) % count]);
        }
    }

    for (IGsize faceId = 0; faceId < mesh->GetNumberOfFaces(); ++faceId) {
        const igIndex* faceIds = nullptr;
        const int count = mesh->GetFaces()->GetCellIds(faceId, faceIds);
        std::vector<detail::PolygonId> ids(faceIds, faceIds + count);
        std::vector<detail::PolygonPoint> points;
        points.reserve(count);
        for (const auto id : ids) {
            const auto point = mesh->GetPoint(static_cast<igIndex>(id));
            points.push_back({point[0], point[1], point[2]});
        }
        std::vector<detail::PolygonTriangle> triangles;
        if (!detail::TriangulatePolygon(ids, points, occupiedEdges, triangles)) {
            igError("Cannot completely triangulate face {} while preserving its boundary and mesh topology", faceId);
            return false; // Never publish a partial surface with missing faces.
        }
        for (const auto& triangle : triangles) {
            Faces->AddCellId3(static_cast<igIndex>(triangle[0]),
                              static_cast<igIndex>(triangle[1]),
                              static_cast<igIndex>(triangle[2]));
            FaceIdMap.push_back(static_cast<int>(faceId));
        }
    }

    SurfaceMesh::Pointer Mesh = SurfaceMesh::New();
    Mesh->SetName(mesh->GetName());
    Mesh->SetPoints(Points);
    Mesh->SetFaces(Faces);

    double cell[IGAME_CELL_MAX_SIZE]{};
    auto newAttrs = Mesh->GetAttributeSet();
    for (int i = 0; Attrs && i < Attrs->GetNumberOfAttributes(); i++) {
        auto& attr = Attrs->GetAttribute(i);
        if (attr.attachmentType == IG_POINT) {
            FloatArray::Pointer arr = FloatArray::New();
            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Reserve(attr.pointer->GetNumberOfElements());

            for (IGsize j = 0; j < attr.pointer->GetNumberOfValues(); j++) {
                arr->AddValue(attr.pointer->GetValue(j));
            }
            newAttrs->AddAttribute(attr.type, IG_POINT, arr);

        } else {

            FloatArray::Pointer arr = FloatArray::New();
            arr->SetName(attr.pointer->GetName());
            arr->SetDimension(attr.pointer->GetDimension());
            arr->Resize(Faces->GetNumberOfCells());

            for (IGsize j = 0; j < Faces->GetNumberOfCells(); j++) {
                attr.pointer->GetElement(FaceIdMap[j], cell);
                arr->SetElement(j, cell);
            }
            newAttrs->AddAttribute(attr.type, IG_CELL, arr);
        }
    }

    SetOutput(Mesh);
    return true;
}

MeshTriangulationFilter::MeshTriangulationFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END


