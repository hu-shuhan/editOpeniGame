#ifndef DATACODEC_CODEC_REFERENCE_COMMON_WAVELETREFERENCECODEC_H
#define DATACODEC_CODEC_REFERENCE_COMMON_WAVELETREFERENCECODEC_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Codec/NumericArray/IntegerResidualCodec.h"
#include "DataCodec/Codec/NumericArray/NumericArrayCodec.h"
#include "DataCodec/Codec/Reference/Common/ReferenceBlockIO.h"
#include "DataCodec/Codec/Reference/Common/ReferenceCodecInterface.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

inline numericarray::NumericArrayBufferLayout MakeWaveletNumericArrayLayout(
    const std::size_t valueSize,
    const std::size_t elementCount,
    const int dimension) {
    numericarray::NumericArrayBufferLayout layout;
    layout.dataType = valueSize == sizeof(double) ? DataType::Float64 : DataType::Float32;
    layout.valueSize = valueSize;
    layout.elementCount = elementCount;
    layout.componentCount = static_cast<std::size_t>(std::max(dimension, 0));
    return layout;
}

struct HaarLevel1Decomposition {
    std::vector<double> low;
    std::vector<double> high;
};

struct WaveletScratchDoubles {
    ScratchByteBuffer storage;
    std::span<double> values;

    void Release() noexcept {
        storage.Release();
        values = {};
    }
};

enum class WaveletScalarBlobCodec : std::uint8_t {
    RawDouble = 0,
    NumericArrayCodec = 1,
};

struct WaveletScalarEncodedBytes {
    bool hasCodecHeader{false};
    WaveletScalarBlobCodec codec{WaveletScalarBlobCodec::RawDouble};
    std::span<const std::uint8_t> payloadBytes;
    numericarray::NumericArrayEncodedBytes numericBytes;

    void Reset() {
        hasCodecHeader = false;
        codec = WaveletScalarBlobCodec::RawDouble;
        payloadBytes = {};
        numericBytes.Reset();
    }

    [[nodiscard]] std::size_t EncodedSize() const noexcept {
        return hasCodecHeader ? sizeof(std::uint8_t) + payloadBytes.size() : 0u;
    }
};

inline bool AcquireWaveletScratchDoubles(
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    const std::size_t valueCount,
    WaveletScratchDoubles& buffer,
    std::string* error = nullptr) {
    buffer.Release();
    if (valueCount == 0u) {
        return true;
    }
    std::size_t byteCount = 0u;
    std::size_t requestedByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            valueCount,
            sizeof(double),
            byteCount,
            "wavelet scratch double buffer",
            error) ||
        !validation::CheckedAddSizeT(
            byteCount,
            alignof(double) - 1u,
            requestedByteCount,
            "wavelet scratch double buffer",
            error)) {
        return false;
    }
    const auto requestedBytes = static_cast<std::uint64_t>(requestedByteCount);
    buffer.storage = scratchBytePool.Acquire(
        static_cast<std::size_t>(requestedBytes),
        acquireScratchQuota ? acquireScratchQuota(requestedBytes) : ScratchByteQuotaLease{});
    void* rawData = buffer.storage.Bytes().data();
    auto rawSpace = buffer.storage.Bytes().size();
    void* alignedData = std::align(alignof(double), byteCount, rawData, rawSpace);
    if (alignedData == nullptr) {
        buffer.Release();
        return validation::AssignError(error, "wavelet scratch double buffer alignment failed");
    }
    buffer.values = std::span<double>(static_cast<double*>(alignedData), valueCount);
    std::fill(buffer.values.begin(), buffer.values.end(), 0.0);
    return true;
}

inline std::span<const std::uint8_t> WaveletDoublePayloadBytes(const std::span<const double> values) noexcept {
    return std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(values.data()),
        values.size() * sizeof(double));
}

inline HaarLevel1Decomposition HaarDecomposeLevel1(const std::vector<double>& values) {
    constexpr double kInvSqrt2 = 0.70710678118654752440;
    HaarLevel1Decomposition decomposition;
    const auto pairCount = values.size() / 2u;
    decomposition.low.assign(pairCount + (values.size() % 2u), 0.0);
    decomposition.high.assign(pairCount, 0.0);
    for (std::size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
        const auto evenValue = values[pairIndex * 2u];
        const auto oddValue = values[pairIndex * 2u + 1u];
        decomposition.low[pairIndex] = (evenValue + oddValue) * kInvSqrt2;
        decomposition.high[pairIndex] = (evenValue - oddValue) * kInvSqrt2;
    }
    if ((values.size() % 2u) != 0u) {
        decomposition.low[pairCount] = values.back();
    }
    return decomposition;
}

inline std::vector<double> HaarReconstructLevel1(
    const std::vector<double>& low,
    const std::vector<double>& high,
    const std::size_t originalSize) {
    constexpr double kInvSqrt2 = 0.70710678118654752440;
    std::vector<double> reconstructed(originalSize, 0.0);
    const auto pairCount = originalSize / 2u;
    for (std::size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
        const auto lowValue = pairIndex < low.size() ? low[pairIndex] : 0.0;
        const auto highValue = pairIndex < high.size() ? high[pairIndex] : 0.0;
        reconstructed[pairIndex * 2u] = (lowValue + highValue) * kInvSqrt2;
        reconstructed[pairIndex * 2u + 1u] = (lowValue - highValue) * kInvSqrt2;
    }
    if ((originalSize % 2u) != 0u && pairCount < low.size()) {
        reconstructed.back() = low[pairCount];
    }
    return reconstructed;
}

template<typename TValue>
inline std::vector<double> ExtractBlockComponentAsDoubleTyped(
    const TValue* values,
    const std::size_t tupleCount,
    const std::size_t componentCount,
    const std::size_t componentIndex) {
    std::vector<double> output(tupleCount, 0.0);
    for (std::size_t tupleIndex = 0; tupleIndex < tupleCount; ++tupleIndex) {
        output[tupleIndex] = static_cast<double>(values[tupleIndex * componentCount + componentIndex]);
    }
    return output;
}

template<typename TValue>
inline void WriteBlockComponentFromDoubleTyped(
    TValue* outputValues,
    const std::size_t tupleCount,
    const std::size_t componentCount,
    const std::size_t componentIndex,
    const std::vector<double>& values) {
    for (std::size_t tupleIndex = 0; tupleIndex < tupleCount && tupleIndex < values.size(); ++tupleIndex) {
        outputValues[tupleIndex * componentCount + componentIndex] = static_cast<TValue>(values[tupleIndex]);
    }
}

inline void AppendBytes(std::vector<std::uint8_t>& output, const std::span<const std::uint8_t> bytes) {
    output.insert(output.end(), bytes.begin(), bytes.end());
}

inline void AppendWaveletScalarBlobWithLength(
    std::vector<std::uint8_t>& output,
    const WaveletScalarEncodedBytes& bytes) {
    detail::AppendScalar(output, static_cast<std::uint64_t>(bytes.EncodedSize()));
    if (!bytes.hasCodecHeader) {
        return;
    }
    output.push_back(static_cast<std::uint8_t>(bytes.codec));
    AppendBytes(output, bytes.payloadBytes);
}

template<typename TValue>
inline bool BuildWaveletDeltaScratchTyped(
    const TValue* currentValues,
    const TValue* referenceValues,
    const std::size_t tupleCount,
    const std::size_t componentCount,
    const std::size_t componentIndex,
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    WaveletScratchDoubles& lowDelta,
    WaveletScratchDoubles& highDelta,
    std::string* error = nullptr) {
    constexpr double kInvSqrt2 = 0.70710678118654752440;
    const auto pairCount = tupleCount / 2u;
    const auto lowCount = pairCount + (tupleCount % 2u);
    if (!AcquireWaveletScratchDoubles(scratchBytePool, acquireScratchQuota, lowCount, lowDelta, error) ||
        !AcquireWaveletScratchDoubles(scratchBytePool, acquireScratchQuota, pairCount, highDelta, error)) {
        return false;
    }
    for (std::size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
        const auto evenIndex = pairIndex * 2u * componentCount + componentIndex;
        const auto oddIndex = (pairIndex * 2u + 1u) * componentCount + componentIndex;
        const auto currentEven = static_cast<double>(currentValues[evenIndex]);
        const auto currentOdd = static_cast<double>(currentValues[oddIndex]);
        const auto referenceEven = static_cast<double>(referenceValues[evenIndex]);
        const auto referenceOdd = static_cast<double>(referenceValues[oddIndex]);
        lowDelta.values[pairIndex] =
            ((currentEven + currentOdd) - (referenceEven + referenceOdd)) * kInvSqrt2;
        highDelta.values[pairIndex] =
            ((currentEven - currentOdd) - (referenceEven - referenceOdd)) * kInvSqrt2;
    }
    if ((tupleCount % 2u) != 0u) {
        const auto valueIndex = (tupleCount - 1u) * componentCount + componentIndex;
        lowDelta.values[pairCount] =
            static_cast<double>(currentValues[valueIndex]) -
            static_cast<double>(referenceValues[valueIndex]);
    }
    return true;
}

inline bool ReadBlobWithLength(
    const std::span<const std::uint8_t> input,
    std::size_t& cursor,
    std::vector<std::uint8_t>& bytes,
    std::string* error = nullptr) {
    std::uint64_t size = 0u;
    if (!detail::ReadScalar(input, cursor, size, error)) {
        return false;
    }
    std::size_t localSize = 0u;
    if (!validation::CheckedCastSizeT(size, localSize, "wavelet blob size", error)) {
        return false;
    }
    if (cursor > input.size() || localSize > input.size() - cursor) {
        return validation::AssignError(error, "unexpected end of bytes while reading wavelet blob");
    }
    bytes.resize(localSize);
    if (localSize > 0u) {
        std::memcpy(bytes.data(), input.data() + cursor, localSize);
    }
    if (!validation::CheckedAddSizeT(cursor, localSize, cursor, "wavelet blob cursor", error)) {
        return false;
    }
    return true;
}

inline bool CompressWaveletScalars(
    const std::span<const double> values,
    const std::size_t valueSize,
    const CompressorConfig& compressor,
    WaveletScalarEncodedBytes& compressed,
    std::string* error = nullptr) {
    compressed.Reset();
    if (values.empty()) {
        return true;
    }

    if (valueSize != sizeof(float) && valueSize != sizeof(double)) {
        return validation::AssignError(error, "wavelet bytes requires float32 or float64 values");
    }

    std::string compressError;
    numericarray::NumericArrayEncodedBytes numericBytes;
    if (!numericarray::NumericArrayEncode::Compress(
            numericarray::NumericArrayBufferView{values.data(), MakeWaveletNumericArrayLayout(sizeof(double), values.size(), 1)},
            compressor,
            numericBytes,
            &compressError)) {
        return validation::AssignError(error, "wavelet scalar numeric array codec compression failed: " + compressError);
    }
    if (numericBytes.Empty()) {
        return validation::AssignError(error, "wavelet scalar numeric array codec returned empty bytes");
    }

    compressed.hasCodecHeader = true;
    compressed.codec = WaveletScalarBlobCodec::NumericArrayCodec;
    compressed.numericBytes = std::move(numericBytes);
    compressed.payloadBytes = compressed.numericBytes.Bytes();
    return true;
}

inline bool DecompressWaveletScalars(
    const std::vector<std::uint8_t>& blobBytes,
    const std::size_t elementCount,
    const std::size_t valueSize,
    const CompressorConfig& compressor,
    std::vector<double>& values,
    std::string* error = nullptr) {
    values.clear();
    if (elementCount == 0u) {
        return true;
    }

    if (valueSize != sizeof(float) && valueSize != sizeof(double)) {
        return validation::AssignError(error, "wavelet bytes requires float32 or float64 values");
    }

    if (blobBytes.empty()) {
        return validation::AssignError(error, "wavelet blob is missing codec header");
    }

    const auto codec = static_cast<WaveletScalarBlobCodec>(blobBytes[0]);
    const auto bytes = std::span<const std::uint8_t>(
        blobBytes.data() + sizeof(std::uint8_t),
        blobBytes.size() - sizeof(std::uint8_t));

    if (codec == WaveletScalarBlobCodec::RawDouble) {
        std::size_t expectedBytes = 0u;
        if (!validation::CheckedMulSizeT(
                elementCount,
                sizeof(double),
                expectedBytes,
                "wavelet raw blob bytes",
                error)) {
            return false;
        }
        if (bytes.size() != expectedBytes) {
            return validation::AssignError(error, "wavelet raw blob size mismatch");
        }
        std::vector<double> buffer(elementCount, 0.0);
        if (!bytes.empty()) {
            std::memcpy(buffer.data(), bytes.data(), bytes.size());
        }
        values = std::move(buffer);
        return true;
    }

    if (codec != WaveletScalarBlobCodec::NumericArrayCodec) {
        return validation::AssignError(error, "wavelet blob codec is invalid");
    }

    const auto layout = MakeWaveletNumericArrayLayout(sizeof(double), elementCount, 1);

    std::vector<double> buffer(elementCount, 0.0);
    if (!numericarray::NumericArrayDecode::Decompress(
            bytes,
            layout,
            compressor,
            numericarray::MutableNumericArrayBufferView{buffer.data(), layout},
            error)) {
        return false;
    }
    values = std::move(buffer);
    return true;
}

template<typename TValue>
inline bool EncodeWaveletDeltaBlockBytesTyped(
    const std::span<const std::uint8_t> currentBytes,
    const std::span<const std::uint8_t> referenceBytes,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    const CompressorConfig& compressor,
    std::vector<std::uint8_t>& bytes,
    std::string* error = nullptr) {
    bytes.clear();
    if (componentCount == 0u) {
        return validation::AssignError(error, "wavelet delta component count is invalid");
    }

    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            sizeof(TValue),
            tupleBytes,
            "wavelet delta tuple bytes",
            error)) {
        return false;
    }
    if (tupleBytes == 0u || currentBytes.size() % tupleBytes != 0u || referenceBytes.size() % tupleBytes != 0u) {
        return validation::AssignError(error, "wavelet delta byte range does not align with tuple size");
    }

    const auto totalTupleCount = currentBytes.size() / tupleBytes;
    const auto referenceTupleCount = referenceBytes.size() / tupleBytes;
    if (static_cast<std::size_t>(elementOffset) + static_cast<std::size_t>(elementCount) > totalTupleCount ||
        static_cast<std::size_t>(elementOffset) + static_cast<std::size_t>(elementCount) > referenceTupleCount) {
        return validation::AssignError(error, "wavelet delta block range exceeds current or reference bytes");
    }

    const auto* currentValues =
        reinterpret_cast<const TValue*>(currentBytes.data()) + static_cast<std::size_t>(elementOffset) * componentCount;
    const auto* referenceValues = reinterpret_cast<const TValue*>(referenceBytes.data()) +
        static_cast<std::size_t>(elementOffset) * componentCount;
    for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
        WaveletScratchDoubles lowDelta;
        WaveletScratchDoubles highDelta;
        if (!BuildWaveletDeltaScratchTyped(
                currentValues,
                referenceValues,
                elementCount,
                componentCount,
                componentIndex,
                scratchBytePool,
                acquireScratchQuota,
                lowDelta,
                highDelta,
                error)) {
            return false;
        }
        WaveletScalarEncodedBytes lowBlob;
        if (!CompressWaveletScalars(lowDelta.values, sizeof(TValue), compressor, lowBlob, error)) {
            return false;
        }
        AppendWaveletScalarBlobWithLength(bytes, lowBlob);
        lowDelta.Release();

        WaveletScalarEncodedBytes highBlob;
        if (!CompressWaveletScalars(highDelta.values, sizeof(TValue), compressor, highBlob, error)) {
            return false;
        }
        AppendWaveletScalarBlobWithLength(bytes, highBlob);
        highDelta.Release();
    }

    return true;
}

struct IntegerHaarLevel1Decomposition {
    std::vector<std::uint64_t> low;
    std::vector<std::uint64_t> high;
    bool hasTail{false};
    std::uint64_t tail{0u};
};

inline bool ValidateIntegerWaveletDataTypeSize(
    const DataType dataType,
    const std::size_t valueSize,
    std::string* error = nullptr) {
    if (!numericarray::IsIntegerNumericArrayDataType(dataType) ||
        valueSize == 0u ||
        valueSize > sizeof(std::uint64_t) ||
        valueSize != DataTypeSize(dataType)) {
        return validation::AssignError(error, "integer wavelet data type and value size do not match");
    }
    return true;
}

inline bool ValidateIntegerWaveletBlockRange(
    const std::span<const std::uint8_t> bytes,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const std::size_t valueSize,
    const char* label,
    std::string* error = nullptr) {
    if (componentCount == 0u || valueSize == 0u) {
        return validation::AssignError(error, "integer wavelet tuple layout is invalid");
    }
    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            valueSize,
            tupleBytes,
            "integer wavelet tuple size",
            error)) {
        return false;
    }
    if (bytes.size() % tupleBytes != 0u) {
        return validation::AssignError(error, std::string(label) + " bytes do not align with tuple size");
    }
    const auto tupleCount = bytes.size() / tupleBytes;
    if (static_cast<std::size_t>(elementOffset) + static_cast<std::size_t>(elementCount) > tupleCount) {
        return validation::AssignError(error, std::string(label) + " range exceeds tuple count");
    }
    return true;
}

inline bool BuildIntegerWaveletComponent(
    const std::span<const std::uint8_t> bytes,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const std::size_t componentIndex,
    const DataType dataType,
    const std::size_t valueSize,
    IntegerHaarLevel1Decomposition& decomposition,
    std::string* error = nullptr) {
    decomposition = {};
    if (componentIndex >= componentCount) {
        return validation::AssignError(error, "integer wavelet component index is out of range");
    }
    if (!ValidateIntegerWaveletBlockRange(
            bytes,
            elementOffset,
            elementCount,
            componentCount,
            valueSize,
            "integer wavelet input",
            error)) {
        return false;
    }

    const auto mask = numericarray::IntegerStorageMask(valueSize);
    const auto tupleBytes = validation::SaturatingMulSizeT(componentCount, valueSize);
    const auto pairCount = static_cast<std::size_t>(elementCount) / 2u;
    decomposition.low.assign(pairCount, 0u);
    decomposition.high.assign(pairCount, 0u);
    auto readCode = [&](const std::size_t localElementIndex) {
        const auto* valueBytes = bytes.data() +
            (static_cast<std::size_t>(elementOffset) + localElementIndex) * tupleBytes +
            componentIndex * valueSize;
        const auto rawValue = numericarray::ReadIntegerStorageValue(valueBytes, valueSize);
        return numericarray::ToIntegerOrderCode(dataType, rawValue, valueSize);
    };
    for (std::size_t pairIndex = 0u; pairIndex < pairCount; ++pairIndex) {
        const auto evenCode = readCode(pairIndex * 2u);
        const auto oddCode = readCode(pairIndex * 2u + 1u);
        const auto high = (oddCode - evenCode) & mask;
        const auto half = numericarray::SignedModuloFloorHalf(high, valueSize);
        decomposition.high[pairIndex] = high;
        decomposition.low[pairIndex] = numericarray::AddSignedOffsetModulo(evenCode, half, valueSize);
    }
    decomposition.hasTail = (elementCount % 2u) != 0u;
    if (decomposition.hasTail) {
        decomposition.tail = readCode(pairCount * 2u);
    }
    return true;
}

inline bool BuildIntegerWaveletResidualZigZags(
    const std::span<const std::uint64_t> current,
    const std::span<const std::uint64_t> reference,
    const std::size_t valueSize,
    std::vector<std::uint64_t>& residuals,
    std::string* error = nullptr) {
    residuals.clear();
    if (current.size() != reference.size()) {
        return validation::AssignError(error, "integer wavelet residual component size mismatch");
    }
    const auto mask = numericarray::IntegerStorageMask(valueSize);
    residuals.reserve(current.size());
    for (std::size_t index = 0u; index < current.size(); ++index) {
        const auto delta = (current[index] - reference[index]) & mask;
        residuals.push_back(numericarray::SignedModuloToZigZag(delta, valueSize));
    }
    return true;
}

inline bool AppendIntegerWaveletResidualBlobWithLength(
    std::vector<std::uint8_t>& output,
    const std::span<const std::uint64_t> residuals,
    const std::size_t valueSize,
    std::string* error = nullptr) {
    std::vector<std::uint8_t> payload;
    if (!numericarray::EncodeIntegerResidualLiteralRunVarint(
            residuals,
            payload,
            valueSize,
            error)) {
        return false;
    }
    detail::AppendScalar(output, static_cast<std::uint64_t>(payload.size()));
    AppendBytes(output, std::span<const std::uint8_t>(payload.data(), payload.size()));
    return true;
}

inline bool ReadIntegerWaveletResidualBlobWithLength(
    const std::span<const std::uint8_t> input,
    std::size_t& cursor,
    const std::size_t expectedCount,
    const std::size_t valueSize,
    std::vector<std::uint64_t>& residuals,
    std::string* error = nullptr) {
    std::vector<std::uint8_t> payload;
    if (!ReadBlobWithLength(input, cursor, payload, error)) {
        return false;
    }
    return numericarray::DecodeIntegerResidualLiteralRunVarint(
        std::span<const std::uint8_t>(payload.data(), payload.size()),
        expectedCount,
        valueSize,
        residuals,
        error);
}

inline bool ApplyIntegerWaveletResiduals(
    const std::span<const std::uint64_t> reference,
    const std::span<const std::uint64_t> residuals,
    const std::size_t valueSize,
    std::vector<std::uint64_t>& output,
    std::string* error = nullptr) {
    output.clear();
    if (reference.size() != residuals.size()) {
        return validation::AssignError(error, "integer wavelet residual count does not match reference");
    }
    const auto mask = numericarray::IntegerStorageMask(valueSize);
    output.reserve(reference.size());
    for (std::size_t index = 0u; index < reference.size(); ++index) {
        std::uint64_t delta = 0u;
        if (!numericarray::ZigZagToSignedModulo(residuals[index], valueSize, delta, error)) {
            output.clear();
            return false;
        }
        output.push_back((reference[index] + delta) & mask);
    }
    return true;
}

inline bool EncodeIntegerWaveletDeltaBlockBytes(
    const std::span<const std::uint8_t> currentBytes,
    const std::span<const std::uint8_t> referenceBytes,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const DataType dataType,
    const std::size_t valueSize,
    std::vector<std::uint8_t>& bytes,
    std::string* error = nullptr) {
    bytes.clear();
    if (!ValidateIntegerWaveletDataTypeSize(dataType, valueSize, error)) {
        return false;
    }
    if (!ValidateIntegerWaveletBlockRange(
            currentBytes,
            elementOffset,
            elementCount,
            componentCount,
            valueSize,
            "integer wavelet current",
            error) ||
        !ValidateIntegerWaveletBlockRange(
            referenceBytes,
            elementOffset,
            elementCount,
            componentCount,
            valueSize,
            "integer wavelet reference",
            error)) {
        return false;
    }

    for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex) {
        IntegerHaarLevel1Decomposition currentWavelet;
        IntegerHaarLevel1Decomposition referenceWavelet;
        if (!BuildIntegerWaveletComponent(
                currentBytes,
                elementOffset,
                elementCount,
                componentCount,
                componentIndex,
                dataType,
                valueSize,
                currentWavelet,
                error) ||
            !BuildIntegerWaveletComponent(
                referenceBytes,
                elementOffset,
                elementCount,
                componentCount,
                componentIndex,
                dataType,
                valueSize,
                referenceWavelet,
                error)) {
            bytes.clear();
            return false;
        }

        std::vector<std::uint64_t> residuals;
        if (!BuildIntegerWaveletResidualZigZags(
                std::span<const std::uint64_t>(currentWavelet.low.data(), currentWavelet.low.size()),
                std::span<const std::uint64_t>(referenceWavelet.low.data(), referenceWavelet.low.size()),
                valueSize,
                residuals,
                error) ||
            !AppendIntegerWaveletResidualBlobWithLength(
                bytes,
                std::span<const std::uint64_t>(residuals.data(), residuals.size()),
                valueSize,
                error) ||
            !BuildIntegerWaveletResidualZigZags(
                std::span<const std::uint64_t>(currentWavelet.high.data(), currentWavelet.high.size()),
                std::span<const std::uint64_t>(referenceWavelet.high.data(), referenceWavelet.high.size()),
                valueSize,
                residuals,
                error) ||
            !AppendIntegerWaveletResidualBlobWithLength(
                bytes,
                std::span<const std::uint64_t>(residuals.data(), residuals.size()),
                valueSize,
                error)) {
            bytes.clear();
            return false;
        }

        if (currentWavelet.hasTail) {
            const std::uint64_t tailResidual =
                numericarray::SignedModuloToZigZag(
                    (currentWavelet.tail - referenceWavelet.tail) & numericarray::IntegerStorageMask(valueSize),
                    valueSize);
            if (!AppendIntegerWaveletResidualBlobWithLength(
                    bytes,
                    std::span<const std::uint64_t>(&tailResidual, 1u),
                    valueSize,
                    error)) {
                bytes.clear();
                return false;
            }
        }
    }
    return true;
}

inline bool DecodeIntegerWaveletDeltaBlockBytes(
    const std::span<const std::uint8_t> referenceBytes,
    const std::size_t referenceElementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const DataType dataType,
    const std::size_t valueSize,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedBlockBytes,
    std::string* error = nullptr) {
    decodedBlockBytes.clear();
    if (!ValidateIntegerWaveletDataTypeSize(dataType, valueSize, error)) {
        return false;
    }
    if (referenceElementOffset > std::numeric_limits<std::uint32_t>::max()) {
        return validation::AssignError(error, "integer wavelet reference offset exceeds uint32 range");
    }
    const auto referenceOffset = static_cast<std::uint32_t>(referenceElementOffset);
    if (!ValidateIntegerWaveletBlockRange(
            referenceBytes,
            referenceOffset,
            elementCount,
            componentCount,
            valueSize,
            "integer wavelet reference",
            error)) {
        return false;
    }
    std::size_t tupleBytes = 0u;
    std::size_t decodedByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            valueSize,
            tupleBytes,
            "integer wavelet tuple size",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(elementCount),
            tupleBytes,
            decodedByteCount,
            "integer wavelet decoded byte size",
            error)) {
        return false;
    }
    decodedBlockBytes.assign(decodedByteCount, 0u);

    const auto mask = numericarray::IntegerStorageMask(valueSize);
    std::size_t cursor = 0u;
    const auto pairCount = static_cast<std::size_t>(elementCount) / 2u;
    for (std::size_t componentIndex = 0u; componentIndex < componentCount; ++componentIndex) {
        IntegerHaarLevel1Decomposition referenceWavelet;
        if (!BuildIntegerWaveletComponent(
                referenceBytes,
                referenceOffset,
                elementCount,
                componentCount,
                componentIndex,
                dataType,
                valueSize,
                referenceWavelet,
                error)) {
            decodedBlockBytes.clear();
            return false;
        }

        std::vector<std::uint64_t> lowResiduals;
        std::vector<std::uint64_t> highResiduals;
        std::vector<std::uint64_t> low;
        std::vector<std::uint64_t> high;
        if (!ReadIntegerWaveletResidualBlobWithLength(
                bytes,
                cursor,
                referenceWavelet.low.size(),
                valueSize,
                lowResiduals,
                error) ||
            !ApplyIntegerWaveletResiduals(
                std::span<const std::uint64_t>(referenceWavelet.low.data(), referenceWavelet.low.size()),
                std::span<const std::uint64_t>(lowResiduals.data(), lowResiduals.size()),
                valueSize,
                low,
                error) ||
            !ReadIntegerWaveletResidualBlobWithLength(
                bytes,
                cursor,
                referenceWavelet.high.size(),
                valueSize,
                highResiduals,
                error) ||
            !ApplyIntegerWaveletResiduals(
                std::span<const std::uint64_t>(referenceWavelet.high.data(), referenceWavelet.high.size()),
                std::span<const std::uint64_t>(highResiduals.data(), highResiduals.size()),
                valueSize,
                high,
                error)) {
            decodedBlockBytes.clear();
            return false;
        }

        auto writeCode = [&](const std::size_t localElementIndex, const std::uint64_t code) {
            const auto rawValue = numericarray::FromIntegerOrderCode(dataType, code, valueSize);
            numericarray::WriteIntegerStorageValue(
                rawValue,
                decodedBlockBytes.data() + localElementIndex * tupleBytes + componentIndex * valueSize,
                valueSize);
        };
        for (std::size_t pairIndex = 0u; pairIndex < pairCount; ++pairIndex) {
            const auto half = numericarray::SignedModuloFloorHalf(high[pairIndex], valueSize);
            const auto evenCode = numericarray::AddSignedOffsetModulo(low[pairIndex], -half, valueSize);
            const auto oddCode = (evenCode + high[pairIndex]) & mask;
            writeCode(pairIndex * 2u, evenCode);
            writeCode(pairIndex * 2u + 1u, oddCode);
        }
        if ((elementCount % 2u) != 0u) {
            std::vector<std::uint64_t> tailResidual;
            if (!ReadIntegerWaveletResidualBlobWithLength(
                    bytes,
                    cursor,
                    1u,
                    valueSize,
                    tailResidual,
                    error)) {
                decodedBlockBytes.clear();
                return false;
            }
            std::uint64_t tailDelta = 0u;
            if (!numericarray::ZigZagToSignedModulo(tailResidual[0], valueSize, tailDelta, error)) {
                decodedBlockBytes.clear();
                return false;
            }
            writeCode(pairCount * 2u, (referenceWavelet.tail + tailDelta) & mask);
        }
    }

    if (cursor != bytes.size()) {
        decodedBlockBytes.clear();
        return validation::AssignError(error, "integer wavelet bytes has trailing bytes");
    }
    return true;
}

inline bool EncodeWaveletDeltaBlockBytes(
    const std::span<const std::uint8_t> currentBytes,
    const std::span<const std::uint8_t> referenceBytes,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const DataType dataType,
    const std::size_t valueSize,
    ScratchByteBufferPool& scratchBytePool,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    const CompressorConfig& compressor,
    std::vector<std::uint8_t>& bytes,
    std::string* error = nullptr) {
    if (numericarray::IsIntegerNumericArrayDataType(dataType)) {
        return EncodeIntegerWaveletDeltaBlockBytes(
            currentBytes,
            referenceBytes,
            elementOffset,
            elementCount,
            componentCount,
            dataType,
            valueSize,
            bytes,
            error);
    }
    if (dataType == DataType::Float32 && valueSize == sizeof(float)) {
        return EncodeWaveletDeltaBlockBytesTyped<float>(
            currentBytes,
            referenceBytes,
            elementOffset,
            elementCount,
            componentCount,
            scratchBytePool,
            acquireScratchQuota,
            compressor,
            bytes,
            error);
    }
    if (dataType == DataType::Float64 && valueSize == sizeof(double)) {
        return EncodeWaveletDeltaBlockBytesTyped<double>(
            currentBytes,
            referenceBytes,
            elementOffset,
            elementCount,
            componentCount,
            scratchBytePool,
            acquireScratchQuota,
            compressor,
            bytes,
            error);
    }

    bytes.clear();
    return validation::AssignError(error, "wavelet delta requires float32, float64, or integer values");
}

template<typename TValue>
inline bool DecodeWaveletDeltaBlockBytesTyped(
    const std::span<const std::uint8_t> referenceBytes,
    const std::size_t referenceElementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const CompressorConfig& compressor,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedBlockBytes,
    NumericArrayReferenceCodecDecodeTelemetry* telemetry = nullptr,
    std::string* error = nullptr) {
    decodedBlockBytes.clear();
    if (componentCount == 0u) {
        return validation::AssignError(error, "wavelet delta component count is invalid");
    }

    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            sizeof(TValue),
            tupleBytes,
            "wavelet delta tuple bytes",
            error)) {
        return false;
    }
    if (tupleBytes == 0u || referenceBytes.size() % tupleBytes != 0u) {
        return validation::AssignError(error, "wavelet delta reference bytes do not align with tuple size");
    }

    const auto referenceTupleCount = referenceBytes.size() / tupleBytes;
    std::size_t referenceRangeEnd = 0u;
    if (!validation::CheckedAddSizeT(
            referenceElementOffset,
            static_cast<std::size_t>(elementCount),
            referenceRangeEnd,
            "wavelet delta reference range end",
            error)) {
        return false;
    }
    if (referenceRangeEnd > referenceTupleCount) {
        return validation::AssignError(error, "wavelet delta reference range exceeds reference bytes");
    }

    std::size_t decodedByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(elementCount),
            tupleBytes,
            decodedByteCount,
            "wavelet decoded block bytes",
            error)) {
        return false;
    }
    decodedBlockBytes.resize(decodedByteCount);
    auto* outputValues = reinterpret_cast<TValue*>(decodedBlockBytes.data());
    const auto* referenceValues =
        reinterpret_cast<const TValue*>(referenceBytes.data()) + referenceElementOffset * componentCount;
    const auto addTelemetryBytes = [](ParamSize& total, const std::size_t bytesToAdd) {
        const auto added = static_cast<ParamSize>(bytesToAdd);
        if (added > std::numeric_limits<ParamSize>::max() - total) {
            total = std::numeric_limits<ParamSize>::max();
            return;
        }
        total += added;
    };
    const auto addDoubleCapacityBytes = [&addTelemetryBytes](
                                            ParamSize& total,
                                            const std::vector<double>& values) {
        if (values.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(double)) {
            total = std::numeric_limits<ParamSize>::max();
            return;
        }
        addTelemetryBytes(total, values.capacity() * sizeof(double));
    };

    std::size_t cursor = 0u;
    for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
        std::vector<std::uint8_t> lowBlobBytes;
        std::vector<std::uint8_t> highBlobBytes;
        if (!ReadBlobWithLength(bytes, cursor, lowBlobBytes, error) ||
            !ReadBlobWithLength(bytes, cursor, highBlobBytes, error)) {
            return false;
        }
        if (telemetry != nullptr) {
            addTelemetryBytes(telemetry->waveletLowBlobBytes, lowBlobBytes.size());
            addTelemetryBytes(telemetry->waveletHighBlobBytes, highBlobBytes.size());
        }

        const auto referenceComponent = ExtractBlockComponentAsDoubleTyped(
            referenceValues,
            elementCount,
            componentCount,
            componentIndex);
        const auto referenceWavelet = HaarDecomposeLevel1(referenceComponent);
        std::vector<double> lowDelta;
        std::vector<double> highDelta;
        if (!DecompressWaveletScalars(lowBlobBytes, referenceWavelet.low.size(), sizeof(TValue), compressor, lowDelta, error) ||
            !DecompressWaveletScalars(highBlobBytes, referenceWavelet.high.size(), sizeof(TValue), compressor, highDelta, error)) {
            return false;
        }

        std::vector<double> low(referenceWavelet.low.size(), 0.0);
        std::vector<double> high(referenceWavelet.high.size(), 0.0);
        for (std::size_t index = 0; index < low.size(); ++index) {
            low[index] = referenceWavelet.low[index] + lowDelta[index];
        }
        for (std::size_t index = 0; index < high.size(); ++index) {
            high[index] = referenceWavelet.high[index] + highDelta[index];
        }

        const auto reconstructed = HaarReconstructLevel1(low, high, elementCount);
        if (telemetry != nullptr) {
            ParamSize temporaryDoubleBytes = 0u;
            addDoubleCapacityBytes(temporaryDoubleBytes, referenceComponent);
            addDoubleCapacityBytes(temporaryDoubleBytes, referenceWavelet.low);
            addDoubleCapacityBytes(temporaryDoubleBytes, referenceWavelet.high);
            addDoubleCapacityBytes(temporaryDoubleBytes, lowDelta);
            addDoubleCapacityBytes(temporaryDoubleBytes, highDelta);
            addDoubleCapacityBytes(temporaryDoubleBytes, low);
            addDoubleCapacityBytes(temporaryDoubleBytes, high);
            addDoubleCapacityBytes(temporaryDoubleBytes, reconstructed);
            telemetry->waveletPeakTemporaryDoubleBytes = std::max(
                telemetry->waveletPeakTemporaryDoubleBytes,
                temporaryDoubleBytes);
        }
        WriteBlockComponentFromDoubleTyped(
            outputValues,
            elementCount,
            componentCount,
            componentIndex,
            reconstructed);
    }

    if (cursor != bytes.size()) {
        decodedBlockBytes.clear();
        return validation::AssignError(error, "wavelet bytes has trailing bytes");
    }

    return true;
}

inline bool DecodeWaveletDeltaBlockBytes(
    const std::span<const std::uint8_t> referenceBytes,
    const std::size_t referenceElementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const DataType dataType,
    const std::size_t valueSize,
    const CompressorConfig& compressor,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedBlockBytes,
    NumericArrayReferenceCodecDecodeTelemetry* telemetry = nullptr,
    std::string* error = nullptr) {
    if (numericarray::IsIntegerNumericArrayDataType(dataType)) {
        return DecodeIntegerWaveletDeltaBlockBytes(
            referenceBytes,
            referenceElementOffset,
            elementCount,
            componentCount,
            dataType,
            valueSize,
            bytes,
            decodedBlockBytes,
            error);
    }
    if (dataType == DataType::Float32 && valueSize == sizeof(float)) {
        return DecodeWaveletDeltaBlockBytesTyped<float>(
            referenceBytes,
            referenceElementOffset,
            elementCount,
            componentCount,
            compressor,
            bytes,
            decodedBlockBytes,
            telemetry,
            error);
    }
    if (dataType == DataType::Float64 && valueSize == sizeof(double)) {
        return DecodeWaveletDeltaBlockBytesTyped<double>(
            referenceBytes,
            referenceElementOffset,
            elementCount,
            componentCount,
            compressor,
            bytes,
            decodedBlockBytes,
            telemetry,
            error);
    }

    decodedBlockBytes.clear();
    return validation::AssignError(error, "wavelet delta requires float32, float64, or integer values");
}

template<typename TValue>
inline bool ComputeWaveletReferenceArithmeticError(
    const NumericArrayReferenceCodecEncodeInput& input,
    double& maximumArithmeticError,
    std::string* error = nullptr) {
    maximumArithmeticError = 0.0;
    if (!ValidateNumericArrayReferenceBlockBytes<TValue>(input, error)) {
        return false;
    }
    constexpr double kInvSqrt2 = 0.70710678118654752440;
    const auto* currentValues = reinterpret_cast<const TValue*>(input.currentBytes.data());
    const auto* referenceValues = reinterpret_cast<const TValue*>(input.referenceBytes.data());
    const auto accumulateError = [&](const TValue expected, const TValue reconstructed) {
        if (expected == reconstructed) {
            return;
        }
        const auto expectedValue = static_cast<double>(expected);
        const auto reconstructedValue = static_cast<double>(reconstructed);
        if (!std::isfinite(expectedValue) || !std::isfinite(reconstructedValue)) {
            maximumArithmeticError = std::numeric_limits<double>::infinity();
            return;
        }
        maximumArithmeticError = std::max(
            maximumArithmeticError,
            std::abs(expectedValue - reconstructedValue));
    };
    const auto pairCount = static_cast<std::size_t>(input.elementCount) / 2u;
    for (std::size_t componentIndex = 0u; componentIndex < input.componentCount; ++componentIndex) {
        for (std::size_t pairIndex = 0u; pairIndex < pairCount; ++pairIndex) {
            const auto evenIndex = pairIndex * 2u * input.componentCount + componentIndex;
            const auto oddIndex = (pairIndex * 2u + 1u) * input.componentCount + componentIndex;
            const auto currentEven = static_cast<double>(currentValues[evenIndex]);
            const auto currentOdd = static_cast<double>(currentValues[oddIndex]);
            const auto referenceEven = static_cast<double>(referenceValues[evenIndex]);
            const auto referenceOdd = static_cast<double>(referenceValues[oddIndex]);
            const auto referenceLow = (referenceEven + referenceOdd) * kInvSqrt2;
            const auto referenceHigh = (referenceEven - referenceOdd) * kInvSqrt2;
            const auto lowDelta =
                ((currentEven + currentOdd) - (referenceEven + referenceOdd)) * kInvSqrt2;
            const auto highDelta =
                ((currentEven - currentOdd) - (referenceEven - referenceOdd)) * kInvSqrt2;
            const auto low = referenceLow + lowDelta;
            const auto high = referenceHigh + highDelta;
            accumulateError(
                currentValues[evenIndex],
                static_cast<TValue>((low + high) * kInvSqrt2));
            accumulateError(
                currentValues[oddIndex],
                static_cast<TValue>((low - high) * kInvSqrt2));
        }
        if ((input.elementCount % 2u) != 0u) {
            const auto valueIndex =
                (static_cast<std::size_t>(input.elementCount) - 1u) * input.componentCount + componentIndex;
            const auto delta =
                static_cast<double>(currentValues[valueIndex]) -
                static_cast<double>(referenceValues[valueIndex]);
            accumulateError(
                currentValues[valueIndex],
                static_cast<TValue>(static_cast<double>(referenceValues[valueIndex]) + delta));
        }
    }
    return true;
}

inline bool ValidateWaveletReferenceBlockShape(
    const NumericArrayReferenceCodecEncodeInput& input,
    std::string* error = nullptr) {
    if (input.meta.dataType == DataType::Float32 && NumericArrayValueSize(input.meta) == sizeof(float)) {
        return ValidateNumericArrayReferenceBlockBytes<float>(input, error);
    }
    if (input.meta.dataType == DataType::Float64 && NumericArrayValueSize(input.meta) == sizeof(double)) {
        return ValidateNumericArrayReferenceBlockBytes<double>(input, error);
    }
    if (!numericarray::IsIntegerNumericArrayDataType(input.meta.dataType) ||
        input.componentCount == 0u ||
        input.meta.dimension <= 0 ||
        static_cast<std::size_t>(input.meta.dimension) != input.componentCount ||
        NumericArrayValueSize(input.meta) != DataTypeSize(input.meta.dataType)) {
        return validation::AssignError(
            error,
            "wavelet reference codec requires float32, float64, or integer data");
    }
    std::size_t tupleBytes = 0u;
    std::size_t expectedBytes = 0u;
    if (!validation::CheckedMulSizeT(
            input.componentCount,
            static_cast<std::size_t>(NumericArrayValueSize(input.meta)),
            tupleBytes,
            "wavelet reference tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            static_cast<std::size_t>(input.elementCount),
            tupleBytes,
            expectedBytes,
            "wavelet reference block bytes",
            error)) {
        return false;
    }
    if (tupleBytes == 0u ||
        input.currentBytes.size() != expectedBytes ||
        input.referenceBytes.size() != expectedBytes) {
        return validation::AssignError(error, "wavelet reference block byte range is invalid");
    }
    return true;
}

class WaveletReferenceCodec final : public INumericArrayReferenceCodec {
public:
    [[nodiscard]] NumericArrayReferenceCodecId CodecId() const noexcept override {
        return NumericArrayReferenceCodecId::Wavelet;
    }

    [[nodiscard]] NumericArrayReferenceEncodeResult PrepareBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        NumericArrayReferencePreparedBlock& prepared,
        std::string* error = nullptr) const override {
        prepared.Reset();
        if (!ValidateWaveletReferenceBlockShape(input, error)) {
            return NumericArrayReferenceEncodeResult::Failed();
        }
        constexpr double kWaveletReconstructionErrorAmplification = 1.41421356237309504880;
        double maximumArithmeticError = 0.0;
        if (input.meta.dataType == DataType::Float32 && NumericArrayValueSize(input.meta) == sizeof(float)) {
            if (!ComputeWaveletReferenceArithmeticError<float>(
                    input,
                    maximumArithmeticError,
                    error)) {
                return NumericArrayReferenceEncodeResult::Failed();
            }
        } else if (input.meta.dataType == DataType::Float64 && NumericArrayValueSize(input.meta) == sizeof(double)) {
            if (!ComputeWaveletReferenceArithmeticError<double>(
                    input,
                    maximumArithmeticError,
                    error)) {
                return NumericArrayReferenceEncodeResult::Failed();
            }
        }
        if (!ResolveReferenceResidualCompressor(
                input.meta,
                input.currentBytes,
                input.defaultCompressor,
                kWaveletReconstructionErrorAmplification,
                prepared.residualCompressor,
                error)) {
            return NumericArrayReferenceEncodeResult::Failed();
        }
        const auto precisionResult = ConsumeReferenceArithmeticErrorBudget(
                prepared.residualCompressor,
                kWaveletReconstructionErrorAmplification,
                maximumArithmeticError,
                error);
        if (!precisionResult.IsEncoded()) {
            prepared.Reset();
            return precisionResult;
        }
        prepared.codecId = CodecId();
        return NumericArrayReferenceEncodeResult::Encoded();
    }

    [[nodiscard]] NumericArrayReferenceEncodeResult EncodePreparedBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        const NumericArrayReferencePreparedBlock& prepared,
        NumericArrayReferenceEncodedBlock& output,
        std::string* error = nullptr) const override {
        output = {};
        if (prepared.codecId != CodecId() ||
            !ValidateWaveletReferenceBlockShape(input, error)) {
            if (prepared.codecId != CodecId() && error != nullptr && error->empty()) {
                validation::AssignError(error, "prepared wavelet reference block is invalid");
            }
            return NumericArrayReferenceEncodeResult::Failed();
        }
        WaveletReferenceBlockFields fields;
        fields.waveletBytes = input.AcquireScratch(input.currentBytes.size());
        auto& waveletBytes = fields.waveletBytes.Bytes();
        if (!EncodeWaveletDeltaBlockBytes(
                input.currentBytes,
                input.referenceBytes,
                0u,
                input.elementCount,
                input.componentCount,
                input.meta.dataType,
                static_cast<std::size_t>(NumericArrayValueSize(input.meta)),
                input.scratchBytePool,
                input.acquireScratchQuota,
                prepared.residualCompressor,
                waveletBytes,
                error)) {
            output = {};
            return NumericArrayReferenceEncodeResult::Failed();
        }
        output.header = NumericArrayBlockHeader{
            .mode = NumericArrayBlockMode::WaveletReference,
            .referenceKind = input.referenceKind,
            .codecId = CodecId(),
            .localParentFieldIndex = input.localParentFieldIndex,
            .elementOffset = input.elementOffset,
            .elementCount = input.elementCount,
            .encodedByteLength = static_cast<std::uint32_t>(waveletBytes.size()),
            .bytesCodec = NumericArrayBytesCodec::RawBytes,
            .predictorOffset = input.predictorOffset,
        };
        output.backgroundCompressor = prepared.residualCompressor;
        output.fields = std::move(fields);
        return NumericArrayReferenceEncodeResult::Encoded();
    }

    [[nodiscard]] bool DecodeBlock(
        const NumericArrayReferenceCodecDecodeInput& input,
        std::vector<std::uint8_t>& decodedBlockBytes,
        std::string* error = nullptr) const override {
        const auto componentCount = static_cast<std::size_t>(std::max(input.meta.dimension, 0));
        return DecodeWaveletDeltaBlockBytes(
            input.referenceBytes,
            input.referenceElementOffset,
            input.block.header.elementCount,
            componentCount,
            input.meta.dataType,
            static_cast<std::size_t>(NumericArrayValueSize(input.meta)),
            input.block.backgroundCompressor,
            input.block.bytes,
            decodedBlockBytes,
            input.telemetry,
            error);
    }
};

} // namespace datacodec

#endif
