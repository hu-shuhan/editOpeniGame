// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/PointLineInterpolator/TestPointLineInterpolator.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <PointLineInterpolator/iGamePointLineInterpolatorFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {
constexpr const char* PointLineInterpolatorModelPath = "./Models/PointLineInterpolatorFilter_Test.vtk";

bool Near(double lhs, double rhs, double tolerance = 1e-5) { return std::abs(lhs - rhs) <= tolerance; }

bool Check(bool condition, const std::string& message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

iGame::PointSet::Pointer CreateSource() {
    std::cout << "Loading model: " << PointLineInterpolatorModelPath << '\n';
    auto dataObject = iGame::FileIO::ReadFile(PointLineInterpolatorModelPath);
    return iGame::DynamicCast<iGame::PointSet>(dataObject);
}

bool TestParameterizedLineAndVoronoi() {
    std::cout << "Running parameterized-line and Voronoi tests..." << std::endl;
    auto source = CreateSource();
    if (!Check(source != nullptr, "the PointLineInterpolator test model must load automatically")) { return false; }
    auto filter = iGame::PointLineInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetPoint1(iGame::Point(0.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(2.0, 0.0, 0.0));
    filter->SetResolution(4);
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::VORONOI);
    if (!Check(filter->Execute(), "Voronoi execution")) return false;

    auto output = filter->GetLineOutput();
    bool ok = Check(output != nullptr, "output exists");
    ok &= Check(output->GetNumberOfPoints() == 5, "Resolution 4 creates 5 points");
    ok &= Check(output->GetNumberOfCells() == 4, "Resolution 4 creates 4 line cells");
    for (int i = 0; i < 5; ++i) {
        const auto& point = output->GetPoint(i);
        ok &= Check(Near(point[0], 0.5 * i) && Near(point[1], 0.0) && Near(point[2], 0.0),
                    "parameterized sample coordinate " + std::to_string(i));
    }

    auto& temperature = output->GetAttributeSet()->GetAttribute("Temperature");
    auto& velocity = output->GetAttributeSet()->GetAttribute("Velocity");
    auto& integer = output->GetAttributeSet()->GetAttribute("IntegerSamples");
    ok &= Check(!temperature.IsNone() && temperature.pointer->GetArrayType() == IG_DoubleArray,
                "scalar name and type are preserved");
    ok &= Check(!velocity.IsNone() && velocity.pointer->GetDimension() == 3 &&
                        velocity.pointer->GetArrayType() == IG_FloatArray,
                "vector name, type, and components are preserved");
    ok &= Check(!integer.IsNone() && integer.pointer->GetArrayType() == IG_FloatArray,
                "integral arrays are promoted to float like VTK");
    ok &= Check(Near(temperature.pointer->GetValue(0), 0.0) && Near(temperature.pointer->GetValue(1), 0.0),
                "Voronoi samples nearest to the first source point");
    ok &= Check(Near(temperature.pointer->GetValue(3), 20.0) && Near(temperature.pointer->GetValue(4), 20.0),
                "Voronoi samples nearest to the second source point");
    const double tieValue = temperature.pointer->GetValue(2);
    ok &= Check(Near(tieValue, 0.0) || Near(tieValue, 20.0),
                "an equidistant Voronoi sample selects either nearest source point");
    ok &= Check(source->GetNumberOfPoints() == 2 &&
                        source->GetAttributeSet()->GetAttribute("Temperature").pointer->GetNumberOfElements() == 2,
                "input remains unchanged");

    filter->SetPoint1(iGame::Point(1.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(3.0, 0.0, 0.0));
    filter->SetResolution(2);
    ok &= Check(filter->Execute(), "repeated execution");
    ok &= Check(filter->GetLineOutput()->GetNumberOfPoints() == 3 &&
                        Near(filter->GetLineOutput()->GetPoint(0)[0], 1.0) &&
                        Near(filter->GetLineOutput()->GetPoint(2)[0], 3.0),
                "repeated execution rebuilds output");
    return ok;
}

bool TestGaussianAndShepard() {
    std::cout << "Running Gaussian and Shepard tests..." << std::endl;
    auto source = CreateSource();
    if (!Check(source != nullptr, "the PointLineInterpolator test model must load automatically")) { return false; }
    auto filter = iGame::PointLineInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetPoint1(iGame::Point(1.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(1.5, 0.0, 0.0));
    filter->SetResolution(1);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::RADIUS);
    filter->SetRadius(2.0);

    filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
    filter->SetSharpness(2.0);
    bool ok = Check(filter->Execute(), "Gaussian execution");
    auto gaussian = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(gaussian->GetValue(0), 10.0), "Gaussian symmetric weights");
    auto promoted = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("IntegerSamples").pointer;
    ok &= Check(Near(promoted->GetValue(0), 1.5), "promoted arrays retain fractional interpolation values");

    filter->SetPoint1(iGame::Point(0.5, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(1.0, 0.0, 0.0));
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::SHEPARD);
    filter->SetPowerParameter(2.0);
    ok &= Check(filter->Execute(), "Shepard execution");
    auto shepard = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(shepard->GetValue(0), 2.0), "Shepard inverse-square weights");
    ok &= Check(Near(shepard->GetValue(1), 10.0), "Shepard symmetric weights");

    filter->SetPoint1(iGame::Point(0.25, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(1.75, 0.0, 0.0));
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::N_CLOSEST);
    filter->SetNumberOfPoints(1);
    ok &= Check(filter->Execute(), "N-closest footprint execution");
    auto nearest = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(nearest->GetValue(0), 0.0) && Near(nearest->GetValue(1), 20.0),
                "N-closest footprint limits the interpolation basis");
    return ok;
}

bool TestNullPointStrategiesAndInvalidParameters() {
    std::cout << "Running null-point and validation tests..." << std::endl;
    auto source = CreateSource();
    if (!Check(source != nullptr, "the PointLineInterpolator test model must load automatically")) { return false; }
    auto filter = iGame::PointLineInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetPoint1(iGame::Point(10.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(11.0, 0.0, 0.0));
    filter->SetResolution(2);
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::RADIUS);
    filter->SetRadius(0.1);
    filter->SetNullPointsStrategy(iGame::PointLineInterpolatorFilter::MASK_POINTS);
    filter->SetNullValue(-7.0);
    bool ok = Check(filter->Execute(), "mask null-points execution");
    auto output = filter->GetLineOutput();
    auto values = output->GetAttributeSet()->GetAttribute("Temperature").pointer;
    auto mask = output->GetAttributeSet()->GetAttribute("vtkValidPointMask").pointer;
    for (int i = 0; i < 3; ++i) {
        ok &= Check(Near(values->GetValue(i), -7.0), "null value " + std::to_string(i));
        ok &= Check(Near(mask->GetValue(i), 0.0), "invalid mask " + std::to_string(i));
    }

    filter->SetNullPointsStrategy(iGame::PointLineInterpolatorFilter::CLOSEST_POINT);
    ok &= Check(filter->Execute(), "closest-point fallback execution");
    values = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(values->GetValue(0), 20.0), "closest-point fallback value");

    filter->SetNullPointsStrategy(iGame::PointLineInterpolatorFilter::NULL_VALUE);
    filter->SetNullValue(-9.0);
    ok &= Check(filter->Execute(), "null-value execution");
    output = filter->GetLineOutput();
    values = output->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(values->GetValue(0), -9.0), "explicit null value is emitted");
    ok &= Check(output->GetAttributeSet()->GetAttribute("vtkValidPointMask").IsNone(),
                "null-value strategy does not emit a validity mask");

    filter->SetResolution(0);
    ok &= Check(!filter->Execute(), "Resolution below one is rejected");
    filter->SetResolution(1);
    filter->SetRadius(0.0);
    ok &= Check(!filter->Execute(), "non-positive radius is rejected");
    filter->SetRadius(1.0);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::N_CLOSEST);
    filter->SetNumberOfPoints(0);
    ok &= Check(!filter->Execute(), "empty N-closest footprint is rejected");
    return ok;
}

void VisualizeResult() {
    std::cout << "\n=== Visualization ===" << std::endl;
    std::cout << "Loading model: " << PointLineInterpolatorModelPath << std::endl;
    auto dataObject = iGame::FileIO::ReadFile(PointLineInterpolatorModelPath);
    auto source = iGame::DynamicCast<iGame::PointSet>(dataObject);
    if (!source) { std::cerr << "Failed to load model.\n"; return; }

    std::cout << "Source: " << source->GetNumberOfPoints() << " points" << std::endl;

    auto filter = iGame::PointLineInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetPoint1(iGame::Point(0.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(2.0, 1.0, 0.0));
    filter->SetResolution(40);
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::RADIUS);
    filter->SetRadius(5.0);
    filter->SetSharpness(2.0);

    if (!filter->Execute()) {
        std::cerr << "Filter execution failed.\n";
        return;
    }

    auto lineOutput = filter->GetLineOutput();
    std::cout << "Interpolated line: " << lineOutput->GetNumberOfPoints() << " points, "
              << lineOutput->GetNumberOfCells() << " line cells" << std::endl;

    auto scene = iGame::Scene::New();

    auto srcDraw = iGame::DynamicCast<iGame::DrawObject>(source);
    if (srcDraw) {
        srcDraw->SetViewStyle(IG_POINTS);
        srcDraw->SetPointSize(10.0f);
        srcDraw->SetDefaultColor(igm::vec3{1.0f, 1.0f, 1.0f});
    }
    scene->AddModel(source);

    auto lineDraw = iGame::DynamicCast<iGame::DrawObject>(lineOutput);
    if (lineDraw) {
        lineDraw->SetViewStyle(IG_WIREFRAME);
        lineDraw->SetLineWidth(3.0f);
        lineDraw->SetLineColor(igm::vec3{0.0f, 1.0f, 1.0f});
    }
    scene->AddModel(lineOutput);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetTitle("PointLineInterpolator - Diagonal Interpolation on Quad");
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    std::cout << "Render window opened. Close the window to exit." << std::endl;
    window->Show();
}
}

int main() {
    const bool ok = TestParameterizedLineAndVoronoi() && TestGaussianAndShepard() &&
                    TestNullPointStrategiesAndInvalidParameters();
    if (!ok) return 1;
    std::cout << "PointLineInterpolator acceptance tests passed.\n";
    if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) VisualizeResult();
    return 0;
}
