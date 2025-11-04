#include <algorithm>
#include <cmath>
#include <iGameFileIO.h>
#include <iGameSelectionFilters/iGameGetClosestPointsInLine.h>
#include <iostream>

static void ShowFilterFunc(iGame::UnstructuredMesh::Pointer mesh, const iGame::Point& startPoint,
                           const iGame::Point& endPoint, double radius = 0.0, bool useAutoValueRange = false,
                           int variableIndex = -1, double expdRate = 1.0) {
    /*Set filter*/
    auto filter = iGame::iGameGetClosestPointsInLine::New(startPoint, endPoint, radius, useAutoValueRange,
                                                          variableIndex, expdRate);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return;
    }
    /*Get results*/
    auto& resultVector = filter->GetResult();
    for (auto& id: resultVector) { std::cout << id << ", "; }
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
}

int main() {
    /*Read data*/
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }
    ShowFilterFunc(mesh, iGame::Point(1.0f, 0.0f, 0.0f), iGame::Point(-1.0f, 0.0f, 0.0f), 0.2);
    ShowFilterFunc(mesh, iGame::Point(1.0f, 0.0f, 0.0f), iGame::Point(-1.0f, 0.0f, 0.0f), 0.2, true, 0, 1.0);
    ShowFilterFunc(mesh, iGame::Point(1.0f, 0.0f, 0.0f), iGame::Point(-1.0f, 0.0f, 0.0f), 0.2, true, 0, 0.1);

    return 0;
}