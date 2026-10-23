#include <cstdlib>
// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
/**
 * TestAppendDatasets.cpp
 * 测试 AppendDatasetsFilter — 对标 ParaView 的 Append Datasets 滤镜
 * (VTK 类: vtkAppendFilter)
 *
 * 验证：
 *   - 多个输入数据集合并为单个 UnstructuredMesh（保留原始 cell 类型）
 *   - MergePoints=true 时合并重合点
 *   - MergePoints=false 时保留所有点
 *   - 属性按交集规则保留（所有输入都具备才保留）
 *   - AddInput API 自动扩展输入数量
 *
 * 类名 AppendDatasetsFilter 与 ParaView 滤镜 "Append Datasets" 一一对应。
 */
#include <AppendDatasets/iGameAppendDatasetsFilter.h>
#include <iostream>
#include <string>

using namespace iGame;

static int g_passCount = 0;
static int g_failCount = 0;

static void TestCheck(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << name << std::endl;
        g_passCount++;
    } else {
        std::cout << "  [FAIL] " << name << std::endl;
        g_failCount++;
    }
}

static SurfaceMesh::Pointer CreateTriangleMesh(
    float v0x, float v0y, float v0z,
    float v1x, float v1y, float v1z,
    float v2x, float v2y, float v2z)
{
    auto points = Points::New();
    points->AddPoint(v0x, v0y, v0z);
    points->AddPoint(v1x, v1y, v1z);
    points->AddPoint(v2x, v2y, v2z);

    auto faces = CellArray::New();
    igIndex faceIds[3] = {0, 1, 2};
    faces->AddCellIds(faceIds, 3);

    auto mesh = SurfaceMesh::New();
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    return mesh;
}

int main() {
    std::cout << "===== TestAppendDatasets =====" << std::endl;

    // mesh1 顶点: (0,0,0), (1,0,0), (0,1,0)
    // mesh2 顶点: (1,0,0), (0,1,0), (1,1,0)
    // 共享 2 个点：(1,0,0), (0,1,0)
    auto mesh1 = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
    auto mesh2 = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

    std::cout << "mesh1: 3 pts, 1 face" << std::endl;
    std::cout << "mesh2: 3 pts, 1 face" << std::endl;
    std::cout << "Total input: 6 pts, 2 faces" << std::endl;

    // ===================================================================
    // Test 1: Merge points ON — 共享点应合并
    // 期望：4 unique pts, 2 cells
    // ===================================================================
    std::cout << "\n[Test 1] Merge points ON" << std::endl;
    {
        auto filter = AppendDatasetsFilter::New();
        filter->AddInput(mesh1);
        filter->AddInput(mesh2);
        filter->SetMergePoints(true);
        filter->SetTolerance(1e-6f);
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        TestCheck("Output is UnstructuredMesh", outMesh != nullptr);
        if (outMesh) {
            IGsize nPts = outMesh->GetPoints()->GetNumberOfPoints();
            IGsize nCells = outMesh->GetCells()->GetNumberOfCells();
            std::cout << "  pts: " << nPts << " (expected 4), cells: " << nCells << " (expected 2)" << std::endl;
            TestCheck("Unique pts == 4 (merged)", nPts == 4);
            TestCheck("Cells == 2", nCells == 2);
        }
    }

    // ===================================================================
    // Test 2: Merge points OFF — 所有点保留
    // 期望：6 pts, 2 cells
    // ===================================================================
    std::cout << "\n[Test 2] Merge points OFF" << std::endl;
    {
        auto filter = AppendDatasetsFilter::New();
        filter->AddInput(mesh1);
        filter->AddInput(mesh2);
        filter->SetMergePoints(false);
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        TestCheck("Output is UnstructuredMesh", outMesh != nullptr);
        if (outMesh) {
            IGsize nPts = outMesh->GetPoints()->GetNumberOfPoints();
            IGsize nCells = outMesh->GetCells()->GetNumberOfCells();
            std::cout << "  pts: " << nPts << " (expected 6), cells: " << nCells << " (expected 2)" << std::endl;
            TestCheck("Total pts == 6 (no merge)", nPts == 6);
            TestCheck("Cells == 2", nCells == 2);
        }
    }

    // ===================================================================
    // Test 3: 单输入场景
    // 期望：3 pts, 1 cell
    // ===================================================================
    std::cout << "\n[Test 3] Single input" << std::endl;
    {
        auto filter = AppendDatasetsFilter::New();
        filter->AddInput(mesh1);
        filter->SetMergePoints(true);
        bool ok = filter->Execute();
        TestCheck("Execute returns true", ok);

        auto outMesh = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        TestCheck("Output is UnstructuredMesh", outMesh != nullptr);
        if (outMesh) {
            IGsize nPts = outMesh->GetPoints()->GetNumberOfPoints();
            IGsize nCells = outMesh->GetCells()->GetNumberOfCells();
            std::cout << "  pts: " << nPts << " (expected 3), cells: " << nCells << " (expected 1)" << std::endl;
            TestCheck("Single input pts == 3", nPts == 3);
            TestCheck("Single input cells == 1", nCells == 1);
        }
    }

    // ===================================================================
    // Test 4: 点标量属性合并（交集规则）
    // 两个 mesh 都有 Temperature 属性，输出应保留并按 pointMap 重映射
    // ===================================================================
    std::cout << "\n[Test 4] Point scalar attribute merge" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto tempA = FloatArray::New();
        tempA->SetName("Temperature");
        tempA->SetDimension(1);
        tempA->AddValue(100.0f);
        tempA->AddValue(200.0f);
        tempA->AddValue(150.0f);
        meshA->GetAttributeSet()->AddScalar(IG_POINT, tempA);

        auto tempB = FloatArray::New();
        tempB->SetName("Temperature");
        tempB->SetDimension(1);
        tempB->AddValue(200.0f);
        tempB->AddValue(150.0f);
        tempB->AddValue(300.0f);
        meshB->GetAttributeSet()->AddScalar(IG_POINT, tempB);

        auto f = AppendDatasetsFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(true);
        f->SetTolerance(1e-6f);
        f->Execute();

        auto out = DynamicCast<UnstructuredMesh>(f->GetOutput());
        TestCheck("Output is UnstructuredMesh", out != nullptr);
        if (out) {
            auto& attr = out->GetAttributeSet()->GetScalar("Temperature");
            TestCheck("Point attr 'Temperature' merged (intersection rule)",
                      !attr.IsNone());
            if (!attr.IsNone()) {
                TestCheck("Temperature elements == 4 (unique pts)",
                          attr.pointer->GetNumberOfElements() == 4);
                TestCheck("Temperature dim == 1",
                          attr.pointer->GetDimension() == 1);
            }
        }
    }

    // ===================================================================
    // Test 5: 单元标量属性合并
    // MergePoints=OFF 时单元按顺序追加，属性按 cellOffset 重映射
    // ===================================================================
    std::cout << "\n[Test 5] Cell scalar attribute merge" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(2, 0, 0,  3, 0, 0,  2, 1, 0);

        auto stressA = FloatArray::New();
        stressA->SetName("Stress");
        stressA->SetDimension(1);
        stressA->AddValue(10.5f);
        meshA->GetAttributeSet()->AddScalar(IG_CELL, stressA);

        auto stressB = FloatArray::New();
        stressB->SetName("Stress");
        stressB->SetDimension(1);
        stressB->AddValue(20.5f);
        meshB->GetAttributeSet()->AddScalar(IG_CELL, stressB);

        auto f = AppendDatasetsFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(false);
        f->Execute();

        auto out = DynamicCast<UnstructuredMesh>(f->GetOutput());
        TestCheck("Output is UnstructuredMesh", out != nullptr);
        if (out) {
            auto& attr = out->GetAttributeSet()->GetScalar("Stress");
            TestCheck("Cell attr 'Stress' merged", !attr.IsNone());
            if (!attr.IsNone()) {
                TestCheck("Stress attachmentType == IG_CELL",
                          attr.attachmentType == IG_CELL);
                TestCheck("Stress elements == 2",
                          attr.pointer->GetNumberOfElements() == 2);
            }
        }
    }

    // ===================================================================
    // Test 6: 属性交集规则
    // Pressure 只在 meshA 出现，输出应不包含
    // ===================================================================
    std::cout << "\n[Test 6] Attribute intersection rule" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto tempA = FloatArray::New();
        tempA->SetName("Temperature");
        tempA->SetDimension(1);
        tempA->AddValue(100.0f);
        tempA->AddValue(200.0f);
        tempA->AddValue(150.0f);
        meshA->GetAttributeSet()->AddScalar(IG_POINT, tempA);

        auto pressA = FloatArray::New();
        pressA->SetName("Pressure");
        pressA->SetDimension(1);
        pressA->AddValue(1.0f);
        pressA->AddValue(2.0f);
        pressA->AddValue(1.5f);
        meshA->GetAttributeSet()->AddScalar(IG_POINT, pressA);

        auto tempB = FloatArray::New();
        tempB->SetName("Temperature");
        tempB->SetDimension(1);
        tempB->AddValue(200.0f);
        tempB->AddValue(150.0f);
        tempB->AddValue(300.0f);
        meshB->GetAttributeSet()->AddScalar(IG_POINT, tempB);

        auto f = AppendDatasetsFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(true);
        f->Execute();

        auto out = DynamicCast<UnstructuredMesh>(f->GetOutput());
        TestCheck("Output is UnstructuredMesh", out != nullptr);
        if (out) {
            auto& tempAttr = out->GetAttributeSet()->GetScalar("Temperature");
            auto& pressAttr = out->GetAttributeSet()->GetScalar("Pressure");
            TestCheck("Temperature exists (in both inputs)", !tempAttr.IsNone());
            TestCheck("Pressure absent (only in meshA, intersection rule)",
                      pressAttr.IsNone());
        }
    }

    // ===================================================================
    // Test 7: 点向量属性合并
    // 验证 dim=3 的向量属性也能按 pointMap 正确合并
    // ===================================================================
    std::cout << "\n[Test 7] Point vector attribute merge" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto velA = FloatArray::New();
        velA->SetName("Velocity");
        velA->SetDimension(3);
        float vA[9] = {1,0,0, 0,1,0, 1,1,0};
        for (int i = 0; i < 9; i++) {
            velA->AddValue(vA[i]);
        }
        meshA->GetAttributeSet()->AddVector(IG_POINT, velA);

        auto velB = FloatArray::New();
        velB->SetName("Velocity");
        velB->SetDimension(3);
        float vB[9] = {0,1,0, 1,1,0, 2,0,0};
        for (int i = 0; i < 9; i++) {
            velB->AddValue(vB[i]);
        }
        meshB->GetAttributeSet()->AddVector(IG_POINT, velB);

        auto f = AppendDatasetsFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(true);
        f->Execute();

        auto out = DynamicCast<UnstructuredMesh>(f->GetOutput());
        TestCheck("Output is UnstructuredMesh", out != nullptr);
        if (out) {
            auto& attr = out->GetAttributeSet()->GetVector("Velocity");
            TestCheck("Vector attr 'Velocity' merged", !attr.IsNone());
            if (!attr.IsNone()) {
                TestCheck("Velocity dim == 3",
                          attr.pointer->GetDimension() == 3);
                TestCheck("Velocity elements == 4 (unique pts)",
                          attr.pointer->GetNumberOfElements() == 4);
            }
        }
    }

    // ===================================================================
    // Test 8: Tolerance=0 精确合并（ParaView 默认语义）
    // 完全重合的点合并；仅接近但不完全一致的点不合并
    // ===================================================================
    std::cout << "\n[Test 8] Tolerance=0 exact merge (ParaView default)" << std::endl;
    {
        // Part A: 两个三角形共享 2 个完全重合的点 -> 4 unique pts
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto f1 = AppendDatasetsFilter::New();
        f1->AddInput(meshA);
        f1->AddInput(meshB);
        f1->SetMergePoints(true);
        f1->SetTolerance(0.0f);
        TestCheck("Execute true with Tolerance=0", f1->Execute());

        auto out1 = DynamicCast<UnstructuredMesh>(f1->GetOutput());
        TestCheck("Output is UnstructuredMesh", out1 != nullptr);
        if (out1) {
            TestCheck("Exactly coincident points merged (4 unique pts)",
                      out1->GetNumberOfPoints() == 4);
        }

        // Part B: 点仅接近 (1+1e-5) 但不完全一致 -> 不合并, 5 pts
        auto meshC = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshD = CreateTriangleMesh(1e-5f + 1.0f, 0, 0,  0, 1, 0,  1, 1, 0);

        auto f2 = AppendDatasetsFilter::New();
        f2->AddInput(meshC);
        f2->AddInput(meshD);
        f2->SetMergePoints(true);
        f2->SetTolerance(0.0f);
        TestCheck("Execute true (near points)", f2->Execute());

        auto out2 = DynamicCast<UnstructuredMesh>(f2->GetOutput());
        TestCheck("Output is UnstructuredMesh", out2 != nullptr);
        if (out2) {
            TestCheck("Near-but-not-exact points NOT merged (5 pts)",
                      out2->GetNumberOfPoints() == 5);
        }
    }

    // ===================================================================
    // Test 9: 构造默认值与 ParaView 一致（MergePoints=false, Tolerance=0）
    // ===================================================================
    std::cout << "\n[Test 9] Default params match ParaView (no merge, tol=0)" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto f = AppendDatasetsFilter::New();
        TestCheck("Default MergePoints == false", f->GetMergePoints() == false);
        f->AddInput(meshA);
        f->AddInput(meshB);
        TestCheck("Execute true with defaults", f->Execute());

        auto out = DynamicCast<UnstructuredMesh>(f->GetOutput());
        TestCheck("Output is UnstructuredMesh", out != nullptr);
        if (out) {
            TestCheck("Default keeps duplicate points (6 pts, ParaView default)",
                      out->GetNumberOfPoints() == 6);
        }
    }

    // ===================================================================
    // 汇总
    // ===================================================================
    std::cout << "\n===== Summary =====" << std::endl;
    std::cout << "  PASS: " << g_passCount << std::endl;
    std::cout << "  FAIL: " << g_failCount << std::endl;
    std::cout << "  Total: " << g_passCount + g_failCount << std::endl;

    if (g_failCount == 0) {
        std::cout << "\n  All tests PASSED!" << std::endl;
    } else {
        std::cout << "\n  Some tests FAILED!" << std::endl;
    }

    std::cout << "\nPress Enter to exit..." << std::endl;
    if (!std::getenv("IGAME_EXAMPLE_NO_RENDER")) std::cin.get();
    return g_failCount > 0 ? 1 : 0;
}
