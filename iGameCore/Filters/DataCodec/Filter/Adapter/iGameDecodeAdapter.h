#ifndef iGameDataCodeciGameDecodeAdapter_h
#define iGameDataCodeciGameDecodeAdapter_h

#include "DataCodec/API/Adapter/IDecodeAdapter.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Filter/Adapter/iGameCellTypeMapping.h"
#include "DataCodec/Common/Views/TopologyViews.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameCellArray.h"
#include "iGameCellType.h"
#include "iGameFlatArray.h"
#include "iGameFaceTable.h"
#include "iGameIdArray.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

class iGameDecodeAdapter final : public ::datacodec::IDecodeAdapter {
public:
    using IndexType = ::datacodec::IndexType;
    using MeshType = ::datacodec::MeshType;
    using PolyhedronTopologyView = ::datacodec::PolyhedronTopologyView;
    using AttrStorageParams = ::datacodec::AttrStorageParams;
    using DataType = ::datacodec::DataType;
    using AttrRole = ::datacodec::AttrRole;
    using AttrAttachment = ::datacodec::AttrAttachment;
    using CellTypeRaw = ::datacodec::CellTypeRaw;
    using CellTypeCodecEntry = ::datacodec::CellTypeCodecEntry;
    using CellTypeMappingMode = ::datacodec::CellTypeMappingMode;
    using CellTypeFamilyCode = ::datacodec::CellTypeFamilyCode;
    using CellTypeLocalCode = ::datacodec::CellTypeLocalCode;

    iGameDecodeAdapter() = default;
    explicit iGameDecodeAdapter(DataObject::Pointer output) : m_output(std::move(output)) {}

    // 为解码后的 mesh type 创建目标原生网格对象
    bool SetMeshType(MeshType type, std::string* error = nullptr) override {
        ReleaseOutputState();
        m_meshType = type;
        m_output = DataObject::CreateDataObject(ToNativeMeshType(type));
        if (m_output == nullptr) {
            return Fail(error, "failed to create iGame output object");
        }
        return true;
    }

    // 开始写入点坐标 range
    bool BeginPoints(
        const std::size_t count,
        const std::size_t dimension,
        std::string* error = nullptr) override {
        const auto pointSet = DynamicCast<PointSet>(m_output);
        if (pointSet == nullptr) {
            return Fail(error, "points can only be written to point-set compatible output");
        }
        if (dimension != 3u) {
            return Fail(error, "iGame point output requires 3 components");
        }

        m_pointDimension = dimension;
        m_pendingPointCount = count;
        m_pendingPoints = Points::New();
        if (m_pendingPoints == nullptr) {
            return Fail(error, "failed to allocate iGame points");
        }
        m_pendingPoints->Resize(count);
        pointSet->SetPoints(m_pendingPoints);
        m_nativeResidentBytes = ::datacodec::validation::SaturatingAddU64(
            m_nativeResidentBytes,
            ::datacodec::validation::SaturatingMulU64(
                static_cast<std::uint64_t>(count),
                ::datacodec::validation::SaturatingMulU64(static_cast<std::uint64_t>(dimension), sizeof(float))));
        return true;
    }

    // 写入点坐标 range
    bool WritePointsRange(
        const std::size_t offset,
        const std::size_t count,
        const float* data,
        std::string* error = nullptr) override {
        if (count == 0u) {
            return true;
        }
        if (m_pendingPoints == nullptr || data == nullptr) {
            return Fail(error, "point range write requires initialized points and input data");
        }
        if (offset > m_pendingPointCount || count > m_pendingPointCount - offset) {
            return Fail(error, "point range write is outside the target points");
        }
        constexpr std::size_t componentCount = 3u;
        std::size_t pointOffset = 0u;
        std::size_t tupleBytes = 0u;
        std::size_t byteCount = 0u;
        if (!::datacodec::validation::CheckedMulSizeT(
                offset,
                componentCount,
                pointOffset,
                "point range write offset",
                error) ||
            !::datacodec::validation::CheckedMulSizeT(
                componentCount,
                sizeof(float),
                tupleBytes,
                "point range write tuple bytes",
                error) ||
            !::datacodec::validation::CheckedMulSizeT(
                count,
                tupleBytes,
                byteCount,
                "point range write byte count",
                error)) {
            return false;
        }
        std::memcpy(
            m_pendingPoints->RawPointer() + pointOffset,
            data,
            byteCount);
        return true;
    }

    // 结束点坐标 range 写入
    bool EndPoints(std::string* = nullptr) override {
        m_pendingPoints = nullptr;
        m_pendingPointCount = 0u;
        m_pointDimension = 3u;
        return true;
    }

    // 开始写入普通拓扑 range
    bool BeginTopology(
        const std::size_t cellCount,
        const std::size_t connectivityCount,
        const bool hasOffsets,
        std::string* error = nullptr) override {
        m_pendingCellCount = cellCount;
        m_pendingConnectivityCount = connectivityCount;
        m_pendingConnectivityIds = IdArray::New();
        if (m_pendingConnectivityIds == nullptr) {
            return Fail(error, "failed to allocate native connectivity array");
        }
        m_pendingConnectivityIds->SetNumberOfIds(connectivityCount);
        m_pendingOffsets = nullptr;
        if (hasOffsets) {
            m_pendingOffsets = UnsignedIntArray::New();
            if (m_pendingOffsets == nullptr) {
                return Fail(error, "failed to allocate native topology offset array");
            }
            m_pendingOffsets->Resize(cellCount + 1u);
        }
        m_nativeResidentBytes = ::datacodec::validation::SaturatingAddU64(
            m_nativeResidentBytes,
            ::datacodec::validation::SaturatingMulU64(static_cast<std::uint64_t>(connectivityCount), sizeof(IndexType)));
        if (hasOffsets) {
            m_nativeResidentBytes = ::datacodec::validation::SaturatingAddU64(
                m_nativeResidentBytes,
                ::datacodec::validation::SaturatingMulU64(
                    ::datacodec::validation::SaturatingAddU64(static_cast<std::uint64_t>(cellCount), 1u),
                    sizeof(IndexType)));
        }
        m_cellArray = nullptr;
        m_cellTypes = nullptr;
        m_cellPolynomialOrders.clear();
        return true;
    }

    // 写入 connectivity range
    bool WriteConnectivityRange(
        const std::size_t offset,
        const IndexType* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        if (count == 0u) {
            return true;
        }
        if (data == nullptr) {
            return Fail(error, "connectivity range write requires input data");
        }
        if (m_pendingConnectivityIds == nullptr ||
            offset > m_pendingConnectivityCount ||
            count > m_pendingConnectivityCount - offset) {
            return Fail(error, "connectivity range write is outside the target buffer");
        }
        auto* target = m_pendingConnectivityIds->RawPointer() + offset;
        std::copy(data, data + count, target);
        return true;
    }

    // 写入 offsets range
    bool WriteOffsetsRange(
        const std::size_t offset,
        const IndexType* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        if (count == 0u) {
            return true;
        }
        if (data == nullptr) {
            return Fail(error, "offset range write requires input data");
        }
        if (m_pendingOffsets == nullptr ||
            offset > m_pendingOffsets->GetNumberOfValues() ||
            count > m_pendingOffsets->GetNumberOfValues() - offset) {
            return Fail(error, "offset range write is outside the target buffer");
        }
        std::copy(data, data + count, m_pendingOffsets->RawPointer() + offset);
        return true;
    }

    // 写入 cell type range
    bool WriteCellTypesRange(
        const std::size_t offset,
        const IndexType* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        if (count == 0u) {
            return true;
        }
        if (data == nullptr) {
            return Fail(error, "cell type range write requires input data");
        }
        if (offset > m_pendingCellCount || count > m_pendingCellCount - offset) {
            return Fail(error, "cell type range write is outside the target cells");
        }
        if (m_cellTypes == nullptr) {
            m_cellTypes = UnsignedIntArray::New();
            if (m_cellTypes == nullptr) {
                return Fail(error, "failed to allocate cell type array");
            }
            m_cellTypes->Resize(m_pendingCellCount);
            std::fill(m_cellTypes->RawPointer(), m_cellTypes->RawPointer() + m_cellTypes->GetNumberOfValues(), 0u);
            m_nativeResidentBytes = ::datacodec::validation::SaturatingAddU64(
                m_nativeResidentBytes,
                ::datacodec::validation::SaturatingMulU64(
                    static_cast<std::uint64_t>(m_cellTypes->GetNumberOfValues()),
                    sizeof(unsigned int)));
        }
        const auto targetCount = m_cellTypes->GetNumberOfValues();
        if (offset >= targetCount) {
            return Fail(error, "cell type range offset is outside the target array");
        }
        auto* target = m_cellTypes->RawPointer() + offset;
        for (std::size_t index = 0; index < count; ++index) {
            target[index] = static_cast<unsigned int>(data[index]);
        }
        return true;
    }

    // 写入 cell polynomial order range
    bool WriteCellPolynomialOrdersRange(
        const std::size_t offset,
        const std::uint16_t* data,
        const std::size_t count,
        std::string* error = nullptr) override {
        if (count == 0u) {
            return true;
        }
        if (data == nullptr) {
            return Fail(error, "cell polynomial order range write requires input data");
        }
        if (offset > m_pendingCellCount || count > m_pendingCellCount - offset) {
            return Fail(error, "cell polynomial order range write is outside the target cells");
        }
        if (m_cellPolynomialOrders.empty()) {
            m_cellPolynomialOrders.resize(m_pendingCellCount, 0);
            m_nativeResidentBytes = ::datacodec::validation::SaturatingAddU64(
                m_nativeResidentBytes,
                ::datacodec::validation::SaturatingMulU64(
                    static_cast<std::uint64_t>(m_cellPolynomialOrders.size()),
                    sizeof(std::uint16_t)));
        }
        std::copy(data, data + count, m_cellPolynomialOrders.begin() + static_cast<std::ptrdiff_t>(offset));
        return true;
    }

    // 结束普通拓扑 range 写入
    bool EndTopology(std::string* error = nullptr) override {
        if (m_pendingCellCount != 0u &&
            (m_pendingConnectivityIds == nullptr || m_pendingConnectivityCount == 0u)) {
            return Fail(error, "topology commit requires connectivity data");
        }
        m_cellArray = CellArray::New();
        if (m_pendingCellCount != 0u && m_cellArray == nullptr) {
            return Fail(error, "failed to build iGame cell array");
        }
        if (m_cellArray != nullptr && m_pendingCellCount != 0u) {
            if (m_pendingOffsets != nullptr) {
                m_cellArray->SetData(m_pendingConnectivityIds, m_pendingOffsets);
            } else {
                const auto fixedCellSize = static_cast<int>(
                    m_pendingConnectivityCount / m_pendingCellCount);
                if (fixedCellSize <= 0) {
                    return Fail(error, "topology commit has an invalid fixed cell size");
                }
                m_cellArray->SetData(m_pendingConnectivityIds, fixedCellSize);
            }
        }
        CommitCellArray();
        m_pendingConnectivityIds = nullptr;
        m_pendingOffsets = nullptr;
        m_pendingConnectivityCount = 0u;
        m_pendingCellCount = 0u;
        return true;
    }

    // 写入 structured mesh 的轴尺寸
    bool SetStructuredAxisSize(const int size[3], std::string* error = nullptr) override {
        const auto structuredMesh = DynamicCast<StructuredMesh>(m_output);
        if (structuredMesh == nullptr || size == nullptr) {
            return Fail(error, "structured axis size requires structured output and input size");
        }

        igIndex axisSize[3]{
            static_cast<igIndex>(size[0]),
            static_cast<igIndex>(size[1]),
            static_cast<igIndex>(size[2]),
        };
        structuredMesh->SetDimensionSize(axisSize);
        structuredMesh->GenStructuredCellConnectivities();
        return true;
    }

    // 解析原生 raw type 的编码信息
    bool ResolveCellType(const CellTypeRaw rawType, CellTypeCodecEntry& entry) const override {
        return m_cellTypeMapping.ResolveCellType(rawType, entry);
    }

    // 返回 cell type 映射模式
    CellTypeMappingMode GetCellTypeMappingMode() const override {
        return m_cellTypeMapping.GetCellTypeMappingMode();
    }

    // 解析 polynomial-order-dependent cell 的精确 size
    bool ResolveCellSizeFromPolynomialOrder(const CellTypeRaw rawType, const std::uint16_t order, int& size) const override {
        return m_cellTypeMapping.ResolveCellSizeFromPolynomialOrder(rawType, order, size);
    }

    // 编码 family-local cell type token
    bool EncodeCellTypeFamilyLocal(
        const CellTypeRaw rawType,
        CellTypeFamilyCode& familyCode,
        CellTypeLocalCode& familyLocalCode) const override {
        return m_cellTypeMapping.EncodeCellTypeFamilyLocal(rawType, familyCode, familyLocalCode);
    }

    // 反解 family-local cell type token
    bool DecodeCellTypeFamilyLocal(
        const CellTypeFamilyCode familyCode,
        const CellTypeLocalCode familyLocalCode,
        CellTypeRaw& rawType) const override {
        return m_cellTypeMapping.DecodeCellTypeFamilyLocal(familyCode, familyLocalCode, rawType);
    }

    // 编码 polynomial-order-dependent cell 的局部 order token
    bool EncodeCellPolynomialOrderLocal(
        const CellTypeRaw rawType,
        const std::uint16_t order,
        CellTypeLocalCode& localOrder) const override {
        return m_cellTypeMapping.EncodeCellPolynomialOrderLocal(rawType, order, localOrder);
    }

    // 反解 polynomial-order-dependent cell 的局部 order token
    bool DecodeCellPolynomialOrderLocal(
        const CellTypeRaw rawType,
        const CellTypeLocalCode localOrder,
        std::uint16_t& order) const override {
        return m_cellTypeMapping.DecodeCellPolynomialOrderLocal(rawType, localOrder, order);
    }

    // 判断当前目标 mesh 是否支持 polyhedron 组装
    bool SupportsPolyhedronTopology() const override {
        return m_meshType == MeshType::VolumeMesh || m_meshType == MeshType::UnstructuredMesh ||
            m_meshType == MeshType::PolyhedronMesh;
    }

    // 开始按 chunk 写入 polyhedron 拓扑
    bool BeginPolyhedronTopology(const std::size_t cellCount, std::string* error = nullptr) override {
        if (!SupportsPolyhedronTopology()) {
            return Fail(error, "target mesh does not support polyhedron topology");
        }
        m_polyCellCount = cellCount;
        m_polyFaceTable = nullptr;
        m_polyCellFaces = nullptr;
        m_polyCells = nullptr;
        m_polyCellTypes = nullptr;
        if (m_meshType == MeshType::VolumeMesh) {
            m_polyFaceTable = FaceTable::New();
            m_polyCellFaces = CellArray::New();
            if (m_polyFaceTable == nullptr || m_polyCellFaces == nullptr) {
                return Fail(error, "failed to allocate volume polyhedron buffers");
            }
            return true;
        }
        m_polyCells = CellArray::New();
        m_polyCellTypes = UnsignedIntArray::New();
        if (m_polyCells == nullptr || m_polyCellTypes == nullptr) {
            return Fail(error, "failed to allocate unstructured polyhedron buffers");
        }
        m_polyCellTypes->Resize(cellCount);
        std::fill(
            m_polyCellTypes->RawPointer(),
            m_polyCellTypes->RawPointer() + cellCount,
            static_cast<unsigned int>(IG_POLYHEDRON));
        m_nativeResidentBytes = ::datacodec::validation::SaturatingAddU64(
            m_nativeResidentBytes,
            ::datacodec::validation::SaturatingMulU64(static_cast<std::uint64_t>(cellCount), sizeof(unsigned int)));
        return true;
    }

    // 写入一段 polyhedron 局部拓扑
    bool WritePolyhedronCellBatch(
        const std::size_t firstCell,
        const PolyhedronTopologyView& batch,
        std::string* error = nullptr) override {
        if (!SupportsPolyhedronTopology() || !HasPolyhedronData(batch)) {
            return Fail(error, "polyhedron batch requires supported target and valid batch data");
        }
        if (firstCell >= m_polyCellCount && m_polyCellCount != 0u) {
            return Fail(error, "polyhedron batch first cell is outside the target cell count");
        }
        if (m_meshType == MeshType::VolumeMesh) {
            AppendVolumePolyhedronChunk(batch);
            return true;
        }
        AppendUnstructuredPolyhedronChunk(batch);
        return true;
    }

    // 结束 polyhedron chunk 写入
    bool EndPolyhedronTopology(std::string* error = nullptr) override {
        if (!SupportsPolyhedronTopology()) {
            return Fail(error, "target mesh does not support polyhedron topology");
        }
        if (m_meshType == MeshType::VolumeMesh) {
            const auto volumeMesh = DynamicCast<VolumeMesh>(m_output);
            if (volumeMesh != nullptr && m_polyFaceTable != nullptr && m_polyCellFaces != nullptr) {
                volumeMesh->InitVolumesWithPolyhedron(m_polyFaceTable->GetOutput(), m_polyCellFaces);
                m_cellArray = volumeMesh->GetCellArray();
                m_cellTypes = nullptr;
                m_cellPolynomialOrders.clear();
            } else {
                return Fail(error, "failed to commit volume polyhedron topology");
            }
        } else if (m_polyCells != nullptr && m_polyCellTypes != nullptr) {
            m_cellArray = m_polyCells;
            m_cellTypes = m_polyCellTypes;
            m_cellPolynomialOrders.clear();
            CommitCellArray();
        } else {
            return Fail(error, "failed to commit unstructured polyhedron topology");
        }
        m_polyFaceTable = nullptr;
        m_polyCellFaces = nullptr;
        m_polyCells = nullptr;
        m_polyCellTypes = nullptr;
        m_polyCellCount = 0u;
        return true;
    }

    // 开始写入属性 range
    bool BeginAttribute(
        const std::size_t attrIndex,
        const AttrStorageParams& meta,
        std::string* error = nullptr) override {
        if (m_output == nullptr || m_output->GetAttributeSet() == nullptr) {
            return Fail(error, "attribute write requires an output object with attribute set");
        }

        const auto array = CreateArray(meta.dataType);
        if (array == nullptr) {
            return Fail(error, "failed to create attribute array");
        }

        array->SetDimension(std::max(meta.dimension, 1));
        array->SetName(meta.name);
        std::size_t localElementCount = 0u;
        if (!::datacodec::TryParamSizeToSizeT(meta.elementCount, localElementCount)) {
            return Fail(error, "attribute element count exceeds this platform size limit");
        }
        array->Resize(localElementCount);
        m_nativeResidentBytes = ::datacodec::validation::SaturatingAddU64(
            m_nativeResidentBytes,
            ::datacodec::validation::SaturatingMulU64(
                static_cast<std::uint64_t>(localElementCount),
                ::datacodec::validation::SaturatingMulU64(
                    static_cast<std::uint64_t>(std::max(meta.dimension, 1)),
                    ::datacodec::DataTypeSize(meta.dataType))));
        if (attrIndex >= m_pendingAttributes.size()) {
            m_pendingAttributes.resize(attrIndex + 1u);
            m_pendingAttributeMeta.resize(attrIndex + 1u);
        }
        m_pendingAttributes[attrIndex] = array;
        m_pendingAttributeMeta[attrIndex] = meta;
        return true;
    }

    // 写入属性 range
    bool WriteAttributeRange(
        const std::size_t attrIndex,
        const std::size_t offset,
        const std::size_t count,
        const void* data,
        const std::size_t byteSize,
        std::string* error = nullptr) override {
        if (attrIndex >= m_pendingAttributes.size() || attrIndex >= m_pendingAttributeMeta.size()) {
            std::lock_guard<std::mutex> lock(m_attributeWriteErrorMutex);
            return Fail(error, "attribute range write uses an unknown attribute index");
        }
        std::string copyError;
        if (!CopyBytesToArrayRange(
                m_pendingAttributes[attrIndex],
                m_pendingAttributeMeta[attrIndex],
                offset,
                count,
                data,
                byteSize,
                &copyError)) {
            std::lock_guard<std::mutex> lock(m_attributeWriteErrorMutex);
            return Fail(error, copyError.empty() ? "attribute range write failed" : copyError);
        }
        return true;
    }

    [[nodiscard]] bool SupportsConcurrentAttributeRangeWrites() const noexcept override {
        return true;
    }

    [[nodiscard]] bool SupportsAttributeDecodeStore() const noexcept override {
        return true;
    }

    std::shared_ptr<::datacodec::bytestore::IRandomAccessByteStore> CreateAttributeDecodeStore(
        const std::size_t attrIndex,
        const AttrStorageParams& meta,
        std::string* error = nullptr) override {
        if (!BeginAttribute(attrIndex, meta, error)) {
            return nullptr;
        }
        std::span<std::uint8_t> bytes;
        if (attrIndex >= m_pendingAttributes.size() ||
            !ResolveMutableArrayBytes(m_pendingAttributes[attrIndex], bytes, error)) {
            return nullptr;
        }
        return std::make_shared<NativeAttributeByteStore>(
            m_pendingAttributes[attrIndex],
            bytes);
    }

    // 结束属性 range 写入并挂接到属性集
    bool EndAttribute(const std::size_t attrIndex, std::string* error = nullptr) override {
        if (m_output == nullptr ||
            m_output->GetAttributeSet() == nullptr ||
            attrIndex >= m_pendingAttributes.size() ||
            attrIndex >= m_pendingAttributeMeta.size() ||
            m_pendingAttributes[attrIndex] == nullptr) {
            return Fail(error, "attribute end requires a pending attribute");
        }

        const auto& meta = m_pendingAttributeMeta[attrIndex];
        const auto nativeIndex = m_output->GetAttributeSet()->AddAttribute(
            ToNativeAttrRole(meta.type),
            ToNativeAttrAttachment(meta.attachmentType),
            m_pendingAttributes[attrIndex]);
        if (nativeIndex < 0) {
            return Fail(error, "failed to append decoded attribute to the output object");
        }
        if (attrIndex >= m_nativeAttributeIndices.size()) {
            m_nativeAttributeIndices.resize(attrIndex + 1u, -1);
        }
        m_nativeAttributeIndices[attrIndex] = static_cast<int>(nativeIndex);
        m_pendingAttributes[attrIndex] = nullptr;
        return true;
    }

    // 提交组装后的 iGame 输出对象
    bool Commit(std::string* error = nullptr) override {
        if (m_failed) {
            return Fail(error, m_failureMessage.empty() ? "decode adapter is in failed state" : m_failureMessage);
        }
        CommitCellArray();
        if (m_output == nullptr) {
            return Fail(error, "decode adapter has no output object");
        }
        return true;
    }

    [[nodiscard]] DataObject::Pointer TakeDataObject() const { return m_output; }

    [[nodiscard]] int NativeAttributeIndex(const std::size_t attrIndex) const noexcept {
        return attrIndex < m_nativeAttributeIndices.size()
            ? m_nativeAttributeIndices[attrIndex]
            : -1;
    }

    std::uint64_t NativeResidentBytesHint() const override { return m_nativeResidentBytes; }

    void ResetOutput() override {
        ReleaseOutputState();
    }

    void Abort() override {
        ReleaseOutputState();
    }

private:
    class NativeAttributeByteStore final : public ::datacodec::bytestore::IRandomAccessByteStore {
    public:
        NativeAttributeByteStore(
            ArrayObject::Pointer owner,
            const std::span<std::uint8_t> bytes) noexcept
            : m_owner(std::move(owner)), m_bytes(bytes) {}

        bool AppendBytes(
            const std::span<const std::uint8_t> bytes,
            std::string* error = nullptr) override {
            if (m_released || m_appendOffset > m_bytes.size() ||
                bytes.size() > m_bytes.size() - m_appendOffset) {
                return ::datacodec::validation::AssignError(error, "native attribute store append is outside the array range");
            }
            if (!bytes.empty()) {
                std::memcpy(m_bytes.data() + m_appendOffset, bytes.data(), bytes.size());
            }
            m_appendOffset += bytes.size();
            return true;
        }

        bool Seal(std::string* error = nullptr) override {
            if (m_released) {
                return ::datacodec::validation::AssignError(error, "native attribute store was already released");
            }
            return true;
        }

        bool ResizeBytes(
            const std::uint64_t byteSize,
            std::string* error = nullptr) override {
            if (m_released || byteSize != static_cast<std::uint64_t>(m_bytes.size())) {
                return ::datacodec::validation::AssignError(error, "native attribute store size does not match the array range");
            }
            m_appendOffset = 0u;
            return true;
        }

        bool WriteBytesAt(
            const std::uint64_t offset,
            const std::span<const std::uint8_t> bytes,
            std::string* error = nullptr) override {
            if (m_released || offset > m_bytes.size() ||
                bytes.size() > m_bytes.size() - static_cast<std::size_t>(offset)) {
                return ::datacodec::validation::AssignError(error, "native attribute store write is outside the array range");
            }
            if (!bytes.empty()) {
                std::memcpy(m_bytes.data() + static_cast<std::size_t>(offset), bytes.data(), bytes.size());
            }
            return true;
        }

        [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
            return static_cast<std::uint64_t>(m_bytes.size());
        }

        [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
            return static_cast<std::uint64_t>(m_bytes.size());
        }

        [[nodiscard]] std::span<const std::uint8_t> ContiguousBytes() const noexcept override {
            return m_released
                ? std::span<const std::uint8_t>{}
                : std::span<const std::uint8_t>(m_bytes.data(), m_bytes.size());
        }

        [[nodiscard]] bool CanRead() const noexcept override {
            return !m_released;
        }

        bool Read(
            const std::uint64_t offset,
            const std::span<std::uint8_t> output,
            std::string* error = nullptr) const override {
            if (m_released || offset > m_bytes.size() ||
                output.size() > m_bytes.size() - static_cast<std::size_t>(offset)) {
                return ::datacodec::validation::AssignError(error, "native attribute store read is outside the array range");
            }
            if (!output.empty()) {
                std::memcpy(output.data(), m_bytes.data() + static_cast<std::size_t>(offset), output.size());
            }
            return true;
        }

        bool CopyTo(
            ::datacodec::bytestore::IByteWriter& writer,
            std::string* error = nullptr) override {
            if (m_released) {
                return ::datacodec::validation::AssignError(error, "native attribute store was already released");
            }
            return writer.Write(
                std::span<const std::uint8_t>(m_bytes.data(), m_bytes.size()),
                error);
        }

        void Release() noexcept override {
            m_owner = nullptr;
            m_bytes = {};
            m_appendOffset = 0u;
            m_released = true;
        }

    private:
        ArrayObject::Pointer m_owner;
        std::span<std::uint8_t> m_bytes;
        std::size_t m_appendOffset{0u};
        bool m_released{false};
    };

    void ReleaseOutputState() {
        ResetPartialState();
        m_output = nullptr;
    }

    bool Fail(std::string* error, std::string message) {
        m_failed = true;
        m_failureMessage = std::move(message);
        return ::datacodec::validation::AssignError(error, m_failureMessage);
    }

    void ResetPartialState() {
        m_failed = false;
        m_failureMessage.clear();
        m_cellArray = nullptr;
        m_cellTypes = nullptr;
        ::datacodec::ReleaseVectorStorage(m_cellPolynomialOrders);
        m_pendingPoints = nullptr;
        m_pendingPointCount = 0u;
        m_pointDimension = 3u;
        m_pendingAttributes.clear();
        m_pendingAttributeMeta.clear();
        m_nativeAttributeIndices.clear();
        m_pendingCellCount = 0u;
        m_pendingConnectivityIds = nullptr;
        m_pendingOffsets = nullptr;
        m_pendingConnectivityCount = 0u;
        m_polyFaceTable = nullptr;
        m_polyCellFaces = nullptr;
        m_polyCells = nullptr;
        m_polyCellTypes = nullptr;
        m_polyCellCount = 0u;
        m_nativeResidentBytes = 0u;
    }

    // 仅供 decode adapter 使用的 codec 到 iGame 枚举桥接
    static IGenum ToNativeMeshType(const MeshType meshType) {
        switch (meshType) {
            case MeshType::SurfaceMesh:
                return IG_SURFACE_MESH;
            case MeshType::VolumeMesh:
                return IG_VOLUME_MESH;
            case MeshType::StructuredMesh:
                return IG_STRUCTURED_MESH;
            case MeshType::UnstructuredMesh:
            case MeshType::PolyhedronMesh:
                return IG_UNSTRUCTURED_MESH;
            case MeshType::PointSet:
            default:
                return IG_POINT_SET;
        }
    }

    static IGenum ToNativeAttrRole(const AttrRole attrRole) {
        switch (attrRole) {
            case AttrRole::Vector:
                return IG_VECTOR;
            case AttrRole::Normal:
                return IG_NORMAL;
            case AttrRole::TexCoord:
                return IG_TCOORD;
            case AttrRole::Tensor:
                return IG_TENSOR;
            case AttrRole::Color:
                return IG_RGB;
            case AttrRole::Scalar:
            case AttrRole::Unknown:
            default:
                return IG_SCALAR;
        }
    }

    static IGenum ToNativeAttrAttachment(const AttrAttachment attachmentType) {
        return attachmentType == AttrAttachment::Cell ? IG_CELL : IG_POINT;
    }

    static ArrayObject::Pointer CreateArray(const DataType dataType) {
        switch (dataType) {
            case DataType::Float32:
                return FloatArray::New();
            case DataType::Float64:
                return DoubleArray::New();
            case DataType::Int8:
                return CharArray::New();
            case DataType::UInt8:
                return UnsignedCharArray::New();
            case DataType::Int16:
                return ShortArray::New();
            case DataType::UInt16:
                return UnsignedShortArray::New();
            case DataType::Int32:
                return IntArray::New();
            case DataType::UInt32:
                return UnsignedIntArray::New();
            case DataType::Int64:
                return LongLongArray::New();
            case DataType::UInt64:
                return UnsignedLongLongArray::New();
            default:
                return nullptr;
        }
    }

    static bool ResolveMutableArrayBytes(
        const ArrayObject::Pointer& array,
        std::span<std::uint8_t>& bytes,
        std::string* error = nullptr) {
        bytes = {};
        if (array == nullptr) {
            return ::datacodec::validation::AssignError(error, "attribute byte store requires a target array");
        }
        const auto resolve = [&bytes](auto typed) {
            if (typed == nullptr) {
                return false;
            }
            bytes = std::span<std::uint8_t>(
                reinterpret_cast<std::uint8_t*>(typed->RawPointer()),
                typed->GetNumberOfValues() * sizeof(*typed->RawPointer()));
            return true;
        };
        bool resolved = false;
        switch (array->GetArrayType()) {
            case IG_FloatArray: resolved = resolve(DynamicCast<FloatArray>(array)); break;
            case IG_DoubleArray: resolved = resolve(DynamicCast<DoubleArray>(array)); break;
            case IG_CharArray: resolved = resolve(DynamicCast<CharArray>(array)); break;
            case IG_UnsignedCharArray: resolved = resolve(DynamicCast<UnsignedCharArray>(array)); break;
            case IG_ShortArray: resolved = resolve(DynamicCast<ShortArray>(array)); break;
            case IG_UnsignedShortArray: resolved = resolve(DynamicCast<UnsignedShortArray>(array)); break;
            case IG_IntArray: resolved = resolve(DynamicCast<IntArray>(array)); break;
            case IG_UnsignedIntArray: resolved = resolve(DynamicCast<UnsignedIntArray>(array)); break;
            case IG_LongLongArray: resolved = resolve(DynamicCast<LongLongArray>(array)); break;
            case IG_UnsignedLongLongArray: resolved = resolve(DynamicCast<UnsignedLongLongArray>(array)); break;
            default: break;
        }
        return resolved || ::datacodec::validation::AssignError(error, "attribute array type cannot expose a writable byte store");
    }

    // 把解码后的原始字节拷进带类型的原生数组 range
    static bool CopyBytesToArrayRange(
        const ArrayObject::Pointer& array,
        const AttrStorageParams& meta,
        const std::size_t offset,
        const std::size_t count,
        const void* data,
        const std::size_t byteSize,
        std::string* error = nullptr) {
        const auto fail = [error](const char* message) {
            return ::datacodec::validation::AssignError(error, message);
        };

        if (array == nullptr) {
            return fail("attribute range write requires a target array");
        }

        const auto componentCount = static_cast<std::size_t>(std::max(meta.dimension, 1));
        const auto valueSize = ::datacodec::DataTypeSize(meta.dataType);
        std::size_t tupleBytes = 0u;
        std::size_t byteOffset = 0u;
        std::size_t expectedBytes = 0u;
        if (!::datacodec::validation::CheckedMulSizeT(
                componentCount,
                valueSize,
                tupleBytes,
                "attribute range write tuple bytes",
                error) ||
            !::datacodec::validation::CheckedMulSizeT(
                offset,
                tupleBytes,
                byteOffset,
                "attribute range write byte offset",
                error) ||
            !::datacodec::validation::CheckedMulSizeT(
                count,
                tupleBytes,
                expectedBytes,
                "attribute range write byte size",
                error)) {
            return false;
        }
        if (byteSize != expectedBytes) {
            return fail("attribute range write byte size does not match metadata");
        }
        if (byteSize == 0u) {
            return true;
        }
        if (data == nullptr) {
            return fail("attribute range write requires input data");
        }
        const auto* source = static_cast<const std::uint8_t*>(data);

        const auto copyRange = [byteOffset, byteSize, source, fail](auto typed) -> bool {
            if (typed == nullptr) {
                return fail("attribute range write array type does not match metadata");
            }
            const auto totalBytes = typed->GetNumberOfValues() * sizeof(*typed->RawPointer());
            if (byteOffset > totalBytes || byteSize > totalBytes - byteOffset) {
                return fail("attribute range write is outside the target array");
            }
            auto* target = reinterpret_cast<std::uint8_t*>(typed->RawPointer()) + byteOffset;
            std::memcpy(target, source, byteSize);
            return true;
        };

        switch (array->GetArrayType()) {
            case IG_FloatArray:
                return copyRange(DynamicCast<FloatArray>(array));
            case IG_DoubleArray:
                return copyRange(DynamicCast<DoubleArray>(array));
            case IG_CharArray:
                return copyRange(DynamicCast<CharArray>(array));
            case IG_UnsignedCharArray:
                return copyRange(DynamicCast<UnsignedCharArray>(array));
            case IG_ShortArray:
                return copyRange(DynamicCast<ShortArray>(array));
            case IG_UnsignedShortArray:
                return copyRange(DynamicCast<UnsignedShortArray>(array));
            case IG_IntArray:
            case IG_INTARRAY:
                return copyRange(DynamicCast<IntArray>(array));
            case IG_UnsignedIntArray:
                return copyRange(DynamicCast<UnsignedIntArray>(array));
            case IG_LongLongArray:
                return copyRange(DynamicCast<LongLongArray>(array));
            case IG_UnsignedLongLongArray:
                return copyRange(DynamicCast<UnsignedLongLongArray>(array));
            default:
                return fail("attribute range write uses an unsupported array type");
        }
    }

    // 在尝试重建 polyhedron 之前做基础形状校验
    static bool HasPolyhedronData(const PolyhedronTopologyView& topology) {
        return topology.cellVertexOffsets != nullptr &&
            topology.cellVertexOffsetCount > 0 &&
            topology.cellUniqueVertexIds != nullptr &&
            topology.cellFaceOffsets != nullptr &&
            topology.cellFaceOffsetCount > 0 &&
            topology.faceVertexOffsets != nullptr &&
            topology.faceVertexOffsetCount > 0 &&
            topology.localFaceVertexIds != nullptr;
    }

    // 把当前准备好的拓扑缓冲区写入目标原生网格对象
    void CommitCellArray() {
        if (m_output == nullptr || m_cellArray == nullptr) {
            return;
        }

        if (ShouldCommitAsLagrangeMesh()) {
            CommitLagrangeCellArray();
            return;
        }

        switch (m_meshType) {
            case MeshType::SurfaceMesh: {
                const auto surfaceMesh = DynamicCast<SurfaceMesh>(m_output);
                if (surfaceMesh != nullptr) {
                    surfaceMesh->SetFaces(m_cellArray);
                }
                break;
            }
            case MeshType::VolumeMesh: {
                const auto volumeMesh = DynamicCast<VolumeMesh>(m_output);
                if (volumeMesh != nullptr) {
                    volumeMesh->SetVolumes(m_cellArray);
                }
                break;
            }
            case MeshType::UnstructuredMesh:
            case MeshType::PolyhedronMesh: {
                const auto unstructuredMesh = DynamicCast<UnstructuredMesh>(m_output);
                if (unstructuredMesh != nullptr && m_cellTypes != nullptr) {
                    unstructuredMesh->SetCells(m_cellArray, m_cellTypes);
                }
                break;
            }
            default:
                break;
        }
    }

    // 把一个局部 polyhedron 面展开成绝对点 id 列表
    static std::vector<igIndex> ExpandPolyhedronFace(
        const PolyhedronTopologyView& topology,
        const std::size_t cellVertexBegin,
        const std::size_t faceIndex) {
        std::vector<igIndex> facePointIds;
        if (faceIndex + 1 >= topology.faceVertexOffsetCount) {
            return facePointIds;
        }

        const auto localBegin = static_cast<std::size_t>(topology.faceVertexOffsets[faceIndex]);
        const auto localEnd = static_cast<std::size_t>(topology.faceVertexOffsets[faceIndex + 1]);
        facePointIds.reserve(localEnd - localBegin);
        for (std::size_t localIndex = localBegin; localIndex < localEnd; ++localIndex) {
            const auto localVertexId = static_cast<std::size_t>(topology.localFaceVertexIds[localIndex]);
            if (cellVertexBegin + localVertexId >= topology.cellUniqueVertexIdCount) {
                continue;
            }
            facePointIds.push_back(static_cast<igIndex>(topology.cellUniqueVertexIds[cellVertexBegin + localVertexId]));
        }
        return facePointIds;
    }

    // 追加一个 polyhedron chunk 到 volume mesh 的面表
    void AppendVolumePolyhedronChunk(const PolyhedronTopologyView& topology) {
        if (m_polyFaceTable == nullptr || m_polyCellFaces == nullptr ||
            topology.cellVertexOffsetCount == 0 || topology.cellFaceOffsetCount == 0) {
            return;
        }

        const auto cellCount = topology.cellVertexOffsetCount - 1;

        for (std::size_t cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
            const auto cellVertexBegin = static_cast<std::size_t>(topology.cellVertexOffsets[cellIndex]);
            const auto faceBegin = static_cast<std::size_t>(topology.cellFaceOffsets[cellIndex]);
            const auto faceEnd = static_cast<std::size_t>(topology.cellFaceOffsets[cellIndex + 1]);

            std::vector<igIndex> cellFaceIds;
            cellFaceIds.reserve(faceEnd - faceBegin);
            for (std::size_t faceIndex = faceBegin; faceIndex < faceEnd; ++faceIndex) {
                auto facePointIds = ExpandPolyhedronFace(topology, cellVertexBegin, faceIndex);
                if (facePointIds.empty()) {
                    continue;
                }
                auto currentFaceId = m_polyFaceTable->IsFace(facePointIds.data(), static_cast<int>(facePointIds.size()));
                if (currentFaceId < 0) {
                    m_polyFaceTable->InsertFace(facePointIds.data(), static_cast<int>(facePointIds.size()));
                    currentFaceId = m_polyFaceTable->GetNumberOfFaces() - 1;
                }
                cellFaceIds.push_back(currentFaceId);
            }
            m_polyCellFaces->AddCellIds(cellFaceIds.data(), static_cast<int>(cellFaceIds.size()));
        }
    }

    [[nodiscard]] bool ShouldCommitAsLagrangeMesh() const noexcept {
        return m_cellArray != nullptr &&
            m_cellTypes != nullptr &&
            !m_cellPolynomialOrders.empty() &&
            m_cellPolynomialOrders.size() == m_pendingCellCount;
    }

    void CommitLagrangeCellArray() {
        auto lagrangeMesh = LagrangeUnstructuredMesh::New();
        if (lagrangeMesh == nullptr) {
            return;
        }

        if (const auto pointSet = DynamicCast<PointSet>(m_output); pointSet != nullptr) {
            lagrangeMesh->SetPoints(pointSet->GetPoints());
        }
        if (m_output->GetAttributeSet() != nullptr) {
            lagrangeMesh->SetAttributeSet(m_output->GetAttributeSet());
        }

        const auto* rawCellTypes = m_cellTypes != nullptr ? m_cellTypes->RawPointer() : nullptr;
        if (rawCellTypes == nullptr) {
            return;
        }

        for (std::size_t cellIndex = 0u; cellIndex < m_pendingCellCount; ++cellIndex) {
            const igIndex* cellIds = nullptr;
            const auto cellSize = m_cellArray->GetCellIds(cellIndex, cellIds);
            if (cellSize <= 0 || cellIds == nullptr) {
                continue;
            }
            lagrangeMesh->AddCell(
                const_cast<igIndex*>(cellIds),
                cellSize,
                static_cast<IGenum>(rawCellTypes[cellIndex]),
                static_cast<int>(m_cellPolynomialOrders[cellIndex]));
        }

        m_output = lagrangeMesh;
    }

    // 追加一个 polyhedron chunk 到 unstructured cell array
    void AppendUnstructuredPolyhedronChunk(const PolyhedronTopologyView& topology) {
        if (m_polyCells == nullptr || topology.cellVertexOffsetCount == 0 || topology.cellFaceOffsetCount == 0) {
            return;
        }

        const auto cellCount = topology.cellVertexOffsetCount - 1;
        for (std::size_t cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
            const auto cellVertexBegin = static_cast<std::size_t>(topology.cellVertexOffsets[cellIndex]);
            const auto faceBegin = static_cast<std::size_t>(topology.cellFaceOffsets[cellIndex]);
            const auto faceEnd = static_cast<std::size_t>(topology.cellFaceOffsets[cellIndex + 1]);

            std::vector<igIndex> expandedCell;
            expandedCell.push_back(static_cast<igIndex>(faceEnd - faceBegin));
            for (std::size_t faceIndex = faceBegin; faceIndex < faceEnd; ++faceIndex) {
                auto facePointIds = ExpandPolyhedronFace(topology, cellVertexBegin, faceIndex);
                expandedCell.push_back(static_cast<igIndex>(facePointIds.size()));
                expandedCell.insert(expandedCell.end(), facePointIds.begin(), facePointIds.end());
            }
            m_polyCells->AddCellIds(expandedCell.data(), static_cast<int>(expandedCell.size()));
        }
    }

    // 当前目标网格类型
    MeshType m_meshType{MeshType::PointSet};
    // 正在组装的原生输出对象
    DataObject::Pointer m_output;
    // 暂存的原生 cell array
    CellArray::Pointer m_cellArray;
    // 暂存的原生 cell type 数组
    UnsignedIntArray::Pointer m_cellTypes;
    // 暂存的逐 cell 阶数流
    std::vector<std::uint16_t> m_cellPolynomialOrders;
    // 正在写入的 cell 数
    std::size_t m_pendingCellCount{0u};
    // 正在写入的原生 connectivity
    IdArray::Pointer m_pendingConnectivityIds;
    // connectivity 元素数量
    std::size_t m_pendingConnectivityCount{0u};
    // 正在写入的原生 offsets
    UnsignedIntArray::Pointer m_pendingOffsets;
    // 正在写入的点坐标对象
    Points::Pointer m_pendingPoints;
    // 正在写入的点数
    std::size_t m_pendingPointCount{0u};
    // 点坐标 tuple 的分量数
    std::size_t m_pointDimension{3u};
    // 正在写入的属性对象
    std::vector<ArrayObject::Pointer> m_pendingAttributes;
    // 正在写入的属性元数据
    std::vector<AttrStorageParams> m_pendingAttributeMeta;
    // 并发 range 写入失败时串行更新 Adapter 错误状态
    std::mutex m_attributeWriteErrorMutex;
    // DataCodec 属性索引到原生 AttributeSet 索引的稳定映射
    std::vector<int> m_nativeAttributeIndices;
    // 正在分块组装的 polyhedron 面表
    FaceTable::Pointer m_polyFaceTable;
    // 正在分块组装的 volume polyhedron cell-face 数组
    CellArray::Pointer m_polyCellFaces;
    // 正在分块组装的 unstructured polyhedron cell 数组
    CellArray::Pointer m_polyCells;
    // 正在分块组装的 unstructured polyhedron cell type 数组
    UnsignedIntArray::Pointer m_polyCellTypes;
    // polyhedron 总 cell 数
    std::size_t m_polyCellCount{0u};
    // iGame 原生 cell type 到 DataCodec cell type token 的映射
    iGameCellTypeMapping m_cellTypeMapping;
    // 原生输出对象已经接收的逻辑字节估计
    std::uint64_t m_nativeResidentBytes{0u};
    bool m_failed{false};
    std::string m_failureMessage;
};

IGAME_NAMESPACE_END

#endif


