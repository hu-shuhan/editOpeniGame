#ifndef DATACODEC_API_ADAPTER_IDECODEADAPTER_H
#define DATACODEC_API_ADAPTER_IDECODEADAPTER_H

#include "DataCodec/Common/Views/TopologyViews.h"
#include "DataCodec/API/Adapter/ICellTypeMapping.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
namespace datacodec {

namespace bytestore {
class IRandomAccessByteStore;
}

struct IDecodeAdapter : public ICellTypeMapping {
    virtual ~IDecodeAdapter() = default;

    // 创建或切换目标原生网格对象类型
    // 解码流水线会先调用它，再填充几何、拓扑和属性
    virtual bool SetMeshType(MeshType type, std::string* error = nullptr) = 0;
    // 开始按 range 写入点坐标
    virtual bool BeginPoints(std::size_t count, std::size_t dimension, std::string* error = nullptr) = 0;
    // 写入一段点坐标
    virtual bool WritePointsRange(
        std::size_t offset,
        std::size_t count,
        const float* data,
        std::string* error = nullptr) = 0;
    // 结束点坐标写入
    virtual bool EndPoints(std::string* error = nullptr) = 0;
    // 开始按 range 写入普通拓扑
    virtual bool BeginTopology(
        std::size_t cellCount,
        std::size_t connectivityCount,
        bool hasOffsets,
        std::string* error = nullptr) = 0;
    // 写入 connectivity range
    virtual bool WriteConnectivityRange(
        std::size_t offset,
        const IndexType* data,
        std::size_t count,
        std::string* error = nullptr) = 0;
    // 写入 offsets range
    virtual bool WriteOffsetsRange(
        std::size_t offset,
        const IndexType* data,
        std::size_t count,
        std::string* error = nullptr) = 0;
    // 写入 cell type range
    virtual bool WriteCellTypesRange(
        std::size_t offset,
        const IndexType* data,
        std::size_t count,
        std::string* error = nullptr) = 0;
    // 写入 cell polynomial order range
    virtual bool WriteCellPolynomialOrdersRange(
        std::size_t offset,
        const std::uint16_t* data,
        std::size_t count,
        std::string* error = nullptr) = 0;
    // 结束普通拓扑 range 写入
    virtual bool EndTopology(std::string* error = nullptr) = 0;
    // 根据逻辑轴尺寸写入 structured 拓扑
    virtual bool SetStructuredAxisSize(const int size[3], std::string* error = nullptr) = 0;
    // 默认映射模式用于不接收显式 cell type 流的 adapter
    CellTypeMappingMode GetCellTypeMappingMode() const override { return CellTypeMappingMode::FamilyLocal; }
    // 默认不支持 raw type 解析
    bool ResolveCellType(CellTypeRaw, CellTypeCodecEntry& entry) const override {
        entry = {};
        return false;
    }
    // 默认不支持 polynomial-order-dependent cell size 解析
    bool ResolveCellSizeFromPolynomialOrder(CellTypeRaw, std::uint16_t, int& size) const override {
        size = -1;
        return false;
    }
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
    // 是否支持 polyhedron 专用组装路径
    virtual bool SupportsPolyhedronTopology() const { return false; }
    // 开始按 chunk 写入 polyhedron 拓扑
    virtual bool BeginPolyhedronTopology(std::size_t cellCount, std::string* error = nullptr) = 0;
    // 写入一段 polyhedron cell batch
    virtual bool WritePolyhedronCellBatch(
        std::size_t firstCell,
        const PolyhedronTopologyView& batch,
        std::string* error = nullptr) = 0;
    // 结束 polyhedron chunk 写入
    virtual bool EndPolyhedronTopology(std::string* error = nullptr) = 0;
    // 开始按 range 写入属性
    virtual bool BeginAttribute(std::size_t attrIndex, const AttrStorageParams& meta, std::string* error = nullptr) = 0;
    // 写入一段属性原始字节
    virtual bool WriteAttributeRange(
        std::size_t attrIndex,
        std::size_t offset,
        std::size_t count,
        const void* data,
        std::size_t byteSize,
        std::string* error = nullptr) = 0;
    // BeginAttribute 全部完成后，不同属性的 range 是否允许并发写入
    [[nodiscard]] virtual bool SupportsConcurrentAttributeRangeWrites() const noexcept {
        return false;
    }
    // 是否允许 DataCodec 直接把解码缓存放入 Adapter 的原生属性存储
    [[nodiscard]] virtual bool SupportsAttributeDecodeStore() const noexcept {
        return false;
    }
    // 创建可读写的原生属性存储
    // 返回空指针表示创建失败
    virtual std::shared_ptr<bytestore::IRandomAccessByteStore> CreateAttributeDecodeStore(
        std::size_t attrIndex,
        const AttrStorageParams& meta,
        std::string* error = nullptr) {
        (void)attrIndex;
        (void)meta;
        (void)error;
        return nullptr;
    }
    // 结束属性写入并挂接到目标对象
    virtual bool EndAttribute(std::size_t attrIndex, std::string* error = nullptr) = 0;
    // 当前后端已经常驻原生对象的逻辑字节估计
    virtual std::uint64_t NativeResidentBytesHint() const { return 0u; }
    // 解码失败时由 Failure 模块调用，保证在失败路径恰好调用一次
    // 实现者必须丢弃所有已写入的半成品数据，释放中间状态
    // 调用后 adapter 应回到可安全析构或可重新使用的状态，不得调用 Commit
    virtual void Abort() { ResetOutput(); }
    // 释放本次解码输出对象和半成品状态，保留外部 reference 配置
    virtual void ResetOutput() = 0;
    // 提交解码结果并让输出对调用方可见
    virtual bool Commit(std::string* error = nullptr) = 0;
};

} // namespace datacodec

#endif
