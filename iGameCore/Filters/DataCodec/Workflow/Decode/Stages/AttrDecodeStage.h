#ifndef DATACODEC_WORKFLOW_DECODE_STAGES_ATTRDECODESTAGE_H
#define DATACODEC_WORKFLOW_DECODE_STAGES_ATTRDECODESTAGE_H

#include "DataCodec/Codec/Attributes/AttributeDecode.h"
#include "DataCodec/Codec/Attributes/AttributeReferenceDecode.h"
#include "DataCodec/Codec/SubCodec/ZstdCodec.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Runtime/Failure/DecodeFailureManagement.h"
#include "DataCodec/Storage/LeafPackage/LeafPackageFieldDecodeStream.h"
#include "DataCodec/Workflow/Decode/Stages/FieldDecodeInput.h"
#include "DataCodec/Workflow/Common/PipelineStageBase.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {

// [DC防护:阶段] attribute decode 入口校验字段 raw size 和 params 声明的字节数契约
inline bool ValidateAttributeFieldSize(
    const CodecStorageParams& params,
    const LeafPackageField& field,
    std::string* error = nullptr) {
    ParamSize expectedBytes = 0u;
    if (!CalculateAttributePayloadBytes(params, expectedBytes, error)) {
        return false;
    }
    if (static_cast<std::uint64_t>(field.rawSize) != expectedBytes) {
        return validation::AssignError(error, "attribute field raw size does not match params binaryCount");
    }
    return true;
}

inline ContiguousViewStatus PrepareDirectAttributePayloadView(
    const LeafPackageField& field,
    std::span<const std::uint8_t>& payloadBytes,
    std::string* error = nullptr) {
    payloadBytes = {};
    if (field.compressionType != EncodedFieldCompressionType::None) {
        if (error != nullptr) { error->clear(); }
        return ContiguousViewStatus::Unavailable;
    }
    if (field.source == nullptr) {
        validation::AssignError(error, "attribute payload source is missing");
        return ContiguousViewStatus::Error;
    }
    if (field.rawSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        validation::AssignError(error, "attribute payload exceeds local address space");
        return ContiguousViewStatus::Error;
    }
    std::span<const std::uint8_t> directBytes;
    const auto status = field.source->PrepareContiguousBytes(directBytes, error);
    if (status != ContiguousViewStatus::Ready) {
        return status;
    }
    if (directBytes.size() != static_cast<std::size_t>(field.rawSize)) {
        validation::AssignError(error, "attribute contiguous payload size does not match recorded raw size");
        return ContiguousViewStatus::Error;
    }
    payloadBytes = directBytes;
    return ContiguousViewStatus::Ready;
}

inline bool SpoolAttributePayloadToByteStore(
    const LeafPackageField& field,
    DecodeLeafWorkspace& workspace,
    const AttributeDecodePayloadMode payloadMode,
    std::shared_ptr<bytestore::IByteSource>& payloadOwner,
    std::span<const std::uint8_t>& payloadBytes,
    std::string* error = nullptr) {
    payloadOwner.reset();
    payloadBytes = {};
    if (field.rawSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return validation::AssignError(error, "attribute payload exceeds local address space");
    }

    const auto memoryPayloadLimit = workspace.ResourceBudget().AttributeDecodeMemoryPayloadLimitBytes();
    std::shared_ptr<bytestore::IByteStore> payloadStore;
    switch (payloadMode) {
        case AttributeDecodePayloadMode::Memory:
            if (memoryPayloadLimit == 0u || field.rawSize > memoryPayloadLimit) {
                return validation::AssignError(error, "attribute payload memory store exceeds configured memory limit");
            }
            payloadStore = workspace.ByteStoreSessionRef().CreateMemoryStore();
            break;
        case AttributeDecodePayloadMode::Managed:
            payloadStore = workspace.ByteStoreSessionRef().CreateManagedByteStore(
                "decoded_attribute_payload_raw",
                error);
            break;
        case AttributeDecodePayloadMode::OneShotZstd:
        default:
            return validation::AssignError(error, "attribute payload byte store mode is invalid");
    }
    if (payloadStore == nullptr) {
        if (error != nullptr && !error->empty()) {
            return false;
        }
        return validation::AssignError(error, "failed to allocate attribute payload store");
    }
    try {
        if (!payloadStore->ResizeBytes(field.rawSize, error)) {
            return false;
        }
    } catch (const std::exception& exception) {
        return validation::AssignError(error, std::string("failed to resize attribute payload store: ") + exception.what());
    } catch (...) {
        return validation::AssignError(error, "failed to resize attribute payload store");
    }

    decodefield::FieldDecodeStreamReader reader;
    if (!decodefield::OpenLeafPackageFieldDecodeStream(
            field,
            workspace.CacheResourcesRef(),
            reader,
            error)) {
        return false;
    }

    std::uint64_t copiedBytes = 0u;
    for (;;) {
        decodefield::FieldOutputSegment segment;
        bool hasSegment = false;
        if (!reader.ReadNext(segment, hasSegment, error)) {
            return false;
        }
        if (!hasSegment) {
            break;
        }
        if (segment.rawOffset > field.rawSize ||
            segment.bytes.size() > field.rawSize - segment.rawOffset) {
            return validation::AssignError(error, "attribute payload segment exceeds recorded raw size");
        }
        if (!segment.bytes.empty() &&
            !payloadStore->WriteBytesAt(segment.rawOffset, segment.bytes, error)) {
            return false;
        }
        if (!validation::CheckedAddU64(
                copiedBytes,
                static_cast<std::uint64_t>(segment.bytes.size()),
                copiedBytes,
                "attribute payload copied bytes",
                error)) {
            return false;
        }
    }
    if (copiedBytes != field.rawSize) {
        return validation::AssignError(error, "attribute payload decoded size does not match recorded raw size");
    }
    if (!payloadStore->Seal(error)) {
        return false;
    }
    payloadOwner = payloadStore;
    std::string contiguousError;
    const auto contiguousStatus = payloadStore->PrepareContiguousBytes(
        payloadBytes,
        &contiguousError);
    if (contiguousStatus == ContiguousViewStatus::Error) {
        return validation::AssignError(
            error,
            contiguousError.empty()
                ? "attribute payload store contiguous view failed"
                : contiguousError);
    }
    if (contiguousStatus == ContiguousViewStatus::Unavailable) {
        payloadBytes = {};
    }
    if ((!payloadBytes.empty() && payloadBytes.size() != static_cast<std::size_t>(field.rawSize)) ||
        payloadOwner->ByteSizeHint() != field.rawSize ||
        !payloadOwner->CanRead()) {
        return validation::AssignError(error, "attribute payload store does not expose the recorded byte range");
    }
    return true;
}

inline bool CanPrepareOneShotZstdAttributePayload(
    const LeafPackageField& field,
    const DecodeLeafWorkspace& workspace) {
    if (field.compressionType != EncodedFieldCompressionType::ZSTD ||
        field.source == nullptr ||
        !field.source->CanRead() ||
        field.rawSize > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return false;
    }

    const auto memoryPayloadLimit = workspace.ResourceBudget().AttributeDecodeMemoryPayloadLimitBytes();
    const auto encodedSize = field.source->ByteSizeHint();
    return memoryPayloadLimit > 0u &&
        field.rawSize <= memoryPayloadLimit &&
        encodedSize <= memoryPayloadLimit &&
        encodedSize <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());
}

inline const char* AttributePayloadModeName(const AttributeDecodePayloadMode mode) noexcept {
    switch (mode) {
        case AttributeDecodePayloadMode::Memory:
            return "memory_store";
        case AttributeDecodePayloadMode::Managed:
            return "managed";
        case AttributeDecodePayloadMode::OneShotZstd:
            return "one_shot_zstd";
        default:
            return "unknown";
    }
}

inline bool SpoolOneShotZstdAttributePayloadToVector(
    const LeafPackageField& field,
    std::shared_ptr<bytestore::IByteSource>& payloadOwner,
    std::span<const std::uint8_t>& payloadBytes,
    std::string* error = nullptr) {
    payloadOwner.reset();
    payloadBytes = {};
    if (field.source == nullptr) {
        return validation::AssignError(error, "attribute payload source is missing");
    }

    const auto encodedSize = field.source->ByteSizeHint();
    std::size_t encodedByteCount = 0u;
    std::size_t decodedByteCount = 0u;
    if (!validation::CheckedCastSizeT(encodedSize, encodedByteCount, "attribute payload encoded size", error) ||
        !validation::CheckedCastSizeT(field.rawSize, decodedByteCount, "attribute payload decoded size", error)) {
        return false;
    }
    try {
        std::span<const std::uint8_t> encodedBytes;
        std::vector<std::uint8_t> ownedEncodedBytes;
        std::string contiguousError;
        const auto contiguousStatus = field.source->PrepareContiguousBytes(
            encodedBytes,
            &contiguousError);
        if (contiguousStatus == ContiguousViewStatus::Error) {
            return validation::AssignError(
                error,
                contiguousError.empty()
                    ? "attribute encoded payload contiguous view failed"
                    : contiguousError);
        }
        if (contiguousStatus == ContiguousViewStatus::Ready &&
            encodedBytes.size() != encodedByteCount) {
            return validation::AssignError(
                error,
                "attribute encoded payload contiguous size does not match recorded size");
        }
        if (contiguousStatus == ContiguousViewStatus::Unavailable) {
            ownedEncodedBytes.resize(encodedByteCount);
            if (!ownedEncodedBytes.empty() &&
                !field.source->Read(
                    0u,
                    std::span<std::uint8_t>(ownedEncodedBytes.data(), ownedEncodedBytes.size()),
                    error)) {
                return false;
            }
            encodedBytes = std::span<const std::uint8_t>(
                ownedEncodedBytes.data(),
                ownedEncodedBytes.size());
        }

        std::vector<std::uint8_t> decodedBytes;
        if (!codec::ZstdCodec::Decompress(
                encodedBytes,
                decodedByteCount,
                decodedBytes,
                error)) {
            return false;
        }

        auto vectorSource = std::make_shared<bytestore::VectorByteSource>(std::move(decodedBytes));
        if (vectorSource->PrepareContiguousBytes(payloadBytes, error) !=
            ContiguousViewStatus::Ready) {
            return false;
        }
        payloadOwner = std::move(vectorSource);
        return true;
    } catch (const std::exception& exception) {
        return validation::AssignError(
            error,
            std::string("failed to allocate one-shot attribute payload buffer: ") + exception.what());
    } catch (...) {
        return validation::AssignError(error, "failed to allocate one-shot attribute payload buffer");
    }
}

inline const char* AttributeAttachmentName(const AttrAttachment attachment) noexcept {
    switch (attachment) {
        case AttrAttachment::Point:
            return "point";
        case AttrAttachment::Cell:
            return "cell";
        default:
            return "unknown";
    }
}

inline const char* DataTypeName(const DataType dataType) noexcept {
    switch (dataType) {
        case DataType::Float32:
            return "float32";
        case DataType::Float64:
            return "float64";
        case DataType::Int32:
            return "int32";
        case DataType::Int64:
            return "int64";
        case DataType::UInt32:
            return "uint32";
        case DataType::UInt64:
            return "uint64";
        case DataType::Int8:
            return "int8";
        case DataType::UInt8:
            return "uint8";
        case DataType::Int16:
            return "int16";
        case DataType::UInt16:
            return "uint16";
        default:
            return "unknown";
    }
}

inline std::string FormatAttributeDecodeWorkerIndex(const std::size_t workerIndex) {
    return workerIndex == kInvalidParallelWorkerIndex
        ? std::string("unknown")
        : std::to_string(workerIndex);
}

inline ParamSize BytesPerElementMilli(const ParamSize byteCount, const ParamSize elementCount) noexcept {
    if (elementCount == 0u) {
        return 0u;
    }
    if (byteCount > std::numeric_limits<ParamSize>::max() / 1000u) {
        return std::numeric_limits<ParamSize>::max();
    }
    return byteCount * 1000u / elementCount;
}

inline std::string FormatAttributeDecodeTimingScope(
    const decodeimpl::detail::AttributeDecodeTimingDetail& detail) {
    const auto name = detail.name.empty() ? std::string("<unnamed>") : detail.name;
    return "name=" + name +
        ";attachment=" + AttributeAttachmentName(detail.attachmentType) +
        ";dataType=" + DataTypeName(detail.dataType) +
        ";elements=" + std::to_string(detail.elementCount) +
        ";dimension=" + std::to_string(detail.dimension) +
        ";valueSize=" + std::to_string(detail.valueSize) +
        ";rawBytes=" + std::to_string(detail.rawValueBytes) +
        ";binaryBytes=" + std::to_string(detail.binaryCount) +
        ";encodedBlockBytes=" + std::to_string(detail.encodedBlockBytes) +
        ";binaryBytesPerElementX1000=" + std::to_string(
            BytesPerElementMilli(detail.binaryCount, detail.elementCount)) +
        ";estimatedDecodeCost=" + std::to_string(detail.estimatedDecodeCost) +
        ";criticalPathCost=" + std::to_string(detail.criticalPathCost) +
        ";readyOffsetMs=" + std::to_string(detail.readyOffsetMs) +
        ";startOffsetMs=" + std::to_string(detail.startOffsetMs) +
        ";finishOffsetMs=" + std::to_string(detail.finishOffsetMs) +
        ";readyWaitMs=" + std::to_string(detail.readyWaitMs) +
        ";workerIndex=" + FormatAttributeDecodeWorkerIndex(detail.workerIndex) +
        ";blocks=" + std::to_string(detail.blockCount) +
        ";nonRefBlocks=" + std::to_string(detail.nonReferenceBlocks) +
        ";refBlocks=" + std::to_string(detail.referenceBlocks) +
        ";intraRefBlocks=" + std::to_string(detail.intraReferenceBlocks) +
        ";temporalRefBlocks=" + std::to_string(detail.temporalReferenceBlocks) +
        ";affineBlocks=" + std::to_string(detail.affineReferenceBlocks) +
        ";waveletBlocks=" + std::to_string(detail.waveletReferenceBlocks) +
        ";predictorBlocks=" + std::to_string(detail.predictorReferenceBlocks) +
        ";layeredResidualBlocks=" + std::to_string(detail.layeredResidualBlocks) +
        ";regionLayers=" + std::to_string(detail.regionLayerCount) +
        ";componentLayouts=" + std::to_string(detail.componentLayoutCount) +
        ";payloadReadMs=" + std::to_string(detail.payloadBlockReadMs) +
        ";ordinaryDecodeMs=" + std::to_string(detail.ordinaryDecodeMs) +
        ";referenceResolveMs=" + std::to_string(detail.referenceResolveMs) +
        ";temporalKeyEnsureMs=" + std::to_string(detail.temporalKeyReferenceEnsureMs) +
        ";referenceRangeResolveMs=" + std::to_string(detail.referenceRangeResolveMs) +
        ";referenceDecodeMs=" + std::to_string(detail.referenceDecodeMs) +
        ";affineReferenceDecodeMs=" + std::to_string(detail.affineReferenceDecodeMs) +
        ";waveletReferenceDecodeMs=" + std::to_string(detail.waveletReferenceDecodeMs) +
        ";predictorReferenceDecodeMs=" + std::to_string(detail.predictorReferenceDecodeMs) +
        ";cacheWriteMs=" + std::to_string(detail.cacheWriteMs) +
        ";ordinaryDecodedBytes=" + std::to_string(detail.ordinaryDecodedBytes) +
        ";referenceDecodedBytes=" + std::to_string(detail.referenceDecodedBytes) +
        ";referenceBytesRead=" + std::to_string(detail.referenceBytesRead) +
        ";intraReferenceWorksetBytes=" + std::to_string(detail.intraReferenceWorksetBytes) +
        ";temporalReferenceWorksetBytes=" + std::to_string(detail.temporalReferenceWorksetBytes) +
        ";resampledReferenceWorksetBytes=" + std::to_string(detail.resampledReferenceWorksetBytes) +
        ";predictorRangeStagingBytes=" + std::to_string(detail.predictorRangeStagingBytes) +
        ";predictorShiftedCopyBytes=" + std::to_string(detail.predictorShiftedCopyBytes) +
        ";waveletLowBlobBytes=" + std::to_string(detail.waveletLowBlobBytes) +
        ";waveletHighBlobBytes=" + std::to_string(detail.waveletHighBlobBytes) +
        ";waveletPeakTemporaryDoubleBytes=" + std::to_string(detail.waveletPeakTemporaryDoubleBytes) +
        ";cacheWriteBytes=" + std::to_string(detail.cacheWriteBytes) +
        ";temporalKeyFieldCacheHits=" + std::to_string(detail.temporalKeyFieldCacheHits) +
        ";temporalKeyFieldCacheMisses=" + std::to_string(detail.temporalKeyFieldCacheMisses) +
        ";predictorZeroOffsetBlocks=" + std::to_string(detail.predictorZeroOffsetBlocks) +
        ";predictorContinuousShiftBlocks=" + std::to_string(detail.predictorContinuousShiftBlocks) +
        ";predictorBoundaryClampBlocks=" + std::to_string(detail.predictorBoundaryClampBlocks);
}

inline bool PrepareDirectAttributeDecodeStores(
    DecodeContext& context,
    DecodeLeafWorkspace& workspace,
    std::string* error = nullptr) {
    if (context.adapter == nullptr || !context.adapter->SupportsAttributeDecodeStore()) {
        return true;
    }
    const auto attrIndices = ResolveAttributeDecodeIndices(
        context.attributeTargets,
        context.frameIndex,
        context.leafPackage != nullptr ? context.leafPackage->path : BlockPath{},
        context.attributeSelection,
        workspace.StorageParams().attrParams.size());
    if (attrIndices.empty()) {
        return true;
    }
    if (!workspace.attributes.IsInitialized() &&
        !workspace.attributes.Initialize(
            workspace.StorageParams(),
            workspace.ByteStoreSessionRef(),
            workspace.ResourceBudget().AttributeDecodeMemoryCacheLimitBytes(),
            workspace.ResourceBudget().AttributeDecodeCacheStorageMode(),
            error)) {
        return false;
    }
    for (const auto attrIndex : attrIndices) {
        if (attrIndex >= workspace.StorageParams().attrParams.size()) {
            return validation::AssignError(error, "attribute decode store index is out of range");
        }
        if (workspace.attributes.Complete(attrIndex) || workspace.attributes.AdapterBacked(attrIndex)) {
            continue;
        }
        auto store = context.adapter->CreateAttributeDecodeStore(
            attrIndex,
            workspace.StorageParams().attrParams[attrIndex],
            error);
        if (store == nullptr ||
            !workspace.attributes.BindAttributeStore(
                attrIndex,
                std::move(store),
                true,
                error)) {
            return false;
        }
    }
    return true;
}

class AttrDecodeStage final : public DecodeStage {
public:
    static constexpr std::string_view kTypeName = "AttrDecodeStage";

    AttrDecodeStage() = default;
    explicit AttrDecodeStage(FieldDecodeInput input) : m_input(input) {}

    const char* Name() const override { return "AttrDecodeStage"; }
    [[nodiscard]] bool UsesInternalParallelism() const noexcept override { return true; }

    // 把属性 domain 按 block 解入 decoded attribute cache
    void Execute(DecodeContext& context, DecodeLeafWorkspace& workspace) override {
        const auto targetAttrIndices = ResolveAttributeDecodeIndices(
            context.attributeTargets,
            context.frameIndex,
            context.leafPackage != nullptr ? context.leafPackage->path : BlockPath{},
            context.attributeSelection,
            workspace.StorageParams().attrParams.size());
        if (targetAttrIndices.empty()) {
            return;
        }
        for (const auto attrIndex : targetAttrIndices) {
            if (attrIndex >= workspace.StorageParams().attrParams.size()) {
                FailDecodeStage(
                    context,
                    workspace,
                    "AttrDecodeStage",
                    CodecErrorCode::InvalidInput,
                    "attribute target index is out of range");
                return;
            }
        }
        if (!m_input.HasField()) {
            if (!workspace.StorageParams().attrParams.empty()) {
                FailDecodeStage(
                    context,
                    workspace,
                    "AttrDecodeStage",
                    CodecErrorCode::MissingInput,
                    "failed to find attribute field");
            }
            return;
        }

        std::string sizeError;
        if (!ValidateAttributeFieldSize(workspace.StorageParams(), *m_input.field, &sizeError)) {
            FailDecodeStage(
                context,
                workspace,
                "AttrDecodeStage",
                CodecErrorCode::DecodeFailure,
                "failed to validate attribute field size: " + sizeError);
            return;
        }

        if (workspace.StorageParams().attrParams.empty()) {
            return;
        }

        auto* attributeKeyFrameReference = context.attributeKeyFrameReference;
        std::span<const std::uint8_t> payloadBytes;
        std::shared_ptr<bytestore::IByteSource> payloadOwner;
        std::string payloadError;
        std::string payloadMode = "direct";
        const auto collectTiming = context.runRecords.Wants(RunRecordKind::StageTiming);
        if (collectTiming) {
            const auto requestedWorkers = workspace.ResourceBudget().AttributeDecodeLaneCount();
            const auto runnerConcurrency = context.parallelTaskRunner != nullptr
                ? context.parallelTaskRunner->Concurrency()
                : 0u;
            const auto resolvedWorkers = ResolveParallelTaskCount(
                targetAttrIndices.size(),
                context.parallelTaskRunner,
                requestedWorkers);
            context.runRecords.RecordStageTiming(
                "AttrDecodeSchedule",
                0.0,
                TelemetryStageCategory::General,
                "requestedWorkers=" + std::to_string(requestedWorkers) +
                    ";runnerConcurrency=" + std::to_string(runnerConcurrency) +
                    ";resolvedWorkers=" + std::to_string(resolvedWorkers) +
                    ";currentWorkerIndex=" + std::to_string(CurrentParallelWorkerIndex()));
        }
        const auto spoolStart = callback::StartTiming(collectTiming);
        const auto preparedCached = workspace.PreparedAttributePayload(
            m_input.field,
            payloadOwner,
            payloadBytes);
        auto directStatus = ContiguousViewStatus::Unavailable;
        if (!preparedCached) {
            directStatus = PrepareDirectAttributePayloadView(
                *m_input.field,
                payloadBytes,
                &payloadError);
        }
        if (preparedCached) {
            payloadMode = "cached";
        } else if (directStatus == ContiguousViewStatus::Error) {
            FailDecodeStage(
                context,
                workspace,
                "AttrDecodeStage",
                CodecErrorCode::PipelineFailure,
                "failed to resolve attribute payload path: " + payloadError);
            return;
        } else if (directStatus == ContiguousViewStatus::Ready) {
            payloadOwner = m_input.field->source;
            workspace.SetPreparedAttributePayload(
                m_input.field,
                payloadOwner,
                payloadBytes);
        } else if (m_input.field->compressionType == EncodedFieldCompressionType::None &&
                   m_input.field->source != nullptr &&
                   m_input.field->source->CanRead() &&
                   m_input.field->source->ByteSizeHint() == m_input.field->rawSize) {
            payloadMode = "ranged_raw";
            payloadOwner = m_input.field->source;
            workspace.SetPreparedAttributePayload(m_input.field, payloadOwner, {});
        } else {
            const auto configuredPayloadMode = workspace.ResourceBudget().AttributeDecodePayloadStorageMode();
            payloadMode = AttributePayloadModeName(configuredPayloadMode);
            bool preparedPayload = false;
            switch (configuredPayloadMode) {
                case AttributeDecodePayloadMode::OneShotZstd:
                    if (!CanPrepareOneShotZstdAttributePayload(*m_input.field, workspace)) {
                        payloadError = "attribute payload one-shot zstd mode is unavailable or exceeds configured memory limit";
                        break;
                    }
                    preparedPayload = SpoolOneShotZstdAttributePayloadToVector(
                        *m_input.field,
                        payloadOwner,
                        payloadBytes,
                        &payloadError);
                    break;
                case AttributeDecodePayloadMode::Memory:
                case AttributeDecodePayloadMode::Managed:
                    preparedPayload = SpoolAttributePayloadToByteStore(
                        *m_input.field,
                        workspace,
                        configuredPayloadMode,
                        payloadOwner,
                        payloadBytes,
                        &payloadError);
                    break;
                default:
                    payloadError = "attribute payload mode is invalid";
                    break;
            }
            if (!preparedPayload) {
                FailDecodeStage(
                    context,
                    workspace,
                    "AttrDecodeStage",
                    CodecErrorCode::PipelineFailure,
                    "failed to prepare attribute payload: " + payloadError);
                return;
            }
            workspace.SetPreparedAttributePayload(
                m_input.field,
                payloadOwner,
                payloadBytes);
        }
        const auto payloadByteSize = !payloadBytes.empty()
            ? static_cast<std::uint64_t>(payloadBytes.size())
            : payloadOwner != nullptr ? payloadOwner->ByteSizeHint() : 0u;
        if (payloadByteSize != m_input.field->rawSize ||
            (payloadBytes.empty() && (payloadOwner == nullptr || !payloadOwner->CanRead()))) {
            FailDecodeStage(
                context,
                workspace,
                "AttrDecodeStage",
                CodecErrorCode::PipelineFailure,
                "failed to prepare attribute payload: payload size does not match recorded raw size");
            return;
        }
        if (collectTiming) {
            context.runRecords.RecordStageTiming(
                "AttrPayloadPrepareStage",
                callback::ElapsedMilliseconds(spoolStart),
                TelemetryStageCategory::General,
                std::move(payloadMode));
        }

        AttributeReferenceDecodeHelper referenceDecoder(workspace.CacheResourcesRef());

        decodeimpl::detail::AttributeDecodeTimingCallback attributeTimingCallback;
        if (collectTiming) {
            attributeTimingCallback = [&context](const decodeimpl::detail::AttributeDecodeTimingDetail& detail) {
                context.runRecords.RecordStageTiming(
                    "AttrDecodeStage[" + std::to_string(detail.attrIndex) + "]",
                    detail.elapsedMs,
                    TelemetryStageCategory::General,
                    FormatAttributeDecodeTimingScope(detail));
            };
        }

        std::string decodeError;
        decodeimpl::detail::AttributePayloadDecodeRuntime decodeRuntime{
            .data = decodeimpl::detail::AttributeDecodeData{
                .storageParams = workspace.StorageParams(),
                .attributeKeyFrameReference = attributeKeyFrameReference,
            },
            .schedule = decodeimpl::detail::AttributeDecodeSchedule{
                .workerCount = workspace.ResourceBudget().AttributeDecodeLaneCount(),
                .parallelTaskRunner = context.parallelTaskRunner,
            },
            .cache = decodeimpl::detail::AttributeDecodeCache{
                .cacheResources = workspace.CacheResourcesRef(),
                .byteStoreSession = workspace.ByteStoreSessionRef(),
                .attributes = workspace.attributes,
                .attributeMemoryCacheLimitBytes = workspace.ResourceBudget().AttributeDecodeMemoryCacheLimitBytes(),
                .attributeCacheStorageMode = workspace.ResourceBudget().AttributeDecodeCacheStorageMode(),
            },
            .context = decodeimpl::detail::AttributeDecodeContext{
                .timingCallback = std::move(attributeTimingCallback),
            },
        };
        const auto decoded = !payloadBytes.empty()
            ? decodeimpl::detail::DecodeAttributePayloadRangesToCache(
                decodeRuntime,
                payloadBytes,
                targetAttrIndices,
                referenceDecoder,
                &decodeError)
            : decodeimpl::detail::DecodeAttributePayloadRangesToCache(
                decodeRuntime,
                *payloadOwner,
                targetAttrIndices,
                referenceDecoder,
                &decodeError);
        if (!decoded) {
            FailDecodeStage(
                context,
                workspace,
                "AttrDecodeStage",
                CodecErrorCode::PipelineFailure,
                "failed to decode attribute: " + decodeError);
            return;
        }
        if (collectTiming) {
            context.runRecords.RecordStageTiming(
                "AttrCacheModeStage",
                0.0,
                TelemetryStageCategory::General,
                workspace.attributes.ByteStoreModeName());
        }
    }

private:
    FieldDecodeInput m_input;
};

} // namespace datacodec

#endif
