#include "iGameSetClearSelectionCallBackFunc.h"
IGAME_NAMESPACE_BEGIN
bool iGameSetClearSelectionCallBackFunc::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    Run();
    SetOutput(0, m_Mesh);
    return true;
}

void iGameSetClearSelectionCallBackFunc::Run() {
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