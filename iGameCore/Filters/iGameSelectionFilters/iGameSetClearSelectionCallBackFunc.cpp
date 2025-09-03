#include "iGameSetClearSelectionCallBackFunc.h"
IGAME_NAMESPACE_BEGIN
bool iGameSetClearSelectionCallBackFunc::Execute() {
    RUN();
    return true;
}

void iGameSetClearSelectionCallBackFunc::RUN() { m_Selection->_SetClearSelectionCallBackEvent_(m_FuncName, m_Func); }

iGameSetClearSelectionCallBackFunc::iGameSetClearSelectionCallBackFunc(Selection::Pointer selection,
                                                                       const std::string& funcName,
                                                                       const std::function<void()>& func) {
    m_Selection = selection;
    m_FuncName = funcName;
    m_Func = func;
    SetNumberOfInputs(0);
    SetNumberOfOutputs(0);
}
IGAME_NAMESPACE_END