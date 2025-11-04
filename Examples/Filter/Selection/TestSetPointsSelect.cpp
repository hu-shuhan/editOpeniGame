#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameFileIO.h>
#include <iGameSelectionFilters/iGameSetPointsSelect.h>

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
            iGame::iGameSetPointsSelect::New(iGame::Selection::Operate::Add, std::vector<int>({0, 1, 2, 3, 4}));
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput(0));
    /*Result*/
    auto& selectedData = resultData->GetSelection()->GetSelectedItems(IG_POINT);
    std::cout << "DataType:" << IG_POINT << std::endl;
    std::cout << "Id:" << std::endl;
    for (auto& id: selectedData) { std::cout << id << ','; }
    std::cout << "\n\n";
    return 0;
}