#pragma once
#include <iGameDataObject.h>
#include <vector>
#include <utility>
#include <set>
#include <algorithm>
#include <map>
#include <string>

IGAME_NAMESPACE_BEGIN
class ParallelCoordinatesData : public DataObject {
public:
    I_OBJECT(ParallelCoordinatesData);
    static Pointer New(int variableNum) { return new ParallelCoordinatesData(variableNum); }
    void UseDefaultVariableName();
    bool SetVariableName(const std::vector<std::string>& variableName);
    bool AddObject(const std::vector<double>& object, bool choosed = true);
    std::vector<std::vector<double>> GetLinkStrings() const;
    std::vector<bool> GetLinkStringChooseCondition() const;
    std::vector<double> GetMaxValueInVariables();
    std::vector<double> GetMinValueInVariables();
    int GetVariableNum() const;
    std::vector<std::string> GetVariableName();

protected:
    ParallelCoordinatesData() = delete;
    ParallelCoordinatesData(int variableNum);
    int m_VariableNum{};
    std::vector<std::string> m_VariableName;
    std::vector<std::vector<double>> m_LinkStrings;
    std::vector<bool> m_ChoosedObjs;
    std::vector<double> m_MaxValueInVariables;
    std::vector<double> m_MinValueInVariables;
};

IGAME_NAMESPACE_END