#ifndef DATACODEC_CODEC_REFERENCE_COMMON_REFERENCEBLOCKIO_H
#define DATACODEC_CODEC_REFERENCE_COMMON_REFERENCEBLOCKIO_H

#include "DataCodec/Codec/NumericArray/NumericArrayBlockDecode.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockEncode.h"
#include "DataCodec/Codec/Reference/Common/ReferenceCodecInterface.h"
#include "DataCodec/Codec/Reference/Common/ReferenceTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
namespace datacodec {

template<typename TValue>
inline bool ComputeReferenceMinimumComponentValueRange(
    const std::span<const std::uint8_t> currentBytes,
    const std::size_t componentCount,
    double& minimumValueRange,
    std::string* error = nullptr) {
    minimumValueRange = 0.0;
    std::size_t tupleBytes = 0u;
    if (componentCount == 0u ||
        !validation::CheckedMulSizeT(
            componentCount,
            sizeof(TValue),
            tupleBytes,
            "reference current tuple bytes",
            error) ||
        tupleBytes == 0u ||
        currentBytes.empty() ||
        currentBytes.size() % tupleBytes != 0u) {
        return validation::AssignError(
            error,
            "reference current bytes do not align with the numeric tuple size");
    }
    const auto* values = reinterpret_cast<const TValue*>(currentBytes.data());
    const auto tupleCount = currentBytes.size() / tupleBytes;
    minimumValueRange = std::numeric_limits<double>::infinity();
    for (std::size_t componentIndex = 0u;
         componentIndex < componentCount;
         ++componentIndex) {
        double componentMinimum = std::numeric_limits<double>::infinity();
        double componentMaximum = -std::numeric_limits<double>::infinity();
        for (std::size_t tupleIndex = 0u; tupleIndex < tupleCount; ++tupleIndex) {
            const auto value = static_cast<double>(
                values[tupleIndex * componentCount + componentIndex]);
            if (!std::isfinite(value)) {
                return validation::AssignError(
                    error,
                    "reference current values contain a non-finite value");
            }
            componentMinimum = std::min(componentMinimum, value);
            componentMaximum = std::max(componentMaximum, value);
        }
        minimumValueRange = std::min(
            minimumValueRange,
            componentMaximum - componentMinimum);
    }
    if (!std::isfinite(minimumValueRange) || minimumValueRange < 0.0) {
        return validation::AssignError(error, "reference component value range is invalid");
    }
    return true;
}

inline bool ResolveReferenceResidualCompressor(
    const NumericArrayStorageParams& meta,
    const std::span<const std::uint8_t> currentBytes,
    const CompressorConfig& requestedCompressor,
    const double reconstructionErrorAmplification,
    CompressorConfig& residualCompressor,
    std::string* error = nullptr) {
    residualCompressor = requestedCompressor;
    if (numericarray::IsIntegerNumericArrayDataType(meta.dataType)) {
        return true;
    }
    if (!std::isfinite(reconstructionErrorAmplification) ||
        reconstructionErrorAmplification <= 0.0) {
        return validation::AssignError(error, "reference reconstruction error amplification is invalid");
    }

    const auto absoluteIterator = requestedCompressor.options.find("pressio:abs");
    const auto relativeIterator = requestedCompressor.options.find("pressio:rel");
    if (absoluteIterator == requestedCompressor.options.end() &&
        relativeIterator == requestedCompressor.options.end()) {
        return true;
    }

    double requestedAbsoluteError = std::numeric_limits<double>::infinity();
    if (absoluteIterator != requestedCompressor.options.end()) {
        if (!std::isfinite(absoluteIterator->second) || absoluteIterator->second < 0.0) {
            return validation::AssignError(error, "reference absolute error bound is invalid");
        }
        requestedAbsoluteError = absoluteIterator->second;
    }
    if (relativeIterator != requestedCompressor.options.end()) {
        if (!std::isfinite(relativeIterator->second) || relativeIterator->second < 0.0) {
            return validation::AssignError(error, "reference relative error bound is invalid");
        }
        const auto componentCount = static_cast<std::size_t>(std::max(meta.dimension, 0));
        double valueRange = 0.0;
        if (meta.dataType == DataType::Float32 && NumericArrayValueSize(meta) == sizeof(float)) {
            if (!ComputeReferenceMinimumComponentValueRange<float>(
                    currentBytes,
                    componentCount,
                    valueRange,
                    error)) {
                return false;
            }
        } else if (meta.dataType == DataType::Float64 && NumericArrayValueSize(meta) == sizeof(double)) {
            if (!ComputeReferenceMinimumComponentValueRange<double>(
                    currentBytes,
                    componentCount,
                    valueRange,
                    error)) {
                return false;
            }
        } else {
            return validation::AssignError(
                error,
                "reference relative error conversion requires float32 or float64 data");
        }
        requestedAbsoluteError = std::min(
            requestedAbsoluteError,
            relativeIterator->second * valueRange);
    }
    if (!std::isfinite(requestedAbsoluteError) || requestedAbsoluteError < 0.0) {
        return validation::AssignError(error, "reference resolved absolute error bound is invalid");
    }

    residualCompressor.options.erase("pressio:rel");
    residualCompressor.options["pressio:abs"] =
        requestedAbsoluteError / reconstructionErrorAmplification;
    return true;
}

inline NumericArrayReferenceEncodeResult ConsumeReferenceArithmeticErrorBudget(
    CompressorConfig& residualCompressor,
    const double reconstructionErrorAmplification,
    const double maximumArithmeticError,
    std::string* error = nullptr) {
    if (!std::isfinite(reconstructionErrorAmplification) ||
        reconstructionErrorAmplification <= 0.0 ||
        !std::isfinite(maximumArithmeticError) ||
        maximumArithmeticError < 0.0) {
        validation::AssignError(error, "reference arithmetic error budget is invalid");
        return NumericArrayReferenceEncodeResult::Failed();
    }
    const auto absoluteIterator = residualCompressor.options.find("pressio:abs");
    if (absoluteIterator == residualCompressor.options.end()) {
        return NumericArrayReferenceEncodeResult::Encoded();
    }
    const auto arithmeticResidualError = maximumArithmeticError / reconstructionErrorAmplification;
    if (arithmeticResidualError > absoluteIterator->second) {
        if (error != nullptr) {
            error->clear();
        }
        return NumericArrayReferenceEncodeResult::Rejected(
            NumericArrayReferenceRejectReason::PrecisionBudgetUnavailable);
    }
    absoluteIterator->second = std::max(0.0, absoluteIterator->second - arithmeticResidualError);
    return NumericArrayReferenceEncodeResult::Encoded();
}

inline NumericArrayBlockLayoutParams MakeNumericArrayReferenceBlockLayout(
    const NumericArrayReferenceEncodedBlock& block) {
    if (block.header.codecId == NumericArrayReferenceCodecId::Affine) {
        const auto* fields = std::get_if<AffineReferenceBlockFields>(&block.fields);
        if (fields != nullptr) {
            auto layout = MakeNumericArrayBlockLayoutParams(
                block.header,
                std::span<const double>(fields->alpha.data(), fields->alpha.size()),
                std::span<const double>(fields->beta.data(), fields->beta.size()));
            layout.backgroundCompressor = block.backgroundCompressor;
            layout.componentLayouts = fields->componentLayouts;
            return layout;
        }
    }
    if (block.header.codecId == NumericArrayReferenceCodecId::Wavelet) {
        const auto* fields = std::get_if<WaveletReferenceBlockFields>(&block.fields);
        if (fields != nullptr) {
            auto layout = MakeNumericArrayBlockLayoutParams(block.header, {}, {});
            layout.backgroundCompressor = block.backgroundCompressor;
            layout.componentLayouts = fields->componentLayouts;
            return layout;
        }
    }
    if (block.header.codecId == NumericArrayReferenceCodecId::Predictor) {
        const auto* fields = std::get_if<PredictorReferenceBlockFields>(&block.fields);
        if (fields != nullptr) {
            auto header = block.header;
            header.predictorOffset = fields->predictorOffset;
            auto layout = MakeNumericArrayBlockLayoutParams(header, {}, {});
            layout.backgroundCompressor = block.backgroundCompressor;
            layout.componentLayouts = fields->componentLayouts;
            return layout;
        }
    }
    auto layout = MakeNumericArrayBlockLayoutParams(block.header, {}, {});
    layout.backgroundCompressor = block.backgroundCompressor;
    return layout;
}

inline bool BuildParsedNumericArrayReferenceBlock(
    const NumericArrayReferenceEncodedBlock& encoded,
    ParsedNumericArrayBlock& parsed,
    std::string* error = nullptr) {
    parsed = {};
    parsed.header = encoded.header;
    parsed.backgroundCompressor = encoded.backgroundCompressor;
    if (const auto* fields = std::get_if<PredictorReferenceBlockFields>(&encoded.fields)) {
        parsed.header.predictorOffset = fields->predictorOffset;
        parsed.componentLayouts = fields->componentLayouts;
        parsed.bytes = fields->deltaBytes.Span();
        return true;
    }
    if (const auto* fields = std::get_if<AffineReferenceBlockFields>(&encoded.fields)) {
        parsed.componentLayouts = fields->componentLayouts;
        parsed.alpha = fields->alpha;
        parsed.beta = fields->beta;
        parsed.bytes = fields->deltaBytes.Span();
        return true;
    }
    if (const auto* fields = std::get_if<WaveletReferenceBlockFields>(&encoded.fields)) {
        parsed.componentLayouts = fields->componentLayouts;
        parsed.bytes = fields->waveletBytes.Span();
        return true;
    }
    return validation::AssignError(error, "reference encoded block fields are invalid");
}

template<typename TValue>
inline bool ValidateReferenceReconstructedPrecisionTyped(
    const NumericArrayReferenceCodecEncodeInput& input,
    const std::span<const std::uint8_t> decodedBytes,
    std::string* error = nullptr) {
    if (decodedBytes.size() != input.currentBytes.size()) {
        return validation::AssignError(error, "reference reconstructed byte size is invalid");
    }
    const auto absoluteIterator = input.defaultCompressor.options.find("pressio:abs");
    const auto relativeIterator = input.defaultCompressor.options.find("pressio:rel");
    const bool hasAbsolute = absoluteIterator != input.defaultCompressor.options.end();
    const bool hasRelative = relativeIterator != input.defaultCompressor.options.end();
    if (hasAbsolute &&
        (!std::isfinite(absoluteIterator->second) || absoluteIterator->second < 0.0)) {
        return validation::AssignError(error, "reference requested absolute error bound is invalid");
    }
    if (hasRelative &&
        (!std::isfinite(relativeIterator->second) || relativeIterator->second < 0.0)) {
        return validation::AssignError(error, "reference requested relative error bound is invalid");
    }

    std::vector<double> componentRanges(input.componentCount, 0.0);
    const auto* expectedValues = reinterpret_cast<const TValue*>(input.currentBytes.data());
    const auto* decodedValues = reinterpret_cast<const TValue*>(decodedBytes.data());
    if (hasRelative) {
        std::vector<double> componentMinimum(
            input.componentCount,
            std::numeric_limits<double>::infinity());
        std::vector<double> componentMaximum(
            input.componentCount,
            -std::numeric_limits<double>::infinity());
        for (std::size_t tupleIndex = 0u; tupleIndex < input.elementCount; ++tupleIndex) {
            for (std::size_t componentIndex = 0u;
                 componentIndex < input.componentCount;
                 ++componentIndex) {
                const auto valueIndex = tupleIndex * input.componentCount + componentIndex;
                const auto expected = static_cast<double>(expectedValues[valueIndex]);
                if (!std::isfinite(expected)) {
                    return validation::AssignError(
                        error,
                        "reference expected values contain a non-finite value");
                }
                componentMinimum[componentIndex] = std::min(
                    componentMinimum[componentIndex],
                    expected);
                componentMaximum[componentIndex] = std::max(
                    componentMaximum[componentIndex],
                    expected);
            }
        }
        for (std::size_t componentIndex = 0u;
             componentIndex < input.componentCount;
             ++componentIndex) {
            componentRanges[componentIndex] =
                componentMaximum[componentIndex] - componentMinimum[componentIndex];
        }
    }

    for (std::size_t tupleIndex = 0u; tupleIndex < input.elementCount; ++tupleIndex) {
        for (std::size_t componentIndex = 0u;
             componentIndex < input.componentCount;
             ++componentIndex) {
            const auto valueIndex = tupleIndex * input.componentCount + componentIndex;
            const auto expected = static_cast<double>(expectedValues[valueIndex]);
            const auto reconstructed = static_cast<double>(decodedValues[valueIndex]);
            if (!std::isfinite(expected) || !std::isfinite(reconstructed)) {
                return validation::AssignError(
                    error,
                    "reference reconstructed values contain a non-finite value");
            }
            if (!hasAbsolute && !hasRelative) {
                continue;
            }
            double errorBound = std::numeric_limits<double>::infinity();
            if (hasAbsolute) {
                errorBound = absoluteIterator->second;
            }
            if (hasRelative) {
                errorBound = std::min(
                    errorBound,
                    relativeIterator->second * componentRanges[componentIndex]);
            }
            const auto allowedError = std::nextafter(
                errorBound,
                std::numeric_limits<double>::infinity());
            if (std::abs(reconstructed - expected) > allowedError) {
                return validation::AssignError(
                    error,
                    "reference reconstructed value exceeds the requested precision");
            }
        }
    }
    return true;
}

inline bool ValidateReferenceReconstructedPrecision(
    const NumericArrayReferenceCodecEncodeInput& input,
    const std::span<const std::uint8_t> decodedBytes,
    std::string* error = nullptr) {
    if (numericarray::IsIntegerNumericArrayDataType(input.meta.dataType)) {
        if (decodedBytes.size() != input.currentBytes.size() ||
            std::memcmp(
                decodedBytes.data(),
                input.currentBytes.data(),
                input.currentBytes.size()) != 0) {
            return validation::AssignError(
                error,
                "integer reference reconstruction is not lossless");
        }
        return true;
    }
    if (input.meta.dataType == DataType::Float32 && NumericArrayValueSize(input.meta) == sizeof(float)) {
        return ValidateReferenceReconstructedPrecisionTyped<float>(input, decodedBytes, error);
    }
    if (input.meta.dataType == DataType::Float64 && NumericArrayValueSize(input.meta) == sizeof(double)) {
        return ValidateReferenceReconstructedPrecisionTyped<double>(input, decodedBytes, error);
    }
    return validation::AssignError(
        error,
        "reference precision validation requires a supported numeric data type");
}

inline bool VerifyNumericArrayReferenceEncodedBlock(
    const INumericArrayReferenceCodec& codec,
    const NumericArrayReferenceCodecEncodeInput& input,
    const NumericArrayReferenceEncodedBlock& encoded,
    std::string* error = nullptr) {
    ParsedNumericArrayBlock parsed;
    if (!BuildParsedNumericArrayReferenceBlock(encoded, parsed, error)) {
        return false;
    }
    std::vector<std::uint8_t> decodedBytes;
    if (!codec.DecodeBlock(
            NumericArrayReferenceCodecDecodeInput{
                .meta = input.meta,
                .block = parsed,
                .referenceBytes = input.referenceBytes,
                .referenceElementOffset = 0u,
            },
            decodedBytes,
            error)) {
        return false;
    }
    return ValidateReferenceReconstructedPrecision(input, decodedBytes, error);
}

inline bool WriteNumericArrayReferenceEncodedBlock(
    bytestore::IByteWriter& writer,
    const NumericArrayReferenceEncodedBlock& block,
    std::string* error = nullptr) {
    if (block.header.codecId == NumericArrayReferenceCodecId::Affine) {
        const auto* fields = std::get_if<AffineReferenceBlockFields>(&block.fields);
        if (fields == nullptr) {
            return validation::AssignError(error, "affine reference block fields are missing");
        }
        return WriteNumericArrayBlock(
            writer,
            block.header,
            std::span<const double>(fields->alpha.data(), fields->alpha.size()),
            std::span<const double>(fields->beta.data(), fields->beta.size()),
            std::span<const std::uint8_t>(
                fields->deltaBytes.Bytes().data(),
                fields->deltaBytes.Bytes().size()),
            error);
    }

    if (block.header.codecId == NumericArrayReferenceCodecId::Wavelet) {
        const auto* fields = std::get_if<WaveletReferenceBlockFields>(&block.fields);
        if (fields == nullptr) {
            return validation::AssignError(error, "wavelet reference block fields are missing");
        }
        return WriteNumericArrayBlock(
            writer,
            block.header,
            {},
            {},
            std::span<const std::uint8_t>(
                fields->waveletBytes.Bytes().data(),
                fields->waveletBytes.Bytes().size()),
            error);
    }

    if (block.header.codecId == NumericArrayReferenceCodecId::Predictor) {
        const auto* fields = std::get_if<PredictorReferenceBlockFields>(&block.fields);
        if (fields == nullptr) {
            return validation::AssignError(error, "predictor reference block fields are missing");
        }
        auto header = block.header;
        header.predictorOffset = fields->predictorOffset;
        return WriteNumericArrayBlock(
            writer,
            header,
            {},
            {},
            std::span<const std::uint8_t>(
                fields->deltaBytes.Bytes().data(),
                fields->deltaBytes.Bytes().size()),
            error);
    }

    return validation::AssignError(error, "unsupported reference encoded block codec");
}

inline bool EncodeNumericArrayReferenceValueBytes(
    const NumericArrayStorageParams& meta,
    const CompressorConfig& defaultCompressor,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> rawBytes,
    ScratchByteBufferPool& scratchBytePool,
    std::vector<std::uint8_t>& encodedBytes,
    NumericArrayBytesCodec& bytesCodec,
    std::vector<NumericArrayComponentLayoutParams>* componentLayouts = nullptr,
    std::string* error = nullptr) {
    numericarray::NumericArrayBlockParams params;
    if (!numericarray::MakeNumericArrayBlockParamsFromMeta(meta, params, error)) {
        return false;
    }
    return numericarray::ResolveEncodedNumericArrayBlockBytes(
        params,
        defaultCompressor,
        elementCount,
        rawBytes,
        encodedBytes,
        bytesCodec,
        error,
        &scratchBytePool,
        componentLayouts);
}

inline bool DecodeNumericArrayReferenceValueBytes(
    const NumericArrayStorageParams& meta,
    const ParsedNumericArrayBlock& block,
    std::vector<std::uint8_t>& decodedBytes,
    std::string* error = nullptr) {
    numericarray::NumericArrayBlockParams params;
    if (!numericarray::MakeNumericArrayBlockParamsFromMeta(meta, params, error)) {
        return false;
    }
    return numericarray::ResolveDecodedNumericArrayBlockBytes(
        params,
        block.backgroundCompressor,
        block.header.elementCount,
        block.header.bytesCodec,
        block.componentLayouts,
        block.bytes,
        decodedBytes,
        error);
}

template<typename TValue>
inline bool ValidateNumericArrayReferenceBlockBytes(
    const NumericArrayReferenceCodecEncodeInput& input,
    std::string* error = nullptr) {
    if (input.componentCount == 0u ||
        input.meta.dimension <= 0 ||
        static_cast<std::size_t>(input.meta.dimension) != input.componentCount ||
        NumericArrayValueSize(input.meta) != sizeof(TValue)) {
        return validation::AssignError(error, "numeric array reference metadata shape is invalid");
    }
    std::size_t tupleBytes = 0u;
    std::size_t expectedBytes = 0u;
    if (!validation::CheckedMulSizeT(
            input.componentCount,
            sizeof(TValue),
            tupleBytes,
            "numeric array reference tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.elementCount),
            tupleBytes,
            expectedBytes,
            "numeric array reference block bytes",
            error)) {
        return false;
    }
    if (tupleBytes == 0u ||
        input.currentBytes.size() != expectedBytes ||
        input.referenceBytes.size() != expectedBytes) {
        return validation::AssignError(error, "numeric array reference block byte range is invalid");
    }
    if constexpr (std::is_floating_point_v<TValue>) {
        const auto* currentValues = reinterpret_cast<const TValue*>(input.currentBytes.data());
        const auto* referenceValues = reinterpret_cast<const TValue*>(input.referenceBytes.data());
        const auto valueCount = expectedBytes / sizeof(TValue);
        for (std::size_t valueIndex = 0u; valueIndex < valueCount; ++valueIndex) {
            if (!std::isfinite(static_cast<double>(currentValues[valueIndex])) ||
                !std::isfinite(static_cast<double>(referenceValues[valueIndex]))) {
                return validation::AssignError(
                    error,
                    "numeric array reference values contain a non-finite value");
            }
        }
    }
    return true;
}

} // namespace datacodec

#endif
