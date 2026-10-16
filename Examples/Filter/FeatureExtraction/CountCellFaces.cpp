// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/FeatureExtraction/CountCellFaces.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include "FeatureExtraction/iGameCountCellFacesFilter.h"
#include "iGameFileIO.h"

#include <initializer_list>
#include <iostream>
#include <stdexcept>

namespace {
void CheckModel(const char* path, std::initializer_list<unsigned int> expected) {
    auto input = iGame::FileIO::ReadFile(path);
    if (!input) throw std::runtime_error("Failed to read model; run from the example directory.");
    auto filter = iGame::CountCellFacesFilter::New();
    filter->SetInput(input);
    if (!filter->Execute()) throw std::runtime_error(filter->GetMessage());
    auto output = filter->GetOutput();
    auto counts = filter->GetResult();
    if (!output || !counts || counts->GetNumberOfValues() != expected.size())
        throw std::runtime_error("Unexpected output or result length.");
    const auto& attribute = output->GetAttributeSet()->GetAttribute("cellFaceCounts");
    if (attribute.pointer != counts || attribute.attachmentType != IG_CELL)
        throw std::runtime_error("Missing cellFaceCounts cell attribute.");

    std::cout << "Input: " << path << '\n';
    IGsize id = 0;
    for (auto value : expected) {
        if (counts->GetValue(id) != value) throw std::runtime_error("Wrong face count.");
        std::cout << "cell[" << id << "] = " << counts->GetValue(id) << '\n';
        ++id;
    }
    std::cout << "Face counts and cell attribute: PASS\n";
}
} // namespace

int main() {
    try {
        CheckModel("Models/CountCellFaces_MixedCells.vtk", {4, 6, 5, 5, 0, 0, 0});
        CheckModel("Models/CountCellFaces_QuadTensor.vtk", {0, 0});
        std::cout << "CountCellFaces: all tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
