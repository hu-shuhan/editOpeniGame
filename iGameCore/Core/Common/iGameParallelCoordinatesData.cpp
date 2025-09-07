#include "iGameParallelCoordinatesData.h"
#include <iGameThreadPool.h>
#include <sstream>
using namespace std;
IGAME_NAMESPACE_BEGIN

void ParallelCoordinatesData::SetFilterMaxValue(const std::vector<double>& filterMaxValue) {
    m_FilterMaxValue = filterMaxValue;
}

const std::vector<double>& ParallelCoordinatesData::GetFilterMaxValue() { return m_FilterMaxValue; }

std::vector<double>& ParallelCoordinatesData::FilterMaxValue() { return m_FilterMaxValue; }

void ParallelCoordinatesData::SetFilterMinValue(const std::vector<double>& filterMinValue) {
    m_FilterMinValue = filterMinValue;
}

const std::vector<double>& ParallelCoordinatesData::GetFilterMinValue() { return m_FilterMinValue; }

std::vector<double>& ParallelCoordinatesData::FilterMinValue() { return m_FilterMinValue; }

std::vector<int> ParallelCoordinatesData::GenerateDefaultVariableSort(int variableNum) {
    std::vector<int> re(variableNum);
    for (int i = 0; i < variableNum; i++) re[i] = i;
    return re;
}

void ParallelCoordinatesData::SetVariableSort(const std::vector<int>& variableSort) { m_VariableSort = variableSort; }

const std::vector<int>& ParallelCoordinatesData::GetVariableSort() { return m_VariableSort; }

ParallelCoordinatesData::Pointer
ParallelCoordinatesData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                             const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
                             int objNum) {
    auto variableNames = ParallelCoordinatesData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return ParallelCoordinatesData::Pointer();
    auto Data = ParallelCoordinatesData::New();
    Data->SetVariableNum(variableNum);
    Data->SetVariableSort(ParallelCoordinatesData::GenerateDefaultVariableSort(variableNum));
    Data->SetVariableName(variableNames);
    auto variableIndex = ParallelCoordinatesData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto objDatas = ParallelCoordinatesData::GenerateObjectDatas(attrs, dataType, objNum, 10000);
    Data->SetObjectDatas(objDatas);
    Data->SetObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, objDatas));
    Data->SetDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetUnChoosedLight()));
    auto choosedObjDatas = ParallelCoordinatesData::GenerateChoosedObjectDatas(selectedItems, attrs, dataType);
    Data->SetChoosedObjectDatas(choosedObjDatas);
    Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, choosedObjDatas));
    Data->SetChoosedDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetChoosedLight()));
    auto [minValue, maxValue] = ParallelCoordinatesData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    Data->SetFilterMinValue(minValue);
    Data->SetFilterMaxValue(maxValue);
    Data->SetDataType(dataType);
    Data->SetDataTypeName(ParallelCoordinatesData::GenerateDataTypeName(dataType));
    return Data;
}

ParallelCoordinatesData::Pointer ParallelCoordinatesData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                              IGenum dataType) {
    auto variableNames = ParallelCoordinatesData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return ParallelCoordinatesData::Pointer();
    int objNum = ParallelCoordinatesData::GetLegalAttrsObjNum(attrs, dataType);
    auto Data = ParallelCoordinatesData::New();
    Data->SetVariableNum(variableNum);
    Data->SetVariableSort(ParallelCoordinatesData::GenerateDefaultVariableSort(variableNum));
    Data->SetVariableName(variableNames);
    auto variableIndex = ParallelCoordinatesData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto objDatas = ParallelCoordinatesData::GenerateObjectDatas(attrs, dataType, objNum, 10000);
    Data->SetObjectDatas(objDatas);
    Data->SetObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, objDatas));
    Data->SetDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetUnChoosedLight()));
    //auto choosedObjDatas = ParallelCoordinatesData::GenerateChoosedObjectDatas(selectedItems, attrs, dataType);
    //Data->SetChoosedObjectDatas(choosedObjDatas);
    //Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateObjectDrawSorts(variableNum, choosedObjDatas));
    Data->SetChoosedObjectDrawSorts(ParallelCoordinatesData::GenerateDefaultObjectDrawSorts(variableNum));
    Data->SetChoosedDefaultColor(ParallelCoordinatesData::GenerateDefaultColor(Data->GetChoosedLight()));
    auto [minValue, maxValue] = ParallelCoordinatesData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    Data->SetFilterMinValue(minValue);
    Data->SetFilterMaxValue(maxValue);
    Data->SetDataType(dataType);
    Data->SetDataTypeName(ParallelCoordinatesData::GenerateDataTypeName(dataType));
    return Data;
}

std::vector<igIndex>
ParallelCoordinatesData::FiltInRangeIds(const std::map<int, std::pair<double, double>>& variableMinMaxValues,
                                        ElementArray<AttributeSet::Attribute>::Pointer attrs, int objNum) {
    std::vector<igIndex> ids;
    static mutex IDS_MUTEX;
    ThreadPool::parallelFor(0, objNum, [&](int st, int ed) {
        std::vector<igIndex> tempIds;
        for (int objId = st; objId < ed; objId++) {
            for (auto& variableMinMaxValue_: variableMinMaxValues) {
                auto& variableId = variableMinMaxValue_.first;
                auto& variableIndex = this->GetVariableIndex()[variableId];
                auto& minValue = variableMinMaxValue_.second.first;
                auto& maxValue = variableMinMaxValue_.second.second;
                auto value =
                        attrs->GetElement(variableIndex.first).pointer->GetElementValue(objId, variableIndex.second);
                if (value < minValue || maxValue < value) continue;
                if (value < this->GetFilterMinValue()[variableId] || this->GetFilterMaxValue()[variableId] < value)
                    continue;
                tempIds.push_back(objId);
                break;
            }
        }
        lock_guard lg(IDS_MUTEX);
        ids.insert(ids.end(), tempIds.begin(), tempIds.end());
    });
    return ids;
}

bool ParallelCoordinatesData::NotInFilterValueRange(const std::vector<double>& objData) {
    for (int i = 0; i < objData.size(); i++) {
        if (objData[i] < this->GetFilterMinValue()[i] || this->GetFilterMaxValue()[i] < objData[i]) return true;
    }
    return false;
}

IGAME_NAMESPACE_END