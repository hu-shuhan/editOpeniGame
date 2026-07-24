#ifndef DATACODEC_WORKFLOW_DECODE_STAGES_TOPODECODESTAGE_H
#define DATACODEC_WORKFLOW_DECODE_STAGES_TOPODECODESTAGE_H

#include "DataCodec/Codec/Topology/TopologyDecode.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageFieldDecodeStream.h"
#include "DataCodec/Runtime/Failure/DecodeFailureManagement.h"
#include "DataCodec/Workflow/Decode/Stages/FieldDecodeInput.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
namespace datacodec {

inline void ResetTopologyDecodeState(DecodeLeafWorkspace& workspace) {
    if (!workspace.topologyBorrowed && workspace.topology != nullptr) {
        workspace.topology->Release();
    }
    workspace.topology.reset();
    workspace.topologyBorrowed = false;
}

inline bool BindTopologyReferenceIfAvailable(DecodeContext& context, DecodeLeafWorkspace& workspace) {
    if (context.topologyReferenceStore == nullptr || context.topologyReferenceKey.empty()) {
        return true;
    }
    auto reference = context.topologyReferenceStore->Get(context.topologyReferenceKey);
    if (reference == nullptr || !reference->complete) {
        FailDecodeStage(
            context,
            workspace,
            "TopoDecodeStage",
            CodecErrorCode::InvalidTopology,
            "failed to bind topology reference cache");
        return false;
    }
    workspace.BindTopologyReference(std::move(reference));
    return true;
}

inline bool DecodeTopologyWithoutField(DecodeContext& context, DecodeLeafWorkspace& workspace) {
    if (workspace.StorageParams().topoParams.isStructured) {
        (void)DecodeStructuredTopologyToCache(
            workspace.StorageParams(),
            workspace.MutableTopology());
        return true;
    }
    return BindTopologyReferenceIfAvailable(context, workspace);
}

inline void RecordTopologyTiming(
    DecodeContext& context,
    std::string name,
    const callback::PhaseTimePoint startTime,
    std::string scope = {}) {
    if (!context.runRecords.Wants(RunRecordKind::StageTiming)) {
        return;
    }
    context.runRecords.RecordStageTiming(
        std::move(name),
        callback::ElapsedMilliseconds(startTime),
        TelemetryStageCategory::Topology,
        std::move(scope));
}

// [DC防护:阶段] topology decode 入口校验字段 raw size 和结构化拓扑无载荷契约
inline bool ValidateTopologyFieldSize(
    const TopoStorageParams& params,
    const LeafPackageField& field,
    std::string* error = nullptr) {
    if (static_cast<std::uint64_t>(field.rawSize) != params.binaryCount) {
        return validation::AssignError(error, "topology field raw size does not match params binaryCount");
    }
    if (params.isStructured != 0u && params.binaryCount != 0u) {
        return validation::AssignError(error, "structured topology must not carry topology payload bytes");
    }
    return true;
}

class TopoDecodeStage final : public DecodeStage {
public:
    static constexpr std::string_view kTypeName = "TopoDecodeStage";

    TopoDecodeStage() = default;
    explicit TopoDecodeStage(FieldDecodeInput input) : m_input(input) {}

    const char* Name() const override { return "TopoDecodeStage"; }

    // 把拓扑字节解入 decoded topology cache
    void Execute(DecodeContext& context, DecodeLeafWorkspace& workspace) override {
        ResetTopologyDecodeState(workspace);

        if (!m_input.HasField()) {
            if (workspace.StorageParams().topoParams.binaryCount != 0u) {
                FailDecodeStage(
                    context,
                    workspace,
                    "TopoDecodeStage",
                    CodecErrorCode::MissingInput,
                    "failed to find topology field");
                return;
            }
            DecodeTopologyWithoutField(context, workspace);
            return;
        }

        std::string sizeError;
        if (!ValidateTopologyFieldSize(workspace.StorageParams().topoParams, *m_input.field, &sizeError)) {
            FailDecodeStage(
                context,
                workspace,
                "TopoDecodeStage",
                CodecErrorCode::DecodeFailure,
                "failed to validate topology field size: " + sizeError);
            return;
        }

        decodefield::FieldDecodeStreamReader reader;
        std::string error;
        if (!decodefield::OpenLeafPackageFieldDecodeStream(
                *m_input.field,
                workspace.CacheResourcesRef(),
                reader,
                &error)) {
            FailDecodeStage(
                context,
                workspace,
                "TopoDecodeStage",
                CodecErrorCode::PipelineFailure,
                "failed to open topology field: " + error);
            return;
        }
        decodefield::FieldDecodeByteStream stream(reader);
        const auto collectTiming = context.runRecords.Wants(RunRecordKind::StageTiming);
        const auto decodeStart = callback::StartTiming(collectTiming);
        TopologyDecodeTimingCallback topologyTiming;
        if (collectTiming) {
            topologyTiming = [&context](const TopologyDecodeTimingEvent& event) {
                context.runRecords.RecordStageTiming(
                    event.name,
                    event.elapsedMs,
                    TelemetryStageCategory::Topology,
                    event.scope);
            };
        }
        const bool retainAsReference =
            context.topologyReferenceStore != nullptr && !context.topologyReferenceKey.empty();
        const auto topologyCacheStorageMode = retainAsReference
            ? workspace.ResourceBudget().TopologyDecodeReferenceCacheStorageMode()
            : workspace.ResourceBudget().TopologyDecodeCacheStorageMode();
        const auto topologyMemoryCacheLimitBytes = retainAsReference
            ? workspace.ResourceBudget().TopologyDecodeMemoryReferenceLimitBytes()
            : workspace.ResourceBudget().TopologyDecodeMemoryCacheLimitBytes();
        TopologyDecodeRuntime decodeRuntime{
            .data = TopologyDecodeData{
                .storageParams = workspace.StorageParams(),
            },
            .cache = TopologyDecodeCache{
                .cacheResources = workspace.CacheResourcesRef(),
                .byteStoreSession = workspace.ByteStoreSessionRef(),
                .topology = workspace.MutableTopology(),
                .topologyMemoryInputLimitBytes =
                    workspace.ResourceBudget().TopologyDecodeMemoryInputLimitBytes(),
                .topologyMemoryCacheLimitBytes = topologyMemoryCacheLimitBytes,
                .topologyInputStorageMode = workspace.ResourceBudget().TopologyDecodeInputStorageMode(),
                .topologyCacheStorageMode = topologyCacheStorageMode,
            },
            .context = TopologyDecodeContext{
                .timingCallback = std::move(topologyTiming),
                .topologyBlockObserver = context.topologyBlockObserver,
            },
            .schedule = TopologyDecodeSchedule{
                .parallelTaskRunner = context.parallelTaskRunner,
                .workerCount = workspace.ResourceBudget().TopologyBlockLaneCount(),
            },
        };
        const auto result = DecodeTopologyFieldToCache(
            decodeRuntime,
            stream);
        const auto* topology = workspace.TopologyForCommit();
        std::string topologyScope;
        if (topology != nullptr) {
            const auto& topoParams = workspace.StorageParams().topoParams;
            topologyScope = std::string("input=") + topology->InputByteStoreModeName() +
                " decoded=" + topology->ByteStoreModeName() +
                " fixedCellSize=" + std::to_string(topoParams.fixedCellSize) +
                " hasCellTypes=" + std::to_string(static_cast<int>(topoParams.hasCellTypes)) +
                " orderBytes=" + std::to_string(topoParams.connectivityLayout.cellPolynomialOrderByteCount);
        }
        RecordTopologyTiming(
            context,
            "TopoDecodeCoreStage",
            decodeStart,
            std::move(topologyScope));
        if (!result) {
            FailDecodeStage(
                context,
                workspace,
                "TopoDecodeStage",
                result.code,
                result.message);
        }
    }

private:
    FieldDecodeInput m_input;
};

} // namespace datacodec

#endif
