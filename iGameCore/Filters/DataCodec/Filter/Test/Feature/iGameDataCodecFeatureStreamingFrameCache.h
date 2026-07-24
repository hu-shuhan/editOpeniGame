#ifndef iGameDataCodecFeatureStreamingFrameCache_h
#define iGameDataCodecFeatureStreamingFrameCache_h

#include "DataCodec/Filter/Adapter/iGameFramePackageDecodeAssembly.h"
#include "DataCodec/Filter/Adapter/iGameStreamingFrameCacheAdapter.h"
#include "iGameDrawObject.h"
#include "iGameStreamingData.h"
#include "iGameStringArray.h"

#include <memory>
#include <unordered_map>

namespace datacodec::test {

class iGameStreamingCacheTestFrame final : public DecodedFrameLease {
public:
    iGameStreamingCacheTestFrame(
        const std::uint32_t frameIndex,
        iGame::DataObject::Pointer output)
        : m_frameIndex(frameIndex),
          m_payload(std::make_shared<iGame::iGameDecodedFramePayload>(std::move(output))) {}

    [[nodiscard]] std::uint32_t FrameIndex() const noexcept override {
        return m_frameIndex;
    }

    [[nodiscard]] IDecodedFramePayload::Pointer Payload() const noexcept override {
        return m_payload;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return m_payload != nullptr ? m_payload->ResidentSizeHint() : 0u;
    }

private:
    std::uint32_t m_frameIndex{0u};
    IDecodedFramePayload::Pointer m_payload;
};

[[nodiscard]] inline bool RuniGameDataCodecFeatureStreamingFrameCache() {
    auto streamingData = iGame::StreamingData::New();
    auto metadata0 = iGame::StringArray::New();
    auto metadata1 = iGame::StringArray::New();
    streamingData->AddTimeStep(0.0f, metadata0, StreamingType::IGCFramePackage);
    streamingData->AddTimeStep(1.0f, metadata1, StreamingType::IGCFramePackage);
    streamingData->EnableCache(0u);

    const DecodeSourceIdentity source0{.stableId = "streaming-source-0", .revision = "r1"};
    const DecodeSourceIdentity source1{.stableId = "streaming-source-1", .revision = "r1"};
    iGame::iGameStreamingFrameCacheAdapter cache(
        streamingData.GetPointer(),
        {{10u, 0u}, {20u, 1u}},
        {{10u, source0}, {20u, source1}});
    const DecodedFrameKey key0{.source = source0, .frameIndex = 10u};
    const DecodedFrameKey key1{.source = source1, .frameIndex = 20u};
    auto frame0 = std::make_shared<iGameStreamingCacheTestFrame>(
        10u,
        iGame::DrawObject::New());
    const auto stored0 = cache.Store(key0, frame0, DecodedFrameAccessKind::UserRequest);
    const auto found0 = cache.Find(key0, DecodedFrameAccessKind::UserRequest);
    if (!stored0.IsStored() || !found0.IsHit() || found0.value != frame0) {
        return false;
    }

    auto frame1 = std::make_shared<iGameStreamingCacheTestFrame>(
        20u,
        iGame::DrawObject::New());
    const auto stored1 = cache.Store(key1, frame1, DecodedFrameAccessKind::Prefetch);
    const auto evicted0 = cache.Find(key0, DecodedFrameAccessKind::UserRequest);
    const auto found1 = cache.Find(key1, DecodedFrameAccessKind::UserRequest);
    if (!stored1.IsStored() || !evicted0.IsMiss() || !found1.IsHit() || found1.value != frame1) {
        return false;
    }
    const auto stats = cache.Statistics();
    if (stats.hits < 2u || stats.misses == 0u || stats.evictions == 0u ||
        stats.residentFrames != 1u) {
        return false;
    }

    const auto retainedPayload = frame1->Payload();
    cache.InvalidateSource(source1);
    if (!cache.ResidentFrameIndices(source1).empty() ||
        cache.Statistics().residentFrames != 0u ||
        frame1->Payload() != retainedPayload) {
        return false;
    }
    const DecodedFrameKey wrongRevision{
        .source = DecodeSourceIdentity{
            .stableId = source0.stableId,
            .revision = "r2",
        },
        .frameIndex = 10u,
    };
    return cache.Find(wrongRevision, DecodedFrameAccessKind::UserRequest).IsError();
}

} // namespace datacodec::test

#endif
