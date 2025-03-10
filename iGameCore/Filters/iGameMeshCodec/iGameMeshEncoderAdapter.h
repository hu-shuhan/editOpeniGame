#ifndef MeshEncoderAdapter_h
#define MeshEncoderAdapter_h

#include "iGameMacro.h"
#include "iGameDataObject.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameThreadPool.h"
#include "iGameMeshCodecAdjacency.h"

IGAME_NAMESPACE_BEGIN
class MeshEncoderAdapter
{
public:
	MeshEncoderAdapter(DataObject::Pointer dataObj) :
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

    IGsize GetNumberOfFaces()
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
            return mesh->GetNumberOfFaces();
            break;
        }
        case IG_STRUCTURED_MESH:
        {
            StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            return mesh->GetNumberOfFaces();
            break;
        }
        default:
            break;
        }
        return 0;
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
        return this->IsSecondaryIndexPolyhedronMesh() ? false : (this->GetCellIdOffset()->GetNumberOfValues() != this->GetNumberOfCells() + 1);
    }

    // 是否是两级索引的多面体网格
    bool IsSecondaryIndexPolyhedronMesh()
    {
        return this->GetMeshType() == IG_VOLUME_MESH ?
            DynamicCast<VolumeMesh>(this->m_DataObj)->GetIsPolyhedronType() : false;
    }
    
    void SplitSecondaryIndex(
        std::vector<unsigned int>& volume2facesIndex, 
        std::vector<unsigned int>& volume2facesOffset,
        std::vector<unsigned int>& face2pointsIndex,
        std::vector<unsigned int>& face2pointsOffset
    ){
        // 备忘 cgns多面体以volume mesh形式读入 且无法转化至unstructure mesh
        // 体 -> 面, 面 -> 顶点 两级索引 各自持有一个offset
        VolumeMesh::Pointer volmesh = DynamicCast<VolumeMesh>(this->m_DataObj);
        int maxThreadSize = 16;
        // 多线程分组处理volume
        
        {
            int volumeNum = volmesh->GetNumberOfVolumes();
            const int tpResultReverseSize = volumeNum * 10 / maxThreadSize;
            std::vector<std::vector<unsigned int>> v2fIndexResults(maxThreadSize);
            std::vector<std::vector<unsigned int>> v2fOffsetResults(maxThreadSize);

            ThreadPool::parallelFor(0, volumeNum, maxThreadSize,
                [&](int start, int end, int threadIndex) -> void {
                    // 先开辟较大的空间 稍后若不足再补充
                    v2fIndexResults[threadIndex].reserve(tpResultReverseSize);
                    v2fOffsetResults[threadIndex].reserve(end - start);
                    for (int i = start; i < end; i++)
                    {
                        // 取一个volume的面ids 并存储
                        std::vector<igIndex> fhs(IGAME_CELL_MAX_SIZE);
                        int fcnt = volmesh->GetVolumeFaceIds(i, fhs.data());
                        fhs.resize(fcnt);

                        // 如果tpResults空间不足就补充一下
                        if (v2fIndexResults.size() + fcnt >= v2fIndexResults[threadIndex].capacity())
                        {
                            v2fIndexResults[threadIndex]
                                .reserve(v2fIndexResults[threadIndex].capacity() + tpResultReverseSize);
                        }

                        v2fOffsetResults[threadIndex].push_back(fcnt);
                        v2fIndexResults[threadIndex].insert(v2fIndexResults[threadIndex].end(),
                            fhs.begin(), fhs.end());
                    }
                }, maxThreadSize);

            // 拼接到结果
            volume2facesOffset.push_back(0);
            for (int i = 0; i < maxThreadSize; i++)
            {
                volume2facesIndex.insert(
                    volume2facesIndex.end(),
                    v2fIndexResults[i].begin(), v2fIndexResults[i].end());

                volume2facesOffset.insert(
                    volume2facesOffset.end(),
                    v2fOffsetResults[i].begin(), v2fOffsetResults[i].end());
            }

            for (int i = 1; i < volume2facesOffset.size(); i++)
            {
                volume2facesOffset[i] += volume2facesOffset[i - 1];
            }
        }

        IdArray::Pointer f2pIndex = volmesh->GetFaces()->GetCellIdArray();
        UnsignedIntArray::Pointer f2pOffset = volmesh->GetFaces()->GetOffset();

        face2pointsOffset.resize(f2pOffset->GetNumberOfValues());
        memcpy(face2pointsOffset.data(), f2pOffset->RawPointer(), f2pOffset->GetNumberOfValues() * sizeof(unsigned int));

        face2pointsIndex.resize(face2pointsOffset[volmesh->GetNumberOfFaces()]);
        memcpy(face2pointsIndex.data(), f2pIndex->RawPointer(), face2pointsOffset[volmesh->GetNumberOfFaces()] * sizeof(unsigned int));

        return;
    }

    // 仅用于解决二级索引问题
    igIndex GetFaceId(igIndex* ids, int size)
    {
        return DynamicCast<VolumeMesh>(this->m_DataObj)->GetFaceIdFormPointIds(ids, size);
    }

    Points::Pointer GetPoints()
    {
        return DynamicCast<PointSet>(this->m_DataObj)->GetPoints();
    }

    int GetFixedCellSize()
    {
        if (! this->IsFixedCellSize())
        {
            return -1; // 这种异常情况说明mesh使用的是非定长offset
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
        return -1;
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
            //std::cout << m_DataObj->GetBoundingBox().center() << std::endl;
            //std::cout << mesh->GetCells() << std::endl;
            //std::cout << mesh->GetCells()->GetNumberOfCellIds() << std::endl;
            return mesh->GetCells()->GetOffset();
            break;
        }
        default:
            break;
        }
        return nullptr;
	}

    IdArray::Pointer GetCellIdBuffer()
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

        return ids;
    }

    // 仅限 UnstructuredMesh --------------------------------------------------------------------
    UnsignedIntArray::Pointer GetCellTypes()
    {
        assert(this->GetMeshType() == IG_UNSTRUCTURED_MESH);

        UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
        return mesh->GetCellTypes();
    }

private:
	DataObject::Pointer m_DataObj;

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