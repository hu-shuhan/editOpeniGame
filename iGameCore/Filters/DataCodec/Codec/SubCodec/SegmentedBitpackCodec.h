#ifndef DATACODEC_CODEC_SUBCODEC_SEGMENTEDBITPACKCODEC_H
#define DATACODEC_CODEC_SUBCODEC_SEGMENTEDBITPACKCODEC_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <span>
#include <string>
#include <vector>
namespace datacodec::codec {

inline std::uint8_t RequiredBitWidthForExclusiveUpperBound(const IndexType exclusiveUpperBound) {
    if (exclusiveUpperBound <= 1) {
        return 0;
    }

    std::uint8_t bitWidth = 0;
    auto maxValue = exclusiveUpperBound - 1;
    while (maxValue != 0) {
        ++bitWidth;
        maxValue >>= 1u;
    }
    return bitWidth;
}

inline bool EncodeSegmentedBitpack(
    const std::vector<IndexType>& values,
    const std::vector<IndexType>& segmentLengths,
    const std::vector<std::uint8_t>& segmentBitWidths,
    std::vector<std::uint8_t>& output,
    std::string* error = nullptr) {
    output.clear();
    if (segmentLengths.size() != segmentBitWidths.size()) {
        return validation::AssignError(error, "segment count and bit width count mismatch");
    }

    std::size_t expectedValueCount = 0;
    std::size_t totalBitCount = 0;
    for (std::size_t segmentIndex = 0; segmentIndex < segmentLengths.size(); ++segmentIndex) {
        const auto segmentLength = static_cast<std::size_t>(segmentLengths[segmentIndex]);
        std::size_t segmentBitCount = 0;
        if (!validation::CheckedMulSizeT(
                segmentLength,
                static_cast<std::size_t>(segmentBitWidths[segmentIndex]),
                segmentBitCount,
                "segmented bitpack bit count",
                error)) {
            return false;
        }
        if (!validation::CheckedAddSizeT(
                expectedValueCount,
                segmentLength,
                expectedValueCount,
                "segmented bitpack value count",
                error)) {
            return false;
        }
        if (!validation::CheckedAddSizeT(
                totalBitCount,
                segmentBitCount,
                totalBitCount,
                "segmented bitpack bit count",
                error)) {
            return false;
        }
    }
    if (expectedValueCount != values.size()) {
        return validation::AssignError(error, "segmented bitpack value count mismatch");
    }

    std::size_t totalByteCount = 0;
    if (!validation::CheckedAddSizeT(totalBitCount, 7u, totalByteCount, "segmented bitpack byte count", error)) {
        return false;
    }
    totalByteCount /= 8u;
    output.assign(totalByteCount, 0);
    std::size_t valueCursor = 0;
    std::size_t bitOffset = 0;

    for (std::size_t segmentIndex = 0; segmentIndex < segmentLengths.size(); ++segmentIndex) {
        const auto segmentLength = static_cast<std::size_t>(segmentLengths[segmentIndex]);
        const auto bitWidth = segmentBitWidths[segmentIndex];

        for (std::size_t localIndex = 0; localIndex < segmentLength; ++localIndex, ++valueCursor) {
            const auto value = values[valueCursor];
            if (bitWidth == 0) {
                if (value != 0) {
                    output.clear();
                    return validation::AssignError(error, "zero-width segment contains non-zero local index");
                }
                continue;
            }
            if (bitWidth < 32 && value >= static_cast<IndexType>(1u << bitWidth)) {
                output.clear();
                return validation::AssignError(error, "local index exceeds segment bit width capacity");
            }

            for (std::uint8_t bit = 0; bit < bitWidth; ++bit) {
                if (((value >> bit) & 1u) == 0) {
                    ++bitOffset;
                    continue;
                }
                const auto byteIndex = bitOffset / 8u;
                const auto bitIndex = bitOffset % 8u;
                output[byteIndex] |= static_cast<std::uint8_t>(1u << bitIndex);
                ++bitOffset;
            }
        }
    }

    return true;
}

inline bool DecodeSegmentedBitpack(
    const std::span<const std::uint8_t> bytes,
    const std::vector<IndexType>& segmentLengths,
    const std::vector<std::uint8_t>& segmentBitWidths,
    std::vector<IndexType>& output,
    std::string* error = nullptr) {
    output.clear();
    if (segmentLengths.size() != segmentBitWidths.size()) {
        return validation::AssignError(error, "segment count and bit width count mismatch");
    }

    std::size_t expectedValueCount = 0;
    std::size_t totalBitCount = 0;
    for (std::size_t segmentIndex = 0; segmentIndex < segmentLengths.size(); ++segmentIndex) {
        const auto segmentLength = static_cast<std::size_t>(segmentLengths[segmentIndex]);
        std::size_t segmentBitCount = 0;
        if (!validation::CheckedMulSizeT(
                segmentLength,
                static_cast<std::size_t>(segmentBitWidths[segmentIndex]),
                segmentBitCount,
                "segmented bitpack bit count",
                error)) {
            return false;
        }
        if (!validation::CheckedAddSizeT(
                expectedValueCount,
                segmentLength,
                expectedValueCount,
                "segmented bitpack value count",
                error)) {
            return false;
        }
        if (!validation::CheckedAddSizeT(
                totalBitCount,
                segmentBitCount,
                totalBitCount,
                "segmented bitpack bit count",
                error)) {
            return false;
        }
    }
    std::size_t totalByteCount = 0;
    if (!validation::CheckedAddSizeT(totalBitCount, 7u, totalByteCount, "segmented bitpack byte count", error)) {
        return false;
    }
    totalByteCount /= 8u;
    if (bytes.size() != totalByteCount) {
        return validation::AssignError(error, "bit stream length mismatch");
    }

    output.reserve(expectedValueCount);
    std::size_t bitOffset = 0;
    for (std::size_t segmentIndex = 0; segmentIndex < segmentLengths.size(); ++segmentIndex) {
        const auto segmentLength = static_cast<std::size_t>(segmentLengths[segmentIndex]);
        const auto bitWidth = segmentBitWidths[segmentIndex];
        for (std::size_t localIndex = 0; localIndex < segmentLength; ++localIndex) {
            IndexType value = 0;
            for (std::uint8_t bit = 0; bit < bitWidth; ++bit) {
                const auto byteIndex = bitOffset / 8u;
                const auto bitIndex = bitOffset % 8u;
                const auto bitValue = (bytes[byteIndex] >> bitIndex) & 1u;
                value |= static_cast<IndexType>(bitValue) << bit;
                ++bitOffset;
            }
            output.push_back(value);
        }
    }
    return true;
}

} // namespace datacodec::codec

#endif
