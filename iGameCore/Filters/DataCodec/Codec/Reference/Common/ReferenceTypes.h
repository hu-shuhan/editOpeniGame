#ifndef DATACODEC_CODEC_REFERENCE_COMMON_REFERENCETYPES_H
#define DATACODEC_CODEC_REFERENCE_COMMON_REFERENCETYPES_H

#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/API/Params/NumericArrayParams.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockWireFormat.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <variant>
#include <vector>
namespace datacodec {

enum class NumericArrayReferenceScope : std::uint8_t {
    None = 0,
    IntraArray = 1,
    TemporalKeyFrame = 2,
};

inline NumericArrayReferenceKind ToNumericArrayReferenceKind(const NumericArrayReferenceScope scope) {
    switch (scope) {
        case NumericArrayReferenceScope::IntraArray:
            return NumericArrayReferenceKind::IntraArray;
        case NumericArrayReferenceScope::TemporalKeyFrame:
            return NumericArrayReferenceKind::TemporalKeyFrame;
        case NumericArrayReferenceScope::None:
        default:
            return NumericArrayReferenceKind::None;
    }
}

struct NumericArrayReferenceCandidate {
    NumericArrayReferenceScope scope{NumericArrayReferenceScope::None};
    std::uint32_t referenceFrameIndex{0};
    std::uint16_t localParentFieldIndex{0xFFFFu};

    [[nodiscard]] bool HasReference() const noexcept {
        return scope != NumericArrayReferenceScope::None;
    }
};

struct NumericArrayReferenceCodecControl {
    double affineBlockRSquared{0.95};
};

struct NumericArrayReferenceCodecEncodeInput {
    const NumericArrayStorageParams& meta;
    CompressorConfig defaultCompressor;
    ScratchByteBufferPool& scratchBytePool;
    ScratchByteQuotaAcquire acquireScratchQuota;
    NumericArrayReferenceCodecControl control;
    std::span<const std::uint8_t> currentBytes;
    std::span<const std::uint8_t> referenceBytes;
    std::uint32_t elementOffset{0};
    std::uint32_t elementCount{0};
    std::size_t componentCount{0};
    NumericArrayReferenceKind referenceKind{NumericArrayReferenceKind::None};
    std::uint16_t localParentFieldIndex{0xFFFFu};
    std::int32_t predictorOffset{0};

    [[nodiscard]] ScratchByteBuffer AcquireScratch(const std::uint64_t bytes) const {
        return scratchBytePool.Acquire(
            static_cast<std::size_t>(bytes),
            acquireScratchQuota ? acquireScratchQuota(bytes) : ScratchByteQuotaLease{});
    }
};

struct NumericArrayReferencePreparedBlock {
    NumericArrayReferenceCodecId codecId{NumericArrayReferenceCodecId::NonReference};
    CompressorConfig residualCompressor;
    std::vector<double> affineAlpha;
    std::vector<double> affineBeta;
    ScratchByteBuffer deltaRawBytes;

    void Reset() {
        codecId = NumericArrayReferenceCodecId::NonReference;
        residualCompressor = {};
        affineAlpha.clear();
        affineBeta.clear();
        deltaRawBytes = {};
    }
};

enum class NumericArrayReferenceEncodeStatus : std::uint8_t {
    Encoded = 0,
    Rejected = 1,
    Failed = 2,
};

enum class NumericArrayReferenceRejectReason : std::uint8_t {
    None = 0,
    InsufficientModelFit = 1,
    PrecisionBudgetUnavailable = 2,
};

struct NumericArrayReferenceEncodeResult {
    NumericArrayReferenceEncodeStatus status{NumericArrayReferenceEncodeStatus::Failed};
    NumericArrayReferenceRejectReason rejectReason{NumericArrayReferenceRejectReason::None};

    [[nodiscard]] bool IsEncoded() const noexcept {
        return status == NumericArrayReferenceEncodeStatus::Encoded;
    }

    [[nodiscard]] bool IsRejected() const noexcept {
        return status == NumericArrayReferenceEncodeStatus::Rejected;
    }

    [[nodiscard]] bool IsFailed() const noexcept {
        return status == NumericArrayReferenceEncodeStatus::Failed;
    }

    [[nodiscard]] static NumericArrayReferenceEncodeResult Encoded() noexcept {
        return {.status = NumericArrayReferenceEncodeStatus::Encoded};
    }

    [[nodiscard]] static NumericArrayReferenceEncodeResult Rejected(
        const NumericArrayReferenceRejectReason reason) noexcept {
        return {
            .status = NumericArrayReferenceEncodeStatus::Rejected,
            .rejectReason = reason,
        };
    }

    [[nodiscard]] static NumericArrayReferenceEncodeResult Failed() noexcept {
        return {.status = NumericArrayReferenceEncodeStatus::Failed};
    }
};

[[nodiscard]] inline const char* NumericArrayReferenceRejectReasonName(
    const NumericArrayReferenceRejectReason reason) noexcept {
    switch (reason) {
        case NumericArrayReferenceRejectReason::InsufficientModelFit:
            return "InsufficientModelFit";
        case NumericArrayReferenceRejectReason::PrecisionBudgetUnavailable:
            return "PrecisionBudgetUnavailable";
        case NumericArrayReferenceRejectReason::None:
        default:
            return "None";
    }
}

struct NumericArrayReferenceCodecDecodeTelemetry {
    ParamSize waveletLowBlobBytes{0u};
    ParamSize waveletHighBlobBytes{0u};
    ParamSize waveletPeakTemporaryDoubleBytes{0u};
};

struct NumericArrayReferenceCodecDecodeInput {
    const NumericArrayStorageParams& meta;
    const ParsedNumericArrayBlock& block;
    std::span<const std::uint8_t> referenceBytes;
    std::size_t referenceElementOffset{0};
    NumericArrayReferenceCodecDecodeTelemetry* telemetry{nullptr};
};

struct AffineReferenceBlockFields {
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;
    std::vector<double> alpha;
    std::vector<double> beta;
    ScratchByteBuffer deltaBytes;
};

struct WaveletReferenceBlockFields {
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;
    ScratchByteBuffer waveletBytes;
};

struct PredictorReferenceBlockFields {
    std::int32_t predictorOffset{0};
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;
    ScratchByteBuffer deltaBytes;
};

using NumericArrayReferenceBlockFields = std::variant<
    AffineReferenceBlockFields,
    WaveletReferenceBlockFields,
    PredictorReferenceBlockFields>;

struct NumericArrayReferenceEncodedBlock {
    NumericArrayBlockHeader header;
    CompressorConfig backgroundCompressor;
    NumericArrayReferenceBlockFields fields;
};

} // namespace datacodec

#endif
