// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestCleanToGridFilter.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <MyFilter/iGameCleanToGridFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameAttributeSet.h>
#include <iGamePointSet.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>

#include <cmath>
#include <string>
#include <iostream>

// 辅助：跑一次 CleanToGrid，返回输出点数、单元数、属性数
struct CleanResult {
    IGsize points = 0;
    IGsize cells  = 0;
    IGsize attrs  = 0;
    bool   ok     = false;
};

static CleanResult runFilter(
    iGame::DataObject::Pointer obj,
    double tolerance,
    bool   isAbsolute,
    bool   mergePoints,
    bool   removeUnusedPoints,
    bool   removeDegenerateCells,
    iGame::CleanToGridFilter::RepresentativePolicy policy =
        iGame::CleanToGridFilter::RepresentativePolicy::FirstUsed)
{
    CleanResult r;

    auto filter = iGame::CleanToGridFilter::New();
    filter->SetInput(obj);

    if (isAbsolute) {
        filter->SetAbsoluteTolerance(tolerance);
        filter->SetToleranceIsAbsolute(true);
    } else {
        filter->SetToleranceFraction(tolerance);
        filter->SetToleranceIsAbsolute(false);
    }

    filter->SetMergePoints(mergePoints);
    filter->SetRemoveUnusedPoints(removeUnusedPoints);
    filter->SetRemoveDegenerateCells(removeDegenerateCells);
    filter->SetRepresentativePolicy(policy);

    if (!filter->Execute()) return r;

    auto out = filter->GetOutput();
    if (!out) return r;

    auto outPoints = out->GetPoints();
    auto outCells  = out->GetCellArray();
    r.points = outPoints ? outPoints->GetNumberOfPoints() : 0;
    r.cells  = outCells  ? outCells ->GetNumberOfCells () : 0;
    r.attrs  = out->GetAttributeSet()
                   ? out->GetAttributeSet()->GetNumberOfAttributes()
                   : 0;
    r.ok = true;
    return r;
}

// main
int main() {
    std::cout << "========== CleanToGridFilter Test ==========" << std::endl;

    // ---- 1. 读测试数据 ----
    const std::string fileName = "./Models/Convert_Quad_Bicycle.vtk";

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR: cannot open " << fileName << std::endl;
        return 1;
    }

    auto inPoints = obj->GetPoints();
    auto inCells  = obj->GetCellArray();
    if (inPoints == nullptr || inCells == nullptr) {
        std::cout << "Input data has no points/cells" << std::endl;
        return 1;
    }

    const IGsize inPointNum = inPoints->GetNumberOfPoints();
    const IGsize inCellNum  = inCells ->GetNumberOfCells();
    const IGsize inAttrNum  = obj->GetAttributeSet()
                                  ? obj->GetAttributeSet()->GetNumberOfAttributes()
                                  : 0;

    std::cout << "Input  - Points: " << inPointNum
              << ", Cells: " << inCellNum << std::endl;
    std::cout << "Input  - Attributes: " << inAttrNum << std::endl;

    // ---- bbox 对角线（Test 6/7 用）----
    auto bbox = obj->GetBoundingBox();
    const double bboxDiag = (bbox.max - bbox.min).norm();
    std::cout << "Bounding box diagonal = " << bboxDiag << std::endl;

    bool allPassed = true;

    // Test 1：对齐 ParaView（只合并点，不删其他）
    //   期望：82212 → 53878；96668 → 96668
    {
        std::cout << "\n--- Test 1: Align with ParaView (merge only) ---" << std::endl;
        auto r = runFilter(obj, 0.001, /*abs*/true,
                           /*merge*/true, /*unused*/false, /*degen*/false);

        if (!r.ok) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else {
            std::cout << "Output - Points: " << r.points
                      << ", Cells: " << r.cells << std::endl;
            if (r.points == 53878 && r.cells == 96668) {
                std::cout << "PASS" << std::endl;
            } else {
                std::cout << "FAIL: expect 53878/96668, got "
                          << r.points << "/" << r.cells << std::endl;
                allPassed = false;
            }
        }
    }

    // Test 2：默认行为（3 个开关都开）
    //   期望：82212 → 53878；96668 → 80836
    {
        std::cout << "\n--- Test 2: Default behavior (all 3 switches ON) ---" << std::endl;
        auto r = runFilter(obj, 0.001, /*abs*/true,
                           /*merge*/true, /*unused*/true, /*degen*/true);

        if (!r.ok) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else {
            std::cout << "Output - Points: " << r.points
                      << ", Cells: " << r.cells << std::endl;
            if (r.points == 53878 && r.cells == 80836) {
                std::cout << "PASS" << std::endl;
            } else {
                std::cout << "FAIL: expect 53878/80836, got "
                          << r.points << "/" << r.cells << std::endl;
                allPassed = false;
            }
        }
    }

    // Test 3：容差 0（精确去重）
    //   此模型无"完全重合"的点 → 点数不变
    {
        std::cout << "\n--- Test 3: Tolerance 0 (exact dedup) ---" << std::endl;
        auto r = runFilter(obj, 0.0, /*abs*/true,
                           /*merge*/true, /*unused*/false, /*degen*/false);

        if (!r.ok) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else {
            std::cout << "Output - Points: " << r.points
                      << ", Cells: " << r.cells << std::endl;
            if (r.points == inPointNum && r.cells == inCellNum) {
                std::cout << "PASS: no exact duplicates, point count unchanged" << std::endl;
            } else {
                std::cout << "FAIL: expect " << inPointNum << "/" << inCellNum
                          << ", got " << r.points << "/" << r.cells << std::endl;
                allPassed = false;
            }
        }
    }

    // Test 4：属性数量保留
    {
        std::cout << "\n--- Test 4: Attribute count preserved ---" << std::endl;
        auto r = runFilter(obj, 0.001, /*abs*/true,
                           /*merge*/true, /*unused*/false, /*degen*/false);

        if (!r.ok) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else if (r.attrs == inAttrNum) {
            std::cout << "PASS: attr count = " << r.attrs << std::endl;
        } else {
            std::cout << "FAIL: attrs " << inAttrNum
                      << " -> " << r.attrs << std::endl;
            allPassed = false;
        }
    }

    // Test 5：点属性策略
    //   FirstUsed 多次运行结果应稳定。
    //   （Average 未实现，界面已拦住用户选择，测试中不涉及。）
    {
        std::cout << "\n--- Test 5: Representative policy ---" << std::endl;

        auto r1 = runFilter(obj, 0.001, true, true, false, false,
                            iGame::CleanToGridFilter::RepresentativePolicy::FirstUsed);
        auto r2 = runFilter(obj, 0.001, true, true, false, false,
                            iGame::CleanToGridFilter::RepresentativePolicy::FirstUsed);
        if (r1.ok && r2.ok && r1.points == r2.points && r1.cells == r2.cells) {
            std::cout << "PASS: FirstUsed stable ("
                      << r1.points << " pts / " << r1.cells << " cells)"
                      << std::endl;
        } else {
            std::cout << "FAIL: FirstUsed unstable" << std::endl;
            allPassed = false;
        }
    }

    // Test 6：相对容差换算正确
    //   effective 应 ≈ bbox 对角线 × fraction
    {
        std::cout << "\n--- Test 6: Relative tolerance conversion ---" << std::endl;

        const double fraction = 0.01;
        auto filter = iGame::CleanToGridFilter::New();
        filter->SetInput(obj);
        filter->SetToleranceFraction(fraction);
        filter->SetToleranceIsAbsolute(false);

        const double effective = filter->GetEffectiveTolerance(obj);
        const double expected  = bboxDiag * fraction;

        std::cout << "fraction  = " << fraction  << std::endl;
        std::cout << "effective = " << effective << std::endl;
        std::cout << "expected  = " << expected  << std::endl;

        if (std::fabs(effective - expected) < 1e-9) {
            std::cout << "PASS" << std::endl;
        } else {
            std::cout << "FAIL: effective != diag * fraction" << std::endl;
            allPassed = false;
        }
    }

    // Test 7：相对容差 vs 等价绝对容差——结果一致
    {
        std::cout << "\n--- Test 7: Relative vs Absolute consistency ---" << std::endl;

        const double absTol    = 0.001;
        const double equivFrac = absTol / bboxDiag;

        std::cout << "absTol = " << absTol
                  << ", equivFrac = " << equivFrac << std::endl;

        auto rAbs = runFilter(obj, absTol, /*abs*/true,
                              true, false, false);
        auto rRel = runFilter(obj, equivFrac, /*abs*/false,
                              true, false, false);

        if (!rAbs.ok || !rRel.ok) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else {
            std::cout << "Absolute 0.001      -> " << rAbs.points
                      << " pts / " << rAbs.cells << " cells" << std::endl;
            std::cout << "Relative equiv frac -> " << rRel.points
                      << " pts / " << rRel.cells << " cells" << std::endl;

            if (rAbs.points == rRel.points && rAbs.cells == rRel.cells) {
                std::cout << "PASS: relative and equivalent absolute consistent" << std::endl;
            } else {
                std::cout << "FAIL: two modes inconsistent" << std::endl;
                allPassed = false;
            }
        }
    }

    // 最终结果
    if (!allPassed) {
        std::cout << "\n========== SOME TESTS FAILED ==========" << std::endl;
        return 1;
    }

    std::cout << "\n========== ALL TESTS PASSED ==========" << std::endl;

    // 弹窗展示：用方案 A（只合并点，与 ParaView 对齐）
    //   - 参数：绝对 0.001；只合并点；移除未使用点关闭；删除退化单元关闭
    //   - 结果：53878 点 / 96668 单元
    std::cout << "\n===== Show cleaned model in window =====" << std::endl;

    {
        auto filterShow = iGame::CleanToGridFilter::New();
        filterShow->SetInput(obj);
        filterShow->SetAbsoluteTolerance(0.001);
        filterShow->SetToleranceIsAbsolute(true);
        filterShow->SetMergePoints(true);
        filterShow->SetRemoveUnusedPoints(false);
        filterShow->SetRemoveDegenerateCells(false);

        if (!filterShow->Execute()) {
            std::cout << "FAIL: filter execute failed" << std::endl;
            return 1;
        }

        auto out = filterShow->GetOutput();
        if (!out) {
            std::cout << "FAIL: output is null" << std::endl;
            return 1;
        }

        auto drawObj = iGame::DynamicCast<iGame::DrawObject>(out);
        if (drawObj) {
            drawObj->SetViewStyle(IG_SURFACE);
            if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
            drawObj->ConvertToDrawableData();
        }

        auto scene = iGame::Scene::New();
        scene->AddModel(out);
        scene->ResetCameraView();

        iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
        window->SetSize(1280, 720);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);

        std::cout << "Showing window, close it to exit..." << std::endl;
        window->Show();
    }

    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}
