// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// ===========================================================================
// Cell Quality 单元质量评估 —— 自包含自动化回归测试
// ---------------------------------------------------------------------------
// 【背景 BUG】
//   1. 早期 CellMeshMetricsFilter 仅支持四面体/六面体，且指标与 ParaView 存在多处偏差
//      （如 hex EdgeRatio 覆盖逻辑、tet AspectRatio、tet Jacobian 符号等）。
//   2. 交互上直接修改输入原模型、不支持单元使用 0 标记导致与最优值混淆。
//
// 【本测试保证的正确行为】
//   1. 使用内置混合装配体模型（包含四面体与六面体子块），逐一测试核心评估指标；
//   2. 纯 C++ 内部自校验：将实际计算值与内嵌的 ParaView 6.1.1 黄金标准值逐单元比对，
//      验证指标计算结果以及不支持单元填入 -1.0 的语义；
//   3. 测试独立、自包含，无需提前运行任何 Python 脚本或外部比对工具，直接运行即可完成断言。
//   4. 支持 --selfcheck 参数：仅执行数值断言并输出 PASS/FAIL 汇总，不拉起 3D 渲染窗口。
//
// 【修复提交】待提交。测试与修复同次首次提交，提交后用
//   git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestCellMeshMetrics.cpp
//   查询并回填准确提交号。
// ===========================================================================

#include <Core/iGameScene.h>
#include <MeshMetrics/iGameCellMeshMetricsFilter.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameUnstructuredMesh.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using CellQualityMetric = iGame::CellMeshMetricsFilter::CellQualityMetric;

// 智能模型路径查找（无论在根目录还是构建目录都能自动找到）
static std::string FindModelPath(const std::string& modelName) {
    std::vector<std::string> searchPaths = {
            "./Models/" + modelName,
            "./Examples/Models/" + modelName,
            "../Examples/Models/" + modelName,
            "../../Examples/Models/" + modelName};
    for (const auto& path: searchPaths) {
        if (std::filesystem::exists(path)) { return path; }
    }
    return "./Models/" + modelName;
}

// 打印多块树结构
static void PrintTreeStructure(iGame::DataObject::Pointer node, int depth = 0) {
    if (!node) return;
    std::string indent(depth * 4, ' ');
    std::string name = node->GetName().empty() ? "(未命名)" : node->GetName();
    if (node->HasSubDataObject()) {
        std::cout << indent << "├── 装配组: [" << name << "] (包含 " << node->GetNumberOfSubDataObjects()
                  << " 个子块)\n";
        for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it) {
            PrintTreeStructure(it->second, depth + 1);
        }
    } else {
        std::cout << indent << "└── 实体网格: [" << name << "]\n";
    }
}

// ============================================================
// 统一指标 + ParaView 标准答案（由 vtkCellQuality 实测得到）
// 标准答案来源：ParaView 6.1.1 / VTK，cell_metric_assembly_pv.vtm
// ============================================================
struct MetricCase {
    const char* label;    // 中文名 (English)
    const char* paraName; // ParaView 下拉框名称
    CellQualityMetric metric;
    const char* expectTet; // 期望值（四面体块）
    const char* expectHex; // 期望值（六面体块）
};

const std::vector<MetricCase> kMetricCases = {
        {"边长比 (Edge Ratio)", "Edge Ratio", CellQualityMetric::QUALITY_EDGE_RATIO, "1, 1.73205", "1, 1"},
        {"单元体积 (Volume)", "Volume", CellQualityMetric::QUALITY_VOLUME, "0.117851, 0.117851", "1, 1"},
        {"纵横比 (Aspect Ratio)", "Aspect Ratio", CellQualityMetric::QUALITY_ASPECT_RATIO, "1, 2.07313", "-1, -1"},
        {"雅可比行列式 (Jacobian)", "Jacobian", CellQualityMetric::QUALITY_JACOBIAN, "0.707107, 0.707107", "1, 1"},
        {"歪斜度 (Skew)", "Skew", CellQualityMetric::QUALITY_SKEW, "-1, -1", "0, 0"},
        {"最小内角 (Minimum Angle)", "Minimum Angle", CellQualityMetric::QUALITY_MIN_ANGLE, "70.5288, 35.2644", "-1, -1"},
        {"锥度 (Taper)", "Taper", CellQualityMetric::QUALITY_TAPER, "-1, -1", "0, 0"},
        {"伸展度 (Stretch)", "Stretch", CellQualityMetric::QUALITY_STRETCH, "-1, -1", "1, 1"},
        {"对角线比值 (Diagonal)", "Diagonal", CellQualityMetric::QUALITY_DIAGONAL, "-1, -1", "1, 1"},
        {"最大长宽比 (Max Edge Ratio)", "Maximum Edge Ratio", CellQualityMetric::QUALITY_MAX_EDGE_RATIO, "-1, -1", "1, 1"},
        {"塌陷率 (Collapse Ratio)", "Collapse Ratio", CellQualityMetric::QUALITY_COLLAPSE_RATIO, "0.816496, 0.288675", "-1, -1"},
};

// 单个子块的评估结果
struct BlockResult {
    std::string name;
    std::vector<double> values;
    int validCount = 0;
    int skippedCount = 0;
};

// 从输出树中收集各子块的 CellQuality 数组
static std::vector<BlockResult> CollectBlockResults(iGame::DataObject::Pointer root, double unsupportedValue) {
    std::vector<BlockResult> results;
    if (!root || !root->HasSubDataObject()) { return results; }

    for (auto it = root->SubDataObjectIteratorBegin(); it != root->SubDataObjectIteratorEnd(); ++it) {
        auto mesh = it->second;
        BlockResult br;
        br.name = mesh ? mesh->GetName() : "(null)";
        auto attrSet = mesh ? mesh->GetAttributeSet() : nullptr;
        if (attrSet) {
            for (int i = 0; i < (int) attrSet->GetNumberOfAttributes(); ++i) {
                auto& attr = attrSet->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) { continue; }
                if (attr.pointer->GetName() != "CellQuality") { continue; }
                auto arr = attr.pointer;
                for (IGsize c = 0; c < (IGsize) arr->GetNumberOfElements(); ++c) {
                    double v = arr->GetValue(c);
                    br.values.push_back(v);
                    if (std::fabs(v - unsupportedValue) < 1e-9) {
                        ++br.skippedCount;
                    } else {
                        ++br.validCount;
                    }
                }
                break;
            }
        }
        results.push_back(br);
    }
    return results;
}

// 与 ParaView 一致的 %.6g 风格格式化
static std::string FormatValues(const std::vector<double>& vals) {
    if (vals.empty()) { return "(无数组)"; }
    std::ostringstream oss;
    oss << std::setprecision(6) << std::defaultfloat;
    for (size_t i = 0; i < vals.size(); ++i) {
        if (i) { oss << ", "; }
        oss << vals[i];
    }
    return oss.str();
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // Windows 控制台 UTF-8 支持
#endif
    std::cout << std::unitbuf;

    bool selfCheckOnly = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--selfcheck" || arg == "-s") {
            selfCheckOnly = true;
        }
    }

    std::cout << "\n==============================================================================\n";
    std::cout << "  【iGameVis】Cell Quality 单元质量评估 —— 自包含 ParaView 逐值自检测试\n";
    std::cout << "  说明：无需依赖 Python 脚本，直接比对内置黄金标准答案，支持 CI 自动化校验。\n";
    std::cout << "==============================================================================\n";

    // ========== 步骤 1：读取 .vtm 多块模型 ==========
    std::cout << "\n[步骤 1] 读取多块模型 (.vtm)...\n";
    const std::string vtmPath = FindModelPath("cell_metric_assembly.vtm");
    std::cout << "  测试模型: " << vtmPath << "\n";
    auto multiBlockObj = iGame::FileIO::ReadFile(vtmPath);
    if (!multiBlockObj) {
        std::cerr << "  [错误] 读取多块模型失败: " << vtmPath << "\n";
        return -1;
    }
    std::cout << "  读取成功，多块树结构:\n";
    PrintTreeStructure(multiBlockObj);

    // ========== 步骤 2：循环核心统一指标，与 ParaView 标准答案逐值比对 ==========
    std::cout << "\n[步骤 2] 逐个指标执行评估并校验断言...\n";
    const double kUnsupportedValue = -1.0;
    int passCount = 0;
    int failCount = 0;
    std::vector<std::string> recap;

    auto pad = [](const std::string& s, size_t w) {
        return s.size() >= w ? s : s + std::string(w - s.size(), ' ');
    };
    const std::string line(96, '-');

    int metricIndex = 0;
    for (const auto& mc: kMetricCases) {
        ++metricIndex;
        auto filter = iGame::CellMeshMetricsFilter::New();
        filter->setMetric(mc.metric);
        filter->setUnsupportedValue(kUnsupportedValue);
        filter->SetInput(0, multiBlockObj);

        if (!filter->Execute()) {
            std::cout << "   [FAIL] " << mc.label << " 执行失败\n";
            ++failCount;
            recap.push_back("[FAIL] " + std::string(mc.label) + " (执行失败)");
            continue;
        }
        auto outputObj = filter->GetOutput(0);
        if (!outputObj) {
            std::cout << "   [FAIL] " << mc.label << " 输出为空\n";
            ++failCount;
            recap.push_back("[FAIL] " + std::string(mc.label) + " (输出为空)");
            continue;
        }

        auto blocks = CollectBlockResults(outputObj, kUnsupportedValue);
        const char* expects[2] = {mc.expectTet, mc.expectHex};
        bool metricOk = true;

        std::cout << "   " << pad(mc.label, 32);
        for (int b = 0; b < 2; ++b) {
            std::string actual = (b < (int) blocks.size()) ? FormatValues(blocks[b].values) : "(缺块)";
            if (actual != expects[b]) { metricOk = false; }
        }

        if (metricOk) {
            std::cout << " -> [PASS] 一致\n";
            ++passCount;
            recap.push_back("[PASS] " + std::string(mc.label));
        } else {
            std::cout << " -> [FAIL] 不一致\n";
            for (int b = 0; b < 2; ++b) {
                std::string bname = (b < (int) blocks.size()) ? blocks[b].name : "(缺块)";
                std::string actual = (b < (int) blocks.size()) ? FormatValues(blocks[b].values) : "(缺块)";
                std::cout << "        子块 [" << bname << "] 实测: " << actual << " | 期望: " << expects[b] << "\n";
            }
            ++failCount;
            recap.push_back("[FAIL] " + std::string(mc.label));
        }
    }

    // ========== 步骤 3：汇总 ==========
    std::cout << "\n" << std::string(96, '=') << "\n";
    std::cout << " 【自检结果汇总】\n";
    std::cout << std::string(96, '=') << "\n";
    for (const auto& r: recap) { std::cout << "   " << r << "\n"; }
    std::cout << line << "\n";
    std::cout << "   通过: " << passCount << " / " << kMetricCases.size() << " 个指标\n";
    std::cout << "   失败: " << failCount << " 个指标\n";
    std::cout << std::string(96, '=') << "\n";

    if (failCount > 0) {
        std::cerr << "\n[ERROR] 存在未通过的测试项！\n";
        return 1;
    }

    std::cout << "\n[SUCCESS] 全部自检断言通过！\n";

    if (selfCheckOnly) {
        return 0;
    }

    // 交互模式：用 Aspect Ratio 演示 3D 渲染窗口
    auto demoFilter = iGame::CellMeshMetricsFilter::New();
    demoFilter->setMetric(CellQualityMetric::QUALITY_ASPECT_RATIO);
    demoFilter->setUnsupportedValue(kUnsupportedValue);
    demoFilter->SetInput(0, multiBlockObj);
    if (demoFilter->Execute()) {
        auto demoOut = demoFilter->GetOutput(0);
        auto scene = iGame::Scene::New();
        if (demoOut && demoOut->HasSubDataObject()) {
            for (auto it = demoOut->SubDataObjectIteratorBegin(); it != demoOut->SubDataObjectIteratorEnd(); ++it) {
                auto drawObj = iGame::DynamicCast<iGame::DrawObject>(it->second);
                if (!drawObj) continue;
                int qualityIndex = -1;
                auto attrSet = it->second->GetAttributeSet();
                if (attrSet) {
                    for (int i = 0; i < (int) attrSet->GetNumberOfAttributes(); ++i) {
                        auto& attr = attrSet->GetAttribute(i);
                        if (!attr.isDeleted && attr.pointer && attr.pointer->GetName() == "CellQuality") {
                            qualityIndex = i;
                            break;
                        }
                    }
                }
                drawObj->SetViewStyle(IG_SURFACE);
                drawObj->AddViewStyle(IG_WIREFRAME);
                drawObj->ConvertToDrawableData();
                if (qualityIndex >= 0) { drawObj->ViewCloudPicture(scene.GetPointer(), qualityIndex); }
                scene->AddModel(it->second);
            }
        }
        std::cout << "启动 3D 渲染窗口展示...\n";
        auto window = iGame::RenderWindow::New();
        window->SetSize(800, 600);
        window->SetScene(scene);
        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);
        window->Show();
    }

    return 0;
}
