#ifndef DATACODEC_RUNTIME_CACHE_DECODEDCACHEREPLAY_H
#define DATACODEC_RUNTIME_CACHE_DECODEDCACHEREPLAY_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
namespace datacodec {

template<typename TValue, typename TWrite>
inline bool ReplayTypedDecodedCache(
    const CacheResources& runtime,
    bytestore::IByteSource& cache,
    const std::size_t elementCount,
    const std::size_t valuesPerElement,
    TWrite&& write,
    std::string* error = nullptr) {
    if (!cache.CanRead()) {
        return validation::AssignError(error, "final commit requires readable decoded cache");
    }
    const auto valuesPerWindow = runtime.ValuesPerWindow<TValue>(valuesPerElement);
    std::size_t totalValueCount = 0u;
    if (!validation::CheckedMulSizeT(
            elementCount,
            valuesPerElement,
            totalValueCount,
            "decoded cache replay value count",
            error)) {
        return false;
    }
    std::size_t totalBytes = 0u;
    if (!validation::CheckedMulSizeT(
            totalValueCount,
            sizeof(TValue),
            totalBytes,
            "decoded cache replay byte size",
            error)) {
        return false;
    }
    std::span<const std::uint8_t> contiguous;
    const auto contiguousStatus = cache.PrepareContiguousBytes(contiguous, error);
    if (contiguousStatus == ContiguousViewStatus::Error) {
        return false;
    }
    if (contiguousStatus == ContiguousViewStatus::Ready) {
        if (contiguous.size() != totalBytes) {
            return validation::AssignError(
                error,
                "decoded cache contiguous byte size does not match expected replay size");
        }
        std::size_t cursor = 0u;
        while (cursor < elementCount) {
            const auto currentCount = std::min(valuesPerWindow, elementCount - cursor);
            const auto byteOffset = cursor * valuesPerElement * sizeof(TValue);
            const auto* values = reinterpret_cast<const TValue*>(contiguous.data() + byteOffset);
            if (!write(cursor, currentCount, values)) {
                return false;
            }
            cursor += currentCount;
        }
        return true;
    }
    std::size_t cursor = 0u;
    while (cursor < elementCount) {
        const auto currentCount = std::min(valuesPerWindow, elementCount - cursor);
        const auto byteCount = currentCount * valuesPerElement * sizeof(TValue);
        auto windowLease = runtime.windowBudget.Acquire(byteCount);
        auto scratchBuffer = runtime.scratchBytePool.Acquire(byteCount);
        auto bytes = scratchBuffer.Span();
        if (!cache.Read(
                static_cast<std::uint64_t>(cursor) * valuesPerElement * sizeof(TValue),
                bytes,
                error)) {
            return false;
        }
        if (!write(cursor, currentCount, reinterpret_cast<const TValue*>(bytes.data()))) {
            return false;
        }
        cursor += currentCount;
    }
    return true;
}

} // namespace datacodec

#endif
