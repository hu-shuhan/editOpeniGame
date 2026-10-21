#include <MaskPoints/iGameMaskPointsFilter.h>

#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <string>

int main() {
    const std::string fileName = "./Examples/Models/MaskPoints_AI_Test.vtk";

    std::cout << "Mask Points model test" << std::endl;
    std::cout << "Model: " << fileName << std::endl;

    auto obj = iGame::FileIO::ReadFile(fileName);

    if (obj.IsNull()) {
        std::cout << "Failed: cannot read model file." << std::endl;
        return 1;
    }

    auto input = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);

    if (input.IsNull()) {
        std::cout << "Failed: input is not an UnstructuredMesh." << std::endl;
        return 1;
    }

    std::cout << "Input points: " << input->GetNumberOfPoints() << std::endl;
    std::cout << "Input cells: " << input->GetNumberOfCells() << std::endl;

    if (input->GetNumberOfPoints() != 27) {
        std::cout << "Failed: unexpected input point count." << std::endl;
        return 1;
    }

    if (input->GetNumberOfCells() != 8) {
        std::cout << "Failed: unexpected input cell count." << std::endl;
        return 1;
    }

    auto filter = iGame::MaskPointsFilter::New();

    filter->SetInput(0, input);
    filter->SetOnRatio(2);
    filter->SetOffset(0);
    filter->SetMaximumNumberOfPoints(0);
    filter->SetRandomMode(false);
    filter->SetGenerateVertices(false);

    if (!filter->Execute()) {
        std::cout << "Failed: MaskPointsFilter execution failed." << std::endl;
        return 1;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) {
        std::cout << "Failed: output is null." << std::endl;
        return 1;
    }

    std::cout << "Output points: " << output->GetNumberOfPoints() << std::endl;
    std::cout << "Output cells: " << output->GetNumberOfCells() << std::endl;

    // 27 points with OnRatio = 2:
    // selected point IDs are 0, 2, 4, ..., 26, for a total of 14 points.
    if (output->GetNumberOfPoints() != 14) {
        std::cout << "Failed: unexpected output point count." << std::endl;
        return 1;
    }

    // GenerateVertices = false, so the output should contain points only.
    if (output->GetNumberOfCells() != 0) {
        std::cout << "Failed: output should not contain cells." << std::endl;
        return 1;
    }

    auto attrSet = output->GetAttributeSet();

    if (attrSet == nullptr) {
        std::cout << "Failed: output has no attribute set." << std::endl;
        return 1;
    }

    auto allAttrs = attrSet->GetAllAttributes();
    bool pointValueFound = false;

    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); ++i) {
        auto attr = allAttrs->GetElement(i);

        if (attr.attachmentType != IG_POINT) { continue; }

        auto array = attr.pointer;

        if (array.IsNull() || array->GetName() != "PointValue") { continue; }

        pointValueFound = true;

        if (array->GetNumberOfElements() != output->GetNumberOfPoints()) {
            std::cout << "Failed: PointValue size does not match output point count." << std::endl;
            return 1;
        }

        double value[IGAME_CELL_MAX_SIZE] = {0};

        for (IGsize i = 0; i < output->GetNumberOfPoints(); ++i) {
            array->GetElement(i, value);

            const double expectedValue = static_cast<double>(i * 2);

            if (value[0] != expectedValue) {
                std::cout << "Failed: PointValue mapping error at output point " << i << "." << std::endl;
                return 1;
            }
        }

        break;
    }

    if (!pointValueFound) {
        std::cout << "Failed: PointValue attribute was not preserved." << std::endl;
        return 1;
    }

    std::cout << "Success: model loaded automatically." << std::endl;
    std::cout << "Success: MaskPointsFilter executed correctly." << std::endl;
    std::cout << "Success: point sampling result is correct." << std::endl;
    std::cout << "Success: PointValue attribute is preserved correctly." << std::endl;
    std::cout << "All Mask Points model tests passed." << std::endl;

    return 0;
}