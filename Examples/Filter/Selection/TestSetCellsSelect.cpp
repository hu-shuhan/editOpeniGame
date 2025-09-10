#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameFileIO.h>
#include <iGameSelectionFilters/iGameSetCellsSelect.h>

int main() {
    /*Read data*/
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }
    /*Set filter*/
    auto filter =
            iGame::iGameSetCellsSelect::New(iGame::Selection::Event::Operate::Add, std::vector<int>({0, 1, 2, 3}));
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput(0));
    /*Result*/
    auto& selectedData = resultData->GetSelection()->GetSelectedItems();
    for (auto& everyTypeData: selectedData) {
        std::cout << "DataType:" << everyTypeData.first << std::endl;
        for (auto& everyObjData: everyTypeData.second) { std::cout << everyObjData.first << ' '; }
        std::cout << "\n\n";
    }
    return 0;
}