#ifndef DATACODEC_CODEC_TOPOLOGY_CONNECTIVITY_CONNECTIVITYTOPOLOGYDECODEINPUTREADER_H
#define DATACODEC_CODEC_TOPOLOGY_CONNECTIVITY_CONNECTIVITYTOPOLOGYDECODEINPUTREADER_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecControlParams.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
namespace datacodec {
namespace topocodec {

inline std::size_t ConnectivityTopologyDecodeInputStreamIndex(
    const ConnectivityTopologyStreamKind kind) noexcept {
    return static_cast<std::size_t>(kind);
}

class ConnectivityTopologyDecodeInputReader final
    : public IConnectivityTopologyEncodedStreamReader {
public:
    enum class ByteStoreMode : std::uint8_t {
        None,
        Memory,
        Managed,
    };

    ConnectivityTopologyDecodeInputReader() = default;
    ConnectivityTopologyDecodeInputReader(const ConnectivityTopologyDecodeInputReader&) = delete;
    ConnectivityTopologyDecodeInputReader& operator=(const ConnectivityTopologyDecodeInputReader&) = delete;

    template<typename TStream>
    bool LoadFrom(
        TStream& stream,
        const ConnectivityTopologyEncodedMetadata& metadata,
        const CacheResources& runtime,
        bytestore::ByteStoreSession& byteStoreSession,
        std::string* error = nullptr) {
        return LoadFrom(
            stream,
            metadata,
            runtime,
            byteStoreSession,
            0u,
            DecodeStorageMode::Managed,
            error);
    }

    template<typename TStream>
    bool LoadFrom(
        TStream& stream,
        const ConnectivityTopologyEncodedMetadata& metadata,
        const CacheResources& runtime,
        bytestore::ByteStoreSession& byteStoreSession,
        const std::uint64_t memoryCacheLimitBytes,
        const DecodeStorageMode storageMode,
        std::string* error = nullptr) {
        Reset();
        m_metadata = metadata;
        std::array<std::uint64_t, 4> byteCounts{
            metadata.connectivityByteCount,
            metadata.cellSizeByteCount,
            metadata.cellPolynomialOrderByteCount,
            metadata.cellTypeByteCount,
        };
        std::uint64_t totalBytes = 0u;
        bool byteCountOverflow = false;
        for (const auto byteCount : byteCounts) {
            if (!validation::CanAddU64(totalBytes, byteCount)) {
                byteCountOverflow = true;
                totalBytes = std::numeric_limits<std::uint64_t>::max();
                break;
            }
            totalBytes = validation::SaturatingAddU64(totalBytes, byteCount);
        }
        if (storageMode == DecodeStorageMode::Memory &&
            (byteCountOverflow || memoryCacheLimitBytes == 0u || totalBytes > memoryCacheLimitBytes)) {
            Reset();
            return validation::AssignError(
                error,
                "connectivity topology decoded input streams exceed configured memory limit");
        }

        const bool useMemoryCache = storageMode == DecodeStorageMode::Memory;
        std::string prepareError;
        if (!PrepareStores(byteCounts, byteStoreSession, useMemoryCache, &prepareError)) {
            Reset();
            return validation::AssignError(
                error,
                prepareError.empty()
                    ? "failed to prepare connectivity topology decoded input streams"
                    : prepareError);
        }
        if (!CopyStream(stream, ConnectivityTopologyStreamKind::Connectivity, metadata.connectivityByteCount, runtime, error) ||
            !CopyStream(stream, ConnectivityTopologyStreamKind::CellSize, metadata.cellSizeByteCount, runtime, error) ||
            !CopyStream(stream, ConnectivityTopologyStreamKind::CellPolynomialOrder, metadata.cellPolynomialOrderByteCount, runtime, error) ||
            !CopyStream(stream, ConnectivityTopologyStreamKind::CellType, metadata.cellTypeByteCount, runtime, error)) {
            Reset();
            return false;
        }
        return true;
    }

    [[nodiscard]] const ConnectivityTopologyEncodedMetadata& Metadata() const noexcept override {
        return m_metadata;
    }

    [[nodiscard]] std::uint64_t StreamSize(
        const ConnectivityTopologyStreamKind kind) const noexcept override {
        const auto index = ConnectivityTopologyDecodeInputStreamIndex(kind);
        return index < m_sizes.size() ? m_sizes[index] : 0u;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        std::uint64_t bytes = 0u;
        for (const auto& stream : m_streams) {
            if (stream != nullptr) {
                bytes = validation::SaturatingAddU64(bytes, stream->ResidentSizeHint());
            }
        }
        return bytes;
    }

    [[nodiscard]] ByteStoreMode StoreMode() const noexcept { return m_storeMode; }

    [[nodiscard]] const char* ByteStoreModeName() const noexcept {
        switch (m_storeMode) {
            case ByteStoreMode::Memory:
                return "memory";
            case ByteStoreMode::Managed:
                return "managed";
            case ByteStoreMode::None:
            default:
                return "none";
        }
    }

    [[nodiscard]] std::span<const std::uint8_t> ContiguousStreamRange(
        const ConnectivityTopologyStreamKind kind,
        const std::uint64_t offset,
        const std::uint64_t byteCount) const noexcept override {
        if (m_storeMode != ByteStoreMode::Memory) {
            return {};
        }
        const auto index = ConnectivityTopologyDecodeInputStreamIndex(kind);
        if (index >= m_streams.size() || m_streams[index] == nullptr) {
            return {};
        }
        if (byteCount > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return {};
        }
        const auto bytes = m_streams[index]->ContiguousBytes();
        if (offset > bytes.size() ||
            byteCount > static_cast<std::uint64_t>(bytes.size() - static_cast<std::size_t>(offset))) {
            return {};
        }
        return bytes.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(byteCount));
    }

    ContiguousViewStatus PrepareContiguousStreamRange(
        const ConnectivityTopologyStreamKind kind,
        const std::uint64_t offset,
        const std::uint64_t byteCount,
        std::span<const std::uint8_t>& output,
        std::string* error = nullptr) const override {
        output = {};
        const auto index = ConnectivityTopologyDecodeInputStreamIndex(kind);
        if (index >= m_streams.size() || m_streams[index] == nullptr ||
            offset > m_sizes[index] || byteCount > m_sizes[index] - offset) {
            validation::AssignError(error, "connectivity topology contiguous stream range is invalid");
            return ContiguousViewStatus::Error;
        }
        if (m_storeMode != ByteStoreMode::Memory) {
            if (error != nullptr) { error->clear(); }
            return ContiguousViewStatus::Unavailable;
        }
        std::span<const std::uint8_t> streamBytes;
        const auto status = m_streams[index]->PrepareContiguousBytes(streamBytes, error);
        if (status != ContiguousViewStatus::Ready) {
            return status;
        }
        if (streamBytes.size() != m_sizes[index] ||
            byteCount > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            validation::AssignError(error, "connectivity topology contiguous stream size is invalid");
            return ContiguousViewStatus::Error;
        }
        output = streamBytes.subspan(
            static_cast<std::size_t>(offset),
            static_cast<std::size_t>(byteCount));
        return ContiguousViewStatus::Ready;
    }

    bool ReadStreamRange(
        const ConnectivityTopologyStreamKind kind,
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) const override {
        const auto index = ConnectivityTopologyDecodeInputStreamIndex(kind);
        if (index >= m_streams.size() || m_streams[index] == nullptr) {
            return validation::AssignError(error, "connectivity topology stream kind is invalid");
        }
        if (offset > m_sizes[index] || output.size() > m_sizes[index] - offset) {
            return validation::AssignError(
                error,
                "connectivity topology decoded stream range is out of bounds");
        }
        return m_streams[index]->Read(offset, output, error);
    }

    void Reset() noexcept {
        for (auto& stream : m_streams) {
            if (stream != nullptr) {
                stream->Release();
            }
            stream.reset();
        }
        m_sizes.fill(0u);
        m_metadata = {};
        m_storeMode = ByteStoreMode::None;
    }

private:
    bool PrepareStores(
        const std::array<std::uint64_t, 4>& byteCounts,
        bytestore::ByteStoreSession& byteStoreSession,
        const bool useMemoryCache,
        std::string* error) {
        m_storeMode = useMemoryCache ? ByteStoreMode::Memory : ByteStoreMode::Managed;
        for (std::size_t index = 0u; index < m_streams.size(); ++index) {
            auto cache = useMemoryCache
                ? byteStoreSession.CreateMemoryStore()
                : byteStoreSession.CreateManagedByteStore(
                    std::string("decoded_topology_stream_") + std::to_string(index),
                    error);
            if (cache == nullptr || !cache->ResizeBytes(byteCounts[index], error)) {
                return false;
            }
            m_streams[index] = std::move(cache);
            m_sizes[index] = byteCounts[index];
        }
        return true;
    }

    template<typename TStream>
    bool CopyStream(
        TStream& input,
        const ConnectivityTopologyStreamKind kind,
        const std::uint64_t byteCount,
        const CacheResources& runtime,
        std::string* error) {
        const auto index = ConnectivityTopologyDecodeInputStreamIndex(kind);
        if (index >= m_streams.size()) {
            return validation::AssignError(error, "connectivity topology stream kind is invalid");
        }
        auto cache = m_streams[index];
        if (cache == nullptr) {
            return validation::AssignError(error, "connectivity topology stream cache is missing");
        }

        std::uint64_t copied = 0u;
        while (copied < byteCount) {
            const auto currentBytes = static_cast<std::size_t>(
                std::min<std::uint64_t>(byteCount - copied, runtime.accessWindowBytes));
            auto windowLease = runtime.windowBudget.Acquire(currentBytes);
            auto scratchBuffer = runtime.scratchBytePool.Acquire(currentBytes);
            auto buffer = scratchBuffer.Span();
            if (!input.ReadBytes(buffer.data(), currentBytes, error) ||
                !cache->WriteBytesAt(copied, buffer, error)) {
                return false;
            }
            if (!validation::CheckedAddU64(
                    copied,
                    static_cast<std::uint64_t>(currentBytes),
                    copied,
                    "connectivity topology decoded input copied bytes",
                    error)) {
                return false;
            }
        }
        m_streams[index] = std::move(cache);
        m_sizes[index] = byteCount;
        return true;
    }

    ConnectivityTopologyEncodedMetadata m_metadata{};
    std::array<std::shared_ptr<bytestore::IRandomAccessByteStore>, 4> m_streams{};
    std::array<std::uint64_t, 4> m_sizes{};
    ByteStoreMode m_storeMode{ByteStoreMode::None};
};

} // namespace topocodec
} // namespace datacodec

#endif
