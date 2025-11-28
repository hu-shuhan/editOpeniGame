#pragma once
#include <functional>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <string>
#include <vector>
IGAME_NAMESPACE_BEGIN
class SetSelectionCallBackFuncFilter : public Filter {
public:
    I_OBJECT(SetSelectionCallBackFuncFilter);
    static Pointer New(const std::string& funcName,
        const std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope)>& func) {
        return new SetSelectionCallBackFuncFilter(funcName, func);
    }
    bool Execute() override;

private:
    void Run();

protected:
    SetSelectionCallBackFuncFilter(const std::string& funcName,
            const std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope)>& func);
    ~SetSelectionCallBackFuncFilter() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    std::string m_FuncName;
    std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope)> m_Func;

private:
    /* Output */
};
IGAME_NAMESPACE_END