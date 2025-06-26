#include "iGameParallelCoordinates.h"
#include <string>
#include <vector>
#include <sstream>
using namespace std;
IGAME_NAMESPACE_BEGIN
bool ParallelCoordinates::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh == nullptr) return false;
    m_Mesh->RequestEditStatus();
    auto parallelCoordinatesPointData = CreateParallelCoordinatesPointData();
    auto parallelCorrdinatesCellData = CreateParallelCoordinatesCellData();
    SetOutput(0, parallelCoordinatesPointData);
    SetOutput(1, parallelCorrdinatesCellData);
    return true;
}

ParallelCoordinatesData::Pointer ParallelCoordinates::CreateParallelCoordinatesPointData() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    //Get the number of variables
    int variableNum{};
    //Get the name of variables
    vector<string> variableNames;
    for (int i = 0; i < attrs->Size(); i++) {
        auto& attr = attrs->GetElement(i);
        if (attr.attachmentType != IG_POINT) continue;
        variableNum += attr.pointer->GetDimension();
        if (attr.pointer->GetDimension() == 1) {
            variableNames.push_back(attr.pointer->GetName());
            continue;
        }
        for (int j = 1; j <= attr.pointer->GetDimension(); j++) {
            stringstream ss;
            ss << attr.pointer->GetName() << j;
            variableNames.push_back(ss.str());
        }
    }
    //Set Data
    auto parallelCoordinatesPointData = ParallelCoordinatesData::New(variableNum);
    parallelCoordinatesPointData->SetVariableName(variableNames);
    for (int pointIndex = 0; pointIndex < m_Mesh->GetNumberOfPoints(); pointIndex++) {
        std::vector<double> obj;
        for (int i = 0; i < attrs->Size(); i++) {
            auto& attr = attrs->GetElement(i);
            if (attr.attachmentType != IG_POINT) continue;
            for (int j = 0; j < attr.pointer->GetDimension(); j++) {
                obj.push_back(attr.pointer->GetElementValue(pointIndex, j));
            }
        }
        parallelCoordinatesPointData->AddObject(obj, (bool) (false));
    }

    return parallelCoordinatesPointData;
}

ParallelCoordinatesData::Pointer ParallelCoordinates::CreateParallelCoordinatesCellData() {
    auto attrs = m_Mesh->GetAttributeSet()->GetAllAttributes();
    //Get the number of variables
    int variableNum{};
    //Get the name of variables
    vector<string> variableNames;
    for (int i = 0; i < attrs->Size(); i++) {
        auto& attr = attrs->GetElement(i);
        if (attr.attachmentType != IG_CELL) continue;
        variableNum += attr.pointer->GetDimension();
        if (attr.pointer->GetDimension() == 1) {
            variableNames.push_back(attr.pointer->GetName());
            continue;
        }
        for (int j = 1; j <= attr.pointer->GetDimension(); j++) {
            stringstream ss;
            ss << attr.pointer->GetName() << j;
            variableNames.push_back(ss.str());
        }
    }
    //Set Data
    auto parallelCoordinatesCellData = ParallelCoordinatesData::New(variableNum);
    parallelCoordinatesCellData->SetVariableName(variableNames);
    for (int cellIndex = 0; cellIndex < m_Mesh->GetNumberOfCells(); cellIndex++) {
        std::vector<double> obj;
        for (int i = 0; i < attrs->Size(); i++) {
            auto& attr = attrs->GetElement(i);
            if (attr.attachmentType != IG_CELL) continue;
            for (int j = 0; j < attr.pointer->GetDimension(); j++) {
                obj.push_back(attr.pointer->GetElementValue(cellIndex, j));
            }
        }
        parallelCoordinatesCellData->AddObject(obj);
    }

    return parallelCoordinatesCellData;
}

IGAME_NAMESPACE_END