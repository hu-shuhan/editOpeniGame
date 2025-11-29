#include <VariableDensity/iGameGenerateVariableDensityDataFilter.h>
#include <algorithm>
#include <cmath>
#include <iGameFileIO.h>
#include <iostream>

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
    //Read point data (cell data uses IG_CELL)
    auto filter = iGame::GenerateVariableDensityDataFilter::New(IG_POINT, 100);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::VariableDensityData>(resultData);
    auto& variableNames = theData->GetVariableName();
    auto& densitys = theData->GetDensity();
    /*Obtain the density of each variable (histogram data)*/
    for (int vI = 0; vI < theData->GetVariableNum(); vI++) {
        std::cout << variableNames[vI] << std::endl;
        for (auto& density: densitys[vI]) { std::cout << density << ' '; }
        std::cout << std::endl;
    }
    /*Data selection*/
    auto pointIds = theData->FiltInRangeIds(0, 0.504, 0.505);
    for (auto& pId: pointIds) { std::cout << pId << ' '; }
    std::cout << std::endl;
    return 0;
}
