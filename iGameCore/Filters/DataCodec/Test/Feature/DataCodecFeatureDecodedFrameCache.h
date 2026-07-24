#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREDECODEDFRAMECACHE_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREDECODEDFRAMECACHE_H

#include "DataCodec/Workflow/Session/PlaybackPrefetchPlanner.h"
#include "DataCodec/Runtime/Cache/DecodedFrameLruCache.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace datacodec::test::feature_decoded_frame_cache
{

class TestPayload final : public IDecodedFramePayload {
public:
    explicit TestPayload(const std::uint64_t residentBytes) : m_residentBytes(residentBytes) {}
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override { return m_residentBytes; }

private:
    std::uint64_t m_residentBytes{0u};
};

class TestFrame final : public DecodedFrameLease {
public:
    TestFrame(const std::uint32_t frameIndex, const std::uint64_t residentBytes)
        : m_frameIndex(frameIndex),
          m_payload(std::make_shared<TestPayload>(residentBytes)) {}

    [[nodiscard]] std::uint32_t FrameIndex() const noexcept override { return m_frameIndex; }
    [[nodiscard]] IDecodedFramePayload::Pointer Payload() const noexcept override { return m_payload; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return m_payload->ResidentSizeHint();
    }

private:
    std::uint32_t m_frameIndex{0u};
    IDecodedFramePayload::Pointer m_payload;
};

inline DecodeSourceIdentity Source(const std::string& revision = "r1") {
    return DecodeSourceIdentity{.stableId = "synthetic-series", .revision = revision};
}

inline DecodedFrameKey Key(const std::uint32_t frameIndex, const std::string& revision = "r1") {
    return DecodedFrameKey{.source = Source(revision), .frameIndex = frameIndex};
}

inline bool TestLeastRecentlyUsedFrameIsEvicted() {
    DecodedFrameLruCache cache;
    cache.Configure(2u, 0u);
    (void)cache.Store(Key(0u), std::make_shared<TestFrame>(0u, 8u), DecodedFrameAccessKind::UserRequest);
    (void)cache.Store(Key(1u), std::make_shared<TestFrame>(1u, 8u), DecodedFrameAccessKind::Prefetch);
    {
        const auto touched = cache.Find(Key(0u), DecodedFrameAccessKind::UserRequest);
        if (!touched.IsHit()) { return false; }
    }
    (void)cache.Store(Key(2u), std::make_shared<TestFrame>(2u, 8u), DecodedFrameAccessKind::UserRequest);
    return cache.Find(Key(0u), DecodedFrameAccessKind::UserRequest).IsHit() &&
        cache.Find(Key(1u), DecodedFrameAccessKind::UserRequest).IsMiss() &&
        cache.Find(Key(2u), DecodedFrameAccessKind::UserRequest).IsHit();
}

inline bool TestResidentByteLimitUsesSameLRUOrder() {
    DecodedFrameLruCache cache;
    cache.Configure(0u, 16u);
    (void)cache.Store(Key(0u), std::make_shared<TestFrame>(0u, 8u), DecodedFrameAccessKind::UserRequest);
    (void)cache.Store(Key(1u), std::make_shared<TestFrame>(1u, 8u), DecodedFrameAccessKind::UserRequest);
    (void)cache.Store(Key(2u), std::make_shared<TestFrame>(2u, 8u), DecodedFrameAccessKind::UserRequest);
    const auto stats = cache.Statistics();
    return cache.Find(Key(0u), DecodedFrameAccessKind::UserRequest).IsMiss() &&
        stats.residentFrames == 2u && stats.residentBytes == 16u;
}

inline bool TestSourceRevisionSeparatesEntries() {
    DecodedFrameLruCache cache;
    cache.Configure(4u, 0u);
    (void)cache.Store(Key(0u, "r1"), std::make_shared<TestFrame>(0u, 4u), DecodedFrameAccessKind::UserRequest);
    return cache.Find(Key(0u, "r1"), DecodedFrameAccessKind::UserRequest).IsHit() &&
        cache.Find(Key(0u, "r2"), DecodedFrameAccessKind::UserRequest).IsMiss();
}

inline bool TestDisableClearsAndBlocksFrameCaching() {
    DecodedFrameLruCache cache;
    cache.Configure(4u, 0u);
    (void)cache.Store(Key(0u), std::make_shared<TestFrame>(0u, 8u), DecodedFrameAccessKind::UserRequest);
    cache.SetEnabled(false);
    const auto disabledStats = cache.Statistics();
    if (cache.IsEnabled() || disabledStats.residentFrames != 0u ||
        !cache.Find(Key(0u), DecodedFrameAccessKind::UserRequest).IsMiss() ||
        !cache.Store(
            Key(1u),
            std::make_shared<TestFrame>(1u, 8u),
            DecodedFrameAccessKind::UserRequest).IsRejectedByPolicy()) {
        return false;
    }
    cache.SetEnabled(true);
    return cache.IsEnabled() &&
        cache.Store(
            Key(1u),
            std::make_shared<TestFrame>(1u, 8u),
            DecodedFrameAccessKind::UserRequest).IsStored() &&
        cache.Find(Key(1u), DecodedFrameAccessKind::UserRequest).IsHit();
}

inline bool TestDirectionalPrefetchIsIndependentFromEviction() {
    PlaybackPrefetchPlanner planner;
    planner.Configure({10u, 20u, 30u, 40u, 50u}, 2u);
    return planner.Plan(30u, PlaybackDirection::Forward) == std::vector<std::uint32_t>({40u, 50u}) &&
        planner.Plan(30u, PlaybackDirection::Backward) == std::vector<std::uint32_t>({20u, 10u}) &&
        planner.Plan(10u, PlaybackDirection::Random) == std::vector<std::uint32_t>({20u, 30u});
}

inline bool TestInvalidCacheAccessIsReportedAsError() {
    DecodedFrameLruCache cache;
    cache.Configure(2u, 0u);
    const DecodedFrameKey unstableKey{.frameIndex = 0u};
    const auto lookup = cache.Find(
        unstableKey,
        DecodedFrameAccessKind::UserRequest);
    const auto store = cache.Store(
        Key(0u),
        std::make_shared<TestFrame>(1u, 8u),
        DecodedFrameAccessKind::UserRequest);
    const auto stats = cache.Statistics();
    return lookup.IsError() && !lookup.error.empty() &&
        store.IsError() && !store.error.empty() &&
        stats.lookupErrors == 1u && stats.storeErrors == 1u;
}

} // namespace datacodec::test::feature_decoded_frame_cache

namespace datacodec::test
{

inline int RunDataCodecFeatureDecodedFrameCache() {
    using namespace feature_decoded_frame_cache;
    if (!TestLeastRecentlyUsedFrameIsEvicted() ||
        !TestResidentByteLimitUsesSameLRUOrder() ||
        !TestSourceRevisionSeparatesEntries() ||
        !TestDisableClearsAndBlocksFrameCaching() ||
        !TestDirectionalPrefetchIsIndependentFromEviction() ||
        !TestInvalidCacheAccessIsReportedAsError()) {
        std::cerr << "DataCodec decoded frame cache feature test failed\n";
        return 1;
    }
    std::cout << "DataCodec decoded frame cache feature test passed\n";
    return 0;
}

} // namespace datacodec::test

#endif
