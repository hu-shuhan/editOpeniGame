#include <iGameFileIO.h>
#include <iGameParallelCoordinatesFilter/iGameGenerateParallelCoordinatesData.h>
#include <iostream>
#include <cmath>
#include <algorithm>

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
    auto filter = iGame::iGameGenerateParallelCoordinatesData::New(IG_POINT);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::ParallelCoordinatesData>(resultData);
    auto& variableNames = theData->GetVariableName();
    /*data procurement*/
    for (auto& variableName: variableNames) { std::cout << variableName << '\t'; }
    std::cout << std::endl;
    for (int i = 0; i < std::min<int>(10, theData->GetKeyObjectIds().size()); i++) {
        auto objId = theData->GetKeyObjectIds()[i];
        for (int v = 0; v < theData->GetVariableNum(); v++) {
            auto value = theData->GetObjectData(objId, v);
            std::cout << value << '\t';
        }
        std::cout << std::endl;
    }
    /*Data selection*/
    //The selected content in the case is a variable with subscript 0, and its range is 0 to 1.
    //For the input of multiple variables, the final choice is the union of the filtering results of their respective variables.
    std::map<int, std::pair<double, double>> variableMinMaxValues{{0, std::pair<double, double>(0, 1)}};
    auto pointIds = theData->FiltInRangeIds(variableMinMaxValues);
    for (auto& pId: pointIds) { std::cout << pId << ' '; }
    std::cout << std::endl;
    return 0;
}