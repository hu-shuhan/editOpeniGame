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

    int GetVariableNum() const;

    void UseDefaultVariableName();
    bool SetVariableName(const std::vector<std::string>& variableName);
    const std::vector<std::string>& GetVariableName();

    void SetObjectData(const std::vector<std::vector<double>>& objData);
    const std::vector<std::vector<double>>& GetObjectDatas();

    void SetMaxValueInVariables(const std::vector<double>& maxValueInVariables);
    const std::vector<double>& GetMaxValueInVariables();

    void SetMinValueInVariables(const std::vector<double>& minValueInVariables);
    const std::vector<double>& GetMinValueInVariables();

    void SetFilterMaxValue(const std::vector<double>& filterMaxValue);
    const std::vector<double>& GetFilterMaxValue();
    std::vector<double>& FilterMaxValue();

    void SetFilterMinValue(const std::vector<double>& filterMinValue);
    const std::vector<double>& GetFilterMinValue();
    std::vector<double>& FilterMinValue();

    void SetDataTypeName(const std::string& name);
    const std::string& GetDataTypeName();

    void SetDataType(IGenum dataType);
    IGenum GetDataType();

    void SetChoosedAlpha(int alpha);
    int GetChoosedAlpha() const;

    void SetUnChoosedAlpha(int alpha);
    int GetUnChoosedAlpha();

protected:
    ParallelCoordinatesData() = delete;
    ParallelCoordinatesData(int variableNum);
    int m_VariableNum{};
    std::vector<std::string> m_VariableName;
    std::vector<std::vector<double>> m_ObjectDatas;
    std::vector<double> m_MaxValueInVariables;
    std::vector<double> m_MinValueInVariables;
    std::vector<double> m_FilterMaxValue;
    std::vector<double> m_FilterMinValue;
    std::string m_DataTypeName; //Point data. Face data
    IGenum m_DataType{};
    int m_ChoosedAlpha{10};
    int m_UnChoosedAlpha{1};
};

IGAME_NAMESPACE_END