#ifndef MeshEncoderAdapter_h
#define MeshEncoderAdapter_h

#include "iGameMacro.h"
#include "iGameDataObject.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class MeshEncoderAdapter
{
public:
	MeshEncoderAdapter(DataObject::Pointer& dataObj) :
		m_DataObj(dataObj)
	{};

	IGenum GetMeshType()
	{
		return this->m_DataObj->GetDataObjectType();
	}

    IGsize GetCellIdOffsetSize()
    {
        return this->GetNumberOfCells() + 1;
    }

    IGsize GetNumberOfCells()
    {
        switch (this->GetMeshType())
        {
        case IG_SURFACE_MESH:
        {
            SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            return mesh->GetNumberOfFaces();
            break;
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            return mesh->GetNumberOfVolumes();
            break;
        }
        case IG_STRUCTURED_MESH:
        {
            StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            return mesh->GetNumberOfCells();
            break;
        }
        case IG_UNSTRUCTURED_MESH:
        {
            UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            return mesh->GetNumberOfCells();
            break;
        }
        default:
            break;
        }
        return 0;
    }

    IGsize GetNumberOfPoints()
    {
        PointSet::Pointer mesh = DynamicCast<PointSet>(this->m_DataObj);
        return mesh->GetNumberOfPoints();
    }

    bool IsFixedCellSize()
    {
        // 有0号索引
        return this->GetCellIdOffset()->GetNumberOfValues() != this->GetNumberOfCells() + 1;
    }

    Points::Pointer GetPoints()
    {
        return DynamicCast<PointSet>(this->m_DataObj)->GetPoints();
    }

    IGsize GetFixedCellSize()
    {
        if (! this->IsFixedCellSize())
        {
            return 0; // 这种异常情况说明mesh使用的是非定长offset
        }

        switch (this->GetMeshType())
        {
        case IG_SURFACE_MESH:
        {
            SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            return mesh->GetFace(0)->GetNumberOfPoints();
            break;
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            return mesh->GetCell(0)->GetNumberOfPoints();
            break;
        }
        case IG_STRUCTURED_MESH:
        {
            StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            return mesh->GetCell(0)->GetNumberOfPoints();
            break;
        }
        case IG_UNSTRUCTURED_MESH:
        {
            UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            return mesh->GetCell(0)->GetNumberOfPoints();
            break;
        }
        default:
            break;
        }
        return 0;
    }

	IGsize GetCellIdBufferSize()
	{
        return this->IsFixedCellSize() ?
            this->GetNumberOfCells() * this->GetFixedCellSize() :
            this->GetCellIdOffset()->GetValue(this->GetCellIdOffsetSize() - 1);
	}

	UnsignedIntArray::Pointer GetCellIdOffset()
	{
        switch (this->GetMeshType())
        {
        case IG_SURFACE_MESH:
        {
            SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            return mesh->GetFaces()->GetOffset();
            break;
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            return mesh->GetCells()->GetOffset();
            break;
        }
        case IG_STRUCTURED_MESH:
        {
            StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            return mesh->GetCells()->GetOffset();
            break;
        }
        case IG_UNSTRUCTURED_MESH:
        {
            UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            return mesh->GetCells()->GetOffset();
            break;
        }
        default:
            break;
        }
        return nullptr;
	}

	UnsignedIntArray::Pointer GetCellIdBuffer()
	{
        IdArray::Pointer ids;
        switch (this->GetMeshType())
        {
        case IG_SURFACE_MESH:
        {
            SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            ids = mesh->GetFaces()->GetCellIdArray();
            
            break;
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            ids = mesh->GetCells()->GetCellIdArray();

            break;
        }
        case IG_STRUCTURED_MESH:
        {
            StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            ids = mesh->GetCells()->GetCellIdArray();
            
            break;
        }
        case IG_UNSTRUCTURED_MESH:
        {
            UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            ids = mesh->GetCells()->GetCellIdArray();
            
            break;
        }
        default:
            break;
        }

        UnsignedIntArray::Pointer uintIds = UnsignedIntArray::New();
        uintIds->Resize(this->GetCellIdBufferSize());
        memcpy(uintIds->RawPointer(), ids->RawPointer(), this->GetCellIdBufferSize() * sizeof(int));
	
        return uintIds;
    }

    // 仅限 UnstructuredMesh
    UnsignedIntArray::Pointer GetCellTypes()
    {
        assert(this->GetMeshType() == IG_UNSTRUCTURED_MESH);

        UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
        return mesh->GetCellTypes();
    }

private:
	DataObject::Pointer& m_DataObj;

    // template ------------------------------------------------------------------
    //switch (this->GetMeshType())
    //{
    //case IG_SURFACE_MESH:
    //{
    //    SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
    //    break;
    //}
    //case IG_VOLUME_MESH:
    //{
    //    VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
    //    break;
    //}
    //case IG_STRUCTURED_MESH:
    //{
    //    StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
    //    break;
    //}
    //case IG_UNSTRUCTURED_MESH:
    //{
    //    UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
    //    break;
    //}
    //default:
    //    break;
    //}
};

IGAME_NAMESPACE_END
#endif