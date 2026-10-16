// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
#include <DataProcessing/TextureMapToPlane/iGameTextureMapToPlaneFilter.h>
#include <iGameFileIO.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

constexpr double Tolerance = 1.0e-5;

bool AlmostEqual(double actual, double expected) {
    return std::abs(actual - expected) <= Tolerance;
}

bool CheckCoordinates(const iGame::FloatArray::Pointer& coordinates,
                      const std::vector<std::array<double, 2>>& expected,
                      const char* caseName) {
    if (coordinates.IsNull() || coordinates->GetDimension() != 2 ||
        coordinates->GetNumberOfElements() != expected.size()) {
        std::cerr << "[FAIL] " << caseName << ": invalid Texture Coordinates array\n";
        return false;
    }
    for (IGsize pointId = 0; pointId < expected.size(); ++pointId) {
        const float* tuple = coordinates->RawPointer(pointId);
        if (!AlmostEqual(tuple[0], expected[pointId][0]) ||
            !AlmostEqual(tuple[1], expected[pointId][1])) {
            std::cerr << "[FAIL] " << caseName << ": point " << pointId
                      << " expected (" << expected[pointId][0] << ", " << expected[pointId][1]
                      << ") but got (" << tuple[0] << ", " << tuple[1] << ")\n";
            return false;
        }
    }
    std::cout << "[PASS] " << caseName << '\n';
    return true;
}

bool RunAutomaticModelCase() {
    auto input = iGame::FileIO::ReadFile("./Models/TextureMapToPlaneSlantedPatch.vtk");
    auto inputMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(input);
    if (inputMesh.IsNull()) {
        std::cerr << "[FAIL] read automatic test model\n";
        return false;
    }
    const iGame::Point originalPoint = inputMesh->GetPoint(0);
    auto filter = iGame::TextureMapToPlaneFilter::New();
    filter->SetInput(inputMesh);
    filter->SetOrigin({100.0f, 200.0f, 300.0f});
    filter->SetPoint1({101.0f, 200.0f, 300.0f});
    filter->SetPoint2({100.0f, 201.0f, 300.0f});
    if (!filter->Execute()) {
        std::cerr << "[FAIL] automatic mapping: " << filter->GetLastError() << '\n';
        return false;
    }

    const std::vector<std::array<double, 2>> expected{
            {0.0, 0.0}, {0.5, 0.1}, {1.0, 0.2},
            {0.0, 0.4}, {0.5, 0.5}, {1.0, 0.6},
            {0.0, 0.8}, {0.5, 0.9}, {1.0, 1.0}};
    if (!CheckCoordinates(filter->GetTextureCoordinates(), expected,
                          "automatic least-squares plane matches ParaView")) return false;
    std::cout << "[PASS] automatic mode ignores manual plane parameters\n";

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (output.IsNull() || output == inputMesh || output->GetNumberOfCells() != inputMesh->GetNumberOfCells() ||
        output->GetAttributeSet()->GetArrayPointer(
                IG_TCOORD, IG_POINT, iGame::TextureMapToPlaneFilter::TextureCoordinatesArrayName()) == nullptr ||
        inputMesh->GetAttributeSet()->GetArrayPointer(
                IG_TCOORD, IG_POINT, iGame::TextureMapToPlaneFilter::TextureCoordinatesArrayName()) != nullptr) {
        std::cerr << "[FAIL] independent output and point TCoords attachment\n";
        return false;
    }
    output->SetPoint(0, iGame::Point(99.0f, 99.0f, 99.0f));
    if ((inputMesh->GetPoint(0) - originalPoint).norm() > Tolerance) {
        std::cerr << "[FAIL] output geometry aliases the input\n";
        return false;
    }
    std::cout << "[PASS] output is independent and input remains unchanged\n";
    return true;
}

bool RunManualModelCase() {
    auto input = iGame::FileIO::ReadFile("./Models/TextureMapToPlaneManualPatch.vtk");
    if (input.IsNull()) {
        std::cerr << "[FAIL] read manual test model\n";
        return false;
    }
    auto filter = iGame::TextureMapToPlaneFilter::New();
    filter->SetInput(input);
    filter->SetAutomaticPlaneGeneration(false);
    filter->SetOrigin({0.0f, 0.0f, 0.0f});
    filter->SetPoint1({2.0f, 0.0f, 0.0f});
    filter->SetPoint2({1.0f, 4.0f, 0.0f});
    if (!filter->Execute()) {
        std::cerr << "[FAIL] manual mapping: " << filter->GetLastError() << '\n';
        return false;
    }

    std::vector<std::array<double, 2>> expected;
    for (double y : {0.0, 2.0, 4.0}) {
        for (double x : {0.0, 1.0, 2.0}) expected.push_back({x / 2.0, (x + 4.0 * y) / 17.0});
    }
    return CheckCoordinates(filter->GetTextureCoordinates(), expected,
                            "manual non-orthogonal axes match ParaView");
}

bool RunCurvedSaddleCase() {
    auto input = iGame::DynamicCast<iGame::UnstructuredMesh>(
            iGame::FileIO::ReadFile("./Models/TextureMapToPlaneCurvedSaddle.vtk"));
    if (input.IsNull() || input->GetNumberOfPoints() != 25 || input->GetNumberOfCells() != 16) {
        std::cerr << "[FAIL] read curved saddle test model\n";
        return false;
    }

    auto manual = iGame::TextureMapToPlaneFilter::New();
    manual->SetInput(input);
    manual->SetAutomaticPlaneGeneration(false);
    manual->SetOrigin({0.0f, 0.0f, 0.0f});
    manual->SetPoint1({4.0f, 0.0f, 0.0f});
    manual->SetPoint2({0.0f, 4.0f, 0.0f});
    if (!manual->Execute()) {
        std::cerr << "[FAIL] curved saddle manual mapping: " << manual->GetLastError() << '\n';
        return false;
    }
    std::vector<std::array<double, 2>> expected;
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            expected.push_back({x / 4.0, y / 4.0});
    if (!CheckCoordinates(manual->GetTextureCoordinates(), expected,
                          "curved saddle manual projection matches analytical XY mapping")) return false;

    auto automatic = iGame::TextureMapToPlaneFilter::New();
    automatic->SetInput(input);
    if (!automatic->Execute()) {
        std::cerr << "[FAIL] curved saddle automatic mapping: " << automatic->GetLastError() << '\n';
        return false;
    }
    auto coordinates = automatic->GetTextureCoordinates();
    if (coordinates.IsNull() || coordinates->GetNumberOfElements() != 25) {
        std::cerr << "[FAIL] curved saddle automatic mapping array\n";
        return false;
    }
    double minimum[2]{1.0, 1.0};
    double maximum[2]{0.0, 0.0};
    for (IGsize pointId = 0; pointId < coordinates->GetNumberOfElements(); ++pointId) {
        for (int component = 0; component < 2; ++component) {
            const double value = coordinates->RawPointer(pointId)[component];
            if (!std::isfinite(value)) {
                std::cerr << "[FAIL] curved saddle automatic mapping contains non-finite values\n";
                return false;
            }
            minimum[component] = std::min(minimum[component], value);
            maximum[component] = std::max(maximum[component], value);
        }
    }
    if (!AlmostEqual(minimum[0], 0.0) || !AlmostEqual(maximum[0], 1.0)
        || !AlmostEqual(minimum[1], 0.0) || !AlmostEqual(maximum[1], 1.0)) {
        std::cerr << "[FAIL] curved saddle automatic S/T ranges are not [0, 1]\n";
        return false;
    }
    std::cout << "[PASS] curved saddle automatic projection produces finite [0, 1] coordinates\n";
    return true;
}

bool RunAutomaticThreeDimensionalCase() {
    auto input = iGame::PointSet::New();
    for (const auto& point : std::array<iGame::Point, 6>{
                 iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(2.0f, 0.0f, 1.0f),
                 iGame::Point(0.0f, 3.0f, 2.0f), iGame::Point(2.0f, 3.0f, 5.0f),
                 iGame::Point(1.0f, 1.0f, 4.0f), iGame::Point(0.5f, 2.5f, 1.5f)}) {
        input->AddPoint(point);
    }

    auto filter = iGame::TextureMapToPlaneFilter::New();
    filter->SetInput(input);
    if (!filter->Execute()) {
        std::cerr << "[FAIL] automatic three-dimensional mapping: " << filter->GetLastError() << '\n';
        return false;
    }
    const std::vector<std::array<double, 2>> expected{
            {0.0, 0.0}, {-0.108147650957, 0.410003840923},
            {0.934773445129, 0.294998079538}, {1.0, 1.0},
            {0.503130793571, 0.721248567104}, {0.715821325779, 0.286874771118}};
    return CheckCoordinates(filter->GetTextureCoordinates(), expected,
                            "automatic three-dimensional mapping matches ParaView");
}

iGame::Points::Pointer MakePlanarPoints() {
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
    points->AddPoint(iGame::Point(1.0f, 1.0f, 0.0f));
    return points;
}

bool CheckSupportedType(const iGame::DataObject::Pointer& input, const char* name) {
    auto filter = iGame::TextureMapToPlaneFilter::New();
    filter->SetInput(input);
    if (!filter->Execute() || filter->GetOutput().IsNull() ||
        filter->GetOutput()->GetDataObjectType() != input->GetDataObjectType()) {
        std::cerr << "[FAIL] supported type " << name << "\n";
        return false;
    }
    std::cout << "[PASS] supported type " << name << '\n';
    return true;
}

bool RunSupportedTypeCases() {
    auto pointSet = iGame::PointSet::New();
    pointSet->SetPoints(MakePlanarPoints());

    auto surface = iGame::SurfaceMesh::New();
    surface->SetPoints(MakePlanarPoints());
    auto faces = iGame::CellArray::New();
    const igIndex face[4]{0, 1, 3, 2};
    faces->AddCellIds(face, 4);
    surface->SetFaces(faces);

    auto volume = iGame::VolumeMesh::New();
    auto volumePoints = iGame::Points::New();
    for (int z = 0; z < 2; ++z)
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x)
                volumePoints->AddPoint(iGame::Point(static_cast<float>(x), static_cast<float>(y),
                                                    static_cast<float>(z)));
    volume->SetPoints(volumePoints);
    auto volumes = iGame::CellArray::New();
    const igIndex hexahedron[8]{0, 1, 3, 2, 4, 5, 7, 6};
    volumes->AddCellIds(hexahedron, 8);
    volume->SetVolumes(volumes);

    auto structured = iGame::StructuredMesh::New();
    structured->SetPoints(MakePlanarPoints());
    igIndex dimensions[3]{2, 2, 1};
    structured->SetDimensionSize(dimensions);

    return CheckSupportedType(pointSet, "PointSet") &&
           CheckSupportedType(surface, "SurfaceMesh") &&
           CheckSupportedType(volume, "VolumeMesh") &&
           CheckSupportedType(structured, "StructuredMesh");
}

bool RunInvalidInputCases() {
    auto line = iGame::PointSet::New();
    line->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    line->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    line->AddPoint(iGame::Point(2.0f, 0.0f, 0.0f));
    auto automatic = iGame::TextureMapToPlaneFilter::New();
    automatic->SetInput(line);
    if (automatic->Execute() || automatic->GetLastError().empty()) {
        std::cerr << "[FAIL] degenerate automatic input must be rejected\n";
        return false;
    }

    auto manual = iGame::TextureMapToPlaneFilter::New();
    manual->SetInput(line);
    manual->SetAutomaticPlaneGeneration(false);
    manual->SetOrigin({0.0f, 0.0f, 0.0f});
    manual->SetPoint1({1.0f, 0.0f, 0.0f});
    manual->SetPoint2({2.0f, 0.0f, 0.0f});
    if (manual->Execute() || manual->GetLastError().empty()) {
        std::cerr << "[FAIL] collinear manual axes must be rejected\n";
        return false;
    }
    std::cout << "[PASS] invalid inputs are rejected with an error message\n";
    return true;
}

} // namespace

int main() {
    const bool passed = RunAutomaticModelCase() && RunManualModelCase() && RunCurvedSaddleCase() &&
                        RunAutomaticThreeDimensionalCase() && RunSupportedTypeCases() &&
                        RunInvalidInputCases();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
