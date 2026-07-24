#ifndef DATACODEC_RUNTIME_CACHE_DECODEDFRAMELRUCACHE_H
#define DATACODEC_RUNTIME_CACHE_DECODEDFRAMELRUCACHE_H

#include "DataCodec/API/Adapter/IDecodedFrameCache.h"
#include "DataCodec/Runtime/Cache/LruCacheIndex.h"

#include <algorithm>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace datacodec {

class DecodedFrameLruCache final : public IDecodedFrameCache {
public:
    void SetEnabled(const bool enabled) {
        std::vector<DecodedFrameLease::Pointer> released;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_enabled = enabled;
            if (!m_enabled) {
                ClearLocked(released);
            }
        }
    }

    [[nodiscard]] bool IsEnabled() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_enabled;
    }

    void Clear() {
        std::vector<DecodedFrameLease::Pointer> released;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            ClearLocked(released);
        }
    }

    void Configure(const std::size_t frameLimit, const std::uint64_t residentLimitBytes) {
        std::vector<DecodedFrameLease::Pointer> evicted;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_index.Configure(frameLimit, residentLimitBytes);
            PruneLocked({}, evicted);
            RefreshStatsLocked();
        }
    }

    [[nodiscard]] DecodedFrameCacheLookupResult Find(
        const DecodedFrameKey& key,
        const DecodedFrameAccessKind accessKind) override {
        std::vector<DecodedFrameLease::Pointer> evicted;
        DecodedFrameLease::Pointer frame;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!key.source.IsStable()) {
                ++m_stats.lookupErrors;
                return DecodedFrameCacheLookupResult::Error(
                    "decoded frame cache lookup requires a stable source identity");
            }
            if (!m_enabled) { return DecodedFrameCacheLookupResult::Miss(); }
            ++m_stats.lookups;
            const auto iterator = m_frames.find(key);
            if (iterator == m_frames.end()) {
                ++m_stats.misses;
                PruneLocked(std::nullopt, evicted);
                RefreshStatsLocked();
            } else {
                if (iterator->second == nullptr ||
                    iterator->second->FrameIndex() != key.frameIndex ||
                    iterator->second->Payload() == nullptr) {
                    ++m_stats.lookupErrors;
                    return DecodedFrameCacheLookupResult::Error(
                        "decoded frame cache contains an invalid frame lease");
                }
                ++m_stats.hits;
                if (accessKind == DecodedFrameAccessKind::UserRequest) {
                    m_index.Touch(key);
                }
                frame = iterator->second;
                PruneLocked(key, evicted);
                RefreshStatsLocked();
            }
        }
        return frame != nullptr
            ? DecodedFrameCacheLookupResult::Hit(std::move(frame))
            : DecodedFrameCacheLookupResult::Miss();
    }

    [[nodiscard]] CacheStoreResult Store(
        const DecodedFrameKey& key,
        DecodedFrameLease::Pointer frame,
        const DecodedFrameAccessKind accessKind) override {
        if (frame == nullptr || !key.source.IsStable() ||
            frame->FrameIndex() != key.frameIndex || frame->Payload() == nullptr) {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_stats.storeErrors;
            return CacheStoreResult::Error("decoded frame cache store input is invalid");
        }
        std::vector<DecodedFrameLease::Pointer> evicted;
        bool stored = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_enabled || !m_index.CanAdmitSingle(frame->ResidentSizeHint())) {
                ++m_stats.storeRejections;
                return CacheStoreResult::RejectedByPolicy();
            }
            const auto existing = m_frames.find(key);
            if (existing != m_frames.end()) {
                evicted.push_back(std::move(existing->second));
                existing->second = std::move(frame);
            } else {
                m_frames.emplace(key, std::move(frame));
            }
            m_index.InsertOrAssign(
                key,
                m_frames.at(key)->ResidentSizeHint(),
                accessKind == DecodedFrameAccessKind::UserRequest);
            ++m_stats.stores;
            PruneLocked(key, evicted);
            if (m_index.OverBudget()) {
                const auto inserted = m_frames.find(key);
                if (inserted != m_frames.end()) {
                    evicted.push_back(std::move(inserted->second));
                    m_frames.erase(inserted);
                    m_index.Erase(key);
                }
                --m_stats.stores;
                ++m_stats.storeRejections;
                RefreshStatsLocked();
                return CacheStoreResult::RejectedByPolicy();
            }
            RefreshStatsLocked();
            stored = m_frames.contains(key);
        }
        if (!stored) {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_stats.storeErrors;
            return CacheStoreResult::Error("decoded frame cache did not retain the stored frame");
        }
        return CacheStoreResult::Stored();
    }

    void InvalidateSource(const DecodeSourceIdentity& source) override {
        std::vector<DecodedFrameLease::Pointer> released;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto iterator = m_frames.begin(); iterator != m_frames.end();) {
                if (iterator->first.source.stableId != source.stableId) {
                    ++iterator;
                    continue;
                }
                released.push_back(std::move(iterator->second));
                m_index.Erase(iterator->first);
                iterator = m_frames.erase(iterator);
            }
            RefreshStatsLocked();
        }
    }

    [[nodiscard]] std::vector<std::uint32_t> ResidentFrameIndices(
        const DecodeSourceIdentity& source) const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::uint32_t> frames;
        for (const auto& [key, frame] : m_frames) {
            (void)frame;
            if (key.source.stableId == source.stableId) {
                frames.push_back(key.frameIndex);
            }
        }
        std::sort(frames.begin(), frames.end());
        return frames;
    }

    [[nodiscard]] DecodedFrameCacheStats Statistics() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stats;
    }

private:
    void ClearLocked(std::vector<DecodedFrameLease::Pointer>& released) {
        released.reserve(released.size() + m_frames.size());
        for (auto& [key, frame] : m_frames) {
            (void)key;
            released.push_back(std::move(frame));
        }
        m_frames.clear();
        m_index.Clear();
        RefreshStatsLocked();
    }

    void PruneLocked(
        const std::optional<DecodedFrameKey>& protectedKey,
        std::vector<DecodedFrameLease::Pointer>& evicted) {
        for (const auto& candidate : m_index.LeastRecentlyUsedOrder()) {
            if (!m_index.OverBudget()) { break; }
            if (protectedKey.has_value() && candidate == *protectedKey) { continue; }
            const auto iterator = m_frames.find(candidate);
            if (iterator == m_frames.end()) {
                m_index.Erase(candidate);
                continue;
            }
            if (iterator->second.use_count() > 1u) { continue; }
            evicted.push_back(std::move(iterator->second));
            m_frames.erase(iterator);
            m_index.Erase(candidate);
            ++m_stats.evictions;
        }
    }

    void RefreshStatsLocked() noexcept {
        m_stats.residentFrames = m_index.Size();
        m_stats.residentBytes = m_index.ResidentBytes();
        m_stats.peakResidentBytes = m_index.PeakResidentBytes();
    }

    mutable std::mutex m_mutex;
    std::unordered_map<DecodedFrameKey, DecodedFrameLease::Pointer, DecodedFrameKeyHash> m_frames;
    LruCacheIndex<DecodedFrameKey, DecodedFrameKeyHash> m_index;
    DecodedFrameCacheStats m_stats;
    bool m_enabled{true};
};

} // namespace datacodec

#endif
