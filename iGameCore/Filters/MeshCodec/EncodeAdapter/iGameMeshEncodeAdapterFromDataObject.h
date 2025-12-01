#ifndef MeshEncoderAdapter_h
#define MeshEncoderAdapter_h

#include "iGameFaceTable.h"
#include "MeshCodec/EncodeAdapter/iGameIMeshEncodeAdapter.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "MeshCodec/Utils/iGameMeshCodecThread.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

class MeshEncodeAdapterFromDataObject final : public IMeshEncodeAdapter
{
public:
	explicit MeshEncodeAdapterFromDataObject(const DataObject::Pointer& dataObj) :
		m_DataObj(dataObj)
	{};

	IGenum GetMeshType() override
	{
		return this->m_DataObj->GetDataObjectType();
	}

    IGsize GetCellIdOffsetSize() override
    {
        return this->GetNumberOfCells() + 1;
    }

    IGsize GetNumberOfFaces() override
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

    IGsize GetNumberOfCells() override
    {
        switch (this->GetMeshType())
        {
        case IG_SURFACE_MESH:
        {
            const SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            return mesh->GetNumberOfFaces();
        }
        case IG_VOLUME_MESH:
        {
            const VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            return mesh->GetNumberOfVolumes();
        }
        case IG_STRUCTURED_MESH:
        {
            const StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            return mesh->GetNumberOfCells();
        }
        case IG_UNSTRUCTURED_MESH:
        {
            const UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            return mesh->GetNumberOfCells();
        }
        case IG_POINT_SET:
        {
            return 0;
        }
        default:
            break;
        }
        return 0;
    }

    IGsize GetNumberOfPoints() override
    {
        PointSet::Pointer mesh = DynamicCast<PointSet>(this->m_DataObj);
        return mesh->GetNumberOfPoints();
    }

    bool IsFixedCellSize() override
    {
        if (this->GetMeshType() == IG_POINT_SET) {
            return false;
        }
        
        return this->IsSecondaryIndexPolyhedronMesh() ? false : (this->GetCellIdOffset()->GetNumberOfValues() != this->GetNumberOfCells() + 1);
    }

    // 是否是两级索引的多面体网格
    bool IsSecondaryIndexPolyhedronMesh() override
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
    ) override {
        // 准备数据源
        int volumeNum = 0;
        CellArray::Pointer facesData;

        const IGenum meshType = this->GetMeshType();
        UnstructuredMesh::Pointer unsMesh;
        VolumeMesh::Pointer volMesh;

        // 根据网格类型，准备不同的数据源和构建索引
        if (meshType == IG_UNSTRUCTURED_MESH) {
            // UnstructuredMesh: 需要先构建多面体face索引
            BuildPolyhedronFacesForUnstructured();
            unsMesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            if (!unsMesh || !m_PolyhedronFaces) { return; }
            volumeNum = static_cast<int>(unsMesh->GetNumberOfCells());
            facesData = m_PolyhedronFaces;
        } else {
            // VolumeMesh: 直接使用现有的face数据
            volMesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            if (!volMesh) { return; }
            volumeNum = static_cast<int>(volMesh->GetNumberOfVolumes());
            facesData = volMesh->GetFaces();
        }

        if (volumeNum <= 0 || !facesData) { return; }

        // 提取 cell/volume -> face 索引（多线程并行）
        const int maxThreadSize = CodecThreadPool::GetDefaultThreadCount();
        const int tpResultReserveSize = std::max(1, volumeNum * 10 / std::max(1, maxThreadSize));
        std::vector<std::vector<unsigned int>> v2fIndexResults(maxThreadSize);
        std::vector<std::vector<unsigned int>> v2fOffsetResults(maxThreadSize);

        CodecThreadPool::parallelFor(0, volumeNum, maxThreadSize,
                                [&](int start, int end, int threadIndex) -> void {
                                    auto& localIndex = v2fIndexResults[threadIndex];
                                    auto& localOffset = v2fOffsetResults[threadIndex];

                                    localIndex.reserve(tpResultReserveSize);
                                    localOffset.reserve(end - start);

                                    std::vector<igIndex> fhs;
                                    fhs.resize(IGAME_CELL_MAX_SIZE);

                                    for (int i = start; i < end; i++) {
                                        int fcnt = 0;

                                        // 根据网格类型调用不同的获取函数（使用已缓存的 mesh 指针）
                                        if (meshType == IG_UNSTRUCTURED_MESH) {
                                            fcnt = GetPolyhedronCellFaceIds(i, fhs.data());
                                        } else {
                                            fcnt = volMesh->GetVolumeFaceIds(i, fhs.data());
                                        }

                                        if (fcnt <= 0) {
                                            localOffset.push_back(0);
                                            continue;
                                        }

                                        // 确保容量足够，采用线性扩容避免频繁小扩展
                                        if (localIndex.size() + static_cast<size_t>(fcnt) > localIndex.capacity()) {
                                            const size_t newCap = localIndex.capacity() + static_cast<size_t>(tpResultReserveSize);
                                            localIndex.reserve(newCap);
                                        }

                                        localOffset.push_back(static_cast<unsigned int>(fcnt));
                                        localIndex.insert(localIndex.end(), fhs.begin(), fhs.begin() + fcnt);
                                    }
                                }, maxThreadSize);

        // 预留整体空间并拼接多线程结果
        volume2facesOffset.clear();
        volume2facesOffset.reserve(static_cast<size_t>(volumeNum) + 1);
        volume2facesOffset.push_back(0);

        size_t totalIndexCount = 0;
        for (int i = 0; i < maxThreadSize; ++i) {
            totalIndexCount += v2fIndexResults[i].size();
        }
        volume2facesIndex.clear();
        volume2facesIndex.reserve(totalIndexCount);

        for (int t = 0; t < maxThreadSize; ++t) {
            auto& localIndex = v2fIndexResults[t];
            auto& localOffset = v2fOffsetResults[t];

            volume2facesIndex.insert(volume2facesIndex.end(), localIndex.begin(), localIndex.end());

            for (unsigned int cnt: localOffset) {
                volume2facesOffset.push_back(volume2facesOffset.back() + cnt);
            }
        }
        
        // 提取 face -> points 索引
        IdArray::Pointer f2pIndex = facesData->GetCellIdArray();
        UnsignedIntArray::Pointer f2pOffset = facesData->GetOffset();

        const auto offsetCount = f2pOffset->GetNumberOfValues();
        face2pointsOffset.resize(offsetCount);
        std::memcpy(face2pointsOffset.data(), f2pOffset->RawPointer(),
                    offsetCount * sizeof(unsigned int));

        const auto indexCount = face2pointsOffset[facesData->GetNumberOfCells()];
        face2pointsIndex.resize(indexCount);
        std::memcpy(face2pointsIndex.data(), f2pIndex->RawPointer(),
                    indexCount * sizeof(unsigned int));
    }

    // 仅用于解决二级索引问题
    igIndex GetFaceId(igIndex* ids, int size) override
    {
        return DynamicCast<VolumeMesh>(this->m_DataObj)->GetFaceIdFormPointIds(ids, size);
    }

    Points::Pointer GetPoints() override
    {
        return DynamicCast<PointSet>(this->m_DataObj)->GetPoints();
    }

    int GetFixedCellSize() override
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
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            return mesh->GetCell(0)->GetNumberOfPoints();
        }
        case IG_STRUCTURED_MESH:
        {
            StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            return mesh->GetCell(0)->GetNumberOfPoints();
        }
        case IG_UNSTRUCTURED_MESH:
        {
            UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            return mesh->GetCell(0)->GetNumberOfPoints();
        }
        default:
            break;
        }
        return -1;
    }

	IGsize GetCellIdBufferSize() override
	{
        return this->IsFixedCellSize() ?
            this->GetNumberOfCells() * this->GetFixedCellSize() :
            this->GetCellIdOffset()->GetValue(this->GetCellIdOffsetSize() - 1);
	}

	UnsignedIntArray::Pointer GetCellIdOffset() override
	{
        switch (this->GetMeshType())
        {
        case IG_SURFACE_MESH:
        {
            SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            return mesh->GetFaces()->GetOffset();
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            return mesh->GetCells()->GetOffset();
        }
        case IG_STRUCTURED_MESH:
        {
            StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
            return mesh->GetCells()->GetOffset();
        }
        case IG_UNSTRUCTURED_MESH:
        {
            UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            return mesh->GetCells()->GetOffset();
        }
        default:
            break;
        }
        return nullptr;
	}

    IdArray::Pointer GetCellIdBuffer() override
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
    UnsignedIntArray::Pointer GetCellTypes() override
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

        const UnstructuredMesh::Pointer unsMesh = DynamicCast<UnstructuredMesh>(m_DataObj);
		if (!unsMesh) return;
		
		// 使用FaceTable去重
		FaceTable::Pointer faceTable = FaceTable::New();
		m_PolyhedronCellFaces = CellArray::New();

        const igIndex cellNum = unsMesh->GetNumberOfCells();
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
	int GetPolyhedronCellFaceIds(const IGsize cellId, igIndex* faceIds) const {
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