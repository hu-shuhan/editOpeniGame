#ifndef DATACODEC_TEST_ADAPTER_DATACODECTESTADAPTER_H
#define DATACODEC_TEST_ADAPTER_DATACODECTESTADAPTER_H

#include "DataCodec/API/Adapter/IDecodeAdapter.h"
#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/API/Adapter/IBlockTreeAdapter.h"
#include "DataCodec/Test/Data/DataCodecTestDataset.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace datacodec::test {

class TestEncodeAttributeView final : public IEncodeAttrView {
public:
    explicit TestEncodeAttributeView(const TestNumericField& field)
        : m_field(&field) {}

    [[nodiscard]] std::string GetName() const override { return m_field->name; }
    [[nodiscard]] DataType GetDataType() const override { return DataType::Float32; }
    [[nodiscard]] AttrRole GetRole() const override { return m_field->role; }
    [[nodiscard]] AttrAttachment GetAttachType() const override { return m_field->attachment; }
    [[nodiscard]] int GetComponentCount() const override {
        return static_cast<int>(m_field->componentCount);
    }
    [[nodiscard]] std::size_t GetElementCount() const override {
        return m_field->ElementCount();
    }
    [[nodiscard]] const void* TryGetRawPtr() const override {
        return m_field->values.empty() ? nullptr : m_field->values.data();
    }
    void GetTuple(const std::size_t index, double* output) const override {
        const auto begin = index * m_field->componentCount;
        for (std::size_t component = 0u; component < m_field->componentCount; ++component) {
            output[component] = static_cast<double>(m_field->values[begin + component]);
        }
    }

private:
    const TestNumericField* m_field{nullptr};
};

class TestEncodeAdapter final : public IEncodeAdapter {
public:
    explicit TestEncodeAdapter(const TestDataset& dataset)
        : m_dataset(&dataset) {
        m_pointFields.reserve(dataset.pointFields.size());
        for (const auto& field : dataset.pointFields) {
            m_pointFields.emplace_back(field);
        }
        m_cellFields.reserve(dataset.cellFields.size());
        for (const auto& field : dataset.cellFields) {
            m_cellFields.emplace_back(field);
        }
    }

    [[nodiscard]] MeshType GetMeshType() const override { return m_dataset->meshType; }
    [[nodiscard]] std::string GetName() const override { return m_dataset->name; }

    [[nodiscard]] std::size_t GetNumberOfPoints() const override {
        return m_dataset->PointCount();
    }
    void GetPoint(const std::size_t index, double output[3]) const override {
        const auto begin = index * 3u;
        for (std::size_t component = 0u; component < 3u; ++component) {
            output[component] = static_cast<double>(m_dataset->points[begin + component]);
        }
    }
    [[nodiscard]] ScalarType GetPointScalarType() const override {
        return ScalarType::Float32;
    }
    [[nodiscard]] const float* TryGetPointsF32() const override {
        return m_dataset->points.empty() ? nullptr : m_dataset->points.data();
    }

    bool DescribeTopology(
        TopologyInputDescriptor& output,
        std::string* = nullptr) const override {
        output = {};
        output.pointCount = GetNumberOfPoints();
        output.cellCount = GetNumberOfCells();
        output.connectivityCount = GetCellIdBufferSize();
        output.connectivity = output.connectivityCount == 0u
            ? TopologyValueSource::Unavailable
            : TopologyValueSource::CompactArray;
        output.offsets = m_dataset->cellOffsets.empty()
            ? TopologyValueSource::Unavailable
            : TopologyValueSource::CompactArray;
        output.cellSize = m_dataset->cellOffsets.empty()
            ? TopologyCellSizeSource::None
            : TopologyCellSizeSource::Offsets;
        output.cellTypes = m_dataset->cellTypes.empty()
            ? TopologyValueSource::Unavailable
            : TopologyValueSource::CompactArray;
        output.cellPolynomialOrders = TopologyValueSource::Unavailable;
        return true;
    }
    [[nodiscard]] std::size_t GetNumberOfCells() const override { return m_dataset->CellCount(); }
    [[nodiscard]] std::size_t GetCellIdBufferSize() const override {
        return m_dataset->cellConnectivity.size();
    }
    [[nodiscard]] const IndexType* GetCellIdBufferPtr() const override {
        return m_dataset->cellConnectivity.empty() ? nullptr : m_dataset->cellConnectivity.data();
    }
    [[nodiscard]] const IndexType* GetCellIdOffsetPtr() const override {
        return m_dataset->cellOffsets.empty() ? nullptr : m_dataset->cellOffsets.data();
    }
    [[nodiscard]] bool IsFixedCellSize() const override { return false; }
    [[nodiscard]] int GetFixedCellSize() const override { return -1; }
    [[nodiscard]] const IndexType* GetCellTypesPtr() const override {
        return m_dataset->cellTypes.empty() ? nullptr : m_dataset->cellTypes.data();
    }
    [[nodiscard]] std::size_t GetCellFaceBufferSize() const override { return 0u; }

    [[nodiscard]] std::size_t GetNumberOfPointAttrs() const override {
        return m_pointFields.size();
    }
    [[nodiscard]] const IEncodeAttrView& GetPointAttr(const std::size_t index) const override {
        return m_pointFields.at(index);
    }
    [[nodiscard]] std::size_t GetNumberOfCellAttrs() const override {
        return m_cellFields.size();
    }
    [[nodiscard]] const IEncodeAttrView& GetCellAttr(const std::size_t index) const override {
        return m_cellFields.at(index);
    }

    void ResetInput() override {}

private:
    const TestDataset* m_dataset{nullptr};
    std::vector<TestEncodeAttributeView> m_pointFields;
    std::vector<TestEncodeAttributeView> m_cellFields;
};

class TestBlockTreeAdapter final : public IBlockTreeAdapter {
public:
    explicit TestBlockTreeAdapter(
        const TestDataset& dataset,
        BlockPath leafPath = "leaf")
        : m_dataset(&dataset), m_leafPath(std::move(leafPath)) {}

    void EnumerateLeafPaths(
        const std::function<void(const BlockPath&)>& visitor) const override {
        visitor(m_leafPath);
    }

    [[nodiscard]] std::unique_ptr<IEncodeAdapter> GetLeaf(
        const BlockPath& path) const override {
        if (path != m_leafPath) {
            return nullptr;
        }
        return std::make_unique<TestEncodeAdapter>(*m_dataset);
    }

    [[nodiscard]] std::string GetRootName() const override {
        return m_dataset != nullptr ? m_dataset->name : std::string{};
    }

private:
    const TestDataset* m_dataset{nullptr};
    BlockPath m_leafPath;
};

struct DecodedTestAttribute {
    AttrStorageParams metadata;
    std::vector<std::uint8_t> bytes;
    bool complete{false};
};

class TestDecodeAdapter final : public IDecodeAdapter {
public:
    bool SetMeshType(const MeshType type, std::string* error = nullptr) override {
        if (type != MeshType::PointSet) {
            return AssignError(error, "test decode adapter expects a point set");
        }
        m_meshType = type;
        return true;
    }

    bool BeginPoints(
        const std::size_t count,
        const std::size_t dimension,
        std::string* error = nullptr) override {
        if (dimension != 3u) {
            return AssignError(error, "test decode adapter expects three-dimensional points");
        }
        m_pointDimension = dimension;
        m_points.assign(count * dimension, 0.0f);
        return true;
    }

    bool WritePointsRange(
        const std::size_t offset,
        const std::size_t count,
        const float* data,
        std::string* error = nullptr) override {
        if (data == nullptr || (offset + count) * m_pointDimension > m_points.size()) {
            return AssignError(error, "point range exceeds the test decode buffer");
        }
        std::copy_n(
            data,
            count * m_pointDimension,
            m_points.begin() + static_cast<std::ptrdiff_t>(offset * m_pointDimension));
        return true;
    }

    bool EndPoints(std::string* = nullptr) override { return true; }

    bool BeginTopology(
        const std::size_t cellCount,
        const std::size_t connectivityCount,
        const bool hasOffsets,
        std::string* = nullptr) override {
        m_connectivity.assign(connectivityCount, 0u);
        m_offsets.assign(hasOffsets ? cellCount + 1u : 0u, 0u);
        m_cellTypes.assign(cellCount, 0u);
        m_cellPolynomialOrders.assign(cellCount, 0u);
        return true;
    }

    bool WriteConnectivityRange(
        const std::size_t offset,
        const IndexType* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        return WriteRange(m_connectivity, offset, data, count, error, "connectivity");
    }

    bool WriteOffsetsRange(
        const std::size_t offset,
        const IndexType* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        return WriteRange(m_offsets, offset, data, count, error, "offset");
    }

    bool WriteCellTypesRange(
        const std::size_t offset,
        const IndexType* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        return WriteRange(m_cellTypes, offset, data, count, error, "cell type");
    }

    bool WriteCellPolynomialOrdersRange(
        const std::size_t offset,
        const std::uint16_t* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        return WriteRange(
            m_cellPolynomialOrders,
            offset,
            data,
            count,
            error,
            "cell polynomial order");
    }

    bool EndTopology(std::string* = nullptr) override { return true; }

    bool SetStructuredAxisSize(const int[3], std::string* error = nullptr) override {
        return AssignError(error, "test decode adapter does not accept structured topology");
    }

    bool BeginPolyhedronTopology(
        std::size_t,
        std::string* error = nullptr) override {
        return AssignError(error, "test decode adapter does not accept polyhedron topology");
    }

    bool WritePolyhedronCellBatch(
        std::size_t,
        const PolyhedronTopologyView&,
        std::string* error = nullptr) override {
        return AssignError(error, "test decode adapter does not accept polyhedron topology");
    }

    bool EndPolyhedronTopology(std::string* error = nullptr) override {
        return AssignError(error, "test decode adapter does not accept polyhedron topology");
    }

    bool BeginAttribute(
        const std::size_t attrIndex,
        const AttrStorageParams& metadata,
        std::string* error = nullptr) override {
        if (metadata.dimension <= 0) {
            return AssignError(error, "attribute dimension is invalid");
        }
        if (attrIndex >= m_attributes.size()) {
            m_attributes.resize(attrIndex + 1u);
        }
        auto& attribute = m_attributes[attrIndex];
        attribute = {};
        attribute.metadata = metadata;
        const auto elementCount = static_cast<std::size_t>(metadata.elementCount);
        const auto componentCount = static_cast<std::size_t>(metadata.dimension);
        const auto valueSize = static_cast<std::size_t>(metadata.valueSize);
        attribute.bytes.assign(elementCount * componentCount * valueSize, 0u);
        return true;
    }

    bool WriteAttributeRange(
        const std::size_t attrIndex,
        const std::size_t offset,
        const std::size_t count,
        const void* data,
        const std::size_t byteSize,
        std::string* error = nullptr) override {
        if (attrIndex >= m_attributes.size() || data == nullptr) {
            return AssignError(error, "attribute range has no destination or source");
        }
        auto& attribute = m_attributes[attrIndex];
        const auto tupleByteSize =
            static_cast<std::size_t>(attribute.metadata.dimension) *
            static_cast<std::size_t>(attribute.metadata.valueSize);
        const auto byteOffset = offset * tupleByteSize;
        if (byteSize != count * tupleByteSize ||
            byteOffset + byteSize > attribute.bytes.size()) {
            return AssignError(error, "attribute range exceeds the test decode buffer");
        }
        std::memcpy(attribute.bytes.data() + byteOffset, data, byteSize);
        return true;
    }

    bool EndAttribute(const std::size_t attrIndex, std::string* error = nullptr) override {
        if (attrIndex >= m_attributes.size()) {
            return AssignError(error, "attribute completion index is invalid");
        }
        m_attributes[attrIndex].complete = true;
        return true;
    }

    [[nodiscard]] std::uint64_t NativeResidentBytesHint() const override {
        std::uint64_t bytes = static_cast<std::uint64_t>(m_points.size() * sizeof(float));
        for (const auto& attribute : m_attributes) {
            bytes += static_cast<std::uint64_t>(attribute.bytes.size());
        }
        return bytes;
    }

    void ResetOutput() override {
        m_meshType = MeshType::PointSet;
        m_pointDimension = 0u;
        m_points.clear();
        m_connectivity.clear();
        m_offsets.clear();
        m_cellTypes.clear();
        m_cellPolynomialOrders.clear();
        m_attributes.clear();
        m_committed = false;
    }

    bool Commit(std::string* = nullptr) override {
        m_committed = true;
        return true;
    }

    [[nodiscard]] MeshType Mesh() const noexcept { return m_meshType; }
    [[nodiscard]] const std::vector<float>& Points() const noexcept { return m_points; }
    [[nodiscard]] const std::vector<DecodedTestAttribute>& Attributes() const noexcept {
        return m_attributes;
    }
    [[nodiscard]] bool Committed() const noexcept { return m_committed; }

private:
    static bool AssignError(std::string* error, std::string message) {
        if (error != nullptr) {
            *error = std::move(message);
        }
        return false;
    }

    template<typename TValue>
    static bool WriteRange(
        std::vector<TValue>& destination,
        const std::size_t offset,
        const TValue* data,
        const std::size_t count,
        std::string* error,
        const char* name) {
        if (data == nullptr || offset + count > destination.size()) {
            return AssignError(error, std::string(name) + " range exceeds the test decode buffer");
        }
        std::copy_n(
            data,
            count,
            destination.begin() + static_cast<std::ptrdiff_t>(offset));
        return true;
    }

    MeshType m_meshType{MeshType::PointSet};
    std::size_t m_pointDimension{0u};
    std::vector<float> m_points;
    std::vector<IndexType> m_connectivity;
    std::vector<IndexType> m_offsets;
    std::vector<IndexType> m_cellTypes;
    std::vector<std::uint16_t> m_cellPolynomialOrders;
    std::vector<DecodedTestAttribute> m_attributes;
    bool m_committed{false};
};

} // datacodec::test命名空间

#endif
