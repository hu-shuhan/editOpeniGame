#include "iGameSetSelectionCallBackFunc.h"
IGAME_NAMESPACE_BEGIN
bool iGameSetSelectionCallBackFunc::Execute() {
    m_Mesh = DynamicCast<UnstructuredMesh>(GetInput(0));
    if (m_Mesh.IsNull()) return false;
    Run();
    SetOutput(0, m_Mesh);
    return true;
}

void iGameSetSelectionCallBackFunc::Run() {
    auto selection = m_Mesh->GetSelection();
    selection->_SetSelectionCallBackEvent_(m_FuncName, m_Func);
}

iGameSetSelectionCallBackFunc::iGameSetSelectionCallBackFunc(
        const std::string& funcName,
        const std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope)>& func) {
    m_FuncName = funcName;
    m_Func = func;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
IGAME_NAMESPACE_END