#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameFileIO.h>
#include <iGameSelectionFilters/iGameGetClosestCellsInLine.h>

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
    auto filter = iGame::iGameGetClosestCellsInLine::New(iGame::Point(-1.0f, -1.0f, -1.0f),
                                                         iGame::Point(1.0f, 1.0f, 1.0f), 0.1);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto& resultVector = filter->GetResult();
    for (auto& id: resultVector) { std::cout << id << ", "; }
    std::cout << std::endl;
    return 0;
}