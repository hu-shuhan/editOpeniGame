#include "DataCodec/Filter/Adapter/iGameStreamingFrameCacheAdapter.h"

#include "DataCodec/Filter/Adapter/iGameFramePackageDecodeAssembly.h"

#include <algorithm>
#include <memory>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {

class iGameDecodedFrameCacheResource final : public IStreamingFrameCacheResource {
public:
    explicit iGameDecodedFrameCacheResource(::datacodec::DecodedFrameLease::Pointer frame)
        : m_frame(std::move(frame)) {}

    [[nodiscard]] ::datacodec::DecodedFrameLease::Pointer Frame() const noexcept {
        return m_frame;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        return m_frame != nullptr ? m_frame->ResidentSizeHint() : 0u;
    }

private:
    ::datacodec::DecodedFrameLease::Pointer m_frame;
};

} // 匿名命名空间

iGameStreamingFrameCacheAdapter::iGameStreamingFrameCacheAdapter(
    StreamingData* streamingData,
    std::unordered_map<std::uint32_t, unsigned int> frameOrdinals,
    std::unordered_map<std::uint32_t, ::datacodec::DecodeSourceIdentity> frameIdentities)
    : m_streamingData(streamingData),
      m_frameOrdinals(std::move(frameOrdinals)),
      m_frameIdentities(std::move(frameIdentities)) {
    for (const auto& [frameIndex, ordinal] : m_frameOrdinals) {
        m_ordinalFrames.emplace(ordinal, frameIndex);
    }
}

::datacodec::DecodedFrameCacheLookupResult iGameStreamingFrameCacheAdapter::Find(
    const ::datacodec::DecodedFrameKey& key,
    const ::datacodec::DecodedFrameAccessKind accessKind) {
    (void)accessKind;
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_stats.lookups;
    }
    if (!Matches(key)) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_stats.lookupErrors;
        return ::datacodec::DecodedFrameCacheLookupResult::Error(
            "streaming frame cache key does not belong to this sequence");
    }
    if (m_streamingData == nullptr) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_stats.lookupErrors;
        return ::datacodec::DecodedFrameCacheLookupResult::Error(
            "streaming frame cache backend is unavailable");
    }
    StreamingFrameCacheEntry entry;
    const auto ordinal = m_frameOrdinals.at(key.frameIndex);
    if (!m_streamingData->FindCachedFrame(ordinal, entry)) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_stats.misses;
        RefreshResidentStatsLocked();
        return ::datacodec::DecodedFrameCacheLookupResult::Miss();
    }
    const auto resource = std::dynamic_pointer_cast<iGameDecodedFrameCacheResource>(entry.resource);
    std::lock_guard<std::mutex> lock(m_statsMutex);
    RefreshResidentStatsLocked();
    if (resource == nullptr || resource->Frame() == nullptr ||
        resource->Frame()->FrameIndex() != key.frameIndex ||
        resource->Frame()->Payload() == nullptr) {
        ++m_stats.lookupErrors;
        return ::datacodec::DecodedFrameCacheLookupResult::Error(
            "streaming frame cache entry is invalid");
    }
    ++m_stats.hits;
    return ::datacodec::DecodedFrameCacheLookupResult::Hit(resource->Frame());
}

::datacodec::CacheStoreResult iGameStreamingFrameCacheAdapter::Store(
    const ::datacodec::DecodedFrameKey& key,
    ::datacodec::DecodedFrameLease::Pointer frame,
    const ::datacodec::DecodedFrameAccessKind accessKind) {
    if (!Matches(key) || m_streamingData == nullptr || frame == nullptr ||
        frame->FrameIndex() != key.frameIndex || frame->Payload() == nullptr) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_stats.storeErrors;
        return ::datacodec::CacheStoreResult::Error(
            "streaming frame cache store input is invalid");
    }
    const auto output = DataObjectFromDecodedFrame(frame);
    if (output == nullptr) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_stats.storeErrors;
        return ::datacodec::CacheStoreResult::Error(
            "streaming frame cache lease does not contain a data object");
    }
    const auto ordinal = m_frameOrdinals.at(key.frameIndex);
    const auto residentBefore = m_streamingData->CachedFrameIndices();
    const bool replacing = std::find(
        residentBefore.begin(),
        residentBefore.end(),
        ordinal) != residentBefore.end();
    const auto stored = m_streamingData->StoreCachedFrame(
        ordinal,
        StreamingFrameCacheEntry{
            .data = {output},
            .resource = std::make_shared<iGameDecodedFrameCacheResource>(std::move(frame)),
        },
        accessKind == ::datacodec::DecodedFrameAccessKind::UserRequest);
    if (!stored) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_stats.storeErrors;
        return ::datacodec::CacheStoreResult::Error(
            "streaming frame cache backend rejected a validated frame");
    }

    const auto residentAfter = m_streamingData->CachedFrameIndices();
    std::lock_guard<std::mutex> lock(m_statsMutex);
    ++m_stats.stores;
    if (!replacing && residentAfter.size() <= residentBefore.size()) {
        ++m_stats.evictions;
    }
    RefreshResidentStatsLocked();
    return ::datacodec::CacheStoreResult::Stored();
}

void iGameStreamingFrameCacheAdapter::InvalidateSource(
    const ::datacodec::DecodeSourceIdentity& source) {
    if (m_streamingData == nullptr) { return; }
    for (const auto& [frameIndex, identity] : m_frameIdentities) {
        if (identity.stableId != source.stableId) { continue; }
        const auto ordinal = m_frameOrdinals.find(frameIndex);
        if (ordinal != m_frameOrdinals.end()) {
            (void)m_streamingData->EraseCachedFrame(ordinal->second);
        }
    }
    std::lock_guard<std::mutex> lock(m_statsMutex);
    RefreshResidentStatsLocked();
}

std::vector<std::uint32_t> iGameStreamingFrameCacheAdapter::ResidentFrameIndices(
    const ::datacodec::DecodeSourceIdentity& source) const {
    std::vector<std::uint32_t> frames;
    if (m_streamingData == nullptr) { return frames; }
    for (const auto ordinal : m_streamingData->CachedFrameIndices()) {
        const auto frame = m_ordinalFrames.find(ordinal);
        if (frame == m_ordinalFrames.end()) { continue; }
        const auto identity = m_frameIdentities.find(frame->second);
        if (identity != m_frameIdentities.end() && identity->second.stableId == source.stableId) {
            frames.push_back(frame->second);
        }
    }
    std::sort(frames.begin(), frames.end());
    return frames;
}

::datacodec::DecodedFrameCacheStats iGameStreamingFrameCacheAdapter::Statistics() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    RefreshResidentStatsLocked();
    return m_stats;
}

bool iGameStreamingFrameCacheAdapter::Matches(
    const ::datacodec::DecodedFrameKey& key) const {
    const auto identity = m_frameIdentities.find(key.frameIndex);
    return identity != m_frameIdentities.end() &&
        identity->second == key.source &&
        m_frameOrdinals.contains(key.frameIndex);
}

void iGameStreamingFrameCacheAdapter::RefreshResidentStatsLocked() const {
    const auto residentFrames = m_streamingData != nullptr
        ? m_streamingData->CachedFrameIndices().size()
        : 0u;
    const auto residentBytes = m_streamingData != nullptr
        ? m_streamingData->CachedResidentSizeHint()
        : 0u;
    m_stats.residentFrames = residentFrames;
    m_stats.residentBytes = residentBytes;
    m_stats.peakResidentBytes = std::max(m_stats.peakResidentBytes, residentBytes);
}

IGAME_NAMESPACE_END
