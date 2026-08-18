#include <ModelSurface/iGameModelGeometryFilter.h>
#include <iGameFileIO.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <string>
#include <vector>

namespace
{

struct ExtractionResult {
    IGsize faceCount{0};
    IGsize edgeCount{0};
};

bool Extract(iGame::UnstructuredMesh::Pointer mesh, const int threadCount, ExtractionResult& result,
             const bool useFullCellExtent = false, const bool pointMerging = true) {
    auto filter = iGame::ModelGeometryFilter::New();
    filter->SetMaxThreadSize(threadCount);
    filter->SetPointMerging(pointMerging);
    if (useFullCellExtent && mesh->GetNumberOfCells() > 0) {
        filter->SetCellIndexExtent(0, static_cast<igIndex>(mesh->GetNumberOfCells() - 1));
    }

    auto surface = iGame::SurfaceMesh::New();
    if (!filter->Execute(mesh, surface)) { return false; }
    result.faceCount = surface->GetNumberOfFaces();
    result.edgeCount = surface->GetNumberOfEdges();
    return true;
}

bool ValidateVtkFile(const std::string& fileName) {
    auto object = iGame::FileIO::ReadFile(fileName);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(object);
    if (mesh == nullptr) {
        std::cerr << "Unable to read unstructured mesh: " << fileName << '\n';
        return false;
    }

    ExtractionResult serial;
    ExtractionResult parallel;
    if (!Extract(mesh, 1, serial) || !Extract(mesh, 8, parallel)) {
        std::cerr << "Initial surface extraction failed\n";
        return false;
    }
    if (serial.faceCount != 1376 || parallel.faceCount != serial.faceCount || parallel.edgeCount != serial.edgeCount ||
        serial.edgeCount == 0) {
        std::cerr << "Unexpected serial/parallel result: faces=" << serial.faceCount << '/' << parallel.faceCount
                  << ", edges=" << serial.edgeCount << '/' << parallel.edgeCount << '\n';
        return false;
    }

    ExtractionResult clipped;
    if (!Extract(mesh, 8, clipped, true) || clipped.faceCount != serial.faceCount ||
        clipped.edgeCount != serial.edgeCount) {
        std::cerr << "Full-range cell clipping changed the extracted surface\n";
        return false;
    }

    for (int iteration = 0; iteration < 100; ++iteration) {
        ExtractionResult stress;
        if (!Extract(mesh, 8, stress) || stress.faceCount != serial.faceCount || stress.edgeCount != serial.edgeCount) {
            std::cerr << "Parallel extraction failed at iteration " << iteration << '\n';
            return false;
        }
    }
    return true;
}

iGame::UnstructuredMesh::Pointer MakePolyhedron(const std::vector<igIndex>& connectivity, const int pointCount,
                                                const bool addTetra = false) {
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    for (int pointId = 0; pointId < pointCount; ++pointId) {
        points->AddPoint(static_cast<double>(pointId), static_cast<double>(pointId % 7), 0.0);
    }
    mesh->SetPoints(points);
    mesh->AddCell(const_cast<igIndex*>(connectivity.data()), static_cast<int>(connectivity.size()),
                  iGame::IG_POLYHEDRON);
    if (addTetra) {
        igIndex tetra[4] = {0, 1, 2, 3};
        mesh->AddCell(tetra, 4, iGame::IG_TETRA);
    }
    return mesh;
}

bool ValidateLargeAndInvalidPolyhedra() {
    constexpr int polygonPointCount = 300;
    constexpr int apexPointId = polygonPointCount;
    std::vector<igIndex> valid;
    valid.reserve(2 + polygonPointCount + polygonPointCount * 4);
    valid.push_back(polygonPointCount + 1);
    valid.push_back(polygonPointCount);
    for (int pointId = 0; pointId < polygonPointCount; ++pointId) { valid.push_back(pointId); }
    for (int pointId = 0; pointId < polygonPointCount; ++pointId) {
        valid.push_back(3);
        valid.push_back(pointId);
        valid.push_back((pointId + 1) % polygonPointCount);
        valid.push_back(apexPointId);
    }

    ExtractionResult result;
    if (!Extract(MakePolyhedron(valid, polygonPointCount + 1), 1, result, false, false) ||
        result.faceCount != polygonPointCount + 1) {
        std::cerr << "A 300-point polyhedron face was not extracted correctly\n";
        return false;
    }

    auto truncated = valid;
    truncated.pop_back();
    if (!Extract(MakePolyhedron(truncated, polygonPointCount + 1, true), 1, result, false, false) ||
        result.faceCount != 4) {
        std::cerr << "Truncated polyhedron connectivity was not rejected\n";
        return false;
    }

    auto invalidPoint = valid;
    invalidPoint.back() = polygonPointCount + 1;
    if (!Extract(MakePolyhedron(invalidPoint, polygonPointCount + 1, true), 1, result, false, false) ||
        result.faceCount != 4) {
        std::cerr << "Out-of-range polyhedron point id was not rejected\n";
        return false;
    }

    auto invalidFaceCount = valid;
    ++invalidFaceCount[0];
    if (!Extract(MakePolyhedron(invalidFaceCount, polygonPointCount + 1, true), 1, result, false, false) ||
        result.faceCount != 4) {
        std::cerr << "Invalid polyhedron face count was not rejected\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--polyhedron-only") { return ValidateLargeAndInvalidPolyhedra() ? 0 : 1; }
    const std::string fileName = argc > 1 ? argv[1] : "./VTKfile-0.vtk";
    if (!ValidateVtkFile(fileName) || !ValidateLargeAndInvalidPolyhedra()) { return 1; }
    std::cout << "Model geometry parallel validation passed\n";
    return 0;
}
