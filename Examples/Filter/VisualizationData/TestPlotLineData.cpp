#include <PlotLine/iGameGeneratePlotLineDataFilter.h>
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
    auto filter = iGame::GeneratePlotLineDataFilter::New(IG_POINT, iGame::Point(-1.0f, -1.0f, -1.0f),
                                                         iGame::Point(1.0f, 1.0f, 1.0f));
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return 0;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::PlotLineData>(resultData);
    auto& objIndexs = theData->GetObjIndexs();
    /*Get the corresponding pointIds, distance and datas*/
    for (auto& objIndex_: objIndexs) {
        auto objId = objIndex_.first;
        auto objIndexInObjDistance = objIndex_.second;
        std::cout << objId;
        std::cout << "(distance " << theData->GetObjDistance()[objIndexInObjDistance] << "):";
        for (int v = 0; v < theData->GetVariableNum(); v++) {
            auto value = theData->GetObjectData(objId, v);
            std::cout << value << ' ';
        }
        std::cout << std::endl;
    }
    /*Data selection*/
    //A value of true corresponding to variableCanBeChoose indicates that this variable will be considered in the filter.
    //Union between multiple variables that are true.
    auto pointIds = theData->FiltInRangeIds(0, 2.0, 0.004, 0.01, {true, false, false, false});
    for (auto& pId: pointIds) { std::cout << pId << ' '; }
    std::cout << std::endl;
    return 0;
}
