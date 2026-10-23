// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
#include <TimeSeries/iGameTemporalShiftScaleFilter.h>

#include <iGameFileIO.h>

#include <cmath>
#include <cstddef>
#include <iostream>

int main() {
    // 带时间序列的模型：Models/sequence.pvd（21 帧，时间值 0 ~ 10）
    auto object = iGame::FileIO::ReadFile("./Models/sequence.pvd");
    if (object == nullptr) {
        std::cout << "Read ERROR!" << std::endl;
        return 1;
    }

    // t' = (t + PreShift) * Scale + PostShift，这里取 t' = 2t + 1
    auto filter = iGame::TemporalShiftScaleFilter::New();
    filter->SetPreShift(-1.0f);
    filter->SetScale(2.0f);
    filter->SetPostShift(3.0f);
    filter->SetInput(object);
    if (!filter->Execute()) {
        std::cout << "TemporalShiftScale ERROR!" << std::endl;
        return 1;
    }

    const auto& inValues = filter->GetInTimeValues();
    const auto& outValues = filter->GetOutTimeValues();
    std::cout << "Time steps  : " << filter->GetNumberOfTimeSteps() << std::endl;
    std::cout << "Input time  :";
    for (float value : inValues) { std::cout << " " << value; }
    std::cout << std::endl;
    std::cout << "Output time :";
    for (float value : outValues) { std::cout << " " << value; }
    std::cout << std::endl;

    // 帧数不变，且每一帧都满足 t' = 2t + 1
    bool pass = inValues.size() == 21 && outValues.size() == inValues.size();
    for (std::size_t i = 0; pass && i < inValues.size(); ++i) {
        pass = std::abs(outValues[i] - (2.0f * inValues[i] + 1.0f)) < 1e-4f;
    }

    if (pass) {
        // 输出是独立对象，带自己的时间轴；输入的时间轴保持不变
        auto inputFrames = object->PeekTimeFrames();
        auto outputFrames = filter->GetOutput()->PeekTimeFrames();
        pass = outputFrames != inputFrames &&
               std::abs(inputFrames->GetTargetTimeFrame(20).GetTimeValue() - 10.0f) < 1e-4f;
    }

    std::cout << "First / last: " << outValues.front() << " / " << outValues.back() << std::endl;
    std::cout << "Result: " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass ? 0 : 1;
}
