#include "iGameSetClearSelectionCallBackFunc.h"
IGAME_NAMESPACE_BEGIN
bool iGameSetClearSelectionCallBackFunc::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    RUN();
    SetOutput(0, m_Mesh);
    return true;
}

void iGameSetClearSelectionCallBackFunc::RUN() {
    auto selection = m_Mesh->GetSelection();
    selection->_SetClearSelectionCallBackEvent_(m_FuncName, m_Func);
}

iGameSetClearSelectionCallBackFunc::iGameSetClearSelectionCallBackFunc(const std::string& funcName,
                                                                       const std::function<void()>& func) {
    m_FuncName = funcName;
    m_Func = func;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END