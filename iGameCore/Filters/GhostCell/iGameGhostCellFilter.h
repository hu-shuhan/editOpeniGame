#pragma once
#include "iGameFilter.h"
#include <string>
IGAME_NAMESPACE_BEGIN
// Single-host generation on an existing partition tree of tetrahedral and
// hexahedral UnstructuredMesh leaves. Creates independent output, no repartitioning.
class GhostCellFilter : public Filter {
public:
    I_OBJECT(GhostCellFilter);
    static Pointer New() { return new GhostCellFilter; }
    void SetNumberOfGhostLayers(int n) { m_Layers = n; }
    int GetNumberOfGhostLayers() const { return m_Layers; }
    // All leaves must provide scalar integer IDs, or none may provide them.
    // Empty name selects exact coordinate matching instead.
    void SetGlobalPointIdsArrayName(const std::string& name) { m_GlobalIds = name; }
    const std::string& GetLastError() const { return m_Error; }
    bool Execute() override;
protected:
    GhostCellFilter();
    ~GhostCellFilter() override = default;
private:
    int m_Layers{1};
    std::string m_GlobalIds{"GlobalPointIds"}, m_Error;
};
IGAME_NAMESPACE_END
