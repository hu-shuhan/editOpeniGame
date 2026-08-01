#ifndef DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDGEOMETRYCACHE_H
#define DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDGEOMETRYCACHE_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
namespace datacodec {

inline bool CalculateGeometryCacheBytes(
    const std::size_t count,
    const std::size_t dimension,
    std::uint64_t& byteCount,
    std::string* error = nullptr) {
    std::uint64_t valueCount = 0u;
    return validation::CheckedMulU64(
               static_cast<std::uint64_t>(count),
               static_cast<std::uint64_t>(dimension),
               valueCount,
               "decoded geometry value count",
               error) &&
        validation::CheckedMulU64(
            valueCount,
            static_cast<std::uint64_t>(sizeof(float)),
            byteCount,
            "decoded geometry cache bytes",
            error);
}

inline std::shared_ptr<bytestore::IRandomAccessByteStore> CreateDecodedGeometryStore(
    bytestore::ByteStoreSession& byteStoreSession,
    const std::string& label,
    const DecodeStorageMode storageMode,
    const std::uint64_t byteCount,
    const std::uint64_t memoryCacheLimitBytes,
    std::string* error = nullptr) {
    if (storageMode == DecodeStorageMode::Memory) {
        if (memoryCacheLimitBytes == 0u || byteCount > memoryCacheLimitBytes) {
            validation::AssignError(error, "decoded geometry memory cache exceeds configured memory limit");
            return nullptr;
        }
        auto store = byteStoreSession.CreateMemoryStore();
        if (store == nullptr) {
            validation::AssignError(error, "failed to allocate decoded geometry memory cache");
        }
        return store;
    }
    return byteStoreSession.CreateManagedByteStore(label, error);
}

struct DecodedGeometryCache {
    DecodedGeometryCache() = default;
    DecodedGeometryCache(const DecodedGeometryCache&) = delete;
    DecodedGeometryCache& operator=(const DecodedGeometryCache&) = delete;

    DecodedGeometryCache(DecodedGeometryCache&& other) noexcept {
        MoveFrom(std::move(other));
    }

    DecodedGeometryCache& operator=(DecodedGeometryCache&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~DecodedGeometryCache() { Release(); }

    std::size_t pointCount{0u};
    std::size_t dimension{0u};
    std::shared_ptr<bytestore::IRandomAccessByteStore> bytes;
    bool complete{false};

    bool Initialize(
        const std::size_t count,
        const std::size_t pointDimension,
        bytestore::ByteStoreSession& byteStoreSession,
        const DecodeStorageMode storageMode = DecodeStorageMode::Managed,
        const std::uint64_t memoryCacheLimitBytes = 0u,
        std::string* error = nullptr) {
        pointCount = count;
        dimension = pointDimension;
        complete = false;
        std::uint64_t byteCount = 0u;
        if (!CalculateGeometryCacheBytes(pointCount, dimension, byteCount, error)) {
            return false;
        }
        bytes = CreateDecodedGeometryStore(
            byteStoreSession,
            "decoded_geometry",
            storageMode,
            byteCount,
            memoryCacheLimitBytes,
            error);
        if (bytes == nullptr) {
            return validation::AssignError(error, "failed to allocate decoded geometry cache");
        }
        return bytes->ResizeBytes(byteCount, error);
    }

    void Release() noexcept {
        if (bytes != nullptr) {
            bytes->Release();
        }
        bytes.reset();
        pointCount = 0u;
        dimension = 0u;
        complete = false;
    }

private:
    void MoveFrom(DecodedGeometryCache&& other) noexcept {
        pointCount = other.pointCount;
        dimension = other.dimension;
        bytes = std::move(other.bytes);
        complete = other.complete;
        other.pointCount = 0u;
        other.dimension = 0u;
        other.complete = false;
    }
};

inline std::size_t DecodeGeometryTupleBytes(const GeometryStorageParams& meta) {
    std::size_t localValueSize = 0u;
    if (!TryParamSizeToSizeT(NumericArrayValueSize(meta), localValueSize)) {
        return 0u;
    }
    return static_cast<std::size_t>(std::max(meta.dimension, 0)) * localValueSize;
}

inline bool ValidateDecodedGeometryRange(
    const GeometryStorageParams& meta,
    const std::size_t offset,
    const std::size_t count,
    const std::size_t byteSize,
    std::string* error = nullptr) {
    const auto tupleBytes = DecodeGeometryTupleBytes(meta);
    if (tupleBytes == 0u) {
        if (count == 0u && byteSize == 0u) {
            return true;
        }
        return validation::AssignError(error, "decoded geometry tuple size is invalid");
    }
    std::size_t localElementCount = 0u;
    if (!TryParamSizeToSizeT(meta.elementCount, localElementCount)) {
        return validation::AssignError(error, "decoded geometry metadata exceeds this platform size limit");
    }
    if (offset > localElementCount || count > localElementCount - offset) {
        return validation::AssignError(error, "decoded geometry range is out of bounds");
    }
    std::size_t expectedBytes = 0u;
    if (!validation::CheckedMulSizeT(
            count,
            tupleBytes,
            expectedBytes,
            "decoded geometry byte size",
            error)) {
        return false;
    }
    if (byteSize != expectedBytes) {
        return validation::AssignError(error, "decoded geometry byte size does not match range");
    }
    return true;
}

class DecodedGeometryReferenceCache final {
public:
    ~DecodedGeometryReferenceCache() { Reset(); }

    bool InitializeOwned(
        const GeometryStorageParams& storageParams,
        std::string* error = nullptr) {
        return InitializeOwned(
            storageParams,
            DecodeStorageMode::Managed,
            0u,
            error);
    }

    bool InitializeOwned(
        const GeometryStorageParams& storageParams,
        const DecodeStorageMode storageMode,
        const std::uint64_t memoryCacheLimitBytes,
        std::string* error = nullptr) {
        Reset();
        auto byteStoreSession = std::make_shared<bytestore::ByteStoreSession>();
        m_ownedByteStoreSession = byteStoreSession;
        m_byteStoreSession = m_ownedByteStoreSession.get();
        if (!Initialize(storageParams, *byteStoreSession, storageMode, memoryCacheLimitBytes, error)) {
            Reset();
            return false;
        }
        return true;
    }

    bool Initialize(
        const GeometryStorageParams& storageParams,
        bytestore::ByteStoreSession& byteStoreSession,
        const DecodeStorageMode storageMode = DecodeStorageMode::Managed,
        const std::uint64_t memoryCacheLimitBytes = 0u,
        std::string* error = nullptr) {
        if (m_initialized || m_bytes != nullptr ||
            (m_ownedByteStoreSession != nullptr &&
             m_ownedByteStoreSession.get() != &byteStoreSession)) {
            Reset();
        }
        std::size_t localElementCount = 0u;
        std::size_t localValueSize = 0u;
        if (!TryParamSizeToSizeT(storageParams.elementCount, localElementCount) ||
            !TryParamSizeToSizeT(NumericArrayValueSize(storageParams), localValueSize)) {
            validation::AssignError(error, "decoded geometry reference metadata exceeds this platform size limit");
            Reset();
            return false;
        }
        m_storageParams = storageParams;
        m_tupleBytes = static_cast<std::size_t>(std::max(storageParams.dimension, 0)) * localValueSize;
        m_complete = false;
        m_byteStoreSession = &byteStoreSession;
        m_storageMode = storageMode;
        m_memoryCacheLimitBytes = memoryCacheLimitBytes;
        const auto totalBytes = static_cast<std::uint64_t>(localElementCount) *
            static_cast<std::uint64_t>(m_tupleBytes);
        m_bytes = CreateDecodedGeometryStore(
            byteStoreSession,
            "decoded_geometry_reference",
            storageMode,
            totalBytes,
            memoryCacheLimitBytes,
            error);
        if (m_bytes == nullptr) {
            validation::AssignError(error, "failed to allocate decoded geometry reference cache");
            Reset();
            return false;
        }
        if (!m_bytes->ResizeBytes(totalBytes, error)) {
            Reset();
            return false;
        }
        m_initialized = true;
        return true;
    }

    void Reset() noexcept {
        auto* retainedByteStoreSession = m_ownedByteStoreSession == nullptr
            ? m_byteStoreSession
            : nullptr;
        if (m_bytes != nullptr) {
            m_bytes->Release();
        }
        m_bytes.reset();
        m_storageParams = {};
        m_tupleBytes = 0u;
        m_storageMode = DecodeStorageMode::Managed;
        m_memoryCacheLimitBytes = 0u;
        if (m_ownedByteStoreSession != nullptr) {
            m_ownedByteStoreSession.reset();
        }
        m_byteStoreSession = retainedByteStoreSession;
        m_initialized = false;
        m_complete = false;
    }

    [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized; }
    [[nodiscard]] bool IsComplete() const noexcept { return m_initialized && m_complete; }
    [[nodiscard]] const GeometryStorageParams& StorageParams() const noexcept { return m_storageParams; }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept {
        return m_bytes != nullptr ? m_bytes->ResidentSizeHint() : 0u;
    }

    bool BeginGeometry(const GeometryStorageParams& meta, std::string* error = nullptr) {
        if (!m_initialized) {
            if (m_byteStoreSession == nullptr) {
                return validation::AssignError(error, "decoded geometry reference cache requires a byte store session");
            }
            if (!Initialize(meta, *m_byteStoreSession, m_storageMode, m_memoryCacheLimitBytes, error)) {
                return false;
            }
        }
        if (!m_initialized) {
            return false;
        }
        std::size_t localElementCount = 0u;
        if (!TryParamSizeToSizeT(meta.elementCount, localElementCount) ||
            !TryParamSizeToSizeT(NumericArrayValueSize(meta), m_tupleBytes)) {
            return validation::AssignError(error, "decoded geometry metadata exceeds this platform size limit");
        }
        m_storageParams = meta;
        m_tupleBytes *= static_cast<std::size_t>(std::max(meta.dimension, 0));
        m_complete = false;
        const auto totalBytes = static_cast<std::uint64_t>(localElementCount) *
            static_cast<std::uint64_t>(m_tupleBytes);
        return m_bytes != nullptr && m_bytes->ResizeBytes(totalBytes, error);
    }

    bool WriteRange(
        const std::size_t offset,
        const std::size_t count,
        const void* data,
        const std::size_t byteSize,
        std::string* error = nullptr) {
        if (!m_initialized) {
            return validation::AssignError(error, "decoded geometry reference cache is not initialized");
        }
        if (!ValidateDecodedGeometryRange(m_storageParams, offset, count, byteSize, error)) {
            return false;
        }
        if (byteSize == 0u) {
            return true;
        }
        if (data == nullptr) {
            return validation::AssignError(error, "decoded geometry reference write source is null");
        }
        return m_bytes != nullptr &&
            m_bytes->WriteBytesAt(
                static_cast<std::uint64_t>(offset) * static_cast<std::uint64_t>(m_tupleBytes),
                std::span<const std::uint8_t>(static_cast<const std::uint8_t*>(data), byteSize),
                error);
    }

    bool EndGeometry(std::string* error = nullptr) {
        if (!m_initialized || m_bytes == nullptr) {
            return validation::AssignError(
                error,
                "decoded geometry reference cache is not initialized");
        }
        if (!m_bytes->Seal(error)) {
            return false;
        }
        m_complete = true;
        return true;
    }

    bool ReadRange(
        const std::size_t offset,
        const std::size_t count,
        void* data,
        const std::size_t byteSize,
        std::string* error = nullptr) const {
        if (!IsComplete()) {
            return validation::AssignError(error, "decoded geometry reference cache is not complete");
        }
        if (!ValidateDecodedGeometryRange(m_storageParams, offset, count, byteSize, error)) {
            return false;
        }
        if (byteSize == 0u) {
            return true;
        }
        if (data == nullptr) {
            return validation::AssignError(error, "decoded geometry reference read target is null");
        }
        return m_bytes != nullptr &&
            m_bytes->Read(
                static_cast<std::uint64_t>(offset) * static_cast<std::uint64_t>(m_tupleBytes),
                std::span<std::uint8_t>(static_cast<std::uint8_t*>(data), byteSize),
                error);
    }

    [[nodiscard]] std::shared_ptr<bytestore::IRandomAccessByteStore> Bytes() const noexcept {
        return m_bytes;
    }

private:
    GeometryStorageParams m_storageParams;
    std::size_t m_tupleBytes{0u};
    std::shared_ptr<bytestore::IRandomAccessByteStore> m_bytes;
    bytestore::ByteStoreSession* m_byteStoreSession{nullptr};
    std::shared_ptr<bytestore::ByteStoreSession> m_ownedByteStoreSession;
    DecodeStorageMode m_storageMode{DecodeStorageMode::Managed};
    std::uint64_t m_memoryCacheLimitBytes{0u};
    bool m_initialized{false};
    bool m_complete{false};
};

} // namespace datacodec

#endif
