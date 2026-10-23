// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
#include <DataProcessing/ConnectedSurfaceProperties/iGameConnectedSurfacePropertiesFilter.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>
#include <iGameScene.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

namespace {

constexpr double Tolerance = 1.0e-9;

bool Close(double actual, double expected) {
    return std::abs(actual - expected) <= Tolerance * std::max(1.0, std::abs(expected));
}

bool Check(bool condition, const std::string& message) {
    if (condition) {
        std::cout << "[PASS] " << message << '\n';
        return true;
    }
    std::cerr << "[FAIL] " << message << '\n';
    return false;
}

bool HeapIsValid() {
#if defined(_MSC_VER) && defined(_DEBUG)
    return _CrtCheckMemory() != 0;
#else
    return true;
#endif
}

bool RunTwoBoxesCase() {
    auto input = iGame::DynamicCast<iGame::SurfaceMesh>(
            iGame::FileIO::ReadFile("./Models/ConnectedSurfacePropertiesTwoBoxes.vtk"));
    if (!Check(input != nullptr, "read the two-box PolyData model")) return false;

    auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(input);
    if (!Check(filter->Execute(), std::string("two-box execution: ") + filter->GetLastError())) return false;

    bool passed = true;
    passed &= Check(filter->GetNumberOfObjects() == 2, "identify two edge-connected surface objects");
    passed &= Check(filter->GetAllValid(), "both closed boxes are valid");
    passed &= Check(Close(filter->GetTotalArea(), 16.0), "total area matches ParaView reference (16)");
    passed &= Check(Close(filter->GetTotalVolume(), 3.0), "total volume matches ParaView reference (3)");

    auto output = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput());
    passed &= Check(output != nullptr && output != input, "create an independent output mesh");
    passed &= Check(input->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "ObjectIds") == nullptr,
                    "leave the input mesh unchanged");
    if (!output) return false;
    output->ConvertToDrawableData();
    passed &= Check(HeapIsValid(), "render conversion keeps the debug heap valid");
    {
        auto scene = iGame::Scene::New();
        scene->AddModel(output);
        passed &= Check(HeapIsValid(), "adding the output to a scene keeps the debug heap valid");
        scene->Finalize();
    }
    passed &= Check(HeapIsValid(), "destroying the scene keeps the debug heap valid");

    auto objectIds = iGame::DynamicCast<iGame::LongLongArray>(
            output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "ObjectIds"));
    auto areas = iGame::DynamicCast<iGame::DoubleArray>(
            output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "Areas"));
    auto volumes = iGame::DynamicCast<iGame::DoubleArray>(
            output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "Volumes"));
    passed &= Check(objectIds != nullptr && objectIds->GetNumberOfElements() == 12
                            && objectIds->RawPointer()[0] == 0 && objectIds->RawPointer()[5] == 0
                            && objectIds->RawPointer()[6] == 1 && objectIds->RawPointer()[11] == 1,
                    "write 64-bit ObjectIds cell data");
    passed &= Check(areas != nullptr && areas->GetNumberOfElements() == 12,
                    "write per-polygon Areas cell data");
    passed &= Check(volumes != nullptr && volumes->GetNumberOfElements() == 12,
                    "write per-polygon Volumes cell data");
    const double expectedAreas[12]{1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                                   2.0, 2.0, 2.0, 1.0, 2.0, 1.0};
    const double expectedVolumes[12]{1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0, -0.5,
                                     1.0 / 6.0, 5.0 / 6.0, 1.0 / 3.0, 1.0 / 3.0,
                                     1.0 / 3.0, 5.0 / 6.0, 1.0 / 3.0, -1.0 / 6.0};
    bool faceValuesMatch = areas != nullptr && volumes != nullptr;
    for (int faceId = 0; faceValuesMatch && faceId < 12; ++faceId) {
        faceValuesMatch = Close(areas->RawPointer()[faceId], expectedAreas[faceId])
                && Close(volumes->RawPointer()[faceId], expectedVolumes[faceId]);
    }
    passed &= Check(faceValuesMatch, "per-polygon Areas and Volumes match ParaView");
    passed &= Check(output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_POINT, "PointValue") != nullptr
                            && output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "FaceValue") != nullptr,
                    "preserve input point and cell arrays");

    auto validity = output->GetMetadata()->GetIntArray("ObjectValidity");
    auto objectAreas = output->GetMetadata()->GetDoubleArray("ObjectAreas");
    auto objectVolumes = output->GetMetadata()->GetDoubleArray("ObjectVolumes");
    auto centroids = output->GetMetadata()->GetDoubleArray("ObjectCentroids");
    passed &= Check(validity != nullptr && validity->GetNumberOfElements() == 2
                            && validity->RawPointer()[0] == 1 && validity->RawPointer()[1] == 1,
                    "store ObjectValidity per object");
    passed &= Check(objectAreas != nullptr && Close(objectAreas->RawPointer()[0], 6.0)
                            && Close(objectAreas->RawPointer()[1], 10.0),
                    "store ObjectAreas [6, 10]");
    passed &= Check(objectVolumes != nullptr && Close(objectVolumes->RawPointer()[0], 1.0)
                            && Close(objectVolumes->RawPointer()[1], 2.0),
                    "store ObjectVolumes [1, 2]");
    passed &= Check(centroids != nullptr && centroids->GetDimension() == 3
                            && Close(centroids->RawPointer(0)[0], 0.5)
                            && Close(centroids->RawPointer(0)[1], 0.5)
                            && Close(centroids->RawPointer(0)[2], 0.5)
                            && Close(centroids->RawPointer(1)[0], 4.0)
                            && Close(centroids->RawPointer(1)[1], 0.5)
                            && Close(centroids->RawPointer(1)[2], 0.5),
                    "store object centroids [(0.5,0.5,0.5), (4,0.5,0.5)]");
    return passed;
}

bool RunOpenPatchCase() {
    auto input = iGame::DynamicCast<iGame::SurfaceMesh>(
            iGame::FileIO::ReadFile("./Models/ConnectedSurfacePropertiesOpenPatch.vtk"));
    if (!Check(input != nullptr, "read the open-patch model")) return false;

    auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(input);
    if (!Check(filter->Execute(), std::string("open-patch execution: ") + filter->GetLastError())) return false;
    auto areas = filter->GetAreas();
    auto volumes = filter->GetVolumes();
    auto validity = filter->GetObjectValidity();
    auto objectAreas = filter->GetObjectAreas();
    auto objectVolumes = filter->GetObjectVolumes();
    auto centroids = filter->GetObjectCentroids();
    return Check(filter->GetNumberOfObjects() == 1, "open patch remains one connected object")
            && Check(!filter->GetAllValid(), "open patch is reported invalid")
            && Check(Close(filter->GetTotalArea(), 6.0), "open patch area is 6")
            && Check(Close(filter->GetTotalVolume(), 0.0), "invalid open patch contributes no total volume")
            && Check(areas != nullptr && Close(areas->RawPointer()[0], 3.0)
                            && Close(areas->RawPointer()[1], 3.0)
                            && volumes != nullptr && Close(volumes->RawPointer()[0], 0.0)
                            && Close(volumes->RawPointer()[1], 0.0),
                     "open-patch cell arrays match ParaView")
            && Check(validity != nullptr && validity->RawPointer()[0] == 0
                            && objectAreas != nullptr && Close(objectAreas->RawPointer()[0], 6.0)
                            && objectVolumes != nullptr && Close(objectVolumes->RawPointer()[0], 0.0),
                     "open-patch object arrays match ParaView")
            && Check(centroids != nullptr && std::isnan(centroids->RawPointer()[0]),
                     "zero-volume object centroid follows ParaView NaN semantics");
}

bool RunPolyhedraCase() {
    auto input = iGame::DynamicCast<iGame::SurfaceMesh>(
            iGame::FileIO::ReadFile("./Models/ConnectedSurfacePropertiesPolyhedra.vtk"));
    if (!Check(input != nullptr && input->GetNumberOfPoints() == 17
                       && input->GetNumberOfFaces() == 22,
               "read the triangulated closed-polyhedra model")) return false;
    input->ConvertToDrawableData();
    if (!Check(HeapIsValid(), "triangulated polyhedra render conversion keeps the debug heap valid"))
        return false;

    auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(input);
    if (!Check(filter->Execute(), std::string("polyhedra execution: ") + filter->GetLastError()))
        return false;

    const double expectedAreas[3]{5.0 + std::sqrt(5.0),
                                  4.0 + 4.0 * std::sqrt(10.0),
                                  4.0 * std::sqrt(3.0)};
    const double expectedVolumes[3]{1.0, 4.0, 4.0 / 3.0};
    const double expectedCentroids[3][3]{{2.0 / 3.0, 1.0 / 3.0, 0.5},
                                          {5.0, 1.0, 0.75},
                                          {9.0, 1.0, 1.0}};

    bool passed = true;
    passed &= Check(filter->GetNumberOfObjects() == 3,
                    "identify prism, pyramid and octahedron as three objects");
    passed &= Check(filter->GetAllValid(), "all three polyhedra are closed and valid");
    passed &= Check(Close(filter->GetTotalArea(), expectedAreas[0] + expectedAreas[1] + expectedAreas[2]),
                    "polyhedra total area matches the analytical value");
    passed &= Check(Close(filter->GetTotalVolume(), 19.0 / 3.0),
                    "polyhedra total volume matches 19/3");

    auto output = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput());
    auto objectIds = filter->GetObjectIds();
    auto validity = filter->GetObjectValidity();
    auto objectAreas = filter->GetObjectAreas();
    auto objectVolumes = filter->GetObjectVolumes();
    auto centroids = filter->GetObjectCentroids();
    passed &= Check(output != nullptr && output != input,
                    "polyhedra output is independent");
    passed &= Check(objectIds != nullptr && objectIds->GetNumberOfElements() == 22
                            && objectIds->RawPointer()[0] == 0 && objectIds->RawPointer()[7] == 0
                            && objectIds->RawPointer()[8] == 1 && objectIds->RawPointer()[13] == 1
                            && objectIds->RawPointer()[14] == 2 && objectIds->RawPointer()[21] == 2,
                    "write stable object IDs for all triangulated faces");

    for (int objectId = 0; objectId < 3; ++objectId) {
        bool propertiesMatch = validity != nullptr && objectAreas != nullptr
                && objectVolumes != nullptr && centroids != nullptr
                && validity->RawPointer()[objectId] == 1
                && Close(objectAreas->RawPointer()[objectId], expectedAreas[objectId])
                && Close(objectVolumes->RawPointer()[objectId], expectedVolumes[objectId]);
        for (int component = 0; propertiesMatch && component < 3; ++component) {
            propertiesMatch = Close(centroids->RawPointer(objectId)[component],
                                    expectedCentroids[objectId][component]);
        }
        passed &= Check(propertiesMatch,
                        std::string("polyhedron ") + std::to_string(objectId) + " properties match reference");
    }
    return passed;
}

bool RunNonPlanarQuadCase() {
    auto input = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(-0.000672542f, 0.019087199f, -0.166466996f));
    points->AddPoint(iGame::Point(-0.013616800f, 0.016878501f, -0.151962996f));
    points->AddPoint(iGame::Point(0.354883999f, 0.017226500f, 0.340409011f));
    points->AddPoint(iGame::Point(0.366243988f, 0.017344501f, 0.323711991f));
    auto faces = iGame::CellArray::New();
    const igIndex quad[4]{0, 1, 2, 3};
    faces->AddCellIds(quad, 4);
    input->SetPoints(points);
    input->SetFaces(faces);

    auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(input);
    if (!Check(filter->Execute(), std::string("non-planar quad execution: ") + filter->GetLastError()))
        return false;

    const auto areas = filter->GetAreas();
    const auto volumes = filter->GetVolumes();
    const auto centroids = filter->GetObjectCentroids();
    return Check(!filter->GetAllValid(), "single non-planar quad is an open surface")
            && Check(areas != nullptr && Close(areas->RawPointer()[0], 0.011728017340777696),
                     "non-planar quad area matches ParaView")
            && Check(volumes != nullptr && Close(volumes->RawPointer()[0], -3.559594693756579e-06),
                     "non-planar quad uses ParaView's diagonal for volume contribution")
            && Check(centroids != nullptr
                            && Close(centroids->RawPointer(0)[0], 0.17484521233789665)
                            && Close(centroids->RawPointer(0)[1], 0.01759939595526564)
                            && Close(centroids->RawPointer(0)[2], 0.08404713634056422),
                     "non-planar quad centroid matches ParaView");
}

bool RunConcavePolygonCase() {
    auto input = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(4.0f, 0.0f, -0.0677023f));
    points->AddPoint(iGame::Point(3.5f, 0.866025f, -0.0568293f));
    points->AddPoint(iGame::Point(2.775f, 0.389711f, 0.0104978f));
    points->AddPoint(iGame::Point(2.0f, 1.22465e-16f, 0.118678f));
    points->AddPoint(iGame::Point(2.5f, -0.866025f, 0.173661f));
    points->AddPoint(iGame::Point(3.5f, -0.866025f, -0.025554f));
    auto faces = iGame::CellArray::New();
    const igIndex polygon[6]{0, 1, 2, 3, 4, 5};
    faces->AddCellIds(polygon, 6);
    input->SetPoints(points);
    input->SetFaces(faces);

    auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(input);
    if (!Check(filter->Execute(), std::string("concave polygon execution: ") + filter->GetLastError()))
        return false;

    const auto areas = filter->GetAreas();
    const auto volumes = filter->GetVolumes();
    const auto centroids = filter->GetObjectCentroids();
    return Check(areas != nullptr && Close(areas->RawPointer()[0], 2.1382043063163794),
                 "concave polygon area matches ParaView")
            && Check(volumes != nullptr && Close(volumes->RawPointer()[0], -0.031863468059382796),
                     "concave polygon ear cutting matches ParaView")
            && Check(centroids != nullptr
                            && Close(centroids->RawPointer(0)[0], 3.12628704101614)
                            && Close(centroids->RawPointer(0)[1], -0.10247715252704519)
                            && Close(centroids->RawPointer(0)[2], 0.019895010027776664),
                     "concave polygon centroid matches ParaView");
}

iGame::SurfaceMesh::Pointer MakeTetraSurfaceWithIds() {
    auto mesh = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
    points->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));
    auto faces = iGame::CellArray::New();
    faces->AddCellId3(0, 2, 1);
    faces->AddCellId3(0, 1, 3);
    faces->AddCellId3(1, 2, 3);
    faces->AddCellId3(2, 0, 3);
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    auto regionIds = iGame::LongLongArray::New();
    regionIds->SetName("RegionIds");
    for (int index = 0; index < 4; ++index) regionIds->AddValue(0);
    mesh->GetAttributeSet()->AddScalar(IG_CELL, regionIds);
    return mesh;
}

bool RunSuppliedIdsCase() {
    auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(MakeTetraSurfaceWithIds());
    filter->SetSkipObjectIdentification(true);
    filter->SetObjectIdsArrayName("RegionIds");
    if (!Check(filter->Execute(), std::string("supplied ObjectIds execution: ") + filter->GetLastError()))
        return false;
    return Check(filter->GetUsedSuppliedObjectIds(), "use an existing 64-bit cell ObjectIds array")
            && Check(filter->GetNumberOfObjects() == 1, "supplied IDs define one object")
            && Check(Close(filter->GetTotalVolume(), 1.0 / 6.0), "supplied-ID tetrahedron volume is 1/6");
}

bool RunUnsupportedCase() {
    auto points = iGame::PointSet::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
    filter->SetInput(points);
    return Check(!filter->Execute() && !filter->GetLastError().empty(),
                 "unsupported input is rejected with an error message");
}

} // namespace

int main() {
    bool passed = true;
    passed &= RunTwoBoxesCase();
    passed &= RunOpenPatchCase();
    passed &= RunPolyhedraCase();
    passed &= RunNonPlanarQuadCase();
    passed &= RunConcavePolygonCase();
    passed &= RunSuppliedIdsCase();
    passed &= RunUnsupportedCase();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
