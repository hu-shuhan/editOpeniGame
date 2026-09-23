#include <AttributeManipulation/iGameRandomVectorsFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameType.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool CheckBrownianVectors(const iGame::DataObject::Pointer& obj, double minSpeed, double maxSpeed,
                          std::string& reason) {
    auto pointSet = iGame::DynamicCast<iGame::PointSet>(obj);
    if (!pointSet) {
        reason = "output is not a PointSet";
        return false;
    }

    auto attrSet = obj->GetAttributeSet();
    if (attrSet == nullptr) {
        reason = "output has no attribute set";
        return false;
    }
    auto& attr = attrSet->GetVector("BrownianVectors");
    if (attr.pointer == nullptr) {
        reason = "vector attribute 'BrownianVectors' not found";
        return false;
    }
    if (attr.attachmentType != IG_POINT) {
        reason = "vector attribute is not attached to points";
        return false;
    }

    auto vectors = iGame::DynamicCast<iGame::FloatArray>(attr.pointer);
    if (!vectors) {
        reason = "vector attribute is not a FloatArray";
        return false;
    }
    if (vectors->GetDimension() != 3) {
        reason = "vector dimension is not 3";
        return false;
    }
    if (vectors->GetNumberOfElements() != pointSet->GetNumberOfPoints()) {
        reason = "vector element count mismatch";
        return false;
    }

    for (IGsize i = 0; i < vectors->GetNumberOfElements(); ++i) {
        float v[3];
        vectors->GetElement(i, v);
        const double mag = std::sqrt(static_cast<double>(v[0]) * v[0] + static_cast<double>(v[1]) * v[1] +
                                     static_cast<double>(v[2]) * v[2]);
        if (mag < minSpeed - 1e-6 || mag > maxSpeed + 1e-6) {
            reason = "vector magnitude " + std::to_string(mag) + " out of range [" + std::to_string(minSpeed) +
                     ", " + std::to_string(maxSpeed) + "]";
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    // 相对路径：Examples 构建目录会自动把 Examples/Models 拷贝为 ./Models
    const std::string fileName = "./Models/RandomVectors_TestA.vtk";

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "Read file failed: " << fileName << "\n";
        return 1;
    }

    const double minSpeed = 0.0;
    const double maxSpeed = 1.0;

    auto filter = iGame::RandomVectorsFilter::New();
    filter->SetMinimumSpeed(minSpeed);
    filter->SetMaximumSpeed(maxSpeed);
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "Filter Execute failed\n";
        return 1;
    }

    auto output = filter->GetOutput();
    std::string reason;
    if (output.get() == obj.get()) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "filter should produce a new mesh, not modify the input in place\n";
        return 1;
    }
    if (!CheckBrownianVectors(output, minSpeed, maxSpeed, reason)) {
        std::cerr << "Result: FAIL\n";
        std::cerr << reason << "\n";
        return 1;
    }
    if (obj->GetAttributeSet() && obj->GetAttributeSet()->GetVector("BrownianVectors").pointer != nullptr) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "input mesh should not be modified (BrownianVectors must not appear on the original)\n";
        return 1;
    }

    auto pointSet = iGame::DynamicCast<iGame::PointSet>(output);
    auto vectors = iGame::DynamicCast<iGame::FloatArray>(
            output->GetAttributeSet()->GetVector("BrownianVectors").pointer);
    std::cout << "File: " << fileName << "\n";
    std::cout << "Points: " << pointSet->GetNumberOfPoints() << "\n";
    std::cout << "Dimension: " << vectors->GetDimension() << "\n";
    std::cout << "Elements: " << vectors->GetNumberOfElements() << "\n";
    std::cout << "Speed range: [" << minSpeed << ", " << maxSpeed << "]\n";
    std::cout << "Result: PASS\n";
    return 0;
}
