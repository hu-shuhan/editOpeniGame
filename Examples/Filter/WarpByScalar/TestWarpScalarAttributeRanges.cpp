// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/WarpByScalar/TestWarpScalarAttributeRanges.cpp
// 回归：Warp By Scalar 迁移漏带属性范围的延迟复制修复，首次只显示第一个
// 属性后执行 Warp，其他属性被复制成非空但零长度的 dataRange；点击它们
// 时读取空范围发生 0xC0000005（地址 0x10）。修复提交：待提交。
// 验证：未计算范围继续保持 nullptr，已有范围独立复制；所有标量/多分量/
// 张量均能取得完整范围并切换显示，原数据与属性数值保持不变。
#include <WarpByScalar/iGameWarpByScalarFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace iGame;
namespace {
void Check(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}

DataObject::Pointer Fixture() {
    auto mesh = UnstructuredMesh::New();
    auto points = Points::New();
    points->AddPoint(Point(0, 0, 0));
    points->AddPoint(Point(1, 0, 0));
    points->AddPoint(Point(0, 1, 0));
    mesh->SetPoints(points);
    igIndex ids[] = {0, 1, 2};
    mesh->AddCell(ids, 3, IG_TRIANGLE);
    const char* names[] = {"test_1", "test_2", "Position", "Curvature", "TEST_TENSOR"};
    const int dims[] = {3, 1, 3, 1, 9};
    for (int a = 0; a < 5; ++a) {
        auto data = FloatArray::New();
        data->SetName(names[a]);
        data->SetDimension(dims[a]);
        data->Resize(3);
        for (int p = 0; p < 3; ++p)
            for (int c = 0; c < dims[a]; ++c)
                data->SetValue(p * dims[a] + c, 1.0 + a + p + c);
        mesh->GetAttributeSet()->AddAttribute(a == 4 ? IG_TENSOR : IG_SCALAR, IG_POINT, data);
    }
    return mesh;
}

void Verify(DataObject::Pointer input) {
    Check(input != nullptr, "model cannot be read");
    auto attrs = input->GetAttributeSet();
    Check(attrs && attrs->GetNumberOfAttributes() >= 2, "need at least two attributes");
    // Reproduce the UI state: only the first field has been displayed.
    for (IGsize a = 0; a < attrs->GetNumberOfAttributes(); ++a)
        attrs->GetAttribute(a).dataRange = nullptr;
    attrs->GetAttribute(0).GetDataRange();
    std::string scalar;
    for (IGsize a = 0; a < attrs->GetNumberOfAttributes(); ++a) {
        auto& field = attrs->GetAttribute(a);
        if (!field.IsNone() && field.attachmentType == IG_POINT && field.pointer->GetDimension() == 1) {
            scalar = field.pointer->GetName();
            break;
        }
    }
    Check(!scalar.empty(), "no single-component point scalar");
    const auto originalPoint = input->GetPoints()->GetPoint(0);
    auto filter = WarpByScalar::New();
    filter->SetInput(input);
    filter->SetScalarsArrayName(scalar);
    filter->SetScaleFactor(0.1);
    filter->SetUseNormal(true);
    filter->SetNormal(0, 0, 1);
    Check(filter->Execute(), "Warp execution failed");
    auto output = filter->GetOutput();
    auto copied = output->GetAttributeSet();
    Check(copied->GetNumberOfAttributes() == attrs->GetNumberOfAttributes(), "attribute count changed");
    for (IGsize a = 0; a < attrs->GetNumberOfAttributes(); ++a) {
        auto& source = attrs->GetAttribute(a);
        auto& dest = copied->GetAttribute(a);
        Check(dest.pointer && dest.pointer != source.pointer, "array not independently copied");
        Check(dest.pointer->GetName() == source.pointer->GetName(), "attribute name changed");
        Check(dest.pointer->GetNumberOfValues() == source.pointer->GetNumberOfValues(), "array size changed");
        if (a == 0) {
            Check(dest.dataRange && dest.dataRange != source.dataRange, "cached range not independently copied");
        } else {
            Check(!dest.dataRange, "uncached attribute acquired an empty range: " + dest.pointer->GetName());
        }
        for (IGsize v = 0; v < source.pointer->GetNumberOfValues(); ++v)
            Check(dest.pointer->GetValue(v) == source.pointer->GetValue(v), "attribute value changed");
        auto range = dest.GetDataRange();
        auto expected = source.GetDataRange();
        Check(range && range->GetDimension() == 2 &&
              range->GetNumberOfElements() == dest.pointer->GetDimension() + 1, "range shape invalid");
        for (IGsize v = 0; v < range->GetNumberOfValues(); ++v)
            Check(std::isfinite(range->GetValue(v)) && range->GetValue(v) == expected->GetValue(v),
                  "copied range differs from original");
        std::cout << "PASS: " << dest.pointer->GetName() << " (" << dest.pointer->GetDimension()
                  << " components), lazy/cached ranges and values\n";
    }
    auto draw = DynamicCast<DrawObject>(output);
    auto scene = Scene::New();
    // Exercise the same selection dispatch used by ModelTreeWidgetItem.
    const auto modelId = scene->AddModel(draw);
    auto model = scene->GetModelById(modelId);
    for (IGsize a = 0; a < copied->GetNumberOfAttributes(); ++a) {
        model->ViewCloudPicture(static_cast<int>(a), -1);
        for (int dim = 0; dim < copied->GetAttribute(a).pointer->GetDimension(); ++dim)
            model->ViewCloudPicture(static_cast<int>(a), dim);
    }
    for (int dim = 0; dim < 3; ++dim)
        Check(input->GetPoints()->GetPoint(0)[dim] == originalPoint[dim], "input geometry changed");
    std::cout << "PASS: all attributes and components can be selected\n";
}
}

int main(int argc, char** argv) {
    try {
        Verify(Fixture());
        if (argc > 1) Verify(FileIO::ReadFile(argv[1]));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
