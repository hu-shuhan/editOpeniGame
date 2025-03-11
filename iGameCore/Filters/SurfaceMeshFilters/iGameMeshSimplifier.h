#ifndef IGAME_MESH_SIMPLIFIER_H
#define IGAME_MESH_SIMPLIFIER_H

#include "iGameFilter.h"
#include "iGameModel.h"
#include "iGameSurfaceMesh.h"

IGAME_NAMESPACE_BEGIN
class MeshSimplifier : public Filter {

    using int_t = unsigned int;

    struct Point3 {
        float v[3];
        FORCEINLINE float& operator[](int i) { return v[i]; }
        FORCEINLINE const float& operator[](int i) const { return v[i]; }
    };

    struct Attribute {
        const float* Primitive;
        int Offset;
        int Stride;
    };

public:
    I_OBJECT(MeshSimplifier);
    static Pointer New() { return new MeshSimplifier; }

    bool Execute() override;



protected:
    MeshSimplifier();
    ~MeshSimplifier() override = default;

    //template<class T>
    class TriMeshInternalSimplifier;

};
IGAME_NAMESPACE_END
#endif