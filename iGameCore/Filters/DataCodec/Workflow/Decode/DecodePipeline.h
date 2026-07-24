#ifndef DATACODEC_WORKFLOW_DECODE_DECODEPIPELINE_H
#define DATACODEC_WORKFLOW_DECODE_DECODEPIPELINE_H

#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"
#include "DataCodec/Runtime/Failure/PipelineFailureManagement.h"
#include "DataCodec/Validation/Policy/CodecValidationPolicy.h"
#include "DataCodec/Runtime/Workspace/DecodeLeafWorkspace.h"
#include "DataCodec/Workflow/Common/PipelineStageNode.h"
#include "DataCodec/Workflow/Decode/Stages/AttrDecodeStage.h"
#include "DataCodec/Workflow/Decode/Stages/DecodeCommitStage.h"
#include "DataCodec/Workflow/Decode/Stages/GeometryDecodeStage.h"
#include "DataCodec/Workflow/Decode/Stages/FieldDecodeInput.h"
#include "DataCodec/Workflow/Decode/Stages/ParamsDecodeStage.h"
#include "DataCodec/Workflow/Decode/Stages/TopoDecodeStage.h"
#include "DataCodec/Log/Telemetry/TelemetryMemoryTrace.h"

#include <cstddef>
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {

using DecodeStageNode = PipelineStageNode<std::unique_ptr<DecodeStage>>;

struct DecodePipelineOptions {
    // 为 false 时强制串行执行
    bool enableParallelStages{true};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
    ResourceBudgetControlParams resourceBudget;
    CodecValidationPolicy validationPolicy;
};

class DecodePipeline {
public:
    explicit DecodePipeline(DecodePipelineOptions options = {}) : m_options(std::move(options)) {}

    [[nodiscard]] static FieldDecodeInput BindFieldInput(
        const DecodeLeafWorkspace& workspace,
        const FieldType type,
        const std::size_t ordinal = 0u) {
        return FieldDecodeInput{
            type,
            workspace.packageFields.FindField(type, ordinal),
        };
    }

    static bool ExecuteParamsAndTopologyStages(
        DecodeContext& context,
        DecodeLeafWorkspace& workspace,
        std::string* error = nullptr) {
        DecodeFailureGuard guard(context, workspace);
        return guard.Run("DecodePipeline", CodecErrorCode::PipelineFailure, [&]() {
            ParamsDecodeStage paramsStage(BindFieldInput(workspace, FieldType::Params));
            ExecuteStage(paramsStage, context, workspace);
            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "decode pipeline stopped while preparing topology reference params");
                return false;
            }
            if (!EnsureAdapterSupportsDecodedParams(context, workspace, error)) {
                return false;
            }

            TopoDecodeStage topologyStage(BindFieldInput(workspace, FieldType::Topology));
            ExecuteStage(topologyStage, context, workspace);
            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "decode pipeline stopped while preparing topology reference");
                return false;
            }
            return true;
        });
    }

    static bool ExecuteParamsAndGeometryStages(
        DecodeContext& context,
        DecodeLeafWorkspace& workspace,
        std::string* error = nullptr) {
        DecodeFailureGuard guard(context, workspace);
        return guard.Run("DecodePipeline", CodecErrorCode::PipelineFailure, [&]() {
            ParamsDecodeStage paramsStage(BindFieldInput(workspace, FieldType::Params));
            ExecuteStage(paramsStage, context, workspace);
            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "decode pipeline stopped while preparing geometry reference params");
                return false;
            }

            GeometryDecodeStage geometryStage(BindFieldInput(workspace, FieldType::Geometry));
            ExecuteStage(geometryStage, context, workspace);
            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "decode pipeline stopped while preparing geometry reference");
                return false;
            }
            return true;
        });
    }

    // 预览固定 decode stage schedule
    std::vector<DecodeStageId> DescribeStageIds(const DecodeContext& context) const {
        DecodeLeafWorkspace workspace;
        workspace.Reset(context.leafPackage);
        std::vector<DecodeStageId> ids;
        ParamsDecodeStage paramsStage(BindFieldInput(workspace, FieldType::Params));
        ids.push_back(paramsStage.Id());
        for (const auto& stageNode : BuildStageSchedule(context, workspace)) {
            ids.push_back(stageNode.stage->Id());
        }
        return ids;
    }

    // 运行一次单帧单块 decode pipeline
    void Execute(DecodeContext& context) const {
        DecodeLeafWorkspace workspace;
        Execute(context, workspace);
    }

    void Execute(DecodeContext& context, DecodeLeafWorkspace& workspace) const {
        DecodeFailureGuard guard(context, workspace);
        guard.Run("DecodePipeline", CodecErrorCode::PipelineFailure, [&]() {
            if (m_options.enableParallelStages && m_options.parallelTaskRunner == nullptr) {
                FailDecodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    "parallel DataCodec decode requires a task runner");
                return false;
            }
            SubmitProgress(context, RunProgressPhase::Begin, 0.0, "开始解码", false);
            PrepareWorkspace(context, workspace, m_options);

            ParamsDecodeStage paramsStage(BindFieldInput(workspace, FieldType::Params));
            ExecuteStage(paramsStage, context, workspace);
            if (context.HasFailure() || workspace.StopRequested()) {
                return true;
            }
            std::string error;
            if (!EnsureAdapterSupportsDecodedParams(context, workspace, &error)) {
                return true;
            }
            if (!PrepareDirectAttributeDecodeStores(context, workspace, &error)) {
                FailDecodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    "failed to prepare direct attribute decode stores: " + error);
                return true;
            }

            auto stageNodes = BuildStageSchedule(context, workspace);
            if (!RunStageSchedule(context, workspace, stageNodes, &error)) {
                FailDecodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    "failed to run stage schedule: " + error);
                return false;
            }
            SubmitProgress(context, RunProgressPhase::Finish, 1.0, "解码完成", true);
            return true;
        });
        RecordRuntimeResourceUsage(context, workspace);
    }

    void ExecuteAttributes(DecodeContext& context, DecodeLeafWorkspace& workspace) const {
        DecodeFailureGuard guard(context, workspace);
        guard.Run("DecodePipeline", CodecErrorCode::PipelineFailure, [&]() {
            if (m_options.enableParallelStages && m_options.parallelTaskRunner == nullptr) {
                FailDecodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    "parallel DataCodec decode requires a task runner");
                return false;
            }
            if (!workspace.MatchesLeafPackage(context.leafPackage)) {
                FailDecodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::InvalidInput,
                    "attribute supplement does not match the prepared leaf package");
                return false;
            }
            const auto targetAttrIndices = ResolveAttributeDecodeIndices(
                context.attributeTargets,
                context.frameIndex,
                context.leafPackage != nullptr ? context.leafPackage->path : BlockPath{},
                context.decodeAllAvailableAttributes,
                workspace.StorageParams().attrParams.size());
            if (targetAttrIndices.empty()) {
                FailDecodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::InvalidInput,
                    "attribute supplement requires at least one attribute target");
                return false;
            }
            const auto requiresDecode = context.attributeRequestMode != AttributeDecodeRequestMode::CommitCached;
            const auto requiresCommit = context.attributeRequestMode != AttributeDecodeRequestMode::DecodeToCache;
            const auto hasDecodeWork = requiresDecode && std::any_of(
                targetAttrIndices.begin(),
                targetAttrIndices.end(),
                [&workspace](const std::size_t attrIndex) {
                    return !workspace.attributes.Complete(attrIndex);
                });
            const auto hasCommitWork = requiresCommit && std::any_of(
                targetAttrIndices.begin(),
                targetAttrIndices.end(),
                [&workspace](const std::size_t attrIndex) {
                    return !workspace.AttributeCommitted(attrIndex);
                });
            if (!hasDecodeWork && !hasCommitWork) {
                SubmitProgress(context, RunProgressPhase::Finish, 1.0, "属性请求已完成", true);
                return true;
            }

            workspace.PrepareSupplementRun(
                m_options.validationPolicy,
                m_options.resourceBudget);
            SubmitProgress(context, RunProgressPhase::Begin, 0.0, "开始处理属性", false);
            if (hasDecodeWork) {
                std::string prepareStoreError;
                if (!PrepareDirectAttributeDecodeStores(context, workspace, &prepareStoreError)) {
                    FailDecodePipeline(
                        context,
                        workspace,
                        CodecErrorCode::PipelineFailure,
                        prepareStoreError.empty()
                            ? "failed to prepare direct attribute decode stores"
                            : prepareStoreError);
                    return false;
                }
                AttrDecodeStage attributeStage(BindFieldInput(workspace, FieldType::Attribute));
                ExecuteStage(attributeStage, context, workspace);
                if (context.HasFailure() || workspace.StopRequested()) {
                    return true;
                }
            }
            if (hasCommitWork) {
                SubmitStageStartProgress(context, DecodeCommitStage::kTypeName);
                CommitAttributeOutput(context, workspace);
                if (context.HasFailure() || workspace.StopRequested()) {
                    return true;
                }
                SubmitStageProgress(context, DecodeCommitStage::kTypeName);
            }
            SubmitProgress(context, RunProgressPhase::Finish, 1.0, "属性处理完成", true);
            return true;
        });
        RecordRuntimeResourceUsage(context, workspace);
    }

private:
    static void RecordRuntimeResourceUsage(
        DecodeContext& context,
        const DecodeLeafWorkspace& workspace) {
        if (!context.runRecords.Wants(RunRecordKind::ResourceUsage)) {
            return;
        }
        const auto storeStats = workspace.ByteStoreSessionRef().SnapshotStats();
        const auto scratchStats = workspace.ScratchBytePool().SnapshotStats();
        const auto windowStats = workspace.CacheResourcesRef().windowBudget.SnapshotStats();
        context.runRecords.RecordResourceUsage("bytestore.logical_bytes", storeStats.logicalBytes);
        context.runRecords.RecordResourceUsage("bytestore.resident_bytes", storeStats.residentBytes);
        context.runRecords.RecordResourceUsage("bytestore.peak_resident_bytes", storeStats.peakResidentBytes);
        context.runRecords.RecordResourceUsage("bytestore.resident_limit_bytes", storeStats.residentLimitBytes);
        context.runRecords.RecordResourceUsage("bytestore.mapped_bytes", storeStats.mappedBytes);
        context.runRecords.RecordResourceUsage("bytestore.managed_file_bytes", storeStats.managedFileBytes);
        context.runRecords.RecordResourceUsage("bytestore.store_count", storeStats.storeCount);
        context.runRecords.RecordResourceUsage("scratch_pool.acquired_bytes", scratchStats.acquiredBytes);
        context.runRecords.RecordResourceUsage(
            "scratch_pool.max_retained_block_count",
            scratchStats.maxRetainedBlockCount);
        context.runRecords.RecordResourceUsage(
            "scratch_pool.max_retained_block_bytes",
            scratchStats.maxRetainedBlockBytes);
        context.runRecords.RecordResourceUsage(
            "scratch_pool.max_retained_total_bytes",
            scratchStats.maxRetainedTotalBytes);
        context.runRecords.RecordResourceUsage("scratch_pool.peak_active_bytes", scratchStats.peakActiveBytes);
        context.runRecords.RecordResourceUsage("scratch_pool.retained_bytes", scratchStats.retainedBytes);
        context.runRecords.RecordResourceUsage("scratch_pool.reused_block_count", scratchStats.reusedBlockCount);
        context.runRecords.RecordResourceUsage("scratch_pool.allocation_count", scratchStats.allocationCount);
        context.runRecords.RecordResourceUsage("window.max_active_bytes", windowStats.maxActiveBytes);
        context.runRecords.RecordResourceUsage("window.peak_active_bytes", windowStats.peakActiveBytes);
        context.runRecords.RecordResourceUsage("window.wait_count", windowStats.waitCount);
        if (context.adapter != nullptr) {
            context.runRecords.RecordResourceUsage(
                "adapter.native_resident_bytes",
                context.adapter->NativeResidentBytesHint());
        }
    }

    static void AddStage(
        std::vector<DecodeStageNode>& stageNodes,
        std::unique_ptr<DecodeStage> stage,
        std::vector<DecodeStageId> dependencies = {}) {
        stageNodes.push_back(DecodeStageNode{std::move(stage), std::move(dependencies)});
    }

    static void PrepareWorkspace(
        DecodeContext& context,
        DecodeLeafWorkspace& workspace,
        const DecodePipelineOptions& options) {
        workspace.Reset(context.leafPackage);
        workspace.SetValidationPolicy(options.validationPolicy);
        workspace.SetResourceBudget(options.resourceBudget);
        workspace.ConfigureCacheResources(
            options.resourceBudget.AccessWindowBytes(),
            options.resourceBudget.ActiveWindowBytes(),
            options.resourceBudget.ScratchRetainedBlockCount(),
            options.resourceBudget.ScratchRetainedBlockBytes(),
            options.resourceBudget.ScratchRetainedTotalBytes(),
            options.resourceBudget.TopologyBufferBudgetBytes(),
            options.resourceBudget.RemapScratchQuotaBytes());
    }

    static std::vector<DecodeStageNode> BuildStageSchedule(
        const DecodeContext& context,
        const DecodeLeafWorkspace& workspace) {
        std::vector<DecodeStageNode> stageNodes;
        auto geometry = std::make_unique<GeometryDecodeStage>(BindFieldInput(workspace, FieldType::Geometry));
        auto topology = std::make_unique<TopoDecodeStage>(BindFieldInput(workspace, FieldType::Topology));
        std::vector<DecodeStageId> commitDeps{
            geometry->Id(),
            topology->Id(),
        };
        AddStage(stageNodes, std::move(geometry));
        AddStage(stageNodes, std::move(topology));
        const auto targetAttrIndices = ResolveAttributeDecodeIndices(
            context.attributeTargets,
            context.frameIndex,
            context.leafPackage != nullptr ? context.leafPackage->path : BlockPath{},
            context.decodeAllAvailableAttributes,
            workspace.StorageParams().attrParams.size());
        if (!targetAttrIndices.empty()) {
            auto attribute = std::make_unique<AttrDecodeStage>(BindFieldInput(workspace, FieldType::Attribute));
            commitDeps.push_back(attribute->Id());
            AddStage(stageNodes, std::move(attribute));
        }
        AddStage(stageNodes, std::make_unique<DecodeCommitStage>(), std::move(commitDeps));
        return stageNodes;
    }

    static bool EnsureAdapterSupportsDecodedParams(
        DecodeContext& context,
        DecodeLeafWorkspace& workspace,
        std::string* error = nullptr) {
        if (!workspace.StorageParams().topoParams.isPolyhedron ||
            context.adapter->SupportsPolyhedronTopology()) {
            return true;
        }
        const std::string message = "decode adapter does not support polyhedron topology";
        FailDecodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
        validation::AssignError(error, message);
        return false;
    }

    static void ExecuteStage(DecodeStage& stage, DecodeContext& context, DecodeLeafWorkspace& workspace) {
        const auto stageName = stage.Id().name;
        const auto collectTiming = context.runRecords.Wants(RunRecordKind::StageTiming);
        SubmitStageStartProgress(context, stageName);
        RecordMemoryTraceStageEvent(context, stageName, true);
        const auto startTime = callback::StartTiming(collectTiming);
        try {
            stage.Execute(context, workspace);
            if (collectTiming) {
                RecordStageTiming(
                    stageName,
                    context,
                    callback::ElapsedMilliseconds(startTime));
            }
            RecordMemoryTraceStageEvent(context, stageName, false);
            SubmitStageProgress(context, stageName);
        } catch (...) {
            RecordMemoryTraceStageEvent(context, stageName, false);
            throw;
        }
    }

    static double DecodeStageProgress(const std::string_view stageName) {
        if (stageName.find("Params") != std::string_view::npos) return 0.10;
        if (stageName.find("Geometry") != std::string_view::npos) return 0.35;
        if (stageName.find("Topo") != std::string_view::npos) return 0.55;
        if (stageName.find("Attr") != std::string_view::npos ||
            stageName.find("Attribute") != std::string_view::npos) return 0.85;
        if (stageName.find("Commit") != std::string_view::npos) return 0.95;
        return 0.0;
    }

    static double DecodeStageStartProgress(const std::string_view stageName) {
        if (stageName.find("Params") != std::string_view::npos) return 0.02;
        if (stageName.find("Geometry") != std::string_view::npos) return 0.15;
        if (stageName.find("Topo") != std::string_view::npos) return 0.35;
        if (stageName.find("Attr") != std::string_view::npos ||
            stageName.find("Attribute") != std::string_view::npos) return 0.60;
        if (stageName.find("Commit") != std::string_view::npos) return 0.90;
        return 0.0;
    }

    static std::string DecodeStageProgressText(const std::string_view stageName) {
        if (stageName.find("Params") != std::string_view::npos) return "解码参数";
        if (stageName.find("Geometry") != std::string_view::npos) return "解码坐标";
        if (stageName.find("Topo") != std::string_view::npos) return "解码拓扑";
        if (stageName.find("Attr") != std::string_view::npos ||
            stageName.find("Attribute") != std::string_view::npos) return "解码数值";
        if (stageName.find("Commit") != std::string_view::npos) return "提交结果";
        return "解码中";
    }

    static void SubmitProgress(
        DecodeContext& context,
        const RunProgressPhase phase,
        const double normalized,
        std::string text,
        const bool success) {
        context.runRecords.SubmitProgress(RunProgressRecord{
            .phase = phase,
            .normalized = normalized,
            .text = std::move(text),
            .success = success,
        });
    }

    static void SubmitStageProgress(DecodeContext& context, const std::string_view stageName) {
        const double normalized = DecodeStageProgress(stageName);
        if (normalized <= 0.0) return;
        SubmitProgress(
            context,
            RunProgressPhase::Update,
            normalized,
            DecodeStageProgressText(stageName),
            false);
    }

    static void SubmitStageStartProgress(DecodeContext& context, const std::string_view stageName) {
        const double normalized = DecodeStageStartProgress(stageName);
        if (normalized <= 0.0) return;
        SubmitProgress(
            context,
            RunProgressPhase::Update,
            normalized,
            DecodeStageProgressText(stageName),
            false);
    }

    bool RunStageSchedule(
        DecodeContext& context,
        DecodeLeafWorkspace& workspace,
        const std::vector<DecodeStageNode>& stageNodes,
        std::string* error = nullptr) const {
        if (stageNodes.empty()) {
            return true;
        }

        if (m_options.enableParallelStages && m_options.parallelTaskRunner == nullptr) {
            const std::string message = "parallel DataCodec decode requires a task runner";
            FailDecodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
            validation::AssignError(error, message);
            return false;
        }

        const auto allowParallelStages = m_options.enableParallelStages &&
            ResolveParallelTaskCount(stageNodes.size(), m_options.parallelTaskRunner) > 1u;
        InlineParallelTaskRunner inlineRunner;
        IParallelTaskRunner* runner = allowParallelStages
            ? m_options.parallelTaskRunner
            : &inlineRunner;

        std::vector<std::vector<std::size_t>> dependents(stageNodes.size());
        std::vector<std::size_t> remainingDependencies(stageNodes.size(), 0u);
        for (std::size_t stageIndex = 0; stageIndex < stageNodes.size(); ++stageIndex) {
            remainingDependencies[stageIndex] = stageNodes[stageIndex].dependencies.size();
            for (const auto& dependencyId : stageNodes[stageIndex].dependencies) {
                const auto dependencyIndex = detail::FindStageIndex(stageNodes, dependencyId);
                if (dependencyIndex == static_cast<std::size_t>(-1)) {
                    const auto message = stageNodes[stageIndex].stage->Describe() +
                        " depends on a missing stage " + dependencyId.ToString();
                    FailDecodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                    validation::AssignError(error, message);
                    return false;
                }
                dependents[dependencyIndex].push_back(stageIndex);
            }
        }

        std::vector<std::uint8_t> completed(stageNodes.size(), 0u);
        std::size_t completedCount = 0u;
        while (completedCount < stageNodes.size()) {
            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "decode pipeline stopped before scheduling the next stage batch");
                return false;
            }
            std::vector<std::size_t> readyStageIndices;
            for (std::size_t stageIndex = 0; stageIndex < stageNodes.size(); ++stageIndex) {
                if (completed[stageIndex] == 0u && remainingDependencies[stageIndex] == 0u) {
                    readyStageIndices.push_back(stageIndex);
                    if (!allowParallelStages) {
                        break;
                    }
                }
            }

            if (readyStageIndices.empty()) {
                const std::string message = "decode pipeline scheduler detected a dependency cycle";
                FailDecodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
                return false;
            }

            for (const auto readyStageIndex : readyStageIndices) {
                completed[readyStageIndex] = 1u;
            }

            try {
                auto taskGroup = runner->CreateGroup(workspace.StopToken());
                if (taskGroup == nullptr) {
                    throw std::runtime_error("decode stage task group is unavailable");
                }
                const auto executeStage = [&context, &workspace, &stageNodes](const std::size_t stageIndex) {
                    if (context.HasFailure() || workspace.StopRequested()) {
                        return;
                    }
                    const auto stageName = stageNodes[stageIndex].stage->Id().name;
                    const auto collectTiming = context.runRecords.Wants(RunRecordKind::StageTiming);
                    SubmitStageStartProgress(context, stageName);
                    RecordMemoryTraceStageEvent(context, stageName, true);
                    const auto startTime = callback::StartTiming(collectTiming);
                    try {
                        stageNodes[stageIndex].stage->Execute(context, workspace);
                        if (collectTiming) {
                            RecordStageTiming(
                                stageName,
                                context,
                                callback::ElapsedMilliseconds(startTime));
                        }
                        RecordMemoryTraceStageEvent(context, stageName, false);
                        SubmitStageProgress(context, stageName);
                    } catch (...) {
                        RecordMemoryTraceStageEvent(context, stageName, false);
                        throw;
                    }
                };
                const auto inlineStageIt = std::find_if(
                    readyStageIndices.begin(),
                    readyStageIndices.end(),
                    [&stageNodes](const std::size_t stageIndex) {
                        return stageNodes[stageIndex].stage->UsesInternalParallelism();
                    });
                const auto inlineStageIndex = inlineStageIt != readyStageIndices.end()
                    ? *inlineStageIt
                    : static_cast<std::size_t>(-1);
                for (const auto readyStageIndex : readyStageIndices) {
                    if (readyStageIndex == inlineStageIndex) {
                        continue;
                    }
                    taskGroup->Submit([&executeStage, readyStageIndex]() {
                        executeStage(readyStageIndex);
                    });
                }
                std::exception_ptr inlineException;
                if (inlineStageIndex != static_cast<std::size_t>(-1)) {
                    // 内部并行阶段留在调用线程，避免同一线程池的嵌套任务被降为单线程
                    try {
                        executeStage(inlineStageIndex);
                    } catch (...) {
                        inlineException = std::current_exception();
                    }
                }
                std::exception_ptr workerException;
                try {
                    taskGroup->Wait();
                } catch (...) {
                    workerException = std::current_exception();
                }
                if (inlineException != nullptr) {
                    std::rethrow_exception(inlineException);
                }
                if (workerException != nullptr) {
                    std::rethrow_exception(workerException);
                }
            } catch (const std::exception& exception) {
                const auto message = std::string("decode stage failed: ") + exception.what();
                FailDecodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
                return false;
            } catch (...) {
                const std::string message = "decode stage failed";
                FailDecodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
                return false;
            }

            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "decode pipeline stopped after a stage batch failed");
                return false;
            }
            for (const auto readyStageIndex : readyStageIndices) {
                ++completedCount;
                for (const auto dependentIndex : dependents[readyStageIndex]) {
                    if (remainingDependencies[dependentIndex] > 0u) {
                        --remainingDependencies[dependentIndex];
                    }
                }
            }
        }
        return true;
    }

    static void RecordStageTiming(
        const std::string_view stageName,
        DecodeContext& context,
        const double elapsedMs) {
        if (!context.runRecords.Wants(RunRecordKind::StageTiming)) {
            return;
        }
        context.runRecords.RecordStageTiming(
            std::string(stageName),
            elapsedMs,
            ResolveStageCategory(stageName));
    }

    static TelemetryStageCategory ResolveStageCategory(const std::string_view stageName) {
        return ResolveTelemetryStageCategory(stageName);
    }

    DecodePipelineOptions m_options;
};

} // namespace datacodec

#endif
