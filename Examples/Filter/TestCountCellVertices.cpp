// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestCountCellVertices.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <CountCellVertices/iGameCountCellVerticesFilter.h>

#include <Core/iGameScene.h>
#include <cstdlib>
#include <filesystem>
#include <iGameAttributeSet.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <string>

// 简单任务 #5（复测整改后）配套测试用例：统计每个单元的顶点数
// 运行：cd Examples && ./testCountCellVertices
//
// 覆盖四个场景，全部通过才输出 PASS：
//   1) 混合单元网格（CountCellVertices_mixed_cells.vtk）
//      —— 输出是独立结果节点；cell_vertex_count 长度与各单元顶点数一致；原模型不被修改；
//   2) 重复执行 —— 同名数组只保留一份（不追加），且始终是最新结果；
//   3) 顶点数完全相同的四面体网格（CellSize_TetraCube.vtk）
//      —— 着色范围有效，重复选择仍能映射到可见颜色；
//   4) 0 单元空网格（CountCellVertices_empty.vtk）
//      —— 执行成功就必须产出（空）数组，界面不会再出现"未找到数组"。

namespace {

int g_failed = 0;

void Check(bool ok, const std::string& what) {
    std::cerr << (ok ? "  [ ok ] " : "  [FAIL] ") << what << "\n";
    if (!ok) { ++g_failed; }
}

iGame::UnstructuredMesh::Pointer LoadMesh(const std::string& fileName) {
    if (!std::filesystem::exists(fileName)) {
        std::cerr << "  [FAIL] model not found: " << fileName << "\n";
        ++g_failed;
        return nullptr;
    }
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "  [FAIL] ReadFile returned null: " << fileName << "\n";
        ++g_failed;
        return nullptr;
    }
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cerr << "  [FAIL] not an UnstructuredMesh: " << fileName << "\n";
        ++g_failed;
    }
    return mesh;
}

/// 按名字 + 挂载位置查属性数组，找不到返回空
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

/// 统计同名同挂载位置的数组个数（复测要求：重复执行不能不断追加同名数组）
int CountArrays(iGame::DataObject::Pointer obj, const std::string& name, IGenum attachment) {
    int n = 0;
    if (obj == nullptr) { return n; }
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return n; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return n; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachment) { continue; }
        if (std::string(attr.pointer->GetName()) == name) { ++n; }
    }
    return n;
}

/// 校验数组每个值都等于对应单元的顶点数
bool CheckValuesEqualCellSizes(iGame::UnstructuredMesh::Pointer mesh, iGame::ArrayObject::Pointer counts) {
    const IGsize n = mesh->GetNumberOfCells();
    if (static_cast<IGsize>(counts->GetNumberOfValues()) != n) { return false; }
    for (IGsize i = 0; i < n; ++i) {
        const igIndex* ids = nullptr;
        const int expected = mesh->GetCellPointIds(i, ids);
        if (static_cast<int>(counts->GetValue(i)) != expected) {
            std::cerr << "  [FAIL] cell " << i << " expected " << expected << " got " << counts->GetValue(i)
                      << "\n";
            return false;
        }
    }
    return true;
}

/// 场景 1：混合单元网格 —— 独立输出 + 结果正确 + 原模型不被修改
void TestMixedCells() {
    std::cerr << "[case 1] mixed cell types (independent output)\n";
    auto mesh = LoadMesh("./Models/CountCellVertices_mixed_cells.vtk");
    if (mesh == nullptr) { return; }

    const IGsize inCells = mesh->GetNumberOfCells();
    Check(inCells == 9, "input: 9 cells");

    auto filter = iGame::CountCellVerticesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out != nullptr, "GetOutput() is an UnstructuredMesh");
    if (out == nullptr) { return; }

    Check(out.GetPointer() != mesh.GetPointer(),
          "output is an independent data object (counts are NOT written into the input)");
    Check(out->GetNumberOfCells() == inCells, "output keeps the same cell count");
    Check(out->GetPoints() != mesh->GetPoints(),
          "output points are deep-copied (independent buffer, not shared with input)");
    Check(out->GetCells() != mesh->GetCells(),
          "output cell array is deep-copied (independent buffer, not shared with input)");
    Check(out->GetAttributeSet() != mesh->GetAttributeSet(),
          "output attribute set is a new object (not shared with input)");

    auto counts = FindArray(out, "cell_vertex_count", IG_CELL);
    Check(counts != nullptr, "output has Cell Data 'cell_vertex_count'");
    if (counts != nullptr) {
        Check(CheckValuesEqualCellSizes(out, counts), "every value equals the cell's vertex count");
    }

    Check(FindArray(mesh, "cell_vertex_count", IG_CELL) == nullptr,
          "input mesh is left untouched (no cell_vertex_count on it)");
}

/// 场景 2：重复执行 —— 同名数组只保留一份，且是最新结果
void TestRepeatedExecute() {
    std::cerr << "[case 2] repeated execution (no duplicated arrays)\n";
    auto mesh = LoadMesh("./Models/CountCellVertices_mixed_cells.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::CountCellVerticesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "first Execute() returns true");
    Check(filter->Execute(), "second Execute() returns true");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    const int n = CountArrays(out, "cell_vertex_count", IG_CELL);
    Check(n == 1, "exactly one cell_vertex_count array after two runs (got " + std::to_string(n) + ")");

    auto counts = FindArray(out, "cell_vertex_count", IG_CELL);
    if (counts != nullptr) {
        Check(CheckValuesEqualCellSizes(out, counts), "values after the second run are correct");
    }
    Check(FindArray(mesh, "cell_vertex_count", IG_CELL) == nullptr,
          "input mesh is still untouched after two runs");
}

/// A uniform tetrahedral mesh must have a usable color range when selected twice.
void TestUniformCellColorRange() {
    std::cerr << "[case 3] uniform tetrahedral cell coloring\n";
    auto mesh = LoadMesh("./Models/CellSize_TetraCube.vtk");
    if (mesh == nullptr) { return; }
    auto filter = iGame::CountCellVerticesFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        Check(false, "uniform tetrahedral filter execution succeeds");
        return;
    }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "uniform tetrahedral result is an UnstructuredMesh");
        return;
    }
    const int index = out->GetAttributeSet()->GetAttributeIndex("cell_vertex_count");
    Check(index >= 0, "uniform result has cell_vertex_count");
    if (index < 0) { return; }
    auto& attribute = out->GetAttributeSet()->GetAttribute(index);
    auto range = attribute.GetDataRange();
    Check(range && range->GetValue(2) < range->GetValue(3),
          "uniform result has a nondegenerate component color range");
    Check(attribute.pointer->GetValue(0) == 4.0,
          "the display range does not change the actual vertex count");
    if (!range) { return; }
    auto scene = iGame::Scene::New();
    out->ViewCloudPicture(scene.GetPointer(), index, 0);
    out->ConvertToDrawableData();
    auto renderable = out->GetRenderableObject();
    renderable->ConvertToDrawableData();
    out->ViewCloudPicture(scene.GetPointer(), index, 0);
    out->ConvertToDrawableData();
    renderable->ConvertToDrawableData();
    const auto mapperRange = out->GetColorMapper()->GetRange();
    Check(mapperRange[0] == 4.0 && mapperRange[1] == 5.0,
          "reselecting the uniform cell array keeps its display color range");
    auto colors = out->GetColorMapper()->MapScalars(attribute.pointer, 0, 4);
    float firstColor[4]{};
    colors->GetElement(0, firstColor);
    Check(firstColor[2] > firstColor[0],
          "uniform vertex counts map to a visible blue rather than the white midpoint");
}

/// 场景 4：0 单元空网格 —— 成功就必须产出数组，不能让界面"找不到数组"
void TestEmptyMesh() {
    std::cerr << "[case 4] empty mesh (0 cells)\n";
    auto mesh = LoadMesh("./Models/CountCellVertices_empty.vtk");
    if (mesh == nullptr) { return; }

    Check(mesh->GetNumberOfCells() == 0, "input has 0 cells");

    auto filter = iGame::CountCellVerticesFilter::New();
    filter->SetInput(mesh);
    const bool ok = filter->Execute();
    Check(ok, "Execute() succeeds for a mesh with 0 cells");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out != nullptr && out.GetPointer() != mesh.GetPointer(),
          "on success the output is an independent data object");
    if (out == nullptr) { return; }

    auto counts = FindArray(out, "cell_vertex_count", IG_CELL);
    Check(counts != nullptr, "empty result still carries a cell_vertex_count array (length 0)");
    if (counts != nullptr) {
        Check(counts->GetNumberOfValues() == 0, "the array has length 0");
    }
}

/// 可视化演示：读混合单元模型，按 cell_vertex_count 着色并弹出渲染窗口（便于录屏对照）
void VisualizeCountResult() {
    // 设了 IGV_TEST_NO_VIEW 时跳过弹窗，便于自动化/无头环境只跑断言
    if (std::getenv("IGV_TEST_NO_VIEW") != nullptr) { return; }
    auto mesh = LoadMesh("./Models/CountCellVertices_mixed_cells.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::CountCellVerticesFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) { return; }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) { return; }

    auto scene = iGame::Scene::New();
    auto draw = iGame::DynamicCast<iGame::DrawObject>(out);
    if (draw != nullptr) {
        draw->SetViewStyle(IG_SURFACE);
        int idx = -1;
        if (out->GetAttributeSet() != nullptr) {
            idx = out->GetAttributeSet()->GetAttributeIndex("cell_vertex_count");
        }
        if (idx >= 0) { draw->ViewCloudPicture(scene.GetPointer(), idx); }
    }
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

int main(int argc, char** argv) {
    std::cerr << "==== testCountCellVertices ====\n";
    TestMixedCells();
    TestRepeatedExecute();
    TestUniformCellColorRange();
    TestEmptyMesh();

    if (g_failed == 0) {
        std::cerr << "[testCountCellVertices] PASS: all checks passed\n";
    } else {
        std::cerr << "[testCountCellVertices] FAIL: " << g_failed << " check(s) failed\n";
    }

    // —— 可视化演示：按 cell_vertex_count 着色弹出渲染窗口 ——
    if (!(argc > 1 && std::string(argv[1]) == "--no-render")) VisualizeCountResult();

    return (g_failed == 0) ? 0 : 1;
}
