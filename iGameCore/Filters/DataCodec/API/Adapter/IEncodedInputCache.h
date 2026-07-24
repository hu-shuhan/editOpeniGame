#ifndef DATACODEC_API_ADAPTER_IENCODEDINPUTCACHE_H
#define DATACODEC_API_ADAPTER_IENCODEDINPUTCACHE_H

#include "DataCodec/API/Adapter/CacheAccessResult.h"
#include "DataCodec/Runtime/Cache/DecodeCacheIdentity.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace datacodec {

using EncodedInputBuffer = std::shared_ptr<const std::vector<std::uint8_t>>;
using EncodedInputCacheLookupResult = CacheLookupResult<EncodedInputBuffer>;

enum class EncodedInputAccessKind : std::uint8_t {
    UserRequest = 0u,
    Prefetch = 1u,
};

struct EncodedInputCacheStats {
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
    std::size_t residentInputs{0u};
};

// 调用方可用该接口替换DataCodec默认编码输入LRU
// 缓存保存共享字节所有权，不复制已由调用方拥有的内存输入
class IEncodedInputCache {
public:
    virtual ~IEncodedInputCache() = default;

    [[nodiscard]] virtual EncodedInputCacheLookupResult Find(
        const DecodeSourceIdentity& source,
        EncodedInputAccessKind accessKind) = 0;
    [[nodiscard]] virtual CacheStoreResult Store(
        const DecodeSourceIdentity& source,
        EncodedInputBuffer input,
        EncodedInputAccessKind accessKind) = 0;
    virtual void InvalidateSource(const DecodeSourceIdentity& source) = 0;
    [[nodiscard]] virtual EncodedInputCacheStats Statistics() const = 0;
};

} // namespace datacodec

#endif
