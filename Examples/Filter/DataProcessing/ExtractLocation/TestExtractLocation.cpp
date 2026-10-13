// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/DataProcessing/ExtractLocation/TestExtractLocation.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
// Regression: PR #170 deleted this example and its build target. Keep the
// algorithm edge cases and model fixtures runnable after merging upstream.
// Fix: Merge upstream main and restore filter model regressions.
// Find: git log --diff-filter=A --oneline -- Examples/Filter/DataProcessing/ExtractLocation/TestExtractLocation.cpp
#include <DataProcessing/ExtractLocation/iGameExtractLocationFilter.h>
#include <iGameFileIO.h>

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void AddTetra(const iGame::UnstructuredMesh::Pointer& mesh, const std::vector<iGame::Point>& points) {
    igIndex ids[4]{};
    for (int index = 0; index < 4; ++index) ids[index] = mesh->AddPoint(points[index]);
    mesh->AddCell(ids, 4, iGame::IG_TETRA);
}

iGame::UnstructuredMesh::Pointer MakeTwoTetraMesh() {
    auto mesh = iGame::UnstructuredMesh::New();
    AddTetra(mesh, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}});
    AddTetra(mesh, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, -1.0}});

    auto pointVector = iGame::FloatArray::New();
    pointVector->SetName("PointVector");
    pointVector->SetDimension(3);
    for (igIndex pointId = 0; pointId < mesh->GetNumberOfPoints(); ++pointId) {
        const float value[3]{static_cast<float>(pointId), static_cast<float>(pointId) + 0.5f,
                             static_cast<float>(pointId) + 1.0f};
        pointVector->AddElement(value);
    }
    mesh->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_POINT, pointVector);

    auto cellScalar = iGame::IntArray::New();
    cellScalar->SetName("CellScalar");
    cellScalar->SetDimension(1);
    cellScalar->AddValue(10);
    cellScalar->AddValue(20);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, cellScalar);

    auto largeSignedIds = iGame::LongLongArray::New();
    largeSignedIds->SetName("LargeSignedIds");
    largeSignedIds->SetDimension(1);
    constexpr long long signedBase = 9007199254740993LL; // 2^53 + 1
    for (igIndex pointId = 0; pointId < mesh->GetNumberOfPoints(); ++pointId)
        largeSignedIds->AddValue(signedBase + pointId);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, largeSignedIds);

    auto largeUnsignedIds = iGame::UnsignedLongLongArray::New();
    largeUnsignedIds->SetName("LargeUnsignedIds");
    largeUnsignedIds->SetDimension(1);
    constexpr unsigned long long unsignedBase = 9007199254741993ULL;
    largeUnsignedIds->AddValue(unsignedBase);
    largeUnsignedIds->AddValue(unsignedBase + 1ULL);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, largeUnsignedIds);
    return mesh;
}

bool CheckIds(const char* name, const std::vector<igIndex>& actual, const std::vector<igIndex>& expected) {
    if (actual == expected) {
        std::cout << "[PASS] " << name << '\n';
        return true;
    }
    std::cerr << "[FAIL] " << name << "\n";
    return false;
}

bool RunInsideCellCase() {
    auto filter = iGame::ExtractLocationFilter::New();
    filter->SetInput(MakeTwoTetraMesh());
    filter->SetLocation(0.2, 0.2, 0.2);
    if (!filter->Execute() || !CheckIds("point inside one tetrahedron", filter->GetExtractedCellIds(), {0})) return false;

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (output.IsNull() || output->GetNumberOfCells() != 1 || output->GetNumberOfPoints() != 4) {
        std::cerr << "[FAIL] extracted topology\n";
        return false;
    }
    auto* originalIds = output->GetAttributeSet()->GetArrayPointer(
            IG_SCALAR, IG_CELL, iGame::ExtractLocationFilter::OriginalCellIdsArrayName());
    auto originalCellIds64 = iGame::DynamicCast<iGame::LongLongArray>(originalIds);
    if (originalCellIds64 == nullptr || originalCellIds64->GetNumberOfElements() != 1 ||
        originalCellIds64->RawPointer()[0] != 0) {
        std::cerr << "[FAIL] vtkOriginalCellIds output\n";
        return false;
    }
    std::cout << "[PASS] vtkOriginalCellIds output\n";

    auto* originalPointIds = output->GetAttributeSet()->GetArrayPointer(
            IG_SCALAR, IG_POINT,
            iGame::ExtractLocationFilter::OriginalPointIdsArrayName());
    auto originalPointIds64 = iGame::DynamicCast<iGame::LongLongArray>(originalPointIds);
    if (originalPointIds64 == nullptr || originalPointIds64->GetNumberOfElements() != 4 ||
        originalPointIds64->RawPointer()[0] != 0 || originalPointIds64->RawPointer()[3] != 3) {
        std::cerr << "[FAIL] vtkOriginalPointIds output\n";
        return false;
    }
    std::cout << "[PASS] vtkOriginalPointIds output\n";

    auto* copiedCellScalar = output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "CellScalar");
    auto* copiedPointVector = output->GetAttributeSet()->GetArrayPointer(IG_VECTOR, IG_POINT, "PointVector");
    if (copiedCellScalar == nullptr || iGame::DynamicCast<iGame::IntArray>(copiedCellScalar) == nullptr ||
        copiedCellScalar->GetNumberOfElements() != 1 ||
        copiedCellScalar->GetValue(0) != 10.0 || copiedPointVector == nullptr ||
        iGame::DynamicCast<iGame::FloatArray>(copiedPointVector) == nullptr ||
        copiedPointVector->GetNumberOfElements() != 4) {
        std::cerr << "[FAIL] extracted data arrays\n";
        return false;
    }
    std::cout << "[PASS] extracted point and cell data arrays\n";

    auto* copiedSignedBase = output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_POINT, "LargeSignedIds");
    auto* copiedUnsignedBase = output->GetAttributeSet()->GetArrayPointer(IG_SCALAR, IG_CELL, "LargeUnsignedIds");
    auto copiedSigned = iGame::DynamicCast<iGame::LongLongArray>(copiedSignedBase);
    auto copiedUnsigned = iGame::DynamicCast<iGame::UnsignedLongLongArray>(copiedUnsignedBase);
    if (copiedSigned == nullptr || copiedUnsigned == nullptr ||
        copiedSigned->RawPointer()[0] != 9007199254740993LL ||
        copiedSigned->RawPointer()[3] != 9007199254740996LL ||
        copiedUnsigned->RawPointer()[0] != 9007199254741993ULL) {
        std::cerr << "[FAIL] exact 64-bit integer array preservation\n";
        return false;
    }
    std::cout << "[PASS] exact 64-bit integer array preservation\n";
    return true;
}

bool RunSharedFaceCase() {
    auto filter = iGame::ExtractLocationFilter::New();
    filter->SetInput(MakeTwoTetraMesh());
    filter->SetLocation(0.2, 0.2, 0.0);
    return filter->Execute() &&
           CheckIds("shared face follows first-cell lookup semantics",
                    filter->GetExtractedCellIds(), {0});
}

bool RunOutsideCase() {
    auto filter = iGame::ExtractLocationFilter::New();
    filter->SetInput(MakeTwoTetraMesh());
    filter->SetLocation(2.0, 2.0, 2.0);
    if (!filter->Execute() || !CheckIds("point outside every cell", filter->GetExtractedCellIds(), {})) return false;
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    return output != nullptr && output->GetNumberOfCells() == 0;
}

bool RunUnsupportedInputCase() {
    auto mesh = iGame::UnstructuredMesh::New();
    igIndex ids[3]{static_cast<igIndex>(mesh->AddPoint({0.0, 0.0, 0.0})),
                   static_cast<igIndex>(mesh->AddPoint({1.0, 0.0, 0.0})),
                   static_cast<igIndex>(mesh->AddPoint({0.0, 1.0, 0.0}))};
    mesh->AddCell(ids, 3, iGame::IG_TRIANGLE);
    auto filter = iGame::ExtractLocationFilter::New();
    filter->SetInput(mesh);
    filter->SetLocation(0.2, 0.2, 0.0);
    if (!filter->Execute() && !filter->GetLastError().empty()) {
        std::cout << "[PASS] unsupported input reports an error\n";
        return true;
    }
    std::cerr << "[FAIL] unsupported input reports an error\n";
    return false;
}

bool CheckModelLocation(const char* fileName, const iGame::Point& location,
                        igIndex expectedCellId, const char* caseName) {
    auto dataObject = iGame::FileIO::ReadFile(fileName);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(dataObject);
    if (mesh.IsNull()) {
        std::cerr << "[FAIL] read " << fileName << '\n';
        return false;
    }
    auto filter = iGame::ExtractLocationFilter::New();
    filter->SetInput(mesh);
    filter->SetLocation(location);
    return filter->Execute() &&
           CheckIds(caseName, filter->GetExtractedCellIds(), {expectedCellId});
}

bool CheckGeneratedCell(const std::vector<iGame::Point>& points, int cellType,
                        const iGame::Point& location, const char* caseName) {
    auto mesh = iGame::UnstructuredMesh::New();
    std::vector<igIndex> ids;
    ids.reserve(points.size());
    for (const auto& point : points) ids.push_back(mesh->AddPoint(point));
    mesh->AddCell(ids.data(), static_cast<igIndex>(ids.size()), cellType);

    auto filter = iGame::ExtractLocationFilter::New();
    filter->SetInput(mesh);
    filter->SetLocation(location);
    return filter->Execute() && CheckIds(caseName, filter->GetExtractedCellIds(), {0});
}

bool RunSupportedCellModelCases() {
    return CheckModelLocation("./Models/ExtractLocationTetraPair.vtk",
                              {0.2, 0.2, 0.2}, 0,
                              "model: upper tetrahedron") &&
           CheckModelLocation("./Models/ExtractLocationTetraPair.vtk",
                              {0.2, 0.2, -0.2}, 1,
                              "model: lower tetrahedron") &&
           CheckGeneratedCell({{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                               {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
                              iGame::IG_HEXAHEDRON, {0.5, 0.5, 0.5}, "hexahedron") &&
           CheckGeneratedCell({{0, 0, 0}, {1, 0, 0}, {0, 1, 0},
                               {0, 0, 1}, {1, 0, 1}, {0, 1, 1}},
                              iGame::IG_PRISM, {0.2, 0.2, 0.5}, "prism") &&
           CheckGeneratedCell({{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, 1}},
                              iGame::IG_PYRAMID, {0.5, 0.5, 0.25}, "pyramid") &&
           CheckModelLocation("./Models/ExtractLocationConnectedSteppedSolid.vtk",
                              {0.5, 0.5, 0.5}, 0,
                              "connected stepped solid lower first cell") &&
           CheckModelLocation("./Models/ExtractLocationConnectedSteppedSolid.vtk",
                              {3.5, 2.5, 0.5}, 11,
                              "connected stepped solid lower far cell") &&
           CheckModelLocation("./Models/ExtractLocationConnectedSteppedSolid.vtk",
                              {2.5, 1.5, 1.5}, 17,
                              "connected stepped solid middle cell") &&
           CheckModelLocation("./Models/ExtractLocationConnectedSteppedSolid.vtk",
                              {1.5, 0.5, 2.5}, 19,
                              "connected stepped solid top cell");
}

} // namespace

int main() {
    const bool passed = RunInsideCellCase() && RunSharedFaceCase() &&
                        RunOutsideCase() && RunUnsupportedInputCase() &&
                        RunSupportedCellModelCases();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
