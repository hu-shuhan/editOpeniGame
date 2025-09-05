#pragma once
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameSelection.h>
#include <functional>
#include <vector>
#include <string>
IGAME_NAMESPACE_BEGIN
class iGameSetSelectionCallBackFunc : public Filter {
public:
    I_OBJECT(iGameSetSelectionCallBackFunc);
    static Pointer New(Selection::Pointer selection, const std::string& funcName,
                       const std::function<void(const std::vector<Selection::Event>&)>& func) {
        return new iGameSetSelectionCallBackFunc(selection, funcName, func);
    }
    bool Execute() override;

private:
    void RUN();

protected:
    iGameSetSelectionCallBackFunc(Selection::Pointer selection, const std::string& funcName,
                                  const std::function<void(const std::vector<Selection::Event>&)>& func);
    ~iGameSetSelectionCallBackFunc() override = default;

private:
    /* Input */
    Selection::Pointer m_Selection;
    std::string m_FuncName;
    std::function<void(const std::vector<Selection::Event>&)> m_Func;

private:
    /* Output */
};
IGAME_NAMESPACE_END