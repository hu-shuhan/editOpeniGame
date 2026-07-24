#ifndef DATACODEC_VALIDATION_COMMON_FINITENUMERICVALIDATION_H
#define DATACODEC_VALIDATION_COMMON_FINITENUMERICVALIDATION_H

#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Validation/Result/ValidationResult.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace datacodec::validation {

template<typename TValue>
[[nodiscard]] ValidationResult ValidateFiniteNumericStore(
    const std::shared_ptr<bytestore::IRandomAccessByteStore>& store,
    const std::uint64_t valueCount,
    const ValidationDomain domain,
    std::string rule) {
    if (valueCount == 0u) {
        return ValidationResult::Success(domain, std::move(rule));
    }
    if (store == nullptr) {
        return ValidationResult::Failure(
            CodecErrorCode::DecodeFailure,
            domain,
            std::move(rule),
            "decoded numeric cache has no readable byte store");
    }
    std::uint64_t expectedBytes = 0u;
    if (!CheckedMulU64(
            valueCount,
            static_cast<std::uint64_t>(sizeof(TValue)),
            expectedBytes,
            "finite numeric validation byte count") ||
        expectedBytes > store->ByteSizeHint()) {
        return ValidationResult::Failure(
            CodecErrorCode::DecodeFailure,
            domain,
            std::move(rule),
            "decoded numeric cache is smaller than its declared value count");
    }
    constexpr std::size_t kChunkValueCount = 65536u;
    std::vector<TValue> values(kChunkValueCount);
    std::uint64_t valueOffset = 0u;
    while (valueOffset < valueCount) {
        const auto chunkCount = static_cast<std::size_t>(std::min<std::uint64_t>(
            valueCount - valueOffset,
            kChunkValueCount));
        const auto byteOffset = valueOffset * sizeof(TValue);
        const auto byteCount = chunkCount * sizeof(TValue);
        std::string readError;
        if (!store->Read(
                byteOffset,
                std::span<std::uint8_t>(
                    reinterpret_cast<std::uint8_t*>(values.data()),
                    byteCount),
                &readError)) {
            return ValidationResult::Failure(
                CodecErrorCode::DecodeFailure,
                domain,
                std::move(rule),
                readError.empty()
                    ? "failed to read decoded numeric cache for finite validation"
                    : std::move(readError));
        }
        for (std::size_t index = 0u; index < chunkCount; ++index) {
            if (!std::isfinite(static_cast<double>(values[index]))) {
                return ValidationResult::Failure(
                    CodecErrorCode::DecodeFailure,
                    domain,
                    std::move(rule),
                    "decoded numeric cache contains a non-finite value");
            }
        }
        valueOffset += chunkCount;
    }
    return ValidationResult::Success(domain, std::move(rule));
}

} // namespace datacodec::validation

#endif
