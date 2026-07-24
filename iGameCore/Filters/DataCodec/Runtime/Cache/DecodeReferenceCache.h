#ifndef DATACODEC_RUNTIME_CACHE_DECODEREFERENCECACHE_H
#define DATACODEC_RUNTIME_CACHE_DECODEREFERENCECACHE_H

#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Runtime/Cache/DecodeCacheIdentity.h"
#include "DataCodec/Codec/Reference/DecodedReference.h"
#include "DataCodec/Runtime/Cache/LruCacheIndex.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace datacodec {

struct DecodedTopologyReference {
    std::shared_ptr<DecodedTopologyCache> store;
    std::shared_ptr<bytestore::ByteStoreSession> byteStoreSession;
};

struct DecodeReferenceLeaf {
    bool requiresAttribute{false};
    bool requiresGeometry{false};
    std::unordered_set<std::string> requiredTopology;
    std::optional<DecodedAttributeReference> attribute;
    std::optional<DecodedGeometryReference> geometry;
    std::unordered_map<std::string, DecodedTopologyReference> topology;
};

struct DecodeReferenceFrame {
    std::uint32_t frameIndex{0u};
    bool complete{false};
    std::unordered_map<BlockPath, DecodeReferenceLeaf> leaves;

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept {
        std::uint64_t bytes = 0u;
        for (const auto& [path, leaf] : leaves) {
            (void)path;
            if (leaf.attribute.has_value() && leaf.attribute->store != nullptr) {
                bytes = validation::SaturatingAddU64(
                    bytes,
                    leaf.attribute->store->ResidentSizeHint());
            }
            if (leaf.geometry.has_value() && leaf.geometry->store != nullptr) {
                bytes = validation::SaturatingAddU64(
                    bytes,
                    leaf.geometry->store->ResidentSizeHint());
            }
            for (const auto& [key, topologyReference] : leaf.topology) {
                (void)key;
                const auto& topology = topologyReference.store;
                if (topology == nullptr) { continue; }
                const auto connectivityBytes =
                    static_cast<std::uint64_t>(topology->connectivityCount) * sizeof(IndexType);
                const auto offsetBytes = topology->hasOffsets
                    ? static_cast<std::uint64_t>(topology->cellCount + 1u) * sizeof(IndexType)
                    : 0u;
                const auto cellTypeBytes = topology->hasCellTypes
                    ? static_cast<std::uint64_t>(topology->cellCount) * sizeof(IndexType)
                    : 0u;
                const auto polynomialOrderBytes = topology->hasCellPolynomialOrders
                    ? static_cast<std::uint64_t>(topology->cellCount) * sizeof(std::uint16_t)
                    : 0u;
                const auto& polyhedron = topology->polyhedron;
                const auto polyhedronIndexCount = validation::SaturatingAddU64(
                    validation::SaturatingAddU64(
                        static_cast<std::uint64_t>(polyhedron.cellCount) * 2u,
                        polyhedron.faceCount),
                    validation::SaturatingAddU64(
                        polyhedron.uniqueVertexIdCount,
                        polyhedron.localFaceVertexIdCount));
                const auto polyhedronBytes = validation::SaturatingMulU64(
                    polyhedronIndexCount,
                    sizeof(IndexType));
                bytes = validation::SaturatingAddU64(bytes, connectivityBytes);
                bytes = validation::SaturatingAddU64(bytes, offsetBytes);
                bytes = validation::SaturatingAddU64(bytes, cellTypeBytes);
                bytes = validation::SaturatingAddU64(bytes, polynomialOrderBytes);
                bytes = validation::SaturatingAddU64(bytes, polyhedronBytes);
            }
        }
        return bytes;
    }
};

struct DecodeReferenceCacheStats {
    std::uint64_t lookups{0u};
    std::uint64_t hits{0u};
    std::uint64_t misses{0u};
    std::uint64_t publishes{0u};
    std::uint64_t evictions{0u};
    std::uint64_t residentBytes{0u};
    std::uint64_t peakResidentBytes{0u};
    std::size_t residentFrames{0u};
};

class DecodeReferenceCache final {
public:
    using FramePointer = std::shared_ptr<DecodeReferenceFrame>;

    void Configure(const std::size_t frameLimit, const std::uint64_t residentLimitBytes) {
        std::vector<FramePointer> evicted;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_index.Configure(frameLimit, residentLimitBytes);
            PruneLocked({}, evicted);
            RefreshStatsLocked();
        }
    }

    [[nodiscard]] FramePointer Find(const DecodeReferenceKey& key) {
        std::vector<FramePointer> evicted;
        FramePointer frame;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_stats.lookups;
            const auto iterator = m_frames.find(key);
            if (iterator == m_frames.end()) {
                ++m_stats.misses;
                PruneLocked(std::nullopt, evicted);
                RefreshStatsLocked();
            } else {
                ++m_stats.hits;
                m_index.Touch(key);
                frame = iterator->second;
                PruneLocked(key, evicted);
                RefreshStatsLocked();
            }
        }
        return frame;
    }

    void Publish(const DecodeReferenceKey& key, DecodeReferenceFrame frame) {
        if (!key.source.IsStable() || frame.leaves.empty()) { return; }
        std::vector<FramePointer> evicted;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            const auto existing = m_frames.find(key);
            if (existing != m_frames.end()) {
                MergeFrame(*existing->second, frame);
            }
            frame.frameIndex = key.keyFrameIndex;
            frame.complete = IsComplete(frame);
            auto published = std::make_shared<DecodeReferenceFrame>(std::move(frame));
            if (existing != m_frames.end()) {
                evicted.push_back(std::move(existing->second));
                existing->second = published;
            } else {
                m_frames.emplace(key, published);
            }
            m_index.InsertOrAssign(key, published->ResidentSizeHint());
            ++m_stats.publishes;
            PruneLocked(key, evicted);
            RefreshStatsLocked();
        }
    }

    void InvalidateSource(const DecodeSourceIdentity& source) {
        std::vector<FramePointer> released;
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

    void Clear() {
        std::unordered_map<DecodeReferenceKey, FramePointer, DecodeReferenceKeyHash> released;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            released.swap(m_frames);
            m_index.Clear();
            RefreshStatsLocked();
        }
    }

    [[nodiscard]] std::vector<std::uint32_t> ResidentFrameIndices(
        const DecodeSourceIdentity& source) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::uint32_t> frames;
        for (const auto& [key, frame] : m_frames) {
            (void)frame;
            if (key.source.stableId == source.stableId) { frames.push_back(key.keyFrameIndex); }
        }
        std::sort(frames.begin(), frames.end());
        frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
        return frames;
    }

    [[nodiscard]] DecodeReferenceCacheStats Statistics() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stats;
    }

private:
    static void MergeFrame(const DecodeReferenceFrame& existing, DecodeReferenceFrame& incoming) {
        for (const auto& [path, oldLeaf] : existing.leaves) {
            auto& leaf = incoming.leaves[path];
            leaf.requiresAttribute = leaf.requiresAttribute || oldLeaf.requiresAttribute;
            leaf.requiresGeometry = leaf.requiresGeometry || oldLeaf.requiresGeometry;
            leaf.requiredTopology.insert(
                oldLeaf.requiredTopology.begin(),
                oldLeaf.requiredTopology.end());
            if (!leaf.attribute.has_value() && oldLeaf.attribute.has_value()) {
                leaf.attribute = oldLeaf.attribute;
            }
            if (!leaf.geometry.has_value() && oldLeaf.geometry.has_value()) {
                leaf.geometry = oldLeaf.geometry;
            }
            for (const auto& [key, topology] : oldLeaf.topology) {
                leaf.topology.try_emplace(key, topology);
            }
        }
    }

    [[nodiscard]] static bool IsComplete(const DecodeReferenceFrame& frame) {
        for (const auto& [path, leaf] : frame.leaves) {
            (void)path;
            if (leaf.requiresAttribute &&
                (!leaf.attribute.has_value() ||
                 leaf.attribute->store == nullptr ||
                 !leaf.attribute->store->IsComplete())) {
                return false;
            }
            if (leaf.requiresGeometry &&
                (!leaf.geometry.has_value() ||
                 leaf.geometry->store == nullptr ||
                 !leaf.geometry->store->IsComplete())) {
                return false;
            }
            for (const auto& key : leaf.requiredTopology) {
                const auto topology = leaf.topology.find(key);
                if (topology == leaf.topology.end() ||
                    topology->second.store == nullptr ||
                    !topology->second.store->complete) {
                    return false;
                }
            }
        }
        return !frame.leaves.empty();
    }

    void PruneLocked(
        const std::optional<DecodeReferenceKey>& protectedKey,
        std::vector<FramePointer>& evicted) {
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
    std::unordered_map<DecodeReferenceKey, FramePointer, DecodeReferenceKeyHash> m_frames;
    LruCacheIndex<DecodeReferenceKey, DecodeReferenceKeyHash> m_index;
    DecodeReferenceCacheStats m_stats;
};

} // namespace datacodec

#endif
