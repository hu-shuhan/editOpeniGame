#ifndef DATACODEC_WORKFLOW_ENCODE_STAGES_TOPOSTAGE_H
#define DATACODEC_WORKFLOW_ENCODE_STAGES_TOPOSTAGE_H

#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyBlockEncode.h"
#include "DataCodec/Runtime/Failure/EncodeFailureManagement.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyEncode.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
namespace datacodec {

inline void ReportTopologyFailure(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const CodecErrorCode code,
    const std::string& message) {
    FailEncodeStage(context, workspace, "TopoStage", code, message);
}

inline void ReleasePolyhedronTopologyInputs(
    EncodeContext& context) {
    if (context.adapter != nullptr) {
        (void)context.adapter->ReleaseConvertedInputs();
    }
}

inline callback::ProgressCallback MakeTopologyProgressTimingCallback(
    EncodeContext& context,
    std::string prefix) {
    auto lastTick = callback::Now();
    double lastProgress = 0.0;
    return [&context, prefix = std::move(prefix), lastTick, lastProgress](const double normalized) mutable {
        const auto now = callback::Now();
        const auto elapsedMs = callback::ElapsedMilliseconds(lastTick, now);
        const auto beginPercent = static_cast<int>(lastProgress * 100.0 + 0.5);
        const auto endPercent = static_cast<int>(normalized * 100.0 + 0.5);
        context.runRecords.RecordStageTiming(
            prefix + "." + std::to_string(endPercent),
            elapsedMs,
            TelemetryStageCategory::Topology,
            std::to_string(beginPercent) + "-" + std::to_string(endPercent) + "%");
        lastTick = now;
        lastProgress = normalized;
    };
}

inline void RunTopologyEncode(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const std::size_t transferCacheIndex) {
    if (context.adapter == nullptr) {
        ReportTopologyFailure(
            context,
            workspace,
            CodecErrorCode::MissingInput,
            "requires a valid encode adapter");
        return;
    }

    topocodec::TopologyEncodeInput input{
        .data = topocodec::TopologyEncodeData{
            .adapter = *context.adapter,
            .pointInverseOrderSource = workspace.PointInverseOrderSource(),
            .cellOrderSource = workspace.CellOrderSource(),
        },
        .execution = topocodec::TopologyEncodeExecutionParams{
            .resourceBudget = workspace.ResourceBudget(),
            .cellElementCount = workspace.StorageParams().spatialBlockParams.cellElementCount,
            .parallelTaskRunner = context.parallelTaskRunner,
            .workerCount = workspace.ResourceBudget().TopologyBlockLaneCount(),
        },
        .runtime = topocodec::TopologyEncodeRuntime{
            .byteStoreSession = workspace.ByteStoreSessionRef(),
        },
    };
    if (context.runRecords.Wants(RunRecordKind::StageTiming)) {
        input.context.progressCallback =
            MakeTopologyProgressTimingCallback(context, "TopoStage.connectivity.progress");
    }
    if (context.runRecords.Wants(RunRecordKind::ResourceUsage)) {
        input.context.memoryCheckpoint =
            [&context](const char* name, const std::uint64_t logicalBytes, std::string scope) {
                context.runRecords.RecordResourceUsage(
                    std::string(name),
                    logicalBytes,
                    TelemetryStageCategory::Topology,
                    std::move(scope));
            };
    }

    topocodec::TopologyEncodeResult result;
    std::string error;
    if (!topocodec::EncodeTopologyToTransferCache(input, result, &error)) {
        ReportTopologyFailure(
            context,
            workspace,
            CodecErrorCode::EncodeFailure,
            "failed to encode connectivity topology: " + error);
        return;
    }
    if (result.transferCache == nullptr) {
        ReportTopologyFailure(
            context,
            workspace,
            CodecErrorCode::EncodeFailure,
            "failed to publish connectivity topology transfer cache");
        return;
    }

    workspace.StorageParams().topoParams = result.topo;
    if (result.topo.isStructured) {
        workspace.StorageParams().structuredMeshParams.axisSize = result.structuredAxisSize;
    }
    context.runSummary.topologyBytes = static_cast<std::uint64_t>(result.topo.binaryCount);
    workspace.PublishTransferCache(transferCacheIndex, std::move(result.transferCache), EncodedFieldCodecType::Topology);
}

inline void RunPolyhedronTopologyEncode(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const std::size_t transferCacheIndex) {
    if (context.adapter == nullptr) {
        ReportTopologyFailure(
            context,
            workspace,
            CodecErrorCode::MissingInput,
            "requires a valid encode adapter for polyhedron topology");
        return;
    }

    polyhedron::PolyhedronTopologyEncodeInput input{
        .data = polyhedron::PolyhedronTopologyData{
            .adapter = *context.adapter,
            .pointInverseOrderSource = workspace.PointInverseOrderSource(),
            .cellOrderSource = workspace.CellOrderSource(),
        },
        .schedule = polyhedron::PolyhedronTopologySchedule{
            .resourceBudget = workspace.ResourceBudget(),
        },
        .cache = polyhedron::PolyhedronTopologyCache{
            .byteStoreSession = workspace.ByteStoreSessionRef(),
        },
    };
    if (context.runRecords.Wants(RunRecordKind::StageTiming)) {
        input.context.phaseTimingCallback =
            [&context](const std::string_view phaseName, const double elapsedMs) {
                context.runRecords.RecordStageTiming(
                    "TopoStage.polyhedron." + std::string(phaseName),
                    elapsedMs,
                    TelemetryStageCategory::Topology);
            };
        input.context.progressCallback =
            MakeTopologyProgressTimingCallback(context, "TopoStage.polyhedron.progress");
    }
    if (context.runRecords.Wants(RunRecordKind::ResourceUsage)) {
        input.context.memoryCheckpoint =
            [&context](const char* name, const std::uint64_t logicalBytes, std::string scope) {
                context.runRecords.RecordResourceUsage(
                    std::string(name),
                    logicalBytes,
                    TelemetryStageCategory::Topology,
                    std::move(scope));
            };
    }

    polyhedron::PolyhedronTopologyEncodeResult result;
    std::string error;
    const auto releaseInputs = [&]() {
        ReleasePolyhedronTopologyInputs(context);
    };
    if (!polyhedron::EncodePolyhedronTopologyToTransferCache(input, result, &error)) {
        releaseInputs();
        ReportTopologyFailure(
            context,
            workspace,
            CodecErrorCode::InvalidTopology,
            "failed to encode polyhedron topology: " + error);
        return;
    }
    releaseInputs();
    if (result.transferCache == nullptr) {
        ReportTopologyFailure(
            context,
            workspace,
            CodecErrorCode::EncodeFailure,
            "failed to publish polyhedron topology transfer cache");
        return;
    }

    workspace.StorageParams().topoParams = result.topo;
    context.runSummary.topologyBytes = static_cast<std::uint64_t>(result.topo.binaryCount);
    workspace.PublishTransferCache(transferCacheIndex, std::move(result.transferCache), EncodedFieldCodecType::Topology);
}

class TopoStage final : public EncodeStage {
public:
    static constexpr std::string_view kTypeName = "TopoStage";

    // 绑定 pipeline 预先准备好的 topology transfer cache
    explicit TopoStage(const std::size_t transferCacheIndex) : m_transferCacheIndex(transferCacheIndex) {}

    const char* Name() const override { return "TopoStage"; }

    // 把拓扑编码结果发布到单个 topology transfer cache
    EncodeStageExecutionStatus Execute(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace) override {
        const bool hasAdapter = context.adapter != nullptr;
        if (!hasAdapter) {
            FailEncodeStage(
                context,
                workspace,
                kTypeName,
                CodecErrorCode::MissingInput,
                "topology encode requires a valid adapter");
            return EncodeStageExecutionStatus::Failed;
        }
        // Pipeline 已显式绑定 Original 或 Computed Order Source
        const bool isPolyhedron = hasAdapter && context.adapter->IsPolyhedronMesh();
        if (context.runRecords.Wants(RunRecordKind::StageTiming)) {
            context.runRecords.RecordStageTiming(
                hasAdapter
                    ? (isPolyhedron ? "TopoStage.path.polyhedron" : "TopoStage.path.connectivity")
                    : "TopoStage.path.missing_adapter",
                0.0,
                TelemetryStageCategory::Topology);
        }
        if (isPolyhedron) {
            RunPolyhedronTopologyEncode(context, workspace, m_transferCacheIndex);
            return context.HasFailure() || workspace.StopRequested()
                ? EncodeStageExecutionStatus::Failed
                : EncodeStageExecutionStatus::Completed;
        }
        RunTopologyEncode(context, workspace, m_transferCacheIndex);
        return context.HasFailure() || workspace.StopRequested()
            ? EncodeStageExecutionStatus::Failed
            : EncodeStageExecutionStatus::Completed;
    }

private:
    std::size_t m_transferCacheIndex{kInvalidTransferCacheIndex};
};

} // namespace datacodec

#endif
