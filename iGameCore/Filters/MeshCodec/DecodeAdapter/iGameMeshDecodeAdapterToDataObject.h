#ifndef MeshDecoderAdapterToDataObject_h
#define MeshDecoderAdapterToDataObject_h

#include "MeshCodec/DecodeAdapter/iGameIMeshDecodeAdapter.h"
#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

class MeshDecodeAdapterToDataObject final : public IMeshDecodeAdapter<DataObject::Pointer>
{
public:
    MeshDecodeAdapterToDataObject() = default;

    void SetMeshType(IGenum meshType) override {
        m_MeshType = meshType;
        switch (m_MeshType)
        {
        case IG_SURFACE_MESH:
            m_DataObj = SurfaceMesh::New();
            break;
        case IG_VOLUME_MESH:
            m_DataObj = VolumeMesh::New();
            break;
        case IG_UNSTRUCTURED_MESH:
            m_DataObj = UnstructuredMesh::New();
            break;
        case IG_STRUCTURED_MESH:
            m_DataObj = StructuredMesh::New();
            break;
        case IG_POINT_SET:
            m_DataObj = PointSet::New();
            break;
        default:
            break;
        }
    }

    DataObject::Pointer GetOutput() override
    {
        return this->m_DataObj;
    }

    void SetPointBuffer(const std::vector<float>& input) override
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

    void AddAttribute(const AttributeBuffer& attr) override
    {
        if (attr.isFloat()) {
            if (attr.floatData.empty()) return;
            FloatArray::Pointer arr = FloatArray::New();
            arr->SetDimension(attr.dimension);
            arr->SetName(attr.name);
            arr->Resize(static_cast<int>(attr.floatData.size() / attr.dimension));
            std::memcpy(arr->RawPointer(), attr.floatData.data(), attr.floatData.size() * sizeof(float));
            this->m_DataObj->GetAttributeSet()->AddAttribute(attr.type, attr.attachmentType, arr);
        } else if (attr.isUInt8()) {
            if (attr.uint8Data.empty()) return;
            UnsignedCharArray::Pointer arr = UnsignedCharArray::New();
            arr->SetDimension(attr.dimension);
            arr->SetName(attr.name);
            arr->Resize(static_cast<int>(attr.uint8Data.size() / attr.dimension));
            std::memcpy(arr->RawPointer(), attr.uint8Data.data(), attr.uint8Data.size() * sizeof(unsigned char));
            this->m_DataObj->GetAttributeSet()->AddAttribute(attr.type, attr.attachmentType, arr);
        } else {
            if (attr.doubleData.empty()) return;
            DoubleArray::Pointer arr = DoubleArray::New();
            arr->SetDimension(attr.dimension);
            arr->SetName(attr.name);
            arr->Resize(static_cast<int>(attr.doubleData.size() / attr.dimension));
            std::memcpy(arr->RawPointer(), attr.doubleData.data(), attr.doubleData.size() * sizeof(double));
            this->m_DataObj->GetAttributeSet()->AddAttribute(attr.type, attr.attachmentType, arr);
        }
    }

    void AddSameTypePolyCells(std::vector<uint32_t>& ids, const std::vector<uint32_t>& cellSizes) override
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

    void AddSameTypeFixedCells(std::vector<uint32_t>& ids, int cellSize) override
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

    void AddUnstructuredFixedCells(std::vector<uint32_t>& ids, int cellSize, std::vector<uint32_t>& types) override
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
        std::vector<uint32_t>& types) override
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
        int axisSize[3]) override
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

    void SetStructuredMeshDimension(int axisSize[3]) override
    {
        StructuredMesh::Pointer mesh = DynamicCast<StructuredMesh>(this->m_DataObj);
        mesh->SetDimensionSize(axisSize);
        mesh->GenStructuredCellConnectivities();
    }
    
    void AddSecondaryIndexCells(
        std::vector<unsigned int> volume2facesIndex,
        std::vector<unsigned int> volume2facesSize,
        std::vector<unsigned int> face2pointsIndex,
        std::vector<unsigned int> face2pointsSize) override
    {
        // 构建 Faces (face->points)
        CellArray::Pointer Faces = CellArray::New();
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

        // 根据网格类型选择不同的处理方式
        if (this->m_MeshType == IG_VOLUME_MESH) {
            // VolumeMesh: 直接使用 InitVolumesWithPolyhedron
            CellArray::Pointer Volumes = CellArray::New();
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
        else if (this->m_MeshType == IG_UNSTRUCTURED_MESH) {
            // UnstructuredMesh: 构建多面体的展开格式
            CellArray::Pointer realCells = CellArray::New();
            UnsignedIntArray::Pointer CellTypes = UnsignedIntArray::New();
            
            igIndex realVhs[IGAME_CELL_MAX_SIZE] = {0};
            igIndex vhs[IGAME_CELL_MAX_SIZE];
            int v2fIdx = 0;
            
            int volumeNum = volume2facesSize.size();
            CellTypes->Resize(volumeNum);
            std::fill(CellTypes->RawPointer(), CellTypes->RawPointer() + volumeNum, IG_POLYHEDRON);
            
            for (int cellId = 0; cellId < volumeNum; cellId++) {
                int realVcnt = 0;
                int fcnt = volume2facesSize[cellId];
                realVhs[realVcnt++] = fcnt;  // 第1个元素：face数量
                
                for (int i = 0; i < fcnt; i++) {
                    igIndex faceId = volume2facesIndex[v2fIdx++];
                    int vcnt = Faces->GetCellIds(faceId, vhs);
                    realVhs[realVcnt++] = vcnt;  // 每个face的顶点数量
                    for (int j = 0; j < vcnt; j++) {
                        realVhs[realVcnt++] = vhs[j];  // face的顶点索引
                    }
                }
                realCells->AddCellIds(realVhs, realVcnt);
            }
            
            this->SetUnstructuredCellArray(realCells, CellTypes);
        }
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
        // ⚠ 这里曾经是**无条件** `DynamicCast<UnstructuredMesh>(m_DataObj)` 然后直接
        // `mesh->SetCells(...)`。当 `m_MeshType == IG_VOLUME_MESH` 时
        // `m_DataObj` 是 VolumeMesh，**不是** UnstructuredMesh 的子类
        // （VolumeMesh : SurfaceMesh : PointSet；UnstructuredMesh 是另一条分支），
        // 于是 DynamicCast 返回 nullptr，紧接着的空指针解引用就是
        // **0xC0000005 访问违例**。
        //
        // 触发路径（IGV P1 的 V1.9 首次覆盖到）：
        //   .igv 里 IGVDataType=VolumeMesh → RebuildFromParsed() 走 else 分支
        //   → 文件含 Types → AddUnstructuredFixedCells() → 本函数 → 崩溃。
        // 也就是说：**读回任何 VolumeMesh 的 .igv 都会崩**，而不是报错。
        // 之所以 P1 一直没发现，是因为当时没有任何夹具能产出 IG_VOLUME_MESH
        // （VTK legacy 读入路径根本不建它，见 kia/测试模型列表.md §5.1）。
        //
        // 修法按 mesh 类型分派：UnstructuredMesh 用 SetCells（带 Types），
        // VolumeMesh 用 SetVolumes（体数组即一级索引；它没有 SetCells，
        // 其 Types 由 GetCellTypes() 从各体点数推导 —— 与 IGV 写侧的
        // "无 Types 时由点数推类型" 对称）。
        switch (this->m_MeshType)
        {
        case IG_UNSTRUCTURED_MESH:
        {
            UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(this->m_DataObj);
            if (!mesh) { return; }
            mesh->SetCells(cells, types);
            break;
        }
        case IG_VOLUME_MESH:
        {
            VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(this->m_DataObj);
            if (!mesh) { return; }
            mesh->SetVolumes(cells);
            break;
        }
        case IG_SURFACE_MESH:
        {
            SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(this->m_DataObj);
            if (!mesh) { return; }
            mesh->SetFaces(cells);
            break;
        }
        default:
            // 其余类型（PointSet / StructuredMesh）不应走到这里。
            break;
        }
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