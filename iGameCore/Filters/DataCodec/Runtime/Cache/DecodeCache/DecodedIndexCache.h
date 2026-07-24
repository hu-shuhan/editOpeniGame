#ifndef DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDINDEXCACHE_H
#define DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDINDEXCACHE_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecControlParams.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

inline bool CalculateIndexCacheBytes(
    const std::uint64_t count,
    std::uint64_t& byteCount,
    std::string* error = nullptr) {
    return validation::CheckedMulU64(
        count,
        static_cast<std::uint64_t>(sizeof(IndexType)),
        byteCount,
        "decoded index cache bytes",
        error);
}

inline std::shared_ptr<bytestore::IRandomAccessByteStore> CreateDecodedIndexStore(
    bytestore::ByteStoreSession& byteStoreSession,
    const DecodeStorageMode storageMode,
    const std::uint64_t byteCount,
    const std::uint64_t memoryCacheLimitBytes,
    std::string* error = nullptr) {
    if (storageMode == DecodeStorageMode::Memory) {
        if (memoryCacheLimitBytes == 0u || byteCount > memoryCacheLimitBytes) {
            validation::AssignError(error, "decoded index memory cache exceeds configured memory limit");
            return nullptr;
        }
        auto store = byteStoreSession.CreateMemoryStore();
        if (store == nullptr) {
            validation::AssignError(error, "failed to allocate decoded index memory cache");
        }
        return store;
    }
    return byteStoreSession.CreateManagedByteStore("decoded_index", error);
}

class DecodedIndexCache final {
public:
    DecodedIndexCache() = default;
    DecodedIndexCache(const DecodedIndexCache&) = delete;
    DecodedIndexCache& operator=(const DecodedIndexCache&) = delete;

    DecodedIndexCache(DecodedIndexCache&& other) noexcept {
        MoveFrom(std::move(other));
    }

    DecodedIndexCache& operator=(DecodedIndexCache&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~DecodedIndexCache() { Release(); }

    bool Initialize(
        const std::uint64_t count,
        bytestore::ByteStoreSession& byteStoreSession,
        const DecodeStorageMode storageMode = DecodeStorageMode::Managed,
        const std::uint64_t memoryCacheLimitBytes = 0u,
        std::string* error = nullptr) {
        Release();
        std::uint64_t byteCount = 0u;
        if (!CalculateIndexCacheBytes(count, byteCount, error)) {
            return false;
        }
        m_count = count;
        m_written = 0u;
        m_bytes = CreateDecodedIndexStore(
            byteStoreSession,
            storageMode,
            byteCount,
            memoryCacheLimitBytes,
            error);
        if (m_bytes == nullptr) {
            return validation::AssignError(error, "failed to allocate decoded index cache");
        }
        if (!m_bytes->ResizeBytes(byteCount, error)) {
            Release();
            return false;
        }
        return true;
    }

    bool WriteRange(
        const std::uint64_t first,
        const std::span<const IndexType> values,
        std::string* error = nullptr) {
        if (values.empty()) {
            return true;
        }
        if (m_bytes == nullptr || first > m_count || values.size() > m_count - first) {
            return validation::AssignError(error, "decoded index cache write is outside the cache range");
        }
        const auto byteOffset = first * sizeof(IndexType);
        return m_bytes->WriteBytesAt(
            byteOffset,
            std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(values.data()),
                values.size() * sizeof(IndexType)),
            error);
    }

    bool Append(const std::span<const IndexType> values, std::string* error = nullptr) {
        if (!WriteRange(m_written, values, error)) {
            return false;
        }
        m_written += values.size();
        return true;
    }

    bool ReadRange(
        const std::uint64_t first,
        const std::size_t count,
        std::vector<IndexType>& output,
        std::string* error = nullptr) const {
        output.assign(count, 0);
        if (count == 0u) {
            return true;
        }
        if (m_bytes == nullptr || first > m_count || count > m_count - first) {
            validation::AssignError(error, "decoded index cache read is outside the cache range");
            output.clear();
            return false;
        }
        return m_bytes->Read(
            first * sizeof(IndexType),
            std::span<std::uint8_t>(
                reinterpret_cast<std::uint8_t*>(output.data()),
                output.size() * sizeof(IndexType)),
            error);
    }

    bool ReadRangeInto(
        const std::uint64_t first,
        const std::span<IndexType> output,
        std::string* error = nullptr) const {
        if (output.empty()) {
            return true;
        }
        if (m_bytes == nullptr || first > m_count || output.size() > m_count - first) {
            return validation::AssignError(error, "decoded index cache read is outside the cache range");
        }
        return m_bytes->Read(
            first * sizeof(IndexType),
            std::span<std::uint8_t>(
                reinterpret_cast<std::uint8_t*>(output.data()),
                output.size() * sizeof(IndexType)),
            error);
    }

    bool ReadScalar(
        const std::uint64_t index,
        IndexType& value,
        std::string* error = nullptr) const {
        value = 0;
        if (m_bytes == nullptr || index >= m_count) {
            return validation::AssignError(error, "decoded index cache scalar read is outside the cache range");
        }
        return m_bytes->Read(
            index * sizeof(IndexType),
            std::span<std::uint8_t>(
                reinterpret_cast<std::uint8_t*>(&value),
                sizeof(IndexType)),
            error);
    }

    [[nodiscard]] std::uint64_t Count() const noexcept { return m_count; }
    [[nodiscard]] std::uint64_t WrittenCount() const noexcept { return m_written; }
    [[nodiscard]] bool Complete() const noexcept { return m_bytes != nullptr && m_written == m_count; }
    [[nodiscard]] bytestore::IByteSource* ByteSource() const noexcept { return m_bytes.get(); }

    bool PrepareForRead(std::string* error = nullptr) {
        if (!Complete()) {
            return validation::AssignError(error, "decoded index cache is incomplete");
        }
        return m_bytes->Seal(error);
    }

    void Release() noexcept {
        if (m_bytes != nullptr) {
            m_bytes->Release();
        }
        m_bytes.reset();
        m_count = 0u;
        m_written = 0u;
    }

private:
    void MoveFrom(DecodedIndexCache&& other) noexcept {
        m_bytes = std::move(other.m_bytes);
        m_count = other.m_count;
        m_written = other.m_written;
        other.m_count = 0u;
        other.m_written = 0u;
    }

    std::shared_ptr<bytestore::IRandomAccessByteStore> m_bytes;
    std::uint64_t m_count{0u};
    std::uint64_t m_written{0u};
};

} // namespace datacodec

#endif
