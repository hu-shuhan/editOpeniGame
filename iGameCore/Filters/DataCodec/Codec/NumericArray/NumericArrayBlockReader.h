#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKREADER_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKREADER_H

#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockDecode.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>
namespace datacodec {
namespace numericarray {

struct NumericArrayBlockPayload {
    NumericArrayBlockHeader header;
    CompressorConfig backgroundCompressor;
    ParamSize backgroundEncodedByteLength{0};
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;
    std::vector<NumericArrayRegionLayerLayoutParams> regionLayers;
    std::vector<double> alpha;
    std::vector<double> beta;
    ScratchByteBuffer bytes;
};

template<typename TStream>
inline bool ReadNumericArrayBlockPayload(
    TStream& stream,
    const std::size_t componentCount,
    const NumericArrayBlockLayoutParams& layout,
    const CacheResources& runtime,
    NumericArrayBlockPayload& block,
    std::string* error = nullptr) {
    block = {};
    if (!MakeNumericArrayBlockHeader(layout, block.header, error)) {
        return false;
    }
    block.backgroundCompressor = layout.backgroundCompressor;
    block.backgroundEncodedByteLength = layout.backgroundEncodedByteLength;
    block.componentLayouts = layout.componentLayouts;
    block.regionLayers = layout.regionLayers;

    if (block.header.mode == NumericArrayBlockMode::AffineReference) {
        if (layout.alpha.size() != componentCount || layout.beta.size() != componentCount) {
            return validation::AssignError(error, "numeric array affine block layout component count mismatch");
        }
        block.alpha = layout.alpha;
        block.beta = layout.beta;
    }

    block.bytes = runtime.scratchBytePool.Acquire(block.header.encodedByteLength);
    return stream.ReadBytes(block.bytes.Bytes().data(), block.bytes.Bytes().size(), error);
}

inline bool ResolveDecodedNumericArrayBlockBytes(
    const numericarray::NumericArrayBlockParams& params,
    const NumericArrayBlockPayload& block,
    std::vector<std::uint8_t>& decodedBytes,
    std::string* error = nullptr) {
    decodedBytes.clear();
    if (!numericarray::ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }

    if (block.header.mode == NumericArrayBlockMode::LayeredResidual) {
        return numericarray::ResolveDecodedLayeredResidualNumericArrayBlockBytes(
            params,
            block.header.elementCount,
            block.backgroundCompressor,
            block.backgroundEncodedByteLength,
            block.componentLayouts,
            block.regionLayers,
            block.bytes.Span(),
            decodedBytes,
            error);
    }
    return numericarray::ResolveDecodedNumericArrayBlockBytes(
        params,
        block.backgroundCompressor,
        block.header.elementCount,
        block.header.bytesCodec,
        block.componentLayouts,
        block.bytes.Span(),
        decodedBytes,
        error);
}

using DecodedNumericArrayBlockConsumer = std::function<bool(
    const NumericArrayBlockHeader& header,
    std::span<const std::uint8_t> decodedBytes,
    std::string* error)>;

template<typename TStream>
inline bool DecodeNumericArrayBlocks(
    TStream& stream,
    const numericarray::NumericArrayBlockParams& params,
    const std::vector<NumericArrayBlockLayoutParams>& blockLayouts,
    const CacheResources& runtime,
    const DecodedNumericArrayBlockConsumer& consumer,
    std::string* error = nullptr) {
    const auto componentCount = params.componentCount;
    std::uint64_t consumedElements = 0u;
    const auto expectedElements = static_cast<std::uint64_t>(params.elementCount);
    if (expectedElements != 0u && blockLayouts.empty()) {
        return validation::AssignError(error, "numeric array params are missing block layout metadata");
    }
    for (const auto& layout : blockLayouts) {
        NumericArrayBlockPayload block;
        if (!ReadNumericArrayBlockPayload(stream, componentCount, layout, runtime, block, error)) {
            return false;
        }
        if ((block.header.mode != NumericArrayBlockMode::NonReference &&
             block.header.mode != NumericArrayBlockMode::LayeredResidual) ||
            block.header.referenceKind != NumericArrayReferenceKind::None ||
            block.header.codecId != NumericArrayReferenceCodecId::NonReference) {
            return validation::AssignError(error, "numeric array decoder only accepts non-reference blocks");
        }
        if (block.header.elementCount == 0u ||
            block.header.elementOffset != consumedElements ||
            block.header.elementCount > expectedElements - consumedElements) {
            return validation::AssignError(error, "numeric array block range is not contiguous");
        }

        std::vector<std::uint8_t> decodedBytes;
        if (!ResolveDecodedNumericArrayBlockBytes(params, block, decodedBytes, error)) {
            return false;
        }
        if (!consumer(
                block.header,
                std::span<const std::uint8_t>(decodedBytes.data(), decodedBytes.size()),
                error)) {
            return false;
        }
        if (!validation::CheckedAddU64(
                consumedElements,
                block.header.elementCount,
                consumedElements,
                "numeric array consumed block elements",
                error)) {
            return false;
        }
    }
    if (consumedElements != expectedElements) {
        return validation::AssignError(error, "numeric array block layouts do not cover the full field");
    }
    return true;
}

} // namespace numericarray
} // namespace datacodec

#endif
