#pragma once
#include <iGameDataObject.h>
#include <iGameCtxPresObjData.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class VariableCorrelationData : public DataObject, public CtxPresObjData_Main, public CtxPresObjData_Draw {
public:
    I_OBJECT(VariableCorrelationData);

    void SetVariableCorrelation(const std::vector<std::vector<double>>& variableCorrelation);
    const std::vector<std::vector<double>>& GetVariableCorrelation();

    void SetChoosedVariableCorrelation(const std::vector<std::vector<double>>& variableCorrelation);
    const std::vector<std::vector<double>>& GetChoosedVariableCorrelation();


protected:

    std::vector<std::vector<double>> m_VariableCorr;

    std::vector<std::vector<double>> m_ChoosedVariableCorr; //[mainVariableIndex][subVariableIndex]

public:
    /* static funcs */
    static std::vector<std::vector<double>>
    CalculateVariableCorrelation(int variableNum, const std::vector<int>& objIds, CtxPresObjData_Main* theData);
    static std::vector<std::vector<double>> CalculateVariableCorrelation(int variableNum, const std::set<int>& objIds,
                                                                         CtxPresObjData_Main* theData);
    static std::vector<std::vector<double>> CalculateDefaultVariableCorrelation(int variableNum);


public:
    /* init func */
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType,
                       const std::map<Selection::Event::Type, std::map<igIndex, Selection::Event>>& selectedItems,
                       int objNum);
    static Pointer New(ElementArray<AttributeSet::Attribute>::Pointer attrs, IGenum dataType);

protected:
    VariableCorrelationData() = default;
    static Pointer New() { return new VariableCorrelationData(); }

public:
    /* choose func */
    std::vector<igIndex> FiltInRangeIds(int mainVariableIndex, int subVariableIndex, double mainVariableMinValue,
                                        double mainVariableMaxValue, double subVariableMinValue,
                                        double subVariableMaxValue);
};
IGAME_NAMESPACE_END