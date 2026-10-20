// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
#include <Smooth/iGameSmoothFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

bool Check(bool condition, const std::string& message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

bool NearlyEqual(double lhs, double rhs) {
    return std::abs(lhs - rhs) <= 1.0e-6 * std::max(1.0, std::abs(rhs));
}

bool CheckCells(iGame::CellArray* lhs, iGame::CellArray* rhs) {
    if (!Check(lhs && rhs && lhs != rhs && lhs->GetNumberOfCells() == rhs->GetNumberOfCells(),
               "cell arrays are independent and have the same size")) return false;
    for (IGsize i = 0; i < lhs->GetNumberOfCells(); ++i) {
        const igIndex* a = nullptr;
        const igIndex* b = nullptr;
        const int count = lhs->GetCellIds(i, a);
        if (!Check(count == rhs->GetCellIds(i, b) && std::equal(a, a + count, b),
                   "cell connectivity is preserved")) return false;
    }
    return true;
}

bool CheckAttributes(iGame::AttributeSet* lhs, iGame::AttributeSet* rhs) {
    if (!Check(lhs && rhs && lhs != rhs && lhs->GetNumberOfAttributes() == rhs->GetNumberOfAttributes(),
               "attribute sets are independent and have the same size")) return false;
    for (IGsize i = 0; i < lhs->GetNumberOfAttributes(); ++i) {
        const auto& a = lhs->GetAttribute(i);
        const auto& b = rhs->GetAttribute(i);
        if (!Check(a.type == b.type && a.attachmentType == b.attachmentType && a.isDeleted == b.isDeleted,
                   "attribute metadata is preserved")) return false;
        if (!a.pointer || !b.pointer) {
            if (!Check(!a.pointer && !b.pointer, "empty attributes are preserved")) return false;
            continue;
        }
        if (!Check(a.pointer != b.pointer && a.pointer->GetName() == b.pointer->GetName() &&
                   a.pointer->GetArrayType() == b.pointer->GetArrayType() &&
                   a.pointer->GetDimension() == b.pointer->GetDimension() &&
                   a.pointer->GetNumberOfValues() == b.pointer->GetNumberOfValues(),
                   "attribute arrays are copied independently")) return false;
        for (IGsize j = 0; j < a.pointer->GetNumberOfValues(); ++j) {
            const double x = a.pointer->GetValue(j);
            const double y = b.pointer->GetValue(j);
            if (!Check(x == y || (std::isnan(x) && std::isnan(y)), "attribute values are preserved")) return false;
        }
    }
    return true;
}

iGame::SurfaceMesh::Pointer CreateFan() {
    using namespace iGame;
    auto mesh = SurfaceMesh::New();
    mesh->AddPoint(Point(0.0f, 0.0f, 0.0f));
    mesh->AddPoint(Point(1.0f, 0.0f, 0.0f));
    mesh->AddPoint(Point(1.0f, 1.0f, 0.0f));
    mesh->AddPoint(Point(0.0f, 1.0f, 0.0f));
    mesh->AddPoint(Point(0.5f, 0.5f, 1.0f));
    mesh->AddPoint(Point(2.0f, 2.0f, 2.0f));
    auto faces = CellArray::New();
    for (igIndex i = 0; i < 4; ++i) faces->AddCellId3(i, (i + 1) % 4, 4);
    mesh->SetFaces(faces);
    return mesh;
}

bool TestBoundaryAndIterations() {
    using namespace iGame;
    auto input = CreateFan();
    auto filter = SmoothFilter::New();
    filter->SetInput(input);
    filter->SetNumberOfIterations(1);
    filter->SetRelaxationFactor(0.5);
    filter->SetPreserveBoundary(true);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!Check(output != nullptr, "surface input produces a surface output")) return false;
    for (IGsize i = 0; i < 4; ++i) {
        if (!Check(output->GetPoint(i) == input->GetPoint(i), "boundary points are fixed")) return false;
    }
    if (!Check(NearlyEqual(output->GetPoint(4)[2], 0.5), "interior point moves towards neighbors")) return false;
    if (!Check(output->GetPoint(5) == input->GetPoint(5), "isolated point is fixed")) return false;
    if (!Check(output != input && output->GetPoints() != input->GetPoints(), "geometry is independent")) return false;
    if (!CheckCells(input->GetFaces(), output->GetFaces())) return false;

    filter->SetNumberOfIterations(2);
    auto previousOutput = output;
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!Check(NearlyEqual(output->GetPoint(4)[2], 0.25), "two iterations use the previous result")) return false;
    if (!Check(output != previousOutput && output->GetPoints() != previousOutput->GetPoints() &&
               NearlyEqual(previousOutput->GetPoint(4)[2], 0.5), "repeated execution preserves the previous output")) return false;

    filter->SetNumberOfIterations(1);
    filter->SetRelaxationFactor(0.25);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!Check(NearlyEqual(output->GetPoint(4)[2], 0.75), "changed parameters apply to the original input")) return false;

    filter->SetRelaxationFactor(0.5);
    filter->SetPreserveBoundary(false);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    const auto& corner = output->GetPoint(0);
    if (!Check(corner == input->GetPoint(0), "sharp boundary corners remain fixed")) return false;
    if (!Check(NearlyEqual(output->GetPoint(4)[2], 0.5), "interior points still smooth")) return false;

    filter->SetNumberOfIterations(0);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    for (IGsize i = 0; i < input->GetNumberOfPoints(); ++i) {
        if (!Check(input->GetPoint(i) == output->GetPoint(i), "zero iterations preserve positions")) return false;
    }
    filter->SetNumberOfIterations(20);
    filter->SetRelaxationFactor(0.0);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    return Check(output->GetPoint(4) == Point(0.5f, 0.5f, 1.0f) && input->GetPoint(4) == Point(0.5f, 0.5f, 1.0f),
                 "zero factor and repeated execution do not change the input");
}

bool TestBoundaryMotionAndConvergence() {
    using namespace iGame;
    auto input = SurfaceMesh::New();
    for (const Point& point : {Point(0, 0, 0), Point(0.25f, 0, 0), Point(1, 0, 0), Point(1, 1, 0), Point(0, 1, 0)}) {
        input->AddPoint(point);
    }
    auto faces = CellArray::New();
    igIndex face[5]{0, 1, 2, 3, 4};
    faces->AddCellIds(face, 5);
    input->SetFaces(faces);
    auto filter = SmoothFilter::New();
    if (!Check(filter->GetNumberOfIterations() == 20 && filter->GetRelaxationFactor() == 0.01 &&
               filter->GetConvergence() == 0.0 && !filter->GetPreserveBoundary(), "defaults match ParaView")) return false;
    filter->SetInput(input);
    filter->SetNumberOfIterations(1);
    filter->SetRelaxationFactor(0.5);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!Check(NearlyEqual(output->GetPoint(1)[0], 0.375) && output->GetPoint(0) == input->GetPoint(0),
               "boundary smoothing follows its two edges and fixes corners")) return false;
    filter->SetPreserveBoundary(true);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!Check(output->GetPoint(1) == input->GetPoint(1), "preserve boundary disables boundary motion")) return false;

    filter->SetInput(CreateFan());
    filter->SetNumberOfIterations(10);
    filter->SetConvergence(0.2);
    if (!Check(filter->Execute() && filter->GetNumberOfIterationsPerformed() == 1,
               "convergence stops when displacement is small relative to the input bounds")) return false;
    filter->SetConvergence(0.0);
    filter->SetNumberOfIterations(3);
    return Check(filter->Execute() && filter->GetNumberOfIterationsPerformed() == 3,
                 "zero convergence disables early termination");
}

// 回归：混合面单元的整数属性曾丢失，检查独立复制和精度。
// 修复提交主题：merge: 同步 main 并完善 Smooth 接入与属性复制
bool TestMixedCellsAndAttributes() {
    using namespace iGame;
    auto fan = CreateFan();
    auto input = UnstructuredMesh::New();
    input->SetPoints(fan->GetPoints());
    igIndex quad[4]{0, 1, 2, 3};
    igIndex triangle[3]{1, 4, 2};
    input->AddCell(quad, 4, IG_QUAD);
    input->AddCell(triangle, 3, IG_TRIANGLE);
    auto pointValues = DoubleArray::New();
    pointValues->SetName("PointValues");
    for (IGsize i = 0; i < input->GetNumberOfPoints(); ++i) pointValues->AddValue(i + 0.25);
    input->GetAttributeSet()->AddScalar(IG_POINT, pointValues);
    auto cellValues = IntArray::New();
    cellValues->SetName("CellValues");
    cellValues->AddValue(7);
    cellValues->AddValue(9);
    input->GetAttributeSet()->AddScalar(IG_CELL, cellValues);

    auto largeIds = LongLongArray::New();
    largeIds->SetName("LargeIds");
    largeIds->AddValue(9007199254740993LL);
    largeIds->AddValue(-9007199254740993LL);
    input->GetAttributeSet()->AddScalar(IG_CELL, largeIds);
    auto range = DoubleArray::New();
    range->SetDimension(2);
    range->AddElement2(7.0, 9.0);
    range->AddElement2(7.0, 9.0);
    auto& cellAttribute = input->GetAttributeSet()->GetAttribute(1);
    cellAttribute.dataRange = range;
    cellAttribute.rangeLocked = true;
    cellAttribute.rangeMode = AttributeSet::RangeMode::FixedGlobal;
    cellAttribute.rangeLockedDimension = 0;
    cellAttribute.runningMin = 7.0;
    cellAttribute.runningMax = 9.0;
    cellAttribute.runningRangeValid = true;

    auto filter = SmoothFilter::New();
    filter->SetInput(input);
    filter->SetPreserveBoundary(false);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!Check(output && output != input, "unstructured output retains the mesh type")) return false;
    if (!CheckCells(input->GetCells(), output->GetCells())) return false;
    if (!Check(output->GetCellTypes() != input->GetCellTypes() && output->GetCellType(0) == IG_QUAD &&
               output->GetCellType(1) == IG_TRIANGLE, "mixed cell types are preserved")) return false;
    if (!CheckAttributes(input->GetAttributeSet(), output->GetAttributeSet())) return false;
    auto copiedIds = DynamicCast<LongLongArray>(output->GetAttributeSet()->GetAttribute(2).pointer);
    if (!Check(copiedIds && copiedIds->RawPointer()[0] == largeIds->RawPointer()[0] &&
               copiedIds->RawPointer()[1] == largeIds->RawPointer()[1],
               "64-bit integer values preserve their exact precision")) return false;
    const auto& copiedCell = output->GetAttributeSet()->GetAttribute(1);
    if (!Check(!output->GetAttributeSet()->GetAttribute(0).dataRange &&
               copiedCell.dataRange && copiedCell.dataRange != range &&
               copiedCell.dataRange->GetValue(0) == 7.0 && copiedCell.dataRange->GetValue(1) == 9.0 &&
               copiedCell.rangeLocked && copiedCell.rangeMode == cellAttribute.rangeMode &&
               copiedCell.rangeLockedDimension == 0 && copiedCell.runningMin == 7.0 &&
               copiedCell.runningMax == 9.0 && copiedCell.runningRangeValid,
               "lazy ranges and cached range states are preserved independently")) return false;
    copiedIds->RawPointer()[0] = 0;
    copiedCell.dataRange->SetValue(0, -7.0);
    output->GetAttributeSet()->GetAttribute(0).pointer->SetValue(0, -1.0);
    output->SetPoint(0, Point(-1.0f, -1.0f, -1.0f));
    return Check(pointValues->GetValue(0) == 0.25 && largeIds->RawPointer()[0] == 9007199254740993LL &&
                 range->GetValue(0) == 7.0 && input->GetPoint(0) == Point(0.0f, 0.0f, 0.0f),
                 "editing the output does not affect the input");
}

bool TestHighValence() {
    using namespace iGame;
    auto input = SurfaceMesh::New();
    input->AddPoint(Point(0.0f, 0.0f, 1.0f));
    constexpr int count = 300;
    auto faces = CellArray::New();
    for (int i = 0; i < count; ++i) {
        const double angle = 6.283185307179586 * i / count;
        input->AddPoint(Point(static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)), 0.0f));
        faces->AddCellId3(0, i + 1, (i + 1) % count + 1);
    }
    input->SetFaces(faces);
    auto filter = SmoothFilter::New();
    filter->SetInput(input);
    filter->SetNumberOfIterations(1);
    filter->SetRelaxationFactor(0.5);
    return Check(!filter->Execute() && !filter->GetOutput() && !filter->GetMessage().empty(),
                 "more than 255 incident edges are rejected safely");
}

bool TestClosedSurfaceAndLimits() {
    using namespace iGame;
    auto input = SurfaceMesh::New();
    input->AddPoint(Point(0.0f, 0.0f, 0.0f));
    input->AddPoint(Point(1.0f, 0.0f, 0.0f));
    input->AddPoint(Point(0.0f, 1.0f, 0.0f));
    input->AddPoint(Point(0.0f, 0.0f, 1.0f));
    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(0, 3, 1);
    faces->AddCellId3(0, 2, 3);
    faces->AddCellId3(1, 3, 2);
    input->SetFaces(faces);
    auto filter = SmoothFilter::New();
    filter->SetInput(input);
    filter->SetNumberOfIterations(1);
    filter->SetRelaxationFactor(0.5);
    filter->SetPreserveBoundary(true);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    for (int axis = 0; axis < 3; ++axis) {
        if (!Check(NearlyEqual(output->GetPoint(0)[axis], 1.0 / 6.0),
                   "closed surfaces still smooth with boundary preservation")) return false;
    }
    const Point expected(19.0f / 36.0f, 7.0f / 36.0f, 7.0f / 36.0f);
    for (int axis = 0; axis < 3; ++axis) {
        if (!Check(NearlyEqual(output->GetPoint(1)[axis], expected[axis]), "ordered update matches VTK's one-pass reference")) return false;
    }

    faces = CellArray::New();
    for (int i = 0; i < 256; ++i) faces->AddCellId3(0, 1, 2);
    input->SetFaces(faces);
    if (!Check(!filter->Execute() && !filter->GetOutput(), "edge-face incidence limit is checked")) return false;

    auto polygon = SurfaceMesh::New();
    std::vector<igIndex> ids(33);
    for (int i = 0; i < 33; ++i) {
        const double angle = 6.283185307179586 * i / 33;
        polygon->AddPoint(Point(static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)), 0.0f));
        ids[i] = i;
    }
    faces = CellArray::New();
    faces->AddCellIds(ids.data(), 33);
    polygon->SetFaces(faces);
    filter->SetInput(polygon);
    return Check(!filter->Execute() && !filter->GetOutput(), "oversized faces are rejected before building adjacency");
}

bool TestInvalidInput() {
    using namespace iGame;
    auto filter = SmoothFilter::New();
    if (!Check(!filter->Execute() && !filter->GetOutput(), "missing input fails")) return false;
    filter->SetInput(SurfaceMesh::New());
    if (!Check(!filter->Execute(), "empty surface fails")) return false;
    auto input = CreateFan();
    filter->SetInput(input);
    if (!Check(filter->Execute(), filter->GetMessage())) return false;
    filter->SetNumberOfIterations(-1);
    if (!Check(!filter->Execute() && !filter->GetOutput(), "failure clears previous output")) return false;
    filter->SetNumberOfIterations(1);
    for (double factor : {-0.1, 1.1, std::numeric_limits<double>::quiet_NaN()}) {
        filter->SetRelaxationFactor(factor);
        if (!Check(!filter->Execute() && !filter->GetOutput(), "invalid factor fails")) return false;
    }
    filter->SetRelaxationFactor(0.1);
    for (double convergence : {-0.1, 1.1, std::numeric_limits<double>::quiet_NaN()}) {
        filter->SetConvergence(convergence);
        if (!Check(!filter->Execute() && !filter->GetOutput(), "invalid convergence fails")) return false;
    }
    filter->SetConvergence(0.0);
    input->SetPoint(0, Point(std::numeric_limits<float>::infinity(), 0.0f, 0.0f));
    if (!Check(!filter->Execute(), "non-finite position fails")) return false;
    input = CreateFan();
    igIndex invalid[3]{0, 1, 100};
    input->GetFaces()->SetCellIds(0, invalid, 3);
    filter->SetInput(input);
    if (!Check(!filter->Execute(), "out-of-range point index fails")) return false;
    auto volume = UnstructuredMesh::New();
    volume->SetPoints(CreateFan()->GetPoints());
    igIndex tetra[4]{0, 1, 2, 4};
    volume->AddCell(tetra, 4, IG_TETRA);
    filter->SetInput(volume);
    return Check(!filter->Execute() && !filter->GetOutput(), "volume cells are rejected");
}

}

int main(int argc, char* argv[]) {
    using namespace iGame;
    bool checkOnly = false;
    int iterations = 20;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--check") {
            checkOnly = true;
        } else if (argument == "--iterations" && i + 1 < argc) {
            char* end = nullptr;
            const long value = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || value < 1 || value > 1000) {
                std::cerr << "Iterations must be an integer from 1 to 1000.\n";
                return 1;
            }
            iterations = static_cast<int>(value);
        } else {
            std::cerr << "Usage: testSmooth [--check] [--iterations 1..1000]\n";
            return 1;
        }
    }
    if (!TestBoundaryAndIterations() || !TestMixedCellsAndAttributes() || !TestHighValence() ||
        !TestClosedSurfaceAndLimits() || !TestBoundaryMotionAndConvergence() || !TestInvalidInput()) return 1;

    const std::string modelPath = "Models/mazewheel.obj";
    auto input = DynamicCast<SurfaceMesh>(FileIO::ReadFile(modelPath));
    if (!Check(input && input->GetNumberOfPoints() == 13607 && input->GetNumberOfFaces() == 27274,
               "load mazewheel.obj with 13607 points and 27274 faces")) return 1;
    std::vector<Point> original(input->GetNumberOfPoints());
    for (IGsize i = 0; i < original.size(); ++i) original[i] = input->GetPoint(i);
    auto inputAttributes = AttributeSet::New();
    inputAttributes->DeepCopy(input->GetAttributeSet());
    const auto pointTime = input->GetPoints()->GetMTime();

    auto filter = SmoothFilter::New();
    filter->SetInput(input);
    filter->SetNumberOfIterations(iterations);
    if (!Check(filter->Execute(), filter->GetMessage())) return 1;
    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!Check(output && output != input && output->GetPoints() != input->GetPoints() &&
               output->GetNumberOfPoints() == original.size(), "output owns independent points")) return 1;
    if (!CheckCells(input->GetFaces(), output->GetFaces())) return 1;
    if (!CheckAttributes(input->GetAttributeSet(), output->GetAttributeSet()) ||
        !CheckAttributes(inputAttributes, input->GetAttributeSet())) return 1;
    // 只在独立结果上构建边界查询
    output->BuildEdges();
    output->BuildEdgeLinks();
    output->BuildFaceEdgeLinks();
    IGsize moved = 0;
    IGsize boundary = 0;
    for (IGsize i = 0; i < original.size(); ++i) {
        if (!Check(input->GetPoint(i) == original[i], "input positions are unchanged")) return 1;
        const auto& point = output->GetPoint(i);
        for (int axis = 0; axis < 3; ++axis) {
            if (!Check(std::isfinite(point[axis]), "smoothed coordinates are finite")) return 1;
        }
        if (output->IsBoundaryPoint(i)) {
            ++boundary;
            if (filter->GetPreserveBoundary() && !Check(point == original[i], "boundary positions are unchanged")) return 1;
        }
        if (point != original[i]) ++moved;
    }
    if (!Check(input->GetPoints()->GetMTime() == pointTime && moved > 0, "input points are untouched and smoothing moves vertices")) return 1;
    std::cout << "Smooth checks passed: " << modelPath << ", " << moved << " moved vertices, " << boundary
              << " boundary vertices, " << filter->GetNumberOfIterationsPerformed() << " iterations, relaxation "
              << filter->GetRelaxationFactor() << ", convergence " << filter->GetConvergence() << ".\n";
    if (checkOnly) return 0;

    // 白色线框显示原模型，蓝色表面显示平滑结果
    input->SetName("Original_Mazewheel");
    input->SetViewStyle(IG_WIREFRAME);
    input->SetLineColor(igm::vec3(1.0f, 1.0f, 1.0f));
    output->SetDefaultColor(igm::vec3(0.3f, 0.6f, 0.9f));
    auto scene = Scene::New();
    scene->AddModel(input);
    scene->AddModel(output);
    auto window = RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}
