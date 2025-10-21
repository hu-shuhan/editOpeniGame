#pragma once
#include <iGameAttributeSet.h>
#include <iGameSelection.h>
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include <set>
#include <iGameScalarsToColors.h>
IGAME_NAMESPACE_BEGIN
class CtxPresObjData_Main {
public:
    CtxPresObjData_Main() = default;

    void SetAttributes(ElementArray<AttributeSet::Attribute>::Pointer attrs);

    void SetObjectNum(int objNum);
    int GetObjectNum() const;

    double GetObjectData(int objId, int variableIndex);

    void SetVariableNum(int variableNum);
    int GetVariableNum() const;

    void SetVariableName(const std::vector<std::string>& variableName);
    const std::vector<std::string>& GetVariableName();

    void SetVariableIndex(const std::vector<std::pair<int, int>>& variableIndex);
    const std::vector<std::pair<int, int>>& GetVariableIndex();

    void SetKeyObjectIds(const std::vector<int>& keyObjIds);
    const std::vector<int>& GetKeyObjectIds() const;

    void SetKeyObjectIdToIndexMap(const std::map<int, int>& keyObjIdToIndex);
    const std::map<int, int>& GetKeyObjectIdToIndexMap() const;

    void SetChoosedObjectIds(const std::set<int>& choosedObjIds);
    void AddChoosedObjectId(int objId);
    void RemoveChoosedObjectId(int objId);
    void ClearChoosedObjectIds();
    const std::set<int>& GetChoosedObjectIds();

    void SetMaxValueInVariables(const std::vector<double>& maxValueInVariables);
    const std::vector<double>& GetMaxValueInVariables();

    void SetMinValueInVariables(const std::vector<double>& minValueInVariables);
    const std::vector<double>& GetMinValueInVariables();

    void SetDataTypeName(const std::string& name);
    const std::string& GetDataTypeName();

    void SetDataType(IGenum dataType);
    IGenum GetDataType() const;

protected:
    ElementArray<AttributeSet::Attribute>::Pointer m_Attrs;
    int m_ObjNum{};

    int m_VariableNum{};
    std::vector<std::string> m_VariableName;

public:
    std::vector<std::pair<int, int>> m_VariableIndex;
    std::vector<int> m_KeyObjIds;
    std::map<int, int> m_KeyObjIdToIndexs;
    std::set<int> m_ChoosedObjIds;

    std::vector<double> m_MaxValueInVariables;
    std::vector<double> m_MinValueInVariables;
    std::string m_DataTypeName; //Point data. Face data
    IGenum m_DataType{};

public:
    /* static funcs */
    static std::vector<std::string> GenerateVariableNames(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                          IGenum dataType);
    static std::vector<std::pair<int, int>> GenerateVariableIndex(ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                                  IGenum dataType);
    static std::vector<int> GenerateKeyObjectIds(int objNum, int maxObjNum);
    static std::set<int>
    GenerateChoosedObjectIds(const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
                             IGenum dataType);
    static std::pair<std::vector<double>, std::vector<double>>
    GenerateMinMaxData(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);
    static std::string GenerateDataTypeName(IGenum dataType);
    static std::map<int, int> GenerateKeyObjectIdToIndexs(const std::vector<int>& objectIds);

protected:
    static bool LegalAttrs(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);
    static int GetLegalAttrsObjNum(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);
};

class CtxPresObjData_LightAlpha {
public:
    void SetChoosedAlpha(int alpha);
    int GetChoosedAlpha() const;

    void SetUnChoosedAlpha(int alpha);
    int GetUnChoosedAlpha() const;

    void SetChoosedLight(int light);
    int GetChoosedLight() const;

    void SetUnChoosedLight(int light);
    int GetUnChoosedLight() const;

protected:
    int m_ChoosedAlpha{255};
    int m_UnChoosedAlpha{140};
    int m_ChoosedLight{255};
    int m_UnChoosedLight{140};

public:
    /* static funcs */
    static std::tuple<int, int, int> ChangeSaturation(const std::tuple<int, int, int>& rgb, int saturation);
};

class CtxPresObjData_Draw : public CtxPresObjData_LightAlpha {
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

protected:
    std::vector<std::tuple<int, int, int>> m_ObjectColor; //RGB
    std::vector<std::vector<int>> m_ObjDrawSortInVariables;
    std::tuple<int, int, int> m_DefaultColor;

    std::map<int, std::tuple<int, int, int>> m_ChoosedObjectColor; //RGB
    std::vector<std::vector<int>> m_ChoosedObjDrawSortInVariables;
    std::tuple<int, int, int> m_ChoosedDefaultColor;

public:
    /* static funcs */
    static std::vector<std::vector<int>> GenerateObjectDrawSorts(int variableNum, const std::vector<int>& objIds,
                                                                 CtxPresObjData_Main* theData);
    static std::vector<std::vector<int>> GenerateObjectDrawSorts(int variableNum, const std::set<int>& objIds,
                                                                 CtxPresObjData_Main* theData);
    static std::vector<std::vector<int>> GenerateDefaultObjectDrawSorts(int variableNum);
    static std::tuple<int, int, int> GenerateDefaultColor(int brightNess);
    static std::vector<std::tuple<int, int, int>>
    GenerateObjectColors(int variableIndex, const std::vector<int>& objIds, CtxPresObjData_Main* theData,
                         const std::vector<double>& maxValues, const std::vector<double>& minValues, int brightness,
                         ScalarsToColors::Pointer colorMap);
    static std::map<int, std::tuple<int, int, int>>
    GenerateObjectColors(int variableIndex, const std::set<int>& objIds, CtxPresObjData_Main* theData,
                         const std::vector<double>& maxValues, const std::vector<double>& minValues, int brightness,
                         ScalarsToColors::Pointer colorMap);
};


IGAME_NAMESPACE_END