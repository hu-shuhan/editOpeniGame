// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestExtractEdges.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
#include <ExtractEdges/iGameExtractEdgesFilter.h>

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
#include <vector>

// 中等任务 #28（复测整改后）配套测试用例：提取网格边（去重）
// 运行：cd Examples && ./testExtractEdges
//
// 覆盖三个场景，全部通过才输出 PASS：
//   1) 带 Point Data / Cell Data 的三角形网格（ExtractEdges_tri_cell_data.vtu）
//      —— 输出必须是独立网格（不是原模型）；Point Data 保留且长度正确；
//         Cell Data 长度必须与输出边数一致（复测反馈的 cell data mismatch 场景）；
//   2) 2x2x1 六面体网格（ExtractEdges_hexa_grid.vtk）—— 共享边去重后 33 条唯一边；
//   3) 0 单元空网格（CountCellVertices_empty.vtk）—— 不得返回原模型。

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

/// 按名字 + 挂载位置（IG_POINT / IG_CELL）查属性数组，找不到返回空
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

/// 收集"挂在单元上、长度却与输出单元数不一致"的数组名（复测要求：输出属性必须正确）
std::vector<std::string> CollectBadCellArrays(iGame::DataObject::Pointer obj, IGsize cellNum) {
    std::vector<std::string> bad;
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return bad; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return bad; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        // 注意用 GetNumberOfElements（元组数）而不是 GetNumberOfValues（标量数）：
        // 多分量数组的标量数是 元组数×分量数，用标量数比较会把合法数组误判为长度错误。
        if (attr.attachmentType != IG_CELL) { continue; }
        if (static_cast<IGsize>(attr.pointer->GetNumberOfElements()) != cellNum) {
            bad.emplace_back(attr.pointer->GetName());
        }
    }
    return bad;
}

/// 校验输出网格全部是 IG_LINE 且每条边恰有 2 个互异端点，返回边数（-1 表示校验失败）
IGsize CheckAllEdges(iGame::UnstructuredMesh::Pointer out) {
    const IGsize n = out->GetNumberOfCells();
    for (IGsize i = 0; i < n; ++i) {
        if (out->GetCellType(i) != iGame::IG_LINE) {
            std::cerr << "  [FAIL] cell " << i << " is not IG_LINE\n";
            ++g_failed;
            return -1;
        }
        const igIndex* ids = nullptr;
        if (out->GetCellPointIds(i, ids) != 2) {
            std::cerr << "  [FAIL] cell " << i << " does not have 2 points\n";
            ++g_failed;
            return -1;
        }
        if (ids[0] == ids[1]) {
            std::cerr << "  [FAIL] cell " << i << " is a degenerate edge\n";
            ++g_failed;
            return -1;
        }
    }
    return n;
}

/// 场景 1：带 Point Data / Cell Data 的三角形网格（复测反馈的 cell data mismatch）
void TestTriMeshWithCellData() {
    std::cerr << "[case 1] tri mesh with Point Data + Cell Data\n";
    auto mesh = LoadMesh("./Models/ExtractEdges_tri_cell_data.vtu");
    if (mesh == nullptr) { return; }

    const IGsize inPoints = mesh->GetNumberOfPoints();
    const IGsize inCells = mesh->GetNumberOfCells();
    Check(inPoints == 9 && inCells == 8, "input: 9 points / 8 triangle cells");

    auto filter = iGame::ExtractEdgesFilter::New();
    filter->SetInput(mesh);
    const bool ok = filter->Execute();
    Check(ok, "Execute() returns true for a valid mesh");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out != nullptr, "GetOutput() is an UnstructuredMesh");
    if (out == nullptr) { return; }

    Check(out.GetPointer() != mesh.GetPointer(),
          "output is an independent data object (input mesh is left untouched)");

    const IGsize edgeNum = CheckAllEdges(out);
    Check(edgeNum == 16, "unique edge count == 16 (got " + std::to_string(edgeNum) + ")");
    Check(out->GetNumberOfPoints() == inPoints, "point count preserved (9)");

    // Point Data：点数不变，应当保留且长度正确
    auto pointScalar = FindArray(out, "point_scalar", IG_POINT);
    Check(pointScalar != nullptr, "Point Data 'point_scalar' is preserved");
    if (pointScalar != nullptr) {
        Check(static_cast<IGsize>(pointScalar->GetNumberOfValues()) == inPoints,
              "point_scalar length == point count");
    }

    // Cell Data：输出单元数（边数）与输入单元数不同，长度必须按输出重建
    auto bad = CollectBadCellArrays(out, edgeNum);
    std::string badNames;
    for (const auto& name : bad) { badNames += name + " "; }
    Check(bad.empty(), "every Cell Data array length == edge count (bad: " + badNames + ")");

    // 不生成"边→来源单元"的映射数组（与 vtkExtractEdges 一致）
    Check(FindArray(out, "edge_source_cell", IG_CELL) == nullptr,
          "no auxiliary 'edge_source_cell' array is added (matches ParaView output)");

    // CellType：每条边的 VTK 单元类型编号（vtkLine = 3），长度 = 边数
    auto cellType = FindArray(out, "CellType", IG_CELL);
    Check(cellType != nullptr, "Cell Data 'CellType' is generated");
    if (cellType != nullptr) {
        Check(static_cast<IGsize>(cellType->GetNumberOfElements()) == edgeNum,
              "CellType tuple count == edge count");
        bool allLine = true;
        for (IGsize i = 0; i < edgeNum; ++i) {
            if (static_cast<int>(cellType->GetElementValue(i, 0)) != 3) { allLine = false; break; }
        }
        Check(allLine, "CellType value == 3 (vtkLine) for every edge");
    }

    // CellType 排在 Cell Data 的第一位（属性面板 / 导出文件里的数组顺序 = 添加顺序）
    {
        auto all = out->GetAttributeSet()->GetAllAttributes();
        std::string firstCellName;
        for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
            auto& a = all->GetElement(i);
            if (a.isDeleted || a.pointer == nullptr) { continue; }
            if (a.attachmentType != IG_CELL) { continue; }
            firstCellName = a.pointer->GetName();
            break;
        }
        Check(firstCellName == "CellType",
              "CellType is the first Cell Data array (got '" + firstCellName + "')");
    }

    // 输入模型的属性不能被改动（独立输出节点）
    auto inBad = CollectBadCellArrays(mesh, inCells);
    Check(inBad.empty(), "input mesh keeps its own (8) cell arrays untouched");
}

/// 场景 2：2x2x1 六面体网格，共享边去重后 33 条唯一边
void TestHexaGrid() {
    std::cerr << "[case 2] 2x2x1 hexahedral grid (deduplication)\n";
    auto mesh = LoadMesh("./Models/ExtractEdges_hexa_grid.vtk");
    if (mesh == nullptr) { return; }

    Check(mesh->GetNumberOfPoints() == 18 && mesh->GetNumberOfCells() == 4,
          "input: 18 points / 4 hexahedron cells");

    auto filter = iGame::ExtractEdgesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    Check(out.GetPointer() != mesh.GetPointer(), "output is independent from the input");

    const IGsize edgeNum = CheckAllEdges(out);
    Check(edgeNum == 33, "unique edge count == 33 (got " + std::to_string(edgeNum) + ")");
}

/// 场景 3：0 单元空网格 —— 失败要能被识别，绝不能把原模型当结果返回
void TestEmptyMesh() {
    std::cerr << "[case 3] empty mesh (0 cells)\n";
    auto mesh = LoadMesh("./Models/CountCellVertices_empty.vtk");
    if (mesh == nullptr) { return; }

    Check(mesh->GetNumberOfCells() == 0, "input has 0 cells");

    auto filter = iGame::ExtractEdgesFilter::New();
    filter->SetInput(mesh);
    const bool ok = filter->Execute();
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (!ok) {
        Check(true, "Execute() reports failure for a mesh without edges (acceptable)");
        if (!filter->GetMessage().empty()) {
            std::cerr << "         message: " << filter->GetMessage() << "\n";
        }
    } else {
        Check(outMesh != nullptr && outMesh.GetPointer() != mesh.GetPointer(),
              "on success the output must be an independent (empty) edge mesh, never the input");
        if (outMesh != nullptr && outMesh.GetPointer() != mesh.GetPointer()) {
            Check(outMesh->GetNumberOfCells() == 0, "empty edge mesh has 0 edges");
        }
    }
}

/// 场景 4：复测示例 —— 输入的单元数据必须按"来源单元"重映射到输出的每条边
///   两个共享一个面的四面体：输出 9 条边；共享边继承来源单元 ID 较小者（Cell0）的数据，
///   因此 CellValue 应为 6 个 10 + 3 个 20，OriginalCellTag 应为 6 个 100 + 3 个 200。
void TestCellDataRemap() {
    std::cerr << "[case 4] cell data remapped by source cell (9 edges: 6x10 + 3x20)\n";
    auto mesh = LoadMesh("./Models/extract_edges_cell_data_mismatch.vtu");
    if (mesh == nullptr) { return; }

    Check(mesh->GetNumberOfPoints() == 5 && mesh->GetNumberOfCells() == 2,
          "input: 5 points / 2 tetrahedra sharing one face");

    auto filter = iGame::ExtractEdgesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out != nullptr, "GetOutput() is an UnstructuredMesh");
    if (out == nullptr) { return; }

    const IGsize edgeNum = CheckAllEdges(out);
    Check(edgeNum == 9, "unique edge count == 9 (got " + std::to_string(edgeNum) + ")");

    // 单元数据不能丢：长度必须等于边数，值按来源单元重映射
    auto cellValue = FindArray(out, "CellValue", IG_CELL);
    Check(cellValue != nullptr, "Cell Data 'CellValue' is preserved (not dropped)");
    if (cellValue != nullptr) {
        Check(static_cast<IGsize>(cellValue->GetNumberOfValues()) == edgeNum,
              "CellValue length == edge count (9)");
        int n10 = 0;
        int n20 = 0;
        for (IGsize i = 0; i < edgeNum; ++i) {
            const int v = static_cast<int>(cellValue->GetValue(i));
            if (v == 10) { ++n10; } else if (v == 20) { ++n20; }
        }
        Check(n10 == 6 && n20 == 3,
              "CellValue is 6x10 + 3x20 (got " + std::to_string(n10) + "x10 + " +
                  std::to_string(n20) + "x20)");
    }

    auto tag = FindArray(out, "OriginalCellTag", IG_CELL);
    Check(tag != nullptr, "Cell Data 'OriginalCellTag' is preserved (not dropped)");
    if (tag != nullptr) {
        Check(static_cast<IGsize>(tag->GetNumberOfValues()) == edgeNum,
              "OriginalCellTag length == edge count (9)");
        int n100 = 0;
        int n200 = 0;
        for (IGsize i = 0; i < edgeNum; ++i) {
            const int v = static_cast<int>(tag->GetValue(i));
            if (v == 100) { ++n100; } else if (v == 200) { ++n200; }
        }
        Check(n100 == 6 && n200 == 3,
              "OriginalCellTag is 6x100 + 3x200 (got " + std::to_string(n100) + "x100 + " +
                  std::to_string(n200) + "x200)");
    }

    // 输入模型本身不能被改动
    auto inCellValue = FindArray(mesh, "CellValue", IG_CELL);
    Check(inCellValue != nullptr && static_cast<IGsize>(inCellValue->GetNumberOfValues()) == 2,
          "input mesh keeps its own 2-value CellValue untouched");
}

/// 场景 5：多分量数组 + 点/单元同名数组（通用性回归）
///   同一个几何（2 个共享面的四面体，9 条边），但数据里同时有：
///     - 多分量：p_vec(点,3分量)、c_vec(单元,3分量)
///     - 同名：shared_tag 同时存在于 Point Data 与 Cell Data
///   期望：多分量数组"元组数"等于对应单元/点数（不能被放大成分量数倍），
///         同名点数组不能被单元数据的同名处理误删。
void TestMultiComponentData() {
    std::cerr << "[case 5] multi-component + same-name arrays stay correct\n";
    auto mesh = LoadMesh("./Models/ExtractEdges_multicomp_cell_data.vtk");
    if (mesh == nullptr) { return; }

    Check(mesh->GetNumberOfPoints() == 5 && mesh->GetNumberOfCells() == 2,
          "input: 5 points / 2 tetrahedra sharing one face");

    auto filter = iGame::ExtractEdgesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out != nullptr, "GetOutput() is an UnstructuredMesh");
    if (out == nullptr) { return; }

    const IGsize edgeNum = CheckAllEdges(out);
    Check(edgeNum == 9, "unique edge count == 9 (got " + std::to_string(edgeNum) + ")");
    const IGsize pointNum = mesh->GetNumberOfPoints();

    // —— 多分量单元数据：元组数必须等于边数，值按来源单元重映射 ——
    auto cVec = FindArray(out, "c_vec", IG_CELL);
    Check(cVec != nullptr, "Cell Data 'c_vec' (3 components) is preserved");
    if (cVec != nullptr) {
        Check(cVec->GetDimension() == 3, "c_vec keeps 3 components");
        Check(static_cast<IGsize>(cVec->GetNumberOfElements()) == edgeNum,
              "c_vec tuple count == edge count (9), not 9*components (got " +
                  std::to_string(cVec->GetNumberOfElements()) + ")");
        int fromCell0 = 0, fromCell1 = 0;
        for (IGsize i = 0; i < edgeNum; ++i) {
            const double x = cVec->GetElementValue(i, 0);
            const double y = cVec->GetElementValue(i, 1);
            const double z = cVec->GetElementValue(i, 2);
            if (x == 10 && y == 11 && z == 12) { ++fromCell0; }
            if (x == 20 && y == 21 && z == 22) { ++fromCell1; }
        }
        Check(fromCell0 == 6 && fromCell1 == 3,
              "c_vec is 6x(10,11,12) + 3x(20,21,22) (got " + std::to_string(fromCell0) +
                  " + " + std::to_string(fromCell1) + ")");
    }

    auto cScalar = FindArray(out, "c_scalar", IG_CELL);
    if (cScalar != nullptr) {
        Check(static_cast<IGsize>(cScalar->GetNumberOfElements()) == edgeNum,
              "c_scalar tuple count == edge count (9)");
    } else {
        Check(false, "Cell Data 'c_scalar' is preserved");
    }

    // —— 点数据：元组数必须仍等于点数（多分量不能被放大）——
    auto pVec = FindArray(out, "p_vec", IG_POINT);
    Check(pVec != nullptr, "Point Data 'p_vec' (3 components) is preserved");
    if (pVec != nullptr) {
        Check(pVec->GetDimension() == 3, "p_vec keeps 3 components");
        Check(static_cast<IGsize>(pVec->GetNumberOfElements()) == pointNum,
              "p_vec tuple count == point count (5), not 5*components (got " +
                  std::to_string(pVec->GetNumberOfElements()) + ")");
        Check(pVec->GetElementValue(4, 0) == 1.0 && pVec->GetElementValue(4, 1) == 1.0 &&
                  pVec->GetElementValue(4, 2) == 1.0,
              "p_vec values preserved");
    }

    auto pScalar = FindArray(out, "p_scalar", IG_POINT);
    if (pScalar != nullptr) {
        Check(static_cast<IGsize>(pScalar->GetNumberOfElements()) == pointNum,
              "p_scalar tuple count == point count (5)");
    } else {
        Check(false, "Point Data 'p_scalar' is preserved");
    }

    // —— 同名数组：点上的 shared_tag 不能被单元上的同名处理误删 ——
    auto pointTag = FindArray(out, "shared_tag", IG_POINT);
    Check(pointTag != nullptr, "Point Data 'shared_tag' survives the cell-data remap");
    if (pointTag != nullptr) {
        Check(static_cast<IGsize>(pointTag->GetNumberOfElements()) == pointNum,
              "point 'shared_tag' tuple count == point count (5)");
        Check(pointTag->GetElementValue(4, 0) == 104.0, "point 'shared_tag' values preserved");
    }
    auto cellTag = FindArray(out, "shared_tag", IG_CELL);
    Check(cellTag != nullptr, "Cell Data 'shared_tag' is remapped to the edges");
    if (cellTag != nullptr) {
        Check(static_cast<IGsize>(cellTag->GetNumberOfElements()) == edgeNum,
              "cell 'shared_tag' tuple count == edge count (9)");
        int n1000 = 0, n2000 = 0;
        for (IGsize i = 0; i < edgeNum; ++i) {
            const int v = static_cast<int>(cellTag->GetElementValue(i, 0));
            if (v == 1000) { ++n1000; } else if (v == 2000) { ++n2000; }
        }
        Check(n1000 == 6 && n2000 == 3,
              "cell 'shared_tag' is 6x1000 + 3x2000 (got " + std::to_string(n1000) +
                  " + " + std::to_string(n2000) + ")");
    }

    // 输入模型不能被改动
    auto inCVec = FindArray(mesh, "c_vec", IG_CELL);
    Check(inCVec != nullptr && static_cast<IGsize>(inCVec->GetNumberOfElements()) == 2,
          "input mesh keeps its own 2-tuple c_vec untouched");
    auto inPointTag = FindArray(mesh, "shared_tag", IG_POINT);
    Check(inPointTag != nullptr &&
              static_cast<IGsize>(inPointTag->GetNumberOfElements()) == pointNum,
          "input mesh keeps its own point 'shared_tag' untouched");
}

/// 可视化演示：读六面体网格，提取边并以线框形式弹出渲染窗口（便于录屏对照）
void VisualizeEdgesResult() {
    // 设了 IGV_TEST_NO_VIEW 时跳过弹窗，便于自动化/无头环境只跑断言
    if (std::getenv("IGV_TEST_NO_VIEW") != nullptr) { return; }
    auto mesh = LoadMesh("./Models/ExtractEdges_hexa_grid.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::ExtractEdgesFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) { return; }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) { return; }

    auto scene = iGame::Scene::New();
    auto draw = iGame::DynamicCast<iGame::DrawObject>(out);
    if (draw != nullptr) {
        draw->SetViewStyle(IG_WIREFRAME);
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
    std::cerr << "==== testExtractEdges ====\n";
    TestTriMeshWithCellData();
    TestHexaGrid();
    TestEmptyMesh();
    TestCellDataRemap();
    TestMultiComponentData();

    if (g_failed == 0) {
        std::cerr << "[testExtractEdges] PASS: all checks passed\n";
    } else {
        std::cerr << "[testExtractEdges] FAIL: " << g_failed << " check(s) failed\n";
    }

    // —— 可视化演示：提取边以线框形式弹出渲染窗口 ——
    if (!(argc > 1 && std::string(argv[1]) == "--no-render")) VisualizeEdgesResult();

    return (g_failed == 0) ? 0 : 1;
}
