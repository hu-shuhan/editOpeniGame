#pragma once
#include <algorithm>
#include <iGameCtxPresObjData.h>
#include <iGameDataObject.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN
class ParallelCoordinatesData : public DataObject, public CtxPresObjData_Main, public CtxPresObjData_Draw {
public:
    I_OBJECT(ParallelCoordinatesData);

    void SetVariableSort(const std::vector<int>& variableSort);
    const std::vector<int>& GetVariableSort();

    void SetFilterMaxValue(const std::vector<double>& filterMaxValue);
    const std::vector<double>& GetFilterMaxValue();
    std::vector<double>& FilterMaxValue();

    void SetFilterMinValue(const std::vector<double>& filterMinValue);
    const std::vector<double>& GetFilterMinValue();
    std::vector<double>& FilterMinValue();

    void SetVariableDiffValue(const std::vector<std::vector<double>>& variableDiffValue);
    const std::vector<std::vector<double>>& GetVariableDiffValue();

protected:
    std::vector<int> m_VariableSort;

    std::vector<double> m_FilterMaxValue;
    std::vector<double> m_FilterMinValue;

    std::vector<std::vector<double>> m_VariableDiffValue;

public:
    /* static funcs */
    static std::vector<int> GenerateDefaultVariableSort(int variableNum);
    static std::vector<std::vector<double>>
    GenerateVariableDiffValue(int variableNum, ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                              int objNum, int maxObjNum, const std::vector<double>& minValues,
                              const std::vector<double>& maxValues);
    //static std::vector<int> GenerateRareObjIdsInValue()
    /**
     * 根据差异值矩阵重新生成变量排序
     * @param variableSort 原有的变量排序，vector中的值是变量索引
     * @param diffValue 对称矩阵，diffValue[i][j]表示变量i和变量j之间的差异值
     * @return 新的变量排序，使得序列中相邻变量的差异值之和最小
     */
    static std::vector<int> GenerateVariableSortByDiffValue(const std::vector<int>& variableSort,
                                                            const std::vector<std::vector<double>>& diffValue);

public:
    /* init func */
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                       const std::set<igIndex>& selectedItems, int objNum);
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);

protected:
    ParallelCoordinatesData() = default;
    static Pointer New() { return new ParallelCoordinatesData(); }

public:
    /* choose func */
    std::vector<igIndex> FiltInRangeIds(const std::map<int, std::pair<double, double>>& variableMinMaxValues);

public:
    /* normal func */
    bool NotInFilterValueRange(int objId);

public:
    /* selection set */
    void SetDefaultSelectionFunc(const std::string& funcName, Selection* selection);

protected:
    void DefaultSelectionCallBackFunc(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope);
    void DefaultClearSelectionCallBackFunc();
};

IGAME_NAMESPACE_END