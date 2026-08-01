#ifndef DATACODEC_CODEC_REFERENCE_COMMON_AFFINEREFERENCECODEC_H
#define DATACODEC_CODEC_REFERENCE_COMMON_AFFINEREFERENCECODEC_H

#include "DataCodec/Codec/Reference/Common/ReferenceBlockIO.h"
#include "DataCodec/Codec/Reference/Common/ReferenceCodecInterface.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cmath>
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
inline bool FitNumericArrayReferenceAffine(
    const TValue* currentValues,
    const TValue* referenceValues,
    const std::size_t tupleCount,
    const std::size_t componentCount,
    std::vector<double>& alpha,
    std::vector<double>& beta,
    double& averageR2) {
    alpha.assign(componentCount, 0.0);
    beta.assign(componentCount, 0.0);
    averageR2 = 0.0;
    if (currentValues == nullptr || referenceValues == nullptr ||
        tupleCount == 0u || componentCount == 0u) {
        return false;
    }

    constexpr long double kEpsilon = 1.0e-18L;
    std::size_t validComponentCount = 0u;
    for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
        long double sumX = 0.0L;
        long double sumY = 0.0L;
        long double sumXX = 0.0L;
        long double sumYY = 0.0L;
        long double sumXY = 0.0L;
        for (std::size_t tupleIndex = 0; tupleIndex < tupleCount; ++tupleIndex) {
            const auto valueIndex = tupleIndex * componentCount + componentIndex;
            const auto x = static_cast<long double>(referenceValues[valueIndex]);
            const auto y = static_cast<long double>(currentValues[valueIndex]);
            sumX += x;
            sumY += y;
            sumXX += x * x;
            sumYY += y * y;
            sumXY += x * y;
        }

        const auto localTupleCount = static_cast<long double>(tupleCount);
        const auto meanX = sumX / localTupleCount;
        const auto meanY = sumY / localTupleCount;
        const auto sxx = sumXX - sumX * sumX / localTupleCount;
        const auto syy = sumYY - sumY * sumY / localTupleCount;
        const auto sxy = sumXY - sumX * sumY / localTupleCount;

        if (syy <= kEpsilon) {
            alpha[componentIndex] = 0.0;
            beta[componentIndex] = static_cast<double>(meanY);
            averageR2 += 1.0;
            ++validComponentCount;
            continue;
        }
        if (sxx <= kEpsilon) {
            alpha[componentIndex] = 0.0;
            beta[componentIndex] = static_cast<double>(meanY);
            ++validComponentCount;
            continue;
        }

        alpha[componentIndex] = static_cast<double>(sxy / sxx);
        beta[componentIndex] = static_cast<double>(
            meanY - static_cast<long double>(alpha[componentIndex]) * meanX);
        averageR2 += static_cast<double>((sxy * sxy) / (sxx * syy));
        ++validComponentCount;
    }

    if (validComponentCount == 0u) {
        alpha.clear();
        beta.clear();
        return false;
    }

    averageR2 /= static_cast<double>(validComponentCount);
    return true;
}

template<typename TValue>
inline NumericArrayReferenceEncodeResult PrepareAffineReferenceBlockTyped(
    const NumericArrayReferenceCodecEncodeInput& input,
    const NumericArrayReferenceCodecId codecId,
    NumericArrayReferencePreparedBlock& prepared,
    std::string* error = nullptr) {
    prepared.Reset();
    if (!ValidateNumericArrayReferenceBlockBytes<TValue>(input, error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }

    const auto* currentValues = reinterpret_cast<const TValue*>(input.currentBytes.data());
    const auto* referenceValues = reinterpret_cast<const TValue*>(input.referenceBytes.data());
    double averageR2 = 0.0;
    if (!FitNumericArrayReferenceAffine(
            currentValues,
            referenceValues,
            input.elementCount,
            input.componentCount,
            prepared.affineAlpha,
            prepared.affineBeta,
            averageR2)) {
        validation::AssignError(error, "numeric array affine reference fitting failed");
        return NumericArrayReferenceEncodeResult::Failed();
    }
    if (averageR2 < input.control.affineBlockRSquared) {
        if (error != nullptr) {
            error->clear();
        }
        return NumericArrayReferenceEncodeResult::Rejected(
            NumericArrayReferenceRejectReason::InsufficientModelFit);
    }

    std::size_t valueCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.elementCount),
            input.componentCount,
            valueCount,
            "numeric array affine reference value count",
            error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    if (valueCount == 0u) {
        validation::AssignError(error, "numeric array affine reference delta block is invalid");
        return NumericArrayReferenceEncodeResult::Failed();
    }
    std::size_t deltaRawByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            valueCount,
            sizeof(TValue),
            deltaRawByteCount,
            "numeric array affine reference delta bytes",
            error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    prepared.deltaRawBytes = input.AcquireScratch(deltaRawByteCount);
    auto& deltaRaw = prepared.deltaRawBytes.Bytes();
    double maximumArithmeticError = 0.0;
    for (std::size_t tupleIndex = 0; tupleIndex < input.elementCount; ++tupleIndex) {
        for (std::size_t componentIndex = 0; componentIndex < input.componentCount; ++componentIndex) {
            const auto valueIndex = tupleIndex * input.componentCount + componentIndex;
            const auto prediction = static_cast<TValue>(
                prepared.affineAlpha[componentIndex] * static_cast<double>(referenceValues[valueIndex]) +
                prepared.affineBeta[componentIndex]);
            const auto delta = static_cast<TValue>(currentValues[valueIndex] - prediction);
            const auto reconstructed = static_cast<TValue>(delta + prediction);
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
    prepared.codecId = codecId;
    return NumericArrayReferenceEncodeResult::Encoded();
}

template<typename TValue>
inline NumericArrayReferenceEncodeResult EncodePreparedAffineReferenceBlockTyped(
    const NumericArrayReferenceCodecEncodeInput& input,
    const NumericArrayReferenceCodecId codecId,
    const NumericArrayReferencePreparedBlock& prepared,
    NumericArrayReferenceEncodedBlock& output,
    std::string* error = nullptr) {
    output = {};
    if (!ValidateNumericArrayReferenceBlockBytes<TValue>(input, error)) {
        return NumericArrayReferenceEncodeResult::Failed();
    }
    if (prepared.codecId != codecId ||
        prepared.affineAlpha.size() != input.componentCount ||
        prepared.affineBeta.size() != input.componentCount) {
        validation::AssignError(error, "prepared affine reference block is invalid");
        return NumericArrayReferenceEncodeResult::Failed();
    }

    std::size_t valueCount = 0u;
    std::size_t deltaRawByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.elementCount),
            input.componentCount,
            valueCount,
            "numeric array affine reference value count",
            error) ||
        valueCount == 0u ||
        !validation::CheckedMulSizeT(
            valueCount,
            sizeof(TValue),
            deltaRawByteCount,
            "numeric array affine reference delta bytes",
            error)) {
        if (valueCount == 0u && error != nullptr && error->empty()) {
            validation::AssignError(error, "numeric array affine reference delta block is invalid");
        }
        return NumericArrayReferenceEncodeResult::Failed();
    }
    if (prepared.deltaRawBytes.Bytes().size() != deltaRawByteCount) {
        validation::AssignError(error, "prepared affine reference delta bytes are invalid");
        return NumericArrayReferenceEncodeResult::Failed();
    }

    AffineReferenceBlockFields fields;
    fields.alpha = prepared.affineAlpha;
    fields.beta = prepared.affineBeta;
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
        .mode = NumericArrayBlockMode::AffineReference,
        .referenceKind = input.referenceKind,
        .codecId = codecId,
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
inline bool DecodeAffineReferenceBlockTyped(
    const NumericArrayReferenceCodecDecodeInput& input,
    std::vector<std::uint8_t>& decodedBlockBytes,
    std::string* error = nullptr) {
    const auto componentCount = static_cast<std::size_t>(std::max(input.meta.dimension, 0));
    if (componentCount == 0u ||
        input.block.alpha.size() != componentCount ||
        input.block.beta.size() != componentCount) {
        return validation::AssignError(error, "numeric array affine reference block coefficients are invalid");
    }
    std::size_t tupleBytes = 0u;
    std::size_t expectedBytes = 0u;
    std::size_t referenceByteOffset = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            sizeof(TValue),
            tupleBytes,
            "numeric array affine reference tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.block.header.elementCount),
            tupleBytes,
            expectedBytes,
            "numeric array affine reference block bytes",
            error) ||
        !validation::CheckedMulSizeT(
            input.referenceElementOffset,
            tupleBytes,
            referenceByteOffset,
            "numeric array affine reference byte offset",
            error)) {
        return false;
    }
    if (tupleBytes == 0u ||
        decodedBlockBytes.size() != expectedBytes ||
        referenceByteOffset > input.referenceBytes.size() ||
        expectedBytes > input.referenceBytes.size() - referenceByteOffset) {
        return validation::AssignError(error, "numeric array affine reference block byte range is invalid");
    }

    auto* outputValues = reinterpret_cast<TValue*>(decodedBlockBytes.data());
    const auto* referenceValues = reinterpret_cast<const TValue*>(input.referenceBytes.data() + referenceByteOffset);
    for (std::size_t tupleIndex = 0; tupleIndex < input.block.header.elementCount; ++tupleIndex) {
        for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
            const auto valueIndex = tupleIndex * componentCount + componentIndex;
            const auto prediction = static_cast<TValue>(
                input.block.alpha[componentIndex] * static_cast<double>(referenceValues[valueIndex]) +
                input.block.beta[componentIndex]);
            outputValues[valueIndex] = static_cast<TValue>(outputValues[valueIndex] + prediction);
        }
    }
    return true;
}

class AffineReferenceCodec final : public INumericArrayReferenceCodec {
public:
    [[nodiscard]] NumericArrayReferenceCodecId CodecId() const noexcept override {
        return NumericArrayReferenceCodecId::Affine;
    }

    [[nodiscard]] NumericArrayReferenceEncodeResult PrepareBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        NumericArrayReferencePreparedBlock& prepared,
        std::string* error = nullptr) const override {
        if (input.meta.dataType == DataType::Float32 && NumericArrayValueSize(input.meta) == sizeof(float)) {
            return PrepareAffineReferenceBlockTyped<float>(input, CodecId(), prepared, error);
        }
        if (input.meta.dataType == DataType::Float64 && NumericArrayValueSize(input.meta) == sizeof(double)) {
            return PrepareAffineReferenceBlockTyped<double>(input, CodecId(), prepared, error);
        }
        prepared.Reset();
        validation::AssignError(error, "affine reference codec requires float32 or float64 data");
        return NumericArrayReferenceEncodeResult::Failed();
    }

    [[nodiscard]] NumericArrayReferenceEncodeResult EncodePreparedBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        const NumericArrayReferencePreparedBlock& prepared,
        NumericArrayReferenceEncodedBlock& output,
        std::string* error = nullptr) const override {
        if (input.meta.dataType == DataType::Float32 && NumericArrayValueSize(input.meta) == sizeof(float)) {
            return EncodePreparedAffineReferenceBlockTyped<float>(
                input, CodecId(), prepared, output, error);
        }
        if (input.meta.dataType == DataType::Float64 && NumericArrayValueSize(input.meta) == sizeof(double)) {
            return EncodePreparedAffineReferenceBlockTyped<double>(
                input, CodecId(), prepared, output, error);
        }
        output = {};
        validation::AssignError(error, "affine reference codec requires float32 or float64 data");
        return NumericArrayReferenceEncodeResult::Failed();
    }

    [[nodiscard]] bool DecodeBlock(
        const NumericArrayReferenceCodecDecodeInput& input,
        std::vector<std::uint8_t>& decodedBlockBytes,
        std::string* error = nullptr) const override {
        if (!DecodeNumericArrayReferenceValueBytes(input.meta, input.block, decodedBlockBytes, error)) {
            return false;
        }
        if (input.meta.dataType == DataType::Float32 && NumericArrayValueSize(input.meta) == sizeof(float)) {
            return DecodeAffineReferenceBlockTyped<float>(input, decodedBlockBytes, error);
        }
        if (input.meta.dataType == DataType::Float64 && NumericArrayValueSize(input.meta) == sizeof(double)) {
            return DecodeAffineReferenceBlockTyped<double>(input, decodedBlockBytes, error);
        }
        decodedBlockBytes.clear();
        return validation::AssignError(error, "affine reference codec requires float32 or float64 data");
    }
};

} // namespace datacodec

#endif
