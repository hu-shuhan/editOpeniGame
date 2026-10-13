// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestMigratedStandardFilters.cpp
// 集成回归（待提交）：源仓库的部分示例只打开渲染窗口，不能自动发现
// 菜单迁移遗漏算法、LFS 模型缺失、属性丢失或输入被修改的问题。
// 使用实际迁移模型检查校验结果、位移数值、同名属性、封端与共享边细分。
// 边界：关闭封端、关闭全部追加属性、多代细分；这些分支不能只看输出是否非空。
#include <AppendAttributes/iGameAppendAttributesFilter.h>
#include <ValidateCells/iGameValidateCellsFilter.h>
#include <Subdivide/iGameSubdivide.h>
#include <Tube/iGameTube.h>
#include <WarpByScalar/iGameWarpByScalarFilter.h>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <cmath>
#include <iostream>

using namespace iGame;

namespace {
bool Check(bool value, const char* message) {
    std::cout << (value ? "PASS: " : "FAIL: ") << message << '\n';
    return value;
}

bool Validate() {
    auto input = FileIO::ReadFile("Models/iGameValidateCellsFilter_mixed.vtk");
    if (!Check(input != nullptr, "validate model readable")) return false;
    auto filter = ValidateCellsFilter::New();
    filter->SetInput(input);
    if (!Check(filter->Execute(), "validate execution")) return false;
    auto output = filter->GetOutput();
    return Check(output && output != input, "validate independent output") &&
           Check(filter->GetValidCellCount() > 0 && filter->GetInvalidCellCount() > 0,
                 "validate detects both valid and invalid cells") &&
           Check(output->GetAttributeSet()->GetAttributeIndex("ValidityState") >= 0,
                 "validate exports ValidityState") &&
           Check(input->GetAttributeSet()->GetAttributeIndex("ValidityState") < 0,
                 "validate preserves input attributes");
}

bool Warp() {
    auto input = FileIO::ReadFile("Models/warp_grid3d_normals.vtk");
    if (!Check(input != nullptr, "warp model readable")) return false;
    auto filter = WarpByScalar::New();
    filter->SetInput(input);
    filter->SetScalarsArrayName("height");
    filter->SetScaleFactor(2.0);
    filter->SetUseNormal(true);
    filter->SetNormal(0, 0, 1);
    if (!Check(filter->Execute(), "warp execution")) return false;
    auto output = filter->GetOutput();
    auto height = input->GetAttributeSet()->GetAttribute("height").pointer;
    if (!Check(output && height && output != input, "warp independent output")) return false;
    for (IGsize i = 0; i < input->GetPoints()->GetNumberOfPoints(); ++i) {
        auto before = input->GetPoints()->GetPoint(i);
        auto after = output->GetPoints()->GetPoint(i);
        if (std::abs(after[0] - before[0]) > 1e-5 ||
            std::abs(after[1] - before[1]) > 1e-5 ||
            std::abs(after[2] - before[2] - 2 * height->GetElementValue(i, 0)) > 1e-4)
            return Check(false, "warp scalar displacement values");
    }
    return Check(true, "warp scalar displacement values");
}

bool Append() {
    auto a = FileIO::ReadFile("Models/append_grid_a.vtk");
    auto b = FileIO::ReadFile("Models/append_grid_b.vtk");
    if (!Check(a && b, "append models readable")) return false;
    auto filter = AppendAttributes::New();
    filter->AddInput(a);
    filter->AddInput(b);
    if (!Check(filter->Execute(), "append execution")) return false;
    auto output = filter->GetOutput();
    auto attrs = output->GetAttributeSet();
    if (!Check(output != a && attrs->GetNumberOfAttributes() == 6,
               "append keeps both duplicate arrays and independent geometry")) return false;
    for (const auto* name : {"pressureA", "temperatureB", "matA", "matB"})
        if (!Check(attrs->GetAttributeIndex(name) >= 0, name)) return false;
    if (!Check(a->GetAttributeSet()->GetNumberOfAttributes() == 3 &&
               b->GetAttributeSet()->GetNumberOfAttributes() == 3,
               "append preserves inputs")) return false;
    filter->SetAppendPointData(false);
    filter->SetAppendCellData(false);
    return Check(!filter->Execute(), "append rejects all attributes disabled");
}

bool Tube() {
    auto input = UnstructuredMesh::New();
    auto points = Points::New();
    points->AddPoint(Point(0, 0, 0));
    points->AddPoint(Point(0, 0, 2));
    input->SetPoints(points);
    igIndex ids[] = {0, 1};
    input->AddCell(ids, 2, IG_LINE);
    auto filter = TubeFilter::New();
    filter->SetInput(input);
    filter->SetNumberOfSides(6);
    filter->SetRadius(0.25);
    filter->SetCapping(true);
    if (!Check(filter->Execute(), "tube vertical line execution")) return false;
    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!Check(output && output->GetNumberOfPoints() == 14 && output->GetNumberOfFaces() == 24,
               "tube closed topology")) return false;
    filter->SetCapping(false);
    if (!Check(filter->Execute(), "tube without caps execution")) return false;
    output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    return Check(output && output->GetNumberOfPoints() == 12 && output->GetNumberOfFaces() == 12,
                 "tube open topology");
}

bool Subdivide() {
    auto input = DynamicCast<SurfaceMesh>(FileIO::ReadFile("SubdivideModels/input.vtk"));
    if (!Check(input != nullptr, "subdivide attribute model readable")) return false;
    auto filter = SubdivideFilter::New();
    filter->SetInput(input);
    for (int level : {1, 2}) {
        filter->SetNumberOfSubdivisions(level);
        if (!Check(filter->Execute(), "subdivide execution")) return false;
        auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
        const int children = level == 1 ? 4 : 16;
        if (!Check(output && output->GetNumberOfPoints() == (level == 1 ? 9 : 25) &&
                   output->GetNumberOfFaces() == 2 * children, "subdivide shared midpoints")) return false;
        auto attrs = output->GetAttributeSet();
        if (!Check(attrs->GetAttributeIndex("RegionId") >= 0 &&
                   attrs->GetAttributeIndex("CellValue") >= 0, "subdivide inherits cell arrays")) return false;
        auto region = attrs->GetAttribute("RegionId").pointer;
        auto value = attrs->GetAttribute("CellValue").pointer;
        if (!Check(region->GetNumberOfElements() == 2 * children &&
                   value->GetNumberOfElements() == 2 * children, "subdivide cell tuple counts")) return false;
        for (int i = 0; i < 2 * children; ++i)
            if (region->GetElementValue(i, 0) != (i < children ? 11 : 22) ||
                value->GetElementValue(i, 0) != (i < children ? 100 : 200))
                return Check(false, "subdivide parent cell values");
    }
    return Check(input->GetNumberOfPoints() == 4 && input->GetNumberOfFaces() == 2,
                 "subdivide preserves input");
}
}

int main() {
    const bool validate = Validate(), warp = Warp(), append = Append(),
               tube = Tube(), subdivide = Subdivide();
    return validate && warp && append && tube && subdivide ? 0 : 1;
}
