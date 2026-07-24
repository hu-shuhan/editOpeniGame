#ifndef DATACODEC_WORKFLOW_LEAF_LEAFENCODEEXECUTOR_H
#define DATACODEC_WORKFLOW_LEAF_LEAFENCODEEXECUTOR_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Storage/LeafPackage/EncodedLeafFieldBundle.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Workflow/Encode/EncodePipeline.h"
#include "DataCodec/Log/Telemetry/TelemetryMemoryTrace.h"
#include "DataCodec/Runtime/Record/RunRecordTimestamp.h"
#include "DataCodec/Runtime/Record/RunMessageCaptureSink.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

struct LeafEncodeRequest {
    EncodeContext* context{nullptr};
    EncodePipelineControlParams pipelineControl;
    DataCodecEncodeConfigurationSource configurationSource;
    IRunRecordSink* runRecordSink{nullptr};
    IByteRangeOutput* outputSink{nullptr};
    EncodedLeafFieldBundle* fieldBundleOutput{nullptr};
    bool includeTopology{true};
    bool enableParallelStages{true};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
};

struct LeafEncodeResult {
    bool success{false};
    bool hasEncodedOutput{false};
    std::vector<std::uint8_t> encodedBytes;
    std::uint64_t encodedByteCount{0u};
    std::vector<EncodeStageExecutionRecord> stageExecutions;
    std::vector<TelemetryMessageRecord> messages;
};

class LeafEncodeExecutor {
public:
    static LeafEncodeResult Execute(const LeafEncodeRequest& request) {
        LeafEncodeResult result;
        if (request.context == nullptr) {
            FinalizeEarlyFailure(
                request,
                nullptr,
                "LeafEncodeExecutor",
                CodecErrorCode::MissingInput,
                "DataCodec leaf encoder requires an encode context",
                result);
            return result;
        }

        EncodeContext& context = *request.context;
        struct ContextFailureCleanup {
            EncodeContext& context;
            LeafEncodeResult& result;

            ~ContextFailureCleanup() noexcept {
                context.memoryTrace = nullptr;
                if (!result.success) {
                    context.CleanupOnFailure();
                }
            }
        } contextFailureCleanup{context, result};
        context.parallelTaskRunner = request.parallelTaskRunner;
        ResolveContextMetadata(context);
        if (context.adapter == nullptr) {
            FinalizeEarlyFailure(
                request,
                &context,
                "LeafEncodeExecutor",
                CodecErrorCode::MissingInput,
                "DataCodec leaf encoder requires an encode adapter",
                result);
            return result;
        }

        CodecControlParams defaultParams;
        const auto* originalControlParams = context.controlParams;
        struct ControlParamsRestore {
            EncodeContext& context;
            const CodecControlParams* originalControlParams{nullptr};
            ~ControlParamsRestore() noexcept { context.controlParams = originalControlParams; }
        } controlParamsRestore{context, originalControlParams};
        if (context.controlParams == nullptr) {
            defaultParams = CodecControlParamsFactory::MakeEncodeConfiguration(
                DataCodecEncodeOptions{}).controlParams;
            context.controlParams = &defaultParams;
        }
        DataCodecEncodeConfigurationParams runtimeConfiguration{
            .controlParams = *context.controlParams,
            .pipelineControl = request.pipelineControl,
            .execution = EncodeExecutionOptions{
                .enableParallelStages = request.enableParallelStages,
            },
            .source = request.configurationSource,
        };
        CodecControlParamsFactory::ApplyEncodeRuntimeConstraint(
            runtimeConfiguration,
            request.configurationSource.runtimeProfile);
        context.controlParams = &runtimeConfiguration.controlParams;
        const auto* controlParams = context.controlParams;
        std::string resourceValidationError;
        if (!ValidateResourceBudgetControlParams(
                controlParams->resourceBudget,
                &resourceValidationError) ||
            !CodecControlParamsFactory::ValidateEncodeRuntimeConstraint(
                controlParams->resourceBudget,
                request.configurationSource.runtimeProfile,
                &resourceValidationError)) {
            FinalizeEarlyFailure(
                request,
                &context,
                "LeafEncodeExecutor",
                CodecErrorCode::InvalidInput,
                resourceValidationError,
                result);
            return result;
        }
        const auto outputKind = request.fieldBundleOutput != nullptr
            ? EncodePipelineOutputKind::EncodedLeafFieldBundle
            : EncodePipelineOutputKind::LeafPackage;
        EncodePipelineBinding pipelineBinding;
        std::string pipelineBindingError;
        if (!ResolveEncodePipelineBinding(
                *context.adapter,
                *controlParams,
                runtimeConfiguration.pipelineControl,
                EncodePipelineExecutionProfile{
                    .resourceBudget = controlParams->resourceBudget,
                    .enableParallelStages = runtimeConfiguration.execution.enableParallelStages,
                    .parallelTaskRunner = request.parallelTaskRunner,
                },
                pipelineBinding,
                &pipelineBindingError,
                outputKind,
                request.includeTopology)) {
            FinalizeEarlyFailure(
                request,
                &context,
                "LeafEncodeExecutor",
                CodecErrorCode::InvalidInput,
                pipelineBindingError,
                result);
            return result;
        }
        RunMessageCaptureSink messageSink;
        RunRecordDispatcher recordDispatcher;
        recordDispatcher.AddSink(&messageSink);
        recordDispatcher.AddSink(request.runRecordSink);
        const auto contextInitResult = context.Initialize(&recordDispatcher);
        if (!contextInitResult) {
            context.runRecords.BeginRun();
            context.RecordFailure("EncodeContext", CodecErrorCode::MissingInput, contextInitResult.message);
            context.CleanupOnFailure();
            context.runRecords.EndRun(context.runSummary);
            result.messages = messageSink.TakeMessages();
            return result;
        }
        TelemetryMemoryTraceRecorder memoryTrace;
        context.memoryTrace = &memoryTrace;
        context.runRecords.BeginRun();
        if (context.runRecords.Requests(RunCollectionKind::MemoryTrace)) {
            std::string memoryTraceError;
            if (!memoryTrace.Start(context.runRecords.RunInfo(), &memoryTraceError)) {
                context.runRecords.AddWarning("TelemetryMemoryTrace", memoryTraceError);
            }
        }
        const auto startTime = callback::Now();

        context.runRecords.SubmitProgress(RunProgressRecord{
            .phase = RunProgressPhase::Begin,
            .normalized = 0.0,
            .text = "编码准备中",
            .success = false,
        });

        const auto& pipelineDescriptor = pipelineBinding.descriptor;
        context.AddInfo(
            "EncodePipelineConfiguration",
            "performance=" + std::string(DataCodecEncodeTierName(
                request.configurationSource.performanceTier)) +
                "; runtime=" + DataCodecRuntimeProfileName(
                    request.configurationSource.runtimeProfile) +
                "; compressionEnhancement=" +
                    (request.configurationSource.compressionEnhancementEnabled ? "true" : "false") +
                "; customControlParams=" +
                    (request.configurationSource.customControlParams ? "true" : "false") +
                "; pipeline=" + EncodePipelineBindingName(pipelineDescriptor.id) +
                "; output=" + EncodePipelineOutputKindName(pipelineDescriptor.outputKind) +
                "; pointOrder=" + EncodePointOrderModeName(pipelineDescriptor.pointOrder) +
                "; cellOrder=" + EncodeCellOrderModeName(pipelineDescriptor.cellOrder) +
                "; topology=" + (pipelineDescriptor.includeTopology ? "Owned" : "Reused") +
                "; packageField=" + PackageFieldEncodingModeName(pipelineDescriptor.packageFields.mode) +
                "; zstdLevel=" + std::to_string(pipelineDescriptor.packageFields.zstdLevel) +
                "; packageWorkers=" + std::to_string(pipelineDescriptor.packageFields.workerCount));
        if (pipelineDescriptor.pointOrder == EncodePointOrderMode::Original) {
            context.AddInfo("PointOrderSource", "kind=Original");
        }
        if (pipelineDescriptor.cellOrder == EncodeCellOrderMode::Original) {
            context.AddInfo("CellOrderSource", "kind=Original");
        }
        EncodePipeline pipeline({
            .binding = std::move(pipelineBinding),
        });
        EncodePipelineResult pipelineResult;
        if (request.fieldBundleOutput != nullptr) {
            pipelineResult = pipeline.ExecuteToFieldBundle(context, *request.fieldBundleOutput);
        } else if (request.outputSink != nullptr) {
            pipelineResult = pipeline.ExecuteToSink(context, *request.outputSink);
        } else {
            pipelineResult = pipeline.Execute(context);
        }
        context.runRecords.SubmitProgress(RunProgressRecord{
            .phase = RunProgressPhase::Finish,
            .normalized = 1.0,
            .success = pipelineResult.success,
        });
        context.runSummary.success = pipelineResult.success;
        context.runSummary.elapsedMs = callback::ElapsedMilliseconds(startTime);
        context.runSummary.outputBytes = ResolveEncodedOutputBytes(
            pipelineResult.encodedBytes,
            pipelineResult.encodedByteCount);
        if (memoryTrace.Active()) {
            std::string stopError;
            if (memoryTrace.Stop(&stopError)) {
                auto csvText = memoryTrace.CsvText();
                if (!csvText.empty()) {
                    context.runRecords.AddArtifact(TelemetryArtifactRecord{
                        .name = "memory_trace",
                        .mediaType = "text/csv",
                        .preferredExtension = ".csv",
                        .text = std::move(csvText),
                    });
                    context.runRecords.AddInfo("TelemetryMemoryTrace", "prepared memory trace artifact");
                }
            } else {
                context.runRecords.AddWarning("TelemetryMemoryTrace", stopError);
            }
        }
        context.memoryTrace = nullptr;
        context.runRecords.EndRun(context.runSummary);
        result.messages = messageSink.TakeMessages();
        result.success = pipelineResult.success;
        result.hasEncodedOutput = pipelineResult.hasEncodedOutput;
        result.encodedBytes = std::move(pipelineResult.encodedBytes);
        result.encodedByteCount = pipelineResult.encodedByteCount;
        result.stageExecutions = std::move(pipelineResult.stageExecutions);
        return result;
    }

private:
    static void FinalizeEarlyFailure(
        const LeafEncodeRequest& request,
        EncodeContext* context,
        const std::string_view origin,
        const CodecErrorCode code,
        const std::string_view message,
        LeafEncodeResult& result) {
        if (context != nullptr) {
            context->CleanupOnFailure();
        }
        RunMessageCaptureSink messageSink;
        RunRecordDispatcher recordDispatcher;
        recordDispatcher.AddSink(&messageSink);
        recordDispatcher.AddSink(request.runRecordSink);
        RunRecordEmitter records;
        records.Reset(
            RunRecordInfo{
                .generatedAtUtc = runrecorddetail::MakeTimestampUtc(),
                .runKind = TelemetryRunKind::Encode,
                .objectName = context != nullptr ? context->objectName : std::string{},
                .leafPath = context != nullptr ? context->path : BlockPath{},
                .meshType = context != nullptr ? context->meshType : "unknown",
            },
            &recordDispatcher);
        records.BeginRun();
        records.AddMessage(MakeCodecTelemetryMessage(std::string(origin), code, std::string(message)));
        records.EndRun(RunEndRecord{.success = false});
        result.messages = messageSink.TakeMessages();
    }

    static std::uint64_t ResolveEncodedOutputBytes(
        const std::vector<std::uint8_t>& encodedBytes,
        const std::uint64_t encodedByteCount) {
        if (!encodedBytes.empty()) {
            return static_cast<std::uint64_t>(encodedBytes.size());
        }
        return encodedByteCount;
    }

    static void ResolveContextMetadata(EncodeContext& context) {
        if (context.adapter != nullptr && context.objectName.empty()) {
            context.objectName = context.adapter->GetName();
        }
        if (context.adapter != nullptr && context.meshType.empty()) {
            context.meshType = MeshTypeName(context.adapter->GetMeshType());
        }
        if (context.meshType.empty()) {
            context.meshType = "unknown";
        }
    }
};

} // namespace datacodec

#endif
