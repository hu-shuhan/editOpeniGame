#ifndef DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDATTRIBUTECACHESET_H
#define DATACODEC_RUNTIME_CACHE_DECODECACHE_DECODEDATTRIBUTECACHESET_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

inline std::size_t DecodeAttributeTupleBytes(const AttrStorageParams& meta) {
    std::size_t localValueSize = 0u;
    if (!TryParamSizeToSizeT(NumericArrayValueSize(meta), localValueSize)) {
        return 0u;
    }
    return static_cast<std::size_t>(std::max(meta.dimension, 0)) * localValueSize;
}

inline bool ValidateDecodedAttributeRange(
    const AttrStorageParams& meta,
    const std::size_t offset,
    const std::size_t count,
    const std::size_t byteSize,
    std::string* error = nullptr) {
    const auto tupleBytes = DecodeAttributeTupleBytes(meta);
    if (tupleBytes == 0u) {
        if (count == 0u && byteSize == 0u) {
            return true;
        }
        return validation::AssignError(error, "decoded attribute tuple size is invalid");
    }
    std::size_t localElementCount = 0u;
    if (!TryParamSizeToSizeT(meta.elementCount, localElementCount)) {
        return validation::AssignError(error, "decoded attribute metadata exceeds this platform size limit");
    }
    if (offset > localElementCount || count > localElementCount - offset) {
        return validation::AssignError(error, "decoded attribute range is out of bounds");
    }
    std::size_t expectedBytes = 0u;
    if (!validation::CheckedMulSizeT(
            count,
            tupleBytes,
            expectedBytes,
            "decoded attribute byte size",
            error)) {
        return false;
    }
    if (byteSize != expectedBytes) {
        return validation::AssignError(error, "decoded attribute byte size does not match range");
    }
    return true;
}

class DecodedAttributeCacheSet final {
public:
    enum class ByteStoreMode : std::uint8_t {
        None,
        Memory,
        Managed,
    };

    DecodedAttributeCacheSet() = default;
    DecodedAttributeCacheSet(const DecodedAttributeCacheSet&) = delete;
    DecodedAttributeCacheSet& operator=(const DecodedAttributeCacheSet&) = delete;

    DecodedAttributeCacheSet(DecodedAttributeCacheSet&& other) noexcept {
        MoveFrom(std::move(other));
    }

    DecodedAttributeCacheSet& operator=(DecodedAttributeCacheSet&& other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~DecodedAttributeCacheSet() { Reset(); }

    bool InitializeOwned(
        const CodecStorageParams& storageParams,
        std::string* error = nullptr) {
        return InitializeOwned(storageParams, 0u, error);
    }

    bool InitializeOwned(
        const CodecStorageParams& storageParams,
        const std::uint64_t memoryCacheLimitBytes,
        std::string* error = nullptr) {
        const auto storageMode = memoryCacheLimitBytes == 0u
            ? DecodeStorageMode::Managed
            : DecodeStorageMode::Memory;
        return InitializeOwned(storageParams, memoryCacheLimitBytes, storageMode, error);
    }

    bool InitializeOwned(
        const CodecStorageParams& storageParams,
        const std::uint64_t memoryCacheLimitBytes,
        const DecodeStorageMode storageMode,
        std::string* error = nullptr) {
        auto byteStoreSession = std::make_shared<bytestore::ByteStoreSession>();
        if (!Initialize(storageParams, *byteStoreSession, memoryCacheLimitBytes, storageMode, error)) {
            return false;
        }
        m_ownedByteStoreSession = std::move(byteStoreSession);
        return true;
    }

    bool Initialize(
        const CodecStorageParams& storageParams,
        bytestore::ByteStoreSession& byteStoreSession,
        std::string* error = nullptr) {
        return Initialize(storageParams, byteStoreSession, 0u, error);
    }

    bool Initialize(
        const CodecStorageParams& storageParams,
        bytestore::ByteStoreSession& byteStoreSession,
        const std::uint64_t memoryCacheLimitBytes,
        std::string* error = nullptr) {
        const auto storageMode = memoryCacheLimitBytes == 0u
            ? DecodeStorageMode::Managed
            : DecodeStorageMode::Memory;
        return Initialize(storageParams, byteStoreSession, memoryCacheLimitBytes, storageMode, error);
    }

    bool Initialize(
        const CodecStorageParams& storageParams,
        bytestore::ByteStoreSession& byteStoreSession,
        const std::uint64_t memoryCacheLimitBytes,
        const DecodeStorageMode storageMode,
        std::string* error = nullptr) {
        if (m_initialized || !m_fields.empty() || m_ownedByteStoreSession != nullptr) {
            Reset();
        }
        m_storageParams = storageParams;
        m_byteStoreSession = &byteStoreSession;
        m_memoryCacheLimitBytes = memoryCacheLimitBytes;
        m_allocatedBytes = 0u;
        m_fields.clear();
        m_fields.resize(storageParams.attrParams.size());
        std::vector<std::uint64_t> fieldByteSizes(storageParams.attrParams.size(), 0u);
        std::uint64_t totalBytes = 0u;
        bool byteCountOverflow = false;
        for (std::size_t index = 0; index < storageParams.attrParams.size(); ++index) {
            auto& field = m_fields[index];
            field.meta = storageParams.attrParams[index];
            std::size_t localElementCount = 0u;
            std::size_t localValueSize = 0u;
            if (!TryParamSizeToSizeT(field.meta.elementCount, localElementCount) ||
                !TryParamSizeToSizeT(NumericArrayValueSize(field.meta), localValueSize)) {
                validation::AssignError(error, "decoded attribute metadata exceeds this platform size limit");
                Reset();
                return false;
            }
            field.tupleBytes = static_cast<std::size_t>(std::max(field.meta.dimension, 0)) * localValueSize;
            const auto elementCount64 = static_cast<std::uint64_t>(localElementCount);
            const auto tupleBytes64 = static_cast<std::uint64_t>(field.tupleBytes);
            if (!validation::CanMulU64(elementCount64, tupleBytes64)) {
                byteCountOverflow = true;
                fieldByteSizes[index] = std::numeric_limits<std::uint64_t>::max();
            } else {
                fieldByteSizes[index] = elementCount64 * tupleBytes64;
            }
        }

        totalBytes = 0u;
        for (const auto fieldBytes : fieldByteSizes) {
            if (!validation::CanAddU64(totalBytes, fieldBytes)) {
                byteCountOverflow = true;
                totalBytes = std::numeric_limits<std::uint64_t>::max();
                break;
            }
            totalBytes = validation::SaturatingAddU64(totalBytes, fieldBytes);
        }
        if (storageMode == DecodeStorageMode::Memory) {
            if (byteCountOverflow || memoryCacheLimitBytes == 0u) {
                validation::AssignError(error, "decoded attribute memory cache configuration is invalid");
                Reset();
                return false;
            }
        }

        m_byteStoreMode = storageMode == DecodeStorageMode::Memory
            ? ByteStoreMode::Memory
            : ByteStoreMode::Managed;
        for (std::size_t index = 0; index < m_fields.size(); ++index) {
            m_fields[index].byteSize = fieldByteSizes[index];
        }

        m_initialized = true;
        return true;
    }

    void Reset() noexcept {
        for (auto& field : m_fields) {
            if (field.bytes != nullptr) {
                field.bytes->Release();
            }
        }
        m_fields.clear();
        m_storageParams = {};
        m_byteStoreSession = nullptr;
        m_memoryCacheLimitBytes = 0u;
        m_allocatedBytes = 0u;
        m_byteStoreMode = ByteStoreMode::None;
        if (m_ownedByteStoreSession != nullptr) {
            m_ownedByteStoreSession.reset();
        }
        m_initialized = false;
    }

    [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized; }
    [[nodiscard]] bool IsComplete() const noexcept {
        return m_initialized &&
            std::all_of(m_fields.begin(), m_fields.end(), [](const Field& field) { return field.complete; });
    }
    [[nodiscard]] const CodecStorageParams& StorageParams() const noexcept { return m_storageParams; }
    [[nodiscard]] std::size_t FieldCount() const noexcept { return m_fields.size(); }
    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept {
        std::uint64_t bytes = 0u;
        for (const auto& field : m_fields) {
            if (field.bytes != nullptr) {
                bytes = validation::SaturatingAddU64(bytes, field.bytes->ResidentSizeHint());
            }
        }
        return bytes;
    }
    [[nodiscard]] std::uint64_t AdapterBackedResidentSizeHint() const noexcept {
        std::uint64_t bytes = 0u;
        for (const auto& field : m_fields) {
            if (field.adapterBacked && field.bytes != nullptr) {
                bytes = validation::SaturatingAddU64(bytes, field.bytes->ResidentSizeHint());
            }
        }
        return bytes;
    }
    [[nodiscard]] const char* ByteStoreModeName() const noexcept {
        switch (m_byteStoreMode) {
            case ByteStoreMode::Memory:
                return "memory";
            case ByteStoreMode::Managed:
                return "managed";
            case ByteStoreMode::None:
            default:
                return "none";
        }
    }

    bool BindAttributeStore(
        const std::size_t attrIndex,
        std::shared_ptr<bytestore::IRandomAccessByteStore> bytes,
        const bool adapterBacked,
        std::string* error = nullptr) {
        if (!ValidateAttrIndex(attrIndex, error)) {
            return false;
        }
        if (bytes == nullptr) {
            return validation::AssignError(error, "decoded attribute cache received a null byte store");
        }
        auto& field = m_fields[attrIndex];
        if (field.bytes != nullptr && field.bytes != bytes) {
            return validation::AssignError(error, "decoded attribute cache field already has a byte store");
        }
        field.bytes = std::move(bytes);
        field.adapterBacked = adapterBacked;
        field.complete = false;
        return true;
    }

    [[nodiscard]] bool AdapterBacked(const std::size_t attrIndex) const noexcept {
        return attrIndex < m_fields.size() && m_fields[attrIndex].adapterBacked;
    }

    bool BeginAttribute(
        const std::size_t attrIndex,
        const AttrStorageParams& meta,
        std::string* error = nullptr) {
        if (!ValidateAttrIndex(attrIndex, error)) {
            return false;
        }
        auto& field = m_fields[attrIndex];
        field.meta = meta;
        std::size_t localElementCount = 0u;
        std::size_t localValueSize = 0u;
        if (!TryParamSizeToSizeT(meta.elementCount, localElementCount) ||
            !TryParamSizeToSizeT(NumericArrayValueSize(meta), localValueSize)) {
            return validation::AssignError(error, "decoded attribute metadata exceeds this platform size limit");
        }
        field.tupleBytes = static_cast<std::size_t>(std::max(meta.dimension, 0)) * localValueSize;
        field.complete = false;
        const auto totalBytes = static_cast<std::uint64_t>(localElementCount) *
            static_cast<std::uint64_t>(field.tupleBytes);
        if (field.bytes == nullptr) {
            if (m_byteStoreSession == nullptr) {
                return validation::AssignError(error, "decoded attribute cache has no byte store session");
            }
            if (m_byteStoreMode == ByteStoreMode::Memory &&
                (m_memoryCacheLimitBytes == 0u ||
                 totalBytes > m_memoryCacheLimitBytes - std::min(m_allocatedBytes, m_memoryCacheLimitBytes))) {
                return validation::AssignError(error, "requested decoded attribute memory cache exceeds configured memory limit");
            }
            field.bytes = m_byteStoreMode == ByteStoreMode::Memory
                ? m_byteStoreSession->CreateMemoryStore()
                : m_byteStoreSession->CreateManagedByteStore(
                    "decoded_attribute_" + std::to_string(attrIndex),
                    error);
            if (field.bytes == nullptr) {
                if (error != nullptr && error->empty()) {
                    validation::AssignError(error, "failed to allocate requested decoded attribute cache");
                }
                return false;
            }
            m_allocatedBytes = validation::SaturatingAddU64(m_allocatedBytes, totalBytes);
        }
        field.byteSize = totalBytes;
        return field.bytes->ResizeBytes(totalBytes, error);
    }

    bool WriteAttributeRange(
        const std::size_t attrIndex,
        const std::size_t offset,
        const std::size_t count,
        const void* data,
        const std::size_t byteSize,
        std::string* error = nullptr) {
        if (!ValidateAttrIndex(attrIndex, error)) {
            return false;
        }
        auto& field = m_fields[attrIndex];
        if (!ValidateDecodedAttributeRange(field.meta, offset, count, byteSize, error)) {
            return false;
        }
        if (byteSize == 0u) {
            return true;
        }
        if (data == nullptr) {
            return validation::AssignError(error, "decoded attribute write source is null");
        }
        return field.bytes != nullptr &&
            field.bytes->WriteBytesAt(
                static_cast<std::uint64_t>(offset) * static_cast<std::uint64_t>(field.tupleBytes),
                std::span<const std::uint8_t>(static_cast<const std::uint8_t*>(data), byteSize),
                error);
    }

    bool EndAttribute(const std::size_t attrIndex, std::string* error = nullptr) {
        if (!ValidateAttrIndex(attrIndex, error)) {
            return false;
        }
        auto& field = m_fields[attrIndex];
        if (field.bytes == nullptr || !field.bytes->Seal(error)) {
            return false;
        }
        field.complete = true;
        return true;
    }

    void ReleaseFieldBytes(const std::size_t attrIndex) noexcept {
        if (attrIndex >= m_fields.size()) {
            return;
        }
        auto& field = m_fields[attrIndex];
        if (field.bytes != nullptr) {
            field.bytes->Release();
        }
        m_allocatedBytes -= std::min(m_allocatedBytes, field.byteSize);
        field.bytes.reset();
        field.adapterBacked = false;
        field.complete = false;
    }

    bool ReadRange(
        const std::size_t attrIndex,
        const std::size_t offset,
        const std::size_t count,
        void* data,
        const std::size_t byteSize,
        std::string* error = nullptr) const {
        if (!ValidateCompleteAttrIndex(attrIndex, error)) {
            return false;
        }
        const auto& field = m_fields[attrIndex];
        if (!ValidateDecodedAttributeRange(field.meta, offset, count, byteSize, error)) {
            return false;
        }
        if (byteSize == 0u) {
            return true;
        }
        if (data == nullptr) {
            return validation::AssignError(error, "decoded attribute read target is null");
        }
        return field.bytes != nullptr &&
            field.bytes->Read(
                static_cast<std::uint64_t>(offset) * static_cast<std::uint64_t>(field.tupleBytes),
                std::span<std::uint8_t>(static_cast<std::uint8_t*>(data), byteSize),
                error);
    }

    [[nodiscard]] const AttrStorageParams* Meta(const std::size_t attrIndex) const noexcept {
        return attrIndex < m_fields.size() ? &m_fields[attrIndex].meta : nullptr;
    }

    [[nodiscard]] std::shared_ptr<bytestore::IRandomAccessByteStore> Bytes(
        const std::size_t attrIndex) const noexcept {
        return attrIndex < m_fields.size() ? m_fields[attrIndex].bytes : nullptr;
    }

    [[nodiscard]] bool Complete(const std::size_t attrIndex) const noexcept {
        return attrIndex < m_fields.size() && m_fields[attrIndex].complete;
    }

private:
    struct Field {
        AttrStorageParams meta;
        std::size_t tupleBytes{0u};
        std::uint64_t byteSize{0u};
        std::shared_ptr<bytestore::IRandomAccessByteStore> bytes;
        bool adapterBacked{false};
        bool complete{false};
    };

    bool ValidateAttrIndex(const std::size_t attrIndex, std::string* error) const {
        if (!m_initialized || attrIndex >= m_fields.size()) {
            return validation::AssignError(error, "decoded attribute cache index is out of range");
        }
        return true;
    }

    bool ValidateCompleteAttrIndex(const std::size_t attrIndex, std::string* error) const {
        if (!ValidateAttrIndex(attrIndex, error)) {
            return false;
        }
        if (!m_fields[attrIndex].complete) {
            return validation::AssignError(error, "decoded attribute cache field is not complete");
        }
        return true;
    }

    void MoveFrom(DecodedAttributeCacheSet&& other) noexcept {
        m_storageParams = std::move(other.m_storageParams);
        m_fields = std::move(other.m_fields);
        m_ownedByteStoreSession = std::move(other.m_ownedByteStoreSession);
        m_byteStoreSession = other.m_byteStoreSession;
        m_memoryCacheLimitBytes = other.m_memoryCacheLimitBytes;
        m_allocatedBytes = other.m_allocatedBytes;
        m_byteStoreMode = other.m_byteStoreMode;
        m_initialized = other.m_initialized;
        other.m_storageParams = {};
        other.m_byteStoreSession = nullptr;
        other.m_memoryCacheLimitBytes = 0u;
        other.m_allocatedBytes = 0u;
        other.m_fields.clear();
        other.m_byteStoreMode = ByteStoreMode::None;
        other.m_initialized = false;
    }

    CodecStorageParams m_storageParams;
    std::vector<Field> m_fields;
    std::shared_ptr<bytestore::ByteStoreSession> m_ownedByteStoreSession;
    bytestore::ByteStoreSession* m_byteStoreSession{nullptr};
    std::uint64_t m_memoryCacheLimitBytes{0u};
    std::uint64_t m_allocatedBytes{0u};
    ByteStoreMode m_byteStoreMode{ByteStoreMode::None};
    bool m_initialized{false};
};

} // namespace datacodec

#endif
