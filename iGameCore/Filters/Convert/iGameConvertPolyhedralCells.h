#ifndef iGameConvertPolyhedralCells_h
#define iGameConvertPolyhedralCells_h

#include "iGameFilter.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"


IGAME_NAMESPACE_BEGIN
class ConvertPolyhedralCells : public Filter {
public:
    I_OBJECT(ConvertPolyhedralCells);
    static Pointer New() { return new ConvertPolyhedralCells; }

	bool Execute() override 
	{ 
		VolumeMesh::Pointer OldMesh = DynamicCast<VolumeMesh>(GetInput(0));
		VolumeMesh::Pointer NewMesh = VolumeMesh::New();
        SetOutput(NewMesh);
		return ConvertToTetra(OldMesh, NewMesh);
    }

	static bool ConvertToTetra(VolumeMesh::Pointer OldMesh, VolumeMesh::Pointer NewMesh)
	{ 
		if (OldMesh == nullptr || OldMesh->GetIsPolyhedronType() == true) return false;

		//auto OldAttrs = OldMesh->GetAttributeSet();
		auto NewAttrs = NewMesh->GetAttributeSet();
        auto NewCells = NewMesh->GetCells();
        NewCells->Reset();
		for (int i = 0; i < OldMesh->GetNumberOfVolumes(); i++)
		{ 
			auto Cell = OldMesh->GetVolume(i);
            std::vector<iGame::Cell::Pointer> tetras = Cell->clipCelltoTetra();
            for (auto tetra: tetras) 
			{ 
				NewCells->AddCellIds(tetra->m_PointIds);
			}
		}

        auto NewPoints = NewMesh->GetPoints();
        NewPoints->Reset();
		for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { 
			NewPoints->AddPoint(OldMesh->GetPoint(i));
		}
        return true;
	}

protected:
    ConvertPolyhedralCells()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
    ~ConvertPolyhedralCells() override = default;
};
IGAME_NAMESPACE_END
#endif