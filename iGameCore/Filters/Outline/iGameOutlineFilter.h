#pragma once

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class OutlineFilter : public Filter {
public:
    I_OBJECT(OutlineFilter);
    static Pointer New() { return new OutlineFilter; }

    bool Execute() override;
    const std::string& GetMessage() const { return m_Message; }

protected:
    OutlineFilter();
    ~OutlineFilter() override = default;

private:
    std::string m_Message;
};

IGAME_NAMESPACE_END
