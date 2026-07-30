#ifndef DATACODEC_API_ADAPTER_IENCODEADAPTER_H
#define DATACODEC_API_ADAPTER_IENCODEADAPTER_H

#include "DataCodec/Common/Views/ArrayViews.h"
#include "DataCodec/Common/Views/AttributeViews.h"
#include "DataCodec/Common/Views/TopologyViews.h"
#include "DataCodec/API/Adapter/ICellTypeMapping.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Codec/Topology/Common/CellTypeCodec.h"

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace datacodec {

enum class EncodeAdapterStatusKind : std::uint8_t {
    None = 0,
    Sorting,
    TopologyCompression,
    GeometryCompression,
    AttributeCompression,
};

struct EncodeAdapterStatusInfo {
    EncodeAdapterStatusKind kind{EncodeAdapterStatusKind::None};
};

struct IEncodeAttrView {
    virtual ~IEncodeAttrView() = default;

    // 属性名称
    virtual std::string GetName() const = 0;
    // 属性底层标量类型
    virtual DataType GetDataType() const = 0;
    // 属性底层类型是否能无转换进入 DataCodec
    virtual bool IsDataTypeSupported() const { return true; }
    // 属性语义角色，例如 scalar、vector、normal
    virtual AttrRole GetRole() const = 0;
    // 属性附着位置，point 或 cell
    virtual AttrAttachment GetAttachType() const = 0;
    // 每个元素的分量数
    virtual int GetComponentCount() const = 0;
    // 属性元素个数
    virtual std::size_t GetElementCount() const = 0;
    // 连续紧凑数组的快速路径；返回空表示走逐 tuple 回退路径
    virtual const void* TryGetRawPtr() const { return nullptr; }
    // 当拿不到连续原始内存时使用的通用逐 tuple 读取路径
    virtual void GetTuple(std::size_t index, double* output) const = 0;
    // 按属性自身标量类型写出 tuple 字节
    virtual bool GetTupleBytes(std::size_t index, void* output, std::string* error = nullptr) const {
        if (!IsDataTypeSupported()) {
            return validation::AssignError(error, "attribute scalar type is unsupported");
        }
        const auto componentCount = GetComponentCount();
        if (componentCount <= 0) {
            return true;
        }
        std::vector<double> tuple(static_cast<std::size_t>(componentCount), 0.0);
        GetTuple(index, tuple.data());
        return WriteDoubleTupleAsScalarBytes(
            ToScalarType(GetDataType()),
            tuple.data(),
            static_cast<std::size_t>(componentCount),
            output,
            error);
    }

    // 构建 DataCodec 属性输入视图，优先借用连续内存
    virtual bool BuildAttributeView(EncodeAttributeView& output) const {
        output = {};
        if (!IsDataTypeSupported()) {
            return false;
        }
        output.name = GetName();
        output.role = GetRole();
        output.attachment = GetAttachType();
        const auto scalarType = ToScalarType(GetDataType());
        if (const auto* raw = TryGetRawPtr(); raw != nullptr) {
            output.values = MakeCompactNumericArrayView(
                raw,
                scalarType,
                GetElementCount(),
                GetComponentCount(),
                ViewBufferOrigin::Borrowed);
            return output.values.IsValid();
        }
        output.values.scalarType = scalarType;
        output.values.layout = ArrayLayout::GetterOnly;
        output.values.origin = ViewBufferOrigin::Borrowed;
        output.values.tupleCount = GetElementCount();
        output.values.componentCount = GetComponentCount();
        output.values.userData = this;
        output.values.getTupleBytes = &IEncodeAttrView::ReadTupleBytesForView;
        return output.values.IsValid();
    }

private:
    static bool ReadTupleBytesForView(
        const void* userData,
        const std::size_t tupleIndex,
        void* output,
        std::string* error) {
        const auto* attr = static_cast<const IEncodeAttrView*>(userData);
        if (attr == nullptr || output == nullptr) {
            return validation::AssignError(error, "attribute tuple byte getter received a null input");
        }
        return attr->GetTupleBytes(tupleIndex, output, error);
    }
};

struct IEncodeGeometryInput {
    virtual ~IEncodeGeometryInput() = default;

    // 点个数
    virtual std::size_t GetNumberOfPoints() const = 0;
    // 逐点读取接口，供没有连续点数组时回退使用
    virtual void GetPoint(std::size_t index, double output[3]) const = 0;
    // getter 路径的点坐标标量类型
    virtual ScalarType GetPointScalarType() const { return ScalarType::Float64; }
    // 按点坐标标量类型写出 3 分量 tuple 字节
    virtual bool GetPointTupleBytes(std::size_t index, void* output, std::string* error = nullptr) const {
        double tuple[3]{0.0, 0.0, 0.0};
        GetPoint(index, tuple);
        return WriteDoubleTupleAsScalarBytes(GetPointScalarType(), tuple, 3u, output, error);
    }
    // 可选的原始几何访问入口，供直接采集字节时使用
    virtual const float* TryGetPointsF32() const { return nullptr; }
    // 预留的 double 点数组快速路径
    virtual const double* TryGetPointsF64() const { return nullptr; }
    // 构建 DataCodec 几何输入视图，优先借用原始点数组
    virtual bool BuildGeometryView(NumericArrayView& output) const {
        output = {};
        output.tupleCount = GetNumberOfPoints();
        output.componentCount = 3;
        if (const auto* raw = TryGetPointsF32(); raw != nullptr) {
            output = MakeCompactNumericArrayView(
                raw,
                ScalarType::Float32,
                GetNumberOfPoints(),
                3,
                ViewBufferOrigin::Borrowed);
            return true;
        }
        if (const auto* raw = TryGetPointsF64(); raw != nullptr) {
            output = MakeCompactNumericArrayView(
                raw,
                ScalarType::Float64,
                GetNumberOfPoints(),
                3,
                ViewBufferOrigin::Borrowed);
            return true;
        }
        output.scalarType = GetPointScalarType();
        output.layout = ArrayLayout::GetterOnly;
        output.origin = ViewBufferOrigin::Borrowed;
        output.userData = this;
        output.getTupleBytes = &IEncodeGeometryInput::ReadPointTupleBytesForView;
        return output.IsValid();
    }

private:
    static bool ReadPointTupleBytesForView(
        const void* userData,
        const std::size_t tupleIndex,
        void* output,
        std::string* error) {
        const auto* geometry = static_cast<const IEncodeGeometryInput*>(userData);
        if (geometry == nullptr || output == nullptr) {
            return validation::AssignError(error, "point tuple byte getter received a null input");
        }
        return geometry->GetPointTupleBytes(tupleIndex, output, error);
    }
};

enum class TopologyValueSource : std::uint8_t {
    Unavailable = 0,
    Derived = 1,
    CompactArray = 2,
    Getter = 3,
};

enum class TopologyCellSizeSource : std::uint8_t {
    None = 0,
    FixedCellSize = 1,
    Offsets = 2,
    CellTypeCodec = 3,
};

struct TopologyInputDescriptor {
    std::size_t pointCount{0};
    std::size_t cellCount{0};
    std::size_t connectivityCount{0};
    TopologyValueSource connectivity{TopologyValueSource::Unavailable};
    TopologyValueSource offsets{TopologyValueSource::Unavailable};
    TopologyCellSizeSource cellSize{TopologyCellSizeSource::None};
    int fixedCellSize{-1};
    TopologyValueSource cellTypes{TopologyValueSource::Unavailable};
    TopologyValueSource cellPolynomialOrders{TopologyValueSource::Unavailable};
    bool structured{false};
    bool polyhedron{false};
};

inline bool TopologyValueSourceProvidesStream(const TopologyValueSource source) noexcept {
    return source == TopologyValueSource::CompactArray || source == TopologyValueSource::Getter;
}

struct IEncodeTopologyInput : public ICellTypeMapping {
    virtual ~IEncodeTopologyInput() = default;

    // 显式描述 topology 每类数据的语义来源
    virtual bool DescribeTopology(TopologyInputDescriptor& output, std::string* error = nullptr) const = 0;
    // 单元个数
    virtual std::size_t GetNumberOfCells() const = 0;
    // 连通性扁平缓冲区长度
    virtual std::size_t GetCellIdBufferSize() const = 0;
    // 连通性扁平缓冲区指针
    virtual const IndexType* GetCellIdBufferPtr() const = 0;
    // 偏移缓冲区指针；定长 cell 网格可返回空
    virtual const IndexType* GetCellIdOffsetPtr() const = 0;
    // 定长 cell 网格可以跳过 offset 编码
    virtual bool IsFixedCellSize() const = 0;
    // 定长 cell 网格的固定元数；未知时返回负值
    virtual int GetFixedCellSize() const = 0;
    // 异构 cell 网格可选的逐 cell 原生类型流
    virtual const IndexType* GetCellTypesPtr() const { return nullptr; }
    // polynomial-order-dependent cell 可选的逐 cell 阶数流
    virtual const std::uint16_t* GetCellPolynomialOrdersPtr() const { return nullptr; }
    // 按 cell 读取原生类型，缺少连续缓冲区时走 adapter 自身的按需路径
    virtual bool ReadCellType(const std::size_t cellIndex, IndexType& output) const {
        const auto* cellTypes = GetCellTypesPtr();
        if (cellTypes == nullptr || cellIndex >= GetNumberOfCells()) {
            output = 0;
            return false;
        }
        output = cellTypes[cellIndex];
        return true;
    }
    // 按 cell 读取阶数，缺少连续缓冲区时走 adapter 自身的按需路径
    virtual bool ReadCellPolynomialOrder(const std::size_t cellIndex, std::uint16_t& output) const {
        const auto* cellPolynomialOrders = GetCellPolynomialOrdersPtr();
        if (cellPolynomialOrders == nullptr || cellIndex >= GetNumberOfCells()) {
            output = 0;
            return false;
        }
        output = cellPolynomialOrders[cellIndex];
        return true;
    }
    // 结构化网格只靠轴尺寸重建拓扑
    virtual bool GetStructuredAxisSize(int output[3]) const {
        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
        return false;
    }

    // 构建 DataCodec 普通拓扑输入视图
    virtual bool BuildTopologyView(TopologyView& output) const {
        output = {};
        TopologyInputDescriptor descriptor;
        if (!DescribeTopology(descriptor)) {
            return false;
        }
        output.cellCount = descriptor.cellCount;
        output.pointCount = descriptor.pointCount;
        output.fixedCellSizeEnabled = descriptor.cellSize == TopologyCellSizeSource::FixedCellSize;
        output.fixedCellSize = output.fixedCellSizeEnabled ? descriptor.fixedCellSize : 0;

        if (descriptor.structured || descriptor.polyhedron) {
            return output.cellCount == 0u;
        }

        if (output.cellCount != 0u && descriptor.connectivity != TopologyValueSource::CompactArray) {
            return false;
        }
        output.connectivity = MakeCompactIndexArrayView(
            GetCellIdBufferPtr(),
            IndexValueType::UInt32,
            descriptor.connectivityCount,
            ViewBufferOrigin::Borrowed);
        if (descriptor.cellSize == TopologyCellSizeSource::Offsets) {
            if (descriptor.offsets != TopologyValueSource::CompactArray) {
                return false;
            }
            output.offsets = MakeCompactIndexArrayView(
                GetCellIdOffsetPtr(),
                IndexValueType::UInt32,
                output.cellCount > 0 ? output.cellCount + 1 : 0,
                ViewBufferOrigin::Borrowed);
        }

        if (descriptor.cellTypes == TopologyValueSource::CompactArray) {
            const auto* cellTypes = GetCellTypesPtr();
            if (output.cellCount != 0u && cellTypes == nullptr) {
                return false;
            }
            output.cellTypes = MakeCompactNumericArrayView(
                cellTypes,
                ScalarType::UInt32,
                output.cellCount,
                1,
                ViewBufferOrigin::Borrowed);
        } else if (descriptor.cellTypes == TopologyValueSource::Getter) {
            output.cellTypes.scalarType = ScalarType::UInt32;
            output.cellTypes.layout = ArrayLayout::GetterOnly;
            output.cellTypes.origin = ViewBufferOrigin::Borrowed;
            output.cellTypes.tupleCount = output.cellCount;
            output.cellTypes.componentCount = 1;
            output.cellTypes.userData = this;
            output.cellTypes.getTupleBytes = &IEncodeTopologyInput::ReadCellTypeTupleBytesForView;
        }
        if (descriptor.cellPolynomialOrders == TopologyValueSource::CompactArray) {
            const auto* cellPolynomialOrders = GetCellPolynomialOrdersPtr();
            if (output.cellCount != 0u && cellPolynomialOrders == nullptr) {
                return false;
            }
            output.cellPolynomialOrders = MakeCompactNumericArrayView(
                cellPolynomialOrders,
                ScalarType::UInt16,
                output.cellCount,
                1,
                ViewBufferOrigin::Borrowed);
        } else if (descriptor.cellPolynomialOrders == TopologyValueSource::Getter) {
            output.cellPolynomialOrders.scalarType = ScalarType::UInt16;
            output.cellPolynomialOrders.layout = ArrayLayout::GetterOnly;
            output.cellPolynomialOrders.origin = ViewBufferOrigin::Borrowed;
            output.cellPolynomialOrders.tupleCount = output.cellCount;
            output.cellPolynomialOrders.componentCount = 1;
            output.cellPolynomialOrders.userData = this;
            output.cellPolynomialOrders.getTupleBytes = &IEncodeTopologyInput::ReadCellPolynomialOrderTupleBytesForView;
        }

        if (output.cellCount == 0) {
            return true;
        }
        if (!output.connectivity.IsValid() || output.connectivity.data == nullptr) {
            return false;
        }
        return output.fixedCellSizeEnabled || output.offsets.IsValid();
    }

private:
    static bool ReadCellTypeTupleBytesForView(
        const void* userData,
        const std::size_t tupleIndex,
        void* output,
        std::string* error) {
        const auto* topology = static_cast<const IEncodeTopologyInput*>(userData);
        if (topology == nullptr || output == nullptr) {
            return validation::AssignError(error, "cell type tuple byte getter received a null input");
        }
        IndexType value = 0u;
        if (!topology->ReadCellType(tupleIndex, value)) {
            return validation::AssignError(error, "cell type tuple byte getter failed to read adapter data");
        }
        std::memcpy(output, &value, sizeof(value));
        return true;
    }

    static bool ReadCellPolynomialOrderTupleBytesForView(
        const void* userData,
        const std::size_t tupleIndex,
        void* output,
        std::string* error) {
        const auto* topology = static_cast<const IEncodeTopologyInput*>(userData);
        if (topology == nullptr || output == nullptr) {
            return validation::AssignError(error, "cell polynomial order tuple byte getter received a null input");
        }
        std::uint16_t value = 0u;
        if (!topology->ReadCellPolynomialOrder(tupleIndex, value)) {
            return validation::AssignError(error, "cell polynomial order tuple byte getter failed to read adapter data");
        }
        std::memcpy(output, &value, sizeof(value));
        return true;
    }
};

struct IEncodePolyhedronInput {
    virtual ~IEncodePolyhedronInput() = default;

    // 是否为多面体网格
    virtual bool IsPolyhedronMesh() const { return false; }
    // 面个数
    virtual std::size_t GetNumberOfFaces() const { return 0; }
    // 面顶点 id 扁平缓冲区
    virtual const IndexType* GetFaceIdBufferPtr() const { return nullptr; }
    // 面顶点 id 扁平缓冲区长度
    virtual std::size_t GetFaceIdBufferSize() const {
        const auto* offsets = GetFaceIdOffsetPtr();
        const auto faceCount = GetNumberOfFaces();
        return offsets != nullptr && faceCount > 0u ? static_cast<std::size_t>(offsets[faceCount]) : 0u;
    }
    // 面顶点 offset 缓冲区
    virtual const IndexType* GetFaceIdOffsetPtr() const { return nullptr; }
    // 单元到面的扁平引用缓冲区
    virtual const IndexType* GetCellFaceBufferPtr() const { return nullptr; }
    // 单元到面的扁平引用缓冲区长度
    virtual std::size_t GetCellFaceBufferSize() const = 0;
    // 单元到面的偏移缓冲区
    virtual const IndexType* GetCellFaceOffsetPtr() const { return nullptr; }
};

struct IEncodeAttributeInput {
    virtual ~IEncodeAttributeInput() = default;

    // 点属性个数
    virtual std::size_t GetNumberOfPointAttrs() const = 0;
    // 按顺序读取一个 point 属性视图
    virtual const IEncodeAttrView& GetPointAttr(std::size_t index) const = 0;
    // 单元属性个数
    virtual std::size_t GetNumberOfCellAttrs() const = 0;
    // 按顺序读取一个 cell 属性视图
    virtual const IEncodeAttrView& GetCellAttr(std::size_t index) const = 0;

    // 构建指定 point 属性的 DataCodec 输入视图
    virtual bool BuildPointAttributeView(
        const std::size_t index,
        EncodeAttributeView& output) const {
        if (index >= GetNumberOfPointAttrs()) {
            output = {};
            return false;
        }
        return GetPointAttr(index).BuildAttributeView(output);
    }

    // 构建指定 cell 属性的 DataCodec 输入视图
    virtual bool BuildCellAttributeView(
        const std::size_t index,
        EncodeAttributeView& output) const {
        if (index >= GetNumberOfCellAttrs()) {
            output = {};
            return false;
        }
        return GetCellAttr(index).BuildAttributeView(output);
    }
};

struct IEncodeAdapter
    : public IEncodeGeometryInput,
      public IEncodeTopologyInput,
      public IEncodePolyhedronInput,
      public IEncodeAttributeInput {
    virtual ~IEncodeAdapter() = default;

    using IEncodeGeometryInput::GetNumberOfPoints;
    using IEncodeGeometryInput::GetPoint;
    using IEncodeGeometryInput::GetPointScalarType;
    using IEncodeGeometryInput::GetPointTupleBytes;
    using IEncodeGeometryInput::TryGetPointsF32;
    using IEncodeGeometryInput::TryGetPointsF64;

    using IEncodeTopologyInput::DescribeTopology;
    using IEncodeTopologyInput::GetNumberOfCells;
    using IEncodeTopologyInput::GetCellIdBufferSize;
    using IEncodeTopologyInput::GetCellIdBufferPtr;
    using IEncodeTopologyInput::GetCellIdOffsetPtr;
    using IEncodeTopologyInput::IsFixedCellSize;
    using IEncodeTopologyInput::GetFixedCellSize;
    using IEncodeTopologyInput::GetCellTypesPtr;
    using IEncodeTopologyInput::GetCellPolynomialOrdersPtr;
    using IEncodeTopologyInput::ReadCellType;
    using IEncodeTopologyInput::ReadCellPolynomialOrder;
    using IEncodeTopologyInput::GetStructuredAxisSize;

    using IEncodePolyhedronInput::IsPolyhedronMesh;
    using IEncodePolyhedronInput::GetNumberOfFaces;
    using IEncodePolyhedronInput::GetFaceIdBufferPtr;
    using IEncodePolyhedronInput::GetFaceIdBufferSize;
    using IEncodePolyhedronInput::GetFaceIdOffsetPtr;
    using IEncodePolyhedronInput::GetCellFaceBufferPtr;
    using IEncodePolyhedronInput::GetCellFaceBufferSize;
    using IEncodePolyhedronInput::GetCellFaceOffsetPtr;

    using IEncodeAttributeInput::GetNumberOfPointAttrs;
    using IEncodeAttributeInput::GetPointAttr;
    using IEncodeAttributeInput::GetNumberOfCellAttrs;
    using IEncodeAttributeInput::GetCellAttr;

    // 当前网格的逻辑类型
    virtual MeshType GetMeshType() const = 0;
    // 当前网格是否可按结构化拓扑处理
    virtual bool IsStructuredMesh() const {
        if (GetMeshType() == MeshType::StructuredMesh) {
            return true;
        }

        int axisSize[3]{0, 0, 0};
        return GetStructuredAxisSize(axisSize);
    }
    // 当前网格名称，主要用于报告和调试
    virtual std::string GetName() const = 0;
    // 根据 DataCodec 阶段名返回 adapter 可展示的状态信息
    virtual EncodeAdapterStatusInfo GetEncodeStatusInfo(std::string_view) const { return {}; }

    // 由第三方 adapter 指定 raw type 的编码信息
    bool ResolveCellType(CellTypeRaw, CellTypeCodecEntry& entry) const override {
        entry = {};
        return false;
    }
    // 由第三方 adapter 指定 polynomial-order-dependent cell 的 size 公式
    bool ResolveCellSizeFromPolynomialOrder(CellTypeRaw, std::uint16_t, int&) const override { return false; }
    // 默认映射模式用于没有显式 cell type 流的 adapter
    CellTypeMappingMode GetCellTypeMappingMode() const override { return CellTypeMappingMode::FamilyLocal; }
    // 默认不支持 family-local cell type 编码
    bool EncodeCellTypeFamilyLocal(CellTypeRaw, CellTypeFamilyCode& familyCode, CellTypeLocalCode& familyLocalCode) const override {
        familyCode = 0;
        familyLocalCode = 0;
        return false;
    }
    // 默认不支持 family-local cell type 反解
    bool DecodeCellTypeFamilyLocal(CellTypeFamilyCode, CellTypeLocalCode, CellTypeRaw& rawType) const override {
        rawType = 0;
        return false;
    }
    // 默认不支持 polynomial-order-dependent cell order 编码
    bool EncodeCellPolynomialOrderLocal(CellTypeRaw, std::uint16_t, CellTypeLocalCode& localOrder) const override {
        localOrder = 0;
        return false;
    }
    // 默认不支持 polynomial-order-dependent cell order 反解
    bool DecodeCellPolynomialOrderLocal(CellTypeRaw, CellTypeLocalCode, std::uint16_t& order) const override {
        order = 0;
        return false;
    }

    // 构建 DataCodec 多面体面表输入视图
    virtual bool BuildPolyhedronFaceTableView(PolyhedronFaceTableView& output) const {
        output = {};
        if (!IsPolyhedronMesh()) {
            return false;
        }
        output.cellCount = GetNumberOfCells();
        output.faceCount = GetNumberOfFaces();
        output.pointCount = GetNumberOfPoints();

        const auto* faceOffsets = GetFaceIdOffsetPtr();
        const auto faceVertexCount = GetFaceIdBufferSize();
        output.faceVertexIds = MakeCompactIndexArrayView(
            GetFaceIdBufferPtr(),
            IndexValueType::UInt32,
            faceVertexCount,
            ViewBufferOrigin::Borrowed);
        output.faceVertexOffsets = MakeCompactIndexArrayView(
            faceOffsets,
            IndexValueType::UInt32,
            output.faceCount > 0 ? output.faceCount + 1 : 0,
            ViewBufferOrigin::Borrowed);

        const auto* cellFaceOffsets = GetCellFaceOffsetPtr();
        const auto cellFaceCount = GetCellFaceBufferSize();
        output.cellFaceIds = MakeCompactIndexArrayView(
            GetCellFaceBufferPtr(),
            IndexValueType::UInt32,
            cellFaceCount,
            ViewBufferOrigin::Borrowed);
        output.cellFaceOffsets = MakeCompactIndexArrayView(
            cellFaceOffsets,
            IndexValueType::UInt32,
            output.cellCount > 0 ? output.cellCount + 1 : 0,
            ViewBufferOrigin::Borrowed);

        return output.faceVertexIds.IsValid() &&
            output.faceVertexOffsets.IsValid() &&
            output.cellFaceIds.IsValid() &&
            output.cellFaceOffsets.IsValid();
    }

    // 供报告使用的可选源数据元数据
    virtual std::int64_t GetSourceByteSizeHint() const { return -1; }
    virtual std::string GetSourceLocationHint() const { return {}; }

    // 释放 adapter 为转换视图准备的临时数据
    virtual std::uint64_t ReleaseConvertedInputs() { return 0; }
    // 释放本次编码输入对象和转换视图缓存，保留外部 reference 配置
    virtual void ResetInput() = 0;
    // 编码失败时由 Failure 模块调用，保证在失败路径恰好调用一次
    // 实现者必须释放所有为本次编码分配的临时状态（临时面表、转换缓存等）
    // 调用后 adapter 应回到可安全析构或可重新使用的状态
    virtual void Abort() { ResetInput(); }
};

} // namespace datacodec

#endif
