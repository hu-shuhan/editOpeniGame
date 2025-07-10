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

IGenum ParallelCoordinatesData::GetDataType() const { return m_DataType; }

void ParallelCoordinatesData::SetChoosedAlpha(int alpha) { m_ChoosedAlpha = alpha; }

int ParallelCoordinatesData::GetChoosedAlpha() const { return m_ChoosedAlpha; }

void ParallelCoordinatesData::SetUnChoosedAlpha(int alpha) { m_UnChoosedAlpha = alpha; }

int ParallelCoordinatesData::GetUnChoosedAlpha() const { return m_UnChoosedAlpha; }

void ParallelCoordinatesData::SetChoosedLight(int light) { m_ChoosedLight = light; }

int ParallelCoordinatesData::GetChoosedLight() const { return m_ChoosedLight; }

void ParallelCoordinatesData::SetUnChoosedLight(int light) { m_UnChoosedLight = light; }

int ParallelCoordinatesData::GetUnChoosedLight() const { return m_UnChoosedLight; }

int ParallelCoordinatesData::GetVariableNum() const { return m_VariableNum; }

void ParallelCoordinatesData::SetVariableSort(const std::vector<int>& variableSort) { m_VariableSort = variableSort; }

const std::vector<int>& ParallelCoordinatesData::GetVariableSort() { return m_VariableSort; }

const std::vector<std::string>& ParallelCoordinatesData::GetVariableName() { return m_VariableName; }

void ParallelCoordinatesData::SetObjectData(const std::vector<std::vector<double>>& objData) {
    m_ObjectDatas = objData;
}

const std::vector<std::vector<double>>& ParallelCoordinatesData::GetObjectDatas() { return m_ObjectDatas; }

void ParallelCoordinatesData::SetObjectChoosedColor(const std::vector<std::tuple<int, int, int>>& objColor) {
    m_ObjChoosedColor = objColor;
}

const std::vector<std::tuple<int, int, int>>& ParallelCoordinatesData::GetObjecChoosedColor() {
    return m_ObjChoosedColor;
}

void ParallelCoordinatesData::SetObjectUnChoosedColor(const std::vector<std::tuple<int, int, int>>& objColor) {
    m_ObjUnChoosedColor = objColor;
}

const std::vector<std::tuple<int, int, int>>& ParallelCoordinatesData::GetObjectUnChoosedColor() {
    return m_ObjUnChoosedColor;
}

void ParallelCoordinatesData::SetDefaultChoosedColor(const std::tuple<int, int, int>& color) {
    m_DefaultChoosedColor = color;
}

const std::tuple<int, int, int>& ParallelCoordinatesData::GetDefaultChoosedColor() { return m_DefaultChoosedColor; }

void ParallelCoordinatesData::SetDefaultUnChoosedColor(const std::tuple<int, int, int>& color) {
    m_DefaultUnChoosedColor = color;
}

const std::tuple<int, int, int>& ParallelCoordinatesData::GetDefaultUnChoosedColor() { return m_DefaultUnChoosedColor; }

const std::tuple<int, int, int>& ParallelCoordinatesData::GetObjColor(bool choosed, int objId) {
    if (choosed) {
        if (objId < 0 || m_ObjChoosedColor.size() <= objId) return m_DefaultChoosedColor;
        return m_ObjChoosedColor[objId];
    } else {
        if (objId < 0 || m_ObjUnChoosedColor.size() <= objId) return m_DefaultUnChoosedColor;
        return m_ObjUnChoosedColor[objId];
    }
}

void ParallelCoordinatesData::SetObjectDrawSorts(const std::vector<std::vector<int>>& drawSorts) {
    m_ObjDrawSortInVariables = drawSorts;
}

const std::vector<std::vector<int>>& ParallelCoordinatesData::GetObjectDrawSorts() { return m_ObjDrawSortInVariables; }

void ParallelCoordinatesData::SetMaxValueInVariables(const std::vector<double>& maxValueInVariables) {
    m_MaxValueInVariables = maxValueInVariables;
}

ParallelCoordinatesData::ParallelCoordinatesData(int variableNum) {
    m_VariableNum = variableNum;
}

IGAME_NAMESPACE_END