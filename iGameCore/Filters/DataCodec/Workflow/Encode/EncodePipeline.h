#ifndef DATACODEC_WORKFLOW_ENCODE_ENCODEPIPELINE_H
#define DATACODEC_WORKFLOW_ENCODE_ENCODEPIPELINE_H

#include "DataCodec/API/Params/CodecParamFactories.h"
#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"
#include "DataCodec/Storage/ByteIO/Window/WindowedCopy.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Storage/ByteStore/SegmentedBinaryObject.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageByteWriter.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageFieldEncode.h"
#include "DataCodec/Storage/LeafPackage/EncodedLeafFieldBundle.h"
#include "DataCodec/Workflow/Leaf/EncodeOutputWriter.h"
#include "DataCodec/Runtime/Failure/PipelineFailureManagement.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"
#include "DataCodec/Workflow/Common/PipelineStageNode.h"
#include "DataCodec/Workflow/Encode/EncodePipelineBinding.h"
#include "DataCodec/Workflow/Encode/Stages/AttributeStage.h"
#include "DataCodec/Workflow/Encode/Stages/CellRemapStage.h"
#include "DataCodec/Workflow/Encode/Stages/GeometryStage.h"
#include "DataCodec/Workflow/Encode/Stages/ParamsEncodeStage.h"
#include "DataCodec/Workflow/Encode/Stages/PointRemapStage.h"
#include "DataCodec/Workflow/Encode/Stages/TopoStage.h"
#include "DataCodec/Log/Telemetry/TelemetryMemoryTrace.h"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

using EncodeStageNode = PipelineStageNode<std::unique_ptr<EncodeStage>>;
using EncodeAfterStageCallback = std::function<void(const EncodeStageId&)>;

struct EncodePipelineRunOptions {
    bool enableParallelStages{true};
    int packageFieldZstdLevel{3};
    std::size_t transferCacheWorkerCount{1u};
    PackageFieldEncodingMode packageFieldEncoding{PackageFieldEncodingMode::Zstd};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
    std::size_t accessWindowBytes{kDefaultEncodeAccessWindowBytes};
    std::uint64_t activeWindowBytes{kDefaultEncodeActiveWindowBytes};
};

namespace detail {

inline const char* EncodeStorageModeName(const EncodeStorageMode mode) noexcept {
    switch (mode) {
        case EncodeStorageMode::Memory:
            return "memory";
        case EncodeStorageMode::Managed:
        default:
            return "managed";
    }
}

} // namespace detail

struct EncodePipelineOptions {
    std::optional<EncodePipelineBinding> binding;
};

struct EncodePipelineResult {
    bool success{false};
    bool hasEncodedOutput{false};
    std::vector<std::uint8_t> encodedBytes;
    std::uint64_t encodedByteCount{0u};
    std::vector<EncodeStageExecutionRecord> stageExecutions;
};

struct EncodeStageDescription {
    EncodeStageId stageId;
    std::vector<EncodeStageId> dependencies;
};

class EncodePipeline {
public:
    explicit EncodePipeline(EncodePipelineOptions options = {}) : m_options(std::move(options)) {}

    // 预览 transfer cache 布局和 repeat 展开之后的具体 encode DAG
    std::vector<EncodeStageId> DescribeStageIds(EncodeContext& context) const {
        const auto descriptions = DescribeStageGraph(context);
        std::vector<EncodeStageId> stageIds;
        stageIds.reserve(descriptions.size() + 8u);
        for (const auto& description : descriptions) {
            stageIds.push_back(description.stageId);
        }
        if (context.HasFailure() || !m_options.binding.has_value()) {
            return {};
        }
        AppendResolvedReferenceStageIds(context, stageIds);
        AppendResolvedPackageStageIds(
            m_options.binding->descriptor,
            stageIds);
        return stageIds;
    }

    // 返回实际解析后的 DAG 及其显式依赖边
    std::vector<EncodeStageDescription> DescribeStageGraph(EncodeContext& context) const {
        if (context.adapter == nullptr) {
            context.RecordFailure(
                "EncodePipeline",
                CodecErrorCode::MissingInput,
                "EncodePipeline requires a valid encode adapter.");
            return {};
        }
        EncodeLeafWorkspace workspace;
        EncodeFailureGuard guard(context, workspace);
        if (!guard.Run("EncodePipeline", CodecErrorCode::PipelineFailure, [&]() {
                PrepareDataflow(context, workspace, false);
                return !context.HasFailure();
            })) {
            return {};
        }
        std::vector<EncodeStageNode> stageNodes;
        if (!guard.Run("EncodePipeline", CodecErrorCode::PipelineFailure, [&]() {
                stageNodes = BuildStageSchedule(context, workspace);
                return !context.HasFailure();
            })) {
            return {};
        }
        std::vector<EncodeStageDescription> descriptions;
        descriptions.reserve(stageNodes.size());
        for (const auto& stageNode : stageNodes) {
            descriptions.push_back(EncodeStageDescription{
                .stageId = stageNode.stage->Id(),
                .dependencies = stageNode.dependencies,
            });
        }
        return descriptions;
    }

    // 运行一次单帧单块 encode pipeline 并返回编码输出字节
    EncodePipelineResult Execute(EncodeContext& context) const {
        MemoryByteRangeOutput sink;
        auto result = ExecuteInternal(context, &sink);
        if (result.hasEncodedOutput && result.encodedBytes.empty() && result.encodedByteCount != 0u) {
            result.encodedBytes = sink.TakeBytes();
            result.encodedByteCount = static_cast<std::uint64_t>(result.encodedBytes.size());
        }
        return result;
    }

    // 运行一次单帧单块 encode pipeline 并直接写入外部 sink
    EncodePipelineResult ExecuteToSink(EncodeContext& context, IByteRangeOutput& sink) const {
        return ExecuteInternal(context, &sink, nullptr);
    }

    // 运行一次单帧单块 encode pipeline 并生成编码字段 bundle
    EncodePipelineResult ExecuteToFieldBundle(
        EncodeContext& context,
        EncodedLeafFieldBundle& output) const {
        return ExecuteInternal(context, nullptr, &output);
    }

private:
    static void AppendIntraReferenceStageId(
        const IntraFieldReferenceControlParams& control,
        std::vector<EncodeStageId>& stageIds) {
        switch (control.codec) {
            case IntraFieldReferenceCodec::Affine:
                stageIds.push_back(MakeEncodeStageId(
                    control.selectionMode == ReferenceSelectionMode::Forced
                        ? "ReferenceEncode.AttributeIntra.AffineSpatialBlock.Forced"
                        : "ReferenceEncode.AttributeIntra.AffineSpatialBlock.Auto"));
                return;
            case IntraFieldReferenceCodec::Wavelet:
                stageIds.push_back(MakeEncodeStageId(
                    control.selectionMode == ReferenceSelectionMode::Forced
                        ? "ReferenceEncode.AttributeIntra.WaveletSpatialBlock.Forced"
                        : "ReferenceEncode.AttributeIntra.WaveletSpatialBlock.Auto"));
                return;
            case IntraFieldReferenceCodec::Predictor:
                stageIds.push_back(MakeEncodeStageId(
                    control.selectionMode == ReferenceSelectionMode::Forced
                        ? "ReferenceEncode.AttributeIntra.PredictorSpatialBlock.Forced"
                        : "ReferenceEncode.AttributeIntra.PredictorSpatialBlock.Auto"));
                return;
            case IntraFieldReferenceCodec::Disabled:
            default:
                return;
        }
    }

    static void AppendTemporalReferenceStageId(
        const std::string_view domain,
        const TemporalFieldReferenceControlParams& control,
        std::vector<EncodeStageId>& stageIds) {
        if (domain == "Attribute") {
            switch (control.codec) {
                case TemporalFieldReferenceCodec::Wavelet:
                    stageIds.push_back(MakeEncodeStageId(
                        control.selectionMode == ReferenceSelectionMode::Forced
                            ? "ReferenceEncode.AttributeTemporal.WaveletSpatialBlock.Forced"
                            : "ReferenceEncode.AttributeTemporal.WaveletSpatialBlock.Auto"));
                    return;
                case TemporalFieldReferenceCodec::Predictor:
                    stageIds.push_back(MakeEncodeStageId(
                        control.selectionMode == ReferenceSelectionMode::Forced
                            ? "ReferenceEncode.AttributeTemporal.PredictorSpatialBlock.Forced"
                            : "ReferenceEncode.AttributeTemporal.PredictorSpatialBlock.Auto"));
                    return;
                case TemporalFieldReferenceCodec::Disabled:
                default:
                    return;
            }
        }
        switch (control.codec) {
            case TemporalFieldReferenceCodec::Wavelet:
                stageIds.push_back(MakeEncodeStageId(
                    control.selectionMode == ReferenceSelectionMode::Forced
                        ? "ReferenceEncode.GeometryTemporal.WaveletSpatialBlock.Forced"
                        : "ReferenceEncode.GeometryTemporal.WaveletSpatialBlock.Auto"));
                return;
            case TemporalFieldReferenceCodec::Predictor:
                stageIds.push_back(MakeEncodeStageId(
                    control.selectionMode == ReferenceSelectionMode::Forced
                        ? "ReferenceEncode.GeometryTemporal.PredictorSpatialBlock.Forced"
                        : "ReferenceEncode.GeometryTemporal.PredictorSpatialBlock.Auto"));
                return;
            case TemporalFieldReferenceCodec::Disabled:
            default:
                return;
        }
    }

    static void AppendResolvedReferenceStageIds(
        const EncodeContext& context,
        std::vector<EncodeStageId>& stageIds) {
        if (context.controlParams == nullptr) {
            return;
        }
        const auto& params = *context.controlParams;
        if (params.attrReference.enabled && !context.attributeTargets.empty()) {
            if (context.attributeTemporalRole == TemporalFieldRole::SingleFrame) {
                AppendIntraReferenceStageId(params.attrReference.intraField, stageIds);
            }
            if (context.attributeTemporalRole == TemporalFieldRole::PredFrame) {
                AppendTemporalReferenceStageId(
                    "Attribute",
                    params.attrReference.temporalField,
                    stageIds);
            }
        }
        if (params.geometryReference.enabled &&
            context.geometryTemporalRole == TemporalFieldRole::PredFrame) {
            AppendTemporalReferenceStageId(
                "Geometry",
                params.geometryReference.temporalField,
                stageIds);
        }
    }

    static void AppendResolvedPackageStageIds(
        const EncodePipelineDescriptor& descriptor,
        std::vector<EncodeStageId>& stageIds) {
        stageIds.push_back(MakeEncodeStageId(
            descriptor.packageFields.mode == PackageFieldEncodingMode::Zstd
                ? "PackageFieldZstd.Streaming"
                : "PackageFieldRaw.Streaming"));
        stageIds.push_back(MakeEncodeStageId(
            descriptor.outputKind == EncodePipelineOutputKind::EncodedLeafFieldBundle
                ? "PackageAssembly.EncodedLeafFieldBundle"
                : "PackageAssembly.LeafPackage"));
    }

    static const char* ActualReferenceStageName(
        const NumericArrayReferenceCodecId codecId) noexcept {
        switch (codecId) {
            case NumericArrayReferenceCodecId::Affine:
                return "ReferenceEncode.AffineSpatialBlock";
            case NumericArrayReferenceCodecId::Wavelet:
                return "ReferenceEncode.WaveletSpatialBlock";
            case NumericArrayReferenceCodecId::Predictor:
                return "ReferenceEncode.PredictorSpatialBlock";
            case NumericArrayReferenceCodecId::NonReference:
            default:
                return nullptr;
        }
    }

    static void AppendActualReferenceExecutionRecords(
        const EncodeLeafWorkspace& workspace,
        EncodeContext& context,
        std::vector<EncodeStageExecutionRecord>& stageExecutions) {
        bool recordedAffine = false;
        bool recordedWavelet = false;
        bool recordedPredictor = false;
        const auto appendLayouts = [&](const std::vector<NumericArrayBlockLayoutParams>& layouts) {
            for (const auto& layout : layouts) {
                bool* recorded = nullptr;
                const auto codecId = NumericArrayBlockModeCodecId(layout.mode);
                switch (codecId) {
                    case NumericArrayReferenceCodecId::Affine:
                        recorded = &recordedAffine;
                        break;
                    case NumericArrayReferenceCodecId::Wavelet:
                        recorded = &recordedWavelet;
                        break;
                    case NumericArrayReferenceCodecId::Predictor:
                        recorded = &recordedPredictor;
                        break;
                    case NumericArrayReferenceCodecId::NonReference:
                    default:
                        continue;
                }
                if (*recorded) {
                    continue;
                }
                const auto* stageName = ActualReferenceStageName(codecId);
                if (stageName == nullptr) {
                    continue;
                }
                *recorded = true;
                stageExecutions.push_back(EncodeStageExecutionRecord{
                    .stageId = MakeEncodeStageId(stageName),
                    .status = EncodeStageExecutionStatus::Completed,
                });
                context.AddInfo(stageName, "stage result=Completed");
            }
        };
        appendLayouts(workspace.StorageParams().geomParams.blockLayouts);
        for (const auto& attr : workspace.StorageParams().attrParams) {
            appendLayouts(attr.blockLayouts);
        }
    }

    EncodePipelineResult ExecuteInternal(
        EncodeContext& context,
        IByteRangeOutput* outputSink,
        EncodedLeafFieldBundle* fieldBundleOutput = nullptr) const {
        EncodePipelineResult result;
        if (context.adapter == nullptr) {
            context.RecordFailure(
                "EncodePipeline",
                CodecErrorCode::MissingInput,
                "EncodePipeline requires a valid encode adapter.");
            return result;
        }
        EncodeLeafWorkspace workspace;
        EncodeFailureGuard guard(context, workspace);
        if (!guard.Run("EncodePipeline", CodecErrorCode::PipelineFailure, [&]() {
                PrepareDataflow(context, workspace);
                return !context.HasFailure();
            })) {
            return result;
        }
        std::vector<EncodeStageNode> stageNodes;
        if (!guard.Run("EncodePipeline", CodecErrorCode::PipelineFailure, [&]() {
                stageNodes = BuildStageSchedule(context, workspace);
                return !context.HasFailure();
            })) {
            return result;
        }
        auto releaseTracker = BuildResourceReleaseTracker(stageNodes);

        EncodePipelineRunOptions runOptions;
        const auto& binding = *m_options.binding;
        runOptions.enableParallelStages = binding.executionProfile.enableParallelStages;
        runOptions.parallelTaskRunner = binding.executionProfile.parallelTaskRunner;
        runOptions.packageFieldEncoding = binding.descriptor.packageFields.mode;
        runOptions.packageFieldZstdLevel = binding.descriptor.packageFields.zstdLevel;
        runOptions.accessWindowBytes = workspace.CacheResourcesRef().accessWindowBytes;
        runOptions.activeWindowBytes = workspace.CacheResourcesRef().activeWindowBytes;
        runOptions.transferCacheWorkerCount = binding.descriptor.packageFields.workerCount;

        std::string runtimeError;
        std::uint64_t encodedByteCount = 0u;
        std::vector<EncodeStageExecutionRecord> stageExecutions;
        const auto runtimeOk = guard.Run("EncodePipeline", CodecErrorCode::PipelineFailure, [&]() {
            return RunStageScheduleAndWriteEncodedOutput(
                context,
                workspace,
                std::move(stageNodes),
                runOptions,
                outputSink,
                fieldBundleOutput,
                [&workspace, &releaseTracker, &context](
                    const EncodeStageId& stageId) {
                    ApplyAfterStageLifecycleRelease(context, workspace, releaseTracker, stageId);
                },
                &stageExecutions,
                &encodedByteCount,
                &runtimeError);
        });
        result.stageExecutions = std::move(stageExecutions);
        if (!runtimeOk) {
            if (!context.HasFailure()) {
                FailEncodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    runtimeError.empty() ? "EncodePipeline runtime failed" : runtimeError);
            }
            return result;
        }
        AppendActualReferenceExecutionRecords(
            workspace,
            context,
            result.stageExecutions);
        if (m_options.binding.has_value()) {
            const auto& descriptor = m_options.binding->descriptor;
            if (outputSink != nullptr) {
                const auto* packageFieldStage =
                    descriptor.packageFields.mode == PackageFieldEncodingMode::Zstd
                        ? "PackageFieldZstd.Streaming"
                        : "PackageFieldRaw.Streaming";
                result.stageExecutions.push_back(EncodeStageExecutionRecord{
                    .stageId = MakeEncodeStageId(packageFieldStage),
                    .status = EncodeStageExecutionStatus::Completed,
                });
                context.AddInfo(packageFieldStage, "stage result=Completed");
            }
            const auto* assemblyName = fieldBundleOutput != nullptr
                ? "PackageAssembly.EncodedLeafFieldBundle"
                : "PackageAssembly.LeafPackage";
            result.stageExecutions.push_back(EncodeStageExecutionRecord{
                .stageId = MakeEncodeStageId(assemblyName),
                .status = EncodeStageExecutionStatus::Completed,
            });
            context.AddInfo(assemblyName, "stage result=Completed");
        }
        RecordAttributeRuntimeResourceUsage(context, workspace);
        RecordStoreRuntimeResourceUsage(context, workspace);
        result.success = true;
        if (!context.HasFailure()) {
            result.hasEncodedOutput = true;
            result.encodedByteCount = encodedByteCount;
        }
        return result;
    }

    bool ResolvePipelineDescriptor(
        EncodeContext& context,
        EncodePipelineDescriptor& descriptor) const {
        if (context.adapter == nullptr) {
            context.RecordFailure(
                "EncodePipelineBinding",
                CodecErrorCode::MissingInput,
                "encode pipeline definition requires a valid adapter");
            return false;
        }
        if (!m_options.binding.has_value()) {
            context.RecordFailure(
                "EncodePipelineBinding",
                CodecErrorCode::InvalidInput,
                "encode entry must resolve the pipeline definition before execution");
            return false;
        }
        const auto& binding = *m_options.binding;
        descriptor = binding.descriptor;
        return true;
    }

    [[nodiscard]] static std::uint64_t EstimateAttributeStageCost(
        const EncodeLeafWorkspace& workspace,
        const AttrAttachment attachment,
        const std::size_t localAttrIndex) noexcept {
        const auto metaIndex = attachment == AttrAttachment::Point
            ? workspace.PointAttrMetaIndex(localAttrIndex)
            : workspace.CellAttrMetaIndex(localAttrIndex);
        const auto& params = workspace.StorageParams().attrParams;
        if (metaIndex >= params.size()) {
            return 0u;
        }
        const auto& meta = params[metaIndex];
        const auto componentCount = meta.dimension > 0
            ? static_cast<std::uint64_t>(meta.dimension)
            : 0u;
        const auto tupleBytes = validation::SaturatingMulU64(
            componentCount,
            static_cast<std::uint64_t>(NumericArrayValueSize(meta)));
        return validation::SaturatingMulU64(
            static_cast<std::uint64_t>(meta.elementCount),
            tupleBytes);
    }

    [[nodiscard]] static std::vector<std::size_t> BuildAttributeSchedulingOrder(
        const EncodeLeafWorkspace& workspace,
        const AttrAttachment attachment) {
        const auto attrCount = attachment == AttrAttachment::Point
            ? workspace.TransferCacheLayout().pointAttrs.size()
            : workspace.TransferCacheLayout().cellAttrs.size();
        std::vector<std::size_t> order;
        order.reserve(attrCount);
        for (std::size_t attrIndex = 0u; attrIndex < attrCount; ++attrIndex) {
            order.push_back(attrIndex);
        }
        std::stable_sort(
            order.begin(),
            order.end(),
            [&workspace, attachment](const std::size_t left, const std::size_t right) {
                return EstimateAttributeStageCost(workspace, attachment, left) >
                    EstimateAttributeStageCost(workspace, attachment, right);
            });
        return order;
    }

    static void AddStage(
        std::vector<EncodeStageNode>& stageNodes,
        std::unique_ptr<EncodeStage> stage,
        std::vector<EncodeStageId> dependencies = {}) {
        stageNodes.push_back(EncodeStageNode{
            std::move(stage),
            std::move(dependencies)});
    }

    std::vector<EncodeStageNode> BuildStageSchedule(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace) const {
        std::vector<EncodeStageNode> stageNodes;
        std::vector<EncodeStageId> pointRemapDeps;
        std::optional<EncodeStageId> pointRemapId;
        std::optional<EncodeStageId> cellRemapId;
        std::vector<EncodeStageId> outputDeps;
        EncodePipelineDescriptor descriptor;
        if (!ResolvePipelineDescriptor(context, descriptor)) {
            return stageNodes;
        }

        if (UsesPointSpatialPartition(descriptor)) {
            auto pointRemap = std::make_unique<PointRemapStage>(
                workspace.ResourceBudget().RemapEncodeStorageMode());
            pointRemapId = pointRemap->Id();
            AddStage(stageNodes, std::move(pointRemap));
            pointRemapDeps.push_back(*pointRemapId);
        }

        if (UsesCellSpatialPartition(descriptor)) {
            auto cellRemap = std::make_unique<CellRemapStage>(
                workspace.ResourceBudget().RemapEncodeStorageMode());
            cellRemapId = cellRemap->Id();
            // Point Remap 存在时通过 DAG 依赖保证先后关系
            // Point Original 时 Cell Remap 直接消费初始 Order Source
            AddStage(
                stageNodes,
                std::move(cellRemap),
                pointRemapDeps);
        }

        {
            auto geometry = std::make_unique<GeometryStage>(workspace.TransferCacheLayout().geometry);
            outputDeps.push_back(geometry->Id());
            AddStage(stageNodes, std::move(geometry), pointRemapDeps);
        }

        if (workspace.TransferCacheLayout().topology.has_value()) {
            std::size_t transferCacheIndex = kInvalidTransferCacheIndex;
            if (ResolveRequiredTransferCacheIndex(
                    *workspace.TransferCacheLayout().topology,
                    "TopoStage",
                    "topology",
                    transferCacheIndex)) {
                std::vector<EncodeStageId> deps = pointRemapDeps;
                if (cellRemapId.has_value()) {
                    deps.push_back(*cellRemapId);
                }
                auto topology = std::make_unique<TopoStage>(transferCacheIndex);
                outputDeps.push_back(topology->Id());
                AddStage(stageNodes, std::move(topology), std::move(deps));
            }
        }

        const auto pointAttrOrder = BuildAttributeSchedulingOrder(workspace, AttrAttachment::Point);
        // Point 属性只依赖 Point Morton，允许与后续 Cell Morton 并行
        for (const auto attrIndex : pointAttrOrder) {
            const auto metaIndex = workspace.PointAttrMetaIndex(attrIndex);
            auto attr = std::make_unique<PointAttributeStage>(
                workspace.PointAttrSourceIndex(attrIndex),
                metaIndex);
            outputDeps.push_back(attr->Id());
            AddStage(stageNodes, std::move(attr), pointRemapDeps);
        }

        const auto cellAttrOrder = BuildAttributeSchedulingOrder(workspace, AttrAttachment::Cell);
        // Cell 属性依赖 Cell Morton，Topology 与 Cell 属性之间没有先后约束
        for (const auto attrIndex : cellAttrOrder) {
            std::vector<EncodeStageId> deps;
            if (cellRemapId.has_value()) {
                deps.push_back(*cellRemapId);
            }
            const auto metaIndex = workspace.CellAttrMetaIndex(attrIndex);
            auto attr = std::make_unique<CellAttributeStage>(
                workspace.CellAttrSourceIndex(attrIndex),
                metaIndex);
            outputDeps.push_back(attr->Id());
            AddStage(stageNodes, std::move(attr), std::move(deps));
        }

        AddStage(stageNodes, std::make_unique<ParamsEncodeStage>(), std::move(outputDeps));
        return stageNodes;
    }

    static void RecordStageTiming(
        const std::string_view stageName,
        EncodeContext& context,
        const double elapsedMs,
        std::string scope = {}) {
        if (!context.runRecords.Wants(RunRecordKind::StageTiming)) {
            return;
        }
        context.runRecords.RecordStageTiming(
            std::string(stageName),
            elapsedMs,
            ResolveStageCategory(stageName),
            std::move(scope));
    }

    static void RecordStageStart(
        const std::string_view stageName,
        EncodeContext& context) {
        if (!context.runRecords.Wants(RunRecordKind::StageTiming)) {
            return;
        }
        context.runRecords.RecordStageTiming(
            std::string(stageName) + ".start",
            0.0,
            ResolveStageCategory(stageName));
    }

    static TelemetryStageCategory ResolveStageCategory(const std::string_view stageName) {
        return ResolveTelemetryStageCategory(stageName);
    }

    static void SubmitPipelineProgress(
        EncodeContext& context,
        const double normalized) {
        context.runRecords.SubmitProgress(RunProgressRecord{
            .phase = RunProgressPhase::Update,
            .normalized = std::clamp(
                normalized,
                0.0,
                EncodeOutputWriter::kEncodedOutputProgressEnd),
            .success = false,
        });
    }

    static void SubmitPipelineProgress(
        EncodeContext& context,
        const double normalized,
        const DataCodecMessageId messageId,
        std::initializer_list<DataCodecMessageArgument> arguments = {}) {
        context.runRecords.SubmitProgress(
            RunProgressPhase::Update,
            std::clamp(
                normalized,
                0.0,
                EncodeOutputWriter::kEncodedOutputProgressEnd),
            messageId,
            arguments);
    }

    static double MapEncodeStageProgress(const double normalized) noexcept {
        return callback::NormalizeProgress(normalized) *
            EncodeOutputWriter::kEncodeStageProgressEnd;
    }

    static void ReportStageProgress(
        EncodeContext& context,
        const EncodeLeafWorkspace& workspace,
        const EncodeStageId& stageId,
        const double normalized) {
        if (!context.runRecords.Wants(RunRecordKind::Progress)) {
            return;
        }
        DataCodecMessageId messageId{DataCodecMessageId::None};
        if (context.adapter != nullptr) {
            switch (context.adapter->GetEncodeStatusInfo(stageId.name).kind) {
                case EncodeAdapterStatusKind::Sorting:
                    messageId = DataCodecMessageId::EncodeSorting;
                    break;
                case EncodeAdapterStatusKind::TopologyCompression:
                    messageId = DataCodecMessageId::EncodeTopology;
                    break;
                case EncodeAdapterStatusKind::GeometryCompression:
                    messageId = DataCodecMessageId::EncodeGeometry;
                    break;
                case EncodeAdapterStatusKind::AttributeCompression:
                    messageId = DataCodecMessageId::EncodeAttribute;
                    break;
                case EncodeAdapterStatusKind::None:
                default:
                    break;
            }
        }
        if ((stageId.name == "PointAttributeStage" || stageId.name == "CellAttributeStage") &&
            stageId.index < workspace.StorageParams().attrParams.size()) {
            const auto& attributeName = workspace.StorageParams().attrParams[stageId.index].name;
            if (attributeName.empty()) {
                SubmitPipelineProgress(
                    context,
                    MapEncodeStageProgress(normalized),
                    DataCodecMessageId::EncodeAttributeUnnamed,
                    {{"index", std::to_string(stageId.index + 1u)}});
            } else {
                SubmitPipelineProgress(
                    context,
                    MapEncodeStageProgress(normalized),
                    DataCodecMessageId::EncodeAttributeNamed,
                    {{"name", attributeName}});
            }
            return;
        }
        SubmitPipelineProgress(
            context,
            MapEncodeStageProgress(normalized),
            messageId);
    }

    static void ReportCompletedProgress(
        EncodeContext& context,
        const std::size_t completedCount,
        const std::size_t stageCount) {
        if (!context.runRecords.Wants(RunRecordKind::Progress) || stageCount == 0u) {
            return;
        }
        SubmitPipelineProgress(
            context,
            MapEncodeStageProgress(
                static_cast<double>(completedCount) / static_cast<double>(stageCount)));
    }

    static bool RunStageScheduleAndWriteEncodedOutput(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        std::vector<EncodeStageNode> stageNodes,
        const EncodePipelineRunOptions& runOptions,
        IByteRangeOutput* outputSink,
        EncodedLeafFieldBundle* fieldBundleOutput,
        EncodeAfterStageCallback afterStage,
        std::vector<EncodeStageExecutionRecord>* stageExecutions,
        std::uint64_t* encodedByteCount,
        std::string* error = nullptr) {
        for (const auto& stageNode : stageNodes) {
            stageNode.stage->PrepareOutputs(context, workspace);
        }
        if (context.HasFailure() || workspace.StopRequested()) {
            AssignFailureOrError(context, error, "encode pipeline stopped during output preparation");
            return false;
        }
        if (!ExecuteStageDag(
                context,
                workspace,
                stageNodes,
                runOptions,
                std::move(afterStage),
                stageExecutions,
                error)) {
            return false;
        }
        if (workspace.TransferCaches().Count() != 0u && !workspace.TransferCaches().WaitUntilComplete(workspace.StopToken())) {
            AssignFailureOrError(
                context,
                error,
                "encode transfer cache output was stopped before every transfer cache became ready");
            return false;
        }
        if (context.HasFailure() || workspace.StopRequested()) {
            AssignFailureOrError(context, error, "encode pipeline stopped before writing encoded output");
            return false;
        }
        if (fieldBundleOutput != nullptr) {
            SubmitPipelineProgress(
                context,
                EncodeOutputWriter::kPackageFieldProgressEnd,
                DataCodecMessageId::EncodeFinalizeResult);
            const auto exported = EncodeOutputWriter::BuildEncodedLeafFieldBundle(
                context,
                workspace,
                *fieldBundleOutput,
                encodedByteCount,
                error);
            if (exported) {
                SubmitPipelineProgress(
                    context,
                    EncodeOutputWriter::kEncodedOutputProgressEnd);
            }
            return exported;
        }
        return EncodeOutputWriter::WriteLeafPackage(
            context,
            workspace,
            PackageFieldEncodingParams{
                .mode = runOptions.packageFieldEncoding,
                .zstdLevel = runOptions.packageFieldZstdLevel,
                .workerCount = runOptions.transferCacheWorkerCount,
            },
            outputSink,
            encodedByteCount,
            error);
    }

    static bool BuildStageDependencyGraph(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        const std::vector<EncodeStageNode>& stageNodes,
        std::vector<std::vector<std::size_t>>& dependents,
        std::vector<std::size_t>& remainingDependencies,
        std::string* error) {
        dependents.assign(stageNodes.size(), {});
        remainingDependencies.assign(stageNodes.size(), 0u);
        if (stageNodes.empty()) {
            return true;
        }
        for (std::size_t stageIndex = 0; stageIndex < stageNodes.size(); ++stageIndex) {
            remainingDependencies[stageIndex] = stageNodes[stageIndex].dependencies.size();
            for (const auto& dependencyId : stageNodes[stageIndex].dependencies) {
                const auto dependencyIndex = detail::FindStageIndex(stageNodes, dependencyId);
                if (dependencyIndex == static_cast<std::size_t>(-1)) {
                    const auto message = stageNodes[stageIndex].stage->Describe() +
                        " depends on a missing stage " + dependencyId.ToString();
                    FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                    validation::AssignError(error, message);
                    return false;
                }
                dependents[dependencyIndex].push_back(stageIndex);
            }
        }
        return true;
    }

    static EncodeStageExecutionStatus ExecuteSingleStageNode(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        const EncodeStageNode& stageNode) {
        const auto stageName = stageNode.stage->Id().name;
        const auto collectTiming = context.runRecords.Wants(RunRecordKind::StageTiming);
        const auto stageScope = collectTiming ? stageNode.stage->Describe() : std::string{};
        RecordMemoryTraceStageEvent(context, stageName, true);
        RecordStageStart(stageName, context);
        const auto startTime = callback::StartTiming(collectTiming);
        try {
            const auto status = stageNode.stage->Execute(context, workspace);
            if (collectTiming) {
                RecordStageTiming(
                    stageName,
                    context,
                    callback::ElapsedMilliseconds(startTime),
                    stageScope);
            }
            RecordMemoryTraceStageEvent(context, stageName, false);
            context.AddInfo(
                stageNode.stage->Describe(),
                std::string("stage result=") + EncodeStageExecutionStatusName(status));
            if (status == EncodeStageExecutionStatus::Failed && !context.HasFailure()) {
                FailEncodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    stageNode.stage->Describe() + " returned Failed without a failure record");
            }
            return status;
        } catch (...) {
            RecordMemoryTraceStageEvent(context, stageName, false);
            throw;
        }
    }

    static bool ExecuteStageDagSerial(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        const std::vector<EncodeStageNode>& stageNodes,
        std::vector<std::vector<std::size_t>>& dependents,
        std::vector<std::size_t>& remainingDependencies,
        EncodeAfterStageCallback afterStage,
        std::vector<EncodeStageExecutionRecord>* stageExecutions,
        std::string* error) {
        std::vector<std::uint8_t> completed(stageNodes.size(), 0u);
        std::size_t completedCount = 0u;
        while (completedCount < stageNodes.size()) {
            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "encode pipeline stopped before scheduling the next stage batch");
                return false;
            }
            std::vector<std::size_t> readyStageIndices;
            for (std::size_t stageIndex = 0; stageIndex < stageNodes.size(); ++stageIndex) {
                if (completed[stageIndex] == 0u && remainingDependencies[stageIndex] == 0u) {
                    readyStageIndices.push_back(stageIndex);
                    break;
                }
            }

            if (readyStageIndices.empty()) {
                const std::string message = "encode pipeline scheduler detected a dependency cycle";
                FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
                return false;
            }

            for (const auto readyStageIndex : readyStageIndices) {
                completed[readyStageIndex] = 1u;
            }

            try {
                for (const auto readyStageIndex : readyStageIndices) {
                    const double stageProgress = static_cast<double>(completedCount) /
                        static_cast<double>(stageNodes.size());
                    ReportStageProgress(
                        context,
                        workspace,
                        stageNodes[readyStageIndex].stage->Id(),
                        stageProgress);
                    const auto status = ExecuteSingleStageNode(
                        context,
                        workspace,
                        stageNodes[readyStageIndex]);
                    if (stageExecutions != nullptr) {
                        stageExecutions->push_back(EncodeStageExecutionRecord{
                            .stageId = stageNodes[readyStageIndex].stage->Id(),
                            .status = status,
                        });
                    }
                    if (status == EncodeStageExecutionStatus::Failed) {
                        AssignFailureOrError(
                            context,
                            error,
                            stageNodes[readyStageIndex].stage->Describe() + " failed");
                        return false;
                    }
                }
            } catch (const std::exception& exception) {
                const auto message = std::string("encode stage failed: ") + exception.what();
                FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
                return false;
            } catch (...) {
                const std::string message = "encode stage failed";
                FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
                return false;
            }

            if (context.HasFailure() || workspace.StopRequested()) {
                AssignFailureOrError(context, error, "encode pipeline stopped after a stage batch failed");
                return false;
            }
            for (const auto readyStageIndex : readyStageIndices) {
                if (afterStage) {
                    afterStage(stageNodes[readyStageIndex].stage->Id());
                }
                ++completedCount;
                ReportCompletedProgress(context, completedCount, stageNodes.size());
                for (const auto dependentIndex : dependents[readyStageIndex]) {
                    if (remainingDependencies[dependentIndex] > 0u) {
                        --remainingDependencies[dependentIndex];
                    }
                }
            }
        }
        return true;
    }

    static bool ExecuteStageDagParallel(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        const std::vector<EncodeStageNode>& stageNodes,
        std::vector<std::vector<std::size_t>>& dependents,
        std::vector<std::size_t>& remainingDependencies,
        EncodeAfterStageCallback afterStage,
        IParallelTaskRunner& runner,
        std::vector<EncodeStageExecutionRecord>* stageExecutions,
        std::string* error) {
        std::mutex schedulerMutex;
        std::condition_variable schedulerCv;
        std::vector<std::uint8_t> submitted(stageNodes.size(), 0u);
        std::vector<std::uint8_t> hasStageStatus(stageNodes.size(), 0u);
        std::vector<EncodeStageExecutionStatus> stageStatuses(
            stageNodes.size(),
            EncodeStageExecutionStatus::Failed);
        std::vector<std::size_t> completedQueue;
        completedQueue.reserve(stageNodes.size());
        std::exception_ptr firstException;
        bool schedulingStopped = false;
        std::size_t submittedCount = 0u;
        std::size_t completedCount = 0u;
        std::size_t activeAttributeStages = 0u;
        const auto attributeStageLimit = static_cast<std::size_t>(
            workspace.ResourceBudget().AttributePressioLaneCount());
        const auto isAttributeStage = [&stageNodes](const std::size_t stageIndex) {
            const auto stageName = stageNodes[stageIndex].stage->Id().name;
            return stageName == PointAttributeStage::kTypeName ||
                stageName == CellAttributeStage::kTypeName;
        };

        auto taskGroup = runner.CreateGroup(workspace.StopToken());
        if (taskGroup == nullptr) {
            const std::string message = "encode stage task group is unavailable";
            FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
            validation::AssignError(error, message);
            return false;
        }

        const auto submitStage = [&](const std::size_t stageIndex) {
            if (submitted[stageIndex] != 0u || schedulingStopped) {
                return false;
            }
            const auto attributeStage = isAttributeStage(stageIndex);
            if (attributeStage && activeAttributeStages >= attributeStageLimit) {
                return false;
            }
            submitted[stageIndex] = 1u;
            ++submittedCount;
            if (attributeStage) {
                ++activeAttributeStages;
            }
            const double stageProgress = static_cast<double>(completedCount) /
                static_cast<double>(stageNodes.size());
            ReportStageProgress(
                context,
                workspace,
                stageNodes[stageIndex].stage->Id(),
                stageProgress);
            taskGroup->Submit([&context, &workspace, &stageNodes, &schedulerMutex, &schedulerCv, &completedQueue,
                           &firstException, &hasStageStatus, &stageStatuses, stageIndex]() {
                try {
                    stageStatuses[stageIndex] = context.HasFailure() || workspace.StopRequested()
                        ? EncodeStageExecutionStatus::Failed
                        : ExecuteSingleStageNode(
                              context,
                              workspace,
                              stageNodes[stageIndex]);
                    hasStageStatus[stageIndex] = 1u;
                } catch (...) {
                    std::lock_guard<std::mutex> lock(schedulerMutex);
                    if (firstException == nullptr) {
                        firstException = std::current_exception();
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(schedulerMutex);
                    completedQueue.push_back(stageIndex);
                }
                schedulerCv.notify_one();
            });
            return true;
        };
        const auto submitReadyStages = [&]() {
            bool submittedAny = false;
            for (std::size_t stageIndex = 0u; stageIndex < stageNodes.size(); ++stageIndex) {
                if (remainingDependencies[stageIndex] == 0u && submitStage(stageIndex)) {
                    submittedAny = true;
                }
            }
            return submittedAny;
        };

        submitReadyStages();
        if (submittedCount == 0u) {
            const std::string message = "encode pipeline scheduler detected a dependency cycle";
            FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
            validation::AssignError(error, message);
            return false;
        }

        while (completedCount < stageNodes.size()) {
            std::vector<std::size_t> justCompleted;
            {
                std::unique_lock<std::mutex> lock(schedulerMutex);
                schedulerCv.wait(lock, [&]() {
                    return !completedQueue.empty() || firstException != nullptr;
                });
                justCompleted.swap(completedQueue);
                if (firstException != nullptr) {
                    schedulingStopped = true;
                }
            }

            for (const auto stageIndex : justCompleted) {
                if (isAttributeStage(stageIndex) && activeAttributeStages > 0u) {
                    --activeAttributeStages;
                }
                ++completedCount;
                ReportCompletedProgress(context, completedCount, stageNodes.size());
                if (hasStageStatus[stageIndex] != 0u && stageExecutions != nullptr) {
                    stageExecutions->push_back(EncodeStageExecutionRecord{
                        .stageId = stageNodes[stageIndex].stage->Id(),
                        .status = stageStatuses[stageIndex],
                    });
                }
                if (hasStageStatus[stageIndex] != 0u &&
                    stageStatuses[stageIndex] == EncodeStageExecutionStatus::Failed) {
                    schedulingStopped = true;
                }
                if (context.HasFailure() || workspace.StopRequested()) {
                    schedulingStopped = true;
                }
                if (schedulingStopped) {
                    continue;
                }
                if (afterStage) {
                    afterStage(stageNodes[stageIndex].stage->Id());
                }
                for (const auto dependentIndex : dependents[stageIndex]) {
                    if (remainingDependencies[dependentIndex] > 0u) {
                        --remainingDependencies[dependentIndex];
                    }
                }
            }
            if (!schedulingStopped) {
                submitReadyStages();
            }

            if (!schedulingStopped && completedCount < stageNodes.size() && submittedCount == completedCount) {
                const std::string message = "encode pipeline scheduler detected a dependency cycle";
                FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
                schedulingStopped = true;
            }
            if (schedulingStopped) {
                break;
            }
        }

        taskGroup->Wait();
        if (firstException != nullptr) {
            try {
                std::rethrow_exception(firstException);
            } catch (const std::exception& exception) {
                const auto message = std::string("encode stage failed: ") + exception.what();
                FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
            } catch (...) {
                const std::string message = "encode stage failed";
                FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
                validation::AssignError(error, message);
            }
            return false;
        }
        if (context.HasFailure() || workspace.StopRequested()) {
            AssignFailureOrError(context, error, "encode pipeline stopped after a stage failed");
            return false;
        }
        return completedCount == stageNodes.size();
    }

    static bool ExecuteStageDag(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        const std::vector<EncodeStageNode>& stageNodes,
        const EncodePipelineRunOptions& runOptions,
        EncodeAfterStageCallback afterStage,
        std::vector<EncodeStageExecutionRecord>* stageExecutions,
        std::string* error) {
        if (stageNodes.empty()) {
            return true;
        }
        std::vector<std::vector<std::size_t>> dependents;
        std::vector<std::size_t> remainingDependencies;
        if (!BuildStageDependencyGraph(context, workspace, stageNodes, dependents, remainingDependencies, error)) {
            return false;
        }
        if (runOptions.enableParallelStages && runOptions.parallelTaskRunner == nullptr) {
            const std::string message = "parallel DataCodec encode requires a task runner";
            FailEncodePipeline(context, workspace, CodecErrorCode::PipelineFailure, message);
            validation::AssignError(error, message);
            return false;
        }
        if (!runOptions.enableParallelStages ||
            ResolveParallelTaskCount(stageNodes.size(), runOptions.parallelTaskRunner) <= 1u) {
            return ExecuteStageDagSerial(
                context,
                workspace,
                stageNodes,
                dependents,
                remainingDependencies,
                std::move(afterStage),
                stageExecutions,
                error);
        }
        return ExecuteStageDagParallel(
            context,
            workspace,
            stageNodes,
            dependents,
            remainingDependencies,
            std::move(afterStage),
            *runOptions.parallelTaskRunner,
            stageExecutions,
            error);
    }

    struct EncodeResourceReleaseTracker {
        std::size_t pointRemapConsumers{0};
        std::size_t cellRemapConsumers{0};
    };

    static bool IsPointSpatialPartitionStage(const std::string_view stageName) {
        return stageName.starts_with(PointRemapStage::kTypeName);
    }

    static bool IsCellSpatialPartitionStage(const std::string_view stageName) {
        return stageName.starts_with(CellRemapStage::kTypeName);
    }

    static bool IsPointRemapConsumer(const std::string_view stageName) {
        return stageName == "GeometryStage" ||
            IsCellSpatialPartitionStage(stageName) ||
            stageName == "TopoStage" ||
            stageName == "PointAttributeStage";
    }

    static bool IsCellRemapConsumer(const std::string_view stageName) {
        return stageName == "TopoStage" ||
            stageName == "CellAttributeStage";
    }

    static void RecordAttributeRuntimeResourceUsage(
        EncodeContext& context,
        const EncodeLeafWorkspace& workspace) {
        if (!context.runRecords.Wants(RunRecordKind::ResourceUsage)) {
            return;
        }
        const auto stats = workspace.AttributeEncodeSchedulerRef().SnapshotStats();
        const auto recordByteQuota = [&context](
            const std::string& prefix,
            const AttributeByteQuotaStats& quota) {
            context.runRecords.RecordResourceUsage(prefix + ".max_active_bytes", quota.maxActiveBytes);
            context.runRecords.RecordResourceUsage(prefix + ".peak_active_bytes", quota.peakActiveBytes);
            context.runRecords.RecordResourceUsage(prefix + ".peak_requested_bytes", quota.peakRequestedBytes);
        };
        recordByteQuota("attribute.runtime.scratch", stats.scratch);
        recordByteQuota("attribute.runtime.staging", stats.staging);
        context.AddInfo(
            "AttributeEncodeScheduler",
            "scratch acquire=" + std::to_string(stats.scratch.acquireCount) +
                " wait=" + std::to_string(stats.scratch.waitCount) +
                " waitNs=" + std::to_string(stats.scratch.totalWaitNanoseconds) +
                "; staging acquire=" + std::to_string(stats.staging.acquireCount) +
                " wait=" + std::to_string(stats.staging.waitCount) +
                " waitNs=" + std::to_string(stats.staging.totalWaitNanoseconds));
        context.AddInfo(
            "AttributeEncodeScheduler",
            "pressio lane peak=" + std::to_string(stats.pressioLane.peakActive) +
                "/" + std::to_string(stats.pressioLane.laneCount) +
                " acquire=" + std::to_string(stats.pressioLane.acquireCount) +
                " wait=" + std::to_string(stats.pressioLane.waitCount) +
                " waitNs=" + std::to_string(stats.pressioLane.totalWaitNanoseconds) +
                "; reference lane peak=" + std::to_string(stats.referenceLane.peakActive) +
                "/" + std::to_string(stats.referenceLane.laneCount) +
                " acquire=" + std::to_string(stats.referenceLane.acquireCount) +
                " wait=" + std::to_string(stats.referenceLane.waitCount) +
                " waitNs=" + std::to_string(stats.referenceLane.totalWaitNanoseconds));
        context.AddInfo(
            "AttributeEncodeScheduler",
            "pressio calls=" + std::to_string(stats.pressioCallCount) +
                " totalNs=" + std::to_string(stats.pressioTotalNanoseconds) +
                " maxNs=" + std::to_string(stats.pressioMaxNanoseconds));
    }

    static void RecordStoreRuntimeResourceUsage(
        EncodeContext& context,
        const EncodeLeafWorkspace& workspace) {
        if (!context.runRecords.Wants(RunRecordKind::ResourceUsage)) {
            return;
        }
        const auto storeStats = workspace.ByteStoreSessionRef().SnapshotStats();
        const auto scratchStats = workspace.ScratchBytePool().SnapshotStats();
        const auto windowStats = workspace.CacheResourcesRef().windowBudget.SnapshotStats();
        const auto remapScratchStats =
            workspace.CacheResourcesRef().remapScratchBudget.SnapshotStats();
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
        context.runRecords.RecordResourceUsage(
            "remap_scratch.max_active_bytes",
            remapScratchStats.maxActiveBytes);
        context.runRecords.RecordResourceUsage(
            "remap_scratch.peak_active_bytes",
            remapScratchStats.peakActiveBytes);
        context.runRecords.RecordResourceUsage(
            "remap_scratch.wait_count",
            remapScratchStats.waitCount);
        if (context.referenceByteStoreSession != nullptr) {
            const auto referenceStoreStats = context.referenceByteStoreSession->SnapshotStats();
            context.runRecords.RecordResourceUsage(
                "encode_reference.logical_bytes",
                referenceStoreStats.logicalBytes);
            context.runRecords.RecordResourceUsage(
                "encode_reference.resident_bytes",
                referenceStoreStats.residentBytes);
            context.runRecords.RecordResourceUsage(
                "encode_reference.peak_resident_bytes",
                referenceStoreStats.peakResidentBytes);
            context.runRecords.RecordResourceUsage(
                "encode_reference.resident_limit_bytes",
                referenceStoreStats.residentLimitBytes);
            context.runRecords.RecordResourceUsage(
                "encode_reference.managed_file_bytes",
                referenceStoreStats.managedFileBytes);
        }
    }

    static EncodeResourceReleaseTracker BuildResourceReleaseTracker(
        const std::vector<EncodeStageNode>& stageNodes) {
        EncodeResourceReleaseTracker tracker;
        for (const auto& stageNode : stageNodes) {
            const auto stageName = stageNode.stage->Id().name;
            if (IsPointRemapConsumer(stageName)) {
                ++tracker.pointRemapConsumers;
            }
            if (IsCellRemapConsumer(stageName)) {
                ++tracker.cellRemapConsumers;
            }
        }
        return tracker;
    }

    static void ApplyAfterStageLifecycleRelease(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        EncodeResourceReleaseTracker& tracker,
        const EncodeStageId& stageId) {
        if (IsPointRemapConsumer(stageId.name) && tracker.pointRemapConsumers > 0u) {
            --tracker.pointRemapConsumers;
            if (tracker.pointRemapConsumers == 0u) {
                RecordPointRemapLog(context, workspace);
                (void)workspace.ReleasePointRemap();
            }
        }

        if (IsPointSpatialPartitionStage(stageId.name) && tracker.pointRemapConsumers == 0u) {
            RecordPointRemapLog(context, workspace);
            (void)workspace.ReleasePointRemap();
        }

        if (IsCellRemapConsumer(stageId.name) && tracker.cellRemapConsumers > 0u) {
            --tracker.cellRemapConsumers;
            if (tracker.cellRemapConsumers == 0u) {
                RecordCellRemapLog(context, workspace);
                (void)workspace.ReleaseCellRemap();
            }
        }

        if (stageId.name == "GeometryStage") {
            (void)workspace.ReleaseGeometrySource();
        }
    }

    static void RecordPointRemapLog(
        EncodeContext& context,
        const EncodeLeafWorkspace& workspace) {
        const auto& source = workspace.PointOrderSource();
        context.AddInfo(
            "PointOrderSource",
            source.IsOriginal()
                ? "kind=Original"
                : "kind=Computed; identity=" +
                    std::string(source.IsComputedIdentity() ? "true" : "false"));
        context.runRecords.RecordRemapOrder(
            context.path,
            RunRemapDomain::Point,
            source.Provider());
    }

    static void RecordCellRemapLog(
        EncodeContext& context,
        const EncodeLeafWorkspace& workspace) {
        const auto& source = workspace.CellOrderSource();
        context.AddInfo(
            "CellOrderSource",
            source.IsOriginal()
                ? "kind=Original"
                : "kind=Computed; identity=" +
                    std::string(source.IsComputedIdentity() ? "true" : "false"));
        context.runRecords.RecordRemapOrder(
            context.path,
            RunRemapDomain::Cell,
            source.Provider());
    }

    // 在 DAG 展开前构建本次运行的 transfer cache 布局
    void PrepareDataflow(
        EncodeContext& context,
        EncodeLeafWorkspace& workspace,
        const bool prepareGeometrySource = true) const {
        workspace.Reset();
        if (!m_options.binding.has_value()) {
            FailEncodePipeline(
                context,
                workspace,
                CodecErrorCode::InvalidInput,
                "encode entry did not provide a resolved pipeline");
            return;
        }
        {
            workspace.SetResourceBudget(
                m_options.binding->executionProfile.resourceBudget);
            const auto& budget = workspace.ResourceBudget();
            workspace.ConfigureCacheResources(
                budget.AccessWindowBytes(),
                budget.ActiveWindowBytes(),
                budget.ScratchRetainedBlockCount(),
                budget.ScratchRetainedBlockBytes(),
                budget.ScratchRetainedTotalBytes(),
                budget.RemapScratchQuotaBytes());
            workspace.ConfigureAttributeEncodeResources(
                context.runRecords.Wants(RunRecordKind::StageTiming));
            context.AddInfo(
                "EncodeStorageMode",
                "geometryTransfer=" + std::string(detail::EncodeStorageModeName(
                    budget.GeometryEncodeTransferCacheStorageMode())) +
                    "; geometryStaging=" + detail::EncodeStorageModeName(
                        budget.GeometryEncodeStagingStorageMode()) +
                    "; attrTransfer=" + detail::EncodeStorageModeName(
                        budget.AttributeEncodeTransferCacheStorageMode()) +
                    "; attrStaging=" + detail::EncodeStorageModeName(
                        budget.AttributeEncodeStagingStorageMode()) +
                    "; topologyTransfer=" + detail::EncodeStorageModeName(
                        budget.TopologyEncodeTransferCacheStorageMode()) +
                    "; remap=" + detail::EncodeStorageModeName(
                        budget.RemapEncodeStorageMode()));
        }
        std::string storageParamsError;
        std::vector<std::string> storageParamsWarnings;
        std::vector<std::size_t> selectedAttributeIndices;
        selectedAttributeIndices.reserve(context.attributeTargets.size());
        for (const auto& target : context.attributeTargets) {
            if (MatchesAttributeTargetLeaf(target, context.frameIndex, context.path)) {
                selectedAttributeIndices.push_back(target.attrIndex);
            }
        }
        if (!CodecStorageParamsFactory::TryFromEncodeAdapter(
                *context.adapter,
                context.controlParams,
                selectedAttributeIndices,
                workspace.StorageParams(),
                &storageParamsError,
                &storageParamsWarnings)) {
            FailEncodePipeline(
                context,
                workspace,
                CodecErrorCode::MissingInput,
                "failed to resolve codec storage params: " + storageParamsError);
            return;
        }
        for (auto& warning : storageParamsWarnings) {
            context.AddWarning("CodecStorageParamsFactory", std::move(warning));
        }
        if (context.adapter != nullptr) {
            if (prepareGeometrySource) {
                NumericArrayView geometry;
                std::string geometryError;
                if (!context.adapter->BuildGeometryView(geometry)) {
                    FailEncodePipeline(
                        context,
                        workspace,
                        CodecErrorCode::MissingInput,
                        "failed to get geometry view");
                } else if (!workspace.InitializeGeometrySource(
                               geometry,
                               {},
                               &geometryError)) {
                    FailEncodePipeline(
                        context,
                        workspace,
                        CodecErrorCode::PipelineFailure,
                        "failed to initialize geometry source: " + geometryError);
                }
            }
        }
        if (context.currentAttributeReferenceCache != nullptr &&
            !context.currentAttributeReferenceCache->IsInitialized()) {
            std::string referenceCacheError;
            if (context.referenceByteStoreSession == nullptr ||
                !context.currentAttributeReferenceCache->Initialize(
                    workspace.StorageParams(),
                    *context.referenceByteStoreSession,
                    workspace.ResourceBudget().EncodeReferenceResidentLimitBytes(),
                    workspace.ResourceBudget().AttributeEncodeReferenceCacheStorageMode() ==
                        EncodeStorageMode::Memory
                        ? DecodeStorageMode::Memory
                        : DecodeStorageMode::Managed,
                    &referenceCacheError)) {
                FailEncodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    "failed to initialize current attribute reference cache: " + referenceCacheError);
            }
        }
        if (context.currentGeometryReferenceCache != nullptr &&
            !context.currentGeometryReferenceCache->IsInitialized()) {
            std::string geometryReferenceCacheError;
            if (context.referenceByteStoreSession == nullptr ||
                !context.currentGeometryReferenceCache->Initialize(
                    workspace.StorageParams().geomParams,
                    *context.referenceByteStoreSession,
                    workspace.ResourceBudget().GeometryEncodeReferenceCacheStorageMode() ==
                        EncodeStorageMode::Memory
                        ? DecodeStorageMode::Memory
                        : DecodeStorageMode::Managed,
                    workspace.ResourceBudget().EncodeReferenceResidentLimitBytes(),
                    &geometryReferenceCacheError)) {
                FailEncodePipeline(
                    context,
                    workspace,
                    CodecErrorCode::PipelineFailure,
                    "failed to initialize current geometry reference cache: " + geometryReferenceCacheError);
            }
        }
        workspace.ClearTransferCaches();
        // 参数和几何 transfer cache 总是存在
        workspace.TransferCacheLayout().params = workspace.AddTransferCache(FieldType::Params, "params", 0, AttrAttachment::Point, EncodedFieldCodecType::Params);
        workspace.TransferCacheLayout().geometry =
            workspace.AddTransferCache(FieldType::Geometry, "geometry", 0, AttrAttachment::Point, EncodedFieldCodecType::NumericArrayBlocks);

        // 对纯 point-set 数据，topology 是可选的
        const bool hasTopology =
            m_options.binding->descriptor.includeTopology &&
            context.adapter != nullptr &&
            (context.adapter->GetNumberOfCells() > 0 || context.adapter->IsStructuredMesh());
        if (hasTopology) {
            workspace.TransferCacheLayout().topology =
                workspace.AddTransferCache(FieldType::Topology, "topology", 0, AttrAttachment::Cell, EncodedFieldCodecType::Topology);
        }

        // point 和 cell 走独立阶段，这里只固定最终落盘顺序
        const auto pointAttrCount = CountAttrStorageParams(workspace.StorageParams(), AttrAttachment::Point);
        const auto cellAttrCount = CountAttrStorageParams(workspace.StorageParams(), AttrAttachment::Cell);
        if (pointAttrCount + cellAttrCount > 0u) {
            workspace.TransferCacheLayout().attributes = workspace.AddTransferCache(
                FieldType::Attribute,
                "attributes",
                0,
                AttrAttachment::Point,
                EncodedFieldCodecType::Raw);
        }
        for (std::size_t metaIndex = 0u; metaIndex < workspace.StorageParams().attrParams.size(); ++metaIndex) {
            const auto attachment = workspace.StorageParams().attrParams[metaIndex].attachmentType;
            if (attachment == AttrAttachment::Point) {
                workspace.TransferCacheLayout().pointAttrs.push_back(metaIndex);
            } else if (attachment == AttrAttachment::Cell) {
                workspace.TransferCacheLayout().cellAttrs.push_back(metaIndex);
            }
        }
        workspace.InitializeAttributeReferenceSchedules(pointAttrCount, cellAttrCount);
        std::string outputError;
        if (!workspace.InitializeAttributeOutput(pointAttrCount + cellAttrCount, &outputError)) {
            FailEncodePipeline(
                context,
                workspace,
                CodecErrorCode::PipelineFailure,
                "failed to initialize attribute output: " + outputError);
        }
    }

    // 当前 pipeline 实例的运行期调度策略
    EncodePipelineOptions m_options;
};

} // namespace datacodec

#endif
