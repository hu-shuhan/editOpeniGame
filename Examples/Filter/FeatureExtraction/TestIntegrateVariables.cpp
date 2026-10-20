// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// 测试命名整理（待提交）：原文件 IntegrateVariables.cpp 未遵循 Test 前缀约定。
// 重命名为 TestIntegrateVariables.cpp；保持 testIntegrateVariables 目标与原有用例行为不变。
#include "IntegrateVariables/iGameIntegrateVariablesFilter.h"
#include "iGameAttributeSet.h"
#include "iGameCellType.h"
#include "iGameFileIO.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct CellSpec {
    IGenum type;
    std::vector<igIndex> points;
};

struct ExpectedArray {
    std::string name;
    IGenum attachment{IG_NONE};
    std::vector<double> values;
};

struct ExpectedResult {
    const char* name;
    int dimension;
    int cellCount;
    const char* measureName;
    double measure;
    std::array<double, 3> center;
    std::vector<ExpectedArray> arrays;
    bool divideCellData{false};
};

const std::vector<iGame::Point> kTetraPoints = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};

bool NearlyEqual(double actual, double expected) {
    const double scale = std::max(1.0, std::abs(expected));
    return std::abs(actual - expected) <= 1e-8 * scale;
}

iGame::UnstructuredMesh::Pointer MakeMesh(
        const std::vector<iGame::Point>& points,
        std::initializer_list<CellSpec> cells) {
    auto mesh = iGame::UnstructuredMesh::New();
    for (const auto& point : points) mesh->AddPoint(point);
    for (auto cell : cells) {
        mesh->AddCell(cell.points.data(), static_cast<int>(cell.points.size()),
                      cell.type);
    }
    return mesh;
}

iGame::SurfaceMesh::Pointer MakeSurface(
        const std::vector<iGame::Point>& points,
        std::initializer_list<std::vector<igIndex>> edges,
        std::initializer_list<std::vector<igIndex>> faces) {
    auto mesh = iGame::SurfaceMesh::New();
    auto meshPoints = iGame::Points::New();
    for (const auto& point : points) meshPoints->AddPoint(point);
    mesh->SetPoints(meshPoints);

    auto meshEdges = iGame::CellArray::New();
    for (const auto& edge : edges) {
        meshEdges->AddCellIds(edge.data(), static_cast<int>(edge.size()));
    }
    mesh->SetEdges(meshEdges);

    auto meshFaces = iGame::CellArray::New();
    for (const auto& face : faces) {
        meshFaces->AddCellIds(face.data(), static_cast<int>(face.size()));
    }
    mesh->SetFaces(meshFaces);
    return mesh;
}

void AddDoubleAttribute(iGame::DataObject* data, IGenum attachment,
                        const std::string& name,
                        const std::vector<double>& values) {
    auto array = iGame::DoubleArray::New();
    array->SetName(name);
    array->SetDimension(1);
    for (double value : values) array->AddValue(value);
    data->GetAttributeSet()->AddScalar(attachment, array);
}

void AddGhostAttribute(iGame::DataObject* data,
                       const std::vector<unsigned char>& values) {
    auto array = iGame::UnsignedCharArray::New();
    array->SetName("vtkGhostType");
    array->SetDimension(1);
    for (unsigned char value : values) array->AddValue(value);
    data->GetAttributeSet()->AddScalar(IG_CELL, array);
}

bool OutputArrayMatches(iGame::DataObject* output, const ExpectedArray& expected) {
    auto* attributes = output ? output->GetAttributeSet() : nullptr;
    if (!attributes) return false;
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.IsNone() || attribute.isDeleted || !attribute.pointer ||
            attribute.attachmentType != expected.attachment ||
            attribute.pointer->GetName() != expected.name ||
            attribute.pointer->GetDimension() !=
                    static_cast<int>(expected.values.size()) ||
            attribute.pointer->GetNumberOfElements() == 0) {
            continue;
        }
        for (size_t component = 0; component < expected.values.size(); ++component) {
            if (!NearlyEqual(attribute.pointer->GetElementValue(
                                     0, static_cast<int>(component)),
                             expected.values[component])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool ValidateResult(iGame::DataObject::Pointer input,
                    const ExpectedResult& expected) {
    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(input);
    filter->SetDivideAllCellDataByMeasure(expected.divideCellData);
    if (!filter->Execute()) {
        std::cerr << "[FAIL] " << expected.name << ": "
                  << filter->GetMessage() << '\n';
        return false;
    }

    const auto output = filter->GetOutput();
    bool valid = output &&
                 filter->GetIntegrationDimension() == expected.dimension &&
                 filter->GetIntegratedCellCount() == expected.cellCount &&
                 NearlyEqual(filter->GetIntegratedMeasure(), expected.measure) &&
                 OutputArrayMatches(output.GetPointer(),
                                    {expected.measureName, IG_CELL,
                                     {expected.measure}});

    auto points = output ? output->GetPoints() : nullptr;
    valid = valid && points && points->GetNumberOfPoints() == 1;
    if (points && points->GetNumberOfPoints() == 1) {
        const auto& center = points->GetPoint(0);
        for (size_t i = 0; i < expected.center.size(); ++i) {
            valid = valid && NearlyEqual(center[i], expected.center[i]);
        }
    }
    for (const auto& array : expected.arrays) {
        valid = OutputArrayMatches(output.GetPointer(), array) && valid;
    }
    if (!valid) {
        std::cerr << "[FAIL] " << expected.name << ": expected "
                  << expected.measureName << '=' << expected.measure
                  << ", dimension=" << expected.dimension
                  << ", cells=" << expected.cellCount << '\n';
    } else {
        std::cout << "[PASS] " << expected.name << ": "
                  << expected.measureName << '=' << expected.measure;
        for (const auto& array : expected.arrays) {
            std::cout << ", " << array.name << '=';
            if (array.values.size() == 1) {
                std::cout << array.values.front();
                continue;
            }
            std::cout << '[';
            for (size_t component = 0; component < array.values.size();
                 ++component) {
                if (component != 0) std::cout << ',';
                std::cout << array.values[component];
            }
            std::cout << ']';
        }
        std::cout << ", dimension=" << expected.dimension
                  << ", cells=" << expected.cellCount << '\n';
    }
    return valid;
}

bool ValidateFile(const char* fileName, const ExpectedResult& expected) {
    auto input = iGame::FileIO::ReadFile(fileName);
    if (!input) {
        std::cerr << "[FAIL] " << expected.name << ": cannot read "
                  << fileName << '\n';
        return false;
    }
    return ValidateResult(input, expected);
}

bool ValidateRejected(iGame::DataObject::Pointer input, int dimension,
                      const char* name) {
    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(input);
    const bool valid = !filter->Execute() && !filter->GetOutput() &&
                       filter->GetIntegrationDimension() == dimension &&
                       filter->GetIntegratedCellCount() == 0;
    if (valid) {
        std::cout << "[PASS] " << name << ": rejected as expected\n";
    } else {
        std::cerr << "[FAIL] " << name << ": input was not rejected\n";
    }
    return valid;
}

// 回归背景：旧测试只确认 Execute 成功，且原一维/三维模型已被上游删除，
// Length、Area、Volume 的权重、中心或属性积分退化时不会失败。用当前三个
// 线性模型锁定 ParaView 5.13.3/6.1.1 数值。
bool ValidateReferenceModels() {
    struct Case {
        const char* fileName;
        ExpectedResult expected;
    };
    const std::vector<Case> cases = {
            {"./Models/tube_test.vtk",
             {"line model", 1, 2, "Length", 48.75209166753643,
              {-1.2231513261795044, 1.2173531627190926e-10,
               1.7768487930297852},
              {{"t", IG_POINT, {24.376045818713163}},
               {"radius", IG_POINT, {11.486011407386906}}}}},
            {"./Models/OutlineCorners_Plane.vtk",
             {"area model", 2, 1, "Area", 48.0, {1.0, 1.0, 2.0}, {}}},
            {"./Models/test_mesh.vtk",
             {"volume model", 3, 1, "Volume", 1.0, {0.5, 0.5, 0.5},
              {{"Temperature", IG_POINT, {3.5}},
               {"Pressure", IG_POINT, {3.9999999999999996}},
               {"Velocity", IG_POINT,
                {0.058333334202567734, 0.07500000111758709,
                 0.07500000111758709}},
               {"Density", IG_CELL, {0.9999999999999999}},
               {"Material", IG_CELL, {99.99999999999997}}}}}};

    bool valid = true;
    for (const auto& test : cases) {
        valid = ValidateFile(test.fileName, test.expected) && valid;
    }
    return valid;
}

// 回归背景：StructuredMesh 的八节点局部点序与通用六面体不同，单位立方体
// 曾得到 Volume=-1，同时使属性积分变号。测试保证体积、中心和属性均为
// 正确的正值。
bool ValidateStructuredCellOrdering() {
    auto mesh = iGame::StructuredMesh::New();
    igIndex dimensions[3] = {2, 2, 2};
    mesh->SetDimensionSize(dimensions);
    auto points = iGame::Points::New();
    for (int k = 0; k < 2; ++k)
        for (int j = 0; j < 2; ++j)
            for (int i = 0; i < 2; ++i) points->AddPoint(i, j, k);
    mesh->SetPoints(points);
    AddDoubleAttribute(mesh, IG_POINT, "ConstantPointValue",
                       {2, 2, 2, 2, 2, 2, 2, 2});
    AddDoubleAttribute(mesh, IG_CELL, "ConstantCellValue", {10});
    return ValidateResult(
            mesh, {"structured ordering", 3, 1, "Volume", 1.0,
                   {0.5, 0.5, 0.5},
                   {{"ConstantPointValue", IG_POINT, {2.0}},
                    {"ConstantCellValue", IG_CELL, {10.0}}}});
}

// 回归背景：积分循环曾忽略 vtkGhostType，将 DUPLICATECELL(1) 和
// HIDDENCELL(32) 的重合三角形重复累加。只有普通单元应参与，结果必须为
// Area=0.5、Pressure=5。
bool ValidateGhostCellsAreSkipped() {
    auto mesh = MakeMesh({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
                         {{iGame::IG_TRIANGLE, {0, 1, 2}},
                          {iGame::IG_TRIANGLE, {0, 1, 2}},
                          {iGame::IG_TRIANGLE, {0, 1, 2}}});
    AddGhostAttribute(mesh, {0, 1, 32});
    AddDoubleAttribute(mesh, IG_CELL, "Pressure", {10, 12, 14});
    return ValidateResult(mesh, {"ghost exclusion", 2, 1, "Area", 0.5,
                                 {1.0 / 3.0, 1.0 / 3.0, 0.0},
                                 {{"Pressure", IG_CELL, {5.0}}}});
}

// 回归背景：非结构网格曾只按“维度+顶点数”选择三维拆分公式，会把
// IG_POLYHEDRON 当四面体、把六节点 IG_HEXAHEDRON 当三棱柱。正常四面体应成功；
// 任一可见最高维单元不支持或畸形时应整体失败。
bool ValidateCellTypes() {
    auto tetra = MakeMesh(kTetraPoints, {{iGame::IG_TETRA, {0, 1, 2, 3}}});
    AddDoubleAttribute(tetra, IG_CELL, "Pressure", {12});
    bool valid = ValidateResult(
            tetra, {"valid tetra", 3, 1, "Volume", 1.0 / 6.0,
                    {0.25, 0.25, 0.25}, {{"Pressure", IG_CELL, {2.0}}}});

    auto mixed = MakeMesh(kTetraPoints,
                          {{iGame::IG_POLYHEDRON, {0, 1, 2, 3}},
                           {iGame::IG_TETRA, {0, 1, 2, 3}}});
    valid = ValidateRejected(mixed, 3, "unsupported same-dimension cell") && valid;

    const std::vector<iGame::Point> sixPoints = {
            {0, 0, 0}, {1, 0, 0}, {0, 1, 0},
            {0, 0, 1}, {1, 0, 1}, {0, 1, 1}};
    auto malformed = MakeMesh(sixPoints,
                              {{iGame::IG_HEXAHEDRON, {0, 1, 2, 3, 4, 5}}});
    return ValidateRejected(malformed, 3, "malformed hexahedron") && valid;
}

// 回归背景：ghost 单元曾在选择最高维度前被过滤，可见三角形加隐藏四面体
// 会错误输出 Area=0.5。ParaView 仍由四面体决定维度，但不积分它，因此应输出
// Volume=0。
bool ValidateGhostSelectsHighestDimension() {
    auto mesh = MakeMesh(kTetraPoints,
                         {{iGame::IG_TRIANGLE, {0, 1, 2}},
                          {iGame::IG_TETRA, {0, 1, 2, 3}}});
    AddGhostAttribute(mesh, {0, 32});
    return ValidateResult(mesh, {"ghost dimension", 3, 0, "Volume", 0.0,
                                 {0.0, 0.0, 0.0}, {}});
}

// 回归背景：IG_PRISM 的旧四面体点序与 vtkWedge::TriangulateLocalIds(1)
// 方向相反，标准三棱柱得到 -0.5而 ParaView 为 +0.5。测试保证体积、中心和
// 常量属性积分一致。
bool ValidatePrism() {
    auto mesh = MakeMesh(
            {{0, 0, 0}, {0, 1, 0}, {1, 0, 0},
             {0, 0, 1}, {0, 1, 1}, {1, 0, 1}},
            {{iGame::IG_PRISM, {0, 1, 2, 3, 4, 5}}});
    AddDoubleAttribute(mesh, IG_POINT, "ConstantPointValue",
                       {2, 2, 2, 2, 2, 2});
    AddDoubleAttribute(mesh, IG_CELL, "ConstantCellValue", {10});
    return ValidateResult(
            mesh, {"prism", 3, 1, "Volume", 0.5,
                   {0.3333333432674408, 0.3333333432674408, 0.5},
                   {{"ConstantPointValue", IG_POINT, {1.0}},
                    {"ConstantCellValue", IG_CELL, {5.0}}}});
}

// 回归背景：IG_PYRAMID 曾按底面对角线长度动态拆分，但 ParaView 固定沿 1--3
// 拆分。扭曲底面时两者体积为 0.793333... 和 0.660000...，必须锁定后者及其
// 积分中心。
bool ValidatePyramid() {
    auto mesh = MakeMesh(
            {{0, 0, 0}, {2, 0, 0}, {2, 1, 0}, {0, 1, 0.4f},
             {0.7f, 0.4f, 1.2f}},
            {{iGame::IG_PYRAMID, {0, 1, 2, 3, 4}}});
    return ValidateResult(
            mesh, {"pyramid", 3, 1, "Volume", 0.6600000281135241,
                   {0.9123737215995789, 0.46868687868118286,
                    0.4000000059604645},
                   {}});
}

// 回归背景：最高维度曾只从“已支持单元”中选择，IG_POLYHEDRON 和三角形
// 混合时会静默回退并输出 Area=0.5。不支持的三维单元必须阻止低维积分并明确
// 失败。
bool ValidateUnsupportedCellDoesNotFallback() {
    auto mesh = MakeMesh(kTetraPoints,
                         {{iGame::IG_TRIANGLE, {0, 1, 2}},
                          {iGame::IG_POLYHEDRON, {0, 1, 2, 3}}});
    return ValidateRejected(mesh, 3, "unsupported highest dimension");
}

// 回归背景：SurfaceMesh 同时含 LINES/POLYGONS 时，旧代码用面内编号读取
// CELL_DATA，使面误用线的属性和 ghost。已知的“边、面”布局应正确偏移；当前边数
// 与属性数无法对应时必须失败，不得猜测。
bool ValidateSurfaceCellDataLayouts() {
    auto mixed = MakeSurface(
            {{0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
            {{0, 1}}, {{2, 3, 4}});
    AddGhostAttribute(mixed, {32, 0});
    AddDoubleAttribute(mixed, IG_CELL, "Pressure", {100, 10});
    bool valid = ValidateResult(
            mixed, {"mixed surface offset", 2, 1, "Area", 0.5,
                    {1.0 / 3.0, 1.0 / 3.0, 0.0},
                    {{"Pressure", IG_CELL, {5.0}}}});

    auto ambiguous = MakeSurface({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
                                 {{0, 1}, {1, 2}, {2, 0}}, {{0, 1, 2}});
    AddDoubleAttribute(ambiguous, IG_CELL, "Pressure", {100, 10});
    return ValidateRejected(ambiguous, 2, "ambiguous surface layout") && valid;
}

// 回归背景：同一 Filter 成功后再遇到无效输入，失败路径曾保留旧 output，
// 调用方会误用上次结果。每次 Execute 开始必须清空输出和统计状态。
bool ValidateFailedReexecutionClearsOutput() {
    auto mesh = MakeMesh({{0, 0, 0}, {1, 0, 0}},
                         {{iGame::IG_LINE, {0, 1}}});
    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute() || !filter->GetOutput()) return false;

    filter->SetInput(iGame::DataObject::Pointer{});
    const bool valid = !filter->Execute() && !filter->GetOutput() &&
                       filter->GetIntegrationDimension() == 0 &&
                       filter->GetIntegratedCellCount() == 0 &&
                       NearlyEqual(filter->GetIntegratedMeasure(), 0.0);
    if (valid) {
        std::cout << "[PASS] failed re-execution: stale output cleared\n";
    } else {
        std::cerr << "[FAIL] stale output was retained\n";
    }
    return valid;
}

// 回归背景：旧演示只打印 Divide All Cell Data By Measure 的结果，即使开关无效
// 也会退出成功。面积 0.5、Pressure=10 的三角形开启归一化后必须输出
// Pressure=10，而非原始积分值 5。
bool ValidateDivideCellDataByMeasure() {
    auto mesh = MakeMesh({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
                         {{iGame::IG_TRIANGLE, {0, 1, 2}}});
    AddDoubleAttribute(mesh, IG_CELL, "Pressure", {10});
    return ValidateResult(
            mesh, {"divide cell data", 2, 1, "Area", 0.5,
                   {1.0 / 3.0, 1.0 / 3.0, 0.0},
                   {{"Pressure", IG_CELL, {10.0}}}, true});
}

} // namespace

int main() {
    using Test = std::pair<const char*, bool (*)()>;
    const Test tests[] = {
            {"reference models", ValidateReferenceModels},
            {"structured ordering", ValidateStructuredCellOrdering},
            {"ghost exclusion", ValidateGhostCellsAreSkipped},
            {"cell types", ValidateCellTypes},
            {"ghost dimension", ValidateGhostSelectsHighestDimension},
            {"prism", ValidatePrism},
            {"pyramid", ValidatePyramid},
            {"unsupported fallback", ValidateUnsupportedCellDoesNotFallback},
            {"surface layouts", ValidateSurfaceCellDataLayouts},
            {"failed re-execution", ValidateFailedReexecutionClearsOutput},
            {"divide cell data", ValidateDivideCellDataByMeasure},
    };

    int failures = 0;
    for (const auto& test : tests) {
        if (!test.second()) {
            ++failures;
            std::cerr << "[FAILED TEST] " << test.first << '\n';
        }
    }
    if (failures == 0) {
        std::cout << "[PASS] IntegrateVariables: all test groups passed\n";
    }
    return failures == 0 ? 0 : 1;
}
