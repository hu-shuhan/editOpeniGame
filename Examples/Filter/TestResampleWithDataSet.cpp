// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
/**
 * ResampleWithDataSet 自检示例
 *
 * 用 Examples/Models/RenameResample_test.vtk 里的六面体网格（8 个角点，点标量 value = 0..7）
 * 作为「被采样网格」，
 * 用自建点云 / 网格作为「采样点网格」，逐条断言：
 *   1) 输出类型与「采样点网格」一致，几何与拓扑是它的深拷贝；
 *   2) 源点数据被插值：六面体体心的三线性 / 均值坐标插值由对称性等于 8 个角点均值 3.5；
 *      自采样（探针 = 被采样网格本身）时有效点的插值结果应等于该点原值；
 *   3) validpointmask 与 GetSampleValidMask() 一致；网格外的点标记无效、插值填 0；
 *   4) 吸附半径关闭时近距网格外的点无效，开启后被吸附为有效，而远处的点仍无效；
 *   5) 「采样点网格」自身的数组**不**被复制到输出（对齐 vtkProbeFilter 默认 PassPointArrays/CellArrays=0）；
 *   6) 「被采样网格」的单元数据按命中单元拷到输出点上（与 vtkProbeFilter 的 InputCellArrays 一致）：
 *      源单元数组 cid = 42 以**点属性**出现在输出上，有效点取命中单元的值、无效点填 0；
 *   7) 缺少输入时 Execute() 返回 false 并给出消息。
 *
 * 「被采样网格」从 Examples/Models/RenameResample_test.vtk 读入（单位六面体，
 * 点数组 value = 0..7、alpha = 100..107，单元数组 cid = 42），不依赖渲染上下文，可直接运行。
 */
#include <ResampleWithDataset/iGameResampleWithDataSet.h>

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameUnstructuredMesh.h"

#include <iGameFileIO.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_Failed = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << what << std::endl;
    if (!ok) { ++g_Failed; }
}

void CheckNear(double actual, double expected, double tol, const std::string& what) {
    const bool ok = std::fabs(actual - expected) <= tol;
    std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << what << "  (actual=" << actual << ", expected=" << expected
              << ")" << std::endl;
    if (!ok) { ++g_Failed; }
}

/** 测试网格文件名（与 TestRenameArrays 共用，位于 Examples/Models/ 下） */
const char* kTestMeshName = "RenameResample_test.vtk";

/**
 * 定位 Examples/Models 下的模型：兼容从 <build>/Examples（ctest 的工作目录）、构建根目录、
 * 源码根目录等不同工作目录下运行。
 */
std::string ResolveModelPath(const std::string& fileName) {
    const std::vector<std::string> candidates = {
            std::string("Models/") + fileName,
            std::string("./Models/") + fileName,
            std::string("../Models/") + fileName,
            std::string("../../Models/") + fileName,
            std::string("Examples/Models/") + fileName,
            std::string("../../../Examples/Models/") + fileName,
    };
    for (const std::string& candidate : candidates) {
        if (std::filesystem::exists(candidate)) { return candidate; }
    }
    return std::string();
}

/** 定位测试网格（单位六面体） */
std::string ResolveTestMeshPath() { return ResolveModelPath(kTestMeshName); }

/**
 * 读入「被采样网格」：单位六面体，点数组 value(0..7)、alpha(100..107)，单元数组 cid=42。
 * 见 Examples/Models/RenameResample_test.vtk；体心的对称插值 = 8 个角点均值 3.5。
 */
iGame::UnstructuredMesh::Pointer MakeUnitHexahedron() {
    const std::string path = ResolveTestMeshPath();
    if (path.empty()) {
        Check(false, std::string("找到测试网格 ") + kTestMeshName +
                             "（请在 <build>/Examples 下运行，或确认 Examples/Models 已拷到构建目录）");
        return nullptr;
    }

    iGame::DataObject::Pointer object = iGame::FileIO::ReadFile(path);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(object);
    if (mesh == nullptr) {
        Check(false, "读取测试网格失败：" + path);
        return nullptr;
    }
    return mesh;
}

/** 5 个采样点 + 一个探针自带的标量数组 "probe_marker" */
iGame::PointSet::Pointer MakeProbePoints() {
    auto probe = iGame::PointSet::New();
    probe->SetName("probe_points");

    probe->AddPoint(iGame::Point(0.5f, 0.5f, 0.5f));    // 0: 体心（对称 -> 3.5）
    probe->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));    // 1: 角点
    probe->AddPoint(iGame::Point(-0.001f, -0.001f, -0.001f)); // 2: 刚出盒（近）
    probe->AddPoint(iGame::Point(10.0f, 10.0f, 10.0f)); // 3: 远处盒外
    probe->AddPoint(iGame::Point(0.5f, 0.5f, 0.0f));    // 4: 面心

    auto attrs = iGame::AttributeSet::New();
    auto marker = iGame::FloatArray::New();
    marker->SetName("probe_marker");
    marker->SetDimension(1);
    marker->Resize(5);
    for (int i = 0; i < 5; ++i) {
        const double v = 100.0 + static_cast<double>(i);
        marker->SetElement(static_cast<IGsize>(i), &v);
    }
    attrs->AddScalar(IG_POINT, marker);
    probe->SetAttributeSet(attrs);
    return probe;
}

/** 找输出里附着在点上的数组；hits 返回同名数组出现的次数（用于检查是否重复） */
iGame::ArrayObject::Pointer FindPointArray(iGame::AttributeSet* attrs, const std::string& name, int* hits) {
    if (hits != nullptr) { *hits = 0; }
    if (attrs == nullptr) { return nullptr; }

    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return nullptr; }

    iGame::ArrayObject::Pointer found;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT) { continue; }
        if (attr.pointer->GetName() != name) { continue; }
        if (hits != nullptr) { ++(*hits); }
        found = attr.pointer;
    }
    return found;
}

/* ------------------------------------------------------------------ */
/* 用例 1：自采样（探针 = 被采样网格本身，网格类型）                    */
/* ------------------------------------------------------------------ */
void TestSelfProbeOnMesh() {
    std::cout << "== 用例 1：网格自采样（检查几何/拓扑拷贝 + 插值还原 + 同名去重）" << std::endl;

    auto mesh = MakeUnitHexahedron();

    auto filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(mesh);
    filter->SetProbeData(mesh);
    Check(filter->Execute(), "Execute() 成功");

    auto output = filter->GetResampledData();
    if (output == nullptr) {
        Check(false, "输出非空");
        return;
    }
    Check(output->GetDataObjectType() == IG_UNSTRUCTURED_MESH, "输出类型与采样点网格一致（UnstructuredMesh）");

    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(output);
    if (outMesh == nullptr) {
        Check(false, "输出可转换为 UnstructuredMesh");
        return;
    }
    Check(outMesh->GetNumberOfPoints() == 8, "输出点数 = 8");
    Check(outMesh->GetNumberOfCells() == 1, "输出单元数 = 1");
    Check(outMesh->GetPoint(7)[2] == 1.0f && outMesh->GetPoint(7)[0] == 1.0f, "输出几何是采样点网格的拷贝");

    int hits = 0;
    auto outValue = FindPointArray(output->GetAttributeSet(), "value", &hits);
    Check(outValue != nullptr, "输出含插值数组 value");
    Check(hits == 1, "value 只出现一次（采样点网格自身数组不再参与，天然不重复）");

    // 探针点即原网格点：有效点的插值结果应等于该点原值
    const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
    int validCount = 0;
    int mismatch = 0;
    for (int i = 0; i < 8 && outValue != nullptr; ++i) {
        if (mask[static_cast<size_t>(i)] == 0) { continue; }
        ++validCount;
        const double interpolated = outValue->GetElementValue(static_cast<IGsize>(i), 0);
        if (std::fabs(interpolated - static_cast<double>(i)) > 1e-3) {
            ++mismatch;
            std::cout << "        点 " << i << " 插值=" << interpolated << " 原值=" << i << std::endl;
        }
    }
    Check(validCount >= 1, "至少有 1 个采样点有效（validCount=" + std::to_string(validCount) + "）");
    Check(mismatch == 0, "所有有效点的插值结果等于原值");

    // 被采样网格的单元数据（这里 cid = 42）按命中单元拷到输出的**点属性**上（vtkProbeFilter 语义），
    // 输出因此仍然没有任何单元附着属性。输出是网格时该断言才有意义，故放在本用例而不是点云用例。
    int cellAttachmentCount = 0;
    auto outAll = output->GetAttributeSet() ? output->GetAttributeSet()->GetAllAttributes() : nullptr;
    for (IGsize i = 0; outAll != nullptr && i < outAll->GetNumberOfElements(); ++i) {
        auto& attr = outAll->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType == IG_CELL) { ++cellAttachmentCount; }
    }
    Check(cellAttachmentCount == 0, "输出不含单元附着属性（源单元数据以点属性形式出现）");

    int cidHitsSelf = 0;
    auto cidSelf = FindPointArray(output->GetAttributeSet(), "cid", &cidHitsSelf);
    Check(cidSelf != nullptr && cidHitsSelf == 1, "源单元数组 cid 出现在输出上且只有一个");
    if (cidSelf != nullptr) {
        int cidMismatch = 0;
        for (int i = 0; i < 8; ++i) {
            if (mask[static_cast<size_t>(i)] == 0) { continue; }
            if (std::fabs(cidSelf->GetElementValue(static_cast<IGsize>(i), 0) - 42.0) > 1e-6) { ++cidMismatch; }
        }
        Check(cidMismatch == 0, "有效点的 cid 等于命中单元的值 42");
    }
}

/* ------------------------------------------------------------------ */
/* 用例 2：点云探针（检查输出类型/坐标/mask/自带数组不复制/数值）        */
/* ------------------------------------------------------------------ */
void TestPointCloudProbe() {
    std::cout << "\n== 用例 2：点云探针（输出为点云拷贝 + 采样属性）" << std::endl;

    auto mesh = MakeUnitHexahedron();
    auto probe = MakeProbePoints();

    auto filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(mesh);
    filter->SetProbeData(probe);
    Check(filter->Execute(), "Execute() 成功");

    auto output = filter->GetResampledData();
    if (output == nullptr) {
        Check(false, "输出非空");
        return;
    }
    Check(output->GetDataObjectType() == IG_POINT_SET, "输出类型与采样点网格一致（PointSet）");

    auto outPoints = iGame::DynamicCast<iGame::PointSet>(output);
    if (outPoints == nullptr) {
        Check(false, "输出可转换为 PointSet");
        return;
    }
    Check(outPoints->GetNumberOfPoints() == 5, "输出点数 = 5");

    bool coordsEqual = true;
    for (int i = 0; i < 5; ++i) {
        const iGame::Point a = outPoints->GetPoint(static_cast<IGsize>(i));
        const iGame::Point b = probe->GetPoint(static_cast<IGsize>(i));
        for (int c = 0; c < 3; ++c) {
            if (std::fabs(a[c] - b[c]) > 1e-6f) { coordsEqual = false; }
        }
    }
    Check(coordsEqual, "输出坐标与采样点网格逐点一致");

    const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
    Check(mask.size() == 5, "GetSampleValidMask() 长度为 5");

    int maskHits = 0;
    auto outMask = FindPointArray(output->GetAttributeSet(), "validpointmask", &maskHits);
    Check(outMask != nullptr && maskHits == 1, "输出含 validpointmask（且只有一个）");
    bool maskMatches = (outMask != nullptr) && (outMask->GetNumberOfValues() == 5);
    if (maskMatches) {
        for (int i = 0; i < 5; ++i) {
            if (static_cast<unsigned char>(outMask->GetElementValue(static_cast<IGsize>(i), 0)) !=
                mask[static_cast<size_t>(i)]) {
                maskMatches = false;
            }
        }
    }
    Check(maskMatches, "validpointmask 属性与 GetSampleValidMask() 一致");

    // 体心：对称性 -> 8 个角点均值
    Check(mask[0] == 1, "体心（0）有效");
    auto outValue = FindPointArray(output->GetAttributeSet(), "value", nullptr);
    if (outValue != nullptr && mask[0] == 1) { CheckNear(outValue->GetElementValue(0, 0), 3.5, 1e-3, "体心插值 = 3.5"); }

    // 远处点：无效且插值填 0
    Check(mask[3] == 0, "远处盒外点（3）无效");
    if (outValue != nullptr) { CheckNear(outValue->GetElementValue(3, 0), 0.0, 1e-6, "无效点的插值填 0"); }

    // 变更说明（与本次修复同次首次提交，为避免自引用此处不写提交号）：
    //   原现象：本滤波器把「采样点网格」自身的数组一并复制到输出（等价 PassPointArrays/PassCellArrays = ON），
    //           无效点上仍是原值，看起来像"未命中却没置 0"。
    //   正确行为：对齐 vtkProbeFilter 默认值（PassPointArrays = 0、PassCellArrays = 0），采样点网格自身的
    //           数组不复制；输出含「被采样网格的插值点数据 + 其单元数据（按命中单元）+ validpointmask」。
    //   本次提交主题："fix: ResampleWithDataSet 不再复制采样点网格自身属性（对齐 vtkProbeFilter 默认 PassPointArrays/CellArrays=0）"
    //   查询该测试引入提交的命令：
    //     git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestResampleWithDataSet.cpp
    int markerHits = 0;
    auto outMarker = FindPointArray(output->GetAttributeSet(), "probe_marker", &markerHits);
    Check(outMarker == nullptr && markerHits == 0, "采样点网格自身的数组 probe_marker 不再出现在输出中");
    if (outMarker != nullptr) { CheckNear(outMarker->GetElementValue(0, 0), 100.0, 1e-6, "probe_marker 数值保持"); }

    // 变更说明（被采样网格的单元数据按命中单元进入输出点属性；提交状态：待提交）：
    //   背景：本滤波器曾一度去掉源单元数据的拷贝（输出只留插值点数据 + validpointmask），
    //         与 vtkProbeFilter 不一致：VTK 会把源单元数组作为 InputCellArrays 拷成**输出点数据**
    //         （见 vtkProbeFilter::InitializeForProbing 与 ProbeEmptyPointsWorklet）。
    //   正确行为：源单元数组 cid（值 42）以点属性出现在输出上，有效点取命中单元的值、
    //         无效点填 0（VTK 在 InitializeOutputArrays 里对输出数组 Fill(0)）。
    int cidHits = 0;
    auto outCid = FindPointArray(output->GetAttributeSet(), "cid", &cidHits);
    Check(outCid != nullptr && cidHits == 1, "被采样网格的单元数组 cid 作为点属性出现在输出中（且只有一个）");
    if (outCid != nullptr) {
        CheckNear(outCid->GetElementValue(0, 0), 42.0, 1e-6, "有效点 cid = 命中单元的值 42");
        CheckNear(outCid->GetElementValue(3, 0), 0.0, 1e-6, "无效点 cid 填 0");
    }

    std::cout << "        有效点数 = " << static_cast<int>(filter->GetSampleValidMask().size()) << " 中 "
              << "有效=" << (int)mask[0] << (int)mask[1] << (int)mask[2] << (int)mask[3] << (int)mask[4] << std::endl;
}

/* ------------------------------------------------------------------ */
/* 用例 3：吸附半径                                                    */
/* ------------------------------------------------------------------ */
void TestSnappingRadius() {
    std::cout << "\n== 用例 3：吸附半径（默认关；开启后近距盒外点被吸附）" << std::endl;

    // 关闭吸附
    {
        auto mesh = MakeUnitHexahedron();
        auto probe = MakeProbePoints();
        auto filter = iGame::ResampleWithDataSet::New();
        filter->SetSourceData(mesh);
        filter->SetProbeData(probe);
        filter->SetSnappingRadius(0.0);
        Check(filter->Execute(), "radius=0: Execute() 成功");
        const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
        Check(mask.size() == 5 && mask[2] == 0, "radius=0: 近距盒外点（2）无效");
        Check(filter->GetSnappedPointCount() == 0, "radius=0: 吸附点数为 0");
    }

    // 开启吸附：对角线 ≈ 1.732，半径 0.01 * diag ≈ 0.0173 > 0.001*sqrt(3)
    {
        auto mesh = MakeUnitHexahedron();
        auto probe = MakeProbePoints();
        auto filter = iGame::ResampleWithDataSet::New();
        filter->SetSourceData(mesh);
        filter->SetProbeData(probe);
        filter->SetSnappingRadius(0.01);
        Check(filter->Execute(), "radius=0.01: Execute() 成功");
        const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
        Check(mask.size() == 5 && mask[2] == 1, "radius=0.01: 近距盒外点（2）被吸附为有效");
        Check(filter->GetSnappedPointCount() >= 1, "radius=0.01: 吸附点数 >= 1");
        Check(mask[3] == 0, "radius=0.01: 远处盒外点（3）仍然无效");
    }
}

/* ------------------------------------------------------------------ */
/* 用例 4：错误路径                                                    */
/* ------------------------------------------------------------------ */
void TestErrorPaths() {
    std::cout << "\n== 用例 4：错误路径" << std::endl;

    auto mesh = MakeUnitHexahedron();

    auto filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(mesh);
    Check(!filter->Execute(), "缺少「采样点网格」时 Execute() 返回 false");
    Check(!filter->GetMessage().empty(), "失败时给出消息：" + filter->GetMessage());

    auto filter2 = iGame::ResampleWithDataSet::New();
    filter2->SetProbeData(MakeProbePoints());
    Check(!filter2->Execute(), "缺少「被采样网格」时 Execute() 返回 false");
    Check(!filter2->GetMessage().empty(), "失败时给出消息：" + filter2->GetMessage());
}

// Bug: ordinary hexahedra used polyhedral MVC, making StreamTest-to-plane
// sampling slow and producing different values from trilinear interpolation.
// An off-center xyz field distinguishes these bases; outside/face probes
// protect containment. Fix commit: pending (not yet committed).
void TestTrilinearHexahedron() {
    auto mesh = MakeUnitHexahedron();
    if (!mesh) { return; }
    auto values = iGame::FloatArray::New();
    values->SetName("xyz");
    values->SetDimension(1);
    values->Resize(8);
    for (int i = 0; i < 8; ++i) {
        const auto& p = mesh->GetPoint(i);
        values->SetValue(i, p[0] * p[1] * p[2]);
    }
    mesh->GetAttributeSet()->AddScalar(IG_POINT, values);
    auto probe = iGame::PointSet::New();
    probe->AddPoint(iGame::Point(.2f, .3f, .4f));
    probe->AddPoint(iGame::Point(1.f, .3f, .4f));
    probe->AddPoint(iGame::Point(1.0005f, .3f, .4f));
    auto filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(mesh);
    filter->SetProbeData(probe);
    Check(filter->Execute(), "trilinear hex execution");
    auto output = filter->GetResampledData();
    if (!output) { return; }
    auto result = FindPointArray(output->GetAttributeSet(), "xyz", nullptr);
    Check(result != nullptr, "trilinear xyz array");
    if (result) {
        CheckNear(result->GetElementValue(0, 0), .024, 1e-6, "off-center xyz reproduction");
        CheckNear(result->GetElementValue(1, 0), .12, 1e-6, "face xyz reproduction");
        CheckNear(result->GetElementValue(2, 0), 0., 1e-6, "outside value cleared");
    }
    const auto& mask = filter->GetSampleValidMask();
    Check(mask.size() == 3 && mask[0] && mask[1] && !mask[2], "hex containment including outside point");
}

} // namespace

int main() {
    std::cout << "==== ResampleWithDataSet 自检 ====" << std::endl;
    std::cout << "测试网格：" << ResolveTestMeshPath() << std::endl;

    // 先确认测试网格可读，避免后续用例在空指针上崩溃
    if (MakeUnitHexahedron() == nullptr) {
        std::cout << "\n==== 结果：测试网格不可用 ====" << std::endl;
        return 1;
    }

    TestSelfProbeOnMesh();
    TestPointCloudProbe();
    TestSnappingRadius();
    TestErrorPaths();
    TestTrilinearHexahedron();

    std::cout << "\n==== 结果：" << (g_Failed == 0 ? "全部通过" : std::to_string(g_Failed) + " 项失败") << " ===="
              << std::endl;
    return g_Failed == 0 ? 0 : 1;
}
