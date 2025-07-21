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
    static Pointer New() { return new VariableDensityData(); }
    VariableDensityData() = default;

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
    static std::vector<std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>>>
    GenerateDensityColor(int copyNum, int brightness, ScalarsToColors::Pointer colorMap);
    static std::set<int> GenerateChoosedObjectIndexs(
            const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
            IGenum dataType);

public:
    /* delete func */
    void SetObjectDatas(const std::vector<std::vector<double>>& objectDatas) = delete;
    const std::vector<std::vector<double>>& GetObjectDatas() = delete;

    void SetChoosedObjectDatas(const std::map<int, std::vector<double>>& choosedObjectDatas) = delete;
    void AddChoosedObjectData(int objId, const std::vector<double>& objData) = delete;
    void RemoveChoosedObjectData(int objId) = delete;
    void ClearChoosedObjectData() = delete;
    const std::map<int, std::vector<double>>& GetChoosedObjectData() = delete;
};
IGAME_NAMESPACE_END