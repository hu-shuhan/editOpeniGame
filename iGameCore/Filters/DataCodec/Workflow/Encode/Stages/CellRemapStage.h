#ifndef DATACODEC_WORKFLOW_ENCODE_STAGES_CELLREMAPSTAGE_H
#define DATACODEC_WORKFLOW_ENCODE_STAGES_CELLREMAPSTAGE_H

#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"
#include "DataCodec/Runtime/Failure/EncodeFailureManagement.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"
#include "DataCodec/Workflow/Encode/Stages/EncodeStageCallbacks.h"
#include "DataCodec/Codec/Remap/CellRemapBuilder.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyRemap.h"

#include <functional>
#include <string_view>
#include <utility>
namespace datacodec {

inline cellremap::BuildOptions MakeCellRemapOptions(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const IRemapProvider* pointInverse,
    WritableRemapProviderFactory providerFactory,
    const bool useMemoryRemap) {
    return cellremap::BuildOptions{
        .pointInverse = pointInverse,
        .providerFactory = std::move(providerFactory),
        .byteStoreSession = &workspace.ByteStoreSessionRef(),
        .scratchBudget = &workspace.CacheResourcesRef().remapScratchBudget,
        .mortonLeafBudgetBytes = workspace.ResourceBudget().RemapMortonLeafBytes(),
        .mortonRunBufferBytes = workspace.ResourceBudget().RemapMortonRunBufferBytes(),
        .useMemoryScratchStore = useMemoryRemap,
        .progressCallback = {},
        .resourceCallback = MakeEncodeResourceCallback(context),
    };
}

class CellRemapStage final : public EncodeStage {
public:
    static constexpr std::string_view kTypeName = "CellSpatialPartition";

    explicit CellRemapStage(const EncodeStorageMode storageMode)
        : m_storageMode(storageMode) {}

    const char* Name() const override {
        return m_storageMode == EncodeStorageMode::Memory
            ? "CellSpatialPartition.MortonMemory"
            : "CellSpatialPartition.MortonManaged";
    }

    // 构建拓扑和单元属性共享的单元重排映射状态
    EncodeStageExecutionStatus Execute(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace) override {
        (void)workspace.ReleaseCellRemap();
        if (context.adapter == nullptr) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::MissingInput,
                "requires a valid encode adapter");
            return EncodeStageExecutionStatus::Failed;
        }
        if (context.adapter->GetNumberOfCells() <= 1 ||
            context.adapter->IsStructuredMesh()) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::InvalidInput,
                "cell spatial partition requires an unstructured input with at least two cells");
            return EncodeStageExecutionStatus::Failed;
        }
        auto& byteStoreSession = workspace.ByteStoreSessionRef();
        const bool useMemoryRemap =
            workspace.ResourceBudget().RemapEncodeStorageMode() == EncodeStorageMode::Memory;
        const auto providerFactory = MakeStoreBackedWritableRemapProviderFactory(
            byteStoreSession,
            "cell_remap",
            useMemoryRemap);
        // Cell Remap 消费明确的 Point Order Source
        const auto pointInverse = workspace.PointInverseOrderSource().Handle();
        if (context.adapter->IsPolyhedronMesh()) {
            std::string cellRangeError;
            std::shared_ptr<IRemapProvider> cellOrderProvider;
            if (!polyhedron::BuildPolyhedronCellRangeMortonRemapProvider(
                    *context.adapter,
                    MakeCellRemapOptions(
                        context,
                        workspace,
                        pointInverse.get(),
                        providerFactory,
                        useMemoryRemap),
                    cellOrderProvider,
                    &cellRangeError)) {
                FailEncodeStage(
                    context,
                    workspace,
                    Name(),
                    CodecErrorCode::InvalidTopology,
                    "failed to build polyhedron cell range remap: " + cellRangeError);
                return EncodeStageExecutionStatus::Failed;
            }
            if (!workspace.SetCellRemap(std::move(cellOrderProvider))) {
                FailEncodeStage(
                    context,
                    workspace,
                    Name(),
                    CodecErrorCode::InvalidRemap,
                    "cell spatial partition did not produce a polyhedron Morton order");
                return EncodeStageExecutionStatus::Failed;
            }
            return EncodeStageExecutionStatus::Completed;
        }

        std::string providerError;
        TopologyView topology;
        if (!cellremap::BuildCellTopologyView(*context.adapter, topology, &providerError)) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::InvalidTopology,
                providerError);
            return EncodeStageExecutionStatus::Failed;
        }
        std::shared_ptr<IRemapProvider> orderProvider;
        if (!cellremap::BuildMortonRemapProvider(
                topology,
                MakeCellRemapOptions(
                    context,
                    workspace,
                    pointInverse.get(),
                    providerFactory,
                    useMemoryRemap),
                orderProvider,
                &providerError)) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::InvalidRemap,
                "failed to build cell remap order: " + providerError);
            return EncodeStageExecutionStatus::Failed;
        }
        if (!workspace.SetCellRemap(std::move(orderProvider))) {
            FailEncodeStage(
                context,
                workspace,
                Name(),
                CodecErrorCode::InvalidRemap,
                "cell spatial partition did not produce a Morton order");
            return EncodeStageExecutionStatus::Failed;
        }
        return EncodeStageExecutionStatus::Completed;
    }

private:
    EncodeStorageMode m_storageMode{EncodeStorageMode::Managed};
};

} // namespace datacodec

#endif
