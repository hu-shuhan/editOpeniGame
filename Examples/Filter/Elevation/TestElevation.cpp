#include <Elevation/iGameElevationFilter.h>

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFileIO.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"

#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace iGame;

// ===== 模型文件路径（相对构建目录；Examples/CMakeLists.txt 会把 Models/ 复制过去）=====
std::string SlopeModelPath = "Models/ElevationSlopeTerrain.vtk";
std::string TerracesModelPath = "Models/ElevationTerraces.vtk";

// 主测试网格：4 点 2 三角形，z = x + 2y + dz（斜面）
//   p0(0,0,dz)  p1(1,0,1+dz)  p2(0,1,2+dz)  p3(1,1,3+dz)
// 默认标尺（低点 (0,0,0)、高点 (0,0,1)）下 t = clamp(z, 0, 1)
// dz 偏移参数用于验证绝对标尺语义：标尺不变、平移点位 -> 输出改变
SurfaceMesh::Pointer MakeSlopeMesh(double dz = 0.0) {
    auto points = Points::New();
    points->AddPoint(0.f, 0.f, static_cast<float>(dz));
    points->AddPoint(1.f, 0.f, static_cast<float>(1.0 + dz));
    points->AddPoint(0.f, 1.f, static_cast<float>(2.0 + dz));
    points->AddPoint(1.f, 1.f, static_cast<float>(3.0 + dz));

    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(1, 3, 2);

    auto mesh = SurfaceMesh::New();
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    return mesh;
}

// 平面网格：所有点 z = 5（投影恒定，验证饱和 / 居中行为）
SurfaceMesh::Pointer MakeFlatMesh() {
    auto points = Points::New();
    points->AddPoint(0.f, 0.f, 5.f);
    points->AddPoint(1.f, 0.f, 5.f);
    points->AddPoint(0.f, 1.f, 5.f);
    points->AddPoint(1.f, 1.f, 5.f);

    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(1, 3, 2);

    auto mesh = SurfaceMesh::New();
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    return mesh;
}

void Check(bool condition, const std::string& message) {
    if (!condition) { throw std::runtime_error(message); }
}

FloatArray::Pointer FindElevationArray(DataObject::Pointer object) {
    if (!object || !object->GetAttributeSet()) { return nullptr; }
    auto* attributes = object->GetAttributeSet();
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.isDeleted || !attribute.pointer) { continue; }
        if (attribute.attachmentType != IG_POINT) { continue; }
        if (attribute.pointer->GetName() == "Elevation") {
            return DynamicCast<FloatArray>(attribute.pointer);
        }
    }
    return nullptr;
}

// 找到输出对象上的 Elevation 属性（返回 Attribute 本体，用于读取 dataRange）
const AttributeSet::Attribute* FindElevationAttribute(DataObject::Pointer object) {
    if (!object || !object->GetAttributeSet()) { return nullptr; }
    auto* attributes = object->GetAttributeSet();
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.isDeleted || !attribute.pointer) { continue; }
        if (attribute.attachmentType != IG_POINT) { continue; }
        if (attribute.pointer->GetName() == "Elevation") { return &attribute; }
    }
    return nullptr;
}

// 独立输出通用断言：输出非空、与输入不同对象、输入未被 Elevation 数组污染、点数一致
void CheckIndependentOutput(const ElevationFilter::Pointer& filter,
                            const PointSet::Pointer& mesh, const std::string& label) {
    auto output = filter->GetOutput();
    Check(output != nullptr, label + ": filter output is missing.");
    Check(output.GetPointer() != mesh.GetPointer(),
          label + ": output must be an independent object, not the input itself.");
    Check(FindElevationArray(mesh) == nullptr,
          label + ": input mesh must not be polluted with the Elevation array.");
    auto outputMesh = DynamicCast<PointSet>(output);
    Check(outputMesh != nullptr, label + ": output must be a PointSet.");
    Check(outputMesh->GetNumberOfPoints() == mesh->GetNumberOfPoints(),
          label + ": output must keep the same number of points.");
}

void CheckValues(const FloatArray::Pointer& array, const std::vector<double>& expected,
                 const std::string& label) {
    Check(array != nullptr, label + ": Elevation array is missing.");
    Check(array->GetDimension() == 1, label + ": dimension must be 1.");
    Check(array->GetNumberOfElements() == expected.size(),
          label + ": unexpected element count.");
    const float* values = array->RawPointer();
    for (IGsize i = 0; i < expected.size(); ++i) {
        if (std::fabs(values[i] - expected[i]) > 1e-6) {
            throw std::runtime_error(label + ": value " + std::to_string(i) +
                                     " is " + std::to_string(values[i]) +
                                     ", expected " + std::to_string(expected[i]));
        }
    }
}

// 读取模型文件并转型为 PointSet（ElevationFilter 的输入类型）
PointSet::Pointer LoadPointSetModel(const std::string& path) {
    auto object = FileIO::ReadFile(path);
    Check(object != nullptr, "failed to read model file: " + path);
    auto pointSet = DynamicCast<PointSet>(object);
    Check(pointSet != nullptr, path + " is not a PointSet-compatible mesh.");
    return pointSet;
}

// 用例 1：默认参数（低点 (0,0,0)、高点 (0,0,1)、范围 [0,1]）——
// v = (0,0,1)，t = clamp(z, 0, 1) = 0,1,1,1 -> 输出 {0, 1, 1, 1}
void TestDefaultAxisMapping() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed.");
    CheckIndependentOutput(filter, mesh, "default axis mapping");
    CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 1.0, 1.0, 1.0},
                "default axis mapping");
}

// 用例 2：标量范围映射——低点 (0,0,0)、高点 (0,0,3)、范围 [0,10]：
// v = (0,0,3)，t = clamp(z/3, 0, 1) = 0,1/3,2/3,1 -> 输出 = 10t = {0, 10/3, 20/3, 10}
void TestScalarRangeMapping() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    filter->SetLowPoint(0.0, 0.0, 0.0);
    filter->SetHighPoint(0.0, 0.0, 3.0);
    filter->SetScalarRange(0.0, 10.0);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed.");
    CheckIndependentOutput(filter, mesh, "scalar range mapping");
    CheckValues(FindElevationArray(filter->GetOutput()),
                {0.0, 10.0 / 3.0, 20.0 / 3.0, 10.0}, "scalar range mapping");
}

// 用例 3：双向饱和——低点 (0,0,1)、高点 (0,0,2)、范围 [0,1]：
// t = clamp(z - 1, 0, 1) = -1,0,1,2 -> 夹断输出 {0, 0, 1, 1}
void TestBidirectionalClamping() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    filter->SetLowPoint(0.0, 0.0, 1.0);
    filter->SetHighPoint(0.0, 0.0, 2.0);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed.");
    CheckIndependentOutput(filter, mesh, "bidirectional clamping");
    CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 0.0, 1.0, 1.0},
                "bidirectional clamping");
}

// 用例 4：任意轴 + 线段参数化——低点 (0,0,0)、高点 (1,1,0)、范围 [0,1]：
// v = (1,1,0)，|v|^2 = 2，t = clamp((x+y)/2, 0, 1) = 0, 0.5, 0.5, 1
// 对照高点 (2,2,0)：t = clamp((x+y)/4) = 0, 0.25, 0.25, 0.5 ——
// t 按线段长度参数化（非单位投影）：线段变长、刻度变疏（vtkElevationFilter 语义）
void TestArbitraryAxis() {
    {
        auto mesh = MakeSlopeMesh();
        auto filter = ElevationFilter::New();
        filter->SetLowPoint(0.0, 0.0, 0.0);
        filter->SetHighPoint(1.0, 1.0, 0.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed.");
        CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 0.5, 0.5, 1.0},
                    "arbitrary axis");
    }
    {
        auto mesh = MakeSlopeMesh();
        auto filter = ElevationFilter::New();
        filter->SetLowPoint(0.0, 0.0, 0.0);
        filter->SetHighPoint(2.0, 2.0, 0.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed.");
        CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 0.25, 0.25, 0.5},
                    "arbitrary axis rescaled segment");
    }
}

// 用例 5：垂直分量归零——低/高点 (0,0,0)-(0,0,1) 与 (5,7,0)-(5,7,1) 输出一致：
// 点积 (p - Low)·v 中，低/高点垂直于线段方向的分量被差值消去，仅沿方向分量有效
void TestVerticalOffsetIrrelevance() {
    auto meshA = MakeSlopeMesh();
    auto filterA = ElevationFilter::New();
    filterA->SetLowPoint(0.0, 0.0, 0.0);
    filterA->SetHighPoint(0.0, 0.0, 1.0);
    filterA->SetInput(meshA);
    Check(filterA->Execute(), "Execute should succeed.");

    auto meshB = MakeSlopeMesh();
    auto filterB = ElevationFilter::New();
    filterB->SetLowPoint(5.0, 7.0, 0.0);
    filterB->SetHighPoint(5.0, 7.0, 1.0);
    filterB->SetInput(meshB);
    Check(filterB->Execute(), "Execute should succeed.");

    auto arrayA = FindElevationArray(filterA->GetOutput());
    auto arrayB = FindElevationArray(filterB->GetOutput());
    Check(arrayA && arrayB, "both arrays must exist.");
    const float* a = arrayA->RawPointer();
    const float* b = arrayB->RawPointer();
    const IGsize count = arrayA->GetNumberOfElements();
    for (IGsize i = 0; i < count; ++i) {
        Check(std::fabs(a[i] - b[i]) < 1e-6,
              "vertical offset of low/high points must not change the output.");
    }
}

// 用例 6：平面网格 z=5（投影恒定）——按标尺公式正常计算，任何情况都不产生 NaN：
// 低/高点 (0,0,0)-(0,0,1)：t = clamp(5, 0, 1) = 1 -> 全 1（高于高点饱和）
// 低/高点 (0,0,5)-(0,0,10)：t = 0                -> 全 0（等于低点）
// 低/高点 (0,0,4)-(0,0,6)：t = (5-4)/2 = 0.5     -> 全 0.5（居中）
void TestFlatMesh() {
    {
        auto mesh = MakeFlatMesh();
        auto filter = ElevationFilter::New();
        filter->SetLowPoint(0.0, 0.0, 0.0);
        filter->SetHighPoint(0.0, 0.0, 1.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed on flat mesh.");
        CheckIndependentOutput(filter, mesh, "flat mesh above high point");
        CheckValues(FindElevationArray(filter->GetOutput()), {1.0, 1.0, 1.0, 1.0},
                    "flat mesh above high point");
    }
    {
        auto mesh = MakeFlatMesh();
        auto filter = ElevationFilter::New();
        filter->SetLowPoint(0.0, 0.0, 5.0);
        filter->SetHighPoint(0.0, 0.0, 10.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed on flat mesh.");
        CheckIndependentOutput(filter, mesh, "flat mesh at low point");
        CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 0.0, 0.0, 0.0},
                    "flat mesh at low point");
    }
    {
        auto mesh = MakeFlatMesh();
        auto filter = ElevationFilter::New();
        filter->SetLowPoint(0.0, 0.0, 4.0);
        filter->SetHighPoint(0.0, 0.0, 6.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed on flat mesh.");
        CheckIndependentOutput(filter, mesh, "flat mesh centered");
        CheckValues(FindElevationArray(filter->GetOutput()), {0.5, 0.5, 0.5, 0.5},
                    "flat mesh centered");
    }
}

// 用例 7：非法输入防御——非法标量范围被拒绝且保持默认值；低/高点重合时 Execute 被拒绝；
// getter 能读回所设参数（内部 float 存储，容差比较）
void TestInvalidInputs() {
    auto filter = ElevationFilter::New();
    filter->SetScalarRange(1.0, 0.0);  // 下限 >= 上限：拒绝
    Check(filter->GetScalarRangeLow() == 0.0 && filter->GetScalarRangeHigh() == 1.0,
          "invalid scalar range must be rejected.");
    filter->SetScalarRange(5.0, 5.0);  // 空范围：拒绝
    Check(filter->GetScalarRangeLow() == 0.0 && filter->GetScalarRangeHigh() == 1.0,
          "empty scalar range must be rejected.");

    filter->SetLowPoint(1.5, 2.5, 3.5);
    filter->SetHighPoint(-1.0, 0.0, 4.0);
    Check(std::fabs(filter->GetLowPoint()[0] - 1.5) < 1e-5 &&
              std::fabs(filter->GetLowPoint()[1] - 2.5) < 1e-5 &&
              std::fabs(filter->GetLowPoint()[2] - 3.5) < 1e-5,
          "GetLowPoint must read back the value set.");
    Check(std::fabs(filter->GetHighPoint()[0] + 1.0) < 1e-5 &&
              std::fabs(filter->GetHighPoint()[2] - 4.0) < 1e-5,
          "GetHighPoint must read back the value set.");

    // 低点与高点重合 -> 投影方向为零向量，Execute 被拒绝
    auto coincident = ElevationFilter::New();
    coincident->SetLowPoint(1.0, 2.0, 3.0);
    coincident->SetHighPoint(1.0, 2.0, 3.0);
    coincident->SetInput(MakeSlopeMesh());
    Check(!coincident->Execute(), "coincident low/high points must be rejected.");
}

// 用例 8：绝对标尺语义（实验室需求核心）——标尺固定（低点 (0,0,0)、高点 (0,0,3)），
// 仅平移点位 -> 输出改变（t = clamp(z/3, 0, 1)）：
//   meshA(dz=0)： z = 0,1,2,3  -> t = {0, 1/3, 2/3, 1}
//   meshB(dz=+3)：z = 3,4,5,6  -> t 全部 >= 1 -> 饱和 {1,1,1,1}
//   meshC(dz=-1)：z = -1,0,1,2 -> t = {-1/3, 0, 1/3, 2/3} -> 夹断 {0, 0, 1/3, 2/3}
// 三个网格形状相同、仅位置不同；同一标尺下输出各不相同——证明标尺不随数据自适应，
// 即"标尺不变、改变 point，颜色会改变"（与 ParaView 行为一致）
void TestAbsoluteRuler() {
    {
        auto mesh = MakeSlopeMesh(0.0);
        auto filter = ElevationFilter::New();
        filter->SetHighPoint(0.0, 0.0, 3.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed.");
        CheckValues(FindElevationArray(filter->GetOutput()),
                    {0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0}, "absolute ruler dz=0");
    }
    {
        auto mesh = MakeSlopeMesh(3.0);
        auto filter = ElevationFilter::New();
        filter->SetHighPoint(0.0, 0.0, 3.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed.");
        CheckValues(FindElevationArray(filter->GetOutput()), {1.0, 1.0, 1.0, 1.0},
                    "absolute ruler dz=+3 (all above high point)");
    }
    {
        auto mesh = MakeSlopeMesh(-1.0);
        auto filter = ElevationFilter::New();
        filter->SetHighPoint(0.0, 0.0, 3.0);
        filter->SetInput(mesh);
        Check(filter->Execute(), "Execute should succeed.");
        CheckValues(FindElevationArray(filter->GetOutput()),
                    {0.0, 0.0, 1.0 / 3.0, 2.0 / 3.0},
                    "absolute ruler dz=-1 (first point below low point)");
    }
}

// 用例 9：斜坡地形模型 + 默认参数（低点 (0,0,0)、高点 (0,0,1)、范围 [0,1]）
// 模型 ElevationSlopeTerrain.vtk：11x11 顶点，x,y ∈ {0..10}，z = 0.1*(x+y)
// 期望 elevation = clamp(0.1*(x+y), 0, 1)（x+y > 10 的区域饱和在 1）
void TestSlopeModelClampedMapping() {
    auto mesh = LoadPointSetModel(SlopeModelPath);
    Check(mesh->GetNumberOfPoints() == 121, "slope model must have 121 points.");

    auto filter = ElevationFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed on the slope model.");
    CheckIndependentOutput(filter, mesh, "slope model");

    auto array = FindElevationArray(filter->GetOutput());
    Check(array != nullptr, "slope model: Elevation array is missing.");
    Check(array->GetNumberOfElements() == 121, "slope model: unexpected element count.");

    const float* values = array->RawPointer();
    for (IGsize i = 0; i < 121; ++i) {
        const Point& p = mesh->GetPoint(i);
        double expected = 0.1 * (p[0] + p[1]);
        if (expected < 0.0) { expected = 0.0; }
        else if (expected > 1.0) { expected = 1.0; }
        if (std::fabs(values[i] - expected) > 1e-5) {
            throw std::runtime_error("slope model: point " + std::to_string(i) +
                                     " elevation is " + std::to_string(values[i]) +
                                     ", expected " + std::to_string(expected));
        }
    }
    // 端点钉扎：首点 (0,0) -> 0；远端 (10,10) 处 z = 2 > 高点 -> 饱和 1
    Check(std::fabs(values[0]) < 1e-6, "slope model: first point must map to 0.");
    Check(std::fabs(values[120] - 1.0) < 1e-6,
          "slope model: last point must saturate at 1.");
}

// 用例 10：梯田地形模型 + 低点 (0,0,0)、高点 (0,0,5)、标量范围 [0,10]
// 模型 ElevationTerraces.vtk：11x11 顶点，z = floor((x+y)/4) ∈ {0..5} 六层台阶
// t = clamp(z/5, 0, 1) = z/5，输出 = 10t ∈ {0, 2, 4, 6, 8, 10} 六个离散值
void TestTerracesModelScalarRange() {
    auto mesh = LoadPointSetModel(TerracesModelPath);
    Check(mesh->GetNumberOfPoints() == 121, "terraces model must have 121 points.");

    auto filter = ElevationFilter::New();
    filter->SetLowPoint(0.0, 0.0, 0.0);
    filter->SetHighPoint(0.0, 0.0, 5.0);
    filter->SetScalarRange(0.0, 10.0);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed on the terraces model.");
    CheckIndependentOutput(filter, mesh, "terraces model");

    auto array = FindElevationArray(filter->GetOutput());
    Check(array != nullptr, "terraces model: Elevation array is missing.");
    Check(array->GetNumberOfElements() == 121, "terraces model: unexpected element count.");

    const float* values = array->RawPointer();
    std::set<double> distinct;
    for (IGsize i = 0; i < 121; ++i) {
        const Point& p = mesh->GetPoint(i);
        const double expected = 10.0 * std::floor((p[0] + p[1]) / 4.0) / 5.0;
        if (std::fabs(values[i] - expected) > 1e-4) {
            throw std::runtime_error("terraces model: point " + std::to_string(i) +
                                     " elevation is " + std::to_string(values[i]) +
                                     ", expected " + std::to_string(expected));
        }
        distinct.insert(values[i]);
    }
    Check(distinct.size() == 6, "terraces model: expected exactly six terrace levels.");
}

// 用例 11：独立输出 + 输出复用——输出为新对象、输入不被污染、几何与输入一致、
// 类型保持；同一输入重复执行复用同一输出对象（模型树不堆节点），
// 复用时仅替换数组指针，属性集中始终只有一个 Elevation 数组
void TestIndependentOutputAndReuse() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed.");

    auto output = filter->GetOutput();
    Check(output != nullptr, "output must not be null.");
    Check(output.GetPointer() != mesh.GetPointer(),
          "output must be a different object from input.");
    Check(FindElevationArray(mesh) == nullptr,
          "input must not be polluted with the Elevation array.");
    Check(FindElevationArray(output) != nullptr,
          "output must contain the Elevation array.");

    // 类型保持：SurfaceMesh 输入 -> SurfaceMesh 输出
    Check(DynamicCast<SurfaceMesh>(output) != nullptr,
          "SurfaceMesh input should yield SurfaceMesh output.");

    // 几何一致：输出共享输入的点几何
    auto outputMesh = DynamicCast<PointSet>(output);
    Check(outputMesh->GetNumberOfPoints() == mesh->GetNumberOfPoints(),
          "output must keep the same number of points.");
    for (IGsize i = 0; i < mesh->GetNumberOfPoints(); ++i) {
        Check((outputMesh->GetPoint(i) - mesh->GetPoint(i)).norm() < 1e-6,
              "output geometry must match input geometry.");
    }

    // 重复执行（同一输入对象）：复用同一输出对象，就地替换数组
    Check(filter->Execute(), "second Execute should succeed.");
    auto output2 = filter->GetOutput();
    Check(output2.GetPointer() == output.GetPointer(),
          "repeated Execute on the same input must reuse the output object.");
    Check(FindElevationArray(mesh) == nullptr, "input must remain unpolluted after re-run.");

    int elevationCount = 0;
    auto* attributes = output2->GetAttributeSet();
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.isDeleted || !attribute.pointer) { continue; }
        if (attribute.pointer->GetName() == "Elevation") { ++elevationCount; }
    }
    Check(elevationCount == 1, "reused output must contain exactly one Elevation array.");
}

// 用例 12：grow-only 取色范围——dataRange 首次挂载数据实际范围，之后只扩不缩：
//   第 1 次 范围 [0,1]： 输出 {0,1,1,1}，实际数据范围 [0,1] -> dataRange {0,1}
//   第 2 次 范围 [0,10]：输出 {0,10,10,10}，实际 [0,10]   -> dataRange 扩张为 {0,10}
//   第 3 次 范围 [0,1]： 输出 {0,1,1,1}，实际 [0,1]       -> dataRange 保持 {0,10}（只扩不缩）
// 与 ParaView 颜色条行为一致：标量范围改大再改小，颜色条保持历史最大范围
void TestGrowOnlyDataRange() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    filter->SetLowPoint(0.0, 0.0, 0.0);
    filter->SetHighPoint(0.0, 0.0, 1.0);

    // 读取 dataRange 的扁平 4 值（行 0 模长范围 / 行 1 分量范围，两行同填相同值）
    auto readRange = [&filter]() -> std::array<double, 4> {
        const auto* attribute = FindElevationAttribute(filter->GetOutput());
        Check(attribute != nullptr, "grow-only: attribute is missing.");
        auto range = attribute->dataRange;
        Check(range != nullptr, "grow-only: explicit dataRange must be attached.");
        return {range->GetValue(0), range->GetValue(1), range->GetValue(2), range->GetValue(3)};
    };

    filter->SetScalarRange(0.0, 1.0);
    filter->SetInput(mesh);
    Check(filter->Execute(), "first Execute should succeed.");
    CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 1.0, 1.0, 1.0},
                "grow-only first run");
    {
        const auto r = readRange();
        Check(std::fabs(r[0]) < 1e-9 && std::fabs(r[1] - 1.0) < 1e-9 &&
                  std::fabs(r[2]) < 1e-9 && std::fabs(r[3] - 1.0) < 1e-9,
              "grow-only: first dataRange must be [0,1].");
    }

    filter->SetScalarRange(0.0, 10.0);
    Check(filter->Execute(), "second Execute should succeed.");
    CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 10.0, 10.0, 10.0},
                "grow-only second run");
    {
        const auto r = readRange();
        Check(std::fabs(r[0]) < 1e-9 && std::fabs(r[1] - 10.0) < 1e-9 &&
                  std::fabs(r[2]) < 1e-9 && std::fabs(r[3] - 10.0) < 1e-9,
              "grow-only: dataRange must expand to [0,10].");
    }

    filter->SetScalarRange(0.0, 1.0);
    Check(filter->Execute(), "third Execute should succeed.");
    CheckValues(FindElevationArray(filter->GetOutput()), {0.0, 1.0, 1.0, 1.0},
                "grow-only third run");
    {
        const auto r = readRange();
        Check(std::fabs(r[0]) < 1e-9 && std::fabs(r[1] - 10.0) < 1e-9 &&
                  std::fabs(r[2]) < 1e-9 && std::fabs(r[3] - 10.0) < 1e-9,
              "grow-only: dataRange must keep the historical maximum [0,10] after shrinking.");
    }

    // 范围锁定验证（UI 链路等价复现）：属性已锁 rangeLocked，
    // 即使外部触发 UpdateAllDataRange（"每帧调整"式按数据重算）也不覆盖 grow-only 范围
    {
        auto attrs = filter->GetOutput()->GetAttributeSet();
        const int idx = (attrs != nullptr) ? attrs->GetAttributeIndex("Elevation") : -1;
        Check(idx >= 0, "grow-only: attribute index must be valid.");
        auto& attr = attrs->GetAttribute(idx);
        Check(attr.rangeLocked, "grow-only: Elevation attribute must be range-locked.");
        Check(attr.rangeMode == AttributeSet::RangeMode::ExpandOnly,
              "grow-only: range mode must be ExpandOnly.");
        Check(attr.UpdateAllDataRange(),
              "grow-only: locked UpdateAllDataRange should succeed keeping the range.");
        const auto r = readRange();
        Check(std::fabs(r[0]) < 1e-9 && std::fabs(r[1] - 10.0) < 1e-9 &&
                  std::fabs(r[2]) < 1e-9 && std::fabs(r[3] - 10.0) < 1e-9,
              "grow-only: locked range must survive UpdateAllDataRange without shrinking.");
    }
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, void (*)()>> tests = {
        {"TestDefaultAxisMapping", TestDefaultAxisMapping},
        {"TestScalarRangeMapping", TestScalarRangeMapping},
        {"TestBidirectionalClamping", TestBidirectionalClamping},
        {"TestArbitraryAxis", TestArbitraryAxis},
        {"TestVerticalOffsetIrrelevance", TestVerticalOffsetIrrelevance},
        {"TestFlatMesh", TestFlatMesh},
        {"TestInvalidInputs", TestInvalidInputs},
        {"TestAbsoluteRuler", TestAbsoluteRuler},
        {"TestSlopeModelClampedMapping", TestSlopeModelClampedMapping},
        {"TestTerracesModelScalarRange", TestTerracesModelScalarRange},
        {"TestIndependentOutputAndReuse", TestIndependentOutputAndReuse},
        {"TestGrowOnlyDataRange", TestGrowOnlyDataRange},
    };

    int failed = 0;
    for (const auto& [name, fn] : tests) {
        try {
            fn();
            std::cout << "[PASS] " << name << std::endl;
        } catch (const std::exception& e) {
            ++failed;
            std::cout << "[FAIL] " << name << ": " << e.what() << std::endl;
        }
    }

    if (failed == 0) {
        std::cout << "All " << tests.size() << " ElevationFilter tests passed." << std::endl;
    } else {
        std::cout << failed << " of " << tests.size()
                  << " ElevationFilter tests failed." << std::endl;
    }
    return failed == 0 ? 0 : 1;
}
