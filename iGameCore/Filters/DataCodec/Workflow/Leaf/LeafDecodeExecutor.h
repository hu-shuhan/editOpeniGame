#ifndef DATACODEC_WORKFLOW_LEAF_LEAFDECODEEXECUTOR_H
#define DATACODEC_WORKFLOW_LEAF_LEAFDECODEEXECUTOR_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Log/Telemetry/TelemetryMemoryTrace.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Validation/Runtime/RuntimeValidator.h"
#include "DataCodec/Validation/Workflow/DecodeValidationLifecycle.h"
#include "DataCodec/Workflow/Decode/DecodePipeline.h"
#include "DataCodec/Runtime/Failure/PipelineFailureManagement.h"
#include "DataCodec/Runtime/Record/RunRecordTimestamp.h"
#include "DataCodec/Runtime/Record/RunMessageCaptureSink.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"

#include <exception>
#include <memory>
#include <span>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

namespace datacodec {

struct LeafDecodeRequest {
    IDecodeAdapter* adapter{nullptr};
    const LeafPackage* leafPackage{nullptr};
    DecodedAttributeReference* attributeKeyFrameReference{nullptr};
    DecodedGeometryReference* geometryKeyFrameReference{nullptr};
    DecodedGeometryReferenceCache* currentGeometryReferenceCache{nullptr};
    DecodedTopologyReferenceCacheStore* topologyReferenceStore{nullptr};
    std::string topologyReferenceKey;
    std::uint32_t topologyOwnerFrameIndex{0u};
    std::uint32_t frameIndex{0u};
    AttributeSelectionMode attributeSelection{AttributeSelectionMode::None};
    std::span<const AttributeTarget> attributeTargets;
    DecodeLeafWorkspace* workspace{nullptr};
    bool supplementAttributesOnly{false};
    AttributeDecodeRequestMode attributeRequestMode{AttributeDecodeRequestMode::DecodeAndCommit};
    DecodeControlParams controlParams{MakeDefaultDecodeControlParams()};
    DecodeExecutionOptions execution{MakeDefaultDecodeExecutionOptions()};
    DataCodecDecodeConfigurationSource configurationSource;
    IRunRecordSink* runRecordSink{nullptr};
    std::stop_token stopToken;
    IParallelTaskRunner* parallelTaskRunner{nullptr};
};

struct LeafDecodeResult {
    bool success{false};
    bool committedOutput{false};
    std::vector<TelemetryMessageRecord> messages;
};

class LeafDecodeExecutor {
public:
    [[nodiscard]] static LeafDecodeResult Execute(const LeafDecodeRequest& request) {
        LeafDecodeResult result;
        DecodeContext context;
        context.adapter = request.adapter;
        bool completedSuccessfully = false;
        struct FailureCleanupScope {
            const LeafDecodeRequest& request;
            DecodeContext* context{nullptr};
            bool& completedSuccessfully;

            ~FailureCleanupScope() noexcept {
                if (completedSuccessfully) {
                    return;
                }
                if (context != nullptr) {
                    if (request.workspace != nullptr) {
                        CleanupAfterDecodeFailure(*context, *request.workspace);
                    } else {
                        context->CleanupOnFailure();
                    }
                    return;
                }
                if (request.workspace != nullptr) {
                    request.workspace->CleanupOnFailure();
                }
                if (request.adapter != nullptr) {
                    try {
                        request.adapter->Abort();
                    } catch (...) {
                    }
                }
            }
        } failureCleanup{request, &context, completedSuccessfully};
        DataCodecDecodeConfigurationParams runtimeConfiguration{
            .controlParams = request.controlParams,
            .execution = request.execution,
            .source = request.configurationSource,
        };
        CodecControlParamsFactory::ApplyDecodeRuntimeConstraint(
            runtimeConfiguration,
            request.configurationSource.runtimeProfile);
        const auto& controlParams = runtimeConfiguration.controlParams;

        const auto runtimeValidation = validation::RuntimeValidator::ValidateDecodeConfiguration(
            controlParams,
            request.configurationSource.runtimeProfile);
        if (!runtimeValidation) {
            FinalizeEarlyFailure(
                request,
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::FormatAndParams)),
                runtimeValidation.code,
                runtimeValidation.message,
                result);
            return result;
        }

        if (request.adapter == nullptr) {
            FinalizeEarlyFailure(
                request,
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::AlgorithmPrecondition)),
                CodecErrorCode::MissingInput,
                "DataCodec leaf decoder requires a decode adapter",
                result);
            return result;
        }
        if (request.leafPackage == nullptr) {
            FinalizeEarlyFailure(
                request,
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::AlgorithmPrecondition)),
                CodecErrorCode::MissingInput,
                "DataCodec leaf decoder requires a leaf package",
                result);
            return result;
        }

        RunMessageCaptureSink messageSink;
        RunRecordDispatcher recordDispatcher;
        recordDispatcher.AddSink(&messageSink);
        recordDispatcher.AddSink(request.runRecordSink);
        context.leafPackage = request.leafPackage;
        context.attributeKeyFrameReference = request.attributeKeyFrameReference;
        context.geometryKeyFrameReference = request.geometryKeyFrameReference;
        context.currentGeometryReferenceCache = request.currentGeometryReferenceCache;
        context.topologyReferenceStore = request.topologyReferenceStore;
        context.topologyReferenceKey = request.topologyReferenceKey;
        context.frameIndex = request.frameIndex;
        context.attributeSelection = request.attributeSelection;
        context.attributeTargets = request.attributeTargets;
        context.attributeRequestMode = request.attributeRequestMode;
        context.parallelTaskRunner = request.parallelTaskRunner;
        context.topologyOutputMode = request.execution.topologyOutputMode;
        context.topologyBlockObserver = request.execution.topologyBlockObserver;

        const auto contextCreateResult = context.Initialize(&recordDispatcher);
        if (!contextCreateResult) {
            context.runRecords.BeginRun();
            context.RecordFailure(
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::AlgorithmPrecondition)),
                CodecErrorCode::MissingInput,
                contextCreateResult.message);
            CleanupDecodeFailure(request, context);
            context.runRecords.EndRun(context.runSummary);
            result.messages = messageSink.TakeMessages();
            return result;
        }

        TelemetryMemoryTraceRecorder memoryTrace;
        context.memoryTrace = &memoryTrace;
        context.runRecords.BeginRun();
        context.AddInfo(
            "DecodePipelineConfiguration",
            "performance=" + std::string(DataCodecDecodeTierName(
                request.configurationSource.performanceTier)) +
                "; runtime=" + DataCodecRuntimeProfileName(
                    request.configurationSource.runtimeProfile) +
                "; validation=" +
                    (request.configurationSource.validationProfile ==
                            DataCodecDecodeValidationProfile::Audit
                        ? "Audit"
                        : "Required") +
                "; customControlParams=" +
                    (request.configurationSource.customControlParams ? "true" : "false"));
        if (context.runRecords.Requests(RunCollectionKind::MemoryTrace)) {
            std::string memoryTraceError;
            if (!memoryTrace.Start(context.runRecords.RunInfo(), &memoryTraceError)) {
                context.runRecords.AddWarning("TelemetryMemoryTrace", memoryTraceError);
            }
        }
        const auto startTime = callback::Now();

        struct ExternalStopBinding {
            DecodeLeafWorkspace* workspace{nullptr};

            ExternalStopBinding(DecodeLeafWorkspace* target, const std::stop_token token)
                : workspace(target) {
                if (workspace != nullptr) { workspace->SetExternalStopToken(token); }
            }

            ~ExternalStopBinding() {
                if (workspace != nullptr) { workspace->ClearExternalStopToken(); }
            }
        } stopBinding(request.workspace, request.stopToken);

        try {
            DecodePipeline pipeline({
                .enableParallelStages = runtimeConfiguration.execution.enableParallelStages,
                .parallelTaskRunner = request.parallelTaskRunner,
                .resourceBudget = controlParams.resourceBudget,
                .validationPolicy = controlParams.validation,
            });
            if (request.supplementAttributesOnly) {
                if (request.workspace == nullptr) {
                    context.RecordFailure(
                        "LeafDecodeExecutor",
                        CodecErrorCode::MissingInput,
                        "attribute supplement requires a persistent decode workspace");
                } else {
                    pipeline.ExecuteAttributes(context, *request.workspace);
                }
            } else if (request.workspace != nullptr) {
                pipeline.Execute(context, *request.workspace);
            } else {
                pipeline.Execute(context);
            }

            const auto requiresAdapterCommit =
                !request.supplementAttributesOnly ||
                request.attributeRequestMode != AttributeDecodeRequestMode::DecodeToCache;
            if (!context.HasFailure() && requiresAdapterCommit) {
                CommitOutput(context, result);
            }
        } catch (const std::exception& exception) {
            context.RecordFailure(
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::AlgorithmExecution)),
                CodecErrorCode::DecodeFailure,
                std::string("unexpected decode exception: ") + exception.what());
        } catch (...) {
            context.RecordFailure(
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::AlgorithmExecution)),
                CodecErrorCode::DecodeFailure,
                "unexpected decode exception");
        }

        const auto requiresAdapterCommit =
            !request.supplementAttributesOnly ||
            request.attributeRequestMode != AttributeDecodeRequestMode::DecodeToCache;
        if (context.HasFailure() || (requiresAdapterCommit && !result.committedOutput)) {
            if (!context.HasFailure() && requiresAdapterCommit) {
                context.RecordFailure(
                    std::string(validation::DecodeValidationNodeName(
                        validation::DecodeValidationNode::Commit)),
                    CodecErrorCode::DecodeFailure,
                    "decode did not commit output");
            }
            CleanupDecodeFailure(request, context);
        }
        result.success = !context.HasFailure() && (!requiresAdapterCommit || result.committedOutput);

        context.runSummary.elapsedMs = callback::ElapsedMilliseconds(startTime);
        context.runSummary.success = result.success;
        context.runSummary.inputBytes = request.leafPackage->EncodedFieldBytes();
        context.runSummary.outputBytes = request.leafPackage->rawFieldBytes;
        CaptureMemoryTrace(context, memoryTrace);
        context.memoryTrace = nullptr;
        context.runRecords.EndRun(context.runSummary);
        result.messages = messageSink.TakeMessages();
        completedSuccessfully = result.success;
        return result;
    }

private:
    static void CleanupDecodeFailure(
        const LeafDecodeRequest& request,
        DecodeContext& context) noexcept {
        if (request.workspace != nullptr) {
            CleanupAfterDecodeFailure(context, *request.workspace);
            return;
        }
        context.CleanupOnFailure();
    }

    static void FinalizeEarlyFailure(
        const LeafDecodeRequest& request,
        const std::string& origin,
        const CodecErrorCode code,
        const std::string& message,
        LeafDecodeResult& result) {
        RunMessageCaptureSink messageSink;
        RunRecordDispatcher recordDispatcher;
        recordDispatcher.AddSink(&messageSink);
        recordDispatcher.AddSink(request.runRecordSink);
        RunRecordEmitter records;
        records.Reset(
            RunRecordInfo{
                .generatedAtUtc = runrecorddetail::MakeTimestampUtc(),
                .runKind = TelemetryRunKind::Decode,
                .objectName = request.leafPackage != nullptr
                    ? request.leafPackage->path
                    : std::string{},
                .leafPath = request.leafPackage != nullptr
                    ? request.leafPackage->path
                    : BlockPath{},
            },
            &recordDispatcher);
        records.BeginRun();
        records.AddMessage(MakeCodecTelemetryMessage(origin, code, message));
        records.EndRun(RunEndRecord{.success = false});
        result.messages = messageSink.TakeMessages();
    }

    static void CommitOutput(
        DecodeContext& context,
        LeafDecodeResult& result) {
        try {
            std::string commitError;
            if (!context.adapter->Commit(&commitError)) {
                context.RecordFailure(
                    std::string(validation::DecodeValidationNodeName(
                        validation::DecodeValidationNode::Commit)),
                    CodecErrorCode::DecodeFailure,
                    commitError.empty() ? "failed to commit decoded output" : commitError);
            } else {
                result.committedOutput = true;
            }
        } catch (const std::exception& exception) {
            context.RecordFailure(
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::Commit)),
                CodecErrorCode::DecodeFailure,
                std::string("failed to commit decoded output: ") + exception.what());
        } catch (...) {
            context.RecordFailure(
                std::string(validation::DecodeValidationNodeName(
                    validation::DecodeValidationNode::Commit)),
                CodecErrorCode::DecodeFailure,
                "failed to commit decoded output");
        }
    }

    static void CaptureMemoryTrace(
        DecodeContext& context,
        TelemetryMemoryTraceRecorder& memoryTrace) {
        if (!memoryTrace.Active()) {
            return;
        }
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
                context.AddInfo("TelemetryMemoryTrace", "prepared memory trace artifact");
            }
        } else {
            context.AddWarning("TelemetryMemoryTrace", stopError);
        }
    }
};

} // namespace datacodec

#endif
