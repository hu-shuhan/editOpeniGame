#ifndef DATACODEC_API_ADAPTER_IDECODEDFRAMECACHE_H
#define DATACODEC_API_ADAPTER_IDECODEDFRAMECACHE_H

#include "DataCodec/API/Adapter/CacheAccessResult.h"
#include "DataCodec/Runtime/Cache/DecodeCacheIdentity.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace datacodec {

class IDecodedFramePayload {
public:
    using Pointer = std::shared_ptr<IDecodedFramePayload>;

    virtual ~IDecodedFramePayload() = default;
    [[nodiscard]] virtual std::uint64_t ResidentSizeHint() const noexcept = 0;
};

class DecodedFrameLease {
public:
    using Pointer = std::shared_ptr<DecodedFrameLease>;

    virtual ~DecodedFrameLease() = default;
    [[nodiscard]] virtual std::uint32_t FrameIndex() const noexcept = 0;
    [[nodiscard]] virtual IDecodedFramePayload::Pointer Payload() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t ResidentSizeHint() const noexcept = 0;
};

using DecodedFrameCacheLookupResult = CacheLookupResult<DecodedFrameLease::Pointer>;

enum class DecodedFrameAccessKind : std::uint8_t {
    UserRequest = 0u,
    Prefetch = 1u,
};

struct DecodedFrameCacheStats {
    std::uint64_t lookups{0u};
    std::uint64_t hits{0u};
    std::uint64_t misses{0u};
    std::uint64_t lookupErrors{0u};
    std::uint64_t stores{0u};
    std::uint64_t storeRejections{0u};
    std::uint64_t storeErrors{0u};
    std::uint64_t evictions{0u};
    std::uint64_t residentBytes{0u};
    std::uint64_t peakResidentBytes{0u};
    std::size_t residentFrames{0u};
};

// 完整解码帧缓存由调用方提供时，容量和淘汰由调用方负责
// DataCodec只通过该接口查询和发布完整帧
class IDecodedFrameCache {
public:
    virtual ~IDecodedFrameCache() = default;

    [[nodiscard]] virtual DecodedFrameCacheLookupResult Find(
        const DecodedFrameKey& key,
        DecodedFrameAccessKind accessKind) = 0;
    [[nodiscard]] virtual CacheStoreResult Store(
        const DecodedFrameKey& key,
        DecodedFrameLease::Pointer frame,
        DecodedFrameAccessKind accessKind) = 0;
    virtual void InvalidateSource(const DecodeSourceIdentity& source) = 0;
    [[nodiscard]] virtual std::vector<std::uint32_t> ResidentFrameIndices(
        const DecodeSourceIdentity& source) const = 0;
    [[nodiscard]] virtual DecodedFrameCacheStats Statistics() const = 0;
};

} // namespace datacodec

#endif
