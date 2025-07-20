#pragma once
#include <iGameAttributeSet.h>
#include <iGameSelection.h>
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include <iGameScalarsToColors.h>
IGAME_NAMESPACE_BEGIN
class CtxPresObjData_Main {
public:
    CtxPresObjData_Main() = default;

    void SetVariableNum(int variableNum);
    int GetVariableNum() const;

    void SetVariableName(const std::vector<std::string>& variableName);
    const std::vector<std::string>& GetVariableName();

    void SetObjectDatas(const std::vector<std::vector<double>>& objectDatas);
    const std::vector<std::vector<double>>& GetObjectDatas();

    void SetChoosedObjectDatas(const std::map<int, std::vector<double>>& choosedObjectDatas);
    void AddChoosedObjectData(int objId, const std::vector<double>& objData);
    void RemoveChoosedObjectData(int objId);
    void ClearChoosedObjectData();
    const std::map<int, std::vector<double>>& GetChoosedObjectData();

    void SetMaxValueInVariables(const std::vector<double>& maxValueInVariables);
    const std::vector<double>& GetMaxValueInVariables();

    void SetMinValueInVariables(const std::vector<double>& minValueInVariables);
    const std::vector<double>& GetMinValueInVariables();

    void SetDataTypeName(const std::string& name);
    const std::string& GetDataTypeName();

    void SetDataType(IGenum dataType);
    IGenum GetDataType() const;

protected:
    int m_VariableNum{};
    std::vector<std::string> m_VariableName;

    std::vector<std::vector<double>> m_ObjectDatas;
    std::map<int, std::vector<double>> m_ChoosedObjectDatas;

    std::vector<double> m_MaxValueInVariables;
    std::vector<double> m_MinValueInVariables;
    std::string m_DataTypeName; //Point data. Face data
    IGenum m_DataType{};

public:
    /* static funcs */
    static std::vector<std::string> GenerateVariableNames(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                          IGenum dataType);
    static std::vector<double> GenerateObjectData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                                                  int objId);
    static std::vector<std::vector<double>> GenerateObjectDatas(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                                IGenum dataTypeint, int objNum, int maxObjNum);
    static std::map<int, std::vector<double>> GenerateChoosedObjectDatas(
            const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
            ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);
    static std::pair<std::vector<double>, std::vector<double>>
    GenerateMinMaxData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);
    static std::string GenerateDataTypeName(IGenum dataType);
};

class CtxPresObjData_Draw {
public:

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

    void SetChoosedAlpha(int alpha);
    int GetChoosedAlpha() const;

    void SetUnChoosedAlpha(int alpha);
    int GetUnChoosedAlpha() const;

    void SetChoosedLight(int light);
    int GetChoosedLight() const;

    void SetUnChoosedLight(int light);
    int GetUnChoosedLight() const;

protected:
    std::vector<std::tuple<int, int, int>> m_ObjectColor; //RGB
    std::vector<std::vector<int>> m_ObjDrawSortInVariables;
    std::tuple<int, int, int> m_DefaultColor;

    std::map<int, std::tuple<int, int, int>> m_ChoosedObjectColor; //RGB
    std::vector<std::vector<int>> m_ChoosedObjDrawSortInVariables;
    std::tuple<int, int, int> m_ChoosedDefaultColor;

    int m_ChoosedAlpha{255};
    int m_UnChoosedAlpha{140};
    int m_ChoosedLight{255};
    int m_UnChoosedLight{140};

public:
    /* static funcs */
    static std::vector<std::vector<int>> GenerateObjectDrawSorts(int variableNum,
                                                                 const std::vector<std::vector<double>>& objcetValues);
    static std::vector<std::vector<int>>
    GenerateObjectDrawSorts(int variableNum, const std::map<int, std::vector<double>>& objcetValues);
    static std::tuple<int, int, int> GenerateDefaultColor(int brightNess);
    static std::vector<std::tuple<int, int, int>>
    GenerateObjectColors(int variableIndex, const std::vector<std::vector<double>>& objDatas,
                         const std::vector<double>& maxValues, const std::vector<double>& minValues, int brightness,
                         ScalarsToColors::Pointer colorMap);
    static std::map<int, std::tuple<int, int, int>>
    GenerateObjectColors(int variableIndex, const std::map<int, std::vector<double>>& objDatas,
                         const std::vector<double>& maxValues, const std::vector<double>& minValues, int brightness,
                         ScalarsToColors::Pointer colorMap);
};


IGAME_NAMESPACE_END