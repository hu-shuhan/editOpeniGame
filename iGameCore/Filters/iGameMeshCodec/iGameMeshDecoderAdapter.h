#ifndef MeshDecoderAdapter_h
#define MeshDecoderAdapter_h

#include "iGameMacro.h"
#include "iGameDataObject.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class MeshDecoderAdapter
{
public:
    MeshDecoderAdapter(IGenum meshType) :
         m_MeshType(meshType)
    {
        switch (this->m_MeshType)
        {
        case IG_SURFACE_MESH:
        {
            this->m_DataObj = SurfaceMesh::New();
            break;
        }
        case IG_VOLUME_MESH:
        {
            this->m_DataObj = VolumeMesh::New();
            break;
        }
        case IG_UNSTRUCTURED_MESH:
        {
            this->m_DataObj = UnstructuredMesh::New();
            break;
        }
        case IG_STRUCTURED_MESH:
        {
            this->m_DataObj = StructuredMesh::New();
            break;
        }
        default:
            break;
        }
    };

    DataObject::Pointer GetDataObj()
    {
        return this->m_DataObj;
    }

    void SetPointBuffer(const std::vector<float>& input)
    {
        PointSet::Pointer mesh = DynamicCast<PointSet>(this->m_DataObj);
        Points::Pointer ps = Points::New();
        ps->Resize(static_cast<int>(input.size() / 3));

        memcpy(ps->RawPointer(), input.data(), input.size() * sizeof(float));
        mesh->SetPoints(ps);

        //for (int i = 0; i < input.size() / 3; i++)
        //{
        //    mesh->AddPoint({ input[i * 3 + 0], input[i * 3 + 1], input[i * 3 + 2]  });
        //}
    }

    void AddAttribute(const IGenum type, const  IGenum attachmentType, const ArrayObject::Pointer attr)
    {
        this->m_DataObj->GetAttributeSet()->AddAttribute(type, attachmentType, attr);
    }

    void AddSameTypePolyCells(std::vector<uint32_t>& ids, const std::vector<uint32_t>& cellSizes)
    {
        CellArray::Pointer ca = CellArray::New();
        int ret = 0;
        int cursor = 0;
        for (int i = 0; i < cellSizes.size(); i++)
        {
            int cellSize = cellSizes[i];
            ret = ca->AddCellIds(
                reinterpret_cast<igIndex*>(ids.data()) + cursor,
                cellSize
            );
            cursor += cellSize;
        }
        this->SetCellArray(ca);
    }

    void AddSameTypeFixedCells(std::vector<uint32_t>& ids, const int cellSize)
    {
        CellArray::Pointer ca = CellArray::New();
        for (int i = 0; i < ids.size() / cellSize; i++)
        {
            ca->AddCellIds(
                reinterpret_cast<igIndex*>(ids.data()) + i * cellSize,
                cellSize
            );
        }
        this->SetCellArray(ca);
    }

    void AddUnstructuredFixedCells(std::vector<uint32_t>& ids, const int cellSize, std::vector<uint32_t>& types)
    {
        CellArray::Pointer ca = CellArray::New();
        int ret = 0;
        for (int i = 0; i < ids.size() / cellSize; i++)
        {
            ret = ca->AddCellIds(
                reinterpret_cast<igIndex*>(ids.data()) + i * cellSize,
                cellSize
            );
        }
        UnsignedIntArray::Pointer ts = UnsignedIntArray::New();
        ts->Resize(types.size());
        std::memcpy(ts->RawPointer(), types.data(), types.size() * sizeof(uint32_t));


        this->SetUnstructuredCellArray(ca, ts);
    }

    void AddUnstructuredPolyCells(
        std::vector<uint32_t>& ids,
        const std::vector<uint32_t>& cellSizes,
        std::vector<uint32_t>& types)
    {
        CellArray::Pointer ca = CellArray::New();
        int ret = 0;
        int cursor = 0;
        for (int i = 0; i < cellSizes.size(); i++)
        {
            int cellSize = cellSizes[i];
            ret = ca->AddCellIds(
                reinterpret_cast<igIndex*>(ids.data()) + cursor,
                cellSize
            );
            cursor += cellSize;
        }
        UnsignedIntArray::Pointer ts = UnsignedIntArray::New();
        ts->Resize(types.size());
        std::memcpy(ts->RawPointer(), types.data(), types.size() * sizeof(uint32_t));


        this->SetUnstructuredCellArray(ca, ts);
    }

    void AddStructureCells(
        std::vector<uint32_t>& ids,
        int axisSize[3])
    {
        int dimension = axisSize[2] <= 1 ? 2 : 3;

        int cellSize = dimension == 2 ? 4 : 8;
        CellArray::Pointer ca = CellArray::New();
        for (int i = 0; i < ids.size() / cellSize; i++)
        {
            ca->AddCellIds(
                reinterpret_cast<igIndex*>(ids.data()) + i * cellSize,
                cellSize
            );
        }

        DynamicCast<StructuredMesh>(this->m_DataObj)->SetDimensionSize(axisSize);
        this->SetStructuredCellArray(ca, dimension);
    }
    
    // 以下仅用于二阶索引
    void AddSecondaryIndexCells(
        std::vector<unsigned int> volume2facesIndex,
        std::vector<unsigned int> volume2facesSize,
        std::vector<unsigned int> face2pointsIndex,
        std::vector<unsigned int> face2pointsSize
    ) {
        CellArray::Pointer Faces = CellArray::New();
        CellArray::Pointer Volumes = CellArray::New();

        {
            igIndex vhs[IGAME_CELL_MAX_SIZE];
            int cellVcnt;
            int idx = 0;

            for (int i = 0; i < face2pointsSize.size(); i++)
            {
                cellVcnt = face2pointsSize[i];
                for (int j = 0; j < cellVcnt; j++)
                {
                    vhs[j] = face2pointsIndex[idx++];
                }

                Faces->AddCellIds(vhs, cellVcnt);
            }
        }

        {
            igIndex fhs[IGAME_CELL_MAX_SIZE];
            int cellFcnt;
            int idx = 0;

            for (int i = 0; i < volume2facesSize.size(); i++)
            {
                cellFcnt = volume2facesSize[i];
                for (int j = 0; j < cellFcnt; j++)
                {
                    fhs[j] = volume2facesIndex[idx++];
                }

                Volumes->AddCellIds(fhs, cellFcnt);
            }
        }
        
        DynamicCast<VolumeMesh>(this->m_DataObj)->InitVolumesWithPolyhedron(Faces, Volumes);
    }

private:
    DataObject::Pointer m_DataObj;
    IGenum m_MeshType;

    void SetStructuredCellArray(CellArray::Pointer cells, int dimension)
    {
        StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
        
        if (dimension == 2)
        {
            mesh->SetFaces(cells);
        }
        else if (dimension == 3)
        {
            mesh->SetVolumes(cells);
        }
    }

    void SetUnstructuredCellArray(CellArray::Pointer cells, UnsignedIntArray::Pointer types)
    {
        UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
        mesh->SetCells(cells, types);
    }

    void SetCellArray(CellArray::Pointer cells)
    {
        switch (this->m_MeshType)
        {
        case IG_SURFACE_MESH:
        {
            SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            mesh->SetFaces(cells);

            break;
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            mesh->SetVolumes(cells);

            break;
        }
        default:
            break;
        }
    }

    // IG_SURFACE_MESH,
    // IG_VOLUME_MESH,
    // IG_UNSTRUCTURED_MESH,
    // IG_STRUCTURED_MESH,

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