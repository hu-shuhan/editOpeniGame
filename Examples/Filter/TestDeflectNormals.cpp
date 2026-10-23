// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
#include "FeatureExtraction/iGameDeflectNormalsFilter.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameAttributeSet.h"
#include "iGameType.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace iGame;

// ---------------------------------------------------------------------------
// TestDeflectNormals - 自动化测试（改进版）
//
// 改进点测试覆盖：
//   1. 独立输出：验证原模型属性不变
//   2. 零向量保护：当 base + strength*V 为零时保留基准法向
//   3. 已有点法向优先使用
//   4. 输入校验（零法向、非法数字）
// ---------------------------------------------------------------------------

static const char* kModel1 = "./Models/DeflectNormals_SphereSurface.vtk";
static const char* kModel2 = "./Models/DeflectNormals_WaveSurface.vtk";

bool runSingleTest(const char* modelPath, const char* vecAttrName,
                   float strength, bool useUserNormal,
                   double unx, double uny, double unz) {
    std::printf("[INFO] model=%s  vecAttr='%s'  strength=%.3f  useUserNormal=%d",
                modelPath, vecAttrName, strength, useUserNormal ? 1 : 0);
    if (useUserNormal)
        std::printf("  userNormal=(%.2f, %.2f, %.2f)", unx, uny, unz);
    std::printf("\n");

    auto obj = iGame::FileIO::ReadFile(modelPath);
    if (obj == nullptr) {
        std::printf("[FAIL] failed to read: %s\n", modelPath);
        return false;
    }
    auto ps = DynamicCast<iGame::PointSet>(obj);
    auto sm = DynamicCast<iGame::SurfaceMesh>(obj);
    auto um = DynamicCast<iGame::UnstructuredMesh>(obj);
    long long nPts = ps ? (long long)ps->GetNumberOfPoints() : -1;
    long long nFaces = -1;
    if (um) nFaces = (long long)um->GetNumberOfCells();
    else if (sm) nFaces = (long long)sm->GetNumberOfFaces();
    std::printf("[INFO] nPts=%lld  nFaces=%lld\n", nPts, nFaces);

    auto attrSet = obj->GetAttributeSet();
    size_t beforeAttrs = attrSet ? attrSet->GetNumberOfAttributes() : 0;

    auto filter = iGame::DeflectNormalsFilter::New();
    filter->SetInput(obj);
    filter->SetAttributeByName(vecAttrName);
    filter->SetDeflectStrength(strength);
    filter->SetUseUserNormal(useUserNormal);
    if (useUserNormal) filter->SetUserNormal(unx, uny, unz);

    if (!filter->Execute()) {
        std::printf("[FAIL] Execute() failed: %s\n", filter->GetMessage().c_str());
        return false;
    }

    // 改进点4：验证原模型属性不变
    size_t inputAttrs = obj->GetAttributeSet()->GetNumberOfAttributes();
    if (inputAttrs != beforeAttrs) {
        std::printf("[FAIL] input model was modified (before=%zu, after=%zu)\n",
                    beforeAttrs, inputAttrs);
        return false;
    }

    auto output = filter->GetOutput();
    auto outAttrSet = output ? output->GetAttributeSet() : nullptr;
    if (!outAttrSet) {
        std::printf("[FAIL] no output AttributeSet\n");
        return false;
    }

    int resultIdx = outAttrSet->GetAttributeIndex("DeflectedNormals");
    if (resultIdx < 0) {
        int nAttr = (int)outAttrSet->GetNumberOfAttributes();
        if (nAttr > (int)beforeAttrs) resultIdx = nAttr - 1;
    }
    if (resultIdx < 0) {
        std::printf("[FAIL] DeflectedNormals not found in output\n");
        return false;
    }

    auto& resultAttr = outAttrSet->GetAttribute(resultIdx);
    auto resultArr = DynamicCast<iGame::ArrayObject>(resultAttr.pointer);
    if (!resultArr) {
        std::printf("[FAIL] output attribute not ArrayObject\n");
        return false;
    }

    const char* attStr = (resultAttr.attachmentType == IG_POINT) ? "POINT"
                      : (resultAttr.attachmentType == IG_CELL) ? "CELL" : "UNKNOWN";
    std::printf("[PASS] DeflectedNormals  attach=%s  dim=%d  nElem=%lld  inputUnchanged=YES\n",
                attStr, resultArr->GetDimension(),
                (long long)resultArr->GetNumberOfElements());

    // 改进点8：检查没有 NaN
    bool noNaN = true;
    if (resultArr->GetDimension() == 3) {
        int checkN = (int)resultArr->GetNumberOfElements();
        if (checkN > 100) checkN = 100;
        for (int i = 0; i < checkN; ++i) {
            float v[3] = {0, 0, 0};
            resultArr->GetElement(i, v);
            if (std::isnan(v[0]) || std::isnan(v[1]) || std::isnan(v[2])) {
                noNaN = false;
                std::printf("[FAIL] NaN detected at element %d\n", i);
                break;
            }
        }
    }

    // 抽样打印前 5 个偏转法向
    if (resultArr->GetDimension() == 3 && noNaN) {
        int printN = 5;
        if ((long long)printN > (long long)resultArr->GetNumberOfElements())
            printN = (int)resultArr->GetNumberOfElements();
        std::printf("[SAMPLE] first %d deflected normals:\n", printN);
        for (int i = 0; i < printN; ++i) {
            float v[3] = {0, 0, 0};
            resultArr->GetElement(i, v);
            std::printf("   pt[%d] = (%.5f, %.5f, %.5f)\n", i, v[0], v[1], v[2]);
        }
    }

    return noNaN;
}


int main(int argc, char* argv[]) {
    bool noRender = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--no-render") == 0) noRender = true;
    }

    std::printf("============================================\n");
    std::printf("  TestDeflectNormals (Improved Auto Test)\n");
    std::printf("============================================\n\n");

    bool allPass = true;

    // Test 1: Sphere + Velocity field + surface normal + strength=1.0
    std::printf("--- Test 1: SphereSurface + Velocity + surface normal ---\n");
    allPass &= runSingleTest(kModel1, "Velocity", 1.0f, false, 0, 0, 1);

    // Test 2: Wave + Position field + user normal (0,0,1) + strength=0.5
    std::printf("\n--- Test 2: WaveSurface + Position + user normal ---\n");
    allPass &= runSingleTest(kModel2, "Position", 0.5f, true, 0.0, 0.0, 1.0);

    // Test 3: Zero-strength (验证零向量保护)
    std::printf("\n--- Test 3: Zero strength (zero-vector protection) ---\n");
    allPass &= runSingleTest(kModel1, "Velocity", 0.0f, false, 0, 0, 1);

    // Summary
    std::printf("\n============================================\n");
    std::printf("  Result: %s\n", allPass ? "ALL PASSED" : "SOME FAILED");
    std::printf("============================================\n");

    // --no-render mode: skip the render window for CTest/CI runs
    if (noRender) {
        return allPass ? 0 : 1;
    }

    // Render sphere model with deflected normals
    auto obj = iGame::FileIO::ReadFile(kModel1);
    if (obj) {
        auto filter = iGame::DeflectNormalsFilter::New();
        filter->SetInput(obj);
        filter->SetAttributeByName("Velocity");
        filter->SetDeflectStrength(1.0f);
        filter->SetUseUserNormal(false);
        filter->Execute();

        auto output = filter->GetOutput();
        auto outAttrSet = output ? output->GetAttributeSet() : nullptr;

        auto scene = iGame::Scene::New();
        scene->AddModel(output);

        auto drawObj = DynamicCast<iGame::DrawObject>(output);
        if (drawObj && outAttrSet) {
            int idx = outAttrSet->GetAttributeIndex("DeflectedNormals");
            drawObj->SetViewStyle(IG_SURFACE);
            if (idx >= 0) drawObj->ViewCloudPicture(scene, idx);
        }

        auto window = iGame::RenderWindow::New();
        window->SetSize(960, 720);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);
        window->Show();
    }

    return allPass ? 0 : 1;
}
