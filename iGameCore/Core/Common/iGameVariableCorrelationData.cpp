#include "iGameVariableCorrelationData.h"
#include <cmath>
#include <iGameThreadPool.h>
using namespace std;
IGAME_NAMESPACE_BEGIN

static std::vector<std::vector<double>> ComputeCorrelationMatrix(int variableNum,
                                                                 const std::vector<std::vector<double>>& variables) {
    int numObjects = variables.empty() ? 0 : variables[0].size();
    std::vector<std::vector<double>> result(variableNum, std::vector<double>(variableNum, 0.0));

    if (numObjects < 2) {
        for (int i = 0; i < variableNum; ++i) {
            for (int j = 0; j < variableNum; ++j) { result[i][j] = (i == j) ? 100.0 : 0.0; }
        }
        return result;
    }

    std::vector<double> means(variableNum, 0.0);
    std::vector<double> sumSquares(variableNum, 0.0);

    for (int i = 0; i < variableNum; ++i) {
        double sum = 0.0;
        for (double val: variables[i]) { sum += val; }
        means[i] = sum / numObjects;

        for (double val: variables[i]) {
            double diff = val - means[i];
            sumSquares[i] += diff * diff;
        }
    }

    const double EPS = 1e-10;
    for (int i = 0; i < variableNum; ++i) {
        for (int j = i; j < variableNum; ++j) {
            if (i == j) {
                result[i][j] = 100.0;
            } else {
                bool is_i_constant = (sumSquares[i] < EPS);
                bool is_j_constant = (sumSquares[j] < EPS);

                if (is_i_constant && is_j_constant) {
                    result[i][j] = 100.0;
                } else if (is_i_constant || is_j_constant) {
                    result[i][j] = 0.0;
                } else {
                    double covariance = 0.0;
                    for (int k = 0; k < numObjects; ++k) {
                        double diff_i = variables[i][k] - means[i];
                        double diff_j = variables[j][k] - means[j];
                        covariance += diff_i * diff_j;
                    }
                    double r = covariance / std::sqrt(sumSquares[i] * sumSquares[j]);
                    result[i][j] = 100.0 * r;
                }
                result[j][i] = result[i][j];
            }
        }
    }
    return result;
}

void VariableCorrelationData::SetVariableCorrelation(const std::vector<std::vector<double>>& variableCorrelation) {
    m_VariableCorr = variableCorrelation;
}

const std::vector<std::vector<double>>& VariableCorrelationData::GetVariableCorrelation() { return m_VariableCorr; }

void VariableCorrelationData::SetChoosedVariableCorrelation(
        const std::vector<std::vector<double>>& variableCorrelation) {
    m_ChoosedVariableCorr = variableCorrelation;
}

const std::vector<std::vector<double>>& VariableCorrelationData::GetChoosedVariableCorrelation() {
    return m_ChoosedVariableCorr;
}

std::vector<std::vector<double>>
VariableCorrelationData::CalculateVariableCorrelation(int variableNum,
                                                      const std::vector<std::vector<double>>& objDatas) {
    int numObjects = objDatas.size();
    std::vector<std::vector<double>> variables(variableNum, std::vector<double>(numObjects, 0.0));

    for (int objIdx = 0; objIdx < numObjects; ++objIdx) {
        //if (objDatas[objIdx].size() != static_cast<size_t>(variableNum)) {
        //    throw std::invalid_argument("Object data size does not match variableNum");
        //}
        for (int varIdx = 0; varIdx < variableNum; ++varIdx) { variables[varIdx][objIdx] = objDatas[objIdx][varIdx]; }
    }

    return ComputeCorrelationMatrix(variableNum, variables);
}

std::vector<std::vector<double>>
VariableCorrelationData::CalculateVariableCorrelation(int variableNum,
                                                      const std::map<int, std::vector<double>>& objDatas) {
    int numObjects = objDatas.size();
    std::vector<std::vector<double>> variables(variableNum, std::vector<double>(numObjects, 0.0));

    int objIdx = 0;
    for (const auto& kv: objDatas) {
        //if (kv.second.size() != static_cast<size_t>(variableNum)) {
        //    throw std::invalid_argument("Object data size does not match variableNum");
        //}
        for (int varIdx = 0; varIdx < variableNum; ++varIdx) { variables[varIdx][objIdx] = kv.second[varIdx]; }
        ++objIdx;
    }

    return ComputeCorrelationMatrix(variableNum, variables);
}

std::vector<std::vector<double>> VariableCorrelationData::CalculateDefaultVariableCorrelation(int variableNum) {
    return std::vector<std::vector<double>>(variableNum, std::vector<double>(variableNum, 0.0));
}

VariableCorrelationData::Pointer
VariableCorrelationData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                             const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
                             int objNum) {
    auto variableNames = VariableCorrelationData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return VariableCorrelationData::Pointer();
    auto Data = VariableCorrelationData::New();
    Data->SetVariableNum(variableNum);
    Data->SetVariableName(variableNames);
    auto variableIndex = VariableCorrelationData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto objDatas = VariableCorrelationData::GenerateObjectDatas(attrs, dataType, objNum, 50000);
    Data->SetObjectDatas(objDatas);
    Data->SetObjectDrawSorts(VariableCorrelationData::GenerateObjectDrawSorts(variableNum, objDatas));
    Data->SetDefaultColor(VariableCorrelationData::GenerateDefaultColor(Data->GetUnChoosedLight()));
    auto choosedObjDatas = VariableCorrelationData::GenerateChoosedObjectDatas(selectedItems, attrs, dataType);
    Data->SetChoosedObjectDatas(choosedObjDatas);
    Data->SetChoosedObjectDrawSorts(VariableCorrelationData::GenerateObjectDrawSorts(variableNum, choosedObjDatas));
    Data->SetChoosedDefaultColor(VariableCorrelationData::GenerateDefaultColor(Data->GetChoosedLight()));
    auto [minValue, maxValue] = VariableCorrelationData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    Data->SetDataType(dataType);
    Data->SetDataTypeName(VariableCorrelationData::GenerateDataTypeName(dataType));
    Data->SetVariableCorrelation(VariableCorrelationData::CalculateVariableCorrelation(variableNum, objDatas));
    Data->SetChoosedVariableCorrelation(
            VariableCorrelationData::CalculateVariableCorrelation(variableNum, choosedObjDatas));
    return Data;
}

VariableCorrelationData::Pointer
VariableCorrelationData::New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType) {
    auto variableNames = VariableCorrelationData::GenerateVariableNames(attrs, dataType);
    int variableNum = variableNames.size();
    if (variableNum == 0) return VariableCorrelationData::Pointer();
    int objNum = VariableCorrelationData::GetLegalAttrsObjNum(attrs, dataType);
    auto Data = VariableCorrelationData::New();
    Data->SetVariableNum(variableNum);
    Data->SetVariableName(variableNames);
    auto variableIndex = VariableCorrelationData::GenerateVariableIndex(attrs, dataType);
    Data->SetVariableIndex(variableIndex);
    auto objDatas = VariableCorrelationData::GenerateObjectDatas(attrs, dataType, objNum, 50000);
    Data->SetObjectDatas(objDatas);
    Data->SetObjectDrawSorts(VariableCorrelationData::GenerateObjectDrawSorts(variableNum, objDatas));
    Data->SetDefaultColor(VariableCorrelationData::GenerateDefaultColor(Data->GetUnChoosedLight()));
    Data->SetChoosedObjectDrawSorts(VariableCorrelationData::GenerateDefaultObjectDrawSorts(variableNum));
    Data->SetChoosedDefaultColor(VariableCorrelationData::GenerateDefaultColor(Data->GetChoosedLight()));
    auto [minValue, maxValue] = VariableCorrelationData::GenerateMinMaxData(attrs, dataType);
    Data->SetMinValueInVariables(minValue);
    Data->SetMaxValueInVariables(maxValue);
    Data->SetDataType(dataType);
    Data->SetDataTypeName(VariableCorrelationData::GenerateDataTypeName(dataType));
    Data->SetVariableCorrelation(VariableCorrelationData::CalculateVariableCorrelation(variableNum, objDatas));
    Data->SetChoosedVariableCorrelation(
            VariableCorrelationData::CalculateDefaultVariableCorrelation(variableNum));
    return Data;
}

std::vector<igIndex> VariableCorrelationData::FiltInRangeIds(int _mainVariableIndex, int _subVariableIndex,
                                                             double mainVariableMinValue, double mainVariableMaxValue,
                                                             double subVariableMinValue, double subVariableMaxValue,
                                                             ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                             int objNum) {
    std::vector<igIndex> ids;
    auto& mainVariableIndex = this->GetVariableIndex()[_mainVariableIndex];
    auto& subVariableIndex = this->GetVariableIndex()[_subVariableIndex];
    static mutex IDS_MUTEX;
    ThreadPool::parallelFor(0, objNum, [&](int st, int ed) {
        std::vector<igIndex> tempIds;
        for (int objId = st; objId < ed; objId++) {
            auto mainVariableValue = attrs->GetElement(mainVariableIndex.first)
                                             .pointer->GetElementValue(objId, mainVariableIndex.second);
            if (mainVariableValue < mainVariableMinValue || mainVariableMaxValue < mainVariableValue) continue;
            auto subVariableValue =
                    attrs->GetElement(subVariableIndex.first).pointer->GetElementValue(objId, subVariableIndex.second);
            if (subVariableValue < subVariableMinValue || subVariableMaxValue < subVariableValue) continue;
            tempIds.push_back(objId);
        }
        lock_guard lg(IDS_MUTEX);
        ids.insert(ids.end(), tempIds.begin(), tempIds.end());
    });
    return ids;
}

IGAME_NAMESPACE_END