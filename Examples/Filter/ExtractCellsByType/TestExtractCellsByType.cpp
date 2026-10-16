// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/ExtractCellsByType/TestExtractCellsByType.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <ExtractCellsByType/iGameExtractCellsByTypeFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGamePoints.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// =============================================================================
// 回归测试说明（ExtractCellsByTypeFilter）
//
// 【当时的 BUG / 问题】
//   本用例原先读取 "./Models/ClipTest_Plane_UnstructuredGrid.vtk"，但该模型文件
//   并不在源码目录 Examples/Models 中（只存在于个别本地构建目录），因此在干净
//   检出（含 CI）下必然读文件失败；上游据此把 testExtractCellsByType 判为
//   "失效 target"，在提交 66882cc（修复 Examples 重复及失效 target 注册）中删除
//   了该测试及其注册。
//
// 【触发条件】在未包含该临时模型文件的源码检出 / CI 环境中运行本用例。
//
// 【错误表现】FileIO::ReadFile 返回空，测试打印 "FAIL: cannot open mixed test model"
//   并以非 0 退出；CI 侧表现为 target 注册失效或被直接移除。
//
// 【测试要保证的正确行为】
//   1) 使用随仓库一起提交的测试模型（Examples/Models/ExtractCellsByType_mixed.vtk、
//      ExtractCellsByType_surface.vtk），相对路径写死在代码里，运行即自动完成测试；
//   2) 混合单元模型须被识别出 4 种单元类型（三角形/四边形/四面体/六面体），全选提取
//      时单元数守恒、属性数量与长度守恒、double 属性类型与精度不丢失；
//   3) 仅提取单一类型时，输出单元类型必须全部等于所选类型；
//   4) 无渲染环境下（CI）不得弹出渲染窗口 —— 支持 --no-show / --no-render 参数，
//      以及环境变量 IGAME_EXAMPLE_NO_RENDER=1（上游 Examples 的免渲染约定）。
//
// 【提交号】待提交（本次提交主题：修复按单元类型提取的失效测试并补充 AI 生成测试模型与使用说明）。
//   本测试文件首次加入仓库的查询命令：
//     git log --diff-filter=A --format="%h %s" -- Examples/Filter/ExtractCellsByType/TestExtractCellsByType.cpp
// =============================================================================

// 简易断言：条件不成立则打印 FAIL 并退出
static int check(bool ok, const char* what) {
    if (!ok) {
        std::cout << "FAIL: " << what << std::endl;
        return 1;
    }
    std::cout << "PASS: " << what << std::endl;
    return 0;
}

// 构造一个混合单元网格（三角形 + 四边形 + 四面体 + 六面体，10 个点，带点属性和单元属性）
static iGame::UnstructuredMesh::Pointer buildMixedMesh() {
    using namespace iGame;
    UnstructuredMesh::Pointer mesh = UnstructuredMesh::New();
    mesh->SetName("mixed_mesh");

    auto pts = mesh->GetPoints();
    const float coords[10][3] = {
        {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 0, 0},
        {2, 1, 0}, {2, 2, 0}, {1, 2, 0}, {0, 0, 1}, {1, 0, 1}};
    for (int i = 0; i < 10; i++) { pts->AddPoint(Point(coords[i][0], coords[i][1], coords[i][2])); }

    igIndex ids[8];
    ids[0] = 0; ids[1] = 1; ids[2] = 2;
    mesh->AddCell(ids, 3, IG_TRIANGLE);              // 三角形 (0,1,2)
    ids[0] = 3; ids[1] = 4; ids[2] = 5; ids[3] = 6;
    mesh->AddCell(ids, 4, IG_QUAD);                  // 四边形 (3,4,5,6)
    ids[0] = 0; ids[1] = 3; ids[2] = 8; ids[3] = 9;
    mesh->AddCell(ids, 4, IG_TETRA);                 // 四面体 (0,3,8,9)
    ids[0] = 0; ids[1] = 1; ids[2] = 2; ids[3] = 3;
    ids[4] = 4; ids[5] = 5; ids[6] = 6; ids[7] = 7;
    mesh->AddCell(ids, 8, IG_HEXAHEDRON);            // 六面体 (0..7)

    // 点属性：pid[i] = i * 10（float 数组）
    auto attrSet = AttributeSet::New();
    auto pArray = FloatArray::New();
    pArray->SetName("pid");
    pArray->SetDimension(1);
    pArray->Resize(10);
    for (int i = 0; i < 10; i++) { pArray->SetValue(i, i * 10.0); }
    attrSet->AddAttribute(IG_SCALAR, IG_POINT, pArray, nullptr);
    // 点属性：pid_double[i] = i * 10 + 0.5（double 数组，验证类型与精度保留）
    auto pdArray = DoubleArray::New();
    pdArray->SetName("pid_double");
    pdArray->SetDimension(1);
    pdArray->Resize(10);
    for (int i = 0; i < 10; i++) { pdArray->SetValue(i, i * 10.0 + 0.5); }
    attrSet->AddAttribute(IG_SCALAR, IG_POINT, pdArray, nullptr);
    // 单元属性：cid[i] = i * 100（float 数组）
    auto cArray = FloatArray::New();
    cArray->SetName("cid");
    cArray->SetDimension(1);
    cArray->Resize(4);
    for (int i = 0; i < 4; i++) { cArray->SetValue(i, i * 100.0); }
    attrSet->AddAttribute(IG_SCALAR, IG_CELL, cArray, nullptr);
    mesh->SetAttributeSet(attrSet);
    return mesh;
}

// 校验输出网格的属性是否与输入对账
static int verifyAttributes(iGame::DataObject::Pointer out, IGsize expectPointNum, IGsize expectCellNum) {
    using namespace iGame;
    auto attrSet = out->GetAttributeSet();
    if (check(attrSet != nullptr, "output has attribute set")) return 1;
    if (check(attrSet->GetNumberOfAttributes() == 3, "output attribute count == 3")) return 1;

    auto allAttrs = attrSet->GetAllAttributes();
    bool seenPoint = false, seenCell = false;
    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); i++) {
        auto& attr = allAttrs->GetElement(i);
        if (attr.attachmentType == IG_POINT) {
            seenPoint = true;
            if (check(attr.pointer->GetNumberOfElements() == expectPointNum,
                      "point attribute length == output point count")) return 1;
        } else if (attr.attachmentType == IG_CELL) {
            seenCell = true;
            if (check(attr.pointer->GetNumberOfElements() == expectCellNum,
                      "cell attribute length == output cell count")) return 1;
        }
        // 数组原始类型保留：pid 仍是 float，pid_double 仍是 double（不降精度）
        if (attr.pointer->GetName() == "pid") {
            if (check(attr.pointer->GetArrayType() == IG_FloatArray,
                      "attribute 'pid' keeps FloatArray type")) return 1;
        } else if (attr.pointer->GetName() == "pid_double") {
            if (check(attr.pointer->GetArrayType() == IG_DoubleArray,
                      "attribute 'pid_double' keeps DoubleArray type")) return 1;
        }
    }
    if (check(seenPoint, "point attribute kept")) return 1;
    if (check(seenCell, "cell attribute kept")) return 1;
    return 0;
}

int main(int argc, char** argv) {
    // 免渲染：支持 --no-show / --no-render 参数，或环境变量 IGAME_EXAMPLE_NO_RENDER=1
    // （--no-render 与 IGAME_EXAMPLE_NO_RENDER 是上游 Examples 的 CI 约定）
    bool noShow = false;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--no-show" || arg == "--no-render") { noShow = true; }
    }
    if (const char* env = std::getenv("IGAME_EXAMPLE_NO_RENDER")) {
        const std::string v = env;
        if (v == "1" || v == "true" || v == "ON") { noShow = true; }
    }
    using namespace iGame;

    // ============ 测试 1：合成混合网格，全类型提取 ============
    std::cout << "===== Test 1: extract all cell types from mixed mesh =====" << std::endl;
    auto mesh = buildMixedMesh();
    const IGsize inCellNum = mesh->GetNumberOfCells();
    const IGsize inPointNum = mesh->GetNumberOfPoints();

    auto f1 = ExtractCellsByTypeFilter::New();
    f1->SetInput(mesh);
    auto available = f1->GetAvailableCellTypes();
    std::cout << "available cell types (" << available.size() << "):";
    for (auto t : available) { std::cout << " " << ExtractCellsByTypeFilter::GetCellTypeDisplayName(t); }
    std::cout << std::endl;
    if (check(available.size() == 4, "mixed mesh reports 4 distinct cell types")) return 1;

    f1->SetExtractCellTypes(available); // 全选
    if (check(f1->Execute(), "Execute with all types")) return 1;

    auto out1 = f1->GetOutput();
    auto um1 = DynamicCast<UnstructuredMesh>(out1);
    if (check(um1 != nullptr, "output is UnstructuredMesh")) return 1;
    if (check(out1->GetName() == "ExtractCellsByType_1", "output named ExtractCellsByType_1")) return 1;
    if (check(um1->GetNumberOfCells() == inCellNum, "all cells extracted (cell count preserved)")) return 1;
    if (check(um1->GetNumberOfPoints() == inPointNum, "all points kept (point count preserved)")) return 1;
    if (verifyAttributes(out1, inPointNum, inCellNum)) return 1;

    // 数值对账：全部点的属性值应原样保留。
    // 注意：输出点序 = "首次使用顺序"（与输入点序不同），但每个点的值都跟着映射走，
    // 因此校验"值的集合不变"（float 属性与 double 属性各自对账）。
    {
        auto outAttr = out1->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < outAttr->GetNumberOfElements(); i++) {
            auto& attr = outAttr->GetElement(i);
            if (attr.attachmentType == IG_POINT) {
                std::vector<double> vals;
                for (IGsize k = 0; k < attr.pointer->GetNumberOfElements(); k++) {
                    vals.push_back(attr.pointer->GetValue(k));
                }
                std::sort(vals.begin(), vals.end());
                if (attr.pointer->GetName() == "pid") {
                    const std::vector<double> expect = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
                    if (check(vals == expect, "point attribute 'pid' values preserved (as a set)")) return 1;
                } else if (attr.pointer->GetName() == "pid_double") {
                    const std::vector<double> expect = {0.5, 10.5, 20.5, 30.5, 40.5, 50.5, 60.5, 70.5, 80.5, 90.5};
                    if (check(vals == expect, "point attribute 'pid_double' values preserved (as a set)")) return 1;
                }
            }
        }
    }

    // ============ 测试 2：只提取四面体 ============
    std::cout << "===== Test 2: extract tetra only =====" << std::endl;
    auto f2 = ExtractCellsByTypeFilter::New();
    f2->SetInput(mesh);
    f2->SetExtractCellTypes({IG_TETRA});
    if (check(f2->Execute(), "Execute with tetra only")) return 1;

    auto out2 = f2->GetOutput();
    auto um2 = DynamicCast<UnstructuredMesh>(out2);
    if (check(um2 != nullptr, "output is UnstructuredMesh")) return 1;
    if (check(out2->GetName() == "ExtractCellsByType_2", "output named ExtractCellsByType_2")) return 1;
    if (check(um2->GetNumberOfCells() == 1, "only 1 tetra extracted")) return 1;
    if (check(um2->GetCellType(0) == IG_TETRA, "extracted cell type is tetra")) return 1;
    // 四面体 (0,3,8,9) → 4 个点
    if (check(um2->GetNumberOfPoints() == 4, "tetra keeps exactly its 4 points")) return 1;
    if (verifyAttributes(out2, 4, 1)) return 1;

    // 数值对账：四面体的 4 个点属性 = 原 0,3,8,9 号点的值
    {
        auto outAttr = out2->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < outAttr->GetNumberOfElements(); i++) {
            auto& attr = outAttr->GetElement(i);
            if (attr.attachmentType == IG_POINT) {
                double v0 = attr.pointer->GetValue(0); // 点0 → 0
                double v3 = attr.pointer->GetValue(1); // 点3 → 30
                double v8 = attr.pointer->GetValue(2); // 点8 → 80
                double v9 = attr.pointer->GetValue(3); // 点9 → 90
                if (attr.pointer->GetName() == "pid_double") {
                    // double 属性精度保留：0.5 / 30.5 / 80.5 / 90.5 逐值一致（float 中转会丢失精度）
                    if (check(v0 == 0.5 && v3 == 30.5 && v8 == 80.5 && v9 == 90.5,
                              "double attribute values keep full precision")) return 1;
                } else if (attr.pointer->GetName() == "pid") {
                    if (check(v0 == 0.0 && v3 == 30.0 && v8 == 80.0 && v9 == 90.0,
                              "point attribute values map to original points")) return 1;
                }
            } else if (attr.attachmentType == IG_CELL) {
                double c0 = attr.pointer->GetValue(0); // 第3个单元(四面体) → 2*100=200
                if (check(c0 == 200.0, "cell attribute value maps to original cell")) return 1;
            }
        }
    }

    // ============ 测试 3：混合单元文件（三角+四边形+四面体+六面体）============
    std::cout << "===== Test 3: mixed-type file test =====" << std::endl;
    auto obj = FileIO::ReadFile("./Models/ExtractCellsByType_mixed.vtk");
    if (obj == nullptr) {
        std::cout << "FAIL: cannot open mixed test model" << std::endl;
        return 1;
    }
    auto inCells = obj->GetCellArray();
    if (inCells == nullptr) {
        std::cout << "FAIL: input has no cells" << std::endl;
        return 1;
    }
    const IGsize fileCellNum = inCells->GetNumberOfCells();
    if (check(fileCellNum == 4, "mixed model has 4 cells")) return 1;

    auto f3 = ExtractCellsByTypeFilter::New();
    f3->SetInput(obj);
    auto fileTypes = f3->GetAvailableCellTypes();
    std::cout << "file cell types:";
    for (auto t : fileTypes) { std::cout << " " << ExtractCellsByTypeFilter::GetCellTypeDisplayName(t); }
    std::cout << std::endl;
    if (check(fileTypes.size() == 4, "mixed model reports 4 distinct cell types")) return 1;

    f3->SetExtractCellTypes(fileTypes); // 全选 = 完整复制
    if (check(f3->Execute(), "Execute with all file types")) return 1;
    auto out3 = f3->GetOutput();
    auto um3 = DynamicCast<UnstructuredMesh>(out3);
    if (check(um3 != nullptr, "file output is UnstructuredMesh")) return 1;
    if (check(um3->GetNumberOfCells() == fileCellNum, "file: all cells extracted")) return 1;
    // 文件模型的 double 属性类型保留
    {
        auto allAttrs = out3->GetAttributeSet()->GetAllAttributes();
        bool seenDouble = false;
        for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); i++) {
            auto& attr = allAttrs->GetElement(i);
            if (attr.pointer->GetName() == "pid_double") {
                if (check(attr.pointer->GetArrayType() == IG_DoubleArray,
                          "file: 'pid_double' keeps DoubleArray type")) return 1;
                seenDouble = true;
            }
        }
        if (!seenDouble) { check(false, "file: pid_double attribute present"); return 1; }
    }

    // 只取第一种类型（三角形），验证输出单元类型全部一致
    f3->SetExtractCellTypes({fileTypes[0]});
    if (check(f3->Execute(), "Execute with single file type")) return 1;
    auto um3b = DynamicCast<UnstructuredMesh>(f3->GetOutput());
    bool allMatch = true;
    for (IGsize i = 0; i < um3b->GetNumberOfCells(); i++) {
        if (um3b->GetCellType(i) != fileTypes[0]) { allMatch = false; break; }
    }
    if (check(allMatch, "file: all output cells match the selected type")) return 1;

    // ============ 测试 4：纯表面文件（三角+四边形）============
    std::cout << "===== Test 4: surface-type file test =====" << std::endl;
    auto obj2 = FileIO::ReadFile("./Models/ExtractCellsByType_surface.vtk");
    if (obj2 == nullptr) {
        std::cout << "FAIL: cannot open surface test model" << std::endl;
        return 1;
    }
    auto f4 = ExtractCellsByTypeFilter::New();
    f4->SetInput(obj2);
    auto surfTypes = f4->GetAvailableCellTypes();
    std::cout << "surface cell types:";
    for (auto t : surfTypes) { std::cout << " " << ExtractCellsByTypeFilter::GetCellTypeDisplayName(t); }
    std::cout << std::endl;
    if (check(surfTypes.size() == 2, "surface model reports 2 distinct cell types")) return 1;
    f4->SetExtractCellTypes({surfTypes[0]}); // 只提取一种
    if (check(f4->Execute(), "Execute with single surface type")) return 1;
    auto um4 = DynamicCast<UnstructuredMesh>(f4->GetOutput());
    bool sfMatch = true;
    for (IGsize i = 0; i < um4->GetNumberOfCells(); i++) {
        if (um4->GetCellType(i) != surfTypes[0]) { sfMatch = false; break; }
    }
    if (check(sfMatch, "surface: all output cells match the selected type")) return 1;

    std::cout << "PASS: ExtractCellsByTypeFilter test finished" << std::endl;

    // ============ 显示（可选 --no-show 跳过） ============
    if (noShow) {
        std::cout << "(--no-show, skip render window)" << std::endl;
        return 0;
    }

    auto scene = Scene::New();
    scene->AddModel(out2); // 显示"四面体提取"结果

    RenderWindow::Pointer window = RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
    return 0;
}
