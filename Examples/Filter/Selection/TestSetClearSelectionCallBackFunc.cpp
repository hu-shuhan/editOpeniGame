#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameFileIO.h>
#include <iGameSelectionFilters/iGameSetClearSelectionCallBackFunc.h>

static void ClearCallBackFunc() { std::cout << "Cleared" << std::endl; }

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
    auto filter = iGame::SetClearSelectionCallBackFuncFilter::New("ClearCallBackFunc_1", ClearCallBackFunc);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput(0));
    /*Clear*/
    resultData->GetSelection()->Reset();
    return 0;
}