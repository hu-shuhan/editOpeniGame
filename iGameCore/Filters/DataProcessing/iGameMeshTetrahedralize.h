#pragma once;

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class MeshTetrahedralize : public Filter {
public:
    I_OBJECT(MeshTetrahedralize);
    static Pointer New() { return new MeshTetrahedralize; }

    bool Execute() override;

protected:
    MeshTetrahedralize();
    ~MeshTetrahedralize() override = default;
};
IGAME_NAMESPACE_END