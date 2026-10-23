#include <filesystem>
// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// Regression: PR #170 deleted this example and its build target. Keep the
// algorithm edge cases and model fixtures runnable after merging upstream.
// Also retain PR #99's independent-output checks: execution must not mutate input.
// Fix: Merge filter model regressions into interaction fixes.
// Find: git log --diff-filter=A --oneline -- Examples/Filter/DataProcessing/OverlappingCellsDetector/TestOverlappingCellsDetector.cpp
#include <DataProcessing/OverlappingCellsDetector/iGameOverlappingCellsDetectorFilter.h>
#include <iGameFileIO.h>
#include <iGameStructuredMesh.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <vector>

namespace {

using CellPoints = std::vector<iGame::Point>;

void AddCell(const iGame::UnstructuredMesh::Pointer& mesh, const CellPoints& points, IGenum cellType) {
    std::vector<igIndex> pointIds(points.size());
    for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
        pointIds[pointIndex] = mesh->AddPoint(points[pointIndex]);
    }
    mesh->AddCell(pointIds.data(), static_cast<int>(pointIds.size()), cellType);
}

void AddVolumeCell(const iGame::Points::Pointer& points, const iGame::CellArray::Pointer& volumes,
                   const CellPoints& cellPoints) {
    std::vector<igIndex> pointIds(cellPoints.size());
    for (std::size_t pointIndex = 0; pointIndex < cellPoints.size(); ++pointIndex) {
        pointIds[pointIndex] = points->AddPoint(cellPoints[pointIndex]);
    }
    volumes->AddCellIds(pointIds.data(), static_cast<int>(pointIds.size()));
}

bool RunCase(const char* caseName, const std::vector<std::pair<CellPoints, IGenum>>& cells,
             const std::vector<igIndex>& expectedCounts) {
    auto mesh = iGame::UnstructuredMesh::New();
    for (const auto& cell : cells) AddCell(mesh, cell.first, cell.second);

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        std::cerr << caseName << ": filter execution failed\n";
        return false;
    }
    if (filter->GetNumberOfOverlapsPerCell() != expectedCounts) {
        std::cerr << caseName << ": unexpected NumberOfOverlapsPerCell values\n";
        return false;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (output.IsNull() || output.GetPointer() == mesh ||
        output->GetNumberOfPoints() != mesh->GetNumberOfPoints() ||
        output->GetNumberOfCells() != mesh->GetNumberOfCells()) {
        std::cerr << caseName << ": output must be an independent topology copy\n";
        return false;
    }
    if (mesh->GetAttributeSet()->GetAttributeIndex(
                iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName()) >= 0) {
        std::cerr << caseName << ": input mesh was modified\n";
        return false;
    }

    // 验证正式输出：独立输出网格附加了与 VTK 同名的 cell scalar。
    auto* attribute = output->GetAttributeSet()->GetArrayPointer(
            IG_SCALAR, IG_CELL,
            iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName());
    if (attribute == nullptr || attribute->GetNumberOfElements() != expectedCounts.size()) {
        std::cerr << caseName << ": output cell scalar is missing\n";
        return false;
    }

    std::cout << "[PASS] " << caseName << '\n';
    return true;
}

bool RunUnsupportedCase() {
    auto mesh = iGame::UnstructuredMesh::New();
    // 待提交：源算法已支持三角形/四边形表面；用仍不支持的线单元验证失败路径。
    AddCell(mesh, {{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f)}},
            iGame::IG_LINE);

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (filter->Execute() || filter->GetLastError().empty()) {
        std::cerr << "unsupported line: expected a safe failure with an error message\n";
        return false;
    }
    std::cout << "[PASS] unsupported line reports an error\n";
    return true;
}

bool RunModelCase(const char* fileName, const std::vector<igIndex>& expectedCounts,
                  const char* caseName) {
    auto dataObject = iGame::FileIO::ReadFile(fileName);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(dataObject);
    if (mesh.IsNull()) {
        std::cerr << "[FAIL] read " << fileName << '\n';
        return false;
    }

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    filter->SetTolerance(0.0);
    if (!filter->Execute() || filter->GetNumberOfOverlapsPerCell() != expectedCounts) {
        std::cerr << "[FAIL] " << caseName << ": unexpected NumberOfOverlapsPerCell values\n";
        return false;
    }
    std::cout << "[PASS] " << caseName << '\n';
    return true;
}

bool RunZeroOverlapModelCase(const char* fileName, IGsize expectedCellCount,
                             const char* caseName) {
    auto dataObject = iGame::FileIO::ReadFile(fileName);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(dataObject);
    if (mesh.IsNull() || mesh->GetNumberOfCells() != expectedCellCount) {
        std::cerr << "[FAIL] " << caseName << ": could not read the expected mesh\n";
        return false;
    }

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    filter->SetTolerance(0.0);
    if (!filter->Execute()) {
        std::cerr << "[FAIL] " << caseName << ": filter execution failed\n";
        return false;
    }

    const auto& counts = filter->GetNumberOfOverlapsPerCell();
    const auto nonzeroCells = std::count_if(counts.begin(), counts.end(),
                                            [](igIndex count) { return count != 0; });
    const auto overlapPairs = std::accumulate(counts.begin(), counts.end(), igIndex{0}) / 2;
    if (nonzeroCells != 0) {
        std::cerr << "[FAIL] " << caseName << ": expected no overlaps, found "
                  << overlapPairs << " pairs affecting " << nonzeroCells << " cells\n";
        return false;
    }

    std::cout << "[PASS] " << caseName << '\n';
    return true;
}

bool RunVolumeMeshCase(const CellPoints& firstCell, const CellPoints& secondCell) {
    auto mesh = iGame::VolumeMesh::New();
    auto points = iGame::Points::New();
    auto volumes = iGame::CellArray::New();
    AddVolumeCell(points, volumes, firstCell);
    AddVolumeCell(points, volumes, secondCell);
    mesh->SetPoints(points);
    mesh->SetVolumes(volumes);

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute() || filter->GetNumberOfOverlapsPerCell() != std::vector<igIndex>{1, 1}) {
        std::cerr << "VolumeMesh: expected overlapping cells to produce [1, 1]\n";
        return false;
    }
    auto output = iGame::DynamicCast<iGame::VolumeMesh>(filter->GetOutput());
    if (output.IsNull() || output.GetPointer() == mesh ||
        output->GetNumberOfPoints() != mesh->GetNumberOfPoints() ||
        output->GetNumberOfVolumes() != mesh->GetNumberOfVolumes() ||
        mesh->GetAttributeSet()->GetAttributeIndex(
                iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName()) >= 0) {
        std::cerr << "VolumeMesh: output must be independent and input unchanged\n";
        return false;
    }
    auto* attribute = output->GetAttributeSet()->GetArrayPointer(
            IG_SCALAR, IG_CELL,
            iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName());
    if (attribute == nullptr || attribute->GetNumberOfElements() != 2 ||
        attribute->GetArrayType() != IG_LongLongArray) {
        std::cerr << "VolumeMesh: output 64-bit cell scalar is missing\n";
        return false;
    }
    std::cout << "[PASS] overlapping VolumeMesh hexahedra\n";
    return true;
}

bool RunOverlappingSharedFaceHexahedraCase() {
    auto mesh = iGame::UnstructuredMesh::New();
    const CellPoints points{
            {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {1.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
            {0.f, 0.f, 1.f}, {1.f, 0.f, 1.f}, {1.f, 1.f, 1.f}, {0.f, 1.f, 1.f},
            {0.f, 0.f, 0.25f}, {1.f, 0.f, 0.25f},
            {1.f, 1.f, 0.25f}, {0.f, 1.f, 0.25f}};
    for (const auto& point : points) mesh->AddPoint(point);

    igIndex firstCell[]{0, 1, 2, 3, 4, 5, 6, 7};
    igIndex secondCell[]{8, 9, 10, 11, 4, 5, 6, 7};
    mesh->AddCell(firstCell, 8, iGame::IG_HEXAHEDRON);
    mesh->AddCell(secondCell, 8, iGame::IG_HEXAHEDRON);

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute() ||
        filter->GetNumberOfOverlapsPerCell() != std::vector<igIndex>{1, 1}) {
        std::cerr << "[FAIL] overlapping hexahedra sharing a face\n";
        return false;
    }
    std::cout << "[PASS] overlapping hexahedra sharing a face\n";
    return true;
}

bool RunStructuredMeshCase() {
    // 3 x 2 x 2 个点构成两个仅共享面的六面体：这是三维 StructuredMesh 的标准连接关系。
    auto mesh = iGame::StructuredMesh::New();
    igIndex dimensions[3]{3, 2, 2};
    mesh->SetDimensionSize(dimensions);
    auto points = iGame::Points::New();
    for (int z = 0; z < 2; ++z)
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 3; ++x) points->AddPoint(iGame::Point(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
    mesh->SetPoints(points);
    mesh->GenStructuredCellConnectivities();

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute() || filter->GetNumberOfOverlapsPerCell() != std::vector<igIndex>{0, 0}) {
        std::cerr << "StructuredMesh: face-touching hexahedra must produce [0, 0]\n";
        return false;
    }
    auto output = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
    if (output.IsNull() || output.GetPointer() == mesh ||
        output->GetNumberOfPoints() != mesh->GetNumberOfPoints() ||
        output->GetNumberOfCells() != mesh->GetNumberOfCells() ||
        mesh->GetAttributeSet()->GetAttributeIndex(
                iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName()) >= 0 ||
        output->GetAttributeSet()->GetAttributeIndex(
                iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName()) < 0) {
        std::cerr << "StructuredMesh: output must preserve type and be independent\n";
        return false;
    }
    std::cout << "[PASS] StructuredMesh face-touching hexahedra\n";
    return true;
}

} // namespace

int main() {
    const CellPoints referenceTetra{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                     iGame::Point(0.0f, 1.0f, 0.0f), iGame::Point(0.0f, 0.0f, 1.0f)}};
    const CellPoints overlappingTetra{{iGame::Point(0.2f, 0.2f, 0.2f), iGame::Point(1.2f, 0.2f, 0.2f),
                                       iGame::Point(0.2f, 1.2f, 0.2f), iGame::Point(0.2f, 0.2f, 1.2f)}};
    const CellPoints referenceHex{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                   iGame::Point(1.0f, 1.0f, 0.0f), iGame::Point(0.0f, 1.0f, 0.0f),
                                   iGame::Point(0.0f, 0.0f, 1.0f), iGame::Point(1.0f, 0.0f, 1.0f),
                                   iGame::Point(1.0f, 1.0f, 1.0f), iGame::Point(0.0f, 1.0f, 1.0f)}};
    const CellPoints overlappingHex{{iGame::Point(0.2f, 0.2f, 0.2f), iGame::Point(1.2f, 0.2f, 0.2f),
                                     iGame::Point(1.2f, 1.2f, 0.2f), iGame::Point(0.2f, 1.2f, 0.2f),
                                     iGame::Point(0.2f, 0.2f, 1.2f), iGame::Point(1.2f, 0.2f, 1.2f),
                                     iGame::Point(1.2f, 1.2f, 1.2f), iGame::Point(0.2f, 1.2f, 1.2f)}};
    const CellPoints referencePrism{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                     iGame::Point(0.0f, 1.0f, 0.0f), iGame::Point(0.0f, 0.0f, 1.0f),
                                     iGame::Point(1.0f, 0.0f, 1.0f), iGame::Point(0.0f, 1.0f, 1.0f)}};
    const CellPoints overlappingPrism{{iGame::Point(0.2f, 0.2f, 0.2f), iGame::Point(1.2f, 0.2f, 0.2f),
                                       iGame::Point(0.2f, 1.2f, 0.2f), iGame::Point(0.2f, 0.2f, 1.2f),
                                       iGame::Point(1.2f, 0.2f, 1.2f), iGame::Point(0.2f, 1.2f, 1.2f)}};
    const CellPoints referencePyramid{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                       iGame::Point(1.0f, 1.0f, 0.0f), iGame::Point(0.0f, 1.0f, 0.0f),
                                       iGame::Point(0.5f, 0.5f, 1.0f)}};
    const CellPoints overlappingPyramid{{iGame::Point(0.2f, 0.2f, 0.1f), iGame::Point(1.2f, 0.2f, 0.1f),
                                         iGame::Point(1.2f, 1.2f, 0.1f), iGame::Point(0.2f, 1.2f, 0.1f),
                                         iGame::Point(0.7f, 0.7f, 1.1f)}};

    bool passed = true;
    passed &= RunModelCase("./Models/OverlappingCellsDetectorValidation.vtk",
                           {1, 1, 0, 0}, "model: overlapping and disjoint tetrahedra");
    passed &= RunModelCase("./Models/OverlappingCellsDetectorFaceTouching.vtk",
                           {0, 0}, "model: face-touching tetrahedra");
    // 源仓库未提供此 7472 单元模型；保留该回归，缺少外部数据时明确跳过（待提交）。
    if (std::filesystem::exists("./Models/OverlappingCellsDetectorStreamTest.vtk"))
        passed &= RunZeroOverlapModelCase("./Models/OverlappingCellsDetectorStreamTest.vtk",
                                          7472, "model: conforming curved hexahedral mesh");
    else std::cout << "[SKIP] upstream stream fixture is unavailable\n";
    passed &= RunCase("disjoint tetrahedra", {{referenceTetra, iGame::IG_TETRA},
                                               {{{iGame::Point(2.0f, 0.0f, 0.0f), iGame::Point(3.0f, 0.0f, 0.0f),
                                                  iGame::Point(2.0f, 1.0f, 0.0f), iGame::Point(2.0f, 0.0f, 1.0f)}},
                                                iGame::IG_TETRA}},
                      {0, 0});
    passed &= RunCase("overlapping tetrahedra", {{referenceTetra, iGame::IG_TETRA}, {overlappingTetra, iGame::IG_TETRA}}, {1, 1});
    passed &= RunCase("face-touching tetrahedra", {{referenceTetra, iGame::IG_TETRA},
                                                     {{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                                        iGame::Point(0.0f, 1.0f, 0.0f), iGame::Point(0.0f, 0.0f, -1.0f)}},
                                                      iGame::IG_TETRA}},
                      {0, 0});
    passed &= RunCase("overlapping hexahedra", {{referenceHex, iGame::IG_HEXAHEDRON}, {overlappingHex, iGame::IG_HEXAHEDRON}}, {1, 1});
    passed &= RunCase("overlapping prisms", {{referencePrism, iGame::IG_PRISM}, {overlappingPrism, iGame::IG_PRISM}}, {1, 1});
    passed &= RunCase("overlapping pyramids", {{referencePyramid, iGame::IG_PYRAMID}, {overlappingPyramid, iGame::IG_PYRAMID}}, {1, 1});
    passed &= RunVolumeMeshCase(referenceHex, overlappingHex);
    passed &= RunOverlappingSharedFaceHexahedraCase();
    passed &= RunStructuredMeshCase();
    passed &= RunUnsupportedCase();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
