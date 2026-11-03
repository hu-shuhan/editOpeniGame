#ifndef iGameExtractSelectionFilter_h
#define iGameExtractSelectionFilter_h

#include "iGameFilter.h"
#include <string>

IGAME_NAMESPACE_BEGIN

class ExtractSelectionFilter : public Filter {
public:
    I_OBJECT(ExtractSelectionFilter);

    static Pointer New() { return new ExtractSelectionFilter; }

    bool Execute() override;
    void SetSelectionType(IGenum type) { m_SelectionType = type; }
    IGenum GetSelectionType() const { return m_SelectionType; }
    const std::string& GetLastError() const { return m_LastError; }

protected:
    ExtractSelectionFilter();
    ~ExtractSelectionFilter() override = default;

private:
    IGenum m_SelectionType{IG_POINT};
    std::string m_LastError;
};

IGAME_NAMESPACE_END

#endif
