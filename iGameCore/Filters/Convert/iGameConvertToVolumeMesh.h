#ifndef iGameConvertToVolumeMesh_h
#define iGameConvertToVolumeMesh_h

#include "iGameFilter.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToVolumeMesh : public Filter {
public:
    I_OBJECT(ConvertToVolumeMesh);
    static Pointer New() { return new ConvertToVolumeMesh; }

	bool Execute() override 
	{ 
        if (GetInput(0) == nullptr)
            return false;
        
        //VolumeMesh::Pointer NewMesh = VolumeMesh::New();
        
        if (DynamicCast<UnstructuredMesh>(GetInput(0)))
        { 
            SetOutput(Convert(DynamicCast<UnstructuredMesh>(GetInput(0))));
            return true;
        } 

		return false;
    }

    static VolumeMesh::Pointer Convert(UnstructuredMesh::Pointer OldMesh) {
        if (OldMesh == nullptr) return nullptr;
        return OldMesh->TransferToVolumeMesh();
    }

protected:
    ConvertToVolumeMesh()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
    ~ConvertToVolumeMesh() override = default;
};
IGAME_NAMESPACE_END
#endif