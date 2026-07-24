#ifndef DATACODEC_TEST_COMMON_REFERENCECODECTESTHARNESS_H
#define DATACODEC_TEST_COMMON_REFERENCECODECTESTHARNESS_H

#include "DataCodec/Codec/Reference/ReferenceCodec.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace datacodec::test {

template<typename TValue>
inline std::span<const std::uint8_t> NumericValueBytes(
    const std::vector<TValue>& values) noexcept {
    return std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(values.data()),
        values.size() * sizeof(TValue));
}

template<typename TValue>
inline constexpr DataType NumericValueDataType() noexcept {
    if constexpr (std::is_same_v<TValue, float>) {
        return DataType::Float32;
    } else if constexpr (std::is_same_v<TValue, double>) {
        return DataType::Float64;
    } else if constexpr (std::is_same_v<TValue, std::int32_t>) {
        return DataType::Int32;
    } else if constexpr (std::is_same_v<TValue, std::int64_t>) {
        return DataType::Int64;
    } else if constexpr (std::is_same_v<TValue, std::uint32_t>) {
        return DataType::UInt32;
    } else if constexpr (std::is_same_v<TValue, std::uint64_t>) {
        return DataType::UInt64;
    } else {
        static_assert(!sizeof(TValue), "unsupported reference test numeric type");
    }
}

template<typename TValue>
inline NumericArrayStorageParams MakeReferenceTestMeta(
    const std::size_t tupleCount,
    const std::size_t componentCount) {
    NumericArrayStorageParams meta;
    meta.dataType = NumericValueDataType<TValue>();
    meta.valueSize = sizeof(TValue);
    meta.elementCount = tupleCount;
    meta.dimension = static_cast<std::int32_t>(componentCount);
    return meta;
}

inline std::size_t ReferenceEncodedPayloadBytes(
    const NumericArrayReferenceEncodedBlock& encoded) noexcept {
    return std::visit(
        [](const auto& fields) -> std::size_t {
            using TFields = std::decay_t<decltype(fields)>;
            if constexpr (std::is_same_v<TFields, AffineReferenceBlockFields>) {
                return fields.deltaBytes.Bytes().size();
            } else if constexpr (std::is_same_v<TFields, WaveletReferenceBlockFields>) {
                return fields.waveletBytes.Bytes().size();
            } else {
                return fields.deltaBytes.Bytes().size();
            }
        },
        encoded.fields);
}

template<typename TValue>
inline bool EncodeDecodeReferenceTestBlock(
    const NumericArrayReferenceCodecId codecId,
    const CompressorConfig& compressor,
    ScratchByteBufferPool& scratchBytePool,
    const std::vector<TValue>& current,
    const std::vector<TValue>& reference,
    const std::size_t componentCount,
    const NumericArrayReferenceKind referenceKind,
    NumericArrayReferenceEncodedBlock& encoded,
    std::vector<std::uint8_t>& decodedBytes,
    std::string* error = nullptr) {
    encoded = {};
    decodedBytes.clear();
    if (componentCount == 0u ||
        current.size() != reference.size() ||
        current.empty() ||
        current.size() % componentCount != 0u) {
        return validation::AssignError(error, "reference test input shape is invalid");
    }
    const auto tupleCount = current.size() / componentCount;
    const auto meta = MakeReferenceTestMeta<TValue>(tupleCount, componentCount);
    const auto* codec = ResolveNumericArrayReferenceCodec(codecId);
    if (codec == nullptr) {
        return validation::AssignError(error, "reference test codec is unavailable");
    }
    const auto encodeResult = codec->EncodeBlock(
            NumericArrayReferenceCodecEncodeInput{
                .meta = meta,
                .defaultCompressor = compressor,
                .scratchBytePool = scratchBytePool,
                .control = NumericArrayReferenceCodecControl{
                    .affineBlockRSquared = 0.0,
                },
                .currentBytes = NumericValueBytes(current),
                .referenceBytes = NumericValueBytes(reference),
                .elementOffset = 0u,
                .elementCount = static_cast<std::uint32_t>(tupleCount),
                .componentCount = componentCount,
                .referenceKind = referenceKind,
                .localParentFieldIndex = 0u,
                .predictorOffset = 0,
            },
            encoded,
            error);
    if (!encodeResult.IsEncoded()) {
        if (encodeResult.IsRejected() && error != nullptr && error->empty()) {
            *error = std::string("reference test block was rejected: ") +
                NumericArrayReferenceRejectReasonName(encodeResult.rejectReason);
        }
        return false;
    }
    ParsedNumericArrayBlock parsed;
    if (!BuildParsedNumericArrayReferenceBlock(encoded, parsed, error)) {
        return false;
    }
    return codec->DecodeBlock(
        NumericArrayReferenceCodecDecodeInput{
            .meta = meta,
            .block = parsed,
            .referenceBytes = NumericValueBytes(reference),
            .referenceElementOffset = 0u,
        },
        decodedBytes,
        error);
}

} // namespace datacodec::test

#endif
