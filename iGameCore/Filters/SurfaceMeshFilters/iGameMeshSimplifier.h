#ifndef IGAME_MESH_SIMPLIFIER_H
#define IGAME_MESH_SIMPLIFIER_H

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN
class MeshSimplifier : public Filter {
public:
    I_OBJECT(MeshSimplifier);
    static Pointer New() { return new MeshSimplifier; }
    
    bool Execute() override;

protected:
    MeshSimplifier();
    ~MeshSimplifier() override = default;


private:

};
IGAME_NAMESPACE_END
#endif