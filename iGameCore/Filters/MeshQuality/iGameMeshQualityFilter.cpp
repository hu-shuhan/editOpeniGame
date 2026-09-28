#include "iGameMeshQualityFilter.h"

#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <limits>
#include <cmath>

IGAME_NAMESPACE_BEGIN

MeshQualityFilter::MeshQualityFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);

    m_Minimum = 0.0;
    m_Maximum = 0.0;
    m_Average = 0.0;
    m_NumberOfCells = 0;
}

bool MeshQualityFilter::Execute() {
    // 检查输入和初始化
    if (m_Inputs == nullptr ||
        m_Inputs->GetNumberOfElements() == 0) {
        igDebug("MeshQualityFilter: 没有输入网格");
        return false;
    }
    auto input = m_Inputs->GetElement(0);
    if (!input) {
        igDebug("MeshQualityFilter: 输入网格为空");
        return false;
    }
    CellArray::Pointer cells = nullptr;
    Points::Pointer points = nullptr;

    // 判断网格类型，支持surface、volume和unstructured
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
        {
            auto mesh = DynamicCast<SurfaceMesh>(input);
            if (!mesh) {
                igDebug("MeshQualityFilter: SurfaceMesh 转换失败");
                return false;
            }
            cells = mesh->GetFaces();
            points = mesh->GetPoints();
            break;
        }
        case IG_VOLUME_MESH:
        {
            auto mesh = DynamicCast<VolumeMesh>(input);
            if (!mesh) {
                igDebug("MeshQualityFilter: VolumeMesh 转换失败");
                return false;
            }
            cells = mesh->GetCells();
            points = mesh->GetPoints();
            break;
        }
        case IG_UNSTRUCTURED_MESH:
        {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            if (!mesh) {
                igDebug("MeshQualityFilter: UnstructuredMesh 转换失败");
                return false;
            }
            cells = mesh->GetCells();
            points = mesh->GetPoints();
            break;
        }
        default:
        {
            igDebug("MeshQualityFilter: 不支持的网格类型");
            return false;
        }
    }

    if (!cells) {
        igDebug("MeshQualityFilter: 没有 Cell");
        return false;
    }
    if (!points) {
        igDebug("MeshQualityFilter: 没有 Points");
        return false;
    }


    // 初始化统计量
    m_Minimum = std::numeric_limits<double>::max();
    m_Maximum = std::numeric_limits<double>::lowest();
    m_Average = 0.0;
    m_NumberOfCells = 0;
    double sum = 0.0;

    auto surfaceMetricFilter =SurfaceMeshMetricsFilter::New();
    auto volumeMetricFilter =VolumeMeshMetricsFilter::New();

    surfaceMetricFilter->SetPoints(points);
    volumeMetricFilter->SetPoints(points);
    surfaceMetricFilter->SetSurfaceMetric(m_TriangleMetric);
    volumeMetricFilter->SetVolumeMetric(m_TetMetric);

    // 保存每个cell的quality的数组
    DoubleArray::Pointer qualityArray = DoubleArray::New();
    qualityArray->SetName("Quality");
    qualityArray->SetDimension(1);
    qualityArray->Reserve(cells->GetNumberOfCells());

    igIndex cellNum = cells->GetNumberOfCells();
    igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};

    for (igIndex i = 0; i < cellNum; ++i) {
        igIndex vNum = cells->GetCellIds(i, vhs);
        IGenum cellType = IG_EMPTY_CELL;

        // 遍历判断cell类型
        switch (input->GetDataObjectType()) {
            case IG_SURFACE_MESH:
            {
                if (vNum == 3) {
                    cellType = IG_TRIANGLE;
                }
                else if (vNum == 4) {
                    cellType = IG_QUAD;
                }
                break;
            }
            case IG_VOLUME_MESH:
            {
                if (vNum == 4) {
                    cellType = IG_TETRA;
                }
                else if (vNum == 8) {
                    cellType = IG_HEXAHEDRON;
                }
                break;
            }
            case IG_UNSTRUCTURED_MESH:
            {
                auto mesh = DynamicCast<UnstructuredMesh>(input);
                if (!mesh) {
                    igDebug("MeshQualityFilter: UnstructuredMesh 转换失败");
                    return false;
                }
                cellType = mesh->GetCellType(i);
                break;
            }
            default:
                break;
        }

        // 调用meshmetrics的计算quality
        // 目前支持三角形、四边形、四面体、六面体
        double quality = 0.0;
        if (cellType == IG_TRIANGLE) {
            surfaceMetricFilter->SetSurfaceMetric(m_TriangleMetric);
            quality =surfaceMetricFilter->ComputeCellMetric(vNum, vhs);
        }else if (cellType == IG_QUAD) {
            surfaceMetricFilter->SetSurfaceMetric(m_QuadMetric);
            quality =surfaceMetricFilter->ComputeCellMetric(vNum, vhs);
        }else if (cellType == IG_TETRA) {
            volumeMetricFilter->SetVolumeMetric(m_TetMetric);
            quality =volumeMetricFilter->ComputeCellMetric(vNum, vhs);
        }
        else if (cellType == IG_HEXAHEDRON) {
            volumeMetricFilter->SetVolumeMetric(m_HexMetric);
            quality =volumeMetricFilter->ComputeCellMetric(vNum, vhs);
        }
        else {
            qualityArray->AddValue(std::numeric_limits<double>::quiet_NaN());           // 不支持的类型quality暂时设NaN
            continue;
        }

        if (!std::isfinite(quality)) {
            qualityArray->AddValue(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        qualityArray->AddValue(quality);
        // mesh_quality最终输出结果为两个最值范围
        m_Minimum =std::min(m_Minimum, quality);
        m_Maximum =std::max(m_Maximum, quality);
        sum += quality;
        ++m_NumberOfCells;
    }

    if (m_NumberOfCells == 0) {
        igDebug("MeshQualityFilter: 没有可计算质量的 Cell");
        m_Minimum = 0.0;
        m_Maximum = 0.0;
        m_Average = 0.0;
        return false;
    }

    // 暂时不用的计算平均值
    m_Average =sum / static_cast<double>(m_NumberOfCells);

    // 创建独立的输出网格
    DataObject::Pointer output = nullptr;

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
        {
            auto inputMesh = DynamicCast<SurfaceMesh>(input);
            if (!inputMesh) {
                return false;
            }
            auto outputMesh = SurfaceMesh::New();
            if (!outputMesh->DeepCopy(inputMesh)) {
                return false;
            }
            output = outputMesh;
            break;
        }
        case IG_VOLUME_MESH:
        {
            auto inputMesh = DynamicCast<VolumeMesh>(input);
            if (!inputMesh) {
                return false;
            }
            auto outputMesh = VolumeMesh::New();
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(inputMesh->GetPoints())) {
                return false;
            }
            outputMesh->SetPoints(newPoints);
            outputMesh->SetVolumes(inputMesh->GetVolumes());
            outputMesh->SetName(inputMesh->GetName());
            auto newAttributeSet = AttributeSet::New();
            if (!newAttributeSet->DeepCopy(inputMesh->GetAttributeSet())) {
                return false;
            }
            outputMesh->SetAttributeSet(newAttributeSet);
            output = outputMesh;
            break;
        }
        case IG_UNSTRUCTURED_MESH:
        {
            auto inputMesh = DynamicCast<UnstructuredMesh>(input);
            if (!inputMesh) {
                return false;
            }
            auto outputMesh = UnstructuredMesh::New();
            outputMesh->SetCells(inputMesh->GetCells(),inputMesh->GetCellTypes());
            outputMesh->SetName(inputMesh->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(inputMesh->GetPoints())) {
                return false;
            }
            outputMesh->SetPoints(newPoints);
            auto newAttributeSet = AttributeSet::New();
            if (!newAttributeSet->DeepCopy(inputMesh->GetAttributeSet())) {
                return false;
            }
            outputMesh->SetAttributeSet(newAttributeSet);
            output = outputMesh;
            break;
        }
        default:
            return false;
    }

    auto outputAttributeSet = output->GetAttributeSet();
    if (outputAttributeSet == nullptr) {
        return false;
    }

    // 如果已经存在 Quality，则直接替换
    IGint qualityIndex =outputAttributeSet->GetAttributeIndex("Quality");

    if (qualityIndex >= 0) {
        auto& qualityAttribute =outputAttributeSet->GetAttribute(qualityIndex);
        qualityAttribute.SetPointer(qualityArray);
        qualityAttribute.SetType(IG_SCALAR);
        qualityAttribute.SetAttachmentType(IG_CELL);
    }else {
        outputAttributeSet->AddAttribute(
            IG_SCALAR,
            IG_CELL,
            qualityArray
        );
    }

    this->SetOutput(output);
    return true;
}


IGAME_NAMESPACE_END
