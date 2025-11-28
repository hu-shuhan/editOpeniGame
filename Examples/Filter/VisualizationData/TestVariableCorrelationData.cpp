#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameFileIO.h>
#include <iGameVariableCorrelationFilter/iGameGenerateVariableCorrelationData.h>

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
    auto filter = iGame::GenerateVariableCorrelationDataFilter::New(IG_POINT);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::VariableCorrelationData>(resultData);
    auto& variableNames = theData->GetVariableName();
    /*Get variable correlation*/
    for (auto& variableName: variableNames) { std::cout << variableName << '\t'; }
    std::cout << std::endl;
    auto& variableCors = theData->GetVariableCorrelation();
    for (auto& variableCor: variableCors) {
        for (auto& cor: variableCor) { std::cout << cor << '\t'; }
        std::cout << std::endl;
    }
    /*Data selection*/
    //Select the variables with subscript 0 and subscript 1, and select the maximum and minimum values for them.
    //Object ids that meet all the maximum and minimum values will be obtained.
    auto pointIds = theData->FiltInRangeIds(0, 1, 0.54, 0.62, 0.31, 0.55);
    for (auto& pId: pointIds) { std::cout << pId << ' '; }
    std::cout << std::endl;
    return 0;
}