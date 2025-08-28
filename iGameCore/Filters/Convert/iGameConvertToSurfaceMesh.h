#ifndef iGameConvertToSurfaceMesh_h
#define iGameConvertToSurfaceMesh_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToSurfaceMesh : public Filter {
public:
    I_OBJECT(ConvertToSurfaceMesh);
    static Pointer New() { return new ConvertToSurfaceMesh; }

	bool Execute() override 
	{ 
        if (GetInput(0) == nullptr)
            return false;
        
        SurfaceMesh::Pointer NewMesh = SurfaceMesh::New();
        SetOutput(NewMesh);
        if (DynamicCast<UnstructuredMesh>(GetInput(0)))
        { 
            return Convert(DynamicCast<UnstructuredMesh>(GetInput(0)), NewMesh);
        } 

		return false;
    }

	static bool Convert(UnstructuredMesh::Pointer OldMesh, SurfaceMesh::Pointer NewMesh)
	{ 
		if (OldMesh == nullptr) return false;

		auto Mesh = OldMesh->TransferToSurfaceMesh();
        return NewMesh->DeepCopy(Mesh);
	}

protected:
    ConvertToSurfaceMesh()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
    ~ConvertToSurfaceMesh() override = default;
};
IGAME_NAMESPACE_END
#endif