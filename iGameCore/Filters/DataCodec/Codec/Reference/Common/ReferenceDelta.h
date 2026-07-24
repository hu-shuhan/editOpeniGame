#ifndef DATACODEC_CODEC_REFERENCE_COMMON_REFERENCEDELTA_H
#define DATACODEC_CODEC_REFERENCE_COMMON_REFERENCEDELTA_H

#include "DataCodec/Common/DataCodecTypes.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
namespace datacodec {

template<typename TValue>
inline bool BuildNumericArrayReferenceDeltas(
    const std::span<const std::uint8_t> currentBytes,
    const std::span<const std::uint8_t> predictorBytes,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    std::vector<TValue>& deltas) {
    deltas.clear();
    if (currentBytes.size() != predictorBytes.size() || componentCount == 0u) {
        return false;
    }
    const auto tupleBytes = componentCount * sizeof(TValue);
    const auto expectedBytes = static_cast<std::size_t>(elementCount) * tupleBytes;
    if (tupleBytes == 0u || currentBytes.size() != expectedBytes) {
        return false;
    }
    const auto* currentValues = reinterpret_cast<const TValue*>(currentBytes.data());
    const auto* predictorValues = reinterpret_cast<const TValue*>(predictorBytes.data());
    deltas.assign(static_cast<std::size_t>(elementCount) * componentCount, TValue{});
    for (std::size_t valueIndex = 0; valueIndex < deltas.size(); ++valueIndex) {
        deltas[valueIndex] = static_cast<TValue>(
            static_cast<double>(currentValues[valueIndex]) -
            static_cast<double>(predictorValues[valueIndex]));
    }
    return true;
}

template<typename TValue>
inline double NumericArrayReferenceDeltaSquaredError(const std::vector<TValue>& deltas) {
    double squaredError = 0.0;
    for (const auto value : deltas) {
        const auto deltaValue = static_cast<double>(value);
        squaredError += deltaValue * deltaValue;
    }
    return squaredError;
}

} // namespace datacodec

#endif
