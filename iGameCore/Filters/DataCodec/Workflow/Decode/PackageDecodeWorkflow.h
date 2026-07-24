#ifndef DATACODEC_WORKFLOW_DECODE_PACKAGEDECODEWORKFLOW_H
#define DATACODEC_WORKFLOW_DECODE_PACKAGEDECODEWORKFLOW_H

#include "DataCodec/API/Entry/DataCodecDecodeEntry.h"
#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Runtime/Execution/DataCodecExecutionResources.h"
#include "DataCodec/Runtime/Record/RunRecordEmitter.h"
#include "DataCodec/Runtime/Record/ProgressRangeRunRecordSink.h"
#include "DataCodec/Runtime/Record/RunRecordDispatcher.h"
#include "DataCodec/Runtime/Record/RunRecordTimestamp.h"
#include "DataCodec/Storage/FramePackage/FramePackageIO.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageIO.h"
#include "DataCodec/Storage/Package/PackageBinaryHeader.h"
#include "DataCodec/Workflow/Frame/FrameTopologyOwnership.h"
#include "DataCodec/Workflow/Leaf/LeafDecodeExecutor.h"
#include "DataCodec/Workflow/Session/DecodeSession.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Validation/Storage/StorageValidator.h"
#include "DataCodec/Validation/Workflow/DecodeValidationLifecycle.h"

#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

namespace datacodec {

inline constexpr const char* kPackageDecodeWorkflowOrigin = "PackageDecodeWorkflow";

[[nodiscard]] inline TelemetryMessageRecord MakeDecodePackageMessage(
    const TelemetryMessageSeverity severity,
    std::string origin,
    std::string text) {
    return TelemetryMessageRecord{
        .severity = severity,
        .origin = std::move(origin),
        .text = std::move(text),
    };
}

inline void AppendDecodePackageMessages(
    std::vector<TelemetryMessageRecord>& target,
    const std::vector<TelemetryMessageRecord>& messages) {
    target.insert(target.end(), messages.begin(), messages.end());
}

inline void AddDecodePackageMessage(
    DecodePackageResult& result,
    RunRecordEmitter& runRecords,
    const TelemetryMessageSeverity severity,
    std::string origin,
    std::string text) {
    auto message = MakeDecodePackageMessage(
        severity,
        std::move(origin),
        std::move(text));
    runRecords.AddMessage(message);
    result.messages.push_back(std::move(message));
}

inline void SubmitDecodePackageProgress(
    RunRecordEmitter& runRecords,
    const RunProgressPhase phase,
    const double normalized,
    std::string text,
    const bool success) {
    runRecords.SubmitProgress(RunProgressRecord{
        .phase = phase,
        .normalized = callback::NormalizeProgress(normalized),
        .text = std::move(text),
        .success = success,
    });
}

[[nodiscard]] inline DecodePackageResult DecodeLeafPackage(
    const LeafPackage& leafPackage,
    const DecodePackageRequest& request,
    IRunRecordSink* runRecordSink,
    RunRecordEmitter& packageRecords,
    DecodeSession& session,
    const DataCodecExecutionResources& resources) {
    DecodePackageResult result;
    if (request.stopToken.stop_requested()) {
        result.cancelled = true;
        return result;
    }
    if (request.leafAdapter == nullptr) {
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            "decode package requires a leaf decode adapter");
        return result;
    }

    const auto frameIndex = request.requestedFrameIndex.value_or(0u);
    auto decodeResult = session.DecodeLeaf(LeafDecodeRequest{
        .adapter = request.leafAdapter,
        .leafPackage = &leafPackage,
        .topologyReferenceKey = request.topologyReferenceKey,
        .topologyOwnerFrameIndex = request.topologyOwnerFrameIndex,
        .frameIndex = frameIndex,
        .attributeTargets = std::span<const AttributeTarget>(request.attributeTargets),
        .decodeAllAvailableAttributes = request.decodeAllAvailableAttributes,
        .controlParams = request.controlParams,
        .execution = request.execution,
        .configurationSource = request.configurationSource,
        .runRecordSink = runRecordSink,
        .stopToken = request.stopToken,
        .parallelTaskRunner = resources.parallelTaskRunner,
    });
    if (request.stopToken.stop_requested()) {
        result.cancelled = true;
        return result;
    }
    result.success = decodeResult.success;
    result.messages = std::move(decodeResult.messages);
    return result;
}

[[nodiscard]] inline DecodePackageResult DecodeLeafByteRange(
    const DecodePackageRequest& request,
    RunRecordEmitter& packageRecords,
    IRunRecordSink* leafRunRecordSink,
    const DataCodecExecutionResources& resources) {
    DecodePackageResult result;
    result.inputBytes = request.inputReader == nullptr ? 0u : request.inputReader->ByteSize();

    LeafPackage leafPackage;
    std::string readError;
    if (!LeafPackageIO::ReadFromByteRange(
            request.inputReader,
            0u,
            result.inputBytes,
            leafPackage,
            &readError)) {
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            readError.empty() ? "failed to read DataCodec leaf package" : readError);
        return result;
    }

    DecodeSession localSession;
    auto& session = request.session != nullptr ? *request.session : localSession;
    result = DecodeLeafPackage(
        leafPackage,
        request,
        leafRunRecordSink,
        packageRecords,
        session,
        resources);
    result.inputBytes = request.inputReader == nullptr ? 0u : request.inputReader->ByteSize();
    return result;
}

[[nodiscard]] inline DecodePackageResult DecodeFramePackage(
    const FramePackage& framePackage,
    const DecodePackageRequest& request,
    RunRecordEmitter& packageRecords,
    IRunRecordSink* leafRunRecordSink,
    const DataCodecExecutionResources& resources) {
    DecodePackageResult result;
    result.decodedFramePackage = true;
    result.inputBytes = request.inputReader == nullptr ? 0u : request.inputReader->ByteSize();
    if (request.stopToken.stop_requested()) {
        result.cancelled = true;
        return result;
    }

    if (request.frameAssembly == nullptr) {
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            "decode memory frame package requires a frame assembly adapter");
        return result;
    }
    DecodeSession localDecodeSession;
    auto& decodeSession = request.session != nullptr ? *request.session : localDecodeSession;
    std::string assemblyError;
    if (!decodeSession.BeginFramePackage(
            framePackage,
            *request.frameAssembly,
            request.requestedFrameIndex,
            &assemblyError)) {
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            assemblyError.empty() ? "failed to begin frame package assembly" : assemblyError);
        return result;
    }

    const auto leafCount = framePackage.leaves.size();
    for (std::size_t leafIndex = 0; leafIndex < leafCount; ++leafIndex) {
        if (request.stopToken.stop_requested()) {
            result.cancelled = true;
            decodeSession.AbortFramePackage();
            return result;
        }
        const auto& leaf = framePackage.leaves[leafIndex];
        const auto segmentBegin = leafCount == 0u
            ? 0.0
            : static_cast<double>(leafIndex) / static_cast<double>(leafCount);
        const auto segmentEnd = leafCount == 0u
            ? 1.0
            : static_cast<double>(leafIndex + 1u) / static_cast<double>(leafCount);
        const auto leafProgressText = leafCount == 1u
            ? std::string("单块解码")
            : "数据块解码 " + std::to_string(leafIndex + 1u) + "/" + std::to_string(leafCount);
        SubmitDecodePackageProgress(
            packageRecords,
            RunProgressPhase::Update,
            segmentBegin,
            leafProgressText,
            false);

        LeafPackage leafPackage;
        std::string readError;
        if (!LeafPackageIO::ReadFromByteRange(
                request.inputReader,
                leaf.leafPackageByteOffset,
                leaf.leafPackageByteSize,
                leafPackage,
                &readError)) {
            AddDecodePackageMessage(
                result,
                packageRecords,
                TelemetryMessageSeverity::Error,
                kPackageDecodeWorkflowOrigin,
                readError.empty() ? "failed to read frame leaf package" : readError);
            decodeSession.AbortFramePackage();
            return result;
        }
        leafPackage.path = leaf.path;

        assemblyError.clear();
        auto leafAdapter = decodeSession.CreateLeafAdapter(leaf, leafPackage, &assemblyError);
        if (leafAdapter == nullptr) {
            AddDecodePackageMessage(
                result,
                packageRecords,
                TelemetryMessageSeverity::Error,
                kPackageDecodeWorkflowOrigin,
                assemblyError.empty() ? "failed to create frame leaf decode adapter" : assemblyError);
            decodeSession.AbortFramePackage();
            return result;
        }

        ProgressRangeRunRecordSink segmentRecords(
            leafRunRecordSink,
            segmentBegin,
            segmentEnd,
            0u,
            0u,
            leafProgressText);
        auto leafRequest = request;
        leafRequest.leafAdapter = leafAdapter.get();
        leafRequest.requestedFrameIndex = framePackage.frameIndex;
        leafRequest.topologyReferenceKey = FrameTopologyOwnership::MakeTopologyOwnerKey(
            leaf.topologyMode == TopologyOwnershipMode::Owned
                ? framePackage.frameIndex
                : leaf.ownerFrameIndex,
            leaf.path);
        leafRequest.topologyOwnerFrameIndex = leaf.topologyMode == TopologyOwnershipMode::Owned
            ? framePackage.frameIndex
            : leaf.ownerFrameIndex;
        auto leafResult = DecodeLeafPackage(
            leafPackage,
            leafRequest,
            &segmentRecords,
            packageRecords,
            decodeSession,
            resources);
        AppendDecodePackageMessages(result.messages, leafResult.messages);
        if (leafResult.cancelled || request.stopToken.stop_requested()) {
            result.cancelled = true;
            decodeSession.AbortFramePackage();
            return result;
        }
        if (!leafResult.success) {
            if (result.messages.empty()) {
                AddDecodePackageMessage(
                    result,
                    packageRecords,
                    TelemetryMessageSeverity::Error,
                    kPackageDecodeWorkflowOrigin,
                    "failed to decode frame leaf package");
            }
            decodeSession.AbortFramePackage();
            return result;
        }
        assemblyError.clear();
        if (!decodeSession.CommitLeaf(leaf, *leafAdapter, &assemblyError)) {
            AddDecodePackageMessage(
                result,
                packageRecords,
                TelemetryMessageSeverity::Error,
                kPackageDecodeWorkflowOrigin,
                assemblyError.empty() ? "failed to commit frame leaf output" : assemblyError);
            decodeSession.AbortFramePackage();
            return result;
        }
    }

    assemblyError.clear();
    if (request.stopToken.stop_requested()) {
        result.cancelled = true;
        decodeSession.AbortFramePackage();
        return result;
    }
    if (!decodeSession.EndFramePackage(&assemblyError)) {
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            assemblyError.empty() ? "failed to end frame package assembly" : assemblyError);
        decodeSession.AbortFramePackage();
        return result;
    }

    result.success = true;
    return result;
}

[[nodiscard]] inline DecodePackageResult ExecutePackageDecodeWorkflowUnchecked(
    const DecodePackageRequest& request,
    RunRecordEmitter& packageRecords,
    IRunRecordSink* leafRunRecordSink,
    const DataCodecExecutionResources& resources) {
    if (request.stopToken.stop_requested()) {
        DecodePackageResult result;
        result.cancelled = true;
        return result;
    }
    const auto inputValidation = validation::StorageValidator::ValidateDecodeInput(
        request.inputReader.get());
    if (!inputValidation) {
        DecodePackageResult result;
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::InputBoundary)),
            inputValidation.message);
        return result;
    }
    if (request.framePackageMetadata != nullptr) {
        return DecodeFramePackage(
            *request.framePackageMetadata,
            request,
            packageRecords,
            leafRunRecordSink,
            resources);
    }

    PackageInspection packageInspection;
    std::string headerError;
    if (!InspectPackage(*request.inputReader, packageInspection, &headerError)) {
        DecodePackageResult result;
        result.inputBytes = request.inputReader->ByteSize();
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            std::string(validation::DecodeValidationNodeName(
                validation::DecodeValidationNode::FormatAndParams)),
            headerError.empty() ? "failed to read DataCodec package header" : headerError);
        return result;
    }

    if (packageInspection.format == PackageBinaryFormat::FramePackage) {
        FramePackage framePackage;
        std::string frameReadError;
        if (!FramePackageIO::ReadMetadata(*request.inputReader, framePackage, &frameReadError)) {
            DecodePackageResult result;
            result.inputBytes = request.inputReader->ByteSize();
            AddDecodePackageMessage(
                result,
                packageRecords,
                TelemetryMessageSeverity::Error,
                kPackageDecodeWorkflowOrigin,
                frameReadError.empty() ? "failed to read DataCodec frame package" : frameReadError);
            return result;
        }
        return DecodeFramePackage(
            framePackage,
            request,
            packageRecords,
            leafRunRecordSink,
            resources);
    }
    if (packageInspection.format == PackageBinaryFormat::LeafPackage) {
        return DecodeLeafByteRange(
            request,
            packageRecords,
            leafRunRecordSink,
            resources);
    }

    DecodePackageResult result;
    result.inputBytes = request.inputReader->ByteSize();
    AddDecodePackageMessage(
        result,
        packageRecords,
        TelemetryMessageSeverity::Error,
        kPackageDecodeWorkflowOrigin,
        "input does not contain a supported DataCodec package");
    return result;
}

[[nodiscard]] inline DecodePackageResult ExecutePackageDecodeWorkflow(
    const DecodePackageRequest& request,
    const DataCodecExecutionResources& resources = {}) {
    RunRecordDispatcher recordDispatcher;
    recordDispatcher.AddSink(request.runRecordSink);
    RunRecordEmitter packageRecords;
    packageRecords.Reset(
        RunRecordInfo{
            .generatedAtUtc = runrecorddetail::MakeTimestampUtc(),
            .runKind = TelemetryRunKind::Decode,
            .objectName = "Package",
            .meshType = "Package",
        },
        &recordDispatcher);
    packageRecords.BeginRun();
    const auto runStart = callback::Now();
    packageRecords.SubmitProgress(RunProgressRecord{
        .phase = RunProgressPhase::Begin,
        .normalized = 0.0,
        .text = "DataCodec package decode started",
    });
    ProgressRangeRunRecordSink leafRunRecords(
        &recordDispatcher,
        0.0,
        1.0,
        0u,
        0u,
        {},
        packageRecords.RunId());

    ByteRangePrefetchResult prefetchResult;
    const auto appendPrefetchWarning = [
        &prefetchResult,
        &packageRecords](DecodePackageResult& result) {
        if (!prefetchResult.IsError()) { return; }
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Warning,
            kPackageDecodeWorkflowOrigin,
            prefetchResult.error.empty()
                ? "input range prefetch failed"
                : prefetchResult.error);
    };
    const auto abortSessionOnFailure = [&request]() noexcept {
        if (request.session == nullptr) {
            return;
        }
        try {
            request.session->AbortFramePackage();
        } catch (...) {
        }
    };
    DecodePackageResult result;
    try {
        if (request.inputReader != nullptr &&
            request.decodeAllAvailableAttributes &&
            request.execution.enableFullInputPrefetch) {
            prefetchResult = request.inputReader->PrefetchRange(
                0u,
                request.inputReader->ByteSize());
        }
        result = ExecutePackageDecodeWorkflowUnchecked(
            request,
            packageRecords,
            &leafRunRecords,
            resources);
        appendPrefetchWarning(result);
    } catch (const std::bad_alloc&) {
        abortSessionOnFailure();
        result.inputBytes = request.inputReader == nullptr ? 0u : request.inputReader->ByteSize();
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            "DataCodec package decode failed because memory allocation was rejected");
        appendPrefetchWarning(result);
    } catch (const std::exception& exception) {
        abortSessionOnFailure();
        result.inputBytes = request.inputReader == nullptr ? 0u : request.inputReader->ByteSize();
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            std::string("DataCodec package decode failed: ") + exception.what());
        appendPrefetchWarning(result);
    } catch (...) {
        abortSessionOnFailure();
        result.inputBytes = request.inputReader == nullptr ? 0u : request.inputReader->ByteSize();
        AddDecodePackageMessage(
            result,
            packageRecords,
            TelemetryMessageSeverity::Error,
            kPackageDecodeWorkflowOrigin,
            "DataCodec package decode failed with an unknown exception");
        appendPrefetchWarning(result);
    }

    packageRecords.SubmitProgress(RunProgressRecord{
        .phase = RunProgressPhase::Finish,
        .normalized = 1.0,
        .text = result.success
            ? "DataCodec package decode completed"
            : "DataCodec package decode failed",
        .success = result.success,
    });
    packageRecords.EndRun(RunEndRecord{
        .success = result.success,
        .elapsedMs = callback::ElapsedMilliseconds(runStart),
        .inputBytes = result.inputBytes,
    });
    return result;
}

} // namespace datacodec

#endif
