// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <Modeling/iGameRibbonFilter.h>

// RibbonFilter（带状面）控制台用例。
//
// 语义基线：VTK Filters/Modeling/vtkRibbonFilter.cxx
//   - 每条线/折线独立计算框架：s = normalize((sPrev + sNext) / 2) 为平均切向，
//     w = normalize(cross(s, n))，nP = normalize(cross(w, s))，
//     v = w * cos(Angle) + nP * sin(Angle)，两侧点 p∓ = p ∓ Width * v（Width 是半宽）；
//   - 输出点顺序与 VTK 相同：偶数索引是减号侧 p-、奇数索引是加号侧 p+，
//     因此输出点数 = 2 * Σnpts；
//   - VTK 的 triangle strip 在 iGame 里展开为逐段 IG_QUAD，故面数 = Σ(npts - 1)；
//   - 退化（点数不足 / 相邻重复点 / 切线与该点法向平行）整条折线跳过并记入 GetMessage()。
//
// 模型（Examples/Models，构建时自动拷贝到 Examples 构建目录的 Models 下）：
//   Ribbon_Polylines2D.vtk：z=0 平面内 阶梯折线 + 共线折线 + 单线段（10 点 / 3 单元），
//                           带点属性 point_id 与单元属性 chain_id；
//   Ribbon_Helix3D.vtk    ：16 点螺旋线 + 1 条斜线（18 点 / 2 单元），
//                           带点属性 height 与单元属性 curve_id。
//
// 全部通过时逐行打印 PASS 并以退出码 0 结束；任一失败打印 FAIL 并返回 1。
// 不带参数时使用 ./Models 下的自带模型（在 Examples 构建目录下运行）；
// 第一个参数既可以是模型目录，也可以直接是一个 .vtk 文件。
//
// 注意：单元类型常量（IG_LINE / IG_QUAD / IG_TETRA ...）在本文件里必须写全限定名
// iGame::IG_xxx —— iGame 命名空间下有同名符号会遮蔽全局枚举。
//
// 回归背景：
//   - BUG / 触发条件 / 错误表现：本用例与 filter 实现首次一并加入，尚无独立的修复提交；
//     开发期间按 VTK 语义逐条校正的行为是：转角处必须用平均切向（否则折线带面在转角处撕裂）、
//     输出点顺序必须是「偶 = 减号侧」（否则与 VTK 的 strip 点序不一致）、
//     四边形必须用环形绕序（写成 (p-ᵢ, p+ᵢ, p-ᵢ₊₁, p+ᵢ₊₁) 会被扇形三角化成自交蝴蝶结）、
//     以及切线与法向平行/相邻重复点时整条跳过（否则写出 NaN 或零长度四边形）。
//   - 用例保证的正确行为：输出点数 = 2Σnpts、面数 = Σ(npts-1)；每个截面宽度恒为 2*Width；
//     单元属性按来源线单元逐段映射、点属性按来源点复制且类型不变；旧法向不复制；
//     非线单元只统计不失败；退化折线被跳过且不留下孤立点；输入对象不被修改。
//   - 重要边界及原因：竖直线配默认法向 (0,0,1)（cross(t, n) == 0 的退化分支，模型里刻意不放）、
//     含重复点的折线（VTK 的 Coincident points 分支）、Width <= 0 与零默认法向的失败路径、
//     以及两个自带模型分别覆盖「平面多折线 + 属性映射」与「三维框架 + 螺旋线」。
//   - 首次提交：测试与实现同次提交（提交主题
//     "feat(filter): add Ribbon filter with examples, models and user notice"），
//     为避免自引用这里不写提交号，可用下面命令查询：
//       git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestRibbon.cpp
//     后续若针对本 filter 提交修复，请把真实提交号补在这一行下面。

namespace ig = iGame;

namespace {

int g_FailureCount = 0;

void Check(bool ok, const std::string& what) {
    if (ok) {
        std::cout << "PASS: " << what << std::endl;
    } else {
        std::cout << "FAIL: " << what << std::endl;
        ++g_FailureCount;
    }
}

void CheckNear(double actual, double expected, double tolerance, const std::string& what) {
    const bool ok = std::fabs(actual - expected) <= tolerance;
    Check(ok, what);
    if (!ok) { std::cout << "      actual=" << actual << " expected=" << expected << std::endl; }
}

void CheckVector(const ig::Vector3d& actual, const ig::Vector3d& expected, double tolerance,
                 const std::string& what) {
    const bool ok = std::fabs(actual[0] - expected[0]) <= tolerance &&
                    std::fabs(actual[1] - expected[1]) <= tolerance &&
                    std::fabs(actual[2] - expected[2]) <= tolerance;
    Check(ok, what);
    if (!ok) {
        std::cout << "      actual=(" << actual[0] << ", " << actual[1] << ", " << actual[2] << ") expected=("
                  << expected[0] << ", " << expected[1] << ", " << expected[2] << ")" << std::endl;
    }
}

bool FileExists(const std::string& path) {
    std::ifstream stream(path);
    return stream.good();
}

// ---------------------------------------------------------------------------
// 工具：与线性拉伸用例同款（按挂载类型取数组、独立读取模型、程序化构造线网格）
// ---------------------------------------------------------------------------
ig::DataObject::Pointer LoadModel(const std::string& fileName) {
    auto object = ig::FileIO::ReadFile(fileName);
    if (object == nullptr) { std::cout << "FAIL: cannot read model " << fileName << std::endl; }
    return object;
}

// 按名字与挂载类型取属性数组：GetScalar(name) 不区分挂载类型，点/单元同名时会取错
ig::ArrayObject* FindArray(ig::DataObject::Pointer object, IGenum attachmentType, const std::string& name) {
    if (object == nullptr || object->GetAttributeSet() == nullptr) { return nullptr; }
    auto attributes = (attachmentType == IG_POINT) ? object->GetAttributeSet()->GetAllPointAttributes()
                                                  : object->GetAttributeSet()->GetAllCellAttributes();
    if (attributes == nullptr) { return nullptr; }
    for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attribute = attributes->GetElement(i);
        if (attribute.IsNone() || attribute.pointer == nullptr) { continue; }
        if (attribute.pointer->GetName() == name) { return attribute.pointer; }
    }
    return nullptr;
}

// 程序化构造只含线单元的 UnstructuredMesh：cells 里每项是 (单元类型, 点号列表)
ig::UnstructuredMesh::Pointer BuildLineMesh(const std::vector<ig::Vector3d>& points,
                                            const std::vector<std::pair<IGenum, std::vector<igIndex>>>& cells) {
    auto mesh = ig::UnstructuredMesh::New();
    auto pointSet = ig::Points::New();
    for (const ig::Vector3d& p: points) { pointSet->AddPoint(p); }
    mesh->SetPoints(pointSet);

    auto cellArray = ig::CellArray::New();
    auto types = ig::UnsignedIntArray::New();
    for (const auto& cell: cells) {
        cellArray->AddCellIds(cell.second.data(), static_cast<int>(cell.second.size()));
        types->AddValue(cell.first);
    }
    mesh->SetCells(cellArray, types);
    return mesh;
}

void EnsureAttributeSet(ig::DataObject::Pointer object) {
    if (object->GetAttributeSet() == nullptr) { object->SetAttributeSet(ig::AttributeSet::New()); }
}

// 一维 int 属性：int 数组由 FlatArray 生成，GetArrayType() 是 IG_IntArray
void AddIntAttribute(ig::DataObject::Pointer object, IGenum attachmentType, const std::string& name,
                     const std::vector<int>& values) {
    EnsureAttributeSet(object);
    ig::ArrayObject::Pointer array = ig::IntArray::New();
    array->SetName(name);
    array->SetDimension(1);
    array->Resize(values.size());
    for (size_t i = 0; i < values.size(); ++i) { array->SetValue(i, values[i]); }
    object->GetAttributeSet()->AddAttribute(IG_SCALAR, attachmentType, array);
}

// 三维 float 法向属性：用于验证旧法向不被复制、以及 UseDefaultNormal 关时读取输入法向
void AddPointNormalAttribute(ig::DataObject::Pointer object, const std::string& name,
                             const std::vector<ig::Vector3d>& normals) {
    EnsureAttributeSet(object);
    ig::ArrayObject::Pointer array = ig::FloatArray::New();
    array->SetName(name);
    array->SetDimension(3);
    array->Resize(normals.size());
    for (size_t i = 0; i < normals.size(); ++i) {
        array->SetValue(i * 3 + 0, normals[i][0]);
        array->SetValue(i * 3 + 1, normals[i][1]);
        array->SetValue(i * 3 + 2, normals[i][2]);
    }
    object->GetAttributeSet()->AddAttribute(IG_NORMAL, IG_POINT, array);
}

ig::Vector3d OutputPoint(ig::SurfaceMesh::Pointer mesh, IGsize id) {
    ig::Vector3d p;
    mesh->GetPoints()->GetPoint(id, p);
    return p;
}

bool AllPointsFinite(ig::SurfaceMesh::Pointer mesh) {
    for (IGsize i = 0; i < mesh->GetNumberOfPoints(); ++i) {
        const ig::Vector3d p = OutputPoint(mesh, i);
        if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) { return false; }
    }
    return true;
}

// 逐段宽度必须恒为 2 * Width：第 i 段的四个点是 (p-ᵢ, p+ᵢ, p+ᵢ₊₁, p-ᵢ₊₁)
bool AllSegmentWidthsMatch(ig::SurfaceMesh::Pointer mesh, double expectedWidth, double tolerance) {
    for (IGsize faceId = 0; faceId < mesh->GetNumberOfFaces(); ++faceId) {
        igIndex ids[8] = {0};
        const int size = mesh->GetFacePointIds(faceId, ids);
        if (size != 4) { return false; }
        const ig::Vector3d minus0 = OutputPoint(mesh, ids[0]);
        const ig::Vector3d plus0 = OutputPoint(mesh, ids[1]);
        const ig::Vector3d plus1 = OutputPoint(mesh, ids[2]);
        const ig::Vector3d minus1 = OutputPoint(mesh, ids[3]);
        if (std::fabs((plus0 - minus0).length() - expectedWidth) > tolerance) { return false; }
        if (std::fabs((plus1 - minus1).length() - expectedWidth) > tolerance) { return false; }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 用例 1：单条直线 + 默认法向，逐点核对框架与输出顺序
//   期望：4 点 1 个四边形；偶数索引在 p - Width*w 一侧（w = cross(s,n) = (0,-1,0)），
//         截面中点仍落在原线上；四边形绕序给出的法向与 nP 同向。
// ---------------------------------------------------------------------------
void TestSingleLineFrame() {
    std::cout << "--- case 1: single line, default frame ---" << std::endl;

    auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}}, {{iGame::IG_LINE, {0, 1}}});

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(mesh);
    filter->SetWidth(0.5);
    filter->SetAngle(0.0);
    filter->SetDefaultNormal(0.0, 0.0, 1.0);
    if (!filter->Execute()) {
        Check(false, "single line: Execute() succeeds");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
        return;
    }
    Check(true, "single line: Execute() succeeds");

    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "single line: output is a SurfaceMesh");
        return;
    }
    Check(output->GetNumberOfPoints() == 4, "single line: point count == 2 * npts");
    Check(output->GetNumberOfFaces() == 1, "single line: one quad for one segment");

    CheckVector(OutputPoint(output, 0), ig::Vector3d(0.0, 0.5, 0.0), 1e-5,
                "single line: even index 0 is the minus side (p - Width*w)");
    CheckVector(OutputPoint(output, 1), ig::Vector3d(0.0, -0.5, 0.0), 1e-5,
                "single line: odd index 1 is the plus side (p + Width*w)");
    CheckVector(OutputPoint(output, 2), ig::Vector3d(2.0, 0.5, 0.0), 1e-5, "single line: point 2 follows the line");
    CheckVector(OutputPoint(output, 3), ig::Vector3d(2.0, -0.5, 0.0), 1e-5, "single line: point 3 follows the line");

    CheckNear((OutputPoint(output, 1) - OutputPoint(output, 0)).length(), 1.0, 1e-5,
              "single line: ribbon width == 2 * Width");
    CheckVector((OutputPoint(output, 0) + OutputPoint(output, 1)) / 2.0, ig::Vector3d(0.0, 0.0, 0.0), 1e-5,
                "single line: cross section stays centered on the input line");

    igIndex ids[4] = {-1, -1, -1, -1};
    const int size = output->GetFacePointIds(0, ids);
    Check(size == 4, "single line: the output face is a quad");
    Check(ids[0] == 0 && ids[1] == 1 && ids[2] == 3 && ids[3] == 2,
          "single line: quad keeps the ring order (p-0, p+0, p+1, p-1)");

    const ig::Vector3d normal =
            (OutputPoint(output, 1) - OutputPoint(output, 0)).cross(OutputPoint(output, 3) - OutputPoint(output, 0));
    Check(normal[2] > 0.0 && std::fabs(normal[0]) < 1e-6 && std::fabs(normal[1]) < 1e-6,
          "single line: quad winding gives the +nP face normal");

    Check(mesh->GetNumberOfPoints() == 2 && mesh->GetNumberOfCells() == 1,
          "single line: the input object is not modified");
}

// ---------------------------------------------------------------------------
// 用例 2：L 形折线（3 点）—— 转角处用平均切向做倒角
//   期望：6 点 2 面；中间点的两侧点由 (p₂ - p₀) 的平均切向解析算出；
//         全部坐标有限；输入点数/单元数不变。
// ---------------------------------------------------------------------------
void TestPolyLineBevel() {
    std::cout << "--- case 2: L shaped polyline ---" << std::endl;

    const std::vector<ig::Vector3d> points = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}};
    auto mesh = BuildLineMesh(points, {{iGame::IG_POLY_LINE, {0, 1, 2}}});

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(mesh);
    filter->SetWidth(0.5);
    if (!filter->Execute()) {
        Check(false, "L polyline: Execute() succeeds");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
        return;
    }
    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "L polyline: output is a SurfaceMesh");
        return;
    }

    Check(output->GetNumberOfPoints() == 6, "L polyline: point count == 2 * 3");
    Check(output->GetNumberOfFaces() == 2, "L polyline: one quad per segment");
    Check(AllPointsFinite(output), "L polyline: every output coordinate is finite");

    // 中间点：s = normalize((1,0,0)+(0,1,0)) = (√2/2, √2/2, 0)，w = (√2/2, -√2/2, 0)
    const double half = 0.5 * std::sqrt(0.5);
    CheckVector(OutputPoint(output, 2), ig::Vector3d(1.0 - half, half, 0.0), 1e-5,
                "L polyline: middle point minus side uses the averaged tangent");
    CheckVector(OutputPoint(output, 3), ig::Vector3d(1.0 + half, -half, 0.0), 1e-5,
                "L polyline: middle point plus side uses the averaged tangent");
    CheckVector((OutputPoint(output, 2) + OutputPoint(output, 3)) / 2.0, points[1], 1e-5,
                "L polyline: beveled cross section stays centered on the corner point");

    // 首尾点用各自线段的切向
    CheckVector(OutputPoint(output, 0), ig::Vector3d(0.0, 0.5, 0.0), 1e-5, "L polyline: first point uses the first segment");
    CheckVector(OutputPoint(output, 5), ig::Vector3d(1.5, 1.0, 0.0), 1e-5, "L polyline: last point uses the last segment");
    Check(AllSegmentWidthsMatch(output, 1.0, 1e-5), "L polyline: every cross section is 2 * Width wide");

    Check(mesh->GetNumberOfPoints() == 3 && mesh->GetNumberOfCells() == 1, "L polyline: the input is not modified");
}

// ---------------------------------------------------------------------------
// 用例 3：Angle = 90° —— 带面方向相对 0° 情形正交（沿 nP 张开）
// ---------------------------------------------------------------------------
void TestRibbonAngle() {
    std::cout << "--- case 3: Angle == 90 degrees ---" << std::endl;

    auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}}, {{iGame::IG_LINE, {0, 1}}});

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(mesh);
    filter->SetWidth(0.5);
    filter->SetAngle(90.0);
    if (!filter->Execute()) {
        Check(false, "angle 90: Execute() succeeds");
        return;
    }
    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "angle 90: output is a SurfaceMesh");
        return;
    }

    CheckVector(OutputPoint(output, 0), ig::Vector3d(0.0, 0.0, -0.5), 1e-5,
                "angle 90: ribbon opens along nP (minus side is -Z)");
    CheckVector(OutputPoint(output, 1), ig::Vector3d(0.0, 0.0, 0.5), 1e-5,
                "angle 90: plus side is +Z");

    const ig::Vector3d side90 = OutputPoint(output, 1) - OutputPoint(output, 0);
    const ig::Vector3d side0(0.0, -1.0, 0.0);  // Angle = 0 时的张开方向
    CheckNear(side90.dot(side0), 0.0, 1e-6, "angle 90: opening direction is orthogonal to the Angle = 0 case");
    CheckNear(side90[2], 1.0, 1e-5, "angle 90: the height difference equals 2 * Width");
    CheckNear(side90[0], 0.0, 1e-6, "angle 90: no X component");
}

// ---------------------------------------------------------------------------
// 用例 4：属性传播 —— 点属性按来源点复制、单元属性按来源线单元复制、旧法向不复制
// ---------------------------------------------------------------------------
void TestAttributePropagation() {
    std::cout << "--- case 4: attribute propagation ---" << std::endl;

    auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}},
                              {{iGame::IG_POLY_LINE, {0, 1, 2}}});
    AddIntAttribute(mesh, IG_POINT, "point_id", {0, 1, 2});
    AddIntAttribute(mesh, IG_CELL, "chain_id", {7});
    AddPointNormalAttribute(mesh, "Normals",
                            {{0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}});

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        Check(false, "attributes: Execute() succeeds");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
        return;
    }
    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "attributes: output is a SurfaceMesh");
        return;
    }

    ig::ArrayObject* pointIds = FindArray(output, IG_POINT, "point_id");
    if (pointIds == nullptr) {
        Check(false, "attributes: point attribute point_id is copied");
        return;
    }
    Check(pointIds->GetArrayType() == IG_IntArray, "attributes: point array keeps its concrete type (int)");
    Check(pointIds->GetDimension() == 1, "attributes: point array keeps its dimension");
    Check(pointIds->GetNumberOfElements() == 6, "attributes: point array is resized to the output point count");
    bool pointValuesOk = true;
    for (IGsize i = 0; i < 3; ++i) {
        // 第 i 个输入点的两侧副本必须是同一个值
        if (pointIds->GetValue(i * 2 + 0) != static_cast<double>(i)) { pointValuesOk = false; }
        if (pointIds->GetValue(i * 2 + 1) != static_cast<double>(i)) { pointValuesOk = false; }
    }
    Check(pointValuesOk, "attributes: both copies of a source point carry the same value");

    ig::ArrayObject* chainIds = FindArray(output, IG_CELL, "chain_id");
    if (chainIds == nullptr) {
        Check(false, "attributes: cell attribute chain_id is copied");
    } else {
        Check(chainIds->GetArrayType() == IG_IntArray, "attributes: cell array keeps its concrete type (int)");
        Check(chainIds->GetNumberOfElements() == 2, "attributes: one cell value per output quad");
        Check(chainIds->GetValue(0) == 7.0 && chainIds->GetValue(1) == 7.0,
              "attributes: every quad of the polyline carries the source cell value");
    }

    Check(FindArray(output, IG_POINT, "Normals") == nullptr,
          "attributes: the old point normals are not copied (same as VTK CopyNormalsOff)");
    Check(mesh->GetNumberOfPoints() == 3, "attributes: the input is not modified");
}

// ---------------------------------------------------------------------------
// 用例 5：多个线单元的来源映射 —— 每个四边形必须拿到它自己那条线单元的属性
// ---------------------------------------------------------------------------
void TestPerCellMapping() {
    std::cout << "--- case 5: per-cell attribute mapping ---" << std::endl;

    const std::vector<ig::Vector3d> points = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
                                              {2.0, 1.0, 0.0}, {2.0, 2.0, 0.0}, {0.0, 3.0, 0.0},
                                              {2.0, 3.0, 0.0}, {0.0, 4.0, 0.0}, {1.0, 4.0, 0.0},
                                              {2.0, 4.0, 0.0}};
    auto mesh = BuildLineMesh(points,
                              {{iGame::IG_POLY_LINE, {0, 1, 2, 3, 4}},  // 4 段
                               {iGame::IG_LINE, {5, 6}},                 // 1 段
                               {iGame::IG_POLY_LINE, {7, 8, 9}}});       // 2 段
    AddIntAttribute(mesh, IG_CELL, "chain_id", {10, 20, 30});

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        Check(false, "cell mapping: Execute() succeeds");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
        return;
    }
    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "cell mapping: output is a SurfaceMesh");
        return;
    }

    Check(output->GetNumberOfPoints() == 20, "cell mapping: point count == 2 * 10");
    Check(output->GetNumberOfFaces() == 7, "cell mapping: face count == sum(npts - 1)");

    ig::ArrayObject* chainIds = FindArray(output, IG_CELL, "chain_id");
    if (chainIds == nullptr) {
        Check(false, "cell mapping: cell attribute chain_id is copied");
        return;
    }
    const double expected[7] = {10.0, 10.0, 10.0, 10.0, 20.0, 30.0, 30.0};
    bool mappingOk = true;
    for (IGsize i = 0; i < 7; ++i) {
        if (chainIds->GetValue(i) != expected[i]) { mappingOk = false; }
    }
    Check(mappingOk, "cell mapping: each quad inherits the value of its own source line cell");
    Check(AllSegmentWidthsMatch(output, 1.0, 1e-5), "cell mapping: every cross section is 2 * Width wide");
}

// ---------------------------------------------------------------------------
// 用例 6：退化输入 —— 竖直线配默认法向、含重复点的折线、非法参数、法向回退
//   期望：单条折线退化时整条跳过（不产生孤立点、不写 NaN），仍有有效折线时 Execute 成功
//         并在 GetMessage() 里说明；全部退化、Width <= 0、零默认法向则失败。
// ---------------------------------------------------------------------------
void TestDegenerateInputs() {
    std::cout << "--- case 6: degenerate inputs ---" << std::endl;

    // 6.1 竖直线（切线与默认法向 (0,0,1) 平行）+ 一条正常线
    {
        auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {5.0, 0.0, 0.0}, {7.0, 0.0, 0.0}},
                                  {{iGame::IG_LINE, {0, 1}}, {iGame::IG_LINE, {2, 3}}});
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        Check(filter->Execute(), "degenerate: a parallel line is skipped and the rest still succeeds");
        auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
        if (output != nullptr) {
            Check(output->GetNumberOfPoints() == 4 && output->GetNumberOfFaces() == 1,
                  "degenerate: the skipped line produces no geometry at all (no orphan points)");
            Check(AllPointsFinite(output), "degenerate: no NaN is written for the skipped line");
        }
        Check(!filter->GetMessage().empty(), "degenerate: GetMessage() explains the skipped line");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
    }

    // 6.2 含重复点的折线（VTK 的 Coincident points 分支）+ 一条正常线
    {
        auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {5.0, 0.0, 0.0}, {7.0, 0.0, 0.0}},
                                  {{iGame::IG_POLY_LINE, {0, 1, 2}}, {iGame::IG_LINE, {3, 4}}});
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        Check(filter->Execute(), "degenerate: a polyline with coincident points is skipped");
        auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
        if (output != nullptr) {
            Check(output->GetNumberOfPoints() == 4 && output->GetNumberOfFaces() == 1,
                  "degenerate: only the valid line contributes geometry");
            Check(AllPointsFinite(output), "degenerate: no NaN for the coincident-point polyline");
        }
        Check(!filter->GetMessage().empty(), "degenerate: GetMessage() explains the coincident points");
    }

    // 6.3 全部折线都退化 -> 失败
    {
        auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}}, {{iGame::IG_LINE, {0, 1}}});
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "degenerate: every line skipped means Execute() fails");
        Check(!filter->GetMessage().empty(), "degenerate: GetMessage() explains the failure");
        Check(filter->GetOutput() == nullptr, "degenerate: a failed execution leaves no output behind");
    }

    // 6.4 Width <= 0 -> 失败（VTK 会 clamp 成 0 产出零宽面，这里明确报错）
    {
        auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}}, {{iGame::IG_LINE, {0, 1}}});
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        filter->SetWidth(0.0);
        Check(!filter->Execute(), "degenerate: Width == 0 fails");
        Check(!filter->GetMessage().empty(), "degenerate: GetMessage() explains the invalid width");
    }

    // 6.5 零默认法向 -> 失败
    {
        auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}}, {{iGame::IG_LINE, {0, 1}}});
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        filter->SetDefaultNormal(0.0, 0.0, 0.0);
        Check(!filter->Execute(), "degenerate: a zero DefaultNormal fails");
        Check(!filter->GetMessage().empty(), "degenerate: GetMessage() explains the zero normal");
    }

    // 6.6 UseDefaultNormal 关且输入没有点法向 -> 回退默认法向并在 message 里说明
    {
        auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}}, {{iGame::IG_LINE, {0, 1}}});
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        filter->SetUseDefaultNormal(false);
        Check(filter->Execute(), "degenerate: falls back to the DefaultNormal when no point normals exist");
        Check(!filter->GetMessage().empty(), "degenerate: the fallback is reported through GetMessage()");
        auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
        if (output != nullptr) {
            CheckVector(OutputPoint(output, 0), ig::Vector3d(0.0, 0.5, 0.0), 1e-5,
                        "degenerate: the fallback uses the DefaultNormal (0,0,1)");
        }
    }
}

// ---------------------------------------------------------------------------
// 用例 7：非法输入与状态恢复
// ---------------------------------------------------------------------------
void TestInvalidInputs(const std::string& modelPath) {
    std::cout << "--- case 7: invalid inputs ---" << std::endl;

    // 无输入
    {
        auto filter = ig::RibbonFilter::New();
        Check(!filter->Execute(), "invalid: Execute() fails without an input");
        Check(!filter->GetMessage().empty(), "invalid: GetMessage() explains the missing input");
        Check(filter->GetOutput() == nullptr, "invalid: no output is left behind");
    }

    // 只有体单元：没有任何线单元
    {
        auto mesh = ig::UnstructuredMesh::New();
        mesh->SetPoints(ig::Points::New());
        mesh->GetPoints()->AddPoint(0.0, 0.0, 0.0);
        mesh->GetPoints()->AddPoint(1.0, 0.0, 0.0);
        mesh->GetPoints()->AddPoint(0.0, 1.0, 0.0);
        mesh->GetPoints()->AddPoint(0.0, 0.0, 1.0);
        auto cells = ig::CellArray::New();
        auto types = ig::UnsignedIntArray::New();
        igIndex tetra[4] = {0, 1, 2, 3};
        cells->AddCellIds(tetra, 4);
        types->AddValue(iGame::IG_TETRA);
        mesh->SetCells(cells, types);

        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "invalid: a tetrahedron-only mesh fails (no line cell)");
        Check(!filter->GetMessage().empty(), "invalid: GetMessage() explains that no line cell was found");
    }

    // 有点但没有单元
    {
        auto mesh = ig::UnstructuredMesh::New();
        mesh->SetPoints(ig::Points::New());
        mesh->GetPoints()->AddPoint(0.0, 0.0, 0.0);
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "invalid: an UnstructuredMesh without cells fails");
    }

    // 没有任何点的网格
    {
        auto mesh = ig::UnstructuredMesh::New();
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "invalid: an input without points fails");
    }

    // SurfaceMesh 输入但没有显式边数组
    {
        auto mesh = ig::SurfaceMesh::New();
        auto pointSet = ig::Points::New();
        pointSet->AddPoint(0.f, 0.f, 0.f);
        pointSet->AddPoint(1.f, 0.f, 0.f);
        pointSet->AddPoint(1.f, 1.f, 0.f);
        mesh->SetPoints(pointSet);
        auto faces = ig::CellArray::New();
        igIndex triangle[3] = {0, 1, 2};
        faces->AddCellIds(triangle, 3);
        mesh->SetFaces(faces);

        auto filter = ig::RibbonFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "invalid: a SurfaceMesh without explicit edges fails (faces are not ribboned)");
        Check(!filter->GetMessage().empty(), "invalid: GetMessage() explains the missing edges");
    }

    // 失败之后再给合法输入：仍然能正常执行（上一次失败不残留状态）
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        auto filter = ig::RibbonFilter::New();
        filter->SetInput(input);
        Check(filter->Execute(), "invalid: a valid run right after a failed run still succeeds");
        Check(filter->GetOutput() != nullptr, "invalid: the successful run produces an output");
    }
}

// ---------------------------------------------------------------------------
// 用例 8：默认参数、Angle 夹取，以及 UseDefaultNormal 关时读取输入点法向
// ---------------------------------------------------------------------------
void TestDefaultsAndInputNormals() {
    std::cout << "--- case 8: defaults, angle clamp and input normals ---" << std::endl;

    auto filter = ig::RibbonFilter::New();
    CheckNear(filter->GetWidth(), 0.5, 1e-12, "defaults: Width == 0.5 (same as the VTK constructor)");
    CheckNear(filter->GetAngle(), 0.0, 1e-12, "defaults: Angle == 0");
    CheckVector(filter->GetDefaultNormal(), ig::Vector3d(0.0, 0.0, 1.0), 1e-12,
                "defaults: DefaultNormal == (0, 0, 1)");
    Check(filter->GetUseDefaultNormal(), "defaults: UseDefaultNormal is on in this first version");

    filter->SetAngle(-30.0);
    CheckNear(filter->GetAngle(), 0.0, 1e-12, "defaults: Angle is clamped to 0 (same as vtkSetClampMacro)");
    filter->SetAngle(400.0);
    CheckNear(filter->GetAngle(), 360.0, 1e-12, "defaults: Angle is clamped to 360");
    filter->SetAngle(90.0);
    CheckNear(filter->GetAngle(), 90.0, 1e-12, "defaults: a valid Angle is kept");

    // UseDefaultNormal 关 + 输入带 IG_NORMAL -> 按 VTK 使用输入点法向
    auto mesh = BuildLineMesh({{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}}, {{iGame::IG_LINE, {0, 1}}});
    AddPointNormalAttribute(mesh, "Normals", {{0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}});

    auto normalFilter = ig::RibbonFilter::New();
    normalFilter->SetInput(mesh);
    normalFilter->SetUseDefaultNormal(false);
    if (!normalFilter->Execute()) {
        Check(false, "input normals: Execute() succeeds with point normals");
        std::cout << "      message: " << normalFilter->GetMessage() << std::endl;
        return;
    }
    auto output = ig::DynamicCast<ig::SurfaceMesh>(normalFilter->GetOutput());
    if (output == nullptr) {
        Check(false, "input normals: output is a SurfaceMesh");
        return;
    }
    // n = (0,1,0) -> w = cross(s, n) = (0,0,1) -> 减号侧在 -Z
    CheckVector(OutputPoint(output, 0), ig::Vector3d(0.0, 0.0, -0.5), 1e-5,
                "input normals: the input point normals drive the ribbon orientation");
    CheckVector(OutputPoint(output, 1), ig::Vector3d(0.0, 0.0, 0.5), 1e-5,
                "input normals: the plus side is on the opposite side");
}

// ---------------------------------------------------------------------------
// 用例 9：自带模型 A（平面折线组合 + 点/单元属性）
//   10 点 / 3 单元（5 点折线、3 点折线、2 点线）-> 20 点 / 7 面
// ---------------------------------------------------------------------------
void TestPolylines2DModel(const std::string& modelPath) {
    std::cout << "--- case 9: model A (planar polylines with attributes) ---" << std::endl;

    auto input = LoadModel(modelPath);
    if (input == nullptr) { return; }
    auto inputMesh = ig::DynamicCast<ig::UnstructuredMesh>(input);
    if (inputMesh == nullptr) {
        Check(false, "model A: input is an UnstructuredMesh");
        return;
    }
    Check(inputMesh->GetNumberOfPoints() == 10, "model A: 10 input points");
    Check(inputMesh->GetNumberOfCells() == 3, "model A: 3 input line cells");
    Check(FindArray(input, IG_POINT, "point_id") != nullptr, "model A: point attribute point_id is present");
    Check(FindArray(input, IG_CELL, "chain_id") != nullptr, "model A: cell attribute chain_id is present");

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(input);
    filter->SetWidth(0.5);
    if (!filter->Execute()) {
        Check(false, "model A: Execute() succeeds");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
        return;
    }
    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "model A: output is a SurfaceMesh");
        return;
    }

    Check(output->GetNumberOfPoints() == 20, "model A: point count == 2 * 10");
    Check(output->GetNumberOfFaces() == 7, "model A: face count == 4 + 2 + 1");
    Check(AllPointsFinite(output), "model A: every output coordinate is finite");
    Check(AllSegmentWidthsMatch(output, 1.0, 1e-5), "model A: every cross section is 2 * Width wide");

    ig::ArrayObject* pointIds = FindArray(output, IG_POINT, "point_id");
    Check(pointIds != nullptr && pointIds->GetNumberOfElements() == 20, "model A: point attribute covers all output points");
    ig::ArrayObject* chainIds = FindArray(output, IG_CELL, "chain_id");
    if (chainIds != nullptr) {
        // 单元 0（5 点阶梯）-> 4 段；单元 1（3 点共线）-> 2 段；单元 2（单线段）-> 1 段
        const double expected[7] = {10.0, 10.0, 10.0, 10.0, 20.0, 20.0, 30.0};
        bool mappingOk = true;
        for (IGsize i = 0; i < 7; ++i) {
            if (chainIds->GetValue(i) != expected[i]) { mappingOk = false; }
        }
        Check(mappingOk, "model A: each quad inherits its own source cell value (10 x4, 20 x2, 30 x1)");
    } else {
        Check(false, "model A: cell attribute chain_id is copied");
    }

    Check(inputMesh->GetNumberOfPoints() == 10 && inputMesh->GetNumberOfCells() == 3,
          "model A: the input file object is not modified");
}

// ---------------------------------------------------------------------------
// 用例 10：自带模型 B（三维螺旋线 + 斜线）
//   18 点 / 2 单元（16 点折线、2 点线）-> 36 点 / 16 面
// ---------------------------------------------------------------------------
void TestHelix3DModel(const std::string& modelPath) {
    std::cout << "--- case 10: model B (3D helix + tilted line) ---" << std::endl;

    auto input = LoadModel(modelPath);
    if (input == nullptr) { return; }
    auto inputMesh = ig::DynamicCast<ig::UnstructuredMesh>(input);
    if (inputMesh == nullptr) {
        Check(false, "model B: input is an UnstructuredMesh");
        return;
    }
    Check(inputMesh->GetNumberOfPoints() == 18, "model B: 18 input points");
    Check(inputMesh->GetNumberOfCells() == 2, "model B: 2 input line cells");

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(input);
    if (!filter->Execute()) {
        Check(false, "model B: Execute() succeeds");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
        return;
    }
    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "model B: output is a SurfaceMesh");
        return;
    }

    Check(output->GetNumberOfPoints() == 36, "model B: point count == 2 * 18");
    Check(output->GetNumberOfFaces() == 16, "model B: face count == 15 + 1");
    Check(AllPointsFinite(output), "model B: every output coordinate is finite");
    Check(AllSegmentWidthsMatch(output, 1.0, 1e-5), "model B: every cross section is 2 * Width wide");
    Check(FindArray(output, IG_POINT, "height") != nullptr, "model B: point attribute height is copied");
    Check(FindArray(output, IG_CELL, "curve_id") != nullptr, "model B: cell attribute curve_id is copied");
}

// ---------------------------------------------------------------------------
// 用例 11：真实文件里的混合单元 —— 只处理线单元，其他类型忽略且不失败
//   iGameValidateCellsFilter_mixed.vtk 含 2 条 VTK_LINE（第二条的两个点坐标相同，
//   属于 VTK 的 Coincident points 分支）+ 三角/四边/四面体/六面体。
// ---------------------------------------------------------------------------
void TestMixedCellModel(const std::string& modelPath) {
    std::cout << "--- case 11: mixed cell types in a real file ---" << std::endl;

    auto input = LoadModel(modelPath);
    if (input == nullptr) { return; }

    auto filter = ig::RibbonFilter::New();
    filter->SetInput(input);
    Check(filter->Execute(), "mixed model: Execute() succeeds with only the line cells processed");
    auto output = ig::DynamicCast<ig::SurfaceMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "mixed model: output is a SurfaceMesh");
        return;
    }
    // 第 1 条线有效；第 2 条线的两个点坐标相同 -> 整条跳过，因此只有 1 个四边形
    Check(output->GetNumberOfPoints() == 4 && output->GetNumberOfFaces() == 1,
          "mixed model: only the usable line cell produces a quad");
    Check(!filter->GetMessage().empty(), "mixed model: ignored cell types and skipped lines are reported");
    std::cout << "      message: " << filter->GetMessage() << std::endl;
}

// ---------------------------------------------------------------------------
// 用例 12：入口参数语义 —— 不带参数时必须使用仓库自带的模型
// ---------------------------------------------------------------------------
void TestModelPaths(bool usedDefaultArgument, const std::string& modelA) {
    std::cout << "--- case 12: model path resolution ---" << std::endl;
    Check(FileExists(modelA), "paths: model A exists at " + modelA);
    if (usedDefaultArgument) {
        Check(modelA == "./Models/Ribbon_Polylines2D.vtk",
              "paths: running without arguments uses ./Models/Ribbon_Polylines2D.vtk");
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::cout << "==== RibbonFilter test ====" << std::endl;

    // 不带参数：使用仓库自带模型（在 Examples 构建目录下运行）；
    // 第一个参数既可以是模型目录（末尾不要带分隔符），也可以直接是一个 .vtk 文件。
    const std::string argument = (argc > 1) ? std::string(argv[1]) : std::string("./Models");
    const bool usedDefaultArgument = (argc <= 1);

    std::string modelDir = argument;
    std::string polylines2D;
    std::string helix3D;
    if (argument.size() > 4 && argument.compare(argument.size() - 4, 4, ".vtk") == 0) {
        polylines2D = argument;
        const size_t slash = argument.find_last_of("/\\");
        modelDir = (slash == std::string::npos) ? std::string(".") : argument.substr(0, slash);
    } else {
        polylines2D = modelDir + "/Ribbon_Polylines2D.vtk";
    }
    helix3D = modelDir + "/Ribbon_Helix3D.vtk";
    const std::string mixedModel = modelDir + "/iGameValidateCellsFilter_mixed.vtk";

    std::cout << "      model A: " << polylines2D << std::endl;
    std::cout << "      model B: " << helix3D << std::endl;
    std::cout << "      mixed  : " << mixedModel << std::endl;

    TestSingleLineFrame();
    TestPolyLineBevel();
    TestRibbonAngle();
    TestAttributePropagation();
    TestPerCellMapping();
    TestDegenerateInputs();
    TestInvalidInputs(polylines2D);
    TestDefaultsAndInputNormals();
    TestPolylines2DModel(polylines2D);
    if (FileExists(helix3D)) {
        TestHelix3DModel(helix3D);
    } else {
        std::cout << "SKIP: case 10 (3D helix model not found)" << std::endl;
    }
    if (FileExists(mixedModel)) {
        TestMixedCellModel(mixedModel);
    } else {
        std::cout << "SKIP: case 11 (mixed cell model not found)" << std::endl;
    }
    TestModelPaths(usedDefaultArgument, polylines2D);

    std::cout << "===================================" << std::endl;
    if (g_FailureCount == 0) {
        std::cout << "PASS: RibbonFilter test finished, all checks passed" << std::endl;
        return 0;
    }
    std::cout << "FAIL: " << g_FailureCount << " check(s) failed" << std::endl;
    return 1;
}
