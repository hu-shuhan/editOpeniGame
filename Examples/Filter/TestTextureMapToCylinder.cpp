// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
#include <TextureMapToCylinder/iGameTextureMapToCylinderFilter.h>

#include <Core/iGameScene.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iGameAttributeSet.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameCellType.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <string>
#include <vector>

// 简单任务配套测试用例：按圆柱面生成 2D 纹理坐标
// 运行：cd Examples && ./testTextureMapToCylinder
//
// 测试模型：TextureMapToCylinder_tube.vtk
//   半径 0.5、轴向 z = -1 / 0 / +1 各 8 个点的圆管（24 点 / 16 个四边形），
//   并带有一份 Point Data（p_val）与 Cell Data（c_val），用于验证"其余属性不受影响"。
//
// 期望值（按使用说明第 1.1 节的公式手算）：
//   - 参考方向 ref = +X（轴为 ±Z 时都是 +X，与轴的正负无关）；
//   - t：z = -1 / 0 / +1 三圈分别为 0 / 0.5 / 1（或整体反向 1 / 0.5 / 0，
//        自动求轴时轴的正负由主轴特征向量的符号决定，不保证朝向）；
//   - s（PreventSeam = true，默认）：绕轴一圈按角度 0°,45°,…,315° 对应
//        0, 0.25, 0.5, 0.75, 1.0, 0.75, 0.5, 0.25（绕到 180° 后折返，避免接缝）；
//   - s（PreventSeam = false）：0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875（整圈 0→1）。

namespace {

int g_failed = 0;

void Check(bool ok, const std::string& what) {
    std::cerr << (ok ? "  [ ok ] " : " [FAIL] ") << what << "\n";
    if (!ok) { ++g_failed; }
}

iGame::UnstructuredMesh::Pointer LoadMesh(const std::string& fileName) {
    if (!std::filesystem::exists(fileName)) {
        std::cerr << " [FAIL] model not found: " << fileName << "\n";
        ++g_failed;
        return nullptr;
    }
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << " [FAIL] ReadFile returned null: " << fileName << "\n";
        ++g_failed;
        return nullptr;
    }
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cerr << " [FAIL] not an UnstructuredMesh: " << fileName << "\n";
        ++g_failed;
    }
    return mesh;
}

/// 读文件但保留原始类型：POLYDATA 模型会被读成表面网格（面表 + 边表），
/// 不能像无结构网格那样直接取
iGame::DataObject::Pointer LoadAny(const std::string& fileName) {
    if (!std::filesystem::exists(fileName)) {
        std::cerr << " [FAIL] model not found: " << fileName << "\n";
        ++g_failed;
        return nullptr;
    }
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << " [FAIL] ReadFile returned null: " << fileName << "\n";
        ++g_failed;
    }
    return obj;
}

/// 按名字 + 挂载位置（IG_POINT / IG_CELL）查属性数组
iGame::ArrayObject::Pointer FindArray(iGame::DataObject::Pointer obj, const std::string& name,
                                      IGenum attachment) {
    if (obj == nullptr) { return nullptr; }
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return nullptr; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return nullptr; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachment) { continue; }
        if (std::string(attr.pointer->GetName()) == name) { return attr.pointer; }
    }
    return nullptr;
}

/// 取第 i 个元组的第 comp 个分量
double Comp(iGame::ArrayObject::Pointer array, IGsize i, int comp) {
    if (array == nullptr) { return -999.0; }
    return array->GetElementValue(i, comp);
}

bool Near(double a, double b, double tol = 1e-6) { return std::fabs(a - b) <= tol; }

/// 统计"名为 name 的点数组"出现次数（验证重复执行不堆积）
int CountArrays(iGame::DataObject::Pointer obj, const std::string& name, IGenum attachment) {
    if (obj == nullptr) { return 0; }
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return 0; }
    auto all = attrs->GetAllAttributes();
    int n = 0;
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachment) { continue; }
        if (std::string(attr.pointer->GetName()) == name) { ++n; }
    }
    return n;
}

/// 场景 1：默认参数（自动求轴 + PreventSeam）
void TestDefaultCylinder() {
    std::cerr << "[case 1] tube model, automatic axis + PreventSeam (default)\n";
    auto mesh = LoadMesh("./Models/TextureMapToCylinder_tube.vtk");
    if (mesh == nullptr) { return; }
    Check(mesh->GetNumberOfPoints() == 24 && mesh->GetNumberOfCells() == 16,
          "input: 24 points / 16 quads");

    auto filter = iGame::TextureMapToCylinderFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true (message: " + filter->GetMessage() + ")");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out != nullptr, "GetOutput() is an UnstructuredMesh");
    if (out == nullptr) { return; }

    Check(out.GetPointer() != mesh.GetPointer(), "output is independent from the input");
    Check(out->GetNumberOfPoints() == 24 && out->GetNumberOfCells() == 16,
          "geometry/topology preserved (24 points / 16 cells)");

    // 纹理坐标数组：名字/挂载/分量数
    auto tc = FindArray(out, "Texture Coordinates", IG_POINT);
    Check(tc != nullptr, "Point Data 'Texture Coordinates' is created");
    if (tc == nullptr) { return; }
    Check(tc->GetDimension() == 2, "texture coordinates have 2 components");
    Check(static_cast<IGsize>(tc->GetNumberOfElements()) == 24,
          "texture coordinate tuple count == point count (24)");

    // 其余属性不受影响
    auto pv = FindArray(out, "p_val", IG_POINT);
    auto cv = FindArray(out, "c_val", IG_CELL);
    Check(pv != nullptr && static_cast<IGsize>(pv->GetNumberOfElements()) == 24,
          "existing Point Data 'p_val' preserved (24)");
    Check(cv != nullptr && static_cast<IGsize>(cv->GetNumberOfElements()) == 16,
          "existing Cell Data 'c_val' preserved (16)");
    Check(FindArray(mesh, "Texture Coordinates", IG_POINT) == nullptr,
          "input mesh is NOT modified (no texture coordinates on it)");

    // 取值范围 [0,1]
    bool inRange = true;
    for (IGsize i = 0; i < 24; ++i) {
        const double s = Comp(tc, i, 0);
        const double t = Comp(tc, i, 1);
        if (s < -1e-9 || s > 1.0 + 1e-9 || t < -1e-9 || t > 1.0 + 1e-9) { inRange = false; }
    }
    Check(inRange, "all (s,t) are within [0,1]");

    // t：三圈分别是 0 / 0.5 / 1（允许整体反向，见文件头说明）
    const double t0 = Comp(tc, 0, 1);
    const double t1 = Comp(tc, 8, 1);
    const double t2 = Comp(tc, 16, 1);
    const bool forward = Near(t0, 0.0) && Near(t1, 0.5) && Near(t2, 1.0);
    const bool backward = Near(t0, 1.0) && Near(t1, 0.5) && Near(t2, 0.0);
    Check(forward || backward,
          "t == 0/0.5/1 on the three rings (got " + std::to_string(t0) + "/" +
              std::to_string(t1) + "/" + std::to_string(t2) + ")");

    // s：绕轴一圈按 45° 步长，PreventSeam 下为 0,0.25,0.5,0.75,1,0.75,0.5,0.25
    const double expectS[8] = {0.0, 0.25, 0.5, 0.75, 1.0, 0.75, 0.5, 0.25};
    bool sOK = true;
    for (int k = 0; k < 8; ++k) {
        if (!Near(Comp(tc, k, 0), expectS[k], 1e-5)) { sOK = false; }
    }
    Check(sOK, "s matches the seam-prevented formula (0, .25, .5, .75, 1, .75, .5, .25)");

    // 自动求轴的结果应当就是 Z 轴的两个端点（x、y 为 0，|z| = 1）
    double a1[3] = {0, 0, 0};
    double a2[3] = {0, 0, 0};
    filter->GetResolvedAxis(a1, a2);
    Check(Near(a1[0], 0.0, 1e-5) && Near(a1[1], 0.0, 1e-5) && Near(a2[0], 0.0, 1e-5) &&
              Near(a2[1], 0.0, 1e-5),
          "resolved axis lies on the Z axis (x == y == 0)");
    Check(Near(std::fabs(a1[2]), 1.0, 1e-5) && Near(std::fabs(a2[2]), 1.0, 1e-5),
          "resolved axis endpoints at z = -1 and z = +1 (got " + std::to_string(a1[2]) +
              " and " + std::to_string(a2[2]) + ")");
}

/// 场景 2：PreventSeam = false（整圈 0→1）
void TestWithoutPreventSeam() {
    std::cerr << "[case 2] PreventSeam off -> s covers the full circle 0..1\n";
    auto mesh = LoadMesh("./Models/TextureMapToCylinder_tube.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::TextureMapToCylinderFilter::New();
    filter->SetPreventSeam(false);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    auto tc = FindArray(out, "Texture Coordinates", IG_POINT);
    Check(tc != nullptr, "Point Data 'Texture Coordinates' is created");
    if (tc == nullptr) { return; }

    const double expectS[8] = {0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875};

    // 自动求轴时轴的正负不确定（主轴特征向量的符号决定，不保证朝向），s 的绕行方向随之可能相反，
    // 因此这里只校验"这 8 个值恰好是 0,0.125,…,0.875 的一个排列"（绕行一整圈且不重复）。
    // 自动求轴时轴的正负不确定（不保证朝向），s 的绕行方向随之可能相反；
    // 另外落在 0° 上的点会因为浮点零符号被翻到 1.0（与 0.0 在纹理上等价）。
    // 因此这里改成校验"s 恰好绕圈一次"：8 个点分别落在 1/8 的 8 个格点上且互不重复。
    std::vector<int> buckets;
    bool gridOK = true;
    for (int k = 0; k < 8; ++k) {
        const double v = Comp(tc, k, 0) * 8.0;
        const long rounded = std::lround(v);
        if (std::fabs(v - static_cast<double>(rounded)) > 1e-4) { gridOK = false; }
        buckets.push_back(static_cast<int>(((rounded % 8) + 8) % 8));
    }
    std::sort(buckets.begin(), buckets.end());
    const bool distinct = std::adjacent_find(buckets.begin(), buckets.end()) == buckets.end();
    Check(gridOK && distinct,
          "s covers the circle exactly once (8 distinct multiples of 1/8; s=1 is the same as s=0)");

    // 手动指定轴后结果完全确定：按逆时针 45° 步长依次 0, .125, ..., .875
    auto filter2 = iGame::TextureMapToCylinderFilter::New();
    filter2->SetAutomaticCylinderGeneration(false);
    filter2->SetPoint1(0.0, 0.0, -1.0);
    filter2->SetPoint2(0.0, 0.0, 1.0);
    filter2->SetPreventSeam(false);
    filter2->SetInput(mesh);
    Check(filter2->Execute(), "manual axis: Execute() returns true");
    auto out2 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter2->GetOutput());
    auto tc2 = FindArray(out2, "Texture Coordinates", IG_POINT);
    bool exactOK = (tc2 != nullptr);
    if (tc2 != nullptr) {
        for (int k = 0; k < 8; ++k) {
            if (!Near(Comp(tc2, k, 0), expectS[k], 1e-5)) { exactOK = false; }
        }
    }
    Check(exactOK, "manual axis: s == angle/360 for 0,45,...,315 degrees");
}

/// 场景 3：手动指定轴（结果完全确定）—— 逐点核对 s 与 t
void TestManualAxis() {
    std::cerr << "[case 3] manual axis (0,0,-1) -> (0,0,1): exact s and t\n";

    // 不手动设轴、也不开自动时的默认轴（本实现取覆盖常见单位模型 ±1 的长度）
    {
        auto fresh = iGame::TextureMapToCylinderFilter::New();
        const double* d1 = fresh->GetPoint1();
        const double* d2 = fresh->GetPoint2();
        Check(Near(d1[0], 0.0) && Near(d1[1], 0.0) && Near(d1[2], -1.0) && Near(d2[0], 0.0) &&
                  Near(d2[1], 0.0) && Near(d2[2], 1.0),
              "default axis is (0,0,-1) -> (0,0,1)");
    }

    auto mesh = LoadMesh("./Models/TextureMapToCylinder_tube.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::TextureMapToCylinderFilter::New();
    filter->SetAutomaticCylinderGeneration(false);
    filter->SetPoint1(0.0, 0.0, -1.0);
    filter->SetPoint2(0.0, 0.0, 1.0);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    auto tc = FindArray(out, "Texture Coordinates", IG_POINT);
    if (tc == nullptr) {
        Check(false, "Point Data 'Texture Coordinates' is created");
        return;
    }

    // t：Point1 处为 0、Point2 处为 1，中间为 0.5
    Check(Near(Comp(tc, 0, 1), 0.0, 1e-6) && Near(Comp(tc, 8, 1), 0.5, 1e-6) &&
              Near(Comp(tc, 16, 1), 1.0, 1e-6),
          "t == 0 / 0.5 / 1 for z = -1 / 0 / +1 (Point1 -> Point2)");

    // s：三圈的值应当完全一致（同一角度 → 同一 s）
    const double expectS[8] = {0.0, 0.25, 0.5, 0.75, 1.0, 0.75, 0.5, 0.25};
    bool sOK = true;
    for (int ring = 0; ring < 3; ++ring) {
        for (int k = 0; k < 8; ++k) {
            if (!Near(Comp(tc, ring * 8 + k, 0), expectS[k], 1e-5)) { sOK = false; }
        }
    }
    Check(sOK, "same angle -> same s on every ring (seam-prevented formula)");
}

/// 场景 4：失败要能被识别（轴退化 / 没有点），不要返回原模型
void TestFailureCases() {
    std::cerr << "[case 4] failure reporting (degenerate axis / no points)\n";

    auto mesh = LoadMesh("./Models/TextureMapToCylinder_tube.vtk");
    if (mesh != nullptr) {
        auto filter = iGame::TextureMapToCylinderFilter::New();
        filter->SetAutomaticCylinderGeneration(false);
        filter->SetPoint1(1.0, 2.0, 3.0);
        filter->SetPoint2(1.0, 2.0, 3.0);  // 两个端点重合 → 轴退化
        filter->SetInput(mesh);
        const bool ok = filter->Execute();
        Check(!ok, "degenerate axis -> Execute() returns false");
        Check(!filter->GetMessage().empty(), "failure message is provided: " + filter->GetMessage());
    }

    // 0 个点的空网格：明确失败，绝不返回原模型
    auto empty = iGame::UnstructuredMesh::New();
    auto filter2 = iGame::TextureMapToCylinderFilter::New();
    filter2->SetInput(empty);
    const bool ok2 = filter2->Execute();
    Check(!ok2, "mesh without points -> Execute() returns false");
    Check(filter2->GetMessage().find("no points") != std::string::npos,
          "message explains the reason: " + filter2->GetMessage());
}

/// 场景 5：重复执行不堆积同名数组
void TestRepeatedExecution() {
    std::cerr << "[case 5] repeated execution keeps a single result array\n";
    auto mesh = LoadMesh("./Models/TextureMapToCylinder_tube.vtk");
    if (mesh == nullptr) { return; }
    auto filter = iGame::TextureMapToCylinderFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "first Execute() returns true");
    auto out1 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out1 != nullptr && CountArrays(out1, "Texture Coordinates", IG_POINT) == 1,
          "exactly one 'Texture Coordinates' array after the first run");
    Check(filter->Execute(), "second Execute() returns true");
    auto out2 = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out2 != nullptr && CountArrays(out2, "Texture Coordinates", IG_POINT) == 1,
          "still exactly one 'Texture Coordinates' array after the second run");
    Check(mesh->GetAttributeSet() != nullptr &&
              CountArrays(mesh, "Texture Coordinates", IG_POINT) == 0,
          "input mesh still untouched after two runs");
}

/// 场景 6：表面网格里的独立线段必须被保留
/// （本过滤器只追加纹理坐标，不应该改变输入的几何）
void TestLooseEdgesPreserved() {
    std::cerr << "[case 6] surface mesh with a standalone line keeps all geometry\n";

    auto obj = LoadAny("./Models/TextureMapToCylinder_loose_line.vtk");
    if (obj == nullptr) { return; }
    auto sm = iGame::DynamicCast<iGame::SurfaceMesh>(obj);
    Check(sm != nullptr, "input is read as a SurfaceMesh (separate face / edge tables)");
    if (sm == nullptr) { return; }

    // 模型内容：5 个点、1 个三角面 + 1 条不属于任何面的独立线段
    Check(sm->GetNumberOfPoints() == 5, "input: 5 points");
    Check(static_cast<IGsize>(sm->GetNumberOfFaces()) == 1, "input: 1 face");
    Check(static_cast<IGsize>(sm->GetNumberOfEdges()) == 1, "input: 1 standalone edge");

    auto filter = iGame::TextureMapToCylinderFilter::New();
    filter->SetAutomaticCylinderGeneration(false);
    filter->SetPoint1(0.0, 0.0, -1.0);
    filter->SetPoint2(0.0, 0.0, 1.0);
    filter->SetInput(obj);
    Check(filter->Execute(), "Execute() returns true (message: " + filter->GetMessage() + ")");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    Check(out->GetNumberOfPoints() == 5, "output keeps all 5 points");
    Check(out->GetNumberOfCells() == 2,
          "output keeps BOTH cells (triangle + standalone line), got " +
              std::to_string(out->GetNumberOfCells()));

    // 两个单元应当正好是一个三角形 + 一条线段
    auto types = out->GetCellTypes();
    int nTri = 0, nLine = 0;
    for (IGsize i = 0; i < out->GetNumberOfCells(); ++i) {
        const unsigned int t = static_cast<unsigned int>(types->GetValue(i));
        if (t == iGame::IG_TRIANGLE) { ++nTri; }
        else if (t == iGame::IG_LINE) { ++nLine; }
    }
    Check(nTri == 1 && nLine == 1,
          "one triangle and one line cell (tri=" + std::to_string(nTri) + ", line=" +
              std::to_string(nLine) + ")");

    // 纹理坐标覆盖每个点，已有属性照旧
    auto tc = FindArray(out, "Texture Coordinates", IG_POINT);
    Check(tc != nullptr && static_cast<IGsize>(tc->GetNumberOfElements()) == 5,
          "texture coordinates created for all 5 points");
    auto pv = FindArray(out, "p_val", IG_POINT);
    Check(pv != nullptr && static_cast<IGsize>(pv->GetNumberOfElements()) == 5,
          "existing Point Data 'p_val' preserved (5)");
}

/// 场景 7：很短的轴不该被误判成退化
/// （轴长只决定 t 的量程，不影响能不能建立参考方向，也不影响 s）
void TestShortAxisNotDegenerate() {
    std::cerr << "[case 7] very short axis (1e-6) is not treated as degenerate\n";
    auto mesh = LoadMesh("./Models/TextureMapToCylinder_tube.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::TextureMapToCylinderFilter::New();
    filter->SetAutomaticCylinderGeneration(false);
    filter->SetPoint1(0.0, 0.0, 0.0);
    filter->SetPoint2(0.0, 0.0, 1e-6);  // 两端点不同，轴长仅 1e-6
    filter->SetInput(mesh);
    const bool ok = filter->Execute();
    Check(ok, "short axis still executes (message: " + filter->GetMessage() + ")");
    if (!ok) { return; }

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    auto tc = FindArray(out, "Texture Coordinates", IG_POINT);
    Check(tc != nullptr, "texture coordinates are produced");
    if (tc == nullptr) { return; }

    // s 只取决于绕轴的角度，与轴的绝对长短无关：应与长轴给出同一组值
    const double expectS[8] = {0.0, 0.25, 0.5, 0.75, 1.0, 0.75, 0.5, 0.25};
    bool sOK = true;
    for (int k = 0; k < 8; ++k) {
        if (!Near(Comp(tc, k, 0), expectS[k], 1e-5)) { sOK = false; }
    }
    Check(sOK, "s is unaffected by the axis length (same as the long axis)");
}

/// 可视化演示：弹出圆柱管模型（纹理坐标本身在 iGameVis 里暂时没有贴图渲染通路，
/// 因此这里只做几何展示；数值验证见上面的断言）
void VisualizeResult() {
    if (std::getenv("IGV_TEST_NO_VIEW") != nullptr) { return; }
    auto mesh = LoadMesh("./Models/TextureMapToCylinder_tube.vtk");
    if (mesh == nullptr) { return; }
    auto filter = iGame::TextureMapToCylinderFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) { return; }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) { return; }

    auto scene = iGame::Scene::New();
    scene->AddModel(out);
    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
}

}  // namespace

int main() {
    std::cerr << "==== testTextureMapToCylinder ====\n";
    TestDefaultCylinder();
    TestWithoutPreventSeam();
    TestManualAxis();
    TestFailureCases();
    TestRepeatedExecution();
    TestLooseEdgesPreserved();
    TestShortAxisNotDegenerate();

    if (g_failed == 0) {
        std::cerr << "[testTextureMapToCylinder] PASS: all checks passed\n";
    } else {
        std::cerr << "[testTextureMapToCylinder] FAIL: " << g_failed << " check(s) failed\n";
    }

    VisualizeResult();
    return (g_failed == 0) ? 0 : 1;
}
