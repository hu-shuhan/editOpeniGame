#pragma once
#include <functional>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <string>
#include <vector>
IGAME_NAMESPACE_BEGIN
class iGameSetSelectionCallBackFunc : public Filter {
public:
    I_OBJECT(iGameSetSelectionCallBackFunc);
    static Pointer New(const std::string& funcName,
        const std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope)>& func) {
        return new iGameSetSelectionCallBackFunc(funcName, func);
    }
    bool Execute() override;

private:
    void Run();

protected:
    iGameSetSelectionCallBackFunc(const std::string& funcName,
            const std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope)>& func);
    ~iGameSetSelectionCallBackFunc() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    std::string m_FuncName;
    std::function<void(IGenum itemType, const std::vector<igIndex>& ids, Selection::Operate ope)> m_Func;

private:
    /* Output */
};
IGAME_NAMESPACE_END