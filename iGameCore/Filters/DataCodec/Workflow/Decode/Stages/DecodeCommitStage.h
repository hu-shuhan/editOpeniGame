#ifndef DATACODEC_WORKFLOW_DECODE_STAGES_DECODECOMMITSTAGE_H
#define DATACODEC_WORKFLOW_DECODE_STAGES_DECODECOMMITSTAGE_H

#include "DataCodec/Runtime/Cache/DecodedCacheCommit.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Validation/Workflow/DecodeOutputValidator.h"
#include "DataCodec/Validation/Workflow/DecodeValidationLifecycle.h"
#include "DataCodec/Runtime/Context/DecodeContext.h"
#include "DataCodec/Runtime/Failure/DecodeFailureManagement.h"
#include "DataCodec/Runtime/Workspace/DecodeLeafWorkspace.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace datacodec {

// [DC防护:结果] strict decode 在 commit 前复核 cache 形状和 storage params 一致
inline bool ValidateDecodedGeometryCacheShape(
    const CodecStorageParams& params,
    const DecodedGeometryCache& geometry,
    std::string* error = nullptr) {
    const auto result = validation::GeometryValidator::ValidateDecodedCacheShape(
        params,
        geometry);
    return result || validation::AssignError(error, result.message);
}

inline bool ValidateDecodedAttributeCacheShape(
    const CodecStorageParams& params,
    const DecodedAttributeCacheSet& attributes,
    const std::span<const std::size_t> attrIndices,
    std::string* error = nullptr) {
    const auto result = validation::AttributeValidator::ValidateDecodedCacheShape(
        params,
        attributes,
        attrIndices);
    return result || validation::AssignError(error, result.message);
}

inline bool ValidateDecodedTopologyCacheShape(
    const CodecStorageParams& params,
    const std::shared_ptr<DecodedTopologyCache>& topology,
    std::string* error = nullptr) {
    const auto result = validation::TopologyValidator::ValidateDecodedCacheShape(
        params,
        topology);
    return result || validation::AssignError(error, result.message);
}

inline bool ValidateDecodedCacheShapesIfStrict(
    const DecodeLeafWorkspace& workspace,
    const std::span<const std::size_t> attrIndices,
    std::string* error = nullptr) {
    const auto result = validation::DecodeOutputValidator::ValidateDecodedCacheShapes(
        workspace,
        attrIndices);
    return result || validation::AssignError(error, result.message);
}

inline bool ValidateDecodedAttributeTargetsIfStrict(
    const DecodeLeafWorkspace& workspace,
    const std::span<const std::size_t> attrIndices,
    std::string* error = nullptr) {
    const auto result = validation::DecodeOutputValidator::ValidateDecodedAttributes(
        workspace,
        attrIndices);
    return result || validation::AssignError(error, result.message);
}

inline void RecordCommitTiming(
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
        TelemetryStageCategory::General,
        std::move(scope));
}

inline void SubmitCommitProgress(
    DecodeContext& context,
    const double normalized,
    std::string text) {
    context.runRecords.SubmitProgress(RunProgressRecord{
        .phase = RunProgressPhase::Update,
        .normalized = normalized,
        .text = std::move(text),
        .success = false,
    });
}

inline bool CommitDecodedTopologyIfPresent(
    DecodeContext& context,
    DecodeLeafWorkspace& workspace,
    std::string* error = nullptr) {
    if (workspace.topology == nullptr || !workspace.topology->complete) {
        return true;
    }
    if (context.topologyOutputMode == TopologyDecodeOutputMode::ObserverOnly) {
        if (!workspace.topologyBorrowed) {
            workspace.topology->Release();
        }
        return true;
    }
    const bool retainAsTopologyReference = !workspace.topologyBorrowed &&
        context.topologyReferenceStore != nullptr &&
        !context.topologyReferenceKey.empty();
    if (retainAsTopologyReference) {
        context.topologyReferenceStore->Put(context.topologyReferenceKey, workspace.topology);
    }
    if (workspace.topologyBorrowed || retainAsTopologyReference) {
        return CommitTopologyCache(
            *context.adapter,
            workspace.CacheResourcesRef(),
            *workspace.topology,
            error);
    }
    if (!CommitTopologyCacheAndRelease(
            *context.adapter,
            workspace.CacheResourcesRef(),
            *workspace.topology,
            error)) {
        return false;
    }
    if (workspace.topology != nullptr) {
        workspace.topology->Release();
    }
    return true;
}

inline std::vector<std::size_t> CollectUncommittedAttributeIndices(
    const DecodeLeafWorkspace& workspace,
    const std::span<const std::size_t> attrIndices) {
    std::vector<std::size_t> output;
    output.reserve(attrIndices.size());
    for (const auto attrIndex : attrIndices) {
        if (!workspace.AttributeCommitted(attrIndex)) {
            output.push_back(attrIndex);
        }
    }
    return output;
}

inline void CommitOutput(DecodeContext& context, DecodeLeafWorkspace& workspace) {
    std::string error;
    const auto targetAttrIndices = ResolveAttributeDecodeIndices(
        context.attributeTargets,
        context.frameIndex,
        context.leafPackage != nullptr ? context.leafPackage->path : BlockPath{},
        context.attributeSelection,
        workspace.StorageParams().attrParams.size());
    const auto uncommittedAttrIndices = CollectUncommittedAttributeIndices(workspace, targetAttrIndices);
    auto stageStart = callback::StartTiming(context.runRecords.Wants(RunRecordKind::StageTiming));
    SubmitCommitProgress(context, 0.905, "校验提交结果");
    if (!ValidateDecodedCacheShapesIfStrict(workspace, targetAttrIndices, &error)) {
        RecordCommitTiming(context, "DecodeCommitStage.Validate", stageStart);
        FailDecodeStage(
            context,
            workspace,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::OutputInvariant)),
            CodecErrorCode::DecodeFailure,
            "failed strict decode validation: " + error);
        return;
    }
    RecordCommitTiming(context, "DecodeCommitStage.Validate", stageStart);
    error.clear();
    stageStart = callback::StartTiming(context.runRecords.Wants(RunRecordKind::StageTiming));
    SubmitCommitProgress(context, 0.920, "提交坐标");
    if (!CommitGeometryCache(
            *context.adapter,
            workspace.CacheResourcesRef(),
            workspace.geometry,
            &error)) {
        RecordCommitTiming(context, "DecodeCommitStage.Geometry", stageStart);
        FailDecodeStage(
            context,
            workspace,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::Commit)),
            CodecErrorCode::PipelineFailure,
            "failed to commit decoded geometry cache: " + error);
        return;
    }
    RecordCommitTiming(context, "DecodeCommitStage.Geometry", stageStart);
    error.clear();
    stageStart = callback::StartTiming(context.runRecords.Wants(RunRecordKind::StageTiming));
    SubmitCommitProgress(context, 0.940, "提交拓扑");
    const auto* topologyBeforeCommit = workspace.TopologyForCommit();
    const std::string topologyCommitMode =
        topologyBeforeCommit != nullptr ? topologyBeforeCommit->ByteStoreModeName() : std::string{};
    if (!CommitDecodedTopologyIfPresent(context, workspace, &error)) {
        RecordCommitTiming(
            context,
            "DecodeCommitStage.Topology",
            stageStart,
            topologyCommitMode);
        FailDecodeStage(
            context,
            workspace,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::Commit)),
            CodecErrorCode::PipelineFailure,
            "failed to commit decoded topology cache: " + error);
        return;
    }
    RecordCommitTiming(
        context,
        "DecodeCommitStage.Topology",
        stageStart,
        topologyCommitMode);
    error.clear();
    stageStart = callback::StartTiming(context.runRecords.Wants(RunRecordKind::StageTiming));
    SubmitCommitProgress(context, 0.970, "提交数值");
    if (!CommitAttributeCacheFields(
            *context.adapter,
            workspace.CacheResourcesRef(),
            workspace.attributes,
            uncommittedAttrIndices,
            &error,
            context.parallelTaskRunner,
            workspace.ResourceBudget().AttributeCommitLaneCount(),
            workspace.StopToken())) {
        RecordCommitTiming(context, "DecodeCommitStage.Attributes", stageStart);
        FailDecodeStage(
            context,
            workspace,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::Commit)),
            CodecErrorCode::PipelineFailure,
            "failed to commit decoded attribute caches: " + error);
        return;
    }
    workspace.MarkAttributesCommitted(uncommittedAttrIndices);
    RecordCommitTiming(context, "DecodeCommitStage.Attributes", stageStart);
    SubmitCommitProgress(context, 0.990, "整理结果");
}

inline void CommitAttributeOutput(DecodeContext& context, DecodeLeafWorkspace& workspace) {
    const auto targetAttrIndices = ResolveAttributeDecodeIndices(
        context.attributeTargets,
        context.frameIndex,
        context.leafPackage != nullptr ? context.leafPackage->path : BlockPath{},
        context.attributeSelection,
        workspace.StorageParams().attrParams.size());
    const auto uncommittedAttrIndices = CollectUncommittedAttributeIndices(workspace, targetAttrIndices);
    std::string error;
    auto stageStart = callback::StartTiming(context.runRecords.Wants(RunRecordKind::StageTiming));
    if (!ValidateDecodedAttributeTargetsIfStrict(workspace, uncommittedAttrIndices, &error)) {
        RecordCommitTiming(context, "DecodeCommitStage.ValidateAttributes", stageStart);
        FailDecodeStage(
            context,
            workspace,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::OutputInvariant)),
            CodecErrorCode::DecodeFailure,
            "failed strict attribute decode validation: " + error);
        return;
    }
    RecordCommitTiming(context, "DecodeCommitStage.ValidateAttributes", stageStart);
    error.clear();
    stageStart = callback::StartTiming(context.runRecords.Wants(RunRecordKind::StageTiming));
    if (!CommitAttributeCacheFields(
            *context.adapter,
            workspace.CacheResourcesRef(),
            workspace.attributes,
            uncommittedAttrIndices,
            &error,
            context.parallelTaskRunner,
            workspace.ResourceBudget().AttributeCommitLaneCount(),
            workspace.StopToken())) {
        RecordCommitTiming(context, "DecodeCommitStage.Attributes", stageStart);
        FailDecodeStage(
            context,
            workspace,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::Commit)),
            CodecErrorCode::PipelineFailure,
            "failed to commit decoded attributes: " + error);
        return;
    }
    workspace.MarkAttributesCommitted(uncommittedAttrIndices);
    RecordCommitTiming(context, "DecodeCommitStage.Attributes", stageStart);
}

class DecodeCommitStage final : public DecodeStage {
public:
    static constexpr std::string_view kTypeName = "DecodeCommitStage";

    const char* Name() const override { return "DecodeCommitStage"; }
    [[nodiscard]] bool UsesInternalParallelism() const noexcept override { return true; }

    // 按固定顺序把 decoded cache 发布给 decode adapter
    void Execute(DecodeContext& context, DecodeLeafWorkspace& workspace) override {
        CommitOutput(context, workspace);
    }
};

} // namespace datacodec

#endif
