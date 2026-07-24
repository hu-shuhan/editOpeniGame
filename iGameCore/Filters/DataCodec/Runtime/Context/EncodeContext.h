#ifndef DATACODEC_RUNTIME_CONTEXT_ENCODECONTEXT_H
#define DATACODEC_RUNTIME_CONTEXT_ENCODECONTEXT_H

#include "DataCodec/Runtime/Failure/FailureCleanable.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/Codec/Reference/EncodeReferenceFrame.h"
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
#include <vector>
namespace datacodec {

class TelemetryMemoryTraceRecorder;
class IParallelTaskRunner;

struct EncodeContextInitializeResult {
    bool success{true};
    std::string message;

    explicit operator bool() const noexcept { return success; }
};

struct EncodeFailureState {
    CodecErrorCode code{CodecErrorCode::EncodeFailure};
    std::string stageName;
    std::string message;
    std::string formattedMessage;
};

struct EncodeContext : IFailureCleanable {
    IEncodeAdapter* adapter{nullptr};
    const CodecControlParams* controlParams{nullptr};
    IParallelTaskRunner* parallelTaskRunner{nullptr};
    RunRecordEmitter runRecords;
    RunEndRecord runSummary;
    TelemetryMemoryTraceRecorder* memoryTrace{nullptr};
    std::string objectName;
    std::string meshType;
    BlockPath path;
    std::uint32_t frameIndex{0u};
    std::span<const AttributeTarget> attributeTargets;
    TemporalFieldRole attributeTemporalRole{TemporalFieldRole::SingleFrame};
    std::uint32_t attributeKeyFrameIndex{0u};
    TemporalFieldRole geometryTemporalRole{TemporalFieldRole::SingleFrame};
    std::uint32_t geometryKeyFrameIndex{0u};
    EncodeAttributeReferenceFrame attributeKeyFrameReference;
    EncodeGeometryReferenceFrame geometryKeyFrameReference;
    DecodedAttributeCacheSet* currentAttributeReferenceCache{nullptr};
    mutable std::mutex currentAttributeReferenceCacheMutex;
    DecodedGeometryReferenceCache* currentGeometryReferenceCache{nullptr};
    bytestore::ByteStoreSession* referenceByteStoreSession{nullptr};
    mutable std::mutex currentGeometryReferenceCacheMutex;
    std::optional<EncodeFailureState> failure;
    mutable std::mutex failureMutex;

    // 校验必要字段并初始化运行记录，调用前需先设置各字段
    EncodeContextInitializeResult Initialize(IRunRecordSink* recordSink) {
        memoryTrace = nullptr;
        runRecords.Reset(
            RunRecordInfo{
                .generatedAtUtc = runrecorddetail::MakeTimestampUtc(),
                .runKind = TelemetryRunKind::Encode,
                .objectName = objectName,
                .leafPath = path,
                .meshType = meshType,
            },
            recordSink);
        runSummary = {};
        failure.reset();
        failureCleanupCompleted.store(false, std::memory_order_release);

        if (adapter == nullptr) {
            return {false, "DataCodec encode context requires an adapter"};
        }
        return {};
    }

    void AddInfo(std::string origin, std::string text) {
        runRecords.AddInfo(std::move(origin), std::move(text));
    }

    void AddWarning(std::string origin, std::string text) {
        runRecords.AddWarning(std::move(origin), std::move(text));
    }

    void AddError(std::string origin, std::string text) {
        runRecords.AddError(std::move(origin), std::move(text));
    }

    bool RecordFailure(
        const std::string_view stageName,
        const CodecErrorCode code,
        const std::string_view message) {
        bool isFirstFailure = false;
        {
            std::lock_guard<std::mutex> lock(failureMutex);
            if (!failure.has_value()) {
                failure = EncodeFailureState{
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

    [[nodiscard]] std::optional<EncodeFailureState> FirstFailure() const {
        std::lock_guard<std::mutex> lock(failureMutex);
        return failure;
    }

    void ResetCurrentAttributeReferenceCache() noexcept {
        std::lock_guard<std::mutex> lock(currentAttributeReferenceCacheMutex);
        if (currentAttributeReferenceCache != nullptr) {
            currentAttributeReferenceCache->Reset();
        }
    }

    void ResetCurrentGeometryReferenceCache() noexcept {
        std::lock_guard<std::mutex> lock(currentGeometryReferenceCacheMutex);
        if (currentGeometryReferenceCache != nullptr) {
            currentGeometryReferenceCache->Reset();
        }
    }

    void CleanupOnFailure() noexcept override {
        if (failureCleanupCompleted.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        ResetCurrentAttributeReferenceCache();
        ResetCurrentGeometryReferenceCache();
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
