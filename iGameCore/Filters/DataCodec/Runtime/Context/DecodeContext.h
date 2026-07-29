#ifndef DATACODEC_RUNTIME_CONTEXT_DECODECONTEXT_H
#define DATACODEC_RUNTIME_CONTEXT_DECODECONTEXT_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Adapter/IDecodeAdapter.h"
#include "DataCodec/API/Adapter/IDecodeTopologyBlockObserver.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Runtime/Failure/FailureCleanable.h"
#include "DataCodec/Codec/Reference/DecodedReference.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
#include "DataCodec/Runtime/Workspace/DecodeLeafWorkspace.h"
#include "DataCodec/Runtime/Record/RunRecordEmitter.h"
#include "DataCodec/Runtime/Record/RunRecordTimestamp.h"

#include <cassert>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
namespace datacodec {

class TelemetryMemoryTraceRecorder;
class IParallelTaskRunner;

struct DecodeContextInitializeResult {
    bool success{true};
    std::string message;

    explicit operator bool() const noexcept { return success; }
};

struct DecodeFailureState {
    CodecErrorCode code{CodecErrorCode::DecodeFailure};
    std::string stageName;
    std::string message;
    std::string formattedMessage;
};

struct DecodeContext : IFailureCleanable {
    IDecodeAdapter* adapter{nullptr};
    const LeafPackage* leafPackage{nullptr};
    DecodedAttributeReference* attributeKeyFrameReference{nullptr};
    DecodedGeometryReference* geometryKeyFrameReference{nullptr};
    DecodedGeometryReferenceCache* currentGeometryReferenceCache{nullptr};
    DecodedTopologyReferenceCacheStore* topologyReferenceStore{nullptr};
    std::string topologyReferenceKey;
    std::uint32_t frameIndex{0u};
    AttributeSelectionMode attributeSelection{AttributeSelectionMode::None};
    std::span<const AttributeTarget> attributeTargets;
    AttributeDecodeRequestMode attributeRequestMode{AttributeDecodeRequestMode::DecodeAndCommit};
    RunRecordEmitter runRecords;
    RunEndRecord runSummary;
    TelemetryMemoryTraceRecorder* memoryTrace{nullptr};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
    TopologyDecodeOutputMode topologyOutputMode{TopologyDecodeOutputMode::CommitToAdapter};
    std::shared_ptr<IDecodeTopologyBlockObserver> topologyBlockObserver;
    std::optional<DecodeFailureState> failure;
    mutable std::mutex failureMutex;

    // 校验必要字段并初始化运行记录，调用前需先设置各字段
    DecodeContextInitializeResult Initialize(IRunRecordSink* recordSink) {
        memoryTrace = nullptr;
        runRecords.Reset(
            RunRecordInfo{
                .generatedAtUtc = runrecorddetail::MakeTimestampUtc(),
                .runKind = TelemetryRunKind::Decode,
                .objectName = leafPackage != nullptr ? leafPackage->path : std::string{},
                .leafPath = leafPackage != nullptr ? leafPackage->path : BlockPath{},
            },
            recordSink);
        runSummary = {};
        failure.reset();
        failureCleanupCompleted.store(false, std::memory_order_release);

        if (adapter == nullptr) {
            return {false, "DataCodec decode context requires an adapter"};
        }
        if (leafPackage == nullptr) {
            return {false, "DataCodec decode context requires an leaf package"};
        }
        if (topologyOutputMode == TopologyDecodeOutputMode::ObserverOnly &&
            topologyBlockObserver == nullptr) {
            return {false, "observer-only topology decode requires a topology observer"};
        }
        return {};
    }

    void AddInfo(std::string origin, std::string text) {
        runRecords.AddInfo(std::move(origin), std::move(text));
    }

    void AddWarning(std::string origin, std::string text) {
        runRecords.AddWarning(std::move(origin), std::move(text));
    }

    bool RecordFailure(
        const std::string_view stageName,
        const CodecErrorCode code,
        const std::string_view message) {
        bool isFirstFailure = false;
        {
            std::lock_guard<std::mutex> lock(failureMutex);
            if (!failure.has_value()) {
                failure = DecodeFailureState{
                    .code = code,
                    .stageName = std::string(stageName),
                    .message = std::string(message),
                    .formattedMessage = FormatStageCodecError(stageName, code, message),
                };
                isFirstFailure = true;
            }
        }
        runRecords.AddMessage(MakeCodecTelemetryMessage(std::string(stageName), code, std::string(message)));
        return isFirstFailure;
    }

    bool RecordFailure(
        const std::string_view stageName,
        const CodecErrorCode code,
        const std::string& message) {
        return RecordFailure(stageName, code, std::string_view(message));
    }

    bool RecordFailure(
        const std::string_view stageName,
        const CodecErrorCode code,
        const char* message) {
        assert(message != nullptr);
        return RecordFailure(stageName, code, std::string_view(message));
    }

    [[nodiscard]] bool HasFailure() const {
        std::lock_guard<std::mutex> lock(failureMutex);
        return failure.has_value();
    }

    [[nodiscard]] std::optional<DecodeFailureState> FirstFailure() const {
        std::lock_guard<std::mutex> lock(failureMutex);
        return failure;
    }

    void AddError(std::string origin, std::string text) {
        runRecords.AddError(std::move(origin), std::move(text));
    }

    void CleanupOnFailure() noexcept override {
        if (failureCleanupCompleted.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        if (adapter != nullptr) {
            try {
                adapter->Abort();
            } catch (...) {
            }
        }
    }

    std::atomic_bool failureCleanupCompleted{false};
};

} // namespace datacodec

#endif
