#include "FeatureExtraction/iGameAdvancedGradientFilter.h"
#include "iGameAttributeSet.h"
#include "iGameDrawObject.h"
#include "iGameFileIO.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{

bool NearlyEqual(double a, double b, double tol = 1e-4) {
    return std::abs(a - b) <= tol;
}

int g_pass = 0;
int g_fail = 0;
int g_skip = 0;

void Report(bool ok, const std::string& name, const std::string& detail) {
    if (ok) {
        ++g_pass;
        std::cerr << "[PASS] " << name << " | " << detail << '\n';
    } else {
        ++g_fail;
        std::cerr << "[FAIL] " << name << " | " << detail << '\n';
    }
    std::cerr.flush();
}

void Skip(const std::string& name, const std::string& detail) {
    ++g_skip;
    std::cerr << "[SKIP] " << name << " | " << detail << '\n';
    std::cerr.flush();
}

iGame::SurfaceMesh::Pointer MakeTriangleSurface() {
    auto surface = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.0, 0.0, 0.0);
    points->AddPoint(1.0, 0.0, 0.0);
    points->AddPoint(0.0, 1.0, 0.0);
    surface->SetPoints(points);
    surface->SetFaces(iGame::CellArray::New());
    surface->RequestEditStatus();

    igIndex tri[3] = {0, 1, 2};
    surface->AddFace(tri, 3);

    auto scalar = iGame::FloatArray::New();
    scalar->SetDimension(1);
    scalar->SetName("field_x");
    float v0[1] = {0.0f};
    float v1[1] = {1.0f};
    float v2[1] = {0.0f};
    scalar->AddElement(v0);
    scalar->AddElement(v1);
    scalar->AddElement(v2);
    surface->GetAttributeSet()->AddScalar(IG_POINT, scalar);
    return surface;
}

bool TestSyntheticSurfaceScalar() {
    auto surface = MakeTriangleSurface();

    iGame::AdvancedGradientFilter::Pointer filter = iGame::AdvancedGradientFilter::New();
    filter->SetInput(surface);
    filter->SetAttributeByName("field_x");
    filter->SetComputeGradientTensor(true);
    filter->SetOutputToPointData(true);
    if (!filter->Execute()) {
        Report(false, "SyntheticSurfaceScalar", "Execute failed");
        return false;
    }

    int idx = surface->GetAttributeSet()->GetAttributeIndex("gradient");
    if (idx < 0) {
        Report(false, "SyntheticSurfaceScalar", "gradient attribute not found");
        return false;
    }
    auto& attr = surface->GetAttributeSet()->GetAttribute(idx);
    if (attr.pointer->GetDimension() != 3 || attr.attachmentType != IG_POINT) {
        Report(false, "SyntheticSurfaceScalar", "unexpected dim/attachment");
        return false;
    }

    // f = x  =>  gradient = (1, 0, 0)
    bool ok = true;
    for (int i = 0; i < 3; ++i) {
        float g[3];
        attr.pointer->GetElement(i, g);
        if (!NearlyEqual(g[0], 1.0) || !NearlyEqual(g[1], 0.0) || !NearlyEqual(g[2], 0.0)) {
            ok = false;
        }
    }
    Report(ok, "SyntheticSurfaceScalar", "gradient should be (1,0,0)");
    return ok;
}

bool TestSyntheticSurfaceCellOutput() {
    auto surface = MakeTriangleSurface();

    iGame::AdvancedGradientFilter::Pointer filter = iGame::AdvancedGradientFilter::New();
    filter->SetInput(surface);
    filter->SetAttributeByName("field_x");
    filter->SetComputeGradientTensor(true);
    filter->SetOutputToPointData(false);
    if (!filter->Execute()) {
        Report(false, "SyntheticSurfaceCellOutput", "Execute failed");
        return false;
    }

    int idx = surface->GetAttributeSet()->GetAttributeIndex("gradient");
    if (idx < 0) {
        Report(false, "SyntheticSurfaceCellOutput", "gradient attribute not found");
        return false;
    }
    auto& attr = surface->GetAttributeSet()->GetAttribute(idx);
    bool ok = attr.attachmentType == IG_CELL && attr.pointer->GetDimension() == 3;
    Report(ok, "SyntheticSurfaceCellOutput", "should be IG_CELL dim=3");
    return ok;
}

bool TestStreamTestVolumeVector() {
    const std::string fileName = "./Models/StreamTest.vtk";
    auto data = iGame::FileIO::ReadFile(fileName);
    if (data == nullptr) {
        Skip("StreamTestVolumeVector", "model not found");
        return true;
    }

    auto attrSet = data->GetAttributeSet();
    int vectorIndex = -1;
    for (int i = 0; i < attrSet->GetNumberOfAttributes(); ++i) {
        auto& attr = attrSet->GetAttribute(i);
        if (attr.pointer && attr.pointer->GetDimension() == 3) {
            vectorIndex = i;
            break;
        }
    }
    if (vectorIndex < 0) {
        Report(false, "StreamTestVolumeVector", "no vector attribute found");
        return false;
    }

    iGame::AdvancedGradientFilter::Pointer filter = iGame::AdvancedGradientFilter::New();
    filter->SetInput(data);
    filter->SetAttributeByIndex(vectorIndex);
    filter->SetComputeGradientTensor(true);
    filter->SetOutputToPointData(true);
    if (!filter->Execute()) {
        Report(false, "StreamTestVolumeVector", "Execute failed");
        return false;
    }

    int idx = attrSet->GetAttributeIndex("gradient");
    if (idx < 0) {
        Report(false, "StreamTestVolumeVector", "gradient attribute not found");
        return false;
    }
    auto& attr = attrSet->GetAttribute(idx);
    if (attr.pointer->GetDimension() != 9 || attr.attachmentType != IG_POINT) {
        Report(false, "StreamTestVolumeVector", "unexpected dim/attachment");
        return false;
    }

    // ParaView 参考值：StreamTest 5 号点梯度模长约 15.75
    float g[9];
    attr.pointer->GetElement(5, g);
    double mag = 0.0;
    for (int k = 0; k < 9; ++k) { mag += g[k] * g[k]; }
    mag = std::sqrt(mag);
    bool ok = NearlyEqual(mag, 15.75, 1.0);
    Report(ok, "StreamTestVolumeVector", "point5 magnitude ~15.75, got " + std::to_string(mag));
    return ok;
}

} // namespace

int main() {
    TestSyntheticSurfaceScalar();
    TestSyntheticSurfaceCellOutput();
    TestStreamTestVolumeVector();

    std::cerr << "\nPassed: " << g_pass << ", Failed: " << g_fail << ", Skipped: " << g_skip << '\n';
    std::cerr.flush();
    return g_fail == 0 ? 0 : 1;
}
