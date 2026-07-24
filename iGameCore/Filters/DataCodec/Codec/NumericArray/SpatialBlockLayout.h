#ifndef DATACODEC_CODEC_NUMERICARRAY_SPATIALBLOCKLAYOUT_H
#define DATACODEC_CODEC_NUMERICARRAY_SPATIALBLOCKLAYOUT_H

#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
namespace datacodec::numericarray {

struct SpatialBlockRange {
    std::uint32_t blockIndex{0u};
    std::uint32_t elementOffset{0u};
    std::uint32_t elementCount{0u};
};

inline bool BuildSpatialBlockLayout(
    const std::size_t elementCount,
    const std::uint32_t spatialBlockElementCount,
    std::vector<SpatialBlockRange>& layout,
    std::string* error = nullptr) {
    layout.clear();
    if (spatialBlockElementCount == 0u) {
        return validation::AssignError(error, "spatial block element count must be positive");
    }
    if (elementCount > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return validation::AssignError(error, "spatial block layout exceeds uint32 element range");
    }
    std::size_t elementOffset = 0u;
    std::uint32_t blockIndex = 0u;
    while (elementOffset < elementCount) {
        const auto localElementCount = std::min<std::size_t>(
            static_cast<std::size_t>(spatialBlockElementCount),
            elementCount - elementOffset);
        layout.push_back(SpatialBlockRange{
            .blockIndex = blockIndex,
            .elementOffset = static_cast<std::uint32_t>(elementOffset),
            .elementCount = static_cast<std::uint32_t>(localElementCount),
        });
        elementOffset += localElementCount;
        ++blockIndex;
    }
    return true;
}

} // namespace datacodec::numericarray

#endif
