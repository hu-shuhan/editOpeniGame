#include "iGameSetSelectionCallBackFunc.h"
IGAME_NAMESPACE_BEGIN
bool iGameSetSelectionCallBackFunc::Execute() {
    RUN();
    return true;
}

void iGameSetSelectionCallBackFunc::RUN() { m_Selection->_SetSelectionCallBackEvent_(m_FuncName, m_Func); }

iGameSetSelectionCallBackFunc::iGameSetSelectionCallBackFunc(
        Selection::Pointer selection, const std::string& funcName,
        const std::function<void(const std::vector<Selection::Event>&)>& func) {
    m_Selection = selection;
    m_FuncName = funcName;
    m_Func = func;
    SetNumberOfInputs(0);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END