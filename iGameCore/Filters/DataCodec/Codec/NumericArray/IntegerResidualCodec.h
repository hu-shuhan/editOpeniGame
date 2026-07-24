#ifndef DATACODEC_CODEC_NUMERICARRAY_INTEGERRESIDUALCODEC_H
#define DATACODEC_CODEC_NUMERICARRAY_INTEGERRESIDUALCODEC_H

#include "DataCodec/Codec/NumericArray/NumericArrayBlockFormat.h"
#include "DataCodec/Codec/SubCodec/VarintCodec.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>
namespace datacodec {
namespace numericarray {

inline bool IsSignedIntegerNumericArrayDataType(const DataType type) noexcept {
    switch (type) {
        case DataType::Int8:
        case DataType::Int16:
        case DataType::Int32:
        case DataType::Int64:
            return true;
        case DataType::UInt8:
        case DataType::UInt16:
        case DataType::UInt32:
        case DataType::UInt64:
        case DataType::Float32:
        case DataType::Float64:
            return false;
    }
    return false;
}

inline std::uint64_t IntegerStorageSignBit(const std::size_t valueSize) noexcept {
    if (valueSize == 0u) {
        return 0u;
    }
    if (valueSize >= sizeof(std::uint64_t)) {
        return std::uint64_t{1u} << 63u;
    }
    return std::uint64_t{1u} << (valueSize * 8u - 1u);
}

inline std::uint64_t ReadIntegerStorageValue(
    const std::uint8_t* bytes,
    const std::size_t valueSize) noexcept {
    std::uint64_t value = 0u;
    for (std::size_t byteIndex = 0u; byteIndex < valueSize; ++byteIndex) {
        value |= static_cast<std::uint64_t>(bytes[byteIndex]) << (byteIndex * 8u);
    }
    return value;
}

inline void WriteIntegerStorageValue(
    const std::uint64_t value,
    std::uint8_t* bytes,
    const std::size_t valueSize) noexcept {
    for (std::size_t byteIndex = 0u; byteIndex < valueSize; ++byteIndex) {
        bytes[byteIndex] = static_cast<std::uint8_t>((value >> (byteIndex * 8u)) & 0xFFu);
    }
}

inline std::uint64_t ToIntegerOrderCode(
    const DataType dataType,
    const std::uint64_t rawValue,
    const std::size_t valueSize) noexcept {
    const auto mask = IntegerStorageMask(valueSize);
    const auto value = rawValue & mask;
    return IsSignedIntegerNumericArrayDataType(dataType)
        ? ((value ^ IntegerStorageSignBit(valueSize)) & mask)
        : value;
}

inline std::uint64_t FromIntegerOrderCode(
    const DataType dataType,
    const std::uint64_t orderCode,
    const std::size_t valueSize) noexcept {
    const auto mask = IntegerStorageMask(valueSize);
    const auto value = orderCode & mask;
    return IsSignedIntegerNumericArrayDataType(dataType)
        ? ((value ^ IntegerStorageSignBit(valueSize)) & mask)
        : value;
}

inline bool IsIntegerResidualValueInRange(
    const std::uint64_t value,
    const std::size_t valueSize) noexcept {
    return valueSize >= sizeof(std::uint64_t) || value <= IntegerStorageMask(valueSize);
}

inline std::uint64_t SignedModuloToZigZag(
    const std::uint64_t rawDelta,
    const std::size_t valueSize) noexcept {
    const auto mask = IntegerStorageMask(valueSize);
    const auto signBit = IntegerStorageSignBit(valueSize);
    const auto delta = rawDelta & mask;
    if ((delta & signBit) == 0u) {
        return (delta << 1u) & std::numeric_limits<std::uint64_t>::max();
    }
    const auto magnitude = ((~delta) + 1u) & mask;
    return (magnitude << 1u) - 1u;
}

inline bool ZigZagToSignedModulo(
    const std::uint64_t encoded,
    const std::size_t valueSize,
    std::uint64_t& rawDelta,
    std::string* error = nullptr) {
    rawDelta = 0u;
    if (!IsIntegerResidualValueInRange(encoded, valueSize)) {
        return validation::AssignError(error, "integer residual value exceeds storage width");
    }
    const auto mask = IntegerStorageMask(valueSize);
    if ((encoded & 1u) == 0u) {
        rawDelta = (encoded >> 1u) & mask;
        return true;
    }
    const auto magnitude = (encoded >> 1u) + 1u;
    rawDelta = (std::uint64_t{0u} - magnitude) & mask;
    return true;
}

inline std::int64_t SignedModuloFloorHalf(
    const std::uint64_t rawDelta,
    const std::size_t valueSize) noexcept {
    const auto mask = IntegerStorageMask(valueSize);
    const auto signBit = IntegerStorageSignBit(valueSize);
    const auto delta = rawDelta & mask;
    if ((delta & signBit) == 0u) {
        return static_cast<std::int64_t>(delta / 2u);
    }
    const auto magnitude = ((~delta) + 1u) & mask;
    const auto halfMagnitude = (magnitude + 1u) / 2u;
    return -static_cast<std::int64_t>(halfMagnitude);
}

inline std::uint64_t AddSignedOffsetModulo(
    const std::uint64_t value,
    const std::int64_t offset,
    const std::size_t valueSize) noexcept {
    const auto mask = IntegerStorageMask(valueSize);
    if (offset >= 0) {
        return (value + static_cast<std::uint64_t>(offset)) & mask;
    }
    const auto magnitude = offset == std::numeric_limits<std::int64_t>::min()
        ? (std::uint64_t{1u} << 63u)
        : static_cast<std::uint64_t>(-offset);
    return (value - magnitude) & mask;
}

inline bool EncodeIntegerResidualLiteralRunVarint(
    const std::span<const std::uint64_t> residuals,
    std::vector<std::uint8_t>& encodedBytes,
    const std::size_t valueSize,
    std::string* error = nullptr) {
    encodedBytes.clear();
    std::size_t index = 0u;
    while (index < residuals.size()) {
        std::size_t runLength = 1u;
        while (index + runLength < residuals.size() &&
               residuals[index + runLength] == residuals[index]) {
            ++runLength;
        }
        if (runLength >= 3u) {
            if (!IsIntegerResidualValueInRange(residuals[index], valueSize)) {
                encodedBytes.clear();
                return validation::AssignError(error, "integer residual value exceeds storage width");
            }
            codec::EncodeVarint64(encodedBytes, (static_cast<std::uint64_t>(runLength) << 1u) | 1u);
            codec::EncodeVarint64(encodedBytes, residuals[index]);
            index += runLength;
            continue;
        }

        const auto literalStart = index;
        index += runLength;
        while (index < residuals.size()) {
            runLength = 1u;
            while (index + runLength < residuals.size() &&
                   residuals[index + runLength] == residuals[index]) {
                ++runLength;
            }
            if (runLength >= 3u) {
                break;
            }
            index += runLength;
        }
        const auto literalCount = index - literalStart;
        codec::EncodeVarint64(encodedBytes, static_cast<std::uint64_t>(literalCount) << 1u);
        for (std::size_t literalIndex = literalStart; literalIndex < index; ++literalIndex) {
            if (!IsIntegerResidualValueInRange(residuals[literalIndex], valueSize)) {
                encodedBytes.clear();
                return validation::AssignError(error, "integer residual value exceeds storage width");
            }
            codec::EncodeVarint64(encodedBytes, residuals[literalIndex]);
        }
    }
    return true;
}

inline bool DecodeIntegerResidualLiteralRunVarint(
    const std::span<const std::uint8_t> bytes,
    const std::size_t expectedCount,
    const std::size_t valueSize,
    std::vector<std::uint64_t>& residuals,
    std::string* error = nullptr) {
    residuals.clear();
    residuals.reserve(expectedCount);
    std::size_t cursor = 0u;
    while (cursor < bytes.size()) {
        std::uint64_t header = 0u;
        if (!codec::DecodeVarint64(bytes, cursor, header, error)) {
            residuals.clear();
            return false;
        }
        const auto isRun = (header & 1u) != 0u;
        const auto count = static_cast<std::size_t>(header >> 1u);
        if (count == 0u || count > expectedCount - residuals.size()) {
            residuals.clear();
            return validation::AssignError(error, "integer residual literal-run length is invalid");
        }
        if (isRun) {
            std::uint64_t value = 0u;
            if (!codec::DecodeVarint64(bytes, cursor, value, error) ||
                !IsIntegerResidualValueInRange(value, valueSize)) {
                if (error != nullptr && !error->empty()) {
                    residuals.clear();
                    return false;
                }
                residuals.clear();
                return validation::AssignError(error, "integer residual run value is invalid");
            }
            residuals.insert(residuals.end(), count, value);
            continue;
        }
        for (std::size_t item = 0u; item < count; ++item) {
            std::uint64_t value = 0u;
            if (!codec::DecodeVarint64(bytes, cursor, value, error) ||
                !IsIntegerResidualValueInRange(value, valueSize)) {
                if (error != nullptr && !error->empty()) {
                    residuals.clear();
                    return false;
                }
                residuals.clear();
                return validation::AssignError(error, "integer residual literal value is invalid");
            }
            residuals.push_back(value);
        }
    }
    if (residuals.size() != expectedCount) {
        residuals.clear();
        return validation::AssignError(error, "integer residual literal-run did not decode the expected count");
    }
    return true;
}

inline bool EncodeIntegerDeltaLiteralRunVarintComponentBytes(
    const NumericArrayBlockParams& params,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> rawBytes,
    std::vector<std::uint8_t>& encodedBytes,
    std::string* error = nullptr) {
    encodedBytes.clear();
    if (!IsIntegerNumericArrayDataType(params.dataType)) {
        return validation::AssignError(error, "integer delta literal-run varint requires an integer data type");
    }
    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayComponentRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }
    if (rawBytes.size() != rawByteCount) {
        return validation::AssignError(error, "integer component bytes size does not match block range");
    }

    const auto valueSize = params.valueSize;
    const auto mask = IntegerStorageMask(valueSize);
    std::vector<std::uint64_t> residuals;
    residuals.reserve(elementCount);
    std::uint64_t previousCode = 0u;
    for (std::uint32_t index = 0u; index < elementCount; ++index) {
        const auto rawValue = ReadIntegerStorageValue(
            rawBytes.data() + static_cast<std::size_t>(index) * valueSize,
            valueSize);
        const auto code = ToIntegerOrderCode(params.dataType, rawValue, valueSize);
        const auto delta = (code - previousCode) & mask;
        residuals.push_back(SignedModuloToZigZag(delta, valueSize));
        previousCode = code;
    }
    return EncodeIntegerResidualLiteralRunVarint(
        std::span<const std::uint64_t>(residuals.data(), residuals.size()),
        encodedBytes,
        valueSize,
        error);
}

inline bool DecodeIntegerDeltaLiteralRunVarintComponentBytes(
    const NumericArrayBlockParams& params,
    const std::uint32_t elementCount,
    const std::span<const std::uint8_t> bytes,
    std::vector<std::uint8_t>& decodedComponent,
    std::string* error = nullptr) {
    decodedComponent.clear();
    if (!IsIntegerNumericArrayDataType(params.dataType)) {
        return validation::AssignError(error, "integer delta literal-run varint requires an integer data type");
    }
    std::size_t rawByteCount = 0u;
    if (!ResolveNumericArrayComponentRawByteCount(params, elementCount, rawByteCount, error)) {
        return false;
    }

    std::vector<std::uint64_t> residuals;
    if (!DecodeIntegerResidualLiteralRunVarint(
            bytes,
            elementCount,
            params.valueSize,
            residuals,
            error)) {
        return false;
    }

    const auto valueSize = params.valueSize;
    const auto mask = IntegerStorageMask(valueSize);
    decodedComponent.assign(rawByteCount, 0u);
    std::uint64_t previousCode = 0u;
    for (std::uint32_t index = 0u; index < elementCount; ++index) {
        std::uint64_t delta = 0u;
        if (!ZigZagToSignedModulo(residuals[index], valueSize, delta, error)) {
            decodedComponent.clear();
            return false;
        }
        const auto code = (previousCode + delta) & mask;
        const auto rawValue = FromIntegerOrderCode(params.dataType, code, valueSize);
        WriteIntegerStorageValue(
            rawValue,
            decodedComponent.data() + static_cast<std::size_t>(index) * valueSize,
            valueSize);
        previousCode = code;
    }
    return true;
}

} // namespace numericarray
} // namespace datacodec

#endif
