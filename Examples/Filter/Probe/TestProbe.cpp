// Source: dayuwan77/igamevis at eccac729b57aeacbe9312d7d5189f6990bb4eebd.
// Integration gap: e7ec6571 imported the filter but omitted its example.
// Preserve the numerical/attribute checks below and reject invalid inputs;
// visual examples support --no-render so CI requires a real exit status.
// Integration commit: test: add examples for first-batch standard filters
// Find it: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Probe/TestProbe.cpp
// ============================================================================
// TestProbe — ProbeFilter 自动探测测试（模型与参数写死，无需手动输入）
//
// 模型与探测参数全部写死在代码中，运行后自动完成测试：
//   模型:  ./Models/AIGen_Hex_PipeSegment.vtk   （六面体体网格）
//   探测:  以管道壁内一个六面体单元中心附近为球心 (1.385819, 0.574025, 0.75)、
//          0.1 为半径的球体内均匀随机采样 10 个查询点（seed 固定为 42）
//
// 流程:
//   1. 读取模型，打印模型规模与点属性清单。
//   2. 在球心 (1.385819, 0.574025, 0.75)、半径 0.1 的球体内生成 10 个查询点。
//   3. 执行 ProbeFilter：对查询点做单元定位 + 点属性线性插值，输出
//      ValidPointMask（命中单元=1，未命中=0），结果原地写回查询点集。
//   4. 逐点打印坐标、有效标记与各插值属性，终端输出 Result: PASS/FAIL。
// ============================================================================
#include <Probe/iGameProbeFilter.h>

#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>

#include <iomanip>
#include <cmath>
#include <iostream>
#include <string>

using namespace iGame;

namespace {

void PrintModelSummary(const DataObject::Pointer& model) {
    if (model.IsNull()) return;

    const IGsize numPoints =
            model->GetPoints().IsNull()
                    ? 0
                    : model->GetPoints()->GetNumberOfPoints();
    const IGsize numCells =
            model->GetCellArray().IsNull()
                    ? 0
                    : model->GetCellArray()->GetNumberOfCells();
    std::cout << "Model points=" << numPoints << " cells=" << numCells << "\n";

    const BoundingBox& bbox = model->GetBoundingBox();
    if (!bbox.isNull() && !bbox.isEmpty()) {
        const Vector3d c = bbox.center();
        std::cout << "Bounding box center=(" << c[0] << ", " << c[1] << ", "
                  << c[2] << ") diag=" << bbox.diag() << "\n";
    }

    AttributeSet* attrs = model->GetAttributeSet();
    if (attrs == nullptr) return;
    auto pointAttrs = attrs->GetAllPointAttributes();
    std::cout << "Point attributes: ";
    bool first = true;
    for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
        auto& attr = pointAttrs->GetElement(i);
        if (attr.isDeleted || attr.pointer.IsNull()) continue;
        if (!first) std::cout << ", ";
        first = false;
        std::cout << attr.pointer->GetName()
                  << "[dim=" << attr.pointer->GetDimension() << "]";
    }
    std::cout << "\n";
}

// radius == 0 时全部点落在球心本身；否则按球体体积均匀随机采样 n 个点。
bool MakeQueryPoints(PointSet::Pointer query, const Point& center, float radius,
                     int count, unsigned seed) {
    if (query.IsNull()) return false;
    auto points = query->GetPoints();
    if (points.IsNull()) return false;
    if (count <= 0) return false;

    if (radius <= 0.0f) {
        // GenerateSpherePoints 在 radius<=0 时直接返回、不生成任何点，
        // 因此这里手动把 n 个查询点全部放到球心。
        points->Reset();
        for (int i = 0; i < count; ++i) {
            points->AddPoint(center);
        }
        query->Modified();
        return points->GetNumberOfPoints() > 0;
    }

    ProbeFilter::GenerateSpherePoints(query, center, radius, count, seed);
    return points->GetNumberOfPoints() > 0;
}

// 打印一个数组元素的所有分量（以及分量数 > 1 时的模）。
void PrintArrayElement(ArrayObject* array, IGsize queryId) {
    if (array == nullptr) return;
    const int dim = array->GetDimension();
    std::cout << "(";
    for (int c = 0; c < dim; ++c) {
        if (c > 0) std::cout << ", ";
        std::cout << array->GetElementValue(queryId, c);
    }
    std::cout << ")";
    if (dim > 1) {
        std::cout << " mag=" << array->GetElementValue(queryId, -1);
    }
}

bool RunProbe(const std::string& modelFile, const Point& center, float radius,
              int count, unsigned seed) {
    std::cout << "Model: " << modelFile << "\n";
    auto model = FileIO::ReadFile(modelFile);
    if (model.IsNull()) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "Read model failed\n";
        return false;
    }
    PrintModelSummary(model);

    auto query = PointSet::New();
    query->SetName("ProbeQueryPoints");
    if (!MakeQueryPoints(query, center, radius, count, seed)) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "Build query points failed (radius=" << radius
                  << ", n=" << count << ")\n";
        return false;
    }

    auto filter = ProbeFilter::New();
    filter->SetInput(0, model);
    filter->SetInput(1, query);
    if (!filter->Execute()) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "ProbeFilter::Execute() returned false\n";
        return false;
    }

    const IGsize numQuery = query->GetPoints()->GetNumberOfPoints();
    AttributeSet* attrs = query->GetAttributeSet();
    int validCount = 0;
    for (IGsize qi = 0; qi < numQuery; ++qi) {
        const Point& p = query->GetPoint(qi);

        int mask = 0;
        if (attrs != nullptr) {
            const int maskIndex = attrs->GetAttributeIndex("ValidPointMask");
            if (maskIndex >= 0) {
                auto maskArray =
                        DynamicCast<IntArray>(attrs->GetAttribute(maskIndex).pointer);
                if (!maskArray.IsNull()) mask = maskArray->GetValue(qi);
            }
        }
        if (mask == 1) ++validCount;

        std::cout << "Point[" << qi << "] = (" << p[0] << ", " << p[1] << ", "
                  << p[2] << ") valid=" << mask;
        if (attrs != nullptr) {
            auto pointAttrs = attrs->GetAllPointAttributes();
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.isDeleted || attr.pointer.IsNull()) continue;
                if (attr.pointer->GetName() == "ValidPointMask") continue;
                std::cout << "  " << attr.pointer->GetName() << "=";
                PrintArrayElement(attr.pointer.GetPointer(), qi);
            }
        }
        std::cout << "\n";
    }

    std::cout << "Valid points: " << validCount << " / " << numQuery << "\n";
    // The fixed sphere lies inside the pipe wall: an all-invalid result used to
    // print PASS unconditionally. Require valid interpolation and finite values.
    if (validCount != count || filter->GetOutput().get() != query.get()) {
        std::cerr << "Result: FAIL (expected every query inside the pipe wall)\n";
        return false;
    }
    auto sourceAttributes = model->GetAttributeSet()->GetAllPointAttributes();
    for (IGsize i = 0; i < sourceAttributes->GetNumberOfElements(); ++i) {
        const auto& source = sourceAttributes->GetElement(i);
        if (source.isDeleted || source.pointer.IsNull()) continue;
        const int index = attrs->GetAttributeIndex(source.pointer->GetName());
        if (index < 0) return false;
        const auto array = attrs->GetAttribute(index).pointer;
        if (!array || array->GetDimension() != source.pointer->GetDimension() ||
            array->GetNumberOfElements() != numQuery) return false;
        for (IGsize q = 0; q < numQuery; ++q) {
            for (int d = 0; d < array->GetDimension(); ++d) {
                if (!std::isfinite(array->GetElementValue(q, d))) return false;
            }
        }
    }
    std::cout << "Result: PASS\n";
    return true;
}

}  // namespace

int main() {
    // 写死的模型相对路径与探测参数：无需手动输入，运行即自动完成测试。
    const std::string modelFile = "./Models/AIGen_Hex_PipeSegment.vtk";
    const Point center(1.385819f, 0.574025f, 0.75f);  // 球心：管道壁内单元中心附近
    const float radius = 0.1f;              // 球体半径
    const int count = 10;                   // 球体内随机采样点数
    const unsigned seed = 42u;              // 固定种子，保证结果可复现

    std::cout << "Probe center=(1.385819, 0.574025, 0.75) radius=0.1 n=10 seed=42\n";
    return RunProbe(modelFile, center, radius, count, seed) ? 0 : 1;
}
