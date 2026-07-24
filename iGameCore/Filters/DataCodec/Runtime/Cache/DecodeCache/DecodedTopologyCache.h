#ifndef DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDTOPOLOGYCACHE_H
#define DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDTOPOLOGYCACHE_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedIndexCache.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecControlParams.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
namespace datacodec {

struct DecodedPolyhedronCache {
    DecodedPolyhedronCache() = default;
    DecodedPolyhedronCache(const DecodedPolyhedronCache&) = delete;
    DecodedPolyhedronCache& operator=(const DecodedPolyhedronCache&) = delete;

    DecodedPolyhedronCache(DecodedPolyhedronCache&& other) noexcept {
        MoveFrom(std::move(other));
    }

    DecodedPolyhedronCache& operator=(DecodedPolyhedronCache&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~DecodedPolyhedronCache() { Release(); }

    std::uint64_t cellCount{0u};
    std::uint64_t faceCount{0u};
    std::uint64_t uniqueVertexIdCount{0u};
    std::uint64_t localFaceVertexIdCount{0u};
    DecodedIndexCache uniqueVertexCounts;
    DecodedIndexCache cellFaceCounts;
    DecodedIndexCache faceVertexCounts;
    DecodedIndexCache cellUniqueVertexIds;
    DecodedIndexCache localFaceVertexIds;
    bool complete{false};

    bool PrepareForRead(std::string* error = nullptr) {
        if (!uniqueVertexCounts.PrepareForRead(error) ||
            !cellFaceCounts.PrepareForRead(error) ||
            !faceVertexCounts.PrepareForRead(error) ||
            !cellUniqueVertexIds.PrepareForRead(error) ||
            !localFaceVertexIds.PrepareForRead(error)) {
            return false;
        }
        complete = true;
        return true;
    }

    void Release() noexcept {
        uniqueVertexCounts.Release();
        cellFaceCounts.Release();
        faceVertexCounts.Release();
        cellUniqueVertexIds.Release();
        localFaceVertexIds.Release();
        cellCount = 0u;
        faceCount = 0u;
        uniqueVertexIdCount = 0u;
        localFaceVertexIdCount = 0u;
        complete = false;
    }

private:
    void MoveFrom(DecodedPolyhedronCache&& other) noexcept {
        cellCount = other.cellCount;
        faceCount = other.faceCount;
        uniqueVertexIdCount = other.uniqueVertexIdCount;
        localFaceVertexIdCount = other.localFaceVertexIdCount;
        uniqueVertexCounts = std::move(other.uniqueVertexCounts);
        cellFaceCounts = std::move(other.cellFaceCounts);
        faceVertexCounts = std::move(other.faceVertexCounts);
        cellUniqueVertexIds = std::move(other.cellUniqueVertexIds);
        localFaceVertexIds = std::move(other.localFaceVertexIds);
        complete = other.complete;
        other.cellCount = 0u;
        other.faceCount = 0u;
        other.uniqueVertexIdCount = 0u;
        other.localFaceVertexIdCount = 0u;
        other.complete = false;
    }
};

struct DecodedTopologyCache {
    DecodedTopologyCache() = default;
    DecodedTopologyCache(const DecodedTopologyCache&) = delete;
    DecodedTopologyCache& operator=(const DecodedTopologyCache&) = delete;

    DecodedTopologyCache(DecodedTopologyCache&& other) noexcept {
        MoveFrom(std::move(other));
    }

    DecodedTopologyCache& operator=(DecodedTopologyCache&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~DecodedTopologyCache() { Release(); }

    enum class Kind : std::uint8_t {
        None,
        Structured,
        Connectivity,
        Polyhedron,
    };

    enum class ByteStoreMode : std::uint8_t {
        None,
        Memory,
        Managed,
    };

    Kind kind{Kind::None};
    ByteStoreMode inputByteStoreMode{ByteStoreMode::None};
    ByteStoreMode byteStoreMode{ByteStoreMode::None};
    std::array<int, 3> structuredAxisSize{0, 0, 0};
    std::size_t cellCount{0u};
    std::size_t connectivityCount{0u};
    bool hasOffsets{false};
    bool hasCellTypes{false};
    bool hasCellPolynomialOrders{false};
    std::shared_ptr<bytestore::IRandomAccessByteStore> connectivity;
    std::shared_ptr<bytestore::IRandomAccessByteStore> offsets;
    std::shared_ptr<bytestore::IRandomAccessByteStore> cellTypes;
    std::shared_ptr<bytestore::IRandomAccessByteStore> cellPolynomialOrders;
    DecodedPolyhedronCache polyhedron;
    bool complete{false};

    bool InitializeConnectivity(
        const std::size_t cells,
        const std::size_t connectivityValues,
        const bool offsetsPresent,
        const bool cellTypesPresent,
        const bool cellPolynomialOrdersPresent,
        bytestore::ByteStoreSession& byteStoreSession,
        const std::uint64_t memoryCacheLimitBytes = 0u,
        const DecodeStorageMode storageMode = DecodeStorageMode::Managed,
        std::string* error = nullptr) {
        Release();
        kind = Kind::Connectivity;
        cellCount = cells;
        connectivityCount = connectivityValues;
        hasOffsets = offsetsPresent;
        hasCellTypes = cellTypesPresent;
        hasCellPolynomialOrders = cellPolynomialOrdersPresent;

        const auto connectivityBytes = static_cast<std::uint64_t>(connectivityCount) * sizeof(IndexType);
        const auto offsetBytes = hasOffsets ? static_cast<std::uint64_t>(cellCount + 1u) * sizeof(IndexType) : 0u;
        const auto cellTypeBytes = hasCellTypes ? static_cast<std::uint64_t>(cellCount) * sizeof(IndexType) : 0u;
        const auto polynomialOrderBytes =
            hasCellPolynomialOrders ? static_cast<std::uint64_t>(cellCount) * sizeof(std::uint16_t) : 0u;
        std::uint64_t totalBytes = 0u;
        bool byteCountOverflow = false;
        const auto addBytes = [&](const std::uint64_t bytes) {
            if (!validation::CanAddU64(totalBytes, bytes)) {
                byteCountOverflow = true;
                totalBytes = std::numeric_limits<std::uint64_t>::max();
                return;
            }
            totalBytes = validation::SaturatingAddU64(totalBytes, bytes);
        };
        addBytes(connectivityBytes);
        addBytes(offsetBytes);
        addBytes(cellTypeBytes);
        addBytes(polynomialOrderBytes);

        if (storageMode == DecodeStorageMode::Memory &&
            (byteCountOverflow || memoryCacheLimitBytes == 0u || totalBytes > memoryCacheLimitBytes)) {
            validation::AssignError(error, "decoded topology memory cache exceeds configured memory limit");
            Release();
            return false;
        }

        const bool useMemoryCache = storageMode == DecodeStorageMode::Memory;
        const auto initializeStores = [&](std::string* storeError) -> bool {
            byteStoreMode = useMemoryCache ? ByteStoreMode::Memory : ByteStoreMode::Managed;
            connectivity = useMemoryCache
                ? byteStoreSession.CreateMemoryStore()
                : byteStoreSession.CreateManagedByteStore("decoded_topology_connectivity", storeError);
            offsets = useMemoryCache
                ? byteStoreSession.CreateMemoryStore()
                : byteStoreSession.CreateManagedByteStore("decoded_topology_offsets", storeError);
            if (hasCellTypes) {
                cellTypes = useMemoryCache
                    ? byteStoreSession.CreateMemoryStore()
                    : byteStoreSession.CreateManagedByteStore("decoded_topology_cell_types", storeError);
            }
            if (hasCellPolynomialOrders) {
                cellPolynomialOrders = useMemoryCache
                    ? byteStoreSession.CreateMemoryStore()
                    : byteStoreSession.CreateManagedByteStore(
                        "decoded_topology_cell_polynomial_orders",
                        storeError);
            }
            if (connectivity == nullptr ||
                offsets == nullptr ||
                (hasCellTypes && cellTypes == nullptr) ||
                (hasCellPolynomialOrders && cellPolynomialOrders == nullptr)) {
                validation::AssignError(storeError, "failed to allocate decoded topology cache");
                return false;
            }
            return connectivity->ResizeBytes(connectivityBytes, storeError) &&
                offsets->ResizeBytes(offsetBytes, storeError) &&
                (!hasCellTypes || cellTypes->ResizeBytes(cellTypeBytes, storeError)) &&
                (!hasCellPolynomialOrders ||
                    cellPolynomialOrders->ResizeBytes(polynomialOrderBytes, storeError));
        };

        std::string storeError;
        if (initializeStores(&storeError)) {
            return true;
        }
        validation::AssignError(error, storeError.empty() ? "failed to allocate decoded topology cache" : storeError);
        Release();
        return false;
    }

    [[nodiscard]] const char* ByteStoreModeName() const noexcept {
        switch (byteStoreMode) {
            case ByteStoreMode::Memory:
                return "memory";
            case ByteStoreMode::Managed:
                return "managed";
            case ByteStoreMode::None:
            default:
                return "none";
        }
    }

    void SetInputByteStoreMode(const ByteStoreMode mode) noexcept {
        inputByteStoreMode = mode;
    }

    [[nodiscard]] const char* InputByteStoreModeName() const noexcept {
        switch (inputByteStoreMode) {
            case ByteStoreMode::Memory:
                return "memory";
            case ByteStoreMode::Managed:
                return "managed";
            case ByteStoreMode::None:
            default:
                return "none";
        }
    }

    void InitializeStructured(const std::array<int, 3>& axisSize) {
        Release();
        kind = Kind::Structured;
        byteStoreMode = ByteStoreMode::None;
        structuredAxisSize = axisSize;
        complete = true;
    }

    bool PrepareForRead(std::string* error = nullptr) {
        if (kind == Kind::Structured) {
            complete = true;
            return true;
        }
        if (kind == Kind::Polyhedron) {
            if (!polyhedron.PrepareForRead(error)) {
                return false;
            }
            complete = true;
            return true;
        }
        if (kind != Kind::Connectivity || connectivity == nullptr || offsets == nullptr ||
            (hasCellTypes && cellTypes == nullptr) ||
            (hasCellPolynomialOrders && cellPolynomialOrders == nullptr)) {
            return validation::AssignError(error, "decoded topology cache is incomplete");
        }
        if (!connectivity->Seal(error) ||
            !offsets->Seal(error) ||
            (hasCellTypes && !cellTypes->Seal(error)) ||
            (hasCellPolynomialOrders && !cellPolynomialOrders->Seal(error))) {
            return false;
        }
        complete = true;
        return true;
    }

    void Release() noexcept {
        if (connectivity != nullptr) {
            connectivity->Release();
        }
        if (offsets != nullptr) {
            offsets->Release();
        }
        if (cellTypes != nullptr) {
            cellTypes->Release();
        }
        if (cellPolynomialOrders != nullptr) {
            cellPolynomialOrders->Release();
        }
        connectivity.reset();
        offsets.reset();
        cellTypes.reset();
        cellPolynomialOrders.reset();
        polyhedron.Release();
        kind = Kind::None;
        inputByteStoreMode = ByteStoreMode::None;
        byteStoreMode = ByteStoreMode::None;
        structuredAxisSize = {0, 0, 0};
        cellCount = 0u;
        connectivityCount = 0u;
        hasOffsets = false;
        hasCellTypes = false;
        hasCellPolynomialOrders = false;
        complete = false;
    }

private:
    void MoveFrom(DecodedTopologyCache&& other) noexcept {
        kind = other.kind;
        inputByteStoreMode = other.inputByteStoreMode;
        byteStoreMode = other.byteStoreMode;
        structuredAxisSize = other.structuredAxisSize;
        cellCount = other.cellCount;
        connectivityCount = other.connectivityCount;
        hasOffsets = other.hasOffsets;
        hasCellTypes = other.hasCellTypes;
        hasCellPolynomialOrders = other.hasCellPolynomialOrders;
        connectivity = std::move(other.connectivity);
        offsets = std::move(other.offsets);
        cellTypes = std::move(other.cellTypes);
        cellPolynomialOrders = std::move(other.cellPolynomialOrders);
        polyhedron = std::move(other.polyhedron);
        complete = other.complete;
        other.kind = Kind::None;
        other.inputByteStoreMode = ByteStoreMode::None;
        other.byteStoreMode = ByteStoreMode::None;
        other.structuredAxisSize = {0, 0, 0};
        other.cellCount = 0u;
        other.connectivityCount = 0u;
        other.hasOffsets = false;
        other.hasCellTypes = false;
        other.hasCellPolynomialOrders = false;
        other.complete = false;
    }
};

class DecodedTopologyReferenceCacheStore final {
public:
    bool Contains(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries.find(key) != m_entries.end();
    }

    void Put(const std::string& key, std::shared_ptr<DecodedTopologyCache> cache) {
        if (!key.empty() && cache != nullptr && cache->complete) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_entries[key] = std::move(cache);
        }
    }

    std::shared_ptr<DecodedTopologyCache> Get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_entries.find(key);
        return it == m_entries.end() ? nullptr : it->second;
    }

    void Erase(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.erase(key);
    }

    void RetainKeys(const std::unordered_set<std::string>& keys) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto iterator = m_entries.begin(); iterator != m_entries.end();) {
            if (!keys.contains(iterator->first)) {
                iterator = m_entries.erase(iterator);
            } else {
                ++iterator;
            }
        }
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries.size();
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<DecodedTopologyCache>> m_entries;
};

} // namespace datacodec

#endif
