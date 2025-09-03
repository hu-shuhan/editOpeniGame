#pragma once
#include <functional>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <string>
IGAME_NAMESPACE_BEGIN
class iGameSetClearSelectionCallBackFunc : public Filter {
public:
    I_OBJECT(iGameSetClearSelectionCallBackFunc);
    static Pointer New(Selection::Pointer selection, const std::string& funcName, const std::function<void()>& func) {
        return new iGameSetClearSelectionCallBackFunc(selection, funcName, func);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameSetClearSelectionCallBackFunc(Selection::Pointer selection, const std::string& funcName,
                                       const std::function<void()>& func);
    ~iGameSetClearSelectionCallBackFunc() override = default;

private:
    /* Input */
    Selection::Pointer m_Selection;
    std::string m_FuncName;
    std::function<void()> m_Func;

private:
    /* Output */
};
IGAME_NAMESPACE_END