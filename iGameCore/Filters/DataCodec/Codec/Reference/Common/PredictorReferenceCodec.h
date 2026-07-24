#ifndef DATACODEC_CODEC_REFERENCE_COMMON_PREDICTORREFERENCECODEC_H
#define DATACODEC_CODEC_REFERENCE_COMMON_PREDICTORREFERENCECODEC_H

#include "DataCodec/Codec/Reference/Common/ReferenceBlockIO.h"
#include "DataCodec/Codec/Reference/Common/ReferenceCodecInterface.h"
#include "DataCodec/Codec/Reference/Common/ReferenceDelta.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

template<typename TValue>
inline NumericArrayReferenceEncodeResult PreparePredictorReferenceBlockTyped(
    const NumericArrayReferenceCodecEncodeInput& input,
    NumericArrayReferencePreparedBlock& prepared,
    std::string* error = nullptr) {
    prepared.Reset();
    if (!ValidateNumericArrayReferenceBlockBytes<TValue>(input, error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    std::size_t valueCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.elementCount),
            input.componentCount,
            valueCount,
            "numeric array predictor reference value count",
            error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    if (valueCount == 0u) {
        validation::AssignError(error, "numeric array predictor delta block is invalid");
        return NumericArrayReferenceEncodeResult::Failed();
    }
    std::size_t deltaRawByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            valueCount,
            sizeof(TValue),
            deltaRawByteCount,
            "numeric array predictor reference delta bytes",
            error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    const auto* currentValues = reinterpret_cast<const TValue*>(input.currentBytes.data());
    const auto* referenceValues = reinterpret_cast<const TValue*>(input.referenceBytes.data());
    prepared.deltaRawBytes = input.AcquireScratch(deltaRawByteCount);
    auto& deltaRaw = prepared.deltaRawBytes.Bytes();
    double maximumArithmeticError = 0.0;
    for (std::size_t valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
        const auto delta = static_cast<TValue>(
            currentValues[valueIndex] - referenceValues[valueIndex]);
        const auto reconstructed = static_cast<TValue>(delta + referenceValues[valueIndex]);
        if (reconstructed != currentValues[valueIndex]) {
            const auto expectedValue = static_cast<double>(currentValues[valueIndex]);
            const auto reconstructedValue = static_cast<double>(reconstructed);
            if (!std::isfinite(expectedValue) || !std::isfinite(reconstructedValue)) {
                maximumArithmeticError = std::numeric_limits<double>::infinity();
            } else {
                maximumArithmeticError = std::max(
                    maximumArithmeticError,
                    std::abs(expectedValue - reconstructedValue));
            }
        }
        std::memcpy(deltaRaw.data() + valueIndex * sizeof(TValue), &delta, sizeof(TValue));
    }

    if (!ResolveReferenceResidualCompressor(
            input.meta,
            input.currentBytes,
            input.defaultCompressor,
            1.0,
            prepared.residualCompressor,
            error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    const auto precisionResult = ConsumeReferenceArithmeticErrorBudget(
            prepared.residualCompressor,
            1.0,
            maximumArithmeticError,
            error);
    if (!precisionResult.IsEncoded()) {
        prepared.Reset();
        return precisionResult;
    }
    prepared.codecId = NumericArrayReferenceCodecId::Predictor;
    return NumericArrayReferenceEncodeResult::Encoded();
}

template<typename TValue>
inline NumericArrayReferenceEncodeResult EncodePreparedPredictorReferenceBlockTyped(
    const NumericArrayReferenceCodecEncodeInput& input,
    const NumericArrayReferencePreparedBlock& prepared,
    NumericArrayReferenceEncodedBlock& output,
    std::string* error = nullptr) {
    output = {};
    if (!ValidateNumericArrayReferenceBlockBytes<TValue>(input, error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    if (prepared.codecId != NumericArrayReferenceCodecId::Predictor) {
        validation::AssignError(error, "prepared predictor reference block is invalid");
        return NumericArrayReferenceEncodeResult::Failed();
    }
    std::size_t valueCount = 0u;
    std::size_t deltaRawByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.elementCount),
            input.componentCount,
            valueCount,
            "numeric array predictor reference value count",
            error) ||
        valueCount == 0u ||
        !validation::CheckedMulSizeT(
            valueCount,
            sizeof(TValue),
            deltaRawByteCount,
            "numeric array predictor reference delta bytes",
            error)) {
        if (valueCount == 0u && error != nullptr && error->empty()) {
            validation::AssignError(error, "numeric array predictor delta block is invalid");
        }
        return NumericArrayReferenceEncodeResult::Failed();
    }
    if (prepared.deltaRawBytes.Bytes().size() != deltaRawByteCount) {
        validation::AssignError(error, "prepared predictor reference delta bytes are invalid");
        return NumericArrayReferenceEncodeResult::Failed();
    }

    PredictorReferenceBlockFields fields;
    fields.predictorOffset = input.predictorOffset;
    fields.deltaBytes = input.AcquireScratch(input.currentBytes.size());
    auto& deltaBytes = fields.deltaBytes.Bytes();
    NumericArrayBytesCodec bytesCodec{NumericArrayBytesCodec::RawBytes};
    if (!EncodeNumericArrayReferenceValueBytes(
            input.meta,
            prepared.residualCompressor,
            input.elementCount,
            prepared.deltaRawBytes.Span(),
            input.scratchBytePool,
            deltaBytes,
            bytesCodec,
            &fields.componentLayouts,
            error)) {
        output = {};
        return NumericArrayReferenceEncodeResult::Failed();
    }

    output.header = NumericArrayBlockHeader{
        .mode = NumericArrayBlockMode::PredictorReference,
        .referenceKind = input.referenceKind,
        .codecId = NumericArrayReferenceCodecId::Predictor,
        .localParentFieldIndex = input.localParentFieldIndex,
        .elementOffset = input.elementOffset,
        .elementCount = input.elementCount,
        .encodedByteLength = static_cast<std::uint32_t>(deltaBytes.size()),
        .bytesCodec = bytesCodec,
        .predictorOffset = input.predictorOffset,
    };
    output.backgroundCompressor = prepared.residualCompressor;
    output.fields = std::move(fields);
    return NumericArrayReferenceEncodeResult::Encoded();
}

template<typename TValue>
inline bool DecodePredictorReferenceBlockTyped(
    const NumericArrayReferenceCodecDecodeInput& input,
    std::vector<std::uint8_t>& decodedBlockBytes,
    std::string* error = nullptr) {
    const auto componentCount = static_cast<std::size_t>(std::max(input.meta.dimension, 0));
    if (componentCount == 0u) {
        return validation::AssignError(error, "numeric array predictor reference component count is invalid");
    }

    std::size_t tupleBytes = 0u;
    std::size_t expectedBytes = 0u;
    std::size_t referenceByteOffset = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            sizeof(TValue),
            tupleBytes,
            "numeric array predictor reference tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.block.header.elementCount),
            tupleBytes,
            expectedBytes,
            "numeric array predictor reference block bytes",
            error) ||
        !validation::CheckedMulSizeT(
            input.referenceElementOffset,
            tupleBytes,
            referenceByteOffset,
            "numeric array predictor reference byte offset",
            error)) {
        return false;
    }
    if (tupleBytes == 0u ||
        decodedBlockBytes.size() != expectedBytes ||
        referenceByteOffset > input.referenceBytes.size() ||
        expectedBytes > input.referenceBytes.size() - referenceByteOffset) {
        return validation::AssignError(error, "numeric array predictor reference block byte range is invalid");
    }

    auto* outputValues = reinterpret_cast<TValue*>(decodedBlockBytes.data());
    const auto* referenceValues = reinterpret_cast<const TValue*>(input.referenceBytes.data() + referenceByteOffset);
    std::size_t valueCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.block.header.elementCount),
            componentCount,
            valueCount,
            "numeric array predictor reference value count",
            error)) {
        return false;
    }
    for (std::size_t valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
        outputValues[valueIndex] = static_cast<TValue>(
            outputValues[valueIndex] + referenceValues[valueIndex]);
    }
    return true;
}

class PredictorReferenceCodec final : public INumericArrayReferenceCodec {
public:
    [[nodiscard]] NumericArrayReferenceCodecId CodecId() const noexcept override {
        return NumericArrayReferenceCodecId::Predictor;
    }

    [[nodiscard]] NumericArrayReferenceEncodeResult PrepareBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        NumericArrayReferencePreparedBlock& prepared,
        std::string* error = nullptr) const override {
        if (input.meta.dataType == DataType::Float32 && input.meta.valueSize == sizeof(float)) {
            return PreparePredictorReferenceBlockTyped<float>(input, prepared, error);
        }
        if (input.meta.dataType == DataType::Float64 && input.meta.valueSize == sizeof(double)) {
            return PreparePredictorReferenceBlockTyped<double>(input, prepared, error);
        }
        prepared.Reset();
        validation::AssignError(error, "predictor reference codec requires float32 or float64 data");
        return NumericArrayReferenceEncodeResult::Failed();
    }

    [[nodiscard]] NumericArrayReferenceEncodeResult EncodePreparedBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        const NumericArrayReferencePreparedBlock& prepared,
        NumericArrayReferenceEncodedBlock& output,
        std::string* error = nullptr) const override {
        if (input.meta.dataType == DataType::Float32 && input.meta.valueSize == sizeof(float)) {
            return EncodePreparedPredictorReferenceBlockTyped<float>(input, prepared, output, error);
        }
        if (input.meta.dataType == DataType::Float64 && input.meta.valueSize == sizeof(double)) {
            return EncodePreparedPredictorReferenceBlockTyped<double>(input, prepared, output, error);
        }
        output = {};
        validation::AssignError(error, "predictor reference codec requires float32 or float64 data");
        return NumericArrayReferenceEncodeResult::Failed();
    }

    [[nodiscard]] bool DecodeBlock(
        const NumericArrayReferenceCodecDecodeInput& input,
        std::vector<std::uint8_t>& decodedBlockBytes,
        std::string* error = nullptr) const override {
        if (!DecodeNumericArrayReferenceValueBytes(input.meta, input.block, decodedBlockBytes, error)) {
            return false;
        }
        if (input.meta.dataType == DataType::Float32 && input.meta.valueSize == sizeof(float)) {
            return DecodePredictorReferenceBlockTyped<float>(input, decodedBlockBytes, error);
        }
        if (input.meta.dataType == DataType::Float64 && input.meta.valueSize == sizeof(double)) {
            return DecodePredictorReferenceBlockTyped<double>(input, decodedBlockBytes, error);
        }
        decodedBlockBytes.clear();
        return validation::AssignError(error, "predictor reference codec requires float32 or float64 data");
    }
};

} // namespace datacodec

#endif
