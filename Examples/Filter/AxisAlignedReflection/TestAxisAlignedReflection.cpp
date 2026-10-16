// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/AxisAlignedReflection/TestAxisAlignedReflection.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <AxisAlignedReflection/iGameAxisAlignedReflectionFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

bool NearlyEqual(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1.0e-5;
}

iGame::UnstructuredMesh::Pointer CreateTetraMesh() {
    using namespace iGame;

    auto mesh = UnstructuredMesh::New();
    auto points = mesh->GetPoints();
    points->AddPoint(Point(0.0f, 0.0f, 0.0f));
    points->AddPoint(Point(1.0f, 0.0f, 0.0f));
    points->AddPoint(Point(0.0f, 1.0f, 0.0f));
    points->AddPoint(Point(0.0f, 0.0f, 1.0f));

    igIndex ids[4]{0, 1, 2, 3};
    mesh->AddCell(ids, 4, IG_TETRA);

    auto attributes = AttributeSet::New();

    auto velocity = FloatArray::New();
    velocity->SetName("Velocity");
    velocity->SetDimension(3);
    velocity->Resize(4);
    for (IGsize i = 0; i < 4; ++i) {
        float tuple[3]{1.0f, 2.0f, 3.0f};
        velocity->SetElement(i, tuple);
    }
    attributes->AddAttribute(IG_VECTOR, IG_POINT, velocity);

    auto tripleScalar = DoubleArray::New();
    tripleScalar->SetName("TripleScalar");
    tripleScalar->SetDimension(3);
    tripleScalar->Resize(4);
    for (IGsize i = 0; i < 4; ++i) {
        double tuple[3]{4.0, 5.0, 6.0};
        tripleScalar->SetElement(i, tuple);
    }
    attributes->AddAttribute(IG_SCALAR, IG_POINT, tripleScalar);

    auto cellValue = IntArray::New();
    cellValue->SetName("CellValue");
    cellValue->SetDimension(1);
    cellValue->Resize(1);
    cellValue->SetValue(0, 7);
    attributes->AddAttribute(IG_SCALAR, IG_CELL, cellValue);

    mesh->SetAttributeSet(attributes);
    return mesh;
}

bool TestCopyInputAndConnectivity() {
    using namespace iGame;

    auto input = CreateTetraMesh();
    auto filter = AxisAlignedReflectionFilter::New();
    filter->SetInput(input);
    filter->SetPlane(AxisAlignedReflectionFilter::Plane::X);
    filter->SetCenter(0.0);

    if (!Check(filter->Execute(), "default CopyInput reflection executes")) return false;

    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!Check(output != nullptr, "output is UnstructuredMesh")) return false;
    if (!Check(output->GetNumberOfPoints() == 8, "CopyInput doubles point count")) return false;
    if (!Check(output->GetNumberOfCells() == 2, "CopyInput doubles cell count")) return false;

    if (!Check(NearlyEqual(output->GetPoint(1)[0], 1.0), "original point is preserved")) return false;
    if (!Check(NearlyEqual(output->GetPoint(5)[0], -1.0), "reflected point is appended")) return false;

    const igIndex* originalCell = nullptr;
    const igIndex* reflectedCell = nullptr;
    output->GetCells()->GetCellIds(0, originalCell);
    output->GetCells()->GetCellIds(1, reflectedCell);

    if (!Check(originalCell[0] == 0 && originalCell[1] == 1 &&
               originalCell[2] == 2 && originalCell[3] == 3,
               "original tetra connectivity is preserved")) return false;

    if (!Check(reflectedCell[0] == 7 && reflectedCell[1] == 5 &&
               reflectedCell[2] == 6 && reflectedCell[3] == 4,
               "reflected tetra connectivity is reoriented and offset")) return false;

    return Check(NearlyEqual(input->GetPoint(1)[0], 1.0),
                 "input geometry remains unchanged");
}

bool TestFlipAllInputArrays() {
    using namespace iGame;

    auto input = CreateTetraMesh();
    auto filter = AxisAlignedReflectionFilter::New();
    filter->SetInput(input);
    filter->SetPlane(AxisAlignedReflectionFilter::Plane::X);
    filter->SetCenter(0.0);

    if (!Check(filter->Execute(), "FlipAllInputArrays=true executes")) return false;

    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    auto attrs = output->GetAttributeSet();
    auto velocity = DynamicCast<FloatArray>(attrs->GetAttribute("Velocity").pointer);
    auto triple = DynamicCast<DoubleArray>(attrs->GetAttribute("TripleScalar").pointer);
    auto cellValue = DynamicCast<IntArray>(attrs->GetAttribute("CellValue").pointer);

    if (!Check(velocity && velocity->GetNumberOfElements() == 8,
               "point vector array doubles")) return false;
    if (!Check(triple && triple->GetNumberOfElements() == 8,
               "3-component scalar array doubles")) return false;
    if (!Check(cellValue && cellValue->GetNumberOfElements() == 2,
               "cell array doubles")) return false;

    float vectorTuple[3]{};
    velocity->GetElement(4, vectorTuple);
    if (!Check(NearlyEqual(vectorTuple[0], -1.0) &&
               NearlyEqual(vectorTuple[1], 2.0) &&
               NearlyEqual(vectorTuple[2], 3.0),
               "vector tuple is reflected")) return false;

    double scalarTuple[3]{};
    triple->GetElement(4, scalarTuple);
    if (!Check(NearlyEqual(scalarTuple[0], -4.0) &&
               NearlyEqual(scalarTuple[1], 5.0) &&
               NearlyEqual(scalarTuple[2], 6.0),
               "signed 3-component scalar is reflected when FlipAllInputArrays=true")) return false;

    auto filterNoFlipAll = AxisAlignedReflectionFilter::New();
    filterNoFlipAll->SetInput(input);
    filterNoFlipAll->SetPlane(AxisAlignedReflectionFilter::Plane::X);
    filterNoFlipAll->SetCenter(0.0);
    filterNoFlipAll->SetFlipAllInputArrays(false);

    if (!Check(filterNoFlipAll->Execute(), "FlipAllInputArrays=false executes")) return false;

    auto outputNoFlipAll = DynamicCast<UnstructuredMesh>(filterNoFlipAll->GetOutput());
    auto attrsNoFlipAll = outputNoFlipAll->GetAttributeSet();
    auto vectorNoFlipAll = DynamicCast<FloatArray>(attrsNoFlipAll->GetAttribute("Velocity").pointer);
    auto scalarNoFlipAll = DynamicCast<DoubleArray>(attrsNoFlipAll->GetAttribute("TripleScalar").pointer);

    vectorNoFlipAll->GetElement(4, vectorTuple);
    scalarNoFlipAll->GetElement(4, scalarTuple);

    if (!Check(NearlyEqual(vectorTuple[0], -1.0),
               "vector still reflects when FlipAllInputArrays=false")) return false;

    return Check(NearlyEqual(scalarTuple[0], 4.0),
                 "ordinary 3-component scalar is copied when FlipAllInputArrays=false");
}

bool TestCopyInputOff() {
    using namespace iGame;

    auto input = CreateTetraMesh();
    auto filter = AxisAlignedReflectionFilter::New();
    filter->SetInput(input);
    filter->SetPlane(AxisAlignedReflectionFilter::Plane::XMax);
    filter->SetCopyInput(false);

    if (!Check(filter->Execute(), "CopyInput=false executes")) return false;

    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!Check(output->GetNumberOfPoints() == 4, "CopyInput=false keeps point count")) return false;
    if (!Check(output->GetNumberOfCells() == 1, "CopyInput=false keeps cell count")) return false;

    if (!Check(NearlyEqual(output->GetPoint(0)[0], 2.0),
               "XMax uses input bounding-box maximum")) return false;

    const igIndex* reflectedCell = nullptr;
    output->GetCells()->GetCellIds(0, reflectedCell);
    return Check(reflectedCell[0] == 3 && reflectedCell[1] == 1 &&
                 reflectedCell[2] == 2 && reflectedCell[3] == 0,
                 "CopyInput=false reflected tetra uses original point ids");
}

bool TestRealModel() {
    using namespace iGame;

    auto object = FileIO::ReadFile("./Models/ClipTest_Plane_UnstructuredGrid.vtk");
    auto input = DynamicCast<UnstructuredMesh>(object);
    if (!Check(input != nullptr, "real model loads as UnstructuredMesh")) return false;

    const IGsize inputPoints = input->GetNumberOfPoints();
    const IGsize inputCells = input->GetNumberOfCells();
    if (!Check(inputPoints == 8604, "real model point count is 8604")) return false;
    if (!Check(inputCells == 30700, "real model cell count is 30700")) return false;

    auto filter = AxisAlignedReflectionFilter::New();
    filter->SetInput(input);

    if (!Check(filter->Execute(), "real model default reflection executes")) return false;

    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!Check(output != nullptr, "real model output is UnstructuredMesh")) return false;
    if (!Check(output->GetNumberOfPoints() == inputPoints * 2,
               "real model point count doubles")) return false;
    if (!Check(output->GetNumberOfCells() == inputCells * 2,
               "real model cell count doubles")) return false;

    const auto& bounds = input->GetBoundingBox();
    const auto& originalPoint = input->GetPoint(0);
    const auto& reflectedPoint = output->GetPoint(inputPoints);
    if (!Check(NearlyEqual(reflectedPoint[0], 2.0 * bounds.min[0] - originalPoint[0]),
               "real model default XMin reflection is correct")) return false;

    auto attributes = output->GetAttributeSet();
    if (attributes) {
        auto all = attributes->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& attr = all->GetElement(i);
            if (attr.IsNone()) continue;
            if (attr.attachmentType == IG_POINT &&
                !Check(attr.pointer->GetNumberOfElements() == output->GetNumberOfPoints(),
                       "real point attribute length matches output points")) {
                return false;
            }
            if (attr.attachmentType == IG_CELL &&
                !Check(attr.pointer->GetNumberOfElements() == output->GetNumberOfCells(),
                       "real cell attribute length matches output cells")) {
                return false;
            }
        }
    }

    return true;
}

bool TestMixedCellSizesWithCopyInput() {
    using namespace iGame;

    auto input = UnstructuredMesh::New();
    input->AddPoint(Point(0.0f, 0.0f, 0.0f));
    input->AddPoint(Point(1.0f, 0.0f, 0.0f));
    input->AddPoint(Point(1.0f, 1.0f, 0.0f));
    input->AddPoint(Point(0.0f, 1.0f, 0.0f));
    input->AddPoint(Point(2.0f, 0.0f, 0.0f));

    igIndex quad[4]{0, 1, 2, 3};
    input->AddCell(quad, 4, IG_QUAD);
    igIndex triangle[3]{1, 4, 2};
    input->AddCell(triangle, 3, IG_TRIANGLE);

    auto filter = AxisAlignedReflectionFilter::New();
    filter->SetInput(input);
    filter->SetPlane(AxisAlignedReflectionFilter::Plane::XMax);

    if (!Check(filter->Execute(), "mixed cell sizes execute")) return false;

    auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    if (!Check(output != nullptr, "mixed cell sizes output exists")) return false;
    if (!Check(output->GetNumberOfPoints() == 10, "mixed cell sizes double points")) return false;
    if (!Check(output->GetNumberOfCells() == 4, "mixed cell sizes double cells")) return false;

    const igIndex* ids = nullptr;
    output->GetCells()->GetCellIds(0, ids);
    if (!Check(ids[0] == 0 && ids[1] == 1 && ids[2] == 2 && ids[3] == 3,
               "mixed original quad is preserved")) return false;

    output->GetCells()->GetCellIds(1, ids);
    if (!Check(ids[0] == 1 && ids[1] == 4 && ids[2] == 2,
               "mixed original triangle is preserved")) return false;

    output->GetCells()->GetCellIds(2, ids);
    if (!Check(ids[0] == 5 && ids[1] == 8 && ids[2] == 7 && ids[3] == 6,
               "mixed reflected quad is correct")) return false;

    output->GetCells()->GetCellIds(3, ids);
    return Check(ids[0] == 6 && ids[1] == 7 && ids[2] == 9,
                 "mixed reflected triangle is correct");
}

// 运行测试后打开渲染窗口，显示反射结果
bool ShowReflectedResult() {
    using namespace iGame;

    auto object = FileIO::ReadFile("./Models/Quad_Bicycle.vtk");
    auto input = DynamicCast<UnstructuredMesh>(object);
    if (!input) {
        std::cerr << "Failed to load ./Models/Quad_Bicycle.vtk for rendering.\n";
        return false;
    }

    auto filter = AxisAlignedReflectionFilter::New();
    filter->SetInput(input);
    // 使用 x=1 镜面，并保留原始模型，便于观察反射结果
    filter->SetPlane(AxisAlignedReflectionFilter::Plane::X);
    filter->SetCenter(1.0);
    filter->SetCopyInput(true);

    if (!filter->Execute()) {
        std::cerr << "AxisAlignedReflectionFilter execute failed for rendering.\n";
        return false;
    }

    auto output = DynamicCast<DrawObject>(filter->GetOutput());
    if (!output) {
        std::cerr << "Reflection output is not drawable.\n";
        return false;
    }
    output->SetViewStyle(IG_SURFACE);

    auto scene = Scene::New();
    scene->AddModel(output);

    RenderWindow::Pointer window = RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
    return true;
}

} // namespace

int main(int argc, char** argv) {
    const bool passed =
        TestCopyInputAndConnectivity() &&
        TestFlipAllInputArrays() &&
        TestCopyInputOff() &&
        TestMixedCellSizesWithCopyInput() &&
        TestRealModel();

    if (!passed) return 1;

    std::cout << "AxisAlignedReflection backend tests passed.\n";

    if (!(argc > 1 && std::string(argv[1]) == "--no-render") && !ShowReflectedResult()) {
        return 1;
    }

    return 0;
}
