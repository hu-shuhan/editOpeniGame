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
#include "iGameFaceTable.h"
#include "iGamePolyhedron.h"

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
        case IG_UNSTRUCTURED_MESH:
        {
            // 对于多面体网格，返回构建的face数量
            if (m_PolyhedronFaces) {
                return m_PolyhedronFaces->GetNumberOfCells();
            }
            return 0;
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
        case IG_POINT_SET:
        {
            return 0;
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
        if (this->GetMeshType() == IG_POINT_SET) {
            return false;
        }
        
        return this->IsSecondaryIndexPolyhedronMesh() ? false : (this->GetCellIdOffset()->GetNumberOfValues() != this->GetNumberOfCells() + 1);
    }

    // 是否是两级索引的多面体网格
    bool IsSecondaryIndexPolyhedronMesh()
    {
	    switch (this->GetMeshType()) {
	        case IG_VOLUME_MESH: {
	            return DynamicCast<VolumeMesh>(this->m_DataObj)->GetIsPolyhedronType();
	        }
	        case IG_UNSTRUCTURED_MESH: {
	            return static_cast<int>(this->GetCellTypes()->GetValue(0)) == IG_POLYHEDRON;
	        }
	        default:
	            return false;
	    }
    }
    
    void SplitSecondaryIndex(
        std::vector<unsigned int>& volume2facesIndex, 
        std::vector<unsigned int>& volume2facesOffset,
        std::vector<unsigned int>& face2pointsIndex,
        std::vector<unsigned int>& face2pointsOffset
    ){
        // 准备数据源
        int volumeNum;
        CellArray::Pointer facesData;
        
        // 根据网格类型，准备不同的数据源和构建索引
        if (this->GetMeshType() == IG_UNSTRUCTURED_MESH) {
            // UnstructuredMesh: 需要先构建多面体face索引
            if (!m_HasBuiltPolyhedronFaces) {
                BuildPolyhedronFacesForUnstructured();
            }
            UnstructuredMesh::Pointer unsMesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            volumeNum = unsMesh->GetNumberOfCells();
            facesData = m_PolyhedronFaces;
        } else {
            // VolumeMesh: 直接使用现有的face数据
            VolumeMesh::Pointer volmesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            volumeNum = volmesh->GetNumberOfVolumes();
            facesData = volmesh->GetFaces();
        }
        
        // 提取 cell/volume -> face 索引（多线程并行）
        int maxThreadSize = ThreadPool::GetDefaultThreadCount();
        const int tpResultReverseSize = volumeNum * 10 / maxThreadSize;
        std::vector<std::vector<unsigned int>> v2fIndexResults(maxThreadSize);
        std::vector<std::vector<unsigned int>> v2fOffsetResults(maxThreadSize);

        ThreadPool::parallelFor(0, volumeNum, maxThreadSize,
            [&](int start, int end, int threadIndex) -> void {
                v2fIndexResults[threadIndex].reserve(tpResultReverseSize);
                v2fOffsetResults[threadIndex].reserve(end - start);
                
                for (int i = start; i < end; i++) {
                    std::vector<igIndex> fhs(IGAME_CELL_MAX_SIZE);
                    int fcnt;
                    
                    // 根据网格类型调用不同的获取函数
                    if (this->GetMeshType() == IG_UNSTRUCTURED_MESH) {
                        fcnt = GetPolyhedronCellFaceIds(i, fhs.data());
                    } else {
                        VolumeMesh::Pointer volmesh = DynamicCast<VolumeMesh>(this->m_DataObj);
                        fcnt = volmesh->GetVolumeFaceIds(i, fhs.data());
                    }
                    
                    fhs.resize(fcnt);
                    
                    // 动态扩容
                    if (v2fIndexResults[threadIndex].size() + fcnt >= 
                        v2fIndexResults[threadIndex].capacity()) {
                        v2fIndexResults[threadIndex].reserve(
                            v2fIndexResults[threadIndex].capacity() + tpResultReverseSize);
                    }

                    v2fOffsetResults[threadIndex].push_back(fcnt);
                    v2fIndexResults[threadIndex].insert(
                        v2fIndexResults[threadIndex].end(),
                        fhs.begin(), fhs.end());
                }
            }, maxThreadSize);

        // 拼接多线程结果
        volume2facesOffset.push_back(0);
        for (int i = 0; i < maxThreadSize; i++) {
            volume2facesIndex.insert(volume2facesIndex.end(),
                v2fIndexResults[i].begin(), v2fIndexResults[i].end());
            volume2facesOffset.insert(volume2facesOffset.end(),
                v2fOffsetResults[i].begin(), v2fOffsetResults[i].end());
        }

        // 计算累积offset
        for (int i = 1; i < volume2facesOffset.size(); i++) {
            volume2facesOffset[i] += volume2facesOffset[i - 1];
        }
        
        // 提取 face -> points 索引
        IdArray::Pointer f2pIndex = facesData->GetCellIdArray();
        UnsignedIntArray::Pointer f2pOffset = facesData->GetOffset();

        face2pointsOffset.resize(f2pOffset->GetNumberOfValues());
        memcpy(face2pointsOffset.data(), f2pOffset->RawPointer(), 
               f2pOffset->GetNumberOfValues() * sizeof(unsigned int));

        face2pointsIndex.resize(face2pointsOffset[facesData->GetNumberOfCells()]);
        memcpy(face2pointsIndex.data(), f2pIndex->RawPointer(), 
               face2pointsOffset[facesData->GetNumberOfCells()] * sizeof(unsigned int));
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
	
	// 用于UnstructuredMesh多面体网格的face索引数据
	CellArray::Pointer m_PolyhedronCellFaces{};  // 多面体cell->face索引
	CellArray::Pointer m_PolyhedronFaces{};      // 多面体face->points索引
	bool m_HasBuiltPolyhedronFaces = false;      // 标记是否已构建多面体face索引
	
	// 从UnstructuredMesh构建多面体的face索引（仅在需要时调用）
	// 前提：调用此函数时已确认是非结构化网格+多面体网格，所有cell都是IG_POLYHEDRON
	void BuildPolyhedronFacesForUnstructured() {
		if (m_HasBuiltPolyhedronFaces) return; // 避免重复构建
		
		UnstructuredMesh::Pointer unsMesh = DynamicCast<UnstructuredMesh>(m_DataObj);
		if (!unsMesh) return;
		
		// 使用FaceTable去重
		FaceTable::Pointer faceTable = FaceTable::New();
		m_PolyhedronCellFaces = CellArray::New();
		
		igIndex cellNum = unsMesh->GetNumberOfCells();
		igIndex faceIds[IGAME_CELL_MAX_SIZE];
		igIndex ptIds[IGAME_CELL_MAX_SIZE];
		
		for (igIndex i = 0; i < cellNum; i++) {
			// 直接获取Cell对象（前提：已确认所有cell都是多面体）
			Cell* cell = unsMesh->GetCell(i);
			
			// 遍历多面体的所有面
			int faceNum = cell->GetNumberOfFaces();
			for (int j = 0; j < faceNum; j++) {
				Cell* face = cell->GetFace(j);
				
				// 获取face的point索引
				int ptNum = face->m_PointIds->GetNumberOfIds();
				for (int k = 0; k < ptNum; k++) {
					ptIds[k] = face->m_PointIds->GetId(k);
				}
				
				// 检查face是否已存在（去重）
				igIndex faceIdx = faceTable->IsFace(ptIds, ptNum);
				if (faceIdx == -1) {
					// 新face，插入
					faceIdx = faceTable->GetNumberOfFaces();
					faceTable->InsertFace(ptIds, ptNum);
				}
				faceIds[j] = faceIdx;
			}
			
			// 存储当前cell的face索引
			m_PolyhedronCellFaces->AddCellIds(faceIds, faceNum);
		}
		
		// 获取构建的face列表
		m_PolyhedronFaces = faceTable->GetOutput();
		m_HasBuiltPolyhedronFaces = true;
	}
	
	// 获取多面体cell的face索引（仅查询，不负责构建）
	int GetPolyhedronCellFaceIds(IGsize cellId, igIndex* faceIds) {
		if (!m_PolyhedronCellFaces) return -1;
		return m_PolyhedronCellFaces->GetCellIds(cellId, faceIds);
	}

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