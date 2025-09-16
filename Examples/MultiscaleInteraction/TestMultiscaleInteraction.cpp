#include <iGameFileIO.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iGameParallelCoordinatesFilter/iGameGenerateParallelCoordinatesData.h>
#include <iGameVariableCorrelationFilter/iGameGenerateVariableCorrelationData.h>
#include <iGameVariableDensityFilter/iGameGenerateVariableDensityData.h>
#include <iGamePlotLineFilter/iGameGeneratePlotLineData.h>
#include <iGameSelectionFilters/iGameSetSelectionCallBackFunc.h>
#include <iGameSelectionFilters/iGameSetClearSelectionCallBackFunc.h>
#include <iGameSelectionFilters/iGameSetPointsSelect.h>

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
    parallelCoordinatesData->SetDefaultSelectionFunc(std::to_string(__LINE__), mesh->GetSelection());
    variableCorrelationData->SetDefaultSelectionFunc(std::to_string(__LINE__), mesh->GetSelection());
    variableDensityData->SetDefaultSelectionFunc(std::to_string(__LINE__), mesh->GetSelection());
    plotLineData->SetDefaultSelectionFunc(std::to_string(__LINE__), mesh->GetSelection());
}

static void OutPutKeyMsgs(iGame::ParallelCoordinatesData::Pointer parallelCoordinatesData,
                          iGame::VariableCorrelationData::Pointer variableCorrelationData,
                          iGame::VariableDensityData::Pointer variableDensityData,
                          iGame::PlotLineData::Pointer plotLineData) {
    {
        auto& theData = parallelCoordinatesData;
        auto& variableNames = theData->GetVariableName();
        std::cout << "ParallelCoordinatesData(Choosed Objs Data):\n";
        for (auto& variableName: variableNames) { std::cout << variableName << ' '; }
        std::cout << std::endl;
        for (auto& objId: theData->GetChoosedObjectIds()) {
            std::cout << "ObjId: " << objId << std::endl << "ObjData: ";
            for (int v = 0; v < theData->GetVariableNum(); v++) {
                auto value = theData->GetObjectData(objId, v);
                std::cout << value << ' ';
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
    {
        auto& theData = variableCorrelationData;
        auto& variableNames = theData->GetVariableName();
        std::cout << "VariableCorrelationData(Choosed Objs Correlation):\n";
        for (auto& variableName: variableNames) { std::cout << variableName << ' '; }
        std::cout << std::endl;
        auto& variableCors = theData->GetChoosedVariableCorrelation();
        for (auto& variableCor: variableCors) {
            for (auto& cor: variableCor) { std::cout << cor << '\t'; }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
    {
        auto& theData = variableDensityData;
        auto& variableNames = theData->GetVariableName();
        auto& densitys = theData->GetChoosedDensity();
        std::cout << "VariableDensityData(Choosed Objs Density):\n";
        for (int vI = 0; vI < theData->GetVariableNum(); vI++) {
            std::cout << variableNames[vI] << ":" << std::endl;
            for (auto& density: densitys[vI]) { std::cout << density << ' '; }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
    {
        auto& theData = plotLineData;
        auto& choosedObjIds = theData->GetChoosedObjectIds();
        auto& objIndexPairs = theData->GetObjIndexs();
        std::cout << "PlotLineData(Choosed Objs Msg):\n";
        for (auto& objIndexPair: objIndexPairs) {
            auto& objId = objIndexPair.first;
            auto& objIndex = objIndexPair.second;
            if (choosedObjIds.count(objId) == 0) continue;
            std::cout << "ObjId: " << objId << std::endl;
            std::cout << "ObjDistance: " << theData->GetObjDistance()[objIndex] << std::endl;
            std::cout << "ObjData:" << std::endl;
            for (int v = 0; v < theData->GetVariableNum(); v++) {
                auto value = theData->GetObjectData(objId, v);
                std::cout << value << ' ';
            }
        }
    }
    std::cout << std::endl;
}

static void SelectPoints(iGame::UnstructuredMesh::Pointer mesh, const std::vector<int>& ids) {
    auto filter = iGame::iGameSetPointsSelect::New(iGame::Selection::Event::Operate::Add, ids);
    filter->SetInput(mesh);
    filter->Execute();
}

static void ClearSelectPoints(iGame::UnstructuredMesh::Pointer mesh) { mesh->GetSelection()->ClearSelections(); }

int main() {
    /*Read data*/
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }
    /*Create data*/
    std::cout << "Create data\n";
    iGame::ParallelCoordinatesData::Pointer parallelCoordinatesData = GetParallelCoordinatesData(mesh);
    iGame::VariableCorrelationData::Pointer variableCorrelationData = GetVariableCorrelationData(mesh);
    iGame::VariableDensityData::Pointer variableDensityData = GetVariableDensityData(mesh);
    iGame::PlotLineData::Pointer plotLineData =
            GetPlotLineData(mesh, iGame::Point(-1.0f, -1.0f, -1.0f), iGame::Point(1.0f, 1.0f, 1.0f));
    /*Set Selection Funcs*/
    std::cout << "Set Selection Funcs\n";
    SetSelectionCallBackFunc(mesh, parallelCoordinatesData, variableCorrelationData, variableDensityData, plotLineData);
    /*Choose and Print Data*/
    std::cout << "Choose and Print Data\n";
    {
        std::cout << "\n\n**************** parallelCoordinatesData Choose ****************\n\n";
        std::map<int, std::pair<double, double>> variableMinMaxValues{{0, std::pair<double, double>(0.95, 1.0)}};
        auto pointIds = parallelCoordinatesData->FiltInRangeIds(variableMinMaxValues);
        SelectPoints(mesh, pointIds);
        OutPutKeyMsgs(parallelCoordinatesData, variableCorrelationData, variableDensityData, plotLineData);
        ClearSelectPoints(mesh);
    }
    {
        std::cout << "\n\n**************** variableCorrelationData Choose ****************\n\n";
        auto pointIds = variableCorrelationData->FiltInRangeIds(0, 1, 0.54, 0.62, 0.31, 0.55);
        SelectPoints(mesh, pointIds);
        OutPutKeyMsgs(parallelCoordinatesData, variableCorrelationData, variableDensityData, plotLineData);
        ClearSelectPoints(mesh);
    }
    {
        std::cout << "\n\n**************** variableDensityData Choose ****************\n\n";
        auto pointIds = variableDensityData->FiltInRangeIds(0, 0.504, 0.505);
        SelectPoints(mesh, pointIds);
        OutPutKeyMsgs(parallelCoordinatesData, variableCorrelationData, variableDensityData, plotLineData);
        ClearSelectPoints(mesh);
    }
    {
        std::cout << "\n\n**************** plotLineData Choose ****************\n\n";
        auto pointIds = plotLineData->FiltInRangeIds(0, 2.0, 0.004, 0.01, {true, false, false, false});
        SelectPoints(mesh, pointIds);
        OutPutKeyMsgs(parallelCoordinatesData, variableCorrelationData, variableDensityData, plotLineData);
        ClearSelectPoints(mesh);
    }
    return 0;
}