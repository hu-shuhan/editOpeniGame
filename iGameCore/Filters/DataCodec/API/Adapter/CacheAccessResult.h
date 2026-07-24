#ifndef DATACODEC_API_ADAPTER_CACHEACCESSRESULT_H
#define DATACODEC_API_ADAPTER_CACHEACCESSRESULT_H

#include <cstdint>
#include <string>
#include <utility>

namespace datacodec {

enum class CacheLookupStatus : std::uint8_t {
    Hit = 0u,
    Miss = 1u,
    Error = 2u,
};

template<typename TValue>
struct CacheLookupResult {
    CacheLookupStatus status{CacheLookupStatus::Miss};
    TValue value{};
    std::string error;

    [[nodiscard]] bool IsHit() const noexcept { return status == CacheLookupStatus::Hit; }
    [[nodiscard]] bool IsMiss() const noexcept { return status == CacheLookupStatus::Miss; }
    [[nodiscard]] bool IsError() const noexcept { return status == CacheLookupStatus::Error; }

    [[nodiscard]] static CacheLookupResult Hit(TValue value) {
        return {
            .status = CacheLookupStatus::Hit,
            .value = std::move(value),
        };
    }

    [[nodiscard]] static CacheLookupResult Miss() {
        return {.status = CacheLookupStatus::Miss};
    }

    [[nodiscard]] static CacheLookupResult Error(std::string error) {
        return {
            .status = CacheLookupStatus::Error,
            .error = std::move(error),
        };
    }
};

enum class CacheStoreStatus : std::uint8_t {
    Stored = 0u,
    RejectedByPolicy = 1u,
    Error = 2u,
};

struct CacheStoreResult {
    CacheStoreStatus status{CacheStoreStatus::Error};
    std::string error;

    [[nodiscard]] bool IsStored() const noexcept { return status == CacheStoreStatus::Stored; }
    [[nodiscard]] bool IsRejectedByPolicy() const noexcept {
        return status == CacheStoreStatus::RejectedByPolicy;
    }
    [[nodiscard]] bool IsError() const noexcept { return status == CacheStoreStatus::Error; }

    [[nodiscard]] static CacheStoreResult Stored() {
        return {.status = CacheStoreStatus::Stored};
    }

    [[nodiscard]] static CacheStoreResult RejectedByPolicy() {
        return {.status = CacheStoreStatus::RejectedByPolicy};
    }

    [[nodiscard]] static CacheStoreResult Error(std::string error) {
        return {
            .status = CacheStoreStatus::Error,
            .error = std::move(error),
        };
    }
};

} // namespace datacodec

#endif
