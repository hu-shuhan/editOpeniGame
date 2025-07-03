#include "iGameParallelCoordinatesData.h"
#include <sstream>
using namespace std;
IGAME_NAMESPACE_BEGIN

void ParallelCoordinatesData::UseDefaultVariableName() {
    m_VariableName.clear();
    for (int i = 0; i < m_VariableNum; i++) {
        stringstream ss;
        ss << i;
        m_VariableName.push_back(ss.str());
    }
}

bool ParallelCoordinatesData::SetVariableName(const std::vector<std::string>& variableName) {
    if (variableName.size() != m_VariableNum) {
        UseDefaultVariableName();
        return false;
    }
    m_VariableName = variableName;
    return true;
}

const std::vector<double>& ParallelCoordinatesData::GetMaxValueInVariables() { return m_MaxValueInVariables; }

void ParallelCoordinatesData::SetMinValueInVariables(const std::vector<double>& minValueInVariables) {
    m_MinValueInVariables = minValueInVariables;
}

const std::vector<double>& ParallelCoordinatesData::GetMinValueInVariables() { return m_MinValueInVariables; }

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

void ParallelCoordinatesData::SetDataTypeName(const std::string& name) { m_DataTypeName = name; }

const std::string& ParallelCoordinatesData::GetDataTypeName() { return m_DataTypeName; }

void ParallelCoordinatesData::SetDataType(IGenum dataType) { m_DataType = dataType; }

IGenum ParallelCoordinatesData::GetDataType() { return m_DataType; }

void ParallelCoordinatesData::SetChoosedAlpha(int alpha) { m_ChoosedAlpha = alpha; }

int ParallelCoordinatesData::GetChoosedAlpha() const { return m_ChoosedAlpha; }

void ParallelCoordinatesData::SetUnChoosedAlpha(int alpha) { m_UnChoosedAlpha = alpha; }

int ParallelCoordinatesData::GetUnChoosedAlpha() { return m_UnChoosedAlpha; }

int ParallelCoordinatesData::GetVariableNum() const { return m_VariableNum; }

const std::vector<std::string>& ParallelCoordinatesData::GetVariableName() { return m_VariableName; }

void ParallelCoordinatesData::SetObjectData(const std::vector<std::vector<double>>& objData) {
    m_ObjectDatas = objData;
}

const std::vector<std::vector<double>>& ParallelCoordinatesData::GetObjectDatas() { return m_ObjectDatas; }

void ParallelCoordinatesData::SetMaxValueInVariables(const std::vector<double>& maxValueInVariables) {
    m_MaxValueInVariables = maxValueInVariables;
}

ParallelCoordinatesData::ParallelCoordinatesData(int variableNum) {
    m_VariableNum = variableNum;
}

IGAME_NAMESPACE_END