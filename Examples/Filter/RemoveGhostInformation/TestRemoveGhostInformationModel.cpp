#include <RemoveGhostInformation/iGameRemoveGhostInformationFilter.h>

#include <VTK/iGameGhostVTKReader.h>
#include <iGameAttributeSet.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

bool IsGhostName(const std::string& name) {
    std::string lowerName = name;

    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lowerName == "vtkghosttype";
}

int main() {
    const std::string fileName = "./Examples/Models/RemoveGhostInformation_AI_Test.vtk";

    std::cout << "Remove Ghost Information model test" << std::endl;
    std::cout << "Model: " << fileName << std::endl;

    auto obj = iGame::GhostVTKReader::ReadFile(fileName);

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

    if (input->GetNumberOfPoints() != 8) {
        std::cout << "Failed: unexpected input point count." << std::endl;
        return 1;
    }

    if (input->GetNumberOfCells() != 2) {
        std::cout << "Failed: unexpected input cell count." << std::endl;
        return 1;
    }

    auto inputAttrSet = input->GetAttributeSet();

    if (inputAttrSet == nullptr) {
        std::cout << "Failed: input has no attribute set." << std::endl;
        return 1;
    }

    auto inputAttrs = inputAttrSet->GetAllAttributes();

    bool inputGhostFound = false;

    for (IGsize i = 0; i < inputAttrs->GetNumberOfElements(); ++i) {
        auto attr = inputAttrs->GetElement(i);

        if (attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_CELL) { continue; }
        if (!IsGhostName(attr.pointer->GetName())) { continue; }

        inputGhostFound = true;

        auto ghostArray = attr.pointer;

        if (ghostArray->GetNumberOfElements() != 2) {
            std::cout << "Failed: unexpected vtkGhostType size." << std::endl;
            return 1;
        }

        const int ghost0 = static_cast<int>(ghostArray->GetValue(0));
        const int ghost1 = static_cast<int>(ghostArray->GetValue(1));

        std::cout << "Input vtkGhostType: " << ghost0 << ", " << ghost1 << std::endl;

        if (ghost0 != 0 || ghost1 != 1) {
            std::cout << "Failed: vtkGhostType values are incorrect." << std::endl;
            return 1;
        }

        break;
    }

    if (!inputGhostFound) {
        std::cout << "Failed: input vtkGhostType was not found." << std::endl;
        return 1;
    }

    auto filter = iGame::RemoveGhostInformationFilter::New();

    filter->SetInput(0, input);

    if (!filter->Execute()) {
        std::cout << "Failed: RemoveGhostInformationFilter execution failed." << std::endl;
        return 1;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

    if (output.IsNull()) {
        std::cout << "Failed: output is null." << std::endl;
        return 1;
    }

    std::cout << "Output points: " << output->GetNumberOfPoints() << std::endl;
    std::cout << "Output cells: " << output->GetNumberOfCells() << std::endl;

    if (output->GetNumberOfPoints() != 4) {
        std::cout << "Failed: unexpected output point count." << std::endl;
        return 1;
    }

    if (output->GetNumberOfCells() != 1) {
        std::cout << "Failed: unexpected output cell count." << std::endl;
        return 1;
    }

    auto outputAttrSet = output->GetAttributeSet();

    if (outputAttrSet == nullptr) {
        std::cout << "Failed: output has no attribute set." << std::endl;
        return 1;
    }

    auto outputAttrs = outputAttrSet->GetAllAttributes();

    bool ghostFound = false;
    bool pointValueFound = false;
    bool cellValueFound = false;

    for (IGsize i = 0; i < outputAttrs->GetNumberOfElements(); ++i) {
        auto attr = outputAttrs->GetElement(i);

        if (attr.pointer == nullptr) { continue; }

        auto array = attr.pointer;
        const std::string name = array->GetName();

        if (IsGhostName(name)) {
            ghostFound = true;
            continue;
        }

        if (attr.attachmentType == IG_POINT && name == "PointValue") {
            if (array->GetNumberOfElements() != 4) {
                std::cout << "Failed: PointValue size is incorrect." << std::endl;
                return 1;
            }

            double value[IGAME_CELL_MAX_SIZE] = {0};

            for (IGsize j = 0; j < 4; ++j) {
                array->GetElement(j, value);

                const double expected = 100.0 + static_cast<double>(j);

                if (value[0] != expected) {
                    std::cout << "Failed: PointValue mapping error." << std::endl;
                    return 1;
                }
            }

            pointValueFound = true;
        }

        if (attr.attachmentType == IG_CELL && name == "CellValue") {
            if (array->GetNumberOfElements() != 1) {
                std::cout << "Failed: CellValue size is incorrect." << std::endl;
                return 1;
            }

            double value[IGAME_CELL_MAX_SIZE] = {0};
            array->GetElement(0, value);

            if (value[0] != 10.0) {
                std::cout << "Failed: CellValue mapping error." << std::endl;
                return 1;
            }

            cellValueFound = true;
        }
    }

    if (ghostFound) {
        std::cout << "Failed: vtkGhostType still exists in output." << std::endl;
        return 1;
    }

    if (!pointValueFound) {
        std::cout << "Failed: PointValue was not preserved." << std::endl;
        return 1;
    }

    if (!cellValueFound) {
        std::cout << "Failed: CellValue was not preserved." << std::endl;
        return 1;
    }

    std::cout << "Success: model loaded automatically." << std::endl;
    std::cout << "Success: vtkGhostType values were read correctly." << std::endl;
    std::cout << "Success: ghost cell was removed correctly." << std::endl;
    std::cout << "Success: unused points were removed correctly." << std::endl;
    std::cout << "Success: vtkGhostType was removed." << std::endl;
    std::cout << "Success: normal point and cell attributes were preserved." << std::endl;
    std::cout << "All Remove Ghost Information model tests passed." << std::endl;

    return 0;
}