#include "ConvertToSurfaceMesh.h"
#include "iGameThreadPool.h"
#include "iGameCell.h"
#include "iGameModelSurfaceFilters/iGameModelGeometryFilter.h"

IGAME_NAMESPACE_BEGIN

ConvertToSurfaceMesh::ConvertToSurfaceMesh() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

ConvertToSurfaceMesh::~ConvertToSurfaceMesh() {}

bool ConvertToSurfaceMesh::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { 
        return false; 
    }
    
    auto input = m_Inputs->GetElement(0);
    if (!input) { 
        return false; 
    }
    
    switch (input->GetDataObjectType()) {
        case IG_NONE:
            return true;
        case IG_VOLUME_MESH:
            return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
        case IG_SURFACE_MESH:
            this->SetOutput(input);
            return true;
        case IG_UNSTRUCTURED_MESH:
            return this->ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(input));
        case IG_STRUCTURED_MESH:
            return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
        default:
            return false;
    }
    return true;
}

bool ConvertToSurfaceMesh::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um) {
    if (!um) return false;
    
    SurfaceMesh::Pointer outputMesh = nullptr;
    
    switch (m_ConvertMethod) {
        case IG_CONVERT_SURFACE_MESH: {
            // 转换为表面，需要原始模型就是表面模型，比如纯表面的非结构化网格
            // 类似 TransferToSurfaceMesh 的实现
            int cellNum = um->GetNumberOfCells();
            bool couldTransfer = true;
            igIndex cellType = IG_NONE;
            
            // 检查所有单元是否都是2D单元（表面单元）
            for (igIndex i = 0; i < cellNum; i++) {
                cellType = um->GetCellType(i);
                if (Cell::GetCellDimension(cellType) != 2) {
                    couldTransfer = false;
                    break;
                }
            }
            
            if (!couldTransfer) {
                igDebug("无法转换，因为包含非表面单元,请切换为IG_EXTRACT_SURFACE_CELL模式");
                return false; // 无法转换，因为包含非表面单元
            }
            
            // 创建表面网格
            outputMesh = SurfaceMesh::New();
            outputMesh->SetName(um->GetName());
            outputMesh->SetPoints(um->GetPoints());
            outputMesh->SetFaces(um->GetCells());
            outputMesh->SetAttributeSet(um->GetAttributeSet());
            break;
        }
        
        case IG_EXTRACT_SURFACE_CELL: {
            // 提取表面单元，专门用于非结构化网格，用于获取非结构化网格中的表面单元
            // 类似 ExtractSurfaceMesh 的实现
            int cellNum = um->GetNumberOfCells();
            CellArray::Pointer faces = CellArray::New();
            igIndex vcnt = 0;
            igIndex vhs[IGAME_CELL_MAX_SIZE];
            std::vector<igIndex> f2c; // face to cell mapping
            
            // 提取所有2D单元（表面单元）
            for (igIndex i = 0; i < cellNum; i++) {
                igIndex cellType = um->GetCellType(i);
                if (Cell::GetCellDimension(cellType) == 2) {
                    vcnt = um->GetCells()->GetCellIds(i, vhs);
                    faces->AddCellIds(vhs, vcnt);
                    f2c.push_back(i); // 记录面对应的原始单元索引
                }
            }
            
            if (!faces->GetNumberOfCells()) {
                igDebug("无法转换，没有表面单元");
                return false; // 没有找到表面单元
            }
            
            // 创建表面网格
            outputMesh = SurfaceMesh::New();
            outputMesh->SetName(um->GetName());
            outputMesh->SetPoints(um->GetPoints());
            outputMesh->SetFaces(faces);
            
            // 复制属性数据
            if (um->GetAttributeSet()) {
                AttributeSet::Pointer outAttributeSet = AttributeSet::New();
                iGameModelGeometryFilter::Pointer geometryFilter = iGameModelGeometryFilter::New();
                geometryFilter->CompositeCellAttribute(f2c, um->GetAttributeSet(), outAttributeSet);
                outputMesh->SetAttributeSet(outAttributeSet);
            }
            break;
        }
        
        case IG_EXTRACT_SURFACE_MESH: {
            auto Extracter = iGame::iGameModelGeometryFilter::New();
            Extracter->SetInput(um);
            Extracter->Execute();
            outputMesh = Extracter->GetExtractMesh();
            break;
        }
        
        default:
            return false;
    }
    
    if (outputMesh) {
        this->SetOutput(outputMesh);
        return true;
    }
    
    return false;
}

bool ConvertToSurfaceMesh::ExecuteWithVolumeMesh(VolumeMesh::Pointer vm) {
    if (!vm) return false;
    
    SurfaceMesh::Pointer outputMesh = nullptr;
    
    // 体网格只支持提取表面网格模式，其他模式输出日志并使用默认模式
    if (m_ConvertMethod != IG_EXTRACT_SURFACE_MESH) {
        igDebug("体网格不支持 " << m_ConvertMethod << " 模式，自动使用 IG_EXTRACT_SURFACE_MESH 模式");
    }
    
    // 统一使用提取表面网格模式
    auto Extracter = iGame::iGameModelGeometryFilter::New();
    Extracter->SetInput(vm);
    Extracter->Execute();
    outputMesh = Extracter->GetExtractMesh();
    
    if (outputMesh) {
        this->SetOutput(outputMesh);
        return true;
    }
    
    return false;
}


void ConvertToSurfaceMesh::SetConvertMethod(ConvertMethod CM) {
    this->m_ConvertMethod = CM;
}


IGAME_NAMESPACE_END