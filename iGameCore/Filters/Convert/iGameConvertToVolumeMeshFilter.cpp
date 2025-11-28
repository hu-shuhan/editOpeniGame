#include "iGameConvertToVolumeMeshFilter.h"
#include "ModelSurfaceFilters/iGameModelGeometryFilter.h"
#include "iGameCell.h"
#include "iGameThreadPool.h"

IGAME_NAMESPACE_BEGIN

ConvertToVolumeMeshFilter::~ConvertToVolumeMeshFilter() {}

bool ConvertToVolumeMeshFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }

    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }

    switch (input->GetDataObjectType()) {
        case IG_NONE:
            return true;
        case IG_VOLUME_MESH:
            this->SetOutput(input);
            return true;
        case IG_SURFACE_MESH:
            return this->ExecuteWithSurfaceMesh(DynamicCast<SurfaceMesh>(input));
        case IG_UNSTRUCTURED_MESH:
            return this->ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(input));
        case IG_STRUCTURED_MESH:
            this->SetOutput(input);
            return true;
        default:
            return false;
    }
    return true;
}

bool ConvertToVolumeMeshFilter::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um) {
    if (!um) return false;

    VolumeMesh::Pointer outputMesh = nullptr;

    switch (m_ConvertMethod) {
        case IG_CONVERT_VOLUME_MESH: {
            // 转换为体网格，需要原始模型就是体网格模型，比如纯体的非结构化网格
            // 类似 TransferToVolumeMesh 的实现
            int cellNum = um->GetNumberOfCells();
            bool couldTransfer = true;
            igIndex cellType = IG_NONE;

            // 检查所有单元是否都是3D单元（体单元）
            for (igIndex i = 0; i < cellNum; i++) {
                cellType = um->GetCellType(i);
                if (Cell::GetCellDimension(cellType) != 3) {
                    couldTransfer = false;
                    break;
                }
            }

            if (!couldTransfer) {
                igDebug("无法转换，因为包含非体单元,请切换为IG_EXTRACT_VOLUME_CELL模式");
                return false; // 无法转换，因为包含非体单元
            }

            // 创建体网格
            outputMesh = VolumeMesh::New();
            outputMesh->SetName(um->GetName());
            outputMesh->SetPoints(um->GetPoints());
            outputMesh->SetVolumes(um->GetCells());
            outputMesh->SetAttributeSet(um->GetAttributeSet());
            break;
        }

        case IG_EXTRACT_VOLUME_CELL: {
            // 提取体单元，专门用于非结构化网格，用于获取非结构化网格中的体单元
            // 类似 ExtractVolumeMesh 的实现
            int cellNum = um->GetNumberOfCells();
            CellArray::Pointer volumes = CellArray::New();
            igIndex vcnt = 0;
            igIndex vhs[IGAME_CELL_MAX_SIZE];
            std::vector<igIndex> v2c; // volume to cell mapping

            // 提取所有3D单元（体单元）
            for (igIndex i = 0; i < cellNum; i++) {
                igIndex cellType = um->GetCellType(i);
                if (Cell::GetCellDimension(cellType) == 3) {
                    vcnt = um->GetCells()->GetCellIds(i, vhs);
                    volumes->AddCellIds(vhs, vcnt);
                    v2c.push_back(i); // 记录体单元对应的原始单元索引
                }
            }

            if (!volumes->GetNumberOfCells()) {
                return false; // 没有找到体单元
            }

            // 创建体网格
            outputMesh = VolumeMesh::New();
            outputMesh->SetName(um->GetName());
            outputMesh->SetPoints(um->GetPoints());
            outputMesh->SetVolumes(volumes);

            // 复制属性数据
            if (um->GetAttributeSet()) {
                AttributeSet::Pointer outAttributeSet = AttributeSet::New();
                iGameModelGeometryFilter::Pointer geometryFilter = iGameModelGeometryFilter::New();
                geometryFilter->CompositeCellAttribute(v2c, um->GetAttributeSet(), outAttributeSet);
                outputMesh->SetAttributeSet(outAttributeSet);
            }
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

bool ConvertToVolumeMeshFilter::ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm) {
    if (!sm) return false;

    VolumeMesh::Pointer outputMesh = nullptr;

    // 表面网格只支持提取体单元模式，其他模式输出日志并使用默认模式
    if (m_ConvertMethod != IG_EXTRACT_VOLUME_CELL) {
        igDebug("表面网格不支持 {} 模式，自动使用 IG_EXTRACT_VOLUME_CELL 模式", (unsigned int) m_ConvertMethod);
    }

    // 表面网格无法直接转换为体网格，输出日志并返回失败
    igDebug("表面网格无法转换为体网格，请使用非结构化网格或体网格作为输入");
    return false;
}


void ConvertToVolumeMeshFilter::SetConvertMethod(ConvertMethod CM) { this->m_ConvertMethod = CM; }

ConvertToVolumeMeshFilter::ConvertToVolumeMeshFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}


IGAME_NAMESPACE_END
