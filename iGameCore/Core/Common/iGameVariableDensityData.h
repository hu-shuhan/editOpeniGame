#pragma once
#include <iGameDataObject.h>
#include <iGameCtxPresObjData.h>
#include <iGameScalarsToColors.h>
#include <vector>
#include <map>
#include <tuple>
#include <set>
IGAME_NAMESPACE_BEGIN
class VariableDensityData : public DataObject, public CtxPresObjData_Main, public CtxPresObjData_LightAlpha {
public:
    I_OBJECT(VariableDensityData);

    void SetCopyNum(int copyNum);
    int GetCopyNum() const;

    void SetChoosedObjectIndexs(const std::set<int>& objIds);
    void AddChoosedObjectIndex(int objId);
    void RemoveChoosedObjectIndex(int objId);
    void ClearChoosedObjectIndex();
    const std::set<int>& GetChoosedObjectIndexs();

    void SetDensity(const std::vector<std::vector<int>>& density);
    const std::vector<std::vector<int>>& GetDensity();

    void SetChoosedDensity(const std::vector<std::vector<int>>& density);
    const std::vector<std::vector<int>>& GetChoosedDensity();

    void
    SetDensityColor(const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor);
    const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& GetDensityColor();

    void SetChoosedDensityColor(
            const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& densityColor);
    const std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>& GetChoosedDensityColor();

protected:
    int m_CopyNum{};

    std::set<int> m_ChoosedObjIndexs;

    std::vector<std::vector<int>> m_Density; //[variableIndex][dataLevel]
    std::vector<std::vector<int>> m_ChoosedDensity; //[variableIndex][dataLevel]

    std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>> m_DensityColor; //st,ed
    std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>> m_ChoosedDensityColor;

public:
    /* static funcs */
    static int CalculateCopyIndexByValue(int copyNum, double value, double maxValue, double minValue);
    static std::vector<std::vector<int>> GenerateDensity(int variableNum, int copyNum,
                                                         const std::vector<double>& maxValueInVariables,
                                                         const std::vector<double>& minValueInVariables,
                                                         const std::vector<std::vector<double>>& objDatas);
    static std::vector<std::vector<int>> GenerateDensity(int variableNum, int copyNum,
                                                         const std::vector<double>& maxValueInVariables,
                                                         const std::vector<double>& minValueInVariables,
                                                         ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                         IGenum dataType, int objNum);
    static std::vector<std::vector<int>> GenerateDensity(int variableNum, int copyNum,
                                                         const std::vector<double>& maxValueInVariables,
                                                         const std::vector<double>& minValueInVariables,
                                                         const std::map<int, std::vector<double>>& objDatas);
    static std::vector<std::vector<int>> GenerateDensity(int variableNum, int copyNum,
                                                         const std::vector<double>& maxValueInVariables,
                                                         const std::vector<double>& minValueInVariables,
                                                         ElementArray<AttributeSet::Attribute>::Pointer attrs,
                                                         IGenum dataType, const std::set<int>& objIndexs);
    static std::vector<std::vector<int>> GenerateDefaultDensity(int variableNum, int copyNum);
    static std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>
    GenerateDensityColor(int copyNum, int brightness, ScalarsToColors::Pointer colorMap);
    static std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>
    GenerateDefaultDensityColor(int copyNum);
    static std::set<int> GenerateChoosedObjectIndexs(
            const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
            IGenum dataType);

public:
    /* delete func */
    void SetKeyObjectIds(const std::vector<int>& keyObjIds) = delete;
    const std::vector<int>& GetKeyObjectIds() = delete;

    void SetChoosedObjectIds(const std::set<int>& choosedObjIds) = delete;
    void AddChoosedObjectId(int objId) = delete;
    void RemoveChoosedObjectId(int objId) = delete;
    void ClearChoosedObjectIds() = delete;
    const std::set<int>& GetChoosedObjectIds() = delete;

public:
    /* init func */
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                       const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
                       int objNum, int boxNum, ScalarsToColors::Pointer colorMap);
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType, int boxNum);

protected:
    VariableDensityData() = default;
    static Pointer New() { return new VariableDensityData(); }

public:
    /* choose func */
    std::vector<igIndex> FiltInRangeIds(int variableIndex, double variableMinValue, double variableMaxValue);
};
IGAME_NAMESPACE_END