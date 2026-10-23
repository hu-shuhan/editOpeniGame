#include "iGameTemporalShiftScaleFilter.h"

#include "iGameAttributeSet.h"
#include "iGameDrawObject.h"
#include "iGamePointSet.h"
#include "iGameStringArray.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <cmath>
#include <cstddef>
#include <vector>

IGAME_NAMESPACE_BEGIN

TemporalShiftScaleFilter::TemporalShiftScaleFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

DataObject::Pointer TemporalShiftScaleFilter::CreateOutputLike(const DataObject::Pointer& input) {
    if (input == nullptr) { return nullptr; }

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto inputMesh = DynamicCast<SurfaceMesh>(input);
            auto outputMesh = SurfaceMesh::New();
            if (inputMesh.IsNotNull()) {
                outputMesh->ShallowCopy(inputMesh);
            }
            return outputMesh;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<UnstructuredMesh>(input);
            auto outputMesh = UnstructuredMesh::New();
            if (inputMesh.IsNotNull()) {
                outputMesh->SetPoints(inputMesh->GetPoints());
                outputMesh->SetCells(inputMesh->GetCells(), inputMesh->GetCellTypes());
                outputMesh->SetAttributeSet(inputMesh->GetAttributeSet());
            }
            return outputMesh;
        }
        case IG_VOLUME_MESH: {
            auto inputMesh = DynamicCast<VolumeMesh>(input);
            auto outputMesh = VolumeMesh::New();
            if (inputMesh.IsNotNull()) {
                outputMesh->SetVolumes(inputMesh->GetVolumes());
                outputMesh->SetPoints(inputMesh->GetPoints());
                outputMesh->SetAttributeSet(inputMesh->GetAttributeSet());
            }
            return outputMesh;
        }
        case IG_STRUCTURED_MESH: {
            auto inputMesh = DynamicCast<StructuredMesh>(input);
            auto outputMesh = StructuredMesh::New();
            if (inputMesh.IsNotNull()) {
                outputMesh->SetDimensionSize(inputMesh->GetDimensionSize());
                outputMesh->SetExtent(inputMesh->GetExtent());
                outputMesh->SetPoints(inputMesh->GetPoints());
                outputMesh->SetAttributeSet(inputMesh->GetAttributeSet());
            }
            return outputMesh;
        }
        case IG_POINT_SET: {
            auto inputMesh = DynamicCast<PointSet>(input);
            auto outputMesh = PointSet::New();
            if (inputMesh.IsNotNull()) {
                outputMesh->SetPoints(inputMesh->GetPoints());
                outputMesh->SetAttributeSet(inputMesh->GetAttributeSet());
            }
            return outputMesh;
        }
        case IG_DRAW_OBJECT: {
            // .pvd 
            auto inputObject = DynamicCast<DrawObject>(input);
            auto outputObject = DrawObject::New();
            if (inputObject.IsNull()) { return outputObject; }

            outputObject->SetAttributeSet(inputObject->GetAttributeSet());
            outputObject->SetColorMapper(inputObject->GetColorMapper());
            outputObject->SetViewStyle(static_cast<IGenum>(inputObject->GetViewStyle()));
            outputObject->SetPointSize(static_cast<float>(inputObject->GetPointSize()));
            outputObject->SetLineWidth(static_cast<float>(inputObject->GetLineWidth()));
            outputObject->SetTransparency(inputObject->GetTransparency());
            outputObject->SetVisibility(inputObject->GetVisibility());
            outputObject->SetAttributeIndex(inputObject->GetAttributeIndex());

            std::vector<DataObject::Pointer> children;
            children.reserve(static_cast<std::size_t>(inputObject->GetNumberOfSubDataObjects()));
            for (auto it = inputObject->SubDataObjectIteratorBegin(); it != inputObject->SubDataObjectIteratorEnd();
                 ++it) {
                children.push_back(it->second);
            }
            for (auto& child : children) {
                if (child == nullptr) { continue; }
                auto originalColorMapper = child->GetColorMapper();
                outputObject->AddSubDataObject(child);
                child->SetParentDataObject(inputObject.GetPointer());
                child->SetColorMapper(originalColorMapper);
            }
            return outputObject;
        }
        default:
            break;
    }

    // 对象类型不支持
    igDebug("TemporalShiftScaleFilter: 不支持的对象类型 {}", input->GetDataObjectType());
    return nullptr;
}

bool TemporalShiftScaleFilter::Execute() {
    auto input = this->GetInput(0);
    if (input == nullptr) {
        igDebug("TemporalShiftScaleFilter: 输入为空");
        return false;
    }

    auto inputFrames = input->PeekTimeFrames();
    if (inputFrames == nullptr || inputFrames->GetTimeNum() == 0) {
        igDebug("TemporalShiftScaleFilter: 输入没有时间序列（TimeFrames 为空）");
        return false;
    }
    if (std::abs(m_Scale) < 1e-12f) {
        igDebug("TemporalShiftScaleFilter: Scale 不能为 0");
        return false;
    }

    const int timeStepCount = static_cast<int>(inputFrames->GetTimeNum());
    auto outputFrames = StreamingData::New();

    m_InTimeValues.clear();
    m_OutTimeValues.clear();
    m_InTimeValues.reserve(static_cast<std::size_t>(timeStepCount));
    m_OutTimeValues.reserve(static_cast<std::size_t>(timeStepCount));

    for (int index = 0; index < timeStepCount; ++index) {
        auto& inputFrame = inputFrames->GetTargetTimeFrame(index);

        const float inValue = inputFrame.GetTimeValue();
        const float outValue = (inValue + m_PreShift) * m_Scale + m_PostShift;
        outputFrames->AddTimeStep(outValue, inputFrame.GetMetaData(), inputFrame.GetFrameType());

        m_InTimeValues.push_back(inValue);
        m_OutTimeValues.push_back(outValue);
    }

    auto output = CreateOutputLike(input);
    if (output == nullptr) {
        igDebug("TemporalShiftScaleFilter: 创建输出对象失败");
        return false;
    }
    output->SetName(input->GetName());
    output->SetTimeFrames(outputFrames);

    this->SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
