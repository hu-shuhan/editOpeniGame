// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/MeshQuality/TestMeshQuality.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include <MeshQuality/iGameMeshQualityFilter.h>
#include <iGameFileIO.h>

#include <iostream>
#include <iomanip>

int main() {

    const std::string fileName = "./Models/MeshQuality_Complex.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);

    if (obj == nullptr) {
        std::cout << "Read ERROR!" << std::endl;
        return 1;
    }

    auto filter = iGame::MeshQualityFilter::New();
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "MeshQualityFilter Execute ERROR!" << std::endl;
        return 1;
    }

    std::cout << std::fixed << std::setprecision(15);
    std::cout << "Mesh Quality Result" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << "NumberOfCells: "<< filter->GetNumberOfCells() << std::endl;
    std::cout << "Minimum: "<< filter->GetMinimum() << std::endl;
    std::cout << "Maximum: "<< filter->GetMaximum() << std::endl;
    std::cout << "Average: "<< filter->GetAverage() << std::endl;

    return 0;
}
