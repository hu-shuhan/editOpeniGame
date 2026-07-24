#ifndef DATACODEC_VALIDATION_TOPOLOGY_TOPOLOGYVALIDATOR_H
#define DATACODEC_VALIDATION_TOPOLOGY_TOPOLOGYVALIDATOR_H

#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedTopologyCache.h"
#include "DataCodec/Validation/Result/ValidationResult.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace datacodec::validation {

class TopologyValidator final {
public:
    [[nodiscard]] static ValidationResult ValidateDecodedCacheShape(
        const CodecStorageParams& params,
        const std::shared_ptr<DecodedTopologyCache>& topology) {
        const auto& topo = params.topoParams;
        if (topo.isStructured != 0u) {
            if (topology == nullptr ||
                !topology->complete ||
                topology->kind != DecodedTopologyCache::Kind::Structured) {
                return Failure(
                    "topology.cache.structured",
                    "decoded structured topology cache does not match params");
            }
            return Success();
        }
        if (topo.cellCount == 0u && topo.binaryCount == 0u) {
            return Success();
        }
        if (topology == nullptr || !topology->complete) {
            return Failure(
                "topology.cache.complete",
                "decoded topology cache is incomplete");
        }
        std::size_t expectedCells = 0u;
        std::size_t expectedConnectivity = 0u;
        if (!TryParamSizeToSizeT(topo.cellCount, expectedCells) ||
            !TryParamSizeToSizeT(topo.cellBufferSize, expectedConnectivity)) {
            return ValidationResult::Failure(
                CodecErrorCode::InvalidInput,
                ValidationDomain::Topology,
                "topology.params.size",
                "topology params exceed this platform size limit");
        }
        if (topo.isPolyhedron != 0u &&
            topology->kind != DecodedTopologyCache::Kind::Polyhedron) {
            return Failure(
                "topology.cache.kind",
                "decoded polyhedron topology cache does not match params");
        }
        if (topo.isPolyhedron != 0u &&
            (topology->polyhedron.cellCount != topo.cellCount ||
             topology->polyhedron.faceCount != topo.polyhedronFaceVertexCount ||
             topology->polyhedron.uniqueVertexIdCount != topo.polyhedronVertexCount ||
             topology->polyhedron.localFaceVertexIdCount != topo.cellBufferSize)) {
            return Failure(
                "topology.cache.count",
                "decoded polyhedron topology count does not match params");
        }
        if (topo.isPolyhedron == 0u &&
            topology->kind != DecodedTopologyCache::Kind::Connectivity) {
            return Failure(
                "topology.cache.kind",
                "decoded connectivity topology cache does not match params");
        }
        if (topo.isPolyhedron == 0u &&
            (topology->cellCount != expectedCells ||
             topology->connectivityCount != expectedConnectivity ||
             topology->hasCellTypes != (topo.hasCellTypes != 0u))) {
            return Failure(
                "topology.cache.count",
                "decoded connectivity topology count does not match params");
        }
        return Success();
    }

    [[nodiscard]] static ValidationResult ValidateReferenceState(
        const bool topologyBorrowed,
        const std::shared_ptr<DecodedTopologyCache>& topology) {
        if (!topologyBorrowed) {
            return ValidationResult::Success(
                ValidationDomain::Topology,
                "topology.reference.complete");
        }
        if (topology == nullptr || !topology->complete) {
            return ValidationResult::Failure(
                CodecErrorCode::InvalidTopology,
                ValidationDomain::Topology,
                "topology.reference.complete",
                "borrowed topology reference is incomplete");
        }
        return ValidationResult::Success(
            ValidationDomain::Topology,
            "topology.reference.complete");
    }

private:
    [[nodiscard]] static ValidationResult Success() {
        return ValidationResult::Success(
            ValidationDomain::Topology,
            "topology.cache.shape");
    }

    [[nodiscard]] static ValidationResult Failure(
        std::string rule,
        std::string message) {
        return ValidationResult::Failure(
            CodecErrorCode::DecodeFailure,
            ValidationDomain::Topology,
            std::move(rule),
            std::move(message));
    }
};

} // namespace datacodec::validation

#endif
