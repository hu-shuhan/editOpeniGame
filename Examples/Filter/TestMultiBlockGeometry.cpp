// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// ─────────────────────────────────────────────────────────────────────────────
// Windows 平台前置处理：必须在【所有 iGameVis 头文件之前】完成。
//
// 原因：windows.h 会定义宏 `#define GetMessage GetMessageA`。
//       若它先被某个 iGameVis 头文件间接引入，随后解析
//       iGameMultiBlockGeometryFilter.h 时，类声明里的
//       `GetMessage()` 就会被宏改写成 `GetMessageA()`，
//       导致调用 `filter->GetMessage()` 报 C2039（不是成员）。
//       因此这里先引入 windows.h 并立即 #undef，之后所有头文件都在
//       无该宏的环境下解析；后续重复 include windows.h 也会被 include guard 挡住。
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef GetMessage
#endif

#include <Core/iGameScene.h>
#include <ModelSurface/iGameMultiBlockGeometryFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameCellType.h>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_Passed = 0;
int g_Failed = 0;

void Check(bool condition, const std::string& name, const std::string& detail = "") {
    if (condition) {
        ++g_Passed;
        std::cout << "    [PASS] " << name << "\n";
    } else {
        ++g_Failed;
        std::cout << "    [FAIL] " << name;
        if (!detail.empty()) { std::cout << "  -> " << detail; }
        std::cout << "\n";
    }
}

/* 在输出装配体中按名称查找子块，返回空指针表示未找到 */
iGame::DataObject::Pointer FindBlockByName(iGame::DataObject::Pointer root, const std::string& name) {
    if (!root || !root->HasSubDataObject()) { return nullptr; }
    for (auto it = root->SubDataObjectIteratorBegin(); it != root->SubDataObjectIteratorEnd(); ++it) {
        if (it->second && it->second->GetName() == name) { return it->second; }
    }
    return nullptr;
}

/* 场景挂载：把抽取结果添加到场景中（含线框叠加） */
void AddToScene(iGame::Scene::Pointer scene, iGame::DataObject::Pointer root) {
    if (!root) { return; }
    if (root->HasSubDataObject()) {
        for (auto it = root->SubDataObjectIteratorBegin(); it != root->SubDataObjectIteratorEnd(); ++it) {
            auto drawObj = iGame::DynamicCast<iGame::DrawObject>(it->second);
            if (drawObj) {
                drawObj->SetViewStyle(IG_SURFACE);
                drawObj->AddViewStyle(IG_WIREFRAME);
                drawObj->ConvertToDrawableData();
                scene->AddModel(it->second);
            }
        }
    } else {
        auto drawObj = iGame::DynamicCast<iGame::DrawObject>(root);
        if (drawObj) {
            drawObj->SetViewStyle(IG_SURFACE);
            drawObj->AddViewStyle(IG_WIREFRAME);
            drawObj->ConvertToDrawableData();
            scene->AddModel(root);
        }
    }
}

/* ========== 测试 1：正常路径，验证几何精度与层级保真 ========== */
iGame::DataObject::Pointer TestNormalCase() {
    std::cout << "\n[Test 1] Normal path: extract every block of a multiblock assembly\n";
    const std::string fileName = "./Models/assembly_primitives.vtm";
    std::cout << "  file: " << fileName << "\n";

    auto root = iGame::FileIO::ReadFile(fileName);
    Check(root != nullptr, "read multiblock assembly");
    if (!root) { return nullptr; }

    auto filter = iGame::MultiBlockGeometryFilter::New();
    filter->SetInput(root);
    Check(filter->Execute(), "Execute() returns true", filter->GetMessage());
    Check(filter->GetFailedBlocks().empty(), "no failed block reported",
          "failed=" + std::to_string(filter->GetFailedBlocks().size()));

    auto res = filter->GetOutput();
    Check(res != nullptr, "output data object is not null");
    if (!res) { return nullptr; }
    Check(res->GetNumberOfSubDataObjects() == 2, "output keeps 2 sub-blocks",
          "actual=" + std::to_string(res->GetNumberOfSubDataObjects()));

    // 六面体零件：2 个单元共 12 面，剔除 2 个内部接触面 => 10 面 / 12 点
    auto cube = FindBlockByName(res, "assembly_cube_hex");
    auto cubeMesh = iGame::DynamicCast<iGame::SurfaceMesh>(cube);
    Check(cubeMesh != nullptr, "cube block found and is a SurfaceMesh");
    if (cubeMesh) {
        std::cout << "      * assembly_cube_hex: Points = " << cubeMesh->GetNumberOfPoints()
                  << ", Faces = " << cubeMesh->GetNumberOfFaces() << "\n";
        Check(cubeMesh->GetNumberOfFaces() == 10, "cube face count == 10",
              "actual=" + std::to_string(cubeMesh->GetNumberOfFaces()));
        Check(cubeMesh->GetNumberOfPoints() == 12, "cube point count == 12",
              "actual=" + std::to_string(cubeMesh->GetNumberOfPoints()));
    }

    // 三棱柱零件：2 个单元共 10 面，剔除 2 个内部接触面 => 8 面 / 8 点
    auto wedge = FindBlockByName(res, "assembly_wedge_prism");
    auto wedgeMesh = iGame::DynamicCast<iGame::SurfaceMesh>(wedge);
    Check(wedgeMesh != nullptr, "wedge block found and is a SurfaceMesh");
    if (wedgeMesh) {
        std::cout << "      * assembly_wedge_prism: Points = " << wedgeMesh->GetNumberOfPoints()
                  << ", Faces = " << wedgeMesh->GetNumberOfFaces() << "\n";
        Check(wedgeMesh->GetNumberOfFaces() == 8, "wedge face count == 8",
              "actual=" + std::to_string(wedgeMesh->GetNumberOfFaces()));
        Check(wedgeMesh->GetNumberOfPoints() == 8, "wedge point count == 8",
              "actual=" + std::to_string(wedgeMesh->GetNumberOfPoints()));
    }
    return res;
}

/* ========== 测试 2：异常路径，验证失败块不丢弃 + 路径原因报告 ========== */
void TestErrorCase() {
    std::cout << "\n[Test 2] Error path: tolerate a broken block and report it\n";
    const std::string fileName = "./Models/assembly_with_error.vtm";
    std::cout << "  file: " << fileName << "\n";

    auto root = iGame::FileIO::ReadFile(fileName);
    Check(root != nullptr, "read assembly containing a broken block");
    if (!root) { return; }

    auto filter = iGame::MultiBlockGeometryFilter::New();
    filter->SetInput(root);

    // 部分成功语义：整体返回 true，但必须报告失败块
    Check(filter->Execute(), "Execute() still returns true on partial success", filter->GetMessage());

    auto res = filter->GetOutput();
    Check(res != nullptr, "output data object is not null");
    if (!res) { return; }

    // 核心断言：失败块不得被静默丢弃，输入 2 块 => 输出必须仍是 2 块
    Check(res->GetNumberOfSubDataObjects() == 2, "failed block preserved (output still has 2 sub-blocks)",
          "actual=" + std::to_string(res->GetNumberOfSubDataObjects()));

    // 失败报告：数量 / 路径 / 原因
    const auto& failed = filter->GetFailedBlocks();
    Check(failed.size() == 1, "failed block report count == 1", "actual=" + std::to_string(failed.size()));
    if (!failed.empty()) {
        std::cout << "      * failed path  : " << failed[0].path << "\n";
        std::cout << "      * failed reason: " << failed[0].reason << "\n";
        Check(!failed[0].path.empty(), "failed block path is not empty");
        Check(failed[0].path.find("assembly_empty_error") != std::string::npos,
              "failed block path contains the broken part name", failed[0].path);
        Check(!failed[0].reason.empty(), "failed block reason is not empty");
    }

    // 正常零件仍应被正确抽取
    auto cube = FindBlockByName(res, "assembly_cube_hex");
    auto cubeMesh = iGame::DynamicCast<iGame::SurfaceMesh>(cube);
    Check(cubeMesh != nullptr && cubeMesh->GetNumberOfFaces() == 10,
          "healthy block in the same assembly still extracted (10 faces)");

    // 失败零件应保留为原始体网格（未被抽面，也不是空壳表面）
    auto bad = FindBlockByName(res, "assembly_empty_error");
    Check(bad != nullptr, "failed block still present in the output tree");
    if (bad) {
        Check(iGame::DynamicCast<iGame::SurfaceMesh>(bad) == nullptr,
              "failed block kept as the original object (not a SurfaceMesh)");
    }
}

/* ========== 测试 3：输入类型校验，验证非多块模型被拒绝 ========== */
void TestInputValidation() {
    std::cout << "\n[Test 3] Input validation: a single mesh must be rejected\n";
    const std::string fileName = "./Models/assembly_cube_hex.vtk";
    std::cout << "  file: " << fileName << " (single unstructured grid)\n";

    auto single = iGame::FileIO::ReadFile(fileName);
    Check(single != nullptr, "read single mesh model");
    if (!single) { return; }
    Check(!single->HasSubDataObject(), "single mesh has no sub-blocks (not a multiblock)");

    auto filter = iGame::MultiBlockGeometryFilter::New();
    filter->SetInput(single);
    Check(!filter->Execute(), "Execute() returns false for single mesh input");
    Check(!filter->GetMessage().empty(), "rejection reason is provided");
    std::cout << "      * rejection reason: " << filter->GetMessage() << "\n";

    // 空输入同样应被拒绝
    auto emptyFilter = iGame::MultiBlockGeometryFilter::New();
    emptyFilter->SetInput(nullptr);
    Check(!emptyFilter->Execute(), "Execute() returns false for null input");
    Check(!emptyFilter->GetMessage().empty(), "null input rejection reason is provided");
}

} // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // 控制台切 UTF-8，保证 Filter 返回的中文提示不乱码
#endif
    std::cout << std::unitbuf; // 立即刷新输出，便于定位崩溃点

    bool selfCheckOnly = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--selfcheck") { selfCheckOnly = true; }
    }

    std::cout << "\n============================================================\n";
    std::cout << "  [iGameVis] MultiBlockGeometryFilter Self-Test\n";
    std::cout << "  usage: testMultiBlockGeometry [--selfcheck]\n";
    std::cout << "         --selfcheck  run assertions only, skip the 3D window\n";
    std::cout << "============================================================\n";

    auto normalOutput = TestNormalCase();
    TestErrorCase();
    TestInputValidation();

    std::cout << "\n============================================================\n";
    std::cout << "  Result: " << g_Passed << " passed, " << g_Failed << " failed\n";
    std::cout << "============================================================\n";

    if (g_Failed > 0) {
        std::cerr << "[FAILED] some assertions did not pass.\n";
        return 1;
    }
    std::cout << "[SUCCESS] all assertions passed.\n";

    if (selfCheckOnly) { return 0; }

    /* 交互演示：将正常路径的抽取结果可视化 */
    if (normalOutput) {
        std::cout << "\nLaunching 3D interactive window (drag to rotate, scroll to zoom)...\n";
        auto scene = iGame::Scene::New();
        AddToScene(scene, normalOutput);

        auto window = iGame::RenderWindow::New();
        window->SetSize(1920, 1080);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);

        window->Show();
    }
    return 0;
}
