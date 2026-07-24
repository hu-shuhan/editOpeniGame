#ifndef DATACODEC_CODEC_TOPOLOGY_TOPOLOGYFINGERPRINT_H
#define DATACODEC_CODEC_TOPOLOGY_TOPOLOGYFINGERPRINT_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstddef>
#include <cstdint>
#include <string>
namespace datacodec {

struct TopologyFingerprint {
    MeshType meshType{MeshType::PointSet};
    std::size_t pointCount{0};
    std::size_t cellCount{0};
    std::uint64_t hash{0};

    [[nodiscard]] bool operator==(const TopologyFingerprint& other) const noexcept {
        return meshType == other.meshType &&
            pointCount == other.pointCount &&
            cellCount == other.cellCount &&
            hash == other.hash;
    }

    [[nodiscard]] bool operator!=(const TopologyFingerprint& other) const noexcept {
        return !(*this == other);
    }
};

struct PointSpatialFingerprint {
    std::size_t pointCount{0u};
    ScalarType scalarType{ScalarType::Float64};
    std::uint64_t hash{0u};

    [[nodiscard]] bool operator==(const PointSpatialFingerprint& other) const noexcept {
        return pointCount == other.pointCount &&
            scalarType == other.scalarType &&
            hash == other.hash;
    }
};

class PointSpatialFingerprintBuilder {
public:
    [[nodiscard]] static PointSpatialFingerprint Build(const IEncodeAdapter& adapter) {
        PointSpatialFingerprint fingerprint;
        fingerprint.pointCount = adapter.GetNumberOfPoints();
        fingerprint.scalarType = adapter.GetPointScalarType();
        std::uint64_t hash = kFnvOffsetBasis;
        MixValue(hash, fingerprint.pointCount);
        MixValue(hash, static_cast<std::uint8_t>(fingerprint.scalarType));
        if (const auto* points = adapter.TryGetPointsF32(); points != nullptr) {
            MixArray(hash, points, fingerprint.pointCount * 3u);
        } else if (const auto* points = adapter.TryGetPointsF64(); points != nullptr) {
            MixArray(hash, points, fingerprint.pointCount * 3u);
        } else {
            for (std::size_t pointIndex = 0u; pointIndex < fingerprint.pointCount; ++pointIndex) {
                double point[3]{0.0, 0.0, 0.0};
                adapter.GetPoint(pointIndex, point);
                MixValue(hash, point[0]);
                MixValue(hash, point[1]);
                MixValue(hash, point[2]);
            }
        }
        fingerprint.hash = hash;
        return fingerprint;
    }

private:
    static constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
    static constexpr std::uint64_t kFnvPrime = 1099511628211ull;

    template<typename TValue>
    static void MixValue(std::uint64_t& hash, const TValue& value) {
        using Storage = detail::WireStorageTypeT<TValue>;
        const auto storage = detail::ToWireStorage(value);
        for (std::size_t byteIndex = 0u; byteIndex < sizeof(Storage); ++byteIndex) {
            const auto byte = static_cast<std::uint8_t>(
                (storage >> (byteIndex * 8u)) & Storage{0xFFu});
            hash ^= static_cast<std::uint64_t>(byte);
            hash *= kFnvPrime;
        }
    }

    template<typename TValue>
    static void MixArray(
        std::uint64_t& hash,
        const TValue* values,
        const std::size_t count) {
        MixValue(hash, count);
        for (std::size_t index = 0u; index < count; ++index) {
            MixValue(hash, values[index]);
        }
    }
};

class TopologyFingerprintBuilder {
public:
    static bool Build(
        const IEncodeAdapter& adapter,
        TopologyFingerprint& fingerprint,
        std::string* error = nullptr) {
        fingerprint = {};
        fingerprint.meshType = adapter.GetMeshType();
        fingerprint.pointCount = adapter.GetNumberOfPoints();
        fingerprint.cellCount = adapter.GetNumberOfCells();
        TopologyInputDescriptor descriptor;
        if (!adapter.DescribeTopology(descriptor, error)) {
            if (error != nullptr && error->empty()) {
                *error = "encode adapter failed to describe topology for fingerprinting";
            }
            return false;
        }
        TopologyView topologyView;
        if (!adapter.BuildTopologyView(topologyView)) {
            return validation::AssignError(
                error,
                "encode adapter failed to build topology view for fingerprinting");
        }
        if (descriptor.cellCount != fingerprint.cellCount ||
            descriptor.pointCount != fingerprint.pointCount ||
            topologyView.cellCount != fingerprint.cellCount ||
            topologyView.pointCount != fingerprint.pointCount) {
            return validation::AssignError(error, "topology fingerprint counts are inconsistent");
        }

        std::uint64_t hash = kFnvOffsetBasis;
        MixValue(hash, static_cast<std::uint32_t>(fingerprint.meshType));
        MixValue(hash, fingerprint.pointCount);
        MixValue(hash, fingerprint.cellCount);
        MixValue(hash, static_cast<std::uint8_t>(descriptor.cellSize));
        MixValue(hash, descriptor.fixedCellSize);
        MixValue(hash, static_cast<std::uint8_t>(descriptor.cellTypes));
        MixValue(hash, static_cast<std::uint8_t>(descriptor.cellPolynomialOrders));

        const auto connectivityCount = adapter.GetCellIdBufferSize();
        const auto* ids = adapter.GetCellIdBufferPtr();
        if (connectivityCount != 0u && ids == nullptr) {
            return validation::AssignError(error, "topology fingerprint connectivity is missing");
        }
        for (std::size_t index = 0u; index < connectivityCount; ++index) {
            if (static_cast<std::size_t>(ids[index]) >= fingerprint.pointCount) {
                return validation::AssignError(error, "topology fingerprint point id is out of range");
            }
        }
        MixArray(hash, ids, connectivityCount);
        if (descriptor.cellSize == TopologyCellSizeSource::Offsets) {
            const auto* offsets = adapter.GetCellIdOffsetPtr();
            if (offsets == nullptr) {
                return validation::AssignError(error, "topology fingerprint offsets are missing");
            }
            if (offsets[0] != 0u) {
                return validation::AssignError(error, "topology fingerprint offsets must start at zero");
            }
            for (std::size_t index = 0u; index < fingerprint.cellCount; ++index) {
                if (offsets[index] > offsets[index + 1u]) {
                    return validation::AssignError(error, "topology fingerprint offsets are not monotonic");
                }
            }
            if (static_cast<std::size_t>(offsets[fingerprint.cellCount]) != connectivityCount) {
                return validation::AssignError(error, "topology fingerprint offsets do not match connectivity");
            }
            MixArray(hash, offsets, fingerprint.cellCount + 1u);
        } else if (descriptor.cellSize == TopologyCellSizeSource::FixedCellSize &&
            (descriptor.fixedCellSize <= 0 ||
             !validation::CanMulSizeT(
                 fingerprint.cellCount,
                 static_cast<std::size_t>(descriptor.fixedCellSize)) ||
             connectivityCount != fingerprint.cellCount *
                 static_cast<std::size_t>(descriptor.fixedCellSize))) {
            return validation::AssignError(error, "topology fingerprint fixed cell size is inconsistent");
        }
        if (TopologyValueSourceProvidesStream(descriptor.cellTypes)) {
            MixValue(hash, adapter.GetNumberOfCells());
            for (std::size_t cellIndex = 0; cellIndex < adapter.GetNumberOfCells(); ++cellIndex) {
                IndexType cellType = 0;
                if (!adapter.ReadCellType(cellIndex, cellType)) {
                    return validation::AssignError(error, "topology fingerprint failed to read cell type");
                }
                MixValue(hash, cellType);
            }
        }
        if (TopologyValueSourceProvidesStream(descriptor.cellPolynomialOrders)) {
            MixValue(hash, adapter.GetNumberOfCells());
            for (std::size_t cellIndex = 0; cellIndex < adapter.GetNumberOfCells(); ++cellIndex) {
                std::uint16_t order = 0u;
                if (!adapter.ReadCellPolynomialOrder(cellIndex, order)) {
                    return validation::AssignError(
                        error,
                        "topology fingerprint failed to read cell polynomial order");
                }
                MixValue(hash, order);
            }
        }

        int axisSize[3]{0, 0, 0};
        if (adapter.IsStructuredMesh()) {
            if (!adapter.GetStructuredAxisSize(axisSize)) {
                return validation::AssignError(
                    error,
                    "topology fingerprint failed to read structured axis sizes");
            }
            MixArray(hash, axisSize, 3);
        }

        if (adapter.IsPolyhedronMesh()) {
            const auto faceCount = adapter.GetNumberOfFaces();
            const auto faceIdCount = adapter.GetFaceIdBufferSize();
            const auto cellFaceIdCount = adapter.GetCellFaceBufferSize();
            const auto* faceIds = adapter.GetFaceIdBufferPtr();
            const auto* faceOffsets = adapter.GetFaceIdOffsetPtr();
            const auto* cellFaceIds = adapter.GetCellFaceBufferPtr();
            const auto* cellFaceOffsets = adapter.GetCellFaceOffsetPtr();
            if (faceIds == nullptr || faceOffsets == nullptr ||
                cellFaceIds == nullptr || cellFaceOffsets == nullptr) {
                return validation::AssignError(error, "topology fingerprint polyhedron table is incomplete");
            }
            if (!ValidateOffsets(faceOffsets, faceCount, faceIdCount, error, "face-vertex") ||
                !ValidateOffsets(
                    cellFaceOffsets,
                    fingerprint.cellCount,
                    cellFaceIdCount,
                    error,
                    "cell-face")) {
                return false;
            }
            for (std::size_t index = 0u; index < faceIdCount; ++index) {
                if (static_cast<std::size_t>(faceIds[index]) >= fingerprint.pointCount) {
                    return validation::AssignError(
                        error,
                        "topology fingerprint polyhedron point id is out of range");
                }
            }
            for (std::size_t index = 0u; index < cellFaceIdCount; ++index) {
                if (static_cast<std::size_t>(cellFaceIds[index]) >= faceCount) {
                    return validation::AssignError(
                        error,
                        "topology fingerprint polyhedron face id is out of range");
                }
            }
            MixValue(hash, faceCount);
            MixArray(hash, faceIds, faceIdCount);
            MixArray(hash, faceOffsets, faceCount + 1u);
            MixArray(hash, cellFaceIds, cellFaceIdCount);
            MixArray(hash, cellFaceOffsets, fingerprint.cellCount + 1u);
        }

        fingerprint.hash = hash;
        return true;
    }

private:
    static constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
    static constexpr std::uint64_t kFnvPrime = 1099511628211ull;

    static bool ValidateOffsets(
        const IndexType* offsets,
        const std::size_t itemCount,
        const std::size_t valueCount,
        std::string* error,
        const char* label) {
        if (offsets[0] != 0u) {
            return validation::AssignError(
                error,
                std::string("topology fingerprint ") + label + " offsets must start at zero");
        }
        for (std::size_t index = 0u; index < itemCount; ++index) {
            if (offsets[index] > offsets[index + 1u]) {
                return validation::AssignError(
                    error,
                    std::string("topology fingerprint ") + label + " offsets are not monotonic");
            }
        }
        if (static_cast<std::size_t>(offsets[itemCount]) != valueCount) {
            return validation::AssignError(
                error,
                std::string("topology fingerprint ") + label + " offsets do not match values");
        }
        return true;
    }

    template<typename TValue>
    static void MixValue(std::uint64_t& hash, const TValue& value) {
        using Storage = detail::WireStorageTypeT<TValue>;
        const auto storage = detail::ToWireStorage(value);
        for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex) {
            const auto byte = static_cast<std::uint8_t>((storage >> (byteIndex * 8u)) & Storage{0xFFu});
            hash ^= static_cast<std::uint64_t>(byte);
            hash *= kFnvPrime;
        }
    }

    template<typename TValue>
    static void MixArray(std::uint64_t& hash, const TValue* values, const std::size_t count) {
        if (values == nullptr || count == 0) {
            MixValue(hash, count);
            return;
        }
        MixValue(hash, count);
        for (std::size_t index = 0; index < count; ++index) {
            MixValue(hash, values[index]);
        }
    }
};

} // namespace datacodec

#endif
