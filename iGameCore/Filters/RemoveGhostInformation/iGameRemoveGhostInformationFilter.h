#pragma once

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class RemoveGhostInformationFilter : public Filter {
public:
    I_OBJECT(RemoveGhostInformationFilter);

    static Pointer New() { return new RemoveGhostInformationFilter; }

    bool Execute() override;

    bool WasModified() const { return m_WasModified; }

protected:
    RemoveGhostInformationFilter();
    ~RemoveGhostInformationFilter() override = default;

private:
    bool m_WasModified{false};
};

IGAME_NAMESPACE_END
