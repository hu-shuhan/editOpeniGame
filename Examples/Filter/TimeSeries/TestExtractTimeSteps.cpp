// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
#include <TimeSeries/iGameExtractTimeStepsFilter.h>

#include <iGameFileIO.h>

#include <iostream>
#include <vector>

int main() {
    // 带时间序列的模型：Models/sequence.pvd（21 帧，时间值 0 ~ 10）
    auto object = iGame::FileIO::ReadFile("./Models/sequence.pvd");
    if (object == nullptr) {
        std::cout << "Read ERROR!" << std::endl;
        return 1;
    }

    // 保留第 0、5、10、15、20 帧
    auto filter = iGame::ExtractTimeStepsFilter::New();
    filter->SetTimeStepIndices({0, 5, 10, 15, 20});
    filter->SetInput(object);
    if (!filter->Execute()) {
        std::cout << "ExtractTimeSteps ERROR!" << std::endl;
        return 1;
    }

    std::cout << "Kept time steps: " << filter->GetNumberOfKeptTimeSteps() << std::endl;
    std::cout << "Kept indices :";
    for (int index : filter->GetKeptTimeStepIndices()) { std::cout << " " << index; }
    std::cout << std::endl;
    std::cout << "Time values  :";
    for (float value : filter->GetKeptTimeValues()) { std::cout << " " << value; }
    std::cout << std::endl;

    const bool pass = filter->GetKeptTimeStepIndices() == std::vector<int>{0, 5, 10, 15, 20};
    std::cout << "Result: " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass ? 0 : 1;
}
