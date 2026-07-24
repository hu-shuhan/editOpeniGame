#ifndef iGameDataCodecTestingDataGenerator_h
#define iGameDataCodecTestingDataGenerator_h

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameDrawObject.h"
#include "iGameFlatArray.h"
#include "iGameMacro.h"
#include "iGamePoints.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <cstddef>
#include <fstream>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
namespace iGame::datacodec_test {
using iGame::AttributeSet;
using iGame::CellArray;
using iGame::DataObject;
using iGame::DrawObject;
using iGame::FloatArray;
using iGame::Points;
using iGame::PointSet;
using iGame::StructuredMesh;
using iGame::SurfaceMesh;
using iGame::UnstructuredMesh;
using iGame::UnsignedIntArray;
using iGame::VolumeMesh;

struct SyntheticSurfaceOptions {
    std::string name{"synthetic_surface_smoke"};
    float zOffset{0.0f};
    float pointScalarOffset{0.0f};
    float cellScalarOffset{0.0f};
};

enum class SyntheticDataShape {
    Surface,
    PointSet,
    Volume,
    Unstructured,
    Structured,
    Polyhedron,
};

[[nodiscard]] inline const char* SyntheticDataShapeName(const SyntheticDataShape shape) {
    switch (shape) {
        case SyntheticDataShape::Surface:
            return "surface";
        case SyntheticDataShape::PointSet:
            return "pointset";
        case SyntheticDataShape::Volume:
            return "volume";
        case SyntheticDataShape::Unstructured:
            return "unstructured";
        case SyntheticDataShape::Structured:
            return "structured";
        case SyntheticDataShape::Polyhedron:
            return "polyhedron";
    }
    return "unknown";
}

[[nodiscard]] inline Points::Pointer BuildSyntheticTetraPoints(const float zOffset = 0.0f) {
    auto points = Points::New();
    points->AddPoint(0.0f, 0.0f, zOffset);
    points->AddPoint(1.0f, 0.0f, zOffset);
    points->AddPoint(0.0f, 1.0f, zOffset);
    points->AddPoint(0.0f, 0.0f, 1.0f + zOffset);
    return points;
}

[[nodiscard]] inline Points::Pointer BuildSyntheticHexPoints(const float zOffset = 0.0f) {
    auto points = Points::New();
    points->AddPoint(0.0f, 0.0f, zOffset);
    points->AddPoint(1.0f, 0.0f, zOffset);
    points->AddPoint(1.0f, 1.0f, zOffset);
    points->AddPoint(0.0f, 1.0f, zOffset);
    points->AddPoint(0.0f, 0.0f, 1.0f + zOffset);
    points->AddPoint(1.0f, 0.0f, 1.0f + zOffset);
    points->AddPoint(1.0f, 1.0f, 1.0f + zOffset);
    points->AddPoint(0.0f, 1.0f, 1.0f + zOffset);
    return points;
}

[[nodiscard]] inline FloatArray::Pointer BuildSyntheticScalarArray(
    const std::string& name,
    const std::size_t count,
    const float offset) {
    auto array = FloatArray::New();
    array->SetName(name);
    array->SetDimension(1);
    for (std::size_t index = 0u; index < count; ++index) {
        const float value = offset + static_cast<float>(index);
        array->AddElement(&value);
    }
    return array;
}

[[nodiscard]] inline AttributeSet::Pointer BuildSyntheticAttributeSet(
    const std::string& prefix,
    const std::size_t pointCount,
    const std::size_t cellCount,
    const float pointScalarOffset,
    const float cellScalarOffset) {
    auto attributes = AttributeSet::New();
    attributes->AddAttribute(
        IG_SCALAR,
        IG_POINT,
        BuildSyntheticScalarArray(prefix + "_point_scalar", pointCount, pointScalarOffset));
    if (cellCount > 0u) {
        attributes->AddAttribute(
            IG_SCALAR,
            IG_CELL,
            BuildSyntheticScalarArray(prefix + "_cell_scalar", cellCount, cellScalarOffset));
    }
    return attributes;
}

[[nodiscard]] inline bool IsSyntheticInputPath(const std::filesystem::path& inputPath) {
    return inputPath.string().rfind("synthetic:", 0u) == 0u;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticSurfaceSmokeObject(const SyntheticSurfaceOptions& options) {
    auto mesh = SurfaceMesh::New();
    mesh->SetName(options.name);

    auto points = Points::New();
    points->AddPoint(0.0f, 0.0f, options.zOffset);
    points->AddPoint(1.0f, 0.0f, options.zOffset);
    points->AddPoint(0.0f, 1.0f, options.zOffset);
    points->AddPoint(1.0f, 1.0f, 0.25f + options.zOffset);
    mesh->SetPoints(points);

    auto faces = CellArray::New();
    const igIndex triangle0[] = {0, 1, 2};
    const igIndex triangle1[] = {1, 3, 2};
    faces->AddCellIds(triangle0, 3);
    faces->AddCellIds(triangle1, 3);
    mesh->SetFaces(faces);

    mesh->SetAttributeSet(BuildSyntheticAttributeSet(
        "synthetic_surface",
        4u,
        2u,
        options.pointScalarOffset,
        options.cellScalarOffset));

    return mesh;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticSurfaceSmokeObject() {
    return BuildSyntheticSurfaceSmokeObject(SyntheticSurfaceOptions{});
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticPointSetSmokeObject();
[[nodiscard]] inline DataObject::Pointer BuildSyntheticVolumeSmokeObject();
[[nodiscard]] inline DataObject::Pointer BuildSyntheticUnstructuredSmokeObject();
[[nodiscard]] inline DataObject::Pointer BuildSyntheticStructuredSmokeObject();
[[nodiscard]] inline DataObject::Pointer BuildSyntheticPolyhedronSmokeObject();

[[nodiscard]] inline DataObject::Pointer BuildSyntheticMultiBlockSmokeObject() {
    auto root = DrawObject::New();
    root->SetName("synthetic_multi_block_smoke");
    root->AddSubDataObject(BuildSyntheticSurfaceSmokeObject(SyntheticSurfaceOptions{
        .name = "synthetic_surface_part_0",
        .zOffset = 0.0f,
        .pointScalarOffset = 0.0f,
        .cellScalarOffset = 0.0f,
    }));
    root->AddSubDataObject(BuildSyntheticSurfaceSmokeObject(SyntheticSurfaceOptions{
        .name = "synthetic_surface_part_1",
        .zOffset = 0.5f,
        .pointScalarOffset = 10.0f,
        .cellScalarOffset = 100.0f,
    }));
    return root;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticNestedMultiBlockSmokeObject() {
    auto root = DrawObject::New();
    root->SetName("synthetic_nested_multi_block_smoke");
    auto branch = DrawObject::New();
    branch->SetName("synthetic_nested_branch");
    branch->AddSubDataObject(BuildSyntheticSurfaceSmokeObject(SyntheticSurfaceOptions{
        .name = "synthetic_nested_surface",
        .zOffset = 0.0f,
        .pointScalarOffset = 0.0f,
        .cellScalarOffset = 0.0f,
    }));
    branch->AddSubDataObject(BuildSyntheticSurfaceSmokeObject(SyntheticSurfaceOptions{
        .name = "synthetic_nested_surface_offset",
        .zOffset = 1.0f,
        .pointScalarOffset = 20.0f,
        .cellScalarOffset = 200.0f,
    }));
    root->AddSubDataObject(branch);
    root->AddSubDataObject(BuildSyntheticPointSetSmokeObject());
    return root;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticPointSetSmokeObject() {
    auto pointSet = PointSet::New();
    pointSet->SetName("synthetic_pointset_smoke");
    auto points = Points::New();
    points->AddPoint(0.0f, 0.0f, 0.0f);
    points->AddPoint(1.0f, 0.0f, 0.0f);
    points->AddPoint(0.0f, 1.0f, 0.0f);
    points->AddPoint(0.0f, 0.0f, 1.0f);
    pointSet->SetPoints(points);
    pointSet->SetAttributeSet(BuildSyntheticAttributeSet("synthetic_pointset", 4u, 0u, 0.0f, 0.0f));
    return pointSet;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticVolumeSmokeObject() {
    auto mesh = VolumeMesh::New();
    mesh->SetName("synthetic_volume_tetra_smoke");
    mesh->SetPoints(BuildSyntheticTetraPoints());
    auto volumes = CellArray::New();
    const igIndex tetra[] = {0, 1, 2, 3};
    volumes->AddCellIds(tetra, 4);
    mesh->SetVolumes(volumes);
    mesh->SetAttributeSet(BuildSyntheticAttributeSet("synthetic_volume", 4u, 1u, 0.0f, 10.0f));
    return mesh;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticUnstructuredSmokeObject() {
    auto mesh = UnstructuredMesh::New();
    mesh->SetName("synthetic_unstructured_tetra_smoke");
    mesh->SetPoints(BuildSyntheticTetraPoints());
    auto cells = CellArray::New();
    const igIndex tetra[] = {0, 1, 2, 3};
    cells->AddCellIds(tetra, 4);
    auto types = UnsignedIntArray::New();
    types->AddValue(iGame::IG_TETRA);
    mesh->SetCells(cells, types);
    mesh->SetAttributeSet(BuildSyntheticAttributeSet("synthetic_unstructured", 4u, 1u, 0.0f, 10.0f));
    return mesh;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticStructuredSmokeObject() {
    auto mesh = StructuredMesh::New();
    mesh->SetName("synthetic_structured_hex_smoke");
    mesh->SetPoints(BuildSyntheticHexPoints());
    igIndex size[] = {2, 2, 2};
    igIndex extent[] = {0, 1, 0, 1, 0, 1};
    mesh->SetDimensionSize(size);
    mesh->SetExtent(extent);
    mesh->GenStructuredCellConnectivities();
    mesh->SetAttributeSet(BuildSyntheticAttributeSet("synthetic_structured", 8u, 1u, 0.0f, 10.0f));
    return mesh;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticPolyhedronSmokeObject() {
    auto mesh = VolumeMesh::New();
    mesh->SetName("synthetic_polyhedron_hex_smoke");
    mesh->SetPoints(BuildSyntheticHexPoints());

    auto faces = CellArray::New();
    const igIndex bottom[] = {0, 1, 2, 3};
    const igIndex top[] = {4, 5, 6, 7};
    const igIndex front[] = {0, 1, 5, 4};
    const igIndex right[] = {1, 2, 6, 5};
    const igIndex back[] = {2, 3, 7, 6};
    const igIndex left[] = {3, 0, 4, 7};
    faces->AddCellIds(bottom, 4);
    faces->AddCellIds(top, 4);
    faces->AddCellIds(front, 4);
    faces->AddCellIds(right, 4);
    faces->AddCellIds(back, 4);
    faces->AddCellIds(left, 4);

    auto volumeFaces = CellArray::New();
    const igIndex faceIds[] = {0, 1, 2, 3, 4, 5};
    volumeFaces->AddCellIds(faceIds, 6);
    mesh->InitVolumesWithPolyhedron(faces, volumeFaces);
    mesh->SetAttributeSet(BuildSyntheticAttributeSet("synthetic_polyhedron", 8u, 1u, 0.0f, 10.0f));
    return mesh;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticSingleBlockSmokeObject(const SyntheticDataShape shape) {
    switch (shape) {
        case SyntheticDataShape::Surface:
            return BuildSyntheticSurfaceSmokeObject();
        case SyntheticDataShape::PointSet:
            return BuildSyntheticPointSetSmokeObject();
        case SyntheticDataShape::Volume:
            return BuildSyntheticVolumeSmokeObject();
        case SyntheticDataShape::Unstructured:
            return BuildSyntheticUnstructuredSmokeObject();
        case SyntheticDataShape::Structured:
            return BuildSyntheticStructuredSmokeObject();
        case SyntheticDataShape::Polyhedron:
            return BuildSyntheticPolyhedronSmokeObject();
    }
    return nullptr;
}

inline bool WriteSyntheticSurfaceVtkFile(
    const std::filesystem::path& outputPath,
    const SyntheticSurfaceOptions& options,
    std::string* error = nullptr) {
    std::error_code errorCode;
    const auto parent = outputPath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, errorCode);
        if (errorCode) {
            if (error != nullptr) {
                *error = "failed to create synthetic vtk directory";
            }
            return false;
        }
    }

    std::ofstream stream(outputPath, std::ios::binary);
    if (!stream.is_open()) {
        if (error != nullptr) {
            *error = "failed to open synthetic vtk output";
        }
        return false;
    }

    const float pointValues[] = {
        0.0f + options.pointScalarOffset,
        1.0f + options.pointScalarOffset,
        2.0f + options.pointScalarOffset,
        3.5f + options.pointScalarOffset};
    const float cellValues[] = {
        10.0f + options.cellScalarOffset,
        20.0f + options.cellScalarOffset};

    stream << "# vtk DataFile Version 3.0\n";
    stream << options.name << "\n";
    stream << "ASCII\n";
    stream << "DATASET POLYDATA\n";
    stream << "POINTS 4 float\n";
    stream << "0 0 " << options.zOffset << "\n";
    stream << "1 0 " << options.zOffset << "\n";
    stream << "0 1 " << options.zOffset << "\n";
    stream << "1 1 " << 0.25f + options.zOffset << "\n";
    stream << "POLYGONS 2 8\n";
    stream << "3 0 1 2\n";
    stream << "3 1 3 2\n";
    stream << "POINT_DATA 4\n";
    stream << "SCALARS synthetic_point_scalar float 1\n";
    stream << "LOOKUP_TABLE default\n";
    for (const auto value : pointValues) {
        stream << value << "\n";
    }
    stream << "CELL_DATA 2\n";
    stream << "SCALARS synthetic_cell_scalar float 1\n";
    stream << "LOOKUP_TABLE default\n";
    for (const auto value : cellValues) {
        stream << value << "\n";
    }

    if (!stream.good()) {
        if (error != nullptr) {
            *error = "failed to write synthetic vtk content";
        }
        return false;
    }
    return true;
}

[[nodiscard]] inline DataObject::Pointer BuildSyntheticSourceObject(const std::filesystem::path& inputPath) {
    const auto inputName = inputPath.string();
    if (inputName == "synthetic:surface-smoke") {
        return BuildSyntheticSingleBlockSmokeObject(SyntheticDataShape::Surface);
    }
    if (inputName == "synthetic:multi-block-smoke") {
        return BuildSyntheticMultiBlockSmokeObject();
    }
    if (inputName == "synthetic:nested-multi-block-smoke") {
        return BuildSyntheticNestedMultiBlockSmokeObject();
    }
    if (inputName == "synthetic:pointset-smoke") {
        return BuildSyntheticSingleBlockSmokeObject(SyntheticDataShape::PointSet);
    }
    if (inputName == "synthetic:volume-smoke") {
        return BuildSyntheticSingleBlockSmokeObject(SyntheticDataShape::Volume);
    }
    if (inputName == "synthetic:unstructured-smoke") {
        return BuildSyntheticSingleBlockSmokeObject(SyntheticDataShape::Unstructured);
    }
    if (inputName == "synthetic:structured-smoke") {
        return BuildSyntheticSingleBlockSmokeObject(SyntheticDataShape::Structured);
    }
    if (inputName == "synthetic:polyhedron-smoke") {
        return BuildSyntheticSingleBlockSmokeObject(SyntheticDataShape::Polyhedron);
    }
    return nullptr;
}

} // namespace iGame::datacodec_test

#endif
