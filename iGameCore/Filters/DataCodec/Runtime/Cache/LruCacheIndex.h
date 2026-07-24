#ifndef DATACODEC_RUNTIME_CACHE_LRUCACHEINDEX_H
#define DATACODEC_RUNTIME_CACHE_LRUCACHEINDEX_H

#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstdint>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace datacodec {

// 仅维护LRU顺序和驻留字节账本，条目内容由具体缓存持有
template<typename Key, typename Hash>
class LruCacheIndex final {
public:
    void Configure(const std::size_t entryLimit, const std::uint64_t residentLimitBytes) noexcept {
        m_entryLimit = entryLimit;
        m_residentLimitBytes = residentLimitBytes;
    }

    void Touch(const Key& key) {
        const auto iterator = m_records.find(key);
        if (iterator == m_records.end()) { return; }
        m_order.splice(m_order.begin(), m_order, iterator->second.orderIterator);
        iterator->second.orderIterator = m_order.begin();
    }

    void InsertOrAssign(
        const Key& key,
        const std::uint64_t residentBytes,
        const bool mostRecent = true) {
        const auto iterator = m_records.find(key);
        if (iterator != m_records.end()) {
            m_residentBytes = iterator->second.residentBytes >= m_residentBytes
                ? 0u
                : m_residentBytes - iterator->second.residentBytes;
            iterator->second.residentBytes = residentBytes;
            m_residentBytes = validation::SaturatingAddU64(m_residentBytes, residentBytes);
            if (mostRecent) { Touch(key); }
        } else {
            if (mostRecent) {
                m_order.push_front(key);
                m_records.emplace(key, Record{residentBytes, m_order.begin()});
            } else {
                m_order.push_back(key);
                auto position = m_order.end();
                --position;
                m_records.emplace(key, Record{residentBytes, position});
            }
            m_residentBytes = validation::SaturatingAddU64(m_residentBytes, residentBytes);
        }
        m_peakResidentBytes = std::max(m_peakResidentBytes, m_residentBytes);
    }

    void Erase(const Key& key) noexcept {
        const auto iterator = m_records.find(key);
        if (iterator == m_records.end()) { return; }
        m_residentBytes = iterator->second.residentBytes >= m_residentBytes
            ? 0u
            : m_residentBytes - iterator->second.residentBytes;
        m_order.erase(iterator->second.orderIterator);
        m_records.erase(iterator);
    }

    void Clear() noexcept {
        m_records.clear();
        m_order.clear();
        m_residentBytes = 0u;
    }

    [[nodiscard]] bool OverBudget() const noexcept {
        const bool entryLimitExceeded = m_entryLimit != 0u && m_records.size() > m_entryLimit;
        const bool residentLimitExceeded =
            m_residentLimitBytes != 0u && m_residentBytes > m_residentLimitBytes;
        return entryLimitExceeded || residentLimitExceeded;
    }

    [[nodiscard]] bool CanAdmitSingle(const std::uint64_t residentBytes) const noexcept {
        return (m_entryLimit == 0u || m_entryLimit >= 1u) &&
            (m_residentLimitBytes == 0u || residentBytes <= m_residentLimitBytes);
    }

    [[nodiscard]] std::optional<Key> LeastRecentlyUsed() const {
        if (m_order.empty()) { return std::nullopt; }
        return m_order.back();
    }

    [[nodiscard]] std::vector<Key> LeastRecentlyUsedOrder() const {
        return {m_order.rbegin(), m_order.rend()};
    }

    [[nodiscard]] std::size_t Size() const noexcept { return m_records.size(); }
    [[nodiscard]] std::uint64_t ResidentBytes() const noexcept { return m_residentBytes; }
    [[nodiscard]] std::uint64_t PeakResidentBytes() const noexcept { return m_peakResidentBytes; }

private:
    struct Record {
        std::uint64_t residentBytes{0u};
        typename std::list<Key>::iterator orderIterator;
    };

    std::size_t m_entryLimit{0u};
    std::uint64_t m_residentLimitBytes{0u};
    std::uint64_t m_residentBytes{0u};
    std::uint64_t m_peakResidentBytes{0u};
    std::list<Key> m_order;
    std::unordered_map<Key, Record, Hash> m_records;
};

} // namespace datacodec

#endif
