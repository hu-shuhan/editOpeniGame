#pragma once
#include <iGameDataObject.h>
#include <vector>
#include <utility>
#include <set>
#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include <iGameCtxPresObjData.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>

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

protected:
    std::vector<int> m_VariableSort;

    std::vector<double> m_FilterMaxValue;
    std::vector<double> m_FilterMinValue;

public:
    /* static funcs */
    static std::vector<int> GenerateDefaultVariableSort(int variableNum);

public:
    /* init func */
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                       const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
                       int objNum);
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
    void DefaultSelectionCallBackFunc(const std::vector<Selection::Event>& _events);
    void DefaultClearSelectionCallBackFunc();
};

IGAME_NAMESPACE_END