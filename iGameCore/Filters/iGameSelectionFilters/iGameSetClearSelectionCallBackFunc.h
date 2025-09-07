#pragma once
#include <functional>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <iGameUnstructuredMesh.h>
#include <string>
IGAME_NAMESPACE_BEGIN
class iGameSetClearSelectionCallBackFunc : public Filter {
public:
    I_OBJECT(iGameSetClearSelectionCallBackFunc);
    static Pointer New(const std::string& funcName, const std::function<void()>& func) {
        return new iGameSetClearSelectionCallBackFunc(funcName, func);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameSetClearSelectionCallBackFunc(const std::string& funcName, const std::function<void()>& func);
    ~iGameSetClearSelectionCallBackFunc() override = default;

private:
    /* Input */
    UnstructuredMesh::Pointer m_Mesh;
    std::string m_FuncName;
    std::function<void()> m_Func;

private:
    /* Output */
};
IGAME_NAMESPACE_END