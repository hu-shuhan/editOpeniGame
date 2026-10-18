// 集成回归（待提交）：原标准菜单缺少本过滤器实现；保留源仓库的数值、类型及错误输入断言，
// 用于保证迁移后结果和边界行为一致，防止仅有菜单入口而算法不可用。
// 生成表面切向量（GenerateSurfaceTangents / generate_surface_tangents）测试：
//   1. 单个三角形：uv 恒等映射 -> T = +x；uv 交换 -> T = +y（手算）
//   2. 累加权重：大三角形(+x) + 小三角形(+y) 共享一个点
//      -> 等权累加 = (0.99995, 0.01, 0)，和 ParaView 的 vtkPolyDataTangents 一致
//   3. 不规则 16 点网格 -> 与 ParaView 6.2 的 vtkPolyDataTangents 逐点一致（1e-5）
//   4. 四边形网格（本 filter 支持，内部扇形三角化）
//   5. 纹理坐标退化（同一个三角形的 uv 共线）时不会出现 NaN / Inf
//   6. 副切向量（本 filter 的扩展输出）
//   7. 非结构网格 / 混合单元 / 孤立点
//   8. 独立输出：输入不被修改
//   9. 错误输入：没有纹理坐标、指定了不存在的数组名、点集、体网格
//  10. 演示模型 Examples/Models/SurfaceTangents.vtk（三角化圆柱面，39 点 / 48 三角形）
#include <GenerateSurfaceTangents/iGameGenerateSurfaceTangentsFilter.h>

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{

bool Check(bool ok, const std::string& name) {
    if (ok) {
        std::cout << "[PASS] " << name << std::endl;
    } else {
        std::cout << "[FAIL] " << name << std::endl;
    }
    return ok;
}

void Step(const std::string& name) { std::cout << "  >> " << name << std::endl; }

bool Near(double value, double expected, double tolerance = 1.0e-5) {
    return std::fabs(value - expected) < tolerance;
}

bool FileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

std::string FindModel() {
    const std::string candidates[] = {
            "./Models/SurfaceTangents.vtk",
            "./Examples/Models/SurfaceTangents.vtk",
            "../Examples/Models/SurfaceTangents.vtk",
    };
    for (const auto& c: candidates) {
        if (FileExists(c)) { return c; }
    }
    return "";
}

// 取输出上的数组（要求名字 + 附着类型 + 分量数都对得上）
iGame::ArrayObject::Pointer GetArray(iGame::DataObject::Pointer obj, const std::string& name,
                                     IGenum attachment, int dimension) {
    if (obj.IsNull()) { return nullptr; }
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return nullptr; }
    const IGsize count = static_cast<IGsize>(attrs->GetNumberOfAttributes());
    for (IGsize i = 0; i < count; i++) {
        auto& attribute = attrs->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer.IsNull()) { continue; }
        if (attribute.attachmentType != attachment) { continue; }
        if (attribute.pointer->GetName() != name) { continue; }
        if (dimension > 0 && attribute.pointer->GetDimension() != dimension) { continue; }
        return attribute.pointer;
    }
    return nullptr;
}

void GetVector(iGame::ArrayObject::Pointer array, IGsize index, double out[3]) {
    out[0] = array->GetElementValue(index, 0);
    out[1] = array->GetElementValue(index, 1);
    out[2] = array->GetElementValue(index, 2);
}

double VectorLength(const double v[3]) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

double Dot(const double a[3], const double b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

bool IsFiniteVector(const double v[3]) {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}

bool ExpectVector(iGame::ArrayObject::Pointer array, IGsize index, const double expected[3],
                  const std::string& name, double tolerance = 1.0e-5) {
    double value[3]{};
    GetVector(array, index, value);
    std::cout << "    [" << index << "] = (" << value[0] << ", " << value[1] << ", " << value[2] << ")"
              << "  expect (" << expected[0] << ", " << expected[1] << ", " << expected[2] << ")" << std::endl;
    return Check(IsFiniteVector(value) && Near(value[0], expected[0], tolerance) &&
                         Near(value[1], expected[1], tolerance) && Near(value[2], expected[2], tolerance),
                 name);
}

// 给网格挂一份纹理坐标点属性（IG_TCOORD，2 个分量）
template<typename MeshPointer>
void AddTextureCoordinates(const MeshPointer& mesh, const std::vector<std::pair<double, double>>& uv,
                           const std::string& name = "TCoords") {
    auto array = iGame::FloatArray::New();
    array->SetDimension(2);
    array->SetName(name);
    array->Resize(uv.size());
    for (size_t i = 0; i < uv.size(); i++) {
        array->SetValue(2 * i + 0, uv[i].first);
        array->SetValue(2 * i + 1, uv[i].second);
    }
    mesh->GetAttributeSet()->AddAttribute(IG_TCOORD, IG_POINT, array);
}

// 三点的三角形网格（点 + uv 由调用者给）
iGame::SurfaceMesh::Pointer MakeTriangleMesh(const std::vector<iGame::Point>& points,
                                             const std::vector<std::pair<double, double>>& uv) {
    auto mesh = iGame::SurfaceMesh::New();
    mesh->SetName("Triangle");
    for (const auto& p: points) { mesh->AddPoint(p); }
    auto faces = iGame::CellArray::New();
    mesh->SetFaces(faces);
    igIndex ids[3] = {0, 1, 2};
    faces->AddCellIds(ids, 3);
    AddTextureCoordinates(mesh, uv);
    return mesh;
}

// 场景 1：单个三角形的手算结果
bool TestSingleTriangle() {
    std::cout << "\n== Test 1: single triangle (hand computed) ==" << std::endl;
    bool ok = true;

    // uv 恒等：(u, v) = (x, y) -> T = +x
    {
        Step("uv identity -> T = +x");
        auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                      iGame::Point(0.0f, 1.0f, 0.0f)},
                                     {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});
        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "filter Execute()")) { return false; }

        auto out = filter->GetOutput();
        auto tangents = GetArray(out, "Tangents", IG_POINT, 3);
        if (!Check(!tangents.IsNull(), "output has point array \"Tangents\" (3 components)")) { return false; }
        const double expected[3] = {1.0, 0.0, 0.0};
        for (IGsize i = 0; i < 3; i++) {
            ok &= ExpectVector(tangents, i, expected, "point " + std::to_string(i) + " tangent = (1, 0, 0)");
        }
        // 单元切向量默认不输出
        ok &= Check(GetArray(out, "Tangents", IG_CELL, 3).IsNull(), "cell \"Tangents\" is off by default");
    }

    // uv 交换：(u, v) = (y, x) -> T = +y
    {
        Step("uv swapped -> T = +y");
        auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                      iGame::Point(0.0f, 1.0f, 0.0f)},
                                     {{0.0, 0.0}, {0.0, 1.0}, {1.0, 0.0}});
        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "filter Execute()")) { return false; }
        auto tangents = GetArray(filter->GetOutput(), "Tangents", IG_POINT, 3);
        if (!Check(!tangents.IsNull(), "output has point array \"Tangents\"")) { return false; }
        const double expected[3] = {0.0, 1.0, 0.0};
        for (IGsize i = 0; i < 3; i++) {
            ok &= ExpectVector(tangents, i, expected, "point " + std::to_string(i) + " tangent = (0, 1, 0)");
        }
    }

    // 切向量的长度必须是 1
    {
        Step("tangents are unit length");
        auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(2.0f, 0.0f, 1.0f),
                                      iGame::Point(0.0f, 3.0f, 2.0f)},
                                     {{0.0, 0.0}, {2.0, 0.5}, {0.0, 3.0}});
        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "filter Execute()")) { return false; }
        auto tangents = GetArray(filter->GetOutput(), "Tangents", IG_POINT, 3);
        if (!Check(!tangents.IsNull(), "output has point array \"Tangents\"")) { return false; }
        for (IGsize i = 0; i < 3; i++) {
            double value[3]{};
            GetVector(tangents, i, value);
            ok &= Check(Near(VectorLength(value), 1.0), "point " + std::to_string(i) + " |T| = 1");
        }
    }
    return ok;
}

// 场景 2：大三角形(+x, 面积 0.5) 与小三角形(+y, 面积 0.005) 共享同一个点
//   等权累加 -> (1, 0.01, 0) 归一化 = (0.99995, 0.01, 0)
//   面积加权 -> (1, 0.0001, 0) 归一化 = (1, 0.0001, 0)
// ParaView 6.2 的 vtkPolyDataTangents 给的是前者（等权），所以这个用例能区分两种写法。
bool TestAccumulationWeightMatchesParaView() {
    std::cout << "\n== Test 2: accumulation weight (equal weight, same as ParaView) ==" << std::endl;

    auto mesh = iGame::SurfaceMesh::New();
    mesh->SetName("BigPlusX_SmallPlusY");
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));  // 0：共享点
    mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));  // 1
    mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));  // 2
    mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));  // 3（和 1 重合）
    mesh->AddPoint(iGame::Point(0.0f, 0.01f, 0.0f)); // 4
    auto faces = iGame::CellArray::New();
    mesh->SetFaces(faces);
    igIndex big[3] = {0, 1, 2};
    igIndex tiny[3] = {0, 3, 4};
    faces->AddCellIds(big, 3);
    faces->AddCellIds(tiny, 3);
    AddTextureCoordinates(mesh, {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}, {1.0, 0.0}});

    auto filter = iGame::GenerateSurfaceTangentsFilter::New();
    filter->SetComputeCellTangents(true); // 顺便检查单元切向量
    filter->SetInput(0, mesh);
    if (!Check(filter->Execute(), "filter Execute()")) { return false; }

    auto out = filter->GetOutput();
    auto tangents = GetArray(out, "Tangents", IG_POINT, 3);
    if (!Check(!tangents.IsNull(), "output has point array \"Tangents\"")) { return false; }

    bool ok = true;
    const double expectedShared[3] = {0.9999499917030334, 0.009999999776482582, 0.0};
    ok &= ExpectVector(tangents, 0, expectedShared,
                       "shared point = (0.99995, 0.01, 0) -> equal weight (ParaView)", 1.0e-5);
    const double expectedX[3] = {1.0, 0.0, 0.0};
    const double expectedTiny[3] = {0.0, 1.0, 0.0};
    ok &= ExpectVector(tangents, 1, expectedX, "point 1 (only in the big triangle) = (1, 0, 0)");
    ok &= ExpectVector(tangents, 4, expectedTiny, "point 4 (only in the tiny triangle) = (0, 1, 0)");

    // 单元切向量：三角形的原始 dP/du，不归一化（和 VTK 一样）
    auto cellTangents = GetArray(out, "Tangents", IG_CELL, 3);
    if (!Check(!cellTangents.IsNull(), "output has cell array \"Tangents\" (ComputeCellTangents = true)")) {
        return false;
    }
    const double expectedCell0[3] = {1.0, 0.0, 0.0};
    const double expectedCell1[3] = {0.0, 0.01, 0.0};
    ok &= ExpectVector(cellTangents, 0, expectedCell0, "cell 0 raw dP/du = (1, 0, 0)");
    ok &= ExpectVector(cellTangents, 1, expectedCell1, "cell 1 raw dP/du = (0, 0.01, 0), not normalized");
    return ok;
}

// 场景 3：不规则 16 点网格（4x4 非均匀网格 + 非线性 uv），
const double IRREGULAR_POINTS[16][3] = {
        {0.0, 0.0, 0.0}, {1.3, 0.0, 0.0}, {2.1, 0.0, 0.0}, {3.0, 0.0, 0.0},
        {0.0, 0.8, 0.0}, {1.3, 0.8, 0.0}, {2.1, 0.8, 0.0}, {3.0, 0.8, 0.0},
        {0.0, 2.4, 0.0}, {1.3, 2.4, 0.0}, {2.1, 2.4, 0.0}, {3.0, 2.4, 0.0},
        {0.0, 3.0, 0.0}, {1.3, 3.0, 0.0}, {2.1, 3.0, 0.0}, {3.0, 3.0, 0.0},
};
const double IRREGULAR_UV[16][2] = {
        {0.0, 0.0},           {1.3, 0.676},          {2.1, 1.764},          {3.0, 3.6},
        {0.256, 0.8},         {1.556, 1.476},        {2.356, 2.564},        {3.256, 4.4},
        {2.304, 2.4},         {3.604, 3.076},        {4.404, 4.164},        {5.304, 6.0},
        {3.6, 3.0},           {4.9, 3.676},          {5.7, 4.764},          {6.6, 6.6},
};
const int IRREGULAR_TRIANGLES[18][3] = {
        {0, 1, 5},   {0, 5, 4},   {1, 2, 6},   {1, 6, 5},   {2, 3, 7},   {2, 7, 6},
        {4, 5, 9},   {4, 9, 8},   {5, 6, 10},  {5, 10, 9},  {6, 7, 11},  {6, 11, 10},
        {8, 9, 13},  {8, 13, 12}, {9, 10, 14}, {9, 14, 13}, {10, 11, 15}, {10, 15, 14},
};
// ParaView 6.2 / vtkPolyDataTangents 的输出
const double IRREGULAR_TANGENTS[16][3] = {
        {0.8872168064117432, -0.46135270595550537, 0.0},
        {0.6570107936859131, -0.7538811564445496, 0.0},
        {0.46958768367767334, -0.8828858137130737, 0.0},
        {0.44015738368034363, -0.8979206681251526, 0.0},
        {0.8872168064117432, -0.46135273575782776, 0.0},
        {0.9453222751617432, -0.3261377215385437, 0.0},
        {0.5180785655975342, -0.8553330302238464, 0.0},
        {0.4401572644710541, -0.8979207277297974, 0.0},
        {-0.8872168660163879, 0.4613526165485382, 0.0},
        {-0.7203536033630371, 0.693606972694397, 0.0},
        {-0.5432006120681763, 0.8396030068397522, 0.0},
        {-0.4401572048664093, 0.8979207277297974, 0.0},
        {-0.8872167468070984, 0.46135276556015015, 0.0},
        {-0.8777376413345337, 0.47914138436317444, 0.0},
        {-0.552003800868988, 0.8338415622711182, 0.0},
        {-0.4401572644710541, 0.8979207277297974, 0.0},
};

bool TestIrregularGridMatchesParaView() {
    std::cout << "\n== Test 3: irregular 16 point grid (reference values from ParaView 6.2) ==" << std::endl;

    auto mesh = iGame::SurfaceMesh::New();
    mesh->SetName("IrregularGrid");
    std::vector<std::pair<double, double>> uv;
    for (int i = 0; i < 16; i++) {
        mesh->AddPoint(iGame::Point(static_cast<float>(IRREGULAR_POINTS[i][0]),
                                    static_cast<float>(IRREGULAR_POINTS[i][1]),
                                    static_cast<float>(IRREGULAR_POINTS[i][2])));
        uv.emplace_back(IRREGULAR_UV[i][0], IRREGULAR_UV[i][1]);
    }
    auto faces = iGame::CellArray::New();
    mesh->SetFaces(faces);
    for (int t = 0; t < 18; t++) {
        igIndex ids[3] = {IRREGULAR_TRIANGLES[t][0], IRREGULAR_TRIANGLES[t][1], IRREGULAR_TRIANGLES[t][2]};
        faces->AddCellIds(ids, 3);
    }
    AddTextureCoordinates(mesh, uv);

    auto filter = iGame::GenerateSurfaceTangentsFilter::New();
    filter->SetInput(0, mesh);
    if (!Check(filter->Execute(), "filter Execute()")) { return false; }

    auto tangents = GetArray(filter->GetOutput(), "Tangents", IG_POINT, 3);
    if (!Check(!tangents.IsNull(), "output has point array \"Tangents\"")) { return false; }
    if (!Check(tangents->GetNumberOfElements() == 16, "Tangents has 16 elements")) { return false; }

    bool ok = true;
    double maxDifference = 0.0;
    for (IGsize i = 0; i < 16; i++) {
        double value[3]{};
        GetVector(tangents, i, value);
        for (int k = 0; k < 3; k++) {
            maxDifference = std::max(maxDifference, std::fabs(value[k] - IRREGULAR_TANGENTS[i][k]));
        }
    }
    std::cout << "    max |our - ParaView| = " << maxDifference << "  (float32 参考值，容差 1e-5)"
              << std::endl;
    ok &= Check(maxDifference < 1.0e-5, "all 16 points match ParaView vtkPolyDataTangents (1e-5)");

    // 单独把第 0 个点打印出来，方便和 ParaView 对照
    const double expected0[3] = {IRREGULAR_TANGENTS[0][0], IRREGULAR_TANGENTS[0][1], IRREGULAR_TANGENTS[0][2]};
    ok &= ExpectVector(tangents, 0, expected0, "point 0 = ParaView value");
    return ok;
}

// 场景 4：四边形网格（VTK 会直接报错，本 filter 内部扇形三角化）
bool TestQuadMesh() {
    std::cout << "\n== Test 4: quad mesh (fan triangulated, VTK would reject it) ==" << std::endl;

    auto mesh = iGame::SurfaceMesh::New();
    mesh->SetName("QuadGrid");
    std::vector<std::pair<double, double>> uv;
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            mesh->AddPoint(iGame::Point(static_cast<float>(i), static_cast<float>(j), 0.0f));
            // uv 交换：u 沿 y 方向 -> T = +y
            uv.emplace_back(static_cast<double>(j), static_cast<double>(i));
        }
    }
    auto faces = iGame::CellArray::New();
    mesh->SetFaces(faces);
    const igIndex quads[4][4] = {{0, 1, 4, 3}, {1, 2, 5, 4}, {3, 4, 7, 6}, {4, 5, 8, 7}};
    for (const auto& q: quads) {
        igIndex ids[4] = {q[0], q[1], q[2], q[3]};
        faces->AddCellIds(ids, 4);
    }
    AddTextureCoordinates(mesh, uv);

    auto filter = iGame::GenerateSurfaceTangentsFilter::New();
    filter->SetInput(0, mesh);
    if (!Check(filter->Execute(), "filter Execute() on a quad mesh")) { return false; }

    auto tangents = GetArray(filter->GetOutput(), "Tangents", IG_POINT, 3);
    if (!Check(!tangents.IsNull(), "output has point array \"Tangents\"")) { return false; }

    bool ok = true;
    const double expected[3] = {0.0, 1.0, 0.0};
    for (IGsize i = 0; i < 9; i++) {
        double value[3]{};
        GetVector(tangents, i, value);
        ok &= Check(Near(value[0], expected[0]) && Near(value[1], expected[1]) && Near(value[2], expected[2]),
                    "point " + std::to_string(i) + " tangent = (0, 1, 0)");
    }
    return ok;
}

// 场景 5：纹理坐标退化（uv 共线）不影响其它三角形，也不会出现 NaN / Inf
bool TestDegenerateTextureCoordinates() {
    std::cout << "\n== Test 5: degenerate texture coordinates (no NaN, unlike VTK) ==" << std::endl;

    // 三角形 0：正常；三角形 1：三个点的 uv 完全一样（den = 0）
    auto mesh = iGame::SurfaceMesh::New();
    mesh->SetName("DegenerateUV");
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
    mesh->AddPoint(iGame::Point(2.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 2.0f, 0.0f));
    auto faces = iGame::CellArray::New();
    mesh->SetFaces(faces);
    igIndex good[3] = {0, 1, 2};
    igIndex bad[3] = {0, 3, 4};
    faces->AddCellIds(good, 3);
    faces->AddCellIds(bad, 3);
    AddTextureCoordinates(mesh, {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {5.0, 5.0}, {5.0, 5.0}});

    auto filter = iGame::GenerateSurfaceTangentsFilter::New();
    filter->SetComputeCellTangents(true);
    filter->SetInput(0, mesh);
    if (!Check(filter->Execute(), "filter Execute() with one degenerate triangle")) { return false; }

    auto out = filter->GetOutput();
    auto tangents = GetArray(out, "Tangents", IG_POINT, 3);
    if (!Check(!tangents.IsNull(), "output has point array \"Tangents\"")) { return false; }

    bool ok = true;
    bool allFinite = true;
    for (IGsize i = 0; i < 5; i++) {
        double value[3]{};
        GetVector(tangents, i, value);
        allFinite &= IsFiniteVector(value);
    }
    ok &= Check(allFinite, "every tangent is finite (VTK would give NaN here)");

    const double expectedGood[3] = {1.0, 0.0, 0.0};
    const double expectedZero[3] = {0.0, 0.0, 0.0};
    ok &= ExpectVector(tangents, 0, expectedGood, "point 0 keeps the good triangle's tangent");
    ok &= ExpectVector(tangents, 1, expectedGood, "point 1 = (1, 0, 0)");
    ok &= ExpectVector(tangents, 3, expectedZero, "point 3 (degenerate uv only) = (0, 0, 0)");

    auto cellTangents = GetArray(out, "Tangents", IG_CELL, 3);
    if (Check(!cellTangents.IsNull(), "output has cell array \"Tangents\"")) {
        ok &= ExpectVector(cellTangents, 0, expectedGood, "cell 0 raw dP/du = (1, 0, 0)");
        ok &= ExpectVector(cellTangents, 1, expectedZero, "cell 1 raw dP/du = (0, 0, 0) (skipped)");
    } else {
        ok = false;
    }
    return ok;
}

// 场景 6：副切向量（本 filter 的扩展输出，默认关闭）
bool TestBitangents() {
    std::cout << "\n== Test 6: bitangents (extension) ==" << std::endl;
    bool ok = true;

    {
        Step("uv identity -> T = (1, 0, 0), B = (0, 1, 0)");
        auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                      iGame::Point(0.0f, 1.0f, 0.0f)},
                                     {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});
        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetComputeBitangents(true);
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "filter Execute()")) { return false; }

        auto out = filter->GetOutput();
        auto bitangents = GetArray(out, "Bitangents", IG_POINT, 3);
        if (!Check(!bitangents.IsNull(), "output has point array \"Bitangents\"")) { return false; }
        const double expected[3] = {0.0, 1.0, 0.0};
        for (IGsize i = 0; i < 3; i++) {
            ok &= ExpectVector(bitangents, i, expected, "point " + std::to_string(i) + " bitangent = (0, 1, 0)");
        }
    }

    {
        Step("uv swapped -> T = (0, 1, 0), B = (1, 0, 0)");
        auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                      iGame::Point(0.0f, 1.0f, 0.0f)},
                                     {{0.0, 0.0}, {0.0, 1.0}, {1.0, 0.0}});
        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetComputeBitangents(true);
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "filter Execute()")) { return false; }
        auto bitangents = GetArray(filter->GetOutput(), "Bitangents", IG_POINT, 3);
        if (!Check(!bitangents.IsNull(), "output has point array \"Bitangents\"")) { return false; }
        const double expected[3] = {1.0, 0.0, 0.0};
        for (IGsize i = 0; i < 3; i++) {
            ok &= ExpectVector(bitangents, i, expected, "point " + std::to_string(i) + " bitangent = (1, 0, 0)");
        }
    }

    {
        Step("bitangents are orthogonal to tangents");
        auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(2.0f, 1.0f, 0.5f),
                                      iGame::Point(-1.0f, 3.0f, 2.0f)},
                                     {{0.0, 0.0}, {2.0, 0.5}, {0.5, 3.0}});
        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetComputeBitangents(true);
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "filter Execute()")) { return false; }

        auto out = filter->GetOutput();
        auto tangents = GetArray(out, "Tangents", IG_POINT, 3);
        auto bitangents = GetArray(out, "Bitangents", IG_POINT, 3);
        if (!Check(!tangents.IsNull() && !bitangents.IsNull(), "both arrays exist")) { return false; }
        for (IGsize i = 0; i < 3; i++) {
            double t[3]{}, b[3]{};
            GetVector(tangents, i, t);
            GetVector(bitangents, i, b);
            ok &= Check(Near(Dot(t, b), 0.0), "point " + std::to_string(i) + " T . B = 0");
            ok &= Check(Near(VectorLength(b), 1.0), "point " + std::to_string(i) + " |B| = 1");
        }
    }
    return ok;
}

// 场景 7：非结构网格 / 混合单元 / 孤立点
bool TestOtherMeshTypes() {
    std::cout << "\n== Test 7: unstructured mesh / mixed cells / isolated point ==" << std::endl;
    bool ok = true;

    {
        Step("unstructured mesh with triangles");
        auto mesh = iGame::UnstructuredMesh::New();
        mesh->SetName("UnstructuredTriangles");
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
        auto cells = iGame::CellArray::New();
        auto types = iGame::UnsignedIntArray::New();
        mesh->SetCells(cells, types);
        igIndex triangle[3] = {0, 1, 2};
        mesh->AddCell(triangle, 3, iGame::IG_TRIANGLE);
        AddTextureCoordinates(mesh, {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});

        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "unstructured mesh: Execute()")) { return false; }
        auto out = filter->GetOutput();
        ok &= Check(!iGame::DynamicCast<iGame::UnstructuredMesh>(out).IsNull(),
                    "unstructured mesh: output keeps the mesh type");
        auto tangents = GetArray(out, "Tangents", IG_POINT, 3);
        if (!Check(!tangents.IsNull(), "unstructured mesh: output has \"Tangents\"")) { return false; }
        const double expected[3] = {1.0, 0.0, 0.0};
        for (IGsize i = 0; i < 3; i++) { ok &= ExpectVector(tangents, i, expected, "point " + std::to_string(i)); }
    }

    {
        Step("mixed cells: a line cell is ignored, an isolated point gets a zero tangent");
        auto mesh = iGame::UnstructuredMesh::New();
        mesh->SetName("MixedCells");
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f)); // 0
        mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f)); // 1
        mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f)); // 2
        mesh->AddPoint(iGame::Point(5.0f, 5.0f, 0.0f)); // 3：孤立点
        auto cells = iGame::CellArray::New();
        auto types = iGame::UnsignedIntArray::New();
        mesh->SetCells(cells, types);
        igIndex triangle[3] = {0, 1, 2};
        mesh->AddCell(triangle, 3, iGame::IG_TRIANGLE);
        igIndex line[2] = {0, 1};
        mesh->AddCell(line, 2, iGame::IG_LINE);
        AddTextureCoordinates(mesh, {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {0.5, 0.5}});

        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, mesh);
        if (!Check(filter->Execute(), "mixed cells: Execute()")) { return false; }
        auto tangents = GetArray(filter->GetOutput(), "Tangents", IG_POINT, 3);
        if (!Check(!tangents.IsNull(), "mixed cells: output has \"Tangents\"")) { return false; }
        const double expected[3] = {1.0, 0.0, 0.0};
        const double expectedZero[3] = {0.0, 0.0, 0.0};
        ok &= ExpectVector(tangents, 0, expected, "point 0 (in the triangle) = (1, 0, 0)");
        ok &= ExpectVector(tangents, 3, expectedZero, "isolated point = (0, 0, 0)");
    }
    return ok;
}

// 场景 8：独立输出，输入不能被修改
bool TestInputNotModified() {
    std::cout << "\n== Test 8: independent output (input is not modified) ==" << std::endl;

    auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                  iGame::Point(0.0f, 1.0f, 0.0f)},
                                 {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});
    auto pressure = iGame::FloatArray::New();
    pressure->SetName("Pressure");
    pressure->Resize(3);
    for (IGsize i = 0; i < 3; i++) { pressure->SetValue(i, static_cast<double>(i) + 1.0); }
    mesh->GetAttributeSet()->AddScalar(IG_POINT, pressure);


    auto filter = iGame::GenerateSurfaceTangentsFilter::New();
    filter->SetInput(0, mesh);
    if (!Check(filter->Execute(), "filter Execute()")) { return false; }

    bool ok = true;
    auto out = filter->GetOutput();
    ok &= Check(!out.IsNull(), "output exists");
    const iGame::DataObject* outRaw = out;
    const iGame::DataObject* inRaw = mesh;
    ok &= Check(outRaw != inRaw, "output is a different object");
    ok &= Check(GetArray(mesh, "Tangents", IG_POINT, 3).IsNull(), "input has no \"Tangents\" after Execute()");
    ok &= Check(!GetArray(mesh, "Pressure", IG_POINT, 1).IsNull(), "input still has its Pressure array");
    ok &= Check(mesh->GetNumberOfPoints() == 3, "input still has 3 points");
    ok &= Check(mesh->GetNumberOfFaces() == 1, "input still has 1 face");
    ok &= Check(!GetArray(out, "Tangents", IG_POINT, 3).IsNull(), "output has \"Tangents\"");
    ok &= Check(!GetArray(out, "Pressure", IG_POINT, 1).IsNull(), "output keeps the Pressure array");

    // 几何和纹理坐标也要原样带过去
    auto outPointSet = iGame::DynamicCast<iGame::PointSet>(out);
    ok &= Check(outPointSet != nullptr && outPointSet->GetNumberOfPoints() == 3, "output has 3 points");
    auto tex = GetArray(out, "TCoords", IG_POINT, 2);
    if (Check(!tex.IsNull(), "output keeps the input texture coordinates")) {
        ok &= Check(Near(tex->GetElementValue(1, 0), 1.0) && Near(tex->GetElementValue(2, 1), 1.0),
                    "output texture coordinates keep their values");
    } else {
        ok = false;
    }
    ok &= Check(out->GetName() == "Triangle_tangents", "output name = <input>_tangents");
    return ok;
}

// 场景 9：错误输入要报错（返回 false），而不是静默给出垃圾结果
bool TestErrorCases() {
    std::cout << "\n== Test 9: error cases ==" << std::endl;
    bool ok = true;

    {
        Step("no texture coordinates");
        auto mesh = iGame::SurfaceMesh::New();
        mesh->SetName("NoUV");
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
        auto faces = iGame::CellArray::New();
        mesh->SetFaces(faces);
        igIndex ids[3] = {0, 1, 2};
        faces->AddCellIds(ids, 3);

        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, mesh);
        ok &= Check(!filter->Execute(), "Execute() returns false when there is no texture coordinate");
    }

    {
        Step("texture coordinate array name not found");
        auto mesh = MakeTriangleMesh({iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                      iGame::Point(0.0f, 1.0f, 0.0f)},
                                     {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});
        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetTextureCoordinatesArrayName("NotExist");
        filter->SetInput(0, mesh);
        ok &= Check(!filter->Execute(), "Execute() returns false for an unknown array name");

        // 指定正确的名字就应该成功
        auto filter2 = iGame::GenerateSurfaceTangentsFilter::New();
        filter2->SetTextureCoordinatesArrayName("TCoords");
        filter2->SetInput(0, mesh);
        ok &= Check(filter2->Execute(), "Execute() succeeds when the name matches");
    }

    {
        Step("point set (no cells)");
        auto cloud = iGame::PointSet::New();
        cloud->SetName("Cloud");
        cloud->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
        cloud->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        AddTextureCoordinates(cloud, {{0.0, 0.0}, {1.0, 0.0}});

        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, cloud);
        ok &= Check(!filter->Execute(), "Execute() returns false for a point set");
    }

    {
        Step("volume mesh");
        auto mesh = iGame::VolumeMesh::New();
        mesh->SetName("Tetra");
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));
        auto volumes = iGame::CellArray::New();
        mesh->SetVolumes(volumes);
        igIndex tet[4] = {0, 1, 2, 3};
        volumes->AddCellIds(tet, 4);
        AddTextureCoordinates(mesh, {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {0.0, 0.0}});

        auto filter = iGame::GenerateSurfaceTangentsFilter::New();
        filter->SetInput(0, mesh);
        ok &= Check(!filter->Execute(), "Execute() returns false for a volume mesh");
    }
    return ok;
}

// 场景 10：演示模型（三角化圆柱面，带 TEXTURE_COORDINATES 和 NORMALS）
bool TestDemoModel() {
    std::cout << "\n== Test 10: demo model Examples/Models/SurfaceTangents.vtk ==" << std::endl;

    const std::string model = FindModel();
    if (model.empty()) {
        std::cout << "  [SKIP] model file not found (run the test from the build Examples folder)" << std::endl;
        return true;
    }
    std::cout << "  [info] model = " << model << std::endl;

    Step("read model");
    auto input = iGame::FileIO::ReadFile(model);
    if (!Check(!input.IsNull(), "read model")) { return false; }
    auto inputMesh = iGame::DynamicCast<iGame::SurfaceMesh>(input);
    if (!Check(!inputMesh.IsNull(), "model is a surface mesh")) { return false; }
    std::cout << "    points = " << inputMesh->GetNumberOfPoints()
              << ", faces = " << inputMesh->GetNumberOfFaces() << std::endl;
    if (!Check(inputMesh->GetNumberOfPoints() == 39 && inputMesh->GetNumberOfFaces() == 48,
               "model has 39 points and 48 triangles")) {
        return false;
    }
    if (!Check(!GetArray(input, "TCoords", IG_POINT, 2).IsNull(), "model has texture coordinates (TCoords)")) {
        return false;
    }
    if (!Check(!GetArray(input, "Normals", IG_POINT, 3).IsNull(), "model has normals (read as IG_NORMAL)")) {
        return false;
    }

    Step("run filter");
    auto filter = iGame::GenerateSurfaceTangentsFilter::New();
    filter->SetInput(0, input);
    if (!Check(filter->Execute(), "filter Execute()")) { return false; }

    Step("check output");
    auto out = filter->GetOutput();
    auto tangents = GetArray(out, "Tangents", IG_POINT, 3);
    if (!Check(!tangents.IsNull(), "output has \"Tangents\"")) { return false; }
    if (!Check(tangents->GetNumberOfElements() == 39, "Tangents has 39 elements")) { return false; }

    // 圆柱面：u 沿圆周方向（13 列，第 12 列是接缝的副本， u = i/12），
    // 所以切向量应该沿着"绕圆周"的方向：点 i 的期望切向量 ≈ (-sin(theta), cos(theta), 0)，
    // theta = 2*pi*(i%13)/12。网格是多边形逼近，接缝那两列只有单侧邻居，
    // 会差到 15 度（cos15° ≈ 0.966），所以用 0.95 判定点积。
    bool ok = true;
    double minLength = 1.0e30, maxLength = -1.0e30, minDot = 1.0e30, maxAbsZ = 0.0;
    for (IGsize i = 0; i < 39; i++) {
        double t[3]{};
        GetVector(tangents, i, t);
        const double length = VectorLength(t);
        minLength = std::min(minLength, length);
        maxLength = std::max(maxLength, length);
        maxAbsZ = std::max(maxAbsZ, std::fabs(t[2]));

        const double theta = 2.0 * 3.14159265358979323846 * static_cast<double>(i % 13) / 12.0;
        const double expected[3] = {-std::sin(theta), std::cos(theta), 0.0};
        minDot = std::min(minDot, Dot(t, expected));
        // 和 参考值_生成与核对_ParaView.py 的打印格式保持一致，方便逐点对照
        std::printf("    demo tangent[%2d] = (%.7f, %.7f, %.7f)\n", static_cast<int>(i), t[0], t[1], t[2]);
    }
    std::cout << "    |T| in [" << minLength << ", " << maxLength << "]" << std::endl;
    std::cout << "    max |T_z| = " << maxAbsZ << ", min dot(T, expected) = " << minDot << std::endl;

    ok &= Check(Near(minLength, 1.0, 1.0e-4) && Near(maxLength, 1.0, 1.0e-4), "every |T| = 1");
    ok &= Check(maxAbsZ < 1.0e-4, "tangents stay in the z = const plane (T_z = 0)");
    ok &= Check(minDot > 0.95, "tangents point along the circumferential (u) direction");

    // 每个点至少在一个三角形里，切向量都不该是 0
    bool noneIsZero = true;
    for (IGsize i = 0; i < 39; i++) {
        double t[3]{};
        GetVector(tangents, i, t);
        noneIsZero &= VectorLength(t) > 0.1;
    }
    ok &= Check(noneIsZero, "no point gets a zero tangent");
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok &= TestSingleTriangle();
    ok &= TestAccumulationWeightMatchesParaView();
    ok &= TestIrregularGridMatchesParaView();
    ok &= TestQuadMesh();
    ok &= TestDegenerateTextureCoordinates();
    ok &= TestBitangents();
    ok &= TestOtherMeshTypes();
    ok &= TestInputNotModified();
    ok &= TestErrorCases();
    ok &= TestDemoModel();

    if (ok) {
        std::cout << "\nALL TESTS PASSED" << std::endl;
        return 0;
    }
    std::cout << "\nSOME TESTS FAILED" << std::endl;
    return 1;
}
