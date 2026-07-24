#ifndef DATACODEC_CODEC_TOPOLOGY_TOPOLOGYDECODECACHESINK_H
#define DATACODEC_CODEC_TOPOLOGY_TOPOLOGYDECODECACHESINK_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/Topology/Connectivity/ConnectivityTopologyTypes.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
namespace datacodec::topology {

class CacheTopologyDecodeSink final : public topocodec::IConnectivityTopologyDecodeSink {
public:
    CacheTopologyDecodeSink(
        DecodedTopologyCache& topology,
        bytestore::ByteStoreSession& byteStoreSession,
        const std::uint64_t memoryCacheLimitBytes = 0u,
        const DecodeStorageMode storageMode = DecodeStorageMode::Managed)
        : m_topology(topology),
          m_byteStoreSession(byteStoreSession),
          m_memoryCacheLimitBytes(memoryCacheLimitBytes),
          m_storageMode(storageMode) {}

    bool BeginConnectivityTopology(
        const std::size_t cellCount,
        const std::size_t connectivityCount,
        const bool hasOffsets,
        const bool hasCellTypes,
        const bool hasCellPolynomialOrders,
        std::string* error = nullptr) override {
        return m_topology.InitializeConnectivity(
            cellCount,
            connectivityCount,
            hasOffsets,
            hasCellTypes,
            hasCellPolynomialOrders,
            m_byteStoreSession,
            m_memoryCacheLimitBytes,
            m_storageMode,
            error);
    }

    bool WriteConnectivityRange(
        const std::size_t offset,
        const std::span<const IndexType> values,
        std::string* error = nullptr) override {
        return m_topology.connectivity != nullptr &&
            m_topology.connectivity->WriteBytesAt(
                static_cast<std::uint64_t>(offset) * sizeof(IndexType),
                std::span<const std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(values.data()),
                    values.size() * sizeof(IndexType)),
                error);
    }

    bool WriteOffsetsRange(
        const std::size_t offset,
        const std::span<const IndexType> values,
        std::string* error = nullptr) override {
        return m_topology.offsets != nullptr &&
            m_topology.offsets->WriteBytesAt(
                static_cast<std::uint64_t>(offset) * sizeof(IndexType),
                std::span<const std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(values.data()),
                    values.size() * sizeof(IndexType)),
                error);
    }

    bool WriteCellTypesRange(
        const std::size_t offset,
        const std::span<const IndexType> values,
        std::string* error = nullptr) override {
        return m_topology.cellTypes != nullptr &&
            m_topology.cellTypes->WriteBytesAt(
                static_cast<std::uint64_t>(offset) * sizeof(IndexType),
                std::span<const std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(values.data()),
                    values.size() * sizeof(IndexType)),
                error);
    }

    bool WriteCellPolynomialOrdersRange(
        const std::size_t offset,
        const std::span<const std::uint16_t> values,
        std::string* error = nullptr) override {
        return m_topology.cellPolynomialOrders != nullptr &&
            m_topology.cellPolynomialOrders->WriteBytesAt(
                static_cast<std::uint64_t>(offset) * sizeof(std::uint16_t),
                std::span<const std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(values.data()),
                    values.size() * sizeof(std::uint16_t)),
                error);
    }

    bool EndConnectivityTopology(std::string* error) override {
        return m_topology.PrepareForRead(error);
    }

private:
    DecodedTopologyCache& m_topology;
    bytestore::ByteStoreSession& m_byteStoreSession;
    std::uint64_t m_memoryCacheLimitBytes{0u};
    DecodeStorageMode m_storageMode{DecodeStorageMode::Managed};
};

} // namespace datacodec::topology

#endif
