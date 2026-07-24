#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREENCODEDINPUTCACHE_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREENCODEDINPUTCACHE_H

#include "DataCodec/Runtime/Cache/EncodedInputCacheLoader.h"
#include "DataCodec/Runtime/Cache/EncodedInputLruCache.h"

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace datacodec::test::feature_encoded_input_cache {

class CountingInputReader final : public IByteRangeReader {
public:
    explicit CountingInputReader(std::vector<std::uint8_t> bytes) : m_bytes(std::move(bytes)) {}

    [[nodiscard]] std::uint64_t ByteSize() const noexcept override {
        return static_cast<std::uint64_t>(m_bytes.size());
    }

    bool ReadAt(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) override {
        if (offset > m_bytes.size() || output.size() > m_bytes.size() - static_cast<std::size_t>(offset)) {
            return validation::AssignError(error, "counting input reader range is outside the buffer");
        }
        ++m_readCount;
        if (!output.empty()) {
            std::memcpy(output.data(), m_bytes.data() + static_cast<std::size_t>(offset), output.size());
        }
        return true;
    }

    [[nodiscard]] std::size_t ReadCount() const noexcept { return m_readCount; }

private:
    std::vector<std::uint8_t> m_bytes;
    std::size_t m_readCount{0u};
};

class FailingEncodedInputCache final : public IEncodedInputCache {
public:
    explicit FailingEncodedInputCache(const bool failLookup)
        : m_failLookup(failLookup) {}

    [[nodiscard]] EncodedInputCacheLookupResult Find(
        const DecodeSourceIdentity&,
        EncodedInputAccessKind) override {
        return m_failLookup
            ? EncodedInputCacheLookupResult::Error("injected encoded cache lookup failure")
            : EncodedInputCacheLookupResult::Miss();
    }

    [[nodiscard]] CacheStoreResult Store(
        const DecodeSourceIdentity&,
        EncodedInputBuffer,
        EncodedInputAccessKind) override {
        return CacheStoreResult::Error("injected encoded cache store failure");
    }

    void InvalidateSource(const DecodeSourceIdentity&) override {}

    [[nodiscard]] EncodedInputCacheStats Statistics() const override {
        return {};
    }

private:
    bool m_failLookup{false};
};

[[nodiscard]] inline DecodeSourceIdentity Source(const std::string& revision) {
    return DecodeSourceIdentity{.stableId = "synthetic-encoded-input", .revision = revision};
}

[[nodiscard]] inline EncodedInputBuffer Input(std::initializer_list<std::uint8_t> bytes) {
    return std::make_shared<const std::vector<std::uint8_t>>(bytes);
}

inline bool TestLeastRecentlyUsedInputIsEvicted() {
    EncodedInputLruCache cache;
    cache.Configure(2u, 0u);
    (void)cache.Store(Source("a"), Input({1u}), EncodedInputAccessKind::UserRequest);
    (void)cache.Store(Source("b"), Input({2u}), EncodedInputAccessKind::Prefetch);
    {
        const auto touched = cache.Find(Source("a"), EncodedInputAccessKind::UserRequest);
        if (!touched.IsHit()) { return false; }
    }
    (void)cache.Store(Source("c"), Input({3u}), EncodedInputAccessKind::UserRequest);
    return cache.Find(Source("a"), EncodedInputAccessKind::UserRequest).IsHit() &&
        cache.Find(Source("b"), EncodedInputAccessKind::UserRequest).IsMiss() &&
        cache.Find(Source("c"), EncodedInputAccessKind::UserRequest).IsHit();
}

inline bool TestMemoryInputKeepsExistingByteOwner() {
    auto bytes = std::make_shared<const std::vector<std::uint8_t>>(
        std::initializer_list<std::uint8_t>{4u, 5u, 6u});
    auto reader = std::make_shared<MemoryByteRangeReader>(bytes);
    auto cache = std::make_shared<EncodedInputLruCache>();
    cache->Configure(2u, 0u);
    EncodedInputCacheLoader loader;
    std::string error;
    const auto retained = loader.Load(
        cache,
        Source("shared"),
        reader,
        EncodedInputAccessKind::UserRequest,
        &error);
    const auto lookup = cache->Find(Source("shared"), EncodedInputAccessKind::UserRequest);
    return error.empty() && retained != nullptr && retained.get() == bytes.get() &&
        lookup.IsHit() && lookup.value.get() == bytes.get();
}

inline bool TestRepeatedStreamInputLoadsOnlyOnce() {
    auto reader = std::make_shared<CountingInputReader>(std::vector<std::uint8_t>{7u, 8u, 9u, 10u});
    auto cache = std::make_shared<EncodedInputLruCache>();
    cache->Configure(2u, 0u);
    EncodedInputCacheLoader loader;
    std::string error;
    const auto first = loader.Load(
        cache,
        Source("stream"),
        reader,
        EncodedInputAccessKind::UserRequest,
        &error);
    const auto second = loader.Load(
        cache,
        Source("stream"),
        reader,
        EncodedInputAccessKind::UserRequest,
        &error);
    return error.empty() && first != nullptr && first == second && reader->ReadCount() == 1u;
}

inline bool TestInvalidCacheAccessIsReportedAsError() {
    EncodedInputLruCache cache;
    cache.Configure(2u, 0u);
    const auto lookup = cache.Find({}, EncodedInputAccessKind::UserRequest);
    const auto store = cache.Store(
        Source("invalid"),
        {},
        EncodedInputAccessKind::UserRequest);
    const auto stats = cache.Statistics();
    return lookup.IsError() && !lookup.error.empty() &&
        store.IsError() && !store.error.empty() &&
        stats.lookupErrors == 1u && stats.storeErrors == 1u;
}

inline bool TestBackendErrorsDoNotLoadThroughAnotherPath() {
    EncodedInputCacheLoader loader;
    auto lookupReader = std::make_shared<CountingInputReader>(
        std::vector<std::uint8_t>{1u, 2u, 3u});
    auto lookupCache = std::make_shared<FailingEncodedInputCache>(true);
    std::string lookupError;
    const auto lookupResult = loader.Load(
        lookupCache,
        Source("lookup-error"),
        lookupReader,
        EncodedInputAccessKind::UserRequest,
        &lookupError);
    if (lookupResult != nullptr || lookupReader->ReadCount() != 0u ||
        lookupError != "injected encoded cache lookup failure") {
        return false;
    }

    auto storeReader = std::make_shared<CountingInputReader>(
        std::vector<std::uint8_t>{4u, 5u, 6u});
    auto storeCache = std::make_shared<FailingEncodedInputCache>(false);
    std::string storeError;
    const auto storeResult = loader.Load(
        storeCache,
        Source("store-error"),
        storeReader,
        EncodedInputAccessKind::UserRequest,
        &storeError);
    return storeResult == nullptr && storeReader->ReadCount() == 1u &&
        storeError == "injected encoded cache store failure";
}

} // namespace datacodec::test::feature_encoded_input_cache

namespace datacodec::test {

inline int RunDataCodecFeatureEncodedInputCache() {
    using namespace feature_encoded_input_cache;
    if (!TestLeastRecentlyUsedInputIsEvicted() ||
        !TestMemoryInputKeepsExistingByteOwner() ||
        !TestRepeatedStreamInputLoadsOnlyOnce() ||
        !TestInvalidCacheAccessIsReportedAsError() ||
        !TestBackendErrorsDoNotLoadThroughAnotherPath()) {
        std::cerr << "DataCodec encoded input cache feature test failed\n";
        return 1;
    }
    std::cout << "DataCodec encoded input cache feature test passed\n";
    return 0;
}

} // namespace datacodec::test

#endif
