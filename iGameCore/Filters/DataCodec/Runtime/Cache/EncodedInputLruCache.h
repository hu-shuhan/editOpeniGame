#ifndef DATACODEC_RUNTIME_CACHE_ENCODEDINPUTLRUCACHE_H
#define DATACODEC_RUNTIME_CACHE_ENCODEDINPUTLRUCACHE_H

#include "DataCodec/API/Adapter/IEncodedInputCache.h"
#include "DataCodec/Runtime/Cache/LruCacheIndex.h"

#include <cstddef>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace datacodec {

class EncodedInputLruCache final : public IEncodedInputCache {
public:
    void Configure(const std::size_t inputLimit, const std::uint64_t residentLimitBytes) {
        std::vector<EncodedInputBuffer> evicted;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_index.Configure(inputLimit, residentLimitBytes);
            PruneLocked({}, evicted);
            RefreshStatsLocked();
        }
    }

    [[nodiscard]] EncodedInputCacheLookupResult Find(
        const DecodeSourceIdentity& source,
        const EncodedInputAccessKind accessKind) override {
        std::vector<EncodedInputBuffer> evicted;
        EncodedInputBuffer input;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!source.IsStable()) {
                ++m_stats.lookupErrors;
                return EncodedInputCacheLookupResult::Error(
                    "encoded input cache lookup requires a stable source identity");
            }
            ++m_stats.lookups;
            const auto iterator = m_inputs.find(source);
            if (iterator == m_inputs.end()) {
                ++m_stats.misses;
                PruneLocked({}, evicted);
            } else {
                if (iterator->second == nullptr) {
                    ++m_stats.lookupErrors;
                    return EncodedInputCacheLookupResult::Error(
                        "encoded input cache contains a null input");
                }
                ++m_stats.hits;
                if (accessKind == EncodedInputAccessKind::UserRequest) {
                    m_index.Touch(source);
                }
                input = iterator->second;
                PruneLocked(source, evicted);
            }
            RefreshStatsLocked();
        }
        return input != nullptr
            ? EncodedInputCacheLookupResult::Hit(std::move(input))
            : EncodedInputCacheLookupResult::Miss();
    }

    [[nodiscard]] CacheStoreResult Store(
        const DecodeSourceIdentity& source,
        EncodedInputBuffer input,
        const EncodedInputAccessKind accessKind) override {
        if (!source.IsStable() || input == nullptr) {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_stats.storeErrors;
            return CacheStoreResult::Error("encoded input cache store input is invalid");
        }
        std::vector<EncodedInputBuffer> evicted;
        bool stored = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_index.CanAdmitSingle(static_cast<std::uint64_t>(input->size()))) {
                ++m_stats.storeRejections;
                return CacheStoreResult::RejectedByPolicy();
            }
            const auto existing = m_inputs.find(source);
            if (existing != m_inputs.end()) {
                evicted.push_back(std::move(existing->second));
                existing->second = std::move(input);
            } else {
                m_inputs.emplace(source, std::move(input));
            }
            m_index.InsertOrAssign(
                source,
                static_cast<std::uint64_t>(m_inputs.at(source)->size()),
                accessKind == EncodedInputAccessKind::UserRequest);
            ++m_stats.stores;
            PruneLocked(source, evicted);
            if (m_index.OverBudget()) {
                const auto inserted = m_inputs.find(source);
                if (inserted != m_inputs.end()) {
                    evicted.push_back(std::move(inserted->second));
                    m_inputs.erase(inserted);
                    m_index.Erase(source);
                }
                --m_stats.stores;
                ++m_stats.storeRejections;
                RefreshStatsLocked();
                return CacheStoreResult::RejectedByPolicy();
            }
            RefreshStatsLocked();
            stored = m_inputs.contains(source);
        }
        if (!stored) {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_stats.storeErrors;
            return CacheStoreResult::Error("encoded input cache did not retain the stored input");
        }
        return CacheStoreResult::Stored();
    }

    void InvalidateSource(const DecodeSourceIdentity& source) override {
        std::vector<EncodedInputBuffer> released;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto iterator = m_inputs.begin(); iterator != m_inputs.end();) {
                if (iterator->first.stableId != source.stableId) {
                    ++iterator;
                    continue;
                }
                released.push_back(std::move(iterator->second));
                m_index.Erase(iterator->first);
                iterator = m_inputs.erase(iterator);
            }
            RefreshStatsLocked();
        }
    }

    [[nodiscard]] EncodedInputCacheStats Statistics() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stats;
    }

private:
    void PruneLocked(
        const std::optional<DecodeSourceIdentity>& protectedSource,
        std::vector<EncodedInputBuffer>& evicted) {
        for (const auto& candidate : m_index.LeastRecentlyUsedOrder()) {
            if (!m_index.OverBudget()) { break; }
            if (protectedSource.has_value() && candidate == *protectedSource) { continue; }
            const auto iterator = m_inputs.find(candidate);
            if (iterator == m_inputs.end()) {
                m_index.Erase(candidate);
                continue;
            }
            if (iterator->second.use_count() > 1u) { continue; }
            evicted.push_back(std::move(iterator->second));
            m_inputs.erase(iterator);
            m_index.Erase(candidate);
            ++m_stats.evictions;
        }
    }

    void RefreshStatsLocked() noexcept {
        m_stats.residentInputs = m_index.Size();
        m_stats.residentBytes = m_index.ResidentBytes();
        m_stats.peakResidentBytes = m_index.PeakResidentBytes();
    }

    mutable std::mutex m_mutex;
    std::unordered_map<DecodeSourceIdentity, EncodedInputBuffer, DecodeSourceIdentityHash> m_inputs;
    LruCacheIndex<DecodeSourceIdentity, DecodeSourceIdentityHash> m_index;
    EncodedInputCacheStats m_stats;
};

} // namespace datacodec

#endif
