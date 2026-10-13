// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/ValidateCells/TestValidateCellsFilter.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <ValidateCells/iGameValidateCellsFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameUnstructuredMesh.h>
#include <iGameCellArray.h>
#include <cstdio>
#include <map>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    auto scene = iGame::Scene::New();

    std::string fileName = "././Models/iGameValidateCellsFilter_mixed.vtk";
    if (argc > 1) {
        fileName = argv[1];
    }
    std::cout << "加载模型: " << fileName << std::endl;
    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "读取文件失败!\n";
        std::cout << "按任意键退出...";
        std::cin.get();
        return 0;
    }

    auto filter = iGame::ValidateCellsFilter::New();
    filter->SetInput(obj);

    // TEMP DEBUG: cell 150 points as the framework sees them
    {
        auto umesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
        if (umesh) {
            auto idl = iGame::IdArray::New();
            umesh->GetCellPointIds(150, idl);
            for (int i = 0; i < idl->GetNumberOfIds(); ++i) {
                const auto pp = umesh->GetPoint(idl->GetId(i));
                std::printf("local %d gid %lld: %.10g %.10g %.10g\n", i,
                            static_cast<long long>(idl->GetId(i)),
                            static_cast<double>(pp[0]),
                            static_cast<double>(pp[1]),
                            static_cast<double>(pp[2]));
            }
            std::fflush(stdout);
        }
    }

    if (!filter->Execute()) {
        std::cout << "单元校验执行失败: " << filter->GetLastError() << "\n";
        std::cout << "按任意键退出...";
        std::cin.get();
        return 0;
    }

    // ValidateCellsFilter 生成独立的 *_ValidateCells 输出节点，原始数据不会被修改。
    auto output = filter->GetOutput();
    scene->AddModel(output);

    std::cout << "共检查单元: " << filter->GetCheckedCellCount()
              << ", 有效: " << filter->GetValidCellCount()
              << ", 无效: " << filter->GetInvalidCellCount() << std::endl;

    // ValidityState 取值分布（位掩码值 -> 单元个数）。
    std::map<short, int> stateDistribution;
    for (short state : filter->GetValidityStates()) {
        stateDistribution[state]++;
    }
    std::cout << "ValidityState 分布:\n";
    for (const auto& entry : stateDistribution) {
        std::cout << "  state=" << entry.first << " ("
                  << iGame::ValidateCellsFilter::GetValidityStateText(entry.first)
                  << "): " << entry.second << " 个\n";
    }

    if (filter->GetInvalidCellCount() > 0) {
        const auto& ids = filter->GetInvalidCellIds();
        std::cout << "无效单元 ID（最多显示前 50 个）: ";
        const int showCount = std::min<int>(50, static_cast<int>(ids.size()));
        for (int i = 0; i < showCount; ++i) std::cout << ids[i] << " ";
        if (ids.size() > 50) std::cout << "...";
        std::cout << std::endl;
    }

    // 独立性检查：原始模型不应包含 ValidityState 属性。
    const int sourceIndex =
        obj->GetAttributeSet() == nullptr
            ? -1
            : obj->GetAttributeSet()->GetAttributeIndex("ValidityState");
    std::cout << "原始模型 ValidityState 属性索引（应为 -1）: " << sourceIndex
              << std::endl;
    const int outputIndex =
        output->GetAttributeSet() == nullptr
            ? -1
            : output->GetAttributeSet()->GetAttributeIndex("ValidityState");
    std::cout << "输出模型 ValidityState 属性索引（应 >= 0）: " << outputIndex
              << std::endl;

    // 可选：argv[2] 指定逐单元状态转储文件路径（用于离线对比）。
    if (argc > 2) {
        std::ofstream dump(argv[2]);
        const auto& states = filter->GetValidityStates();
        for (size_t id = 0; id < states.size(); ++id) {
            dump << id << " " << states[id] << "\n";
        }
        dump.close();
        std::cout << "逐单元状态已转储到: " << argv[2] << std::endl;
    }

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();  // 此函数会阻塞直到渲染窗口关闭

    // 渲染窗口关闭后，等待用户按键再退出
    std::cout << "按任意键退出..." << std::endl;
    std::cin.get();

    return 0;
}
