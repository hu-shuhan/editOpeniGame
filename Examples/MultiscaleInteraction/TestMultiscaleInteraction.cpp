#include <iGameFileIO.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameParallelCoordinatesFilter/iGameGenerateParallelCoordinatesData.h>
#include <iGameVariableCorrelationFilter/iGameGenerateVariableCorrelationData.h>
#include <iGameVariableDensityFilter/iGameGenerateVariableDensityData.h>
#include <iGamePlotLineFilter/iGameGeneratePlotLineData.h>
#include <iGameSelectionFilters/iGameSetSelectionCallBackFunc.h>

static constexpr auto BOXNUM = 50;

static iGame::ParallelCoordinatesData::Pointer GetParallelCoordinatesData(iGame::UnstructuredMesh::Pointer mesh) {
    /*Set filter*/
    //Read point data (cell data uses IG_CELL)
    auto filter = iGame::iGameGenerateParallelCoordinatesData::New(IG_POINT);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return nullptr;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::ParallelCoordinatesData>(resultData);
    return theData;
}

static iGame::VariableCorrelationData::Pointer GetVariableCorrelationData(iGame::UnstructuredMesh::Pointer mesh) {
    /*Set filter*/
    //Read point data (cell data uses IG_CELL)
    auto filter = iGame::iGameGenerateVariableCorrelationData::New(IG_POINT);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return nullptr;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::VariableCorrelationData>(resultData);
    return theData;
}

static iGame::VariableDensityData::Pointer GetVariableDensityData(iGame::UnstructuredMesh::Pointer mesh) {
    /*Set filter*/
    //Read point data (cell data uses IG_CELL)
    auto filter = iGame::iGameGenerateVariableDensityData::New(IG_POINT, BOXNUM);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return nullptr;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::VariableDensityData>(resultData);
    return theData;
}

static iGame::PlotLineData::Pointer GetPlotLineData(iGame::UnstructuredMesh::Pointer mesh,
                                                    const iGame::Point& startPoint, const iGame::Point& endPoint) {
    /*Set filter*/
    //Read point data (cell data uses IG_CELL)
    auto filter = iGame::iGameGeneratePlotLineData::New(IG_POINT, startPoint, endPoint);
    filter->SetInput(0, mesh);
    auto resultStation = filter->Execute();
    if (!resultStation) {
        std::cout << "Mesh ERROR!\n";
        return nullptr;
    }
    /*Get results*/
    auto resultData = filter->GetOutput(0);
    auto theData = iGame::DynamicCast<iGame::PlotLineData>(resultData);
    return theData;
}

static void SetSelectionCallBackFunc(iGame::UnstructuredMesh::Pointer mesh,
                                     iGame::ParallelCoordinatesData::Pointer parallelCoordinatesData,
                                     iGame::VariableCorrelationData::Pointer variableCorrelationData,
                                     iGame::VariableDensityData::Pointer variableDensityData,
                                     iGame::PlotLineData::Pointer plotLineData) {
    auto filterA = iGame::iGameSetSelectionCallBackFunc::New(
            "ParallelCoordinatesDataCBF", [&](const std::vector<iGame::Selection::Event>& _events) {
                for (auto& e: _events) {
                    //switch (e.type) {
                    //    case iGame::Selection::Event::Type::PickPoint:
                    //        if (Data->GetDataType() != IG_POINT) break;
                    //        if (e.operate == Selection::Event::Operate::Add)
                    //            Data->AddChoosedObjectData(e.pickId, GetObjectData(Data->GetDataType(), e.pickId));
                    //        else if (e.operate == Selection::Event::Operate::Remove)
                    //            Data->RemoveChoosedObjectData(e.pickId);
                    //        break;
                    //    case iGame::Selection::Event::Type::PickFace:
                    //        if (Data->GetDataType() != IG_CELL) break;
                    //        if (e.operate == iGame::Selection::Event::Operate::Add)
                    //            Data->AddChoosedObjectData(e.pickId, GetObjectData(Data->GetDataType(), e.pickId));
                    //        else if (e.operate == Selection::Event::Operate::Remove)
                    //            Data->RemoveChoosedObjectData(e.pickId);
                    //        break;
                    //    default:
                    //        break;
                    //}
                }
            });
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

}