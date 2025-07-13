#pragma once
#include <iGameDataObject.h>
#include <vector>
#include <utility>
#include <set>
#include <algorithm>
#include <map>
#include <string>
#include <tuple>

IGAME_NAMESPACE_BEGIN
class ParallelCoordinatesData : public DataObject {
public:
    I_OBJECT(ParallelCoordinatesData);
    static Pointer New(int variableNum) { return new ParallelCoordinatesData(variableNum); }

    int GetVariableNum() const;

    void SetVariableSort(const std::vector<int>& variableSort);
    const std::vector<int>& GetVariableSort();

    void UseDefaultVariableName();
    bool SetVariableName(const std::vector<std::string>& variableName);
    const std::vector<std::string>& GetVariableName();

    void SetObjectData(const std::vector<std::vector<double>>& objData);
    const std::vector<std::vector<double>>& GetObjectDatas();

    void SetChoosedObjectData(const std::map<int, std::vector<double>>& objData);
    void AddChoosedObjectData(int objId, const std::vector<double>& objData);
    void RemoveChoosedObjectData(int objId);
    void ClearChoosedObjectData();
    const std::map<int, std::vector<double>>& GetChoosedObjectData();

    void SetChoosedObjectColor(const std::map<int, std::tuple<int, int, int>>& objColor);
    const std::map<int, std::tuple<int, int, int>>& GetChoosedObjectColor();

    void SetObjectColor(const std::vector<std::tuple<int, int, int>>& objColor);
    const std::vector<std::tuple<int, int, int>>& GetObjectColor();

    void SetChoosedDefaultColor(const std::tuple<int, int, int>& color);
    const std::tuple<int, int, int>& GetChoosedDefaultColor();

    void SetDefaultColor(const std::tuple<int, int, int>& color);
    const std::tuple<int, int, int>& GetDefaultColor();

    const std::tuple<int, int, int>& GetObjectColor(bool choosed, int objId);

    void SetObjectDrawSorts(const std::vector<std::vector<int>>& drawSorts);
    const std::vector<std::vector<int>>& GetObjectDrawSorts();

    void SetChoosedObjectDrawSorts(const std::vector<std::vector<int>>& drawSorts);
    const std::vector<std::vector<int>>& GetChoosedObjDrawSorts();

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
    IGenum GetDataType() const;

    void SetChoosedAlpha(int alpha);
    int GetChoosedAlpha() const;

    void SetUnChoosedAlpha(int alpha);
    int GetUnChoosedAlpha() const;

    void SetChoosedLight(int light);
    int GetChoosedLight() const;

    void SetUnChoosedLight(int light);
    int GetUnChoosedLight() const;

protected:
    ParallelCoordinatesData() = delete;
    ParallelCoordinatesData(int variableNum);
    int m_VariableNum{};
    std::vector<std::string> m_VariableName;
    std::vector<int> m_VariableSort;

    std::vector<std::vector<double>> m_ObjectDatas;
    std::vector<std::tuple<int, int, int>> m_ObjectColor; //RGB
    std::vector<std::vector<int>> m_ObjDrawSortInVariables;
    std::tuple<int, int, int> m_DefaultColor;

    std::map<int, std::vector<double>> m_ChoosedObjectDatas;
    std::map<int, std::tuple<int, int, int>> m_ChoosedObjectColor; //RGB
    std::vector<std::vector<int>> m_ChoosedObjDrawSortInVariables;
    std::tuple<int, int, int> m_ChoosedDefaultColor;

    std::vector<double> m_MaxValueInVariables;
    std::vector<double> m_MinValueInVariables;
    std::vector<double> m_FilterMaxValue;
    std::vector<double> m_FilterMinValue;
    std::string m_DataTypeName; //Point data. Face data
    IGenum m_DataType{};
    int m_ChoosedAlpha{255};
    int m_UnChoosedAlpha{140};
    int m_ChoosedLight{255};
    int m_UnChoosedLight{140};
};

IGAME_NAMESPACE_END