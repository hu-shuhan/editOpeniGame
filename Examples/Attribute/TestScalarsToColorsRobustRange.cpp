#include "iGameFlatArray.h"
#include "iGameScalarsToColors.h"

#include <cmath>
#include <iostream>

namespace {

bool IsClose(const double left, const double right, const double tolerance) {
    return std::abs(left - right) <= tolerance;
}

}

int main() {
    auto mapper = iGame::ScalarsToColors::New();
    if (mapper->GetAutoRangeMode() != iGame::ScalarsToColors::EXACT_AUTO_RANGE) {
        std::cerr << "default auto range mode is not exact\n";
        return 1;
    }
    mapper->SetAutoRangeMode(iGame::ScalarsToColors::ROBUST_AUTO_RANGE);
    if (mapper->GetAutoRangeMode() != iGame::ScalarsToColors::ROBUST_AUTO_RANGE) {
        std::cerr << "robust auto range mode was not retained\n";
        return 1;
    }

    auto values = iGame::DoubleArray::New();
    values->SetDimension(1);
    values->Resize(1002);
    values->SetValue(0, -1.0e12);
    for (IGsize index = 1; index <= 1000; ++index) {
        values->SetValue(index, 10.0 + static_cast<double>(index % 10));
    }
    values->SetValue(1001, 1.0e12);

    mapper->InitRangeRobust(values, 0);
    const double* robustRange = mapper->GetRange();
    if (robustRange[0] < 9.0 || robustRange[0] > 11.0 ||
        robustRange[1] < 18.0 || robustRange[1] > 20.0) {
        std::cerr << "robust range did not reject isolated outliers: ["
                  << robustRange[0] << ", " << robustRange[1] << "]\n";
        return 1;
    }

    auto constantValues = iGame::DoubleArray::New();
    constantValues->SetDimension(1);
    constantValues->Resize(32);
    for (IGsize index = 0; index < 32; ++index) {
        constantValues->SetValue(index, 300.0);
    }
    mapper->InitRangeRobust(constantValues, 0);
    const double* constantRange = mapper->GetRange();
    if (!(constantRange[0] < 300.0 && constantRange[1] > 300.0) ||
        !IsClose((constantRange[0] + constantRange[1]) * 0.5, 300.0, 1e-9)) {
        std::cerr << "constant robust range was not centered around the value\n";
        return 1;
    }

    std::cout << "ScalarsToColors robust range contract passed\n";
    return 0;
}
