// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Interpolation/TestPointVolumeInterpolatorSelfCheck.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
// 点体积插值（PointVolumeInterpolator）无 GUI 自动回归自检。
// 覆盖：Voronoi 精确还原、常数场加权归一、N 近邻、Null 三策略、输出结构。
#include <Interpolation/iGamePointVolumeInterpolatorFilter.h>

#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameStructuredMesh.h>
#include <iGameType.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace iGame;

namespace {

int g_checks = 0;
int g_failures = 0;

void Expect(bool ok, const char* what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  -> %s : %s\n", ok ? "PASS" : "FAIL", what);
}

bool Near(double a, double b, double tol = 1e-4) {
    return std::fabs(a - b) <= tol;
}

FloatArray::Pointer MakeScalar(const char* name, IGsize n,
                               const std::vector<float>& values) {
    auto arr = FloatArray::New();
    arr->SetName(name);
    arr->SetDimension(1);
    arr->Resize(n);
    for (IGsize i = 0; i < n; ++i) { arr->SetValue(i, values[i]); }
    return arr;
}

FloatArray::Pointer MakeVector(const char* name, IGsize n,
                               const std::vector<float>& values) {
    auto arr = FloatArray::New();
    arr->SetName(name);
    arr->SetDimension(3);
    arr->Resize(n);
    for (IGsize i = 0; i < n; ++i) {
        float v[3] = {values[i * 3], values[i * 3 + 1], values[i * 3 + 2]};
        arr->SetElement(i, v);
    }
    return arr;
}

StructuredMesh::Pointer RunFilter(PointSet::Pointer input, PointKernelType kernel,
                                  PointKernelFootprint footprint, double radius,
                                  int numberOfPoints,
                                  PointNullPointsStrategy strategy, double nullValue,
                                  bool useInputBounds, int res,
                                  bool* okOut = nullptr) {
    auto filter = PointVolumeInterpolatorFilter::New();
    filter->SetInput(input);
    filter->SetKernelType(kernel);
    filter->SetKernelFootprint(footprint);
    filter->SetRadius(radius);
    filter->SetNumberOfPoints(numberOfPoints);
    filter->SetNullPointsStrategy(strategy);
    filter->SetNullValue(nullValue);
    filter->SetUseInputBounds(useInputBounds);
    filter->SetResolution(res, res, res);
    if (okOut) *okOut = false;
    if (!filter->Execute()) {
        std::printf("  filter Execute failed: %s\n", filter->GetMessage().c_str());
        return nullptr;
    }
    if (okOut) *okOut = true;
    return DynamicCast<StructuredMesh>(filter->GetOutput());
}

bool GetFloatArray(const StructuredMesh::Pointer& mesh, const char* name,
                   FloatArray::Pointer& out) {
    if (!mesh || !mesh->GetAttributeSet()) return false;
    auto& attr = mesh->GetAttributeSet()->GetAttribute(name);
    out = DynamicCast<FloatArray>(attr.pointer);
    return out != nullptr;
}

double MaskAt(const StructuredMesh::Pointer& mesh, IGsize id) {
    auto& attr = mesh->GetAttributeSet()->GetAttribute(PointVolumeInterpolatorFilter::ValidPointsMaskName);
    auto mask = DynamicCast<CharArray>(attr.pointer);
    return (mask != nullptr) ? mask->GetValue(id) : -1.0;
}

// 构造 3x3x3 规则点云（点序 i 最快，与外层 k、内层 i 的网格一致），
// 标量 f=i+10j+100k，向量 g=(i,j,k)。
PointSet::Pointer MakeGridCloud() {
    auto cloud = PointSet::New();
    auto pts = cloud->GetPoints();
    std::vector<float> f, g;
    const int n = 3;
    for (int k = 0; k < n; ++k)
        for (int j = 0; j < n; ++j)
            for (int i = 0; i < n; ++i) {
                pts->AddPoint(float(i), float(j), float(k));
                f.push_back(float(i + 10 * j + 100 * k));
                g.push_back(float(i));
                g.push_back(float(j));
                g.push_back(float(k));
            }
    auto attrs = cloud->GetAttributeSet();
    attrs->AddAttribute(IG_SCALAR, IG_POINT, MakeScalar("f", 27, f));
    attrs->AddAttribute(IG_VECTOR, IG_POINT, MakeVector("g", 27, g));
    return cloud;
}

// 非规则点云（确定性伪随机），常数标量 c。
PointSet::Pointer MakeScatterCloud(float c) {
    auto cloud = PointSet::New();
    auto pts = cloud->GetPoints();
    std::vector<float> vals;
    const int n = 40;
    for (int i = 0; i < n; ++i) {
        const float t = float(i);
        const float x = 2.0f + 0.31f * t - 0.017f * t * t * 0.1f;
        const float y = 1.0f + 0.13f * i - 0.02f * (i % 7);
        const float z = 0.5f + 0.07f * (i * i % 13);
        pts->AddPoint(x, y, z);
        vals.push_back(c);
    }
    cloud->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, MakeScalar("c", n, vals));
    return cloud;
}

// 3x3x3 规则点云，常数标量 c（便于验证权重归一）。
PointSet::Pointer MakeConstantGridCloud(float c) {
    auto cloud = PointSet::New();
    auto pts = cloud->GetPoints();
    std::vector<float> vals;
    for (int k = 0; k < 3; ++k)
        for (int j = 0; j < 3; ++j)
            for (int i = 0; i < 3; ++i) {
                pts->AddPoint(float(i), float(j), float(k));
                vals.push_back(c);
            }
    cloud->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, MakeScalar("c", 27, vals));
    return cloud;
}

void TestVoronoiExactAndStructure() {
    std::printf("[case] Voronoi exact reproduction + output structure\n");
    auto cloud = MakeGridCloud();
    bool ok = false;
    auto out = RunFilter(cloud, PointKernelType::Voronoi, PointKernelFootprint::Radius,
                         1.0, 8, PointNullPointsStrategy::NullValue, 0.0, true, 3, &ok);
    Expect(ok && out != nullptr, "Execute returns true");
    if (!out) return;

    igIndex* mdims = out->GetDimensionSize();
    Expect(mdims[0] == 3 && mdims[1] == 3 && mdims[2] == 3, "output dims = 3x3x3");

    FloatArray::Pointer f, g;
    Expect(GetFloatArray(out, "f", f), "scalar 'f' present");
    Expect(GetFloatArray(out, "g", g), "vector 'g' present");
    if (!f || !g) return;
    Expect(f->GetDimension() == 1, "f dimension = 1");
    Expect(g->GetDimension() == 3, "g dimension = 3");

    bool exact = true, vectorOk = true, maskOk = true;
    for (IGsize id = 0; id < 27; ++id) {
        const int i = static_cast<int>(id % 3);
        const int j = static_cast<int>((id / 3) % 3);
        const int k = static_cast<int>(id / 9);
        if (!Near(f->GetValue(id), double(i + 10 * j + 100 * k))) exact = false;
        if (!Near(g->GetElementValue(id, 0), i) ||
            !Near(g->GetElementValue(id, 1), j) ||
            !Near(g->GetElementValue(id, 2), k)) vectorOk = false;
        if (!Near(MaskAt(out, id), 1.0)) maskOk = false;
    }
    Expect(exact, "Voronoi reproduces scalar exactly at coincident points");
    Expect(vectorOk, "Voronoi reproduces vector exactly");
    Expect(maskOk, "vtkValidPointMask = 1 for all hit points");
}

void TestConstantField() {
    std::printf("[case] constant field preserved (weight normalization)\n");
    const float c = 2.5f;

    struct Case { PointKernelType kernel; const char* name; };
    const Case cases[] = {
            {PointKernelType::Voronoi, "Voronoi"},
            {PointKernelType::Gaussian, "Gaussian(Radius)"},
            {PointKernelType::Shepard, "Shepard(Radius)"}};

    for (const auto& cs : cases) {
        auto cloud = MakeScatterCloud(c);
        bool ok = false;
        auto out = RunFilter(cloud, cs.kernel, PointKernelFootprint::Radius,
                             1000.0, 8, PointNullPointsStrategy::NullValue, 0.0, true, 4, &ok);
        if (!ok || !out) { Expect(false, cs.name); continue; }
        FloatArray::Pointer outArr;
        if (!GetFloatArray(out, "c", outArr)) { Expect(false, cs.name); continue; }
        bool allC = true;
        for (IGsize id = 0; id < outArr->GetNumberOfElements(); ++id) {
            if (!Near(outArr->GetValue(id), c, 1e-3)) allC = false;
        }
        Expect(allC, cs.name);
    }

    // N_CLOSEST（Gaussian k=4）：结果有限且全部命中
    auto cloud = MakeScatterCloud(c);
    bool ok = false;
    auto out = RunFilter(cloud, PointKernelType::Gaussian, PointKernelFootprint::NClosest,
                         1.0, 4, PointNullPointsStrategy::NullValue, 0.0, true, 4, &ok);
    if (ok && out) {
        FloatArray::Pointer outArr;
        bool finiteOk = GetFloatArray(out, "c", outArr);
        bool maskOk = true;
        if (finiteOk) {
            for (IGsize id = 0; id < outArr->GetNumberOfElements(); ++id) {
                const double v = outArr->GetValue(id);
                if (!std::isfinite(v) || v < -1e6 || v > 1e6) finiteOk = false;
                if (!Near(MaskAt(out, id), 1.0)) maskOk = false;
            }
        }
        Expect(finiteOk, "Gaussian N_CLOSEST produces finite values");
        Expect(maskOk, "Gaussian N_CLOSEST marks all points valid");
    } else {
        Expect(false, "Gaussian N_CLOSEST Execute");
    }
}

void TestNullStrategies() {
    std::printf("[case] null points strategies\n");
    auto cloud = PointSet::New();
    cloud->GetPoints()->AddPoint(0.f, 0.f, 0.f);
    std::vector<float> one = {7.0f};
    cloud->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, MakeScalar("s", 1, one));

    // 采样区域放大到 [-2,2]^3，半径 0.1：仅原点附近命中
    auto run = [&](PointNullPointsStrategy strategy, double nullValue, bool* ok) {
        auto filter = PointVolumeInterpolatorFilter::New();
        filter->SetInput(cloud);
        filter->SetKernelType(PointKernelType::Gaussian);
        filter->SetKernelFootprint(PointKernelFootprint::Radius);
        filter->SetRadius(0.1);
        filter->SetNullPointsStrategy(strategy);
        filter->SetNullValue(nullValue);
        filter->SetUseInputBounds(false);
        filter->SetSamplingBounds(-2.0, 2.0, -2.0, 2.0, -2.0, 2.0);
        filter->SetResolution(5, 5, 5);
        *ok = filter->Execute();
        return DynamicCast<StructuredMesh>(filter->GetOutput());
    };

    {
        bool ok = false;
        auto out = run(PointNullPointsStrategy::NullValue, -5.0, &ok);
        Expect(ok && out != nullptr, "NullValue Execute");
        if (ok && out) {
            FloatArray::Pointer s;
            GetFloatArray(out, "s", s);
            // id=0 是 (-2,-2,-2)，远离样本
            Expect(s && Near(s->GetValue(0), -5.0), "NullValue fills NullValue");
            Expect(Near(MaskAt(out, 0), 0.0), "NullValue leaves mask 0");
            // 原点 (i=2,j=2,k=2) -> id = 2 + 5*2 + 25*2 = 62
            Expect(s && Near(s->GetValue(62), 7.0), "NullValue still interpolates hit point");
            Expect(Near(MaskAt(out, 62), 1.0), "hit point mask = 1");
        }
    }
    {
        bool ok = false;
        auto out = run(PointNullPointsStrategy::MaskPoints, 0.0, &ok);
        Expect(ok && out != nullptr, "MaskPoints Execute");
        if (ok && out) {
            Expect(Near(MaskAt(out, 0), 0.0), "MaskPoints marks null point 0");
            Expect(Near(MaskAt(out, 62), 1.0), "MaskPoints marks hit point 1");
        }
    }
    {
        bool ok = false;
        auto out = run(PointNullPointsStrategy::ClosestPoint, 0.0, &ok);
        Expect(ok && out != nullptr, "ClosestPoint Execute");
        if (ok && out) {
            FloatArray::Pointer s;
            GetFloatArray(out, "s", s);
            Expect(s && Near(s->GetValue(0), 7.0), "ClosestPoint falls back to nearest value");
            Expect(Near(MaskAt(out, 0), 1.0), "ClosestPoint mask = 1");
        }
    }
}

void TestLinearConstant() {
    std::printf("[case] Linear kernel preserves constant field\n");
    const float c = 3.25f;
    struct KernelCase { PointKernelType kernel; const char* name; };
    const KernelCase cases[] = {
            {PointKernelType::Linear, "Linear(Radius)"}};

    for (const auto& kc : cases) {
        auto cloud = MakeConstantGridCloud(c);
        auto filter = PointVolumeInterpolatorFilter::New();
        filter->SetInput(cloud);
        filter->SetKernelType(kc.kernel);
        filter->SetKernelFootprint(PointKernelFootprint::Radius);
        filter->SetRadius(10.0);     // Linear：覆盖全部点
        filter->SetNullPointsStrategy(PointNullPointsStrategy::NullValue);
        filter->SetUseInputBounds(true);
        filter->SetResolution(4, 4, 4);
        if (!filter->Execute()) {
            std::printf("  Execute failed: %s\n", filter->GetMessage().c_str());
            Expect(false, kc.name);
            continue;
        }
        auto out = DynamicCast<StructuredMesh>(filter->GetOutput());
        FloatArray::Pointer arr;
        if (!out || !GetFloatArray(out, "c", arr)) {
            Expect(false, kc.name);
            continue;
        }
        bool allC = true;
        for (IGsize id = 0; id < arr->GetNumberOfElements(); ++id) {
            if (!Near(arr->GetValue(id), c, 1e-3)) allC = false;
        }
        Expect(allC, kc.name);
    }
}

void TestArraySelectionAndResolutionGuard() {
    std::printf("[case] array selection + resolution guard\n");
    auto cloud = MakeGridCloud(); // 含 f(标量) 与 g(向量)

    // 只插值 f
    auto filter = PointVolumeInterpolatorFilter::New();
    filter->SetInput(cloud);
    filter->SetKernelType(PointKernelType::Voronoi);
    filter->SetInterpolateArrayNames({"f"});
    filter->SetUseInputBounds(true);
    filter->SetResolution(3, 3, 3);
    const bool ok = filter->Execute();
    Expect(ok, "Execute with selected array 'f'");
    FloatArray::Pointer f, g;
    auto out = DynamicCast<StructuredMesh>(filter->GetOutput());
    Expect(out && GetFloatArray(out, "f", f), "selected array 'f' present");
    Expect(out && !GetFloatArray(out, "g", g), "unselected array 'g' absent");

    // 分辨率超上限 -> 明确失败（不做大内存分配）
    auto tooBig = PointVolumeInterpolatorFilter::New();
    tooBig->SetInput(cloud);
    tooBig->SetResolution(1000, 1000, 1000);
    Expect(!tooBig->Execute(), "over-large resolution rejected");
}

void TestNullPointsDoNotCorruptVectors() {
    std::printf("[case] null points must not corrupt valid vector values\n");
    // 单点云：位置 (0,0.5,0)，标量 f=5，向量 v=(1,2,3)
    auto cloud = PointSet::New();
    cloud->GetPoints()->AddPoint(0.f, 0.5f, 0.f);
    std::vector<float> fv = {5.0f};
    std::vector<float> vv = {1.0f, 2.0f, 3.0f};
    cloud->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, MakeScalar("f", 1, fv));
    cloud->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_POINT, MakeVector("v", 1, vv));

    auto filter = PointVolumeInterpolatorFilter::New();
    filter->SetInput(cloud);
    filter->SetKernelType(PointKernelType::Gaussian);
    filter->SetKernelFootprint(PointKernelFootprint::Radius);
    filter->SetRadius(0.01);            // 只有 (0,0.5,0) 附近命中
    filter->SetNullPointsStrategy(PointNullPointsStrategy::NullValue);
    filter->SetUseInputBounds(false);
    filter->SetSamplingBounds(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
    filter->SetResolution(5, 5, 5);     // 间距 0.25；(0,0.5,0) 的 ptId = 0 + 5*2 + 25*0 = 10
    if (!filter->Execute()) {
        std::printf("  Execute failed: %s\n", filter->GetMessage().c_str());
        Expect(false, "Execute");
        return;
    }
    auto out = DynamicCast<StructuredMesh>(filter->GetOutput());
    FloatArray::Pointer f, v;
    GetFloatArray(out, "f", f);
    GetFloatArray(out, "v", v);
    Expect(f && v, "arrays f/v present");
    if (!f || !v) return;

    // 命中点 ptId=10：标量=5，向量=(1,2,3)（修复前会被空点的 SetValue 冲成 0）
    Expect(Near(MaskAt(out, 10), 1.0), "hit point mask=1");
    Expect(Near(f->GetValue(10), 5.0, 1e-3), "hit point scalar preserved");
    Expect(Near(v->GetElementValue(10, 0), 1.0, 1e-4) &&
           Near(v->GetElementValue(10, 1), 2.0, 1e-4) &&
           Near(v->GetElementValue(10, 2), 3.0, 1e-4),
           "hit point vector preserved (not zeroed by null points)");

    // 空点（例如 ptId=30/31/32，在修复前正是它们把 ptId=10 的向量写坏的）
    Expect(Near(MaskAt(out, 30), 0.0) && Near(MaskAt(out, 31), 0.0) &&
           Near(MaskAt(out, 32), 0.0), "null points mask=0");
    Expect(v->GetElementValue(30, 0) == 0.0 && v->GetElementValue(31, 1) == 0.0 &&
           v->GetElementValue(32, 2) == 0.0, "null points vector = NullValue");
}

} // namespace

int main() {
    std::printf("PointVolumeInterpolator self check\n");
    TestVoronoiExactAndStructure();
    TestConstantField();
    TestNullStrategies();
    TestLinearConstant();
    TestArraySelectionAndResolutionGuard();
    TestNullPointsDoNotCorruptVectors();

    std::printf("total checks: %d, failures: %d\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
