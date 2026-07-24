#ifndef DATACODEC_WORKFLOW_ENCODE_STAGES_POINTREMAPSTAGE_H
#define DATACODEC_WORKFLOW_ENCODE_STAGES_POINTREMAPSTAGE_H

#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"
#include "DataCodec/Runtime/Failure/EncodeFailureManagement.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"
#include "DataCodec/Codec/Remap/PointRemapBuilder.h"

namespace datacodec {

class PointRemapStage final : public EncodeStage {
public:
    static constexpr std::string_view kTypeName = "PointSpatialPartition";

    explicit PointRemapStage(const EncodeStorageMode storageMode)
        : m_storageMode(storageMode) {}

    const char* Name() const override {
        return m_storageMode == EncodeStorageMode::Memory
            ? "PointSpatialPartition.MortonMemory"
            : "PointSpatialPartition.MortonManaged";
    }

    // 构建几何、拓扑和点属性共享的点重排映射状态
    EncodeStageExecutionStatus Execute(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace) override {
        const auto* geometry = workspace.GeometrySourceView();
        if (geometry == nullptr) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::MissingInput,
                "requires a prepared geometry source");
            return EncodeStageExecutionStatus::Failed;
        }
        if (context.adapter == nullptr ||
            context.adapter->IsStructuredMesh() ||
            geometry->tupleCount <= 1u) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::InvalidInput,
                "point spatial partition requires an unstructured input with at least two points");
            return EncodeStageExecutionStatus::Failed;
        }

        std::string error;
        pointremap::RemapProviders result;
        auto& byteStoreSession = workspace.ByteStoreSessionRef();
        auto& scratchBudget = workspace.CacheResourcesRef().remapScratchBudget;
        const bool useMemoryPointRemap =
            workspace.ResourceBudget().RemapEncodeStorageMode() == EncodeStorageMode::Memory;
        const auto providerFactory = MakeStoreBackedWritableRemapProviderFactory(
            byteStoreSession,
            "point_remap",
            useMemoryPointRemap);
        if (!pointremap::BuildPointMortonRemapProviders(
            *geometry,
            result,
            &error,
            pointremap::BuildOptions{
                .providerFactory = providerFactory,
                .byteStoreSession = &byteStoreSession,
                .scratchBudget = &scratchBudget,
                .mortonLeafBudgetBytes = workspace.ResourceBudget().RemapMortonLeafBytes(),
                .mortonRunBufferBytes = workspace.ResourceBudget().RemapMortonRunBufferBytes(),
                .useMemoryScratchStore = useMemoryPointRemap,
            })) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::InvalidRemap,
                "failed to build point remap order: " + error);
            return EncodeStageExecutionStatus::Failed;
        }
        if (!workspace.SetPointRemap(
                std::move(result.orderProvider),
                std::move(result.inverseProvider))) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::InvalidRemap,
                "point spatial partition did not produce a Morton order");
            return EncodeStageExecutionStatus::Failed;
        }
        return EncodeStageExecutionStatus::Completed;
    }

private:
    EncodeStorageMode m_storageMode{EncodeStorageMode::Managed};
};

} // namespace datacodec

#endif
