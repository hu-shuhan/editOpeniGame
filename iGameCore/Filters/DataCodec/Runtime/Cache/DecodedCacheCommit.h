#ifndef DATACODEC_RUNTIME_CACHE_DECODEDCACHECOMMIT_H
#define DATACODEC_RUNTIME_CACHE_DECODEDCACHECOMMIT_H

#include "DataCodec/API/Adapter/IDecodeAdapter.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedAttributeCacheSet.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Runtime/Cache/DecodedCacheReplay.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyEmit.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stop_token>
#include <string>
#include <vector>
namespace datacodec {

inline void ReleaseDecodedByteStore(
    std::shared_ptr<bytestore::IRandomAccessByteStore>& bytes) noexcept {
    if (bytes != nullptr) {
        bytes->Release();
    }
    bytes.reset();
}

inline bool ResolveAttributeCommitShape(
    const AttrStorageParams& meta,
    const std::size_t tupleBytes,
    std::size_t& elementCount,
    std::size_t& totalBytes,
    std::string* error = nullptr) {
    elementCount = 0u;
    totalBytes = 0u;
    if (!TryParamSizeToSizeT(meta.elementCount, elementCount)) {
        return validation::AssignError(error, "decoded attribute element count exceeds local size capacity");
    }
    if (!validation::CheckedMulSizeT(
            elementCount,
            tupleBytes,
            totalBytes,
            "decoded attribute byte count",
            error)) {
        return false;
    }
    return true;
}

inline bool CommitGeometryCache(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    DecodedGeometryCache& geometry,
    std::string* error = nullptr) {
    if (!geometry.complete || geometry.bytes == nullptr) {
        return true;
    }
    if (!adapter.BeginPoints(geometry.pointCount, geometry.dimension, error)) {
        return false;
    }
    if (!ReplayTypedDecodedCache<float>(
            runtime,
            *geometry.bytes,
            geometry.pointCount,
            geometry.dimension,
            [&](const std::size_t offset, const std::size_t count, const float* values) {
                return adapter.WritePointsRange(offset, count, values, error);
            },
            error)) {
        (void)adapter.EndPoints(error);
        return false;
    }
    if (!adapter.EndPoints(error)) {
        return false;
    }
    geometry.Release();
    return true;
}

inline bool CommitConnectivityTopologyCache(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    const DecodedTopologyCache& topology,
    DecodedTopologyCache* releaseTarget,
    std::string* error = nullptr) {
    if (topology.connectivity == nullptr ||
        topology.offsets == nullptr ||
        (topology.hasCellTypes && topology.cellTypes == nullptr) ||
        (topology.hasCellPolynomialOrders && topology.cellPolynomialOrders == nullptr)) {
        return validation::AssignError(error, "decoded topology cache is incomplete");
    }
    if (!adapter.BeginTopology(topology.cellCount, topology.connectivityCount, topology.hasOffsets, error)) {
        return false;
    }
    if (!ReplayTypedDecodedCache<IndexType>(
            runtime,
            *topology.connectivity,
            topology.connectivityCount,
            1u,
            [&](const std::size_t offset, const std::size_t count, const IndexType* values) {
                return adapter.WriteConnectivityRange(offset, values, count, error);
            },
            error)) {
        (void)adapter.EndTopology(error);
        return false;
    }
    if (releaseTarget != nullptr) {
        ReleaseDecodedByteStore(releaseTarget->connectivity);
    }
    if (topology.hasOffsets &&
        !ReplayTypedDecodedCache<IndexType>(
            runtime,
            *topology.offsets,
            topology.cellCount + 1u,
            1u,
            [&](const std::size_t offset, const std::size_t count, const IndexType* values) {
                return adapter.WriteOffsetsRange(offset, values, count, error);
            },
            error)) {
        (void)adapter.EndTopology(error);
        return false;
    }
    if (releaseTarget != nullptr) {
        ReleaseDecodedByteStore(releaseTarget->offsets);
    }
    if (topology.hasCellTypes &&
        !ReplayTypedDecodedCache<IndexType>(
            runtime,
            *topology.cellTypes,
            topology.cellCount,
            1u,
            [&](const std::size_t offset, const std::size_t count, const IndexType* values) {
                return adapter.WriteCellTypesRange(offset, values, count, error);
            },
            error)) {
        (void)adapter.EndTopology(error);
        return false;
    }
    if (releaseTarget != nullptr && topology.hasCellTypes) {
        ReleaseDecodedByteStore(releaseTarget->cellTypes);
    }
    if (topology.hasCellPolynomialOrders &&
        !ReplayTypedDecodedCache<std::uint16_t>(
            runtime,
            *topology.cellPolynomialOrders,
            topology.cellCount,
            1u,
            [&](const std::size_t offset, const std::size_t count, const std::uint16_t* values) {
                return adapter.WriteCellPolynomialOrdersRange(offset, values, count, error);
            },
            error)) {
        (void)adapter.EndTopology(error);
        return false;
    }
    if (releaseTarget != nullptr && topology.hasCellPolynomialOrders) {
        ReleaseDecodedByteStore(releaseTarget->cellPolynomialOrders);
    }
    return adapter.EndTopology(error);
}

inline bool CommitConnectivityTopologyCache(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    const DecodedTopologyCache& topology,
    std::string* error = nullptr) {
    return CommitConnectivityTopologyCache(adapter, runtime, topology, nullptr, error);
}

inline bool CommitConnectivityTopologyCacheAndRelease(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    DecodedTopologyCache& topology,
    std::string* error = nullptr) {
    return CommitConnectivityTopologyCache(adapter, runtime, topology, &topology, error);
}

inline bool CommitPolyhedronTopologyCache(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    const DecodedPolyhedronCache& polyhedron,
    DecodedPolyhedronCache* releaseTarget,
    std::string* error = nullptr) {
    if (!adapter.SupportsPolyhedronTopology()) {
        return validation::AssignError(error, "decode adapter does not support polyhedron topology");
    }
    polyhedron::PolyhedronTopologyStreamHeader header;
    header.cellCount = polyhedron.cellCount;
    header.faceCount = polyhedron.faceCount;
    header.uniqueVertexIdCount = polyhedron.uniqueVertexIdCount;
    header.localFaceVertexIdCount = polyhedron.localFaceVertexIdCount;

    if (!adapter.BeginPolyhedronTopology(static_cast<std::size_t>(header.cellCount), error)) {
        return false;
    }
    std::uint64_t peakBatchBytes = 0u;
    std::uint64_t batchCount = 0u;
    if (!polyhedron::EmitPolyhedronCacheToAdapter(
            runtime,
            adapter,
            header,
            polyhedron.uniqueVertexCounts,
            polyhedron.cellFaceCounts,
            polyhedron.faceVertexCounts,
            polyhedron.cellUniqueVertexIds,
            polyhedron.localFaceVertexIds,
            peakBatchBytes,
            batchCount,
            error)) {
        (void)adapter.EndPolyhedronTopology(error);
        return false;
    }
    if (releaseTarget != nullptr) {
        releaseTarget->Release();
    }
    return adapter.EndPolyhedronTopology(error);
}

inline bool CommitPolyhedronTopologyCache(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    const DecodedPolyhedronCache& polyhedron,
    std::string* error = nullptr) {
    return CommitPolyhedronTopologyCache(adapter, runtime, polyhedron, nullptr, error);
}

inline bool CommitPolyhedronTopologyCacheAndRelease(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    DecodedPolyhedronCache& polyhedron,
    std::string* error = nullptr) {
    return CommitPolyhedronTopologyCache(adapter, runtime, polyhedron, &polyhedron, error);
}

inline bool CommitTopologyCache(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    const DecodedTopologyCache& topology,
    std::string* error = nullptr) {
    switch (topology.kind) {
        case DecodedTopologyCache::Kind::Structured:
            return adapter.SetStructuredAxisSize(topology.structuredAxisSize.data(), error);
        case DecodedTopologyCache::Kind::Connectivity:
            return CommitConnectivityTopologyCache(adapter, runtime, topology, error);
        case DecodedTopologyCache::Kind::Polyhedron:
            return topology.polyhedron.complete &&
                CommitPolyhedronTopologyCache(adapter, runtime, topology.polyhedron, error);
        case DecodedTopologyCache::Kind::None:
            return true;
    }
    return true;
}

inline bool CommitTopologyCacheAndRelease(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    DecodedTopologyCache& topology,
    std::string* error = nullptr) {
    switch (topology.kind) {
        case DecodedTopologyCache::Kind::Structured:
            return adapter.SetStructuredAxisSize(topology.structuredAxisSize.data(), error);
        case DecodedTopologyCache::Kind::Connectivity:
            return CommitConnectivityTopologyCacheAndRelease(adapter, runtime, topology, error);
        case DecodedTopologyCache::Kind::Polyhedron:
            return topology.polyhedron.complete &&
                CommitPolyhedronTopologyCacheAndRelease(adapter, runtime, topology.polyhedron, error);
        case DecodedTopologyCache::Kind::None:
            return true;
    }
    return true;
}

struct AttributeCommitField {
    std::size_t attrIndex{0u};
    const AttrStorageParams* meta{nullptr};
    std::shared_ptr<bytestore::IRandomAccessByteStore> bytes;
    std::size_t tupleBytes{0u};
    std::size_t elementCount{0u};
    std::size_t totalBytes{0u};
    bool adapterBacked{false};
};

inline bool WriteAttributeCommitField(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    const AttributeCommitField& field,
    std::string* error = nullptr) {
    if (field.meta == nullptr || field.bytes == nullptr) {
        return validation::AssignError(error, "decoded attribute commit field is incomplete");
    }
    if (field.tupleBytes == 0u) {
        return true;
    }

    return ReplayTypedDecodedCache<std::uint8_t>(
        runtime,
        *field.bytes,
        field.elementCount,
        field.tupleBytes,
        [&](const std::size_t offset, const std::size_t count, const std::uint8_t* values) {
            std::size_t byteCount = 0u;
            if (!validation::CheckedMulSizeT(
                    count,
                    field.tupleBytes,
                    byteCount,
                    "decoded attribute replay byte count",
                    error)) {
                return false;
            }
            return adapter.WriteAttributeRange(
                field.attrIndex,
                offset,
                count,
                values,
                byteCount,
                error);
        },
        error);
}

inline bool CommitAttributeCacheFields(
    IDecodeAdapter& adapter,
    const CacheResources& runtime,
    DecodedAttributeCacheSet& attributes,
    const std::span<const std::size_t> attrIndices,
    std::string* error = nullptr,
    IParallelTaskRunner* parallelTaskRunner = nullptr,
    const std::size_t workerLimit = 0u,
    const std::stop_token stopToken = {}) {
    if (attrIndices.empty()) {
        return true;
    }
    if (!attributes.IsInitialized()) {
        return validation::AssignError(error, "decoded attribute cache set is not initialized");
    }

    std::vector<AttributeCommitField> fields;
    fields.reserve(attrIndices.size());
    for (const auto attrIndex : attrIndices) {
        if (attrIndex >= attributes.FieldCount() || !attributes.Complete(attrIndex)) {
            return validation::AssignError(error, "requested decoded attribute cache field is incomplete");
        }
        const auto* meta = attributes.Meta(attrIndex);
        auto bytes = attributes.Bytes(attrIndex);
        if (meta == nullptr || bytes == nullptr) {
            return validation::AssignError(error, "decoded attribute cache field is missing");
        }
        const auto tupleBytes = DecodeAttributeTupleBytes(*meta);
        std::size_t localElementCount = 0u;
        std::size_t totalBytes = 0u;
        if (!ResolveAttributeCommitShape(*meta, tupleBytes, localElementCount, totalBytes, error)) {
            return false;
        }
        fields.push_back(AttributeCommitField{
            .attrIndex = attrIndex,
            .meta = meta,
            .bytes = std::move(bytes),
            .tupleBytes = tupleBytes,
            .elementCount = localElementCount,
            .totalBytes = totalBytes,
            .adapterBacked = attributes.AdapterBacked(attrIndex),
        });
    }

    // Adapter 的容器扩容和原生数组分配保持串行
    for (const auto& field : fields) {
        if (stopToken.stop_requested()) {
            return validation::AssignError(error, "decoded attribute commit was cancelled");
        }
        if (!field.adapterBacked && !adapter.BeginAttribute(field.attrIndex, *field.meta, error)) {
            return false;
        }
    }

    std::atomic<bool> writeFailed{false};
    std::mutex writeErrorMutex;
    std::string writeError;
    const auto writeFields = [&](const std::size_t begin, const std::size_t end) {
        for (auto index = begin; index < end; ++index) {
            if (writeFailed.load(std::memory_order_acquire) || stopToken.stop_requested()) {
                return;
            }
            std::string localError;
            try {
                if (fields[index].adapterBacked) {
                    continue;
                }
                if (WriteAttributeCommitField(adapter, runtime, fields[index], &localError)) {
                    continue;
                }
            } catch (const std::exception& exception) {
                localError = std::string("decoded attribute commit failed: ") + exception.what();
            } catch (...) {
                localError = "decoded attribute commit failed with an unknown exception";
            }
            if (!writeFailed.exchange(true, std::memory_order_acq_rel)) {
                std::lock_guard<std::mutex> lock(writeErrorMutex);
                writeError = localError.empty()
                    ? "failed to write decoded attribute cache"
                    : std::move(localError);
            }
        }
    };

    const auto useParallelWrites = adapter.SupportsConcurrentAttributeRangeWrites() &&
        ShouldParallelizeRange(fields.size(), 2u, parallelTaskRunner, workerLimit);
    if (useParallelWrites) {
        ParallelForChunks(
            0u,
            fields.size(),
            writeFields,
            parallelTaskRunner,
            workerLimit,
            stopToken);
    } else {
        writeFields(0u, fields.size());
    }
    if (stopToken.stop_requested() && !writeFailed.load(std::memory_order_acquire)) {
        return validation::AssignError(error, "decoded attribute commit was cancelled");
    }
    if (writeFailed.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(writeErrorMutex);
        return validation::AssignError(error, writeError);
    }

    // AttributeSet 挂接保持串行，避免外部对象容器并发修改
    for (const auto& field : fields) {
        if (!adapter.EndAttribute(field.attrIndex, error)) {
            return false;
        }
    }
    return true;
}

} // namespace datacodec

#endif
