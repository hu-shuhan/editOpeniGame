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

bool ParallelCoordinatesData::AddObject(const std::vector<double>& object,bool choosed) {
    if (object.size() != m_VariableNum) return false;
    if (m_VariableNum == 0) return false;
    m_ChoosedObjs.push_back(choosed);
    m_LinkStrings.push_back(object);
    if (m_MaxValueInVariables.empty()) m_MaxValueInVariables = object;
    else {
        for (int i = 0; i < m_VariableNum; i++) { m_MaxValueInVariables[i] = max(m_MaxValueInVariables[i], object[i]); }
    }
    if (m_MinValueInVariables.empty()) m_MinValueInVariables = object;
    else {
        for (int i = 0; i < m_VariableNum; i++) { m_MinValueInVariables[i] = min(m_MinValueInVariables[i], object[i]); }
    }
    return true;
}

std::vector<std::vector<double>> ParallelCoordinatesData::GetLinkStrings() const {
    return m_LinkStrings; }

std::vector<bool> ParallelCoordinatesData::GetLinkStringChooseCondition() const { return m_ChoosedObjs; }

std::vector<double> ParallelCoordinatesData::GetMaxValueInVariables() { return m_MaxValueInVariables; }

std::vector<double> ParallelCoordinatesData::GetMinValueInVariables() { return m_MinValueInVariables; }

int ParallelCoordinatesData::GetVariableNum() const{ return m_VariableNum; }
std::vector<std::string> ParallelCoordinatesData::GetVariableName() {
    return m_VariableName;
};

ParallelCoordinatesData::ParallelCoordinatesData(int variableNum) {
    m_VariableNum = variableNum;
}

IGAME_NAMESPACE_END