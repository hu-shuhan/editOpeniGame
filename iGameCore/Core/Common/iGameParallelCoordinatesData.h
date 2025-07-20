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

IGAME_NAMESPACE_BEGIN
class ParallelCoordinatesData : public DataObject, public CtxPresObjData_Main, public CtxPresObjData_Draw {
public:
    I_OBJECT(ParallelCoordinatesData);
    static Pointer New() { return new ParallelCoordinatesData(); }
    ParallelCoordinatesData() = default;

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
};

IGAME_NAMESPACE_END