// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
#include <ForceStaticMesh/iGameForceStaticMeshFilter.h>
#include <iGameDataObject.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>
#include <iGameStreamingData.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <string>

namespace {

// 相对路径：Examples 构建目录会自动把 Examples/Models 拷贝为 ./Models
const std::string kModelA = "./Models/ForceStaticMesh_TimeSeries/deform_0.vtu";
const std::string kModelB = "./Models/ForceStaticMesh_TimeSeries/deform_1.vtu";
// 时序素材：同一拓扑的三种变形 + 一帧规模不同（64 点 / 27 单元）
const std::string kTimeSeriesPvd = "./Models/ForceStaticMesh_TimeSeries/deform.pvd";

int g_failed = 0;

void Check(bool ok, const std::string& what) {
    if (ok) {
        std::cout << "[ok]   " << what << "\n";
    } else {
        std::cout << "[FAIL] " << what << "\n";
        ++g_failed;
    }
}

// 取输出对象第一段的某个属性在 pointId 处的值（用于核对属性是否随帧更新）
double PointAttributeValue(iGame::DataObject::Pointer obj, const std::string& name, int pointId) {
    using namespace iGame;
    if (!obj) return -1.0;
    auto attrSet = obj->GetAttributeSet();
    if (!attrSet) return -1.0;
    const int index = attrSet->GetAttributeIndex(name);
    if (index < 0) return -1.0;
    auto ptr = attrSet->GetAttribute(index).pointer;
    if (!ptr) return -1.0;
    return ptr->GetValue(pointId);
}

} // namespace

int main() {
    using namespace iGame;

    // 两个点/单元数相同、几何不同的独立模型文件（模拟“规模相同的另一个模型”）
    auto meshA = DynamicCast<UnstructuredMesh>(FileIO::ReadFile(kModelA));
    auto meshB = DynamicCast<UnstructuredMesh>(FileIO::ReadFile(kModelB));
    if (!meshA || !meshB) {
        std::cerr << "FAIL: read model files\n";
        return 1;
    }
    if (meshA->GetNumberOfPoints() != 27 || meshA->GetNumberOfCells() != 8) {
        std::cerr << "FAIL: unexpected model size\n";
        return 1;
    }
    if (meshB->GetNumberOfPoints() != meshA->GetNumberOfPoints() ||
        meshB->GetNumberOfCells() != meshA->GetNumberOfCells()) {
        std::cerr << "FAIL: models should have same point/cell counts\n";
        return 1;
    }

    auto filter = ForceStaticMeshFilter::New();

    // 场景 1：同一输入对象多次执行 → 复用缓存（输出为同一对象）
    filter->SetInput(meshA);
    if (!filter->Execute()) { std::cerr << "FAIL: execute meshA #1\n"; return 1; }
    auto out1 = filter->GetOutput();

    filter->SetInput(meshA);
    if (!filter->Execute()) { std::cerr << "FAIL: execute meshA #2\n"; return 1; }
    auto out2 = filter->GetOutput();
    Check(out1.get() == out2.get(), "same-input cache reuse: yes");
    Check(!filter->WasCacheRebuilt(), "same-input re-execute does not rebuild geometry");

    // 场景 2：切换到规模相同的另一个输入对象 → 强制重建缓存（几何对应 meshB）
    filter->SetInput(meshB);
    if (!filter->Execute()) { std::cerr << "FAIL: execute meshB\n"; return 1; }
    auto out3 = filter->GetOutput();
    Check(out2.get() != out3.get() || filter->WasCacheRebuilt(),
          "different input object rebuilds cache");
    Check(filter->GetLastRebuildReason() == ForceStaticMeshFilter::RR_INPUT_CHANGED,
          "rebuild reason is RR_INPUT_CHANGED when input object changes");
    auto ps3 = DynamicCast<PointSet>(out3);
    Check(ps3 && ps3->GetNumberOfPoints() == 27, "rebuilt cache point count");
    // 待提交：源 TestA/TestB 缺失，使用已提供的变形帧验证重建后的几何。
    Check(ps3 && ps3->GetPoint(9) == meshB->GetPoint(9),
          "rebuilt cache geometry matches new input (available deformed frame geometry)");

    // 场景 3：强制重建（ForceCacheComputation）
    filter->SetForceCacheComputation(true);
    if (!filter->Execute()) { std::cerr << "FAIL: forced rebuild\n"; return 1; }
    filter->SetForceCacheComputation(false);
    Check(filter->WasCacheRebuilt(), "force rebuild actually rebuilds geometry");
    Check(filter->GetLastRebuildReason() == ForceStaticMeshFilter::RR_FORCED,
          "rebuild reason is RR_FORCED");

    // 场景 4：关闭静态缓存 → 输出直接跟随输入对象
    filter->SetStaticCacheEnabled(false);
    filter->SetInput(meshA);
    if (!filter->Execute()) { std::cerr << "FAIL: execute with static cache disabled\n"; return 1; }
    Check(filter->GetOutput().get() == meshA.get(),
          "static cache disabled: output follows input");
    filter->SetStaticCacheEnabled(true);

    /* ---------- 时序（时间步）驱动：固定第一帧几何、只更新属性 ---------- */

    auto tsObject = FileIO::ReadFile(kTimeSeriesPvd);
    if (!tsObject) {
        std::cerr << "FAIL: read time series pvd\n";
        return 1;
    }
    auto tsFrames = tsObject->PeekTimeFrames();
    if (!tsFrames || tsFrames->GetTimeNum() != 4) {
        std::cerr << "FAIL: time series should expose 4 frames\n";
        return 1;
    }

    auto tsFilter = ForceStaticMeshFilter::New();
    tsFilter->SetInput(tsObject);

    // 第 1 帧（t=0）：建立几何缓存
    tsObject->UpdateAnimation(0);
    if (!tsFilter->ExecuteAtTimeStep(0, tsFrames->GetTargetTimeValue(0))) {
        std::cerr << "FAIL: execute at frame 0: " << tsFilter->GetStatusMessage() << "\n";
        return 1;
    }
    auto tsOut = tsFilter->GetOutput();
    auto tsOutPoints = DynamicCast<PointSet>(tsOut);
    Check(tsOutPoints && tsOutPoints->GetNumberOfPoints() == 27, "time step 0: cache built");
    Check(tsFilter->GetCacheTimeStepIndex() == 0, "cache geometry records frame 0");
    Check(tsFilter->GetCacheDescription().find("t=0") != std::string::npos,
          "cache description mentions t=0 (\"缓存几何来自 t=0\")");
    const float frame0Z = tsOutPoints ? tsOutPoints->GetPoint(9)[2] : -1.f;
    Check(frame0Z == 1.f, "cache geometry is frame 0 (z of point 9 == 1)");

    // 第 2 帧（t=1）：几何变形（z 拉伸 1.5 倍），拓扑不变 → 只更新属性
    tsObject->UpdateAnimation(1);
    if (!tsFilter->ExecuteAtTimeStep(1, tsFrames->GetTargetTimeValue(1))) {
        std::cerr << "FAIL: execute at frame 1: " << tsFilter->GetStatusMessage() << "\n";
        return 1;
    }
    auto tsOut1 = tsFilter->GetOutput();
    Check(tsOut1.get() == tsOut.get(), "time step 1: cache object identity kept");
    Check(!tsFilter->WasCacheRebuilt(), "time step 1: geometry cache reused (no rebuild)");
    Check(tsFilter->GetCacheTimeStepIndex() == 0, "cache geometry still originates from frame 0");
    auto outPs1 = DynamicCast<PointSet>(tsOut1);
    Check(outPs1 && outPs1->GetPoint(9)[2] == 1.f,
          "time step 1: geometry stays at frame 0 (z of point 9 == 1, not 1.5)");
    Check(PointAttributeValue(tsOut1, "pressure", 0) == 100.0,
          "time step 1: attribute updated to frame 1 value (pressure[0] == 100)");

    // 属性更新后对象 / 属性集必须被标记为已修改（否则渲染不会刷新）
    const unsigned int mtimeAfter = tsOut1->GetAttributeSet()
            ? tsOut1->GetAttributeSet()->GetMTime().GetMTime() : 0u;
    tsObject->UpdateAnimation(2);
    if (!tsFilter->ExecuteAtTimeStep(2, tsFrames->GetTargetTimeValue(2))) {
        std::cerr << "FAIL: execute at frame 2\n";
        return 1;
    }
    const unsigned int mtimeAfter2 = tsOut1->GetAttributeSet()
            ? tsOut1->GetAttributeSet()->GetMTime().GetMTime() : 0u;
    Check(mtimeAfter2 > mtimeAfter, "attribute set is marked modified after each time step");
    Check(!tsFilter->WasCacheRebuilt(), "time step 2: geometry cache reused (no rebuild)");
    Check(PointAttributeValue(tsOut1, "pressure", 0) == 200.0,
          "time step 2: attribute updated (pressure[0] == 200)");

    // 第 4 帧（t=3）：点数 27 → 64、单元数 8 → 27 → 自动重建几何缓存并给出提示
    tsObject->UpdateAnimation(3);
    if (!tsFilter->ExecuteAtTimeStep(3, tsFrames->GetTargetTimeValue(3))) {
        std::cerr << "FAIL: execute at frame 3\n";
        return 1;
    }
    Check(tsFilter->WasCacheRebuilt(), "size change: geometry cache rebuilt");
    Check(tsFilter->GetLastRebuildReason() == ForceStaticMeshFilter::RR_SIZE_CHANGED,
          "size change: rebuild reason is RR_SIZE_CHANGED");
    Check(tsFilter->GetCacheNumberOfPoints() == 64 && tsFilter->GetCacheNumberOfCells() == 27,
          "size change: cache follows new size (64 points / 27 cells)");
    Check(tsFilter->GetCacheTimeStepIndex() == 3,
          "size change: cache geometry origin updated to frame 3");
    Check(tsFilter->GetStatusMessage().find("几何规模变化") != std::string::npos,
          "size change: status message reports the size change");
    auto outPs3 = DynamicCast<PointSet>(tsFilter->GetOutput());
    Check(outPs3 && outPs3->GetNumberOfPoints() == 64, "size change: output has 64 points");

    std::cout << "\ntime series status: " << tsFilter->GetStatusMessage() << "\n";
    std::cout << "cache description: " << tsFilter->GetCacheDescription() << "\n";

    if (g_failed != 0) {
        std::cout << "\nResult: FAIL (" << g_failed << " checks failed)\n";
        return 1;
    }
    std::cout << "\nResult: PASS\n";
    return 0;
}
