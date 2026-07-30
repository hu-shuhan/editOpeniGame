#ifndef iGameDataCodeciGameEncodeAdapter_h
#define iGameDataCodeciGameEncodeAdapter_h

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Filter/Adapter/iGameCellTypeMapping.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameCellType.h"
#include "iGameDataObject.h"
#include "iGameFlatArray.h"
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
#include <limits>
#include <map>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

class iGameEncodeAttrView final : public ::datacodec::IEncodeAttrView {
public:
    using DataType = ::datacodec::DataType;
    using AttrRole = ::datacodec::AttrRole;
    using AttrAttachment = ::datacodec::AttrAttachment;

    iGameEncodeAttrView() = default;

    // 把一个原生属性描述快照成 encode 侧视图
    explicit iGameEncodeAttrView(const AttributeSet::Attribute& attribute)
        : m_name(attribute.pointer != nullptr ? attribute.pointer->GetName() : std::string{}),
          m_dataTypeSupported(
              attribute.pointer != nullptr && IsSupportedArrayType(attribute.pointer->GetArrayType())),
          m_dataType(
              m_dataTypeSupported
                  ? ToCodecDataType(attribute.pointer->GetArrayType())
                  : DataType::Float64),
          m_role(ToCodecAttrRole(attribute.type)),
          m_attachment(ToCodecAttrAttachment(attribute.attachmentType)),
          m_componentCount(attribute.pointer != nullptr ? attribute.pointer->GetDimension() : 0),
          m_elementCount(attribute.pointer != nullptr ? attribute.pointer->GetNumberOfElements() : 0),
          m_array(attribute.pointer) {}

    // 返回缓存后的属性名
    std::string GetName() const override { return m_name; }
    // 返回缓存后的标量类型
    DataType GetDataType() const override { return m_dataType; }
    // 返回底层数组类型是否可以按原类型编码
    bool IsDataTypeSupported() const override { return m_dataTypeSupported; }
    // 返回缓存后的语义角色
    AttrRole GetRole() const override { return m_role; }
    // 返回缓存后的附着位置
    AttrAttachment GetAttachType() const override { return m_attachment; }
    // 返回每个元素的分量数
    int GetComponentCount() const override { return m_componentCount; }
    // 返回属性元素个数
    std::size_t GetElementCount() const override { return m_elementCount; }

    // 连续数组快速路径
    const void* TryGetRawPtr() const override { return TryGetArrayRawPointer(m_array); }

    // 逐 tuple 读取路径
    void GetTuple(const std::size_t index, double* output) const override {
        if (m_array == nullptr || output == nullptr || index >= m_elementCount) {
            return;
        }
        m_array->GetElement(index, output);
    }

private:
    // 仅供 encode adapter 使用的 iGame 到 codec 枚举桥接
    static AttrRole ToCodecAttrRole(const IGenum attrType) {
        switch (attrType) {
            case IG_SCALAR:
                return AttrRole::Scalar;
            case IG_VECTOR:
                return AttrRole::Vector;
            case IG_NORMAL:
                return AttrRole::Normal;
            case IG_TCOORD:
                return AttrRole::TexCoord;
            case IG_TENSOR:
                return AttrRole::Tensor;
            case IG_RGB:
                return AttrRole::Color;
            default:
                return AttrRole::Unknown;
        }
    }

    static AttrAttachment ToCodecAttrAttachment(const IGenum attachmentType) {
        return attachmentType == IG_CELL ? AttrAttachment::Cell : AttrAttachment::Point;
    }

    static bool IsSupportedArrayType(const IGenum arrayType) {
        switch (arrayType) {
            case IG_FloatArray:
            case IG_DoubleArray:
            case IG_IntArray:
            case IG_INTARRAY:
            case IG_UnsignedIntArray:
            case IG_CharArray:
            case IG_UnsignedCharArray:
            case IG_ShortArray:
            case IG_UnsignedShortArray:
            case IG_LongLongArray:
            case IG_UnsignedLongLongArray:
                return true;
            default:
                return false;
        }
    }

    static DataType ToCodecDataType(const IGenum arrayType) {
        switch (arrayType) {
            case IG_FloatArray:
                return DataType::Float32;
            case IG_DoubleArray:
                return DataType::Float64;
            case IG_IntArray:
            case IG_INTARRAY:
                return DataType::Int32;
            case IG_UnsignedIntArray:
                return DataType::UInt32;
            case IG_CharArray:
                return DataType::Int8;
            case IG_UnsignedCharArray:
                return DataType::UInt8;
            case IG_ShortArray:
                return DataType::Int16;
            case IG_UnsignedShortArray:
                return DataType::UInt16;
            case IG_LongLongArray:
                return DataType::Int64;
            case IG_UnsignedLongLongArray:
                return DataType::UInt64;
            default:
                return DataType::Float64;
        }
    }

    static const void* TryGetArrayRawPointer(const ArrayObject::Pointer& array) {
        if (array == nullptr) {
            return nullptr;
        }

        switch (array->GetArrayType()) {
            case IG_FloatArray: {
                const auto typed = DynamicCast<FloatArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_DoubleArray: {
                const auto typed = DynamicCast<DoubleArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_IntArray:
            case IG_INTARRAY: {
                const auto typed = DynamicCast<IntArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_UnsignedIntArray: {
                const auto typed = DynamicCast<UnsignedIntArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_CharArray: {
                const auto typed = DynamicCast<CharArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_UnsignedCharArray: {
                const auto typed = DynamicCast<UnsignedCharArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_ShortArray: {
                const auto typed = DynamicCast<ShortArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_UnsignedShortArray: {
                const auto typed = DynamicCast<UnsignedShortArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_LongLongArray: {
                const auto typed = DynamicCast<LongLongArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            case IG_UnsignedLongLongArray: {
                const auto typed = DynamicCast<UnsignedLongLongArray>(array);
                return typed != nullptr ? typed->RawPointer() : nullptr;
            }
            default:
                return nullptr;
        }
    }

    // 缓存后的元数据和原生数组句柄
    std::string m_name;
    bool m_dataTypeSupported{false};
    DataType m_dataType{DataType::Float64};
    AttrRole m_role{AttrRole::Unknown};
    AttrAttachment m_attachment{AttrAttachment::Point};
    int m_componentCount{0};
    std::size_t m_elementCount{0};
    ArrayObject::Pointer m_array;
};

class iGameEncodeAdapter final : public ::datacodec::IEncodeAdapter {
public:
    using IndexType = ::datacodec::IndexType;
    using MeshType = ::datacodec::MeshType;
    using TopologyInputDescriptor = ::datacodec::TopologyInputDescriptor;
    using TopologyValueSource = ::datacodec::TopologyValueSource;
    using TopologyCellSizeSource = ::datacodec::TopologyCellSizeSource;
    using CellTypeRaw = ::datacodec::CellTypeRaw;
    using CellTypeCodecEntry = ::datacodec::CellTypeCodecEntry;
    using CellTypeMappingMode = ::datacodec::CellTypeMappingMode;
    using CellTypeFamilyCode = ::datacodec::CellTypeFamilyCode;
    using CellTypeLocalCode = ::datacodec::CellTypeLocalCode;
    using EncodeAdapterStatusInfo = ::datacodec::EncodeAdapterStatusInfo;
    using EncodeAdapterStatusKind = ::datacodec::EncodeAdapterStatusKind;

    explicit iGameEncodeAdapter(const DataObject::Pointer& dataObject) : m_dataObject(dataObject) {
        static_assert(sizeof(igIndex) == sizeof(IndexType));
        static_assert(sizeof(unsigned int) == sizeof(IndexType));
        BuildAttrViews();
        m_isNativePolyhedronMesh = ComputeNativePolyhedronMesh();
    }

    // 编码侧把原生数据对象类型映射到 codec mesh type
    MeshType GetMeshType() const override {
        if (IsNativePolyhedronMesh()) {
            return MeshType::PolyhedronMesh;
        }
        return m_dataObject != nullptr ? ToCodecMeshType(m_dataObject->GetDataObjectType()) : MeshType::PointSet;
    }

    // 返回源数据对象名称
    std::string GetName() const override { return m_dataObject != nullptr ? m_dataObject->GetName() : std::string{}; }

    // 返回点个数
    std::size_t GetNumberOfPoints() const override {
        const auto pointSet = DynamicCast<PointSet>(m_dataObject);
        return pointSet != nullptr ? pointSet->GetNumberOfPoints() : 0;
    }

    // 逐点读取坐标
    void GetPoint(std::size_t index, double output[3]) const override {
        if (output == nullptr) {
            return;
        }
        output[0] = 0.0;
        output[1] = 0.0;
        output[2] = 0.0;

        const auto pointSet = DynamicCast<PointSet>(m_dataObject);
        if (pointSet == nullptr || index >= pointSet->GetNumberOfPoints()) {
            return;
        }

        const auto& point = pointSet->GetPoint(index);
        output[0] = point[0];
        output[1] = point[1];
        output[2] = point[2];
    }

    // 单精度点坐标连续缓冲区快速路径
    const float* TryGetPointsF32() const override {
        const auto pointSet = DynamicCast<PointSet>(m_dataObject);
        const auto points = pointSet != nullptr ? pointSet->GetPoints() : nullptr;
        return points != nullptr ? points->RawPointer() : nullptr;
    }

    // 当前 iGame adapter 没有提供 double 点坐标快速路径
    const double* TryGetPointsF64() const override { return nullptr; }

    // 返回 cell 个数；structured mesh 走轴尺寸推导
    std::size_t GetNumberOfCells() const override {
        if (const auto structuredMesh = DynamicCast<StructuredMesh>(m_dataObject); structuredMesh != nullptr) {
            return ComputeStructuredCellCount(*structuredMesh);
        }

        const auto cells = GetCellArray();
        return cells != nullptr ? cells->GetNumberOfCells() : 0;
    }

    // 显式描述 iGame 原生 topology 数据来源
    bool DescribeTopology(TopologyInputDescriptor& output, std::string* error = nullptr) const override {
        output = {};
        output.pointCount = GetNumberOfPoints();
        output.cellCount = GetNumberOfCells();
        output.connectivityCount = GetCellIdBufferSize();
        output.structured = DynamicCast<StructuredMesh>(m_dataObject) != nullptr;
        output.polyhedron = IsNativePolyhedronMesh();
        if (output.structured || output.polyhedron || output.cellCount == 0u) {
            return true;
        }

        const auto cells = GetCellArray();
        if (cells == nullptr) {
            return ::datacodec::validation::AssignError(error, "iGame topology descriptor requires a cell array");
        }

        output.connectivity = TopologyValueSource::CompactArray;
        if (cells->IsUseOffSet()) {
            output.cellSize = TopologyCellSizeSource::Offsets;
            output.offsets = TopologyValueSource::CompactArray;
        } else {
            output.cellSize = TopologyCellSizeSource::FixedCellSize;
            output.offsets = TopologyValueSource::Derived;
            output.fixedCellSize = GetFixedCellSize();
        }

        if (DynamicCast<SurfaceMesh>(m_dataObject) != nullptr ||
            DynamicCast<VolumeMesh>(m_dataObject) != nullptr) {
            output.cellTypes = TopologyValueSource::Derived;
        }

        const auto unstructuredMesh = DynamicCast<UnstructuredMesh>(m_dataObject);
        if (unstructuredMesh != nullptr &&
            unstructuredMesh->GetCellTypes() != nullptr &&
            unstructuredMesh->GetNumberOfCells() > 0u) {
            output.cellTypes = TopologyValueSource::CompactArray;
        }

        const auto lagrangeMesh = DynamicCast<LagrangeUnstructuredMesh>(m_dataObject);
        if (lagrangeMesh != nullptr && lagrangeMesh->GetNumberOfCells() > 0u) {
            output.cellTypes = TopologyValueSource::Getter;
            output.cellPolynomialOrders = TopologyValueSource::Getter;
        }
        return true;
    }

    // 返回 connectivity 扁平缓冲区长度
    std::size_t GetCellIdBufferSize() const override {
        const auto cells = GetCellArray();
        return cells != nullptr && cells->GetNumberOfCells() > 0 ? cells->GetNumberOfCellIds() : 0;
    }

    // 返回 connectivity 扁平缓冲区指针
    const IndexType* GetCellIdBufferPtr() const override {
        const auto cells = GetCellArray();
        const auto ids = cells != nullptr ? cells->GetCellIdArray() : nullptr;
        return ids != nullptr ? reinterpret_cast<const IndexType*>(ids->RawPointer()) : nullptr;
    }

    // 返回 offset 缓冲区指针
    const IndexType* GetCellIdOffsetPtr() const override {
        const auto cells = GetCellArray();
        if (cells == nullptr || !cells->IsUseOffSet()) {
            return nullptr;
        }
        const auto offsets = cells->GetOffset();
        return offsets != nullptr ? reinterpret_cast<const IndexType*>(offsets->RawPointer()) : nullptr;
    }

    // 判断是否为定长 cell 拓扑
    bool IsFixedCellSize() const override {
        if (DynamicCast<StructuredMesh>(m_dataObject) != nullptr) {
            return false;
        }

        const auto cells = GetCellArray();
        return cells != nullptr && !cells->IsUseOffSet();
    }

    // 返回定长 cell 的元数
    int GetFixedCellSize() const override {
        const auto cells = GetCellArray();
        if (cells == nullptr || cells->GetNumberOfCells() == 0 || cells->IsUseOffSet()) {
            return -1;
        }
        return cells->GetCellSize(0);
    }

    // 普通非结构网格直接暴露连续 cell type 缓冲
    const IndexType* GetCellTypesPtr() const override {
        if (DynamicCast<LagrangeUnstructuredMesh>(m_dataObject) != nullptr) {
            return nullptr;
        }
        const auto unstructuredMesh = DynamicCast<UnstructuredMesh>(m_dataObject);
        const auto cellTypes = unstructuredMesh != nullptr
            ? unstructuredMesh->GetCellTypes()
            : nullptr;
        return cellTypes != nullptr
            ? reinterpret_cast<const IndexType*>(cellTypes->RawPointer())
            : nullptr;
    }

    // iGame type/order 走按 cell 读取，不在 adapter 里构造完整缓存
    const std::uint16_t* GetCellPolynomialOrdersPtr() const override { return nullptr; }

    // 按需读取 cell type
    bool ReadCellType(const std::size_t cellIndex, IndexType& output) const override {
        output = 0;
        const auto unstructuredMesh = DynamicCast<UnstructuredMesh>(m_dataObject);
        if (unstructuredMesh != nullptr && unstructuredMesh->GetCellTypes() != nullptr) {
            if (cellIndex >= unstructuredMesh->GetNumberOfCells()) {
                return false;
            }
            output = static_cast<IndexType>(unstructuredMesh->GetCellType(cellIndex));
            return true;
        }

        const auto lagrangeMesh = DynamicCast<LagrangeUnstructuredMesh>(m_dataObject);
        if (lagrangeMesh == nullptr || cellIndex >= lagrangeMesh->GetNumberOfCells()) {
            return false;
        }
        output = static_cast<IndexType>(lagrangeMesh->GetSpecificCellType(cellIndex));
        return true;
    }

    // 按需读取 cell polynomial order
    bool ReadCellPolynomialOrder(const std::size_t cellIndex, std::uint16_t& output) const override {
        output = 0u;
        const auto unstructuredMesh = DynamicCast<UnstructuredMesh>(m_dataObject);
        if (unstructuredMesh != nullptr && unstructuredMesh->GetCellTypes() != nullptr) {
            return false;
        }
        const auto lagrangeMesh = DynamicCast<LagrangeUnstructuredMesh>(m_dataObject);
        if (lagrangeMesh == nullptr || cellIndex >= lagrangeMesh->GetNumberOfCells()) {
            return false;
        }
        output = static_cast<std::uint16_t>(lagrangeMesh->GetCellOrder(cellIndex));
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

    bool EncodeCellTypeFamilyLocal(
        const CellTypeRaw rawType,
        CellTypeFamilyCode& familyCode,
        CellTypeLocalCode& familyLocalCode) const override {
        return m_cellTypeMapping.EncodeCellTypeFamilyLocal(rawType, familyCode, familyLocalCode);
    }

    bool DecodeCellTypeFamilyLocal(
        const CellTypeFamilyCode familyCode,
        const CellTypeLocalCode familyLocalCode,
        CellTypeRaw& rawType) const override {
        return m_cellTypeMapping.DecodeCellTypeFamilyLocal(familyCode, familyLocalCode, rawType);
    }

    bool EncodeCellPolynomialOrderLocal(
        const CellTypeRaw rawType,
        const std::uint16_t order,
        CellTypeLocalCode& localOrder) const override {
        return m_cellTypeMapping.EncodeCellPolynomialOrderLocal(rawType, order, localOrder);
    }

    bool DecodeCellPolynomialOrderLocal(
        const CellTypeRaw rawType,
        const CellTypeLocalCode localOrder,
        std::uint16_t& order) const override {
        return m_cellTypeMapping.DecodeCellPolynomialOrderLocal(rawType, localOrder, order);
    }

    // 返回 structured mesh 的轴尺寸
    bool GetStructuredAxisSize(int output[3]) const override {
        if (output == nullptr) {
            return false;
        }
        output[0] = 0;
        output[1] = 0;
        output[2] = 0;

        const auto structuredMesh = DynamicCast<StructuredMesh>(m_dataObject);
        if (structuredMesh == nullptr) {
            return false;
        }

        const auto* axisSize = structuredMesh->GetDimensionSize();
        if (axisSize == nullptr) {
            return false;
        }

        output[0] = axisSize[0];
        output[1] = axisSize[1];
        output[2] = axisSize[2];
        return true;
    }

    // 判断当前网格是否整体属于 polyhedron 类型
    bool IsPolyhedronMesh() const override {
        return IsNativePolyhedronMesh();
    }

    // 返回当前 adapter 暴露给 DataCodec 的多面体面表规模
    std::size_t GetNumberOfFaces() const override {
        const auto* faceCells = GetPolyhedronFaceCells();
        return faceCells != nullptr ? faceCells->GetNumberOfCells() : 0u;
    }

    // 借用面顶点扁平缓冲区
    const IndexType* GetFaceIdBufferPtr() const override {
        return BorrowCellArrayIds(GetPolyhedronFaceCells());
    }

    // 返回面顶点扁平缓冲区长度
    std::size_t GetFaceIdBufferSize() const override {
        return BorrowCellArrayIdCount(GetPolyhedronFaceCells());
    }

    // 借用面顶点 offset 缓冲区
    const IndexType* GetFaceIdOffsetPtr() const override {
        return ResolvePolyhedronCellArrayOffsets(GetPolyhedronFaceCells(), m_polyhedronFaceOffsets);
    }

    // 借用单元到面的扁平缓冲区
    const IndexType* GetCellFaceBufferPtr() const override {
        return BorrowCellArrayIds(GetPolyhedronCellFaceCells());
    }

    // 返回单元到面的扁平缓冲区长度
    std::size_t GetCellFaceBufferSize() const override {
        return BorrowCellArrayIdCount(GetPolyhedronCellFaceCells());
    }

    // 借用单元到面的 offset 缓冲区
    const IndexType* GetCellFaceOffsetPtr() const override {
        return ResolvePolyhedronCellArrayOffsets(GetPolyhedronCellFaceCells(), m_polyhedronCellFaceOffsets);
    }

    // 点属性个数
    std::size_t GetNumberOfPointAttrs() const override { return m_pointAttrs.size(); }
    // 按顺序返回一个 point 属性视图
    const ::datacodec::IEncodeAttrView& GetPointAttr(std::size_t index) const override { return m_pointAttrs.at(index); }

    // 单元属性个数
    std::size_t GetNumberOfCellAttrs() const override { return m_cellAttrs.size(); }
    // 按顺序返回一个 cell 属性视图
    const ::datacodec::IEncodeAttrView& GetCellAttr(std::size_t index) const override { return m_cellAttrs.at(index); }

    // 当前实现没有真实源数据大小信息
    std::int64_t GetSourceByteSizeHint() const override { return -1; }

    // 从 metadata 里取源数据路径
    std::string GetSourceLocationHint() const override {
        if (m_dataObject == nullptr || m_dataObject->GetMetadata() == nullptr) {
            return {};
        }
        return m_dataObject->GetMetadata()->GetString(FILE_PATH);
    }

    EncodeAdapterStatusInfo GetEncodeStatusInfo(const std::string_view stageName) const override {
        if (stageName.find("SpatialPartition") != std::string_view::npos ||
            stageName.find("Remap") != std::string_view::npos) {
            return {EncodeAdapterStatusKind::Sorting};
        }
        if (stageName.find("Topo") != std::string_view::npos) {
            return {EncodeAdapterStatusKind::TopologyCompression};
        }
        if (stageName.find("Geometry") != std::string_view::npos) {
            return {EncodeAdapterStatusKind::GeometryCompression};
        }
        if (stageName.find("Attribute") != std::string_view::npos) {
            return {EncodeAdapterStatusKind::AttributeCompression};
        }
        return {};
    }

    // 释放 iGame adapter 为多面体输入构建的临时面表
    void Abort() override {
        ResetInput();
    }

    // 释放 iGame adapter 为多面体输入构建的临时面表
    std::uint64_t ReleaseConvertedInputs() override {
        m_polyhedronCellFaces = nullptr;
        m_polyhedronFaces = nullptr;
        m_polyhedronCellFaceOffsets.clear();
        m_polyhedronFaceOffsets.clear();
        m_hasBuiltPolyhedronFaceTable = false;
        return 0;
    }

    // 释放本次编码输入对象和由输入派生的视图缓存
    void ResetInput() override {
        (void)ReleaseConvertedInputs();
        m_pointAttrs.clear();
        m_cellAttrs.clear();
        m_isNativePolyhedronMesh = false;
        m_dataObject = nullptr;
    }

private:
    bool IsNativePolyhedronMesh() const {
        return m_isNativePolyhedronMesh;
    }

    bool ComputeNativePolyhedronMesh() const {
        if (const auto volumeMesh = DynamicCast<VolumeMesh>(m_dataObject); volumeMesh != nullptr) {
            return volumeMesh->GetIsPolyhedronType();
        }

        const auto unstructuredMesh = DynamicCast<UnstructuredMesh>(m_dataObject);
        if (unstructuredMesh == nullptr || unstructuredMesh->GetCellTypes() == nullptr ||
            unstructuredMesh->GetNumberOfCells() == 0) {
            return false;
        }

        for (std::size_t cellIndex = 0; cellIndex < unstructuredMesh->GetNumberOfCells(); ++cellIndex) {
            if (unstructuredMesh->GetCellType(cellIndex) != IG_POLYHEDRON) {
                return false;
            }
        }
        return true;
    }

    // 仅供 encode adapter 使用的 iGame 到 codec 枚举桥接
    static MeshType ToCodecMeshType(const IGenum dataObjectType) {
        switch (dataObjectType) {
            case IG_SURFACE_MESH:
                return MeshType::SurfaceMesh;
            case IG_VOLUME_MESH:
                return MeshType::VolumeMesh;
            case IG_STRUCTURED_MESH:
                return MeshType::StructuredMesh;
            case IG_LAGRANGE_UNSTRUCTURED_MESH:
            case IG_UNSTRUCTURED_MESH:
                return MeshType::UnstructuredMesh;
            case IG_POINT_SET:
            default:
                return MeshType::PointSet;
        }
    }

    // 非 structured 网格读取 cell array 的便捷入口
    const CellArray::Pointer GetCellArray() const {
        return m_dataObject != nullptr ? m_dataObject->GetCellArray() : nullptr;
    }

    CellArray* GetPolyhedronFaceCells() const {
        const auto volumeMesh = DynamicCast<VolumeMesh>(m_dataObject);
        if (volumeMesh == nullptr || !volumeMesh->GetIsPolyhedronType()) {
            if (!BuildPolyhedronFacesForUnstructured()) {
                return nullptr;
            }
            return m_polyhedronFaces;
        }
        return volumeMesh->GetFaces();
    }

    CellArray* GetPolyhedronCellFaceCells() const {
        const auto volumeMesh = DynamicCast<VolumeMesh>(m_dataObject);
        if (volumeMesh == nullptr || !volumeMesh->GetIsPolyhedronType()) {
            if (!BuildPolyhedronFacesForUnstructured()) {
                return nullptr;
            }
            return m_polyhedronCellFaces;
        }
        return volumeMesh->GetVolumeFaces();
    }

    bool BuildPolyhedronFacesForUnstructured() const {
        if (m_hasBuiltPolyhedronFaceTable) {
            return m_polyhedronFaces != nullptr && m_polyhedronCellFaces != nullptr;
        }
        m_hasBuiltPolyhedronFaceTable = true;
        m_polyhedronFaces = nullptr;
        m_polyhedronCellFaces = nullptr;

        const auto unstructuredMesh = DynamicCast<UnstructuredMesh>(m_dataObject);
        if (unstructuredMesh == nullptr || unstructuredMesh->GetNumberOfCells() == 0u) {
            return false;
        }
        const auto totalPointCount = GetNumberOfPoints();
        if (totalPointCount == 0u) {
            return false;
        }

        auto faces = CellArray::New();
        const auto cellFaces = CellArray::New();
        std::map<std::vector<igIndex>, igIndex> faceIdsByKey;
        std::vector<igIndex> faceIds;
        std::vector<igIndex> pointIds;
        std::vector<igIndex> faceKey;
        for (std::size_t cellIndex = 0; cellIndex < unstructuredMesh->GetNumberOfCells(); ++cellIndex) {
            if (unstructuredMesh->GetCellType(cellIndex) != IG_POLYHEDRON) {
                return false;
            }
            auto* cell = unstructuredMesh->GetCell(cellIndex);
            if (cell == nullptr) {
                return false;
            }

            const auto faceCount = cell->GetNumberOfFaces();
            if (faceCount <= 0) {
                return false;
            }
            faceIds.clear();
            faceIds.reserve(static_cast<std::size_t>(faceCount));
            for (int faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
                auto* face = cell->GetFace(faceIndex);
                if (face == nullptr || face->m_PointIds == nullptr) {
                    return false;
                }
                const auto rawPointCount = face->m_PointIds->GetNumberOfIds();
                if (rawPointCount < 3 || rawPointCount > std::numeric_limits<int>::max()) {
                    return false;
                }
                const auto pointCount = static_cast<int>(rawPointCount);
                pointIds.clear();
                pointIds.reserve(static_cast<std::size_t>(pointCount));
                for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                    const auto pointId = face->m_PointIds->GetId(pointIndex);
                    if (pointId < 0 || static_cast<std::size_t>(pointId) >= totalPointCount) {
                        return false;
                    }
                    pointIds.push_back(pointId);
                }

                faceKey = pointIds;
                std::sort(faceKey.begin(), faceKey.end());
                const auto faceIterator = faceIdsByKey.find(faceKey);
                igIndex faceId = 0;
                if (faceIterator == faceIdsByKey.end()) {
                    if (faceIdsByKey.size() > static_cast<std::size_t>(std::numeric_limits<igIndex>::max())) {
                        return false;
                    }
                    faceId = static_cast<igIndex>(faceIdsByKey.size());
                    faceIdsByKey.emplace(faceKey, faceId);
                    faces->AddCellIds(pointIds.data(), pointCount);
                } else {
                    faceId = faceIterator->second;
                }
                faceIds.push_back(faceId);
            }
            cellFaces->AddCellIds(faceIds.data(), static_cast<int>(faceIds.size()));
        }

        m_polyhedronFaces = faces;
        m_polyhedronCellFaces = cellFaces;
        return m_polyhedronFaces != nullptr && m_polyhedronCellFaces != nullptr;
    }

    static const IndexType* BorrowCellArrayIds(CellArray* cells) {
        if (cells == nullptr) {
            return nullptr;
        }
        const auto ids = cells->GetCellIdArray();
        return ids != nullptr ? reinterpret_cast<const IndexType*>(ids->RawPointer()) : nullptr;
    }

    static std::size_t BorrowCellArrayIdCount(CellArray* cells) {
        return cells != nullptr ? cells->GetNumberOfCellIds() : 0u;
    }

    static const IndexType* BorrowCellArrayOffsets(CellArray* cells) {
        if (cells == nullptr || !cells->IsUseOffSet()) {
            return nullptr;
        }
        const auto offsets = cells->GetOffset();
        return offsets != nullptr ? reinterpret_cast<const IndexType*>(offsets->RawPointer()) : nullptr;
    }

    static const IndexType* ResolvePolyhedronCellArrayOffsets(
        CellArray* cells,
        std::vector<IndexType>& synthesizedOffsets) {
        synthesizedOffsets.clear();
        if (cells == nullptr) {
            return nullptr;
        }
        if (const auto* offsets = BorrowCellArrayOffsets(cells); offsets != nullptr) {
            return offsets;
        }

        const auto cellCount = cells->GetNumberOfCells();
        synthesizedOffsets.reserve(static_cast<std::size_t>(cellCount) + 1u);
        synthesizedOffsets.push_back(0);
        for (IGsize cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
            synthesizedOffsets.push_back(
                static_cast<IndexType>(synthesizedOffsets.back() + cells->GetCellSize(cellIndex)));
        }
        return synthesizedOffsets.data();
    }

    // 结构化网格用轴尺寸推导 cell 数，不直接依赖 cell array
    static std::size_t ComputeStructuredCellCount(const StructuredMesh& mesh) {
        const auto* axisSize = const_cast<StructuredMesh&>(mesh).GetDimensionSize();
        if (axisSize == nullptr || axisSize[0] <= 0 || axisSize[1] <= 0) {
            return 0;
        }

        const auto xCount = static_cast<std::size_t>(std::max(axisSize[0] - 1, 0));
        const auto yCount = static_cast<std::size_t>(std::max(axisSize[1] - 1, 0));
        if (axisSize[2] <= 1) {
            return xCount * yCount;
        }

        const auto zCount = static_cast<std::size_t>(std::max(axisSize[2] - 1, 0));
        return xCount * yCount * zCount;
    }

    // 按 pipeline 可见顺序缓存 point 和 cell 属性描述
    void BuildAttrViews() {
        m_pointAttrs.clear();
        m_cellAttrs.clear();
        if (m_dataObject == nullptr || m_dataObject->GetAttributeSet() == nullptr) {
            return;
        }

        const auto pointAttrs = m_dataObject->GetAttributeSet()->GetAllPointAttributes();
        if (pointAttrs != nullptr) {
            m_pointAttrs.reserve(pointAttrs->GetNumberOfElements());
            for (std::size_t attrIndex = 0; attrIndex < pointAttrs->GetNumberOfElements(); ++attrIndex) {
                const auto& attr = pointAttrs->GetElement(attrIndex);
                if (attr.pointer == nullptr) {
                    continue;
                }
                m_pointAttrs.emplace_back(attr);
            }
        }

        const auto cellAttrs = m_dataObject->GetAttributeSet()->GetAllCellAttributes();
        if (cellAttrs != nullptr) {
            m_cellAttrs.reserve(cellAttrs->GetNumberOfElements());
            for (std::size_t attrIndex = 0; attrIndex < cellAttrs->GetNumberOfElements(); ++attrIndex) {
                const auto& attr = cellAttrs->GetElement(attrIndex);
                if (attr.pointer == nullptr) {
                    continue;
                }
                m_cellAttrs.emplace_back(attr);
            }
        }
    }

    // 源网格对象
    DataObject::Pointer m_dataObject;
    // 缓存原生 polyhedron 判定，避免编码阶段重复扫描所有 cell type
    bool m_isNativePolyhedronMesh{false};
    // 点属性视图缓存
    std::vector<iGameEncodeAttrView> m_pointAttrs;
    // 单元属性视图缓存
    std::vector<iGameEncodeAttrView> m_cellAttrs;
    // iGame unstructured polyhedron 转成 DataCodec 多面体面表后的 cell->face 缓存
    mutable CellArray::Pointer m_polyhedronCellFaces;
    // iGame unstructured polyhedron 转成 DataCodec 多面体面表后的 face->point 缓存
    mutable CellArray::Pointer m_polyhedronFaces;
    // DataCodec 多面体 cell->face offset 缓存
    mutable std::vector<IndexType> m_polyhedronCellFaceOffsets;
    // DataCodec 多面体 face->point offset 缓存
    mutable std::vector<IndexType> m_polyhedronFaceOffsets;
    // 标记临时多面体面表是否已经尝试构建
    mutable bool m_hasBuiltPolyhedronFaceTable{false};
    // iGame 原生 cell type 到 DataCodec cell type token 的映射
    iGameCellTypeMapping m_cellTypeMapping;
};

[[nodiscard]] inline bool CanCreateiGameEncodeAdapter(const DataObject::Pointer& input) {
    return input != nullptr &&
        (DynamicCast<PointSet>(input) != nullptr ||
         DynamicCast<SurfaceMesh>(input) != nullptr ||
         DynamicCast<VolumeMesh>(input) != nullptr ||
         DynamicCast<UnstructuredMesh>(input) != nullptr ||
         DynamicCast<StructuredMesh>(input) != nullptr ||
         DynamicCast<LagrangeUnstructuredMesh>(input) != nullptr);
}

IGAME_NAMESPACE_END

#endif
