#include "iGamePointAndCellIdsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

int FindAttributeIndex(AttributeSet* attributes, const std::string& name, IGenum attachmentType) {
    auto allAttributes = attributes->GetAllAttributes();
    if (!allAttributes) {
        return -1;
    }

    int result = -1;

    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = allAttributes->GetElement(i);

        if (attribute.IsNone() ||
            attribute.GetAttachmentType() != attachmentType ||
            attribute.GetPointer()->GetName() != name) {
            continue;
        }

        if (result >= 0) {
            return -2;
        }

        result = static_cast<int>(i);
    }

    return result;
}

bool GetCellCount(const DataObject::Pointer& input, IGsize& count) {
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            count = DynamicCast<SurfaceMesh>(input)->GetNumberOfFaces();
            return true;

        case IG_VOLUME_MESH:
            count = DynamicCast<VolumeMesh>(input)->GetNumberOfVolumes();
            return true;

        case IG_UNSTRUCTURED_MESH:
            count = DynamicCast<UnstructuredMesh>(input)->GetNumberOfCells();
            return true;

        case IG_STRUCTURED_MESH:
            count = DynamicCast<StructuredMesh>(input)->GetNumberOfCells();
            return true;

        case IG_LAGRANGE_UNSTRUCTURED_MESH:
            count = DynamicCast<LagrangeUnstructuredMesh>(input)->GetNumberOfCells();
            return true;

        case IG_POINT_SET:
        default:
        return false;
    }
}

// 复制单元数组，避免 CellArray::DeepCopy 在混合单元尺寸下产生错误偏移
CellArray::Pointer DeepCopyCellArray(CellArray* source) {
    auto output = CellArray::New();
    if (!source) {
        return output;
    }

    for (IGsize i = 0; i < source->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        const int count = source->GetCellIds(i, ids);
        output->AddCellIds(ids, count);
    }

    return output;
}

// 复制 PointSet 共有的点坐标和属性
void DeepCopyPointSetBase(PointSet* output, PointSet* input) {
    auto points = Points::New();
    if (input->GetPoints()) {
        points->DeepCopy(input->GetPoints());
    }
    output->SetPoints(points);

    auto attributes = AttributeSet::New();
    if (input->GetAttributeSet()) {
        attributes->DeepCopy(input->GetAttributeSet());
    }
    output->SetAttributeSet(attributes);
    output->SetName(input->GetName());
}

// 按数据类型创建并深拷贝一个独立输出对象
DataObject::Pointer DeepCopyDataObject(DataObject::Pointer input) {
    const IGenum type = input->GetDataObjectType();
    DataObject::Pointer output = DataObject::CreateDataObject(type);
    if (!output && type == IG_LAGRANGE_UNSTRUCTURED_MESH) {
        output = LagrangeUnstructuredMesh::New();
    }
    if (!output) {
        return nullptr;
    }

    auto inputPointSet = DynamicCast<PointSet>(input);
    auto outputPointSet = DynamicCast<PointSet>(output);
    if (!inputPointSet || !outputPointSet) {
        return nullptr;
    }
    DeepCopyPointSetBase(outputPointSet.get(), inputPointSet.get());

    switch (type) {
        case IG_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<UnstructuredMesh>(input);
            auto outputMesh = DynamicCast<UnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            auto cellTypes = UnsignedIntArray::New();
            if (inputMesh->GetCellTypes()) {
                cellTypes->DeepCopy(inputMesh->GetCellTypes());
            }
            outputMesh->SetCells(DeepCopyCellArray(inputMesh->GetCells()), cellTypes);
            break;
        }
        case IG_SURFACE_MESH: {
            auto inputMesh = DynamicCast<SurfaceMesh>(input);
            auto outputMesh = DynamicCast<SurfaceMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_VOLUME_MESH: {
            auto inputMesh = DynamicCast<VolumeMesh>(input);
            auto outputMesh = DynamicCast<VolumeMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetIsPolyhedronType()) {
                // 多面体网格需要同时重建面片和体-面索引
                auto faces = DeepCopyCellArray(inputMesh->GetFaces());
                auto volumeFaces = CellArray::New();
                for (IGsize i = 0; i < inputMesh->GetNumberOfVolumes(); ++i) {
                    std::vector<igIndex> faceIds(IGAME_CELL_MAX_SIZE);
                    const int faceCount = inputMesh->GetVolumeFaceIds(i, faceIds.data());
                    volumeFaces->AddCellIds(faceIds.data(), faceCount);
                }
                outputMesh->InitVolumesWithPolyhedron(faces, volumeFaces);
                break;
            }

            if (inputMesh->GetVolumes()) {
                outputMesh->SetVolumes(DeepCopyCellArray(inputMesh->GetVolumes()));
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            outputMesh->SetIsPolyhedronType(false);
            break;
        }
        case IG_STRUCTURED_MESH: {
            auto inputMesh = DynamicCast<StructuredMesh>(input);
            auto outputMesh = DynamicCast<StructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            igIndex* dimensions = inputMesh->GetDimensionSize();
            if (dimensions) {
                igIndex copiedDimensions[3]{dimensions[0], dimensions[1], dimensions[2]};
                outputMesh->SetDimensionSize(copiedDimensions);
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<LagrangeUnstructuredMesh>(input);
            auto outputMesh = DynamicCast<LagrangeUnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            for (IGsize i = 0; i < inputMesh->GetNumberOfCells(); ++i) {
                const igIndex* ids = nullptr;
                const int count = inputMesh->GetCellPointIds(i, ids);
                std::vector<igIndex> cellIds;
                if (count > 0 && ids) {
                    cellIds.assign(ids, ids + count);
                }
                outputMesh->AddCell(cellIds.data(), count,
                                    inputMesh->GetSpecificCellType(i),
                                    inputMesh->GetCellOrder(i));
            }
            break;
        }
        case IG_POINT_SET:
        default:
            break;
    }

    return output;
}

bool GenerateIds(AttributeSet* attributes,
                 IGenum attachmentType,
                 const std::string& name,
                 IGsize count,
                 LongLongArray::Pointer& output) {
    const int existingIndex = FindAttributeIndex(attributes, name, attachmentType);

    if (existingIndex == -2) {
        return false;
    }

    int attributeIndex = existingIndex;

    if (existingIndex >= 0) {
        auto& attribute = attributes->GetAttribute(existingIndex);

        if (attribute.GetType() != IG_SCALAR) {
            return false;
        }

        output = DynamicCast<LongLongArray>(attribute.GetPointer());
        if (!output) {
            return false;
        }
    } else {
        output = LongLongArray::New();
        output->SetName(name);

        const IGsize newIndex = attributes->AddScalar(attachmentType, output);
        if (newIndex == static_cast<IGsize>(-1)) {
            return false;
        }

        attributeIndex = static_cast<int>(newIndex);
    }

    output->SetName(name);
    output->SetDimension(1);
    output->Resize(count);

    for (IGsize i = 0; i < count; ++i) {
        output->SetValue(i, static_cast<long long>(i));
    }

    if (count > 0) {
        attributes->GetAttribute(attributeIndex).UpdateAllDataRange();
    }

    return true;
}

}

PointAndCellIdsFilter::PointAndCellIdsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void PointAndCellIdsFilter::SetGeneratePointIds(bool value) {
    if (m_GeneratePointIds != value) {
        m_GeneratePointIds = value;
        Modified();
    }
}

void PointAndCellIdsFilter::SetGenerateCellIds(bool value) {
    if (m_GenerateCellIds != value) {
        m_GenerateCellIds = value;
        Modified();
    }
}

void PointAndCellIdsFilter::SetPointIdsArrayName(const std::string& name) {
    if (m_PointIdsArrayName != name) {
        m_PointIdsArrayName = name;
        Modified();
    }
}

void PointAndCellIdsFilter::SetCellIdsArrayName(const std::string& name) {
    if (m_CellIdsArrayName != name) {
        m_CellIdsArrayName = name;
        Modified();
    }
}

bool PointAndCellIdsFilter::Execute() {
    m_Message.clear();
    m_PointIdsArray = nullptr;
    m_CellIdsArray = nullptr;
    SetOutput(nullptr);

    auto input = GetInput(0);
    if (!input) {
        m_Message = "PointAndCellIdsFilter has no input DataObject.";
        return false;
    }

    auto mesh = DynamicCast<PointSet>(input);
    if (!mesh) {
        m_Message = "PointAndCellIdsFilter requires a PointSet input.";
        return false;
    }

    if (!input->GetAttributeSet()) {
        m_Message = "PointAndCellIdsFilter input has no AttributeSet.";
        return false;
    }

    if (m_GeneratePointIds && m_PointIdsArrayName.empty()) {
        m_Message = "Point IDs array name cannot be empty.";
        return false;
    }
    if (m_GenerateCellIds && m_CellIdsArrayName.empty()) {
        m_Message = "Cell IDs array name cannot be empty.";
        return false;
    }

    // 创建独立输出对象，ID 只写入输出对象，不修改输入模型
    auto output = DeepCopyDataObject(input);
    if (!output) {
        m_Message = "Failed to create an independent output object.";
        return false;
    }

    auto outputMesh = DynamicCast<PointSet>(output);
    auto attributes = output->GetAttributeSet();
    if (!outputMesh || !attributes) {
        m_Message = "Failed to prepare output attributes.";
        return false;
    }

    if (m_GeneratePointIds) {
        if (!GenerateIds(attributes,
                         IG_POINT,
                         m_PointIdsArrayName,
                         outputMesh->GetNumberOfPoints(),
                         m_PointIdsArray)) {
            m_Message = "Failed to generate point IDs.";
            return false;
        }
    }

    if (m_GenerateCellIds) {
        IGsize cellCount = 0;
        if (!GetCellCount(output, cellCount)) {
            m_Message = "Unsupported mesh type for cell IDs.";
            return false;
        }

        if (!GenerateIds(attributes,
                         IG_CELL,
                         m_CellIdsArrayName,
                         cellCount,
                         m_CellIdsArray)) {
            m_Message = "Failed to generate cell IDs.";
            return false;
        }
    }

    output->Modified();
    SetOutput(0, output);

    m_Message = "Point and cell IDs generated successfully.";
    return true;
}

IGAME_NAMESPACE_END
