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

    void UseDefaultVariableName();
    bool SetVariableName(const std::vector<std::string>& variableName);
    const std::vector<std::string>& GetVariableName();

    void SetObjectData(const std::vector<std::vector<double>>& objData);
    const std::vector<std::vector<double>>& GetObjectDatas();

    void SetObjectChoosedColor(const std::vector<std::tuple<int, int, int>>& objColor);
    const std::vector<std::tuple<int, int, int>>& GetObjecChoosedColor();

    void SetObjectUnChoosedColor(const std::vector<std::tuple<int, int, int>>& objColor);
    const std::vector<std::tuple<int, int, int>>& GetObjectUnChoosedColor();

    void SetDefaultChoosedColor(const std::tuple<int, int, int>& color);
    const std::tuple<int, int, int>& GetDefaultChoosedColor();

    void SetDefaultUnChoosedColor(const std::tuple<int, int, int>& color);
    const std::tuple<int, int, int>& GetDefaultUnChoosedColor();

    const std::tuple<int, int, int>& GetObjColor(bool choosed, int objId);

    void SetObjectDrawSorts(const std::vector<std::vector<int>>& drawSorts);
    const std::vector<std::vector<int>>& GetObjectDrawSorts();

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
    std::vector<std::vector<double>> m_ObjectDatas;
    std::vector<std::tuple<int, int, int>> m_ObjChoosedColor;//RGB
    std::vector<std::tuple<int, int, int>> m_ObjUnChoosedColor; //RGB
    std::vector<std::vector<int>> m_ObjDrawSortInVariables;
    std::tuple<int, int, int> m_DefaultChoosedColor;
    std::tuple<int, int, int> m_DefaultUnChoosedColor;
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