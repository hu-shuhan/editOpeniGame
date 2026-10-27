#pragma once

#include "iGameFilter.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class ExtractBlockFilter : public Filter {
public:
    I_OBJECT(ExtractBlockFilter);
    static Pointer New() { return new ExtractBlockFilter; }

    // Zero-based positions in the direct child list, not global DataObject IDs.
    void SetBlockIndex(int index) { SetBlockPath({index}); }
    void SetBlockPath(const std::vector<int>& path) { m_Path = path; m_ByName = false; }
    void SetBlockName(const std::string& name) { m_Name = name; m_ByName = true; }
    const std::string& GetLastError() const { return m_LastError; }
    static std::vector<DataObject::Pointer> GetBlocks(DataObject::Pointer input);
    bool Execute() override;

protected:
    ExtractBlockFilter();
    ~ExtractBlockFilter() override = default;

private:
    std::vector<int> m_Path{0};
    std::string m_Name;
    std::string m_LastError;
    bool m_ByName{false};
};

IGAME_NAMESPACE_END
