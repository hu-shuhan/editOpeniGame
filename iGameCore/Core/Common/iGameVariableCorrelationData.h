#pragma once
#include <iGameDataObject.h>
#include <iGameCtxPresObjData.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class VariableCorrelationData : public DataObject, public CtxPresObjData_Main, public CtxPresObjData_Draw {
public:
    I_OBJECT(VariableCorrelationData);
    static Pointer New() { return new VariableCorrelationData(); }
    VariableCorrelationData() = default;

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
    CalculateVariableCorrelation(int variableNum, const std::vector<std::vector<double>>& objDatas);
    static std::vector<std::vector<double>>
    CalculateVariableCorrelation(int variableNum, const std::map<int, std::vector<double>>& objDatas);

};
IGAME_NAMESPACE_END