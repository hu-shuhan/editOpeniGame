#ifndef DATACODEC_API_PARAMS_CODECPERFORMANCEPARAMS_H
#define DATACODEC_API_PARAMS_CODECPERFORMANCEPARAMS_H

#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
namespace datacodec {

enum class DecodeStorageMode : std::uint8_t {
    Managed = 0,
    Memory = 1,
};

enum class EncodeStorageMode : std::uint8_t {
    Managed = 0,
    Memory = 1,
};

enum class AttributeDecodePayloadMode : std::uint8_t {
    Managed = 0,
    Memory = 1,
    OneShotZstd = 2,
};

struct ResourceBudgetControlParams {
    // Memory Store 和长期驻留缓存的总量上限，0 表示不限制
    std::uint64_t residentLimitMiB{0u};
    // 单次窗口化读写的访问粒度，单位 MiB
    std::uint64_t accessWindowMiB{64u};
    // 共享 window budget 允许的活跃预算总量，单位 MiB
    std::uint64_t activeWindowMiB{256u};
    // Scratch Pool 最多留存的块数量
    std::uint32_t scratchRetainedBlockCount{16u};
    // Scratch Pool 单块留存上限，单位 MiB
    std::uint64_t scratchRetainedBlockMiB{64u};
    // Scratch Pool 总留存上限，单位 MiB
    std::uint64_t scratchRetainedTotalMiB{1024u};
    // ATTR 自有 scratch 活跃分配额度，0 表示不单独限制
    std::uint64_t attributeScratchQuotaMiB{256u};
    // Memory staging 的驻留容量上限，0 表示不允许创建 Memory staging
    std::uint64_t attributeMemoryStagingLimitMiB{0u};
    // Managed staging 的在途逻辑数据上限，0 表示不单独限制
    std::uint64_t attributeManagedStagingLogicalLimitMiB{0u};
    // ATTR 解码 raw payload 策略
    AttributeDecodePayloadMode attributeDecodePayloadMode{
        AttributeDecodePayloadMode::Managed};
    // ATTR 解码后 decoded cache 策略
    DecodeStorageMode attributeDecodeCacheMode{DecodeStorageMode::Managed};
    // geometry 解码后 decoded cache 策略
    DecodeStorageMode geometryDecodeCacheMode{DecodeStorageMode::Managed};
    // geometry 关键帧 reference cache 策略
    DecodeStorageMode geometryDecodeReferenceCacheMode{DecodeStorageMode::Managed};
    // topology encoded 输入 staging 策略
    DecodeStorageMode topologyDecodeInputMode{DecodeStorageMode::Managed};
    // topology decoded cache 策略
    DecodeStorageMode topologyDecodeCacheMode{DecodeStorageMode::Managed};
    // topology reference cache 策略
    DecodeStorageMode topologyDecodeReferenceCacheMode{DecodeStorageMode::Managed};
    // geometry 编码 transfer cache 策略
    EncodeStorageMode geometryEncodeTransferCacheMode{EncodeStorageMode::Managed};
    // geometry 编码 staging 策略
    EncodeStorageMode geometryEncodeStagingMode{EncodeStorageMode::Managed};
    // ATTR 编码 transfer cache 策略
    EncodeStorageMode attributeEncodeTransferCacheMode{EncodeStorageMode::Managed};
    // ATTR 编码 staging 策略
    EncodeStorageMode attributeEncodeStagingMode{EncodeStorageMode::Managed};
    // topology 编码最终 transfer cache 策略
    EncodeStorageMode topologyEncodeTransferCacheMode{EncodeStorageMode::Managed};
    // remap 编码中间结果策略
    EncodeStorageMode remapEncodeStorageMode{EncodeStorageMode::Managed};
    // 最终 package field 压缩 staging 策略
    EncodeStorageMode packageFieldStagingMode{EncodeStorageMode::Managed};
    // ATTR 编码关键帧 reference cache 策略
    EncodeStorageMode attributeEncodeReferenceCacheMode{EncodeStorageMode::Managed};
    // Geometry 编码关键帧 reference cache 策略
    EncodeStorageMode geometryEncodeReferenceCacheMode{EncodeStorageMode::Managed};
    // 编码关键帧 reference cache 的驻留总量，0 表示不限制
    std::uint64_t encodeReferenceResidentLimitMiB{0u};
    // ATTR 解码 raw payload 内存 staging 上限，0 表示不允许内存 staging
    std::uint64_t attributeDecodeMemoryPayloadLimitMiB{0u};
    // ATTR 解码后 decoded cache 内存上限，0 表示不允许内存 cache
    std::uint64_t attributeDecodeMemoryCacheLimitMiB{0u};
    // geometry 解码后 decoded cache 内存上限，0 表示不允许内存 cache
    std::uint64_t geometryDecodeMemoryCacheLimitMiB{0u};
    // geometry 关键帧 reference cache 内存上限，0 表示不允许内存 cache
    std::uint64_t geometryDecodeMemoryReferenceLimitMiB{0u};
    // topology encoded 输入内存上限，0 表示不允许内存 input
    std::uint64_t topologyDecodeMemoryInputLimitMiB{0u};
    // topology decoded cache 内存上限，0 表示不允许内存 cache
    std::uint64_t topologyDecodeMemoryCacheLimitMiB{0u};
    // topology reference cache 内存上限，0 表示不允许内存 cache
    std::uint64_t topologyDecodeMemoryReferenceLimitMiB{0u};
    // DataCodec 内部 reference 帧缓存的驻留总量，0 表示不限制驻留字节
    std::uint64_t decodeReferenceResidentLimitMiB{4096u};
    // DataCodec 内部可保留的 reference 帧条目上限，0 表示不限制条目数
    std::uint32_t decodeReferenceFrameLimit{4u};
    // 同时进入 pressio 的 ATTR 任务数
    std::uint32_t attributePressioLanes{1u};
    // 同时运行 reference-heavy ATTR 任务数
    std::uint32_t attributeReferenceLanes{1u};
    // 同时运行 ATTR 解码任务数
    std::uint32_t attributeDecodeLanes{4u};
    // 同时把独立 ATTR cache 写入 Adapter 的任务数
    std::uint32_t attributeCommitLanes{2u};
    // 同时运行拓扑块编解码任务的数量
    std::uint32_t topologyBlockLanes{4u};
    // 每个 topology stream 预留的缓冲估算，单位 MiB
    std::uint64_t topologyStreamBufferMiB{1u};
    // 所有 topology stream buffer 的活跃总量，单位 MiB
    std::uint64_t topologyBufferBudgetMiB{4u};
    // Morton leaf 排序块预算，单位 MiB
    std::uint64_t remapMortonLeafMiB{256u};
    // Morton run 读取缓冲预算，单位 MiB
    std::uint64_t remapMortonRunBufferMiB{8u};
    // Remap 临时数组的活跃总量，单位 MiB
    std::uint64_t remapScratchQuotaMiB{256u};

    ResourceBudgetControlParams& SetCacheWindowMiB(
        const std::uint64_t accessMiB,
        const std::uint64_t activeMiB) noexcept {
        accessWindowMiB = accessMiB;
        activeWindowMiB = activeMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetResidentLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        residentLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetScratchPoolRetention(
        const std::uint32_t blockCount,
        const std::uint64_t blockMiB,
        const std::uint64_t totalMiB) noexcept {
        scratchRetainedBlockCount = blockCount;
        scratchRetainedBlockMiB = blockMiB;
        scratchRetainedTotalMiB = totalMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetAccessWindowMiB(const std::uint64_t valueMiB) noexcept {
        accessWindowMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetActiveWindowMiB(const std::uint64_t valueMiB) noexcept {
        activeWindowMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeScratchQuotaMiB(const std::uint64_t valueMiB) noexcept {
        attributeScratchQuotaMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeMemoryStagingLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        attributeMemoryStagingLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeManagedStagingLogicalLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        attributeManagedStagingLogicalLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeDecodePayloadMode(
        const AttributeDecodePayloadMode mode) noexcept {
        attributeDecodePayloadMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeDecodeCacheMode(
        const DecodeStorageMode mode) noexcept {
        attributeDecodeCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetGeometryDecodeCacheMode(
        const DecodeStorageMode mode) noexcept {
        geometryDecodeCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetGeometryDecodeReferenceCacheMode(
        const DecodeStorageMode mode) noexcept {
        geometryDecodeReferenceCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyDecodeInputMode(
        const DecodeStorageMode mode) noexcept {
        topologyDecodeInputMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyDecodeCacheMode(
        const DecodeStorageMode mode) noexcept {
        topologyDecodeCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyDecodeReferenceCacheMode(
        const DecodeStorageMode mode) noexcept {
        topologyDecodeReferenceCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetGeometryEncodeTransferCacheMode(
        const EncodeStorageMode mode) noexcept {
        geometryEncodeTransferCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetGeometryEncodeStagingMode(
        const EncodeStorageMode mode) noexcept {
        geometryEncodeStagingMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeEncodeTransferCacheMode(
        const EncodeStorageMode mode) noexcept {
        attributeEncodeTransferCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeEncodeStagingMode(
        const EncodeStorageMode mode) noexcept {
        attributeEncodeStagingMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyEncodeTransferCacheMode(
        const EncodeStorageMode mode) noexcept {
        topologyEncodeTransferCacheMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetRemapEncodeStorageMode(
        const EncodeStorageMode mode) noexcept {
        remapEncodeStorageMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetPackageFieldStagingMode(
        const EncodeStorageMode mode) noexcept {
        packageFieldStagingMode = mode;
        return *this;
    }

    ResourceBudgetControlParams& SetEncodeReferenceCacheModes(
        const EncodeStorageMode attributeMode,
        const EncodeStorageMode geometryMode) noexcept {
        attributeEncodeReferenceCacheMode = attributeMode;
        geometryEncodeReferenceCacheMode = geometryMode;
        return *this;
    }

    ResourceBudgetControlParams& SetEncodeReferenceResidentLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        encodeReferenceResidentLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetEncodeStorageModes(
        const EncodeStorageMode attributeTransferCacheMode,
        const EncodeStorageMode attributeStagingStorageMode,
        const EncodeStorageMode remapStorageMode) noexcept {
        attributeEncodeTransferCacheMode = attributeTransferCacheMode;
        attributeEncodeStagingMode = attributeStagingStorageMode;
        remapEncodeStorageMode = remapStorageMode;
        return *this;
    }

    ResourceBudgetControlParams& SetEncodeStorageModes(
        const EncodeStorageMode geometryTransferCacheMode,
        const EncodeStorageMode geometryStagingStorageMode,
        const EncodeStorageMode attributeTransferCacheMode,
        const EncodeStorageMode attributeStagingStorageMode,
        const EncodeStorageMode remapStorageMode) noexcept {
        geometryEncodeTransferCacheMode = geometryTransferCacheMode;
        geometryEncodeStagingMode = geometryStagingStorageMode;
        return SetEncodeStorageModes(
            attributeTransferCacheMode,
            attributeStagingStorageMode,
            remapStorageMode);
    }

    ResourceBudgetControlParams& SetAttributeDecodeMemoryPayloadLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        attributeDecodeMemoryPayloadLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeDecodeMemoryCacheLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        attributeDecodeMemoryCacheLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetGeometryDecodeMemoryCacheLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        geometryDecodeMemoryCacheLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetGeometryDecodeMemoryReferenceLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        geometryDecodeMemoryReferenceLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyDecodeMemoryInputLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        topologyDecodeMemoryInputLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyDecodeMemoryCacheLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        topologyDecodeMemoryCacheLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyDecodeMemoryReferenceLimitMiB(
        const std::uint64_t valueMiB) noexcept {
        topologyDecodeMemoryReferenceLimitMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributePressioLanes(const std::uint32_t value) noexcept {
        attributePressioLanes = value;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeReferenceLanes(const std::uint32_t value) noexcept {
        attributeReferenceLanes = value;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeDecodeLanes(const std::uint32_t value) noexcept {
        attributeDecodeLanes = value;
        return *this;
    }

    ResourceBudgetControlParams& SetAttributeCommitLanes(const std::uint32_t value) noexcept {
        attributeCommitLanes = value;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyBlockLanes(const std::uint32_t value) noexcept {
        topologyBlockLanes = value;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyStreamBufferMiB(const std::uint64_t valueMiB) noexcept {
        topologyStreamBufferMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetDecodeReferenceRetention(
        const std::uint64_t residentLimitMiB,
        const std::uint32_t frameLimit) noexcept {
        decodeReferenceResidentLimitMiB = residentLimitMiB;
        decodeReferenceFrameLimit = frameLimit;
        return *this;
    }

    ResourceBudgetControlParams& SetTopologyBufferBudgetMiB(const std::uint64_t valueMiB) noexcept {
        topologyBufferBudgetMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetRemapMortonLeafMiB(const std::uint64_t valueMiB) noexcept {
        remapMortonLeafMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetRemapMortonRunBufferMiB(const std::uint64_t valueMiB) noexcept {
        remapMortonRunBufferMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetRemapScratchQuotaMiB(const std::uint64_t valueMiB) noexcept {
        remapScratchQuotaMiB = valueMiB;
        return *this;
    }

    ResourceBudgetControlParams& SetRemapBudgetMiB(
        const std::uint64_t mortonLeafMiB,
        const std::uint64_t mortonRunBufferMiB) noexcept {
        remapMortonLeafMiB = mortonLeafMiB;
        remapMortonRunBufferMiB = mortonRunBufferMiB;
        return *this;
    }

    [[nodiscard]] std::size_t AccessWindowBytes() const noexcept {
        return ClampToSize(MiBToBytesAtLeastOne(accessWindowMiB));
    }

    [[nodiscard]] std::uint64_t ActiveWindowBytes() const noexcept {
        return MiBToBytesAtLeastOne(activeWindowMiB);
    }

    [[nodiscard]] std::uint64_t ResidentLimitBytes() const noexcept {
        return MiBToBytesOrUnlimited(residentLimitMiB);
    }

    [[nodiscard]] std::size_t ScratchRetainedBlockCount() const noexcept {
        return static_cast<std::size_t>(scratchRetainedBlockCount);
    }

    [[nodiscard]] std::size_t ScratchRetainedBlockBytes() const noexcept {
        return ClampToSize(MiBToBytes(scratchRetainedBlockMiB));
    }

    [[nodiscard]] std::uint64_t ScratchRetainedTotalBytes() const noexcept {
        return MiBToBytes(scratchRetainedTotalMiB);
    }

    [[nodiscard]] std::uint64_t ScratchRetainedCapacityBytes() const noexcept {
        const auto byBlockCount = validation::SaturatingMulU64(
            static_cast<std::uint64_t>(ScratchRetainedBlockCount()),
            static_cast<std::uint64_t>(ScratchRetainedBlockBytes()));
        return std::min(ScratchRetainedTotalBytes(), byBlockCount);
    }

    [[nodiscard]] std::uint64_t AttributeScratchQuotaBytes() const noexcept {
        return MiBToBytesOrUnlimited(attributeScratchQuotaMiB);
    }

    [[nodiscard]] std::uint64_t AttributeStagingQuotaBytes(
        const EncodeStorageMode mode) const noexcept {
        return mode == EncodeStorageMode::Memory
            ? MiBToBytes(attributeMemoryStagingLimitMiB)
            : MiBToBytesOrUnlimited(attributeManagedStagingLogicalLimitMiB);
    }

    [[nodiscard]] AttributeDecodePayloadMode AttributeDecodePayloadStorageMode() const noexcept {
        return attributeDecodePayloadMode;
    }

    [[nodiscard]] DecodeStorageMode AttributeDecodeCacheStorageMode() const noexcept {
        return attributeDecodeCacheMode;
    }

    [[nodiscard]] DecodeStorageMode GeometryDecodeCacheStorageMode() const noexcept {
        return geometryDecodeCacheMode;
    }

    [[nodiscard]] DecodeStorageMode GeometryDecodeReferenceCacheStorageMode() const noexcept {
        return geometryDecodeReferenceCacheMode;
    }

    [[nodiscard]] DecodeStorageMode TopologyDecodeInputStorageMode() const noexcept {
        return topologyDecodeInputMode;
    }

    [[nodiscard]] DecodeStorageMode TopologyDecodeCacheStorageMode() const noexcept {
        return topologyDecodeCacheMode;
    }

    [[nodiscard]] DecodeStorageMode TopologyDecodeReferenceCacheStorageMode() const noexcept {
        return topologyDecodeReferenceCacheMode;
    }

    [[nodiscard]] EncodeStorageMode GeometryEncodeTransferCacheStorageMode() const noexcept {
        return geometryEncodeTransferCacheMode;
    }

    [[nodiscard]] EncodeStorageMode GeometryEncodeStagingStorageMode() const noexcept {
        return geometryEncodeStagingMode;
    }

    [[nodiscard]] EncodeStorageMode AttributeEncodeTransferCacheStorageMode() const noexcept {
        return attributeEncodeTransferCacheMode;
    }

    [[nodiscard]] EncodeStorageMode AttributeEncodeStagingStorageMode() const noexcept {
        return attributeEncodeStagingMode;
    }

    [[nodiscard]] EncodeStorageMode TopologyEncodeTransferCacheStorageMode() const noexcept {
        return topologyEncodeTransferCacheMode;
    }

    [[nodiscard]] EncodeStorageMode RemapEncodeStorageMode() const noexcept {
        return remapEncodeStorageMode;
    }

    [[nodiscard]] EncodeStorageMode PackageFieldStagingStorageMode() const noexcept {
        return packageFieldStagingMode;
    }

    [[nodiscard]] EncodeStorageMode AttributeEncodeReferenceCacheStorageMode() const noexcept {
        return attributeEncodeReferenceCacheMode;
    }

    [[nodiscard]] EncodeStorageMode GeometryEncodeReferenceCacheStorageMode() const noexcept {
        return geometryEncodeReferenceCacheMode;
    }

    [[nodiscard]] std::uint64_t EncodeReferenceResidentLimitBytes() const noexcept {
        return MiBToBytesOrUnlimited(encodeReferenceResidentLimitMiB);
    }

    [[nodiscard]] std::uint64_t AttributeDecodeMemoryPayloadLimitBytes() const noexcept {
        return MiBToBytes(attributeDecodeMemoryPayloadLimitMiB);
    }

    [[nodiscard]] std::uint64_t AttributeDecodeMemoryCacheLimitBytes() const noexcept {
        return MiBToBytes(attributeDecodeMemoryCacheLimitMiB);
    }

    [[nodiscard]] std::uint64_t GeometryDecodeMemoryCacheLimitBytes() const noexcept {
        return MiBToBytes(geometryDecodeMemoryCacheLimitMiB);
    }

    [[nodiscard]] std::uint64_t GeometryDecodeMemoryReferenceLimitBytes() const noexcept {
        return MiBToBytes(geometryDecodeMemoryReferenceLimitMiB);
    }

    [[nodiscard]] std::uint64_t TopologyDecodeMemoryInputLimitBytes() const noexcept {
        return MiBToBytes(topologyDecodeMemoryInputLimitMiB);
    }

    [[nodiscard]] std::uint64_t TopologyDecodeMemoryCacheLimitBytes() const noexcept {
        return MiBToBytes(topologyDecodeMemoryCacheLimitMiB);
    }

    [[nodiscard]] std::uint64_t TopologyDecodeMemoryReferenceLimitBytes() const noexcept {
        return MiBToBytes(topologyDecodeMemoryReferenceLimitMiB);
    }

    [[nodiscard]] std::uint64_t DecodeReferenceResidentLimitBytes() const noexcept {
        return MiBToBytes(decodeReferenceResidentLimitMiB);
    }

    [[nodiscard]] std::size_t DecodeReferenceFrameLimit() const noexcept {
        return static_cast<std::size_t>(decodeReferenceFrameLimit);
    }

    [[nodiscard]] std::uint32_t AttributePressioLaneCount() const noexcept {
        return std::max<std::uint32_t>(attributePressioLanes, 1u);
    }

    [[nodiscard]] std::uint32_t AttributeReferenceLaneCount() const noexcept {
        return std::max<std::uint32_t>(attributeReferenceLanes, 1u);
    }

    [[nodiscard]] std::uint32_t AttributeDecodeLaneCount() const noexcept {
        return std::max<std::uint32_t>(attributeDecodeLanes, 1u);
    }

    [[nodiscard]] std::uint32_t AttributeCommitLaneCount() const noexcept {
        return std::max<std::uint32_t>(attributeCommitLanes, 1u);
    }

    [[nodiscard]] std::uint32_t TopologyBlockLaneCount() const noexcept {
        return std::max<std::uint32_t>(topologyBlockLanes, 1u);
    }

    [[nodiscard]] std::uint64_t TopologyStreamBufferBytes() const noexcept {
        return MiBToBytes(topologyStreamBufferMiB);
    }

    [[nodiscard]] std::uint64_t TopologyBufferBudgetBytes() const noexcept {
        return MiBToBytesAtLeastOne(topologyBufferBudgetMiB);
    }

    [[nodiscard]] std::size_t RemapMortonLeafBytes() const noexcept {
        return ClampToSize(MiBToBytesAtLeastOne(remapMortonLeafMiB));
    }

    [[nodiscard]] std::size_t RemapMortonRunBufferBytes() const noexcept {
        return ClampToSize(MiBToBytesAtLeastOne(remapMortonRunBufferMiB));
    }

    [[nodiscard]] std::uint64_t RemapScratchQuotaBytes() const noexcept {
        return MiBToBytesAtLeastOne(remapScratchQuotaMiB);
    }

private:
    static constexpr std::uint64_t kResourceBudgetBytesPerMiB = 1024u * 1024u;

    [[nodiscard]] static std::uint64_t MiBToBytes(const std::uint64_t valueMiB) noexcept {
        return validation::SaturatingMulU64(valueMiB, kResourceBudgetBytesPerMiB);
    }

    [[nodiscard]] static std::uint64_t MiBToBytesAtLeastOne(const std::uint64_t valueMiB) noexcept {
        return MiBToBytes(std::max<std::uint64_t>(1u, valueMiB));
    }

    [[nodiscard]] static std::uint64_t MiBToBytesOrUnlimited(const std::uint64_t valueMiB) noexcept {
        return valueMiB == 0u ? std::numeric_limits<std::uint64_t>::max() : MiBToBytes(valueMiB);
    }

    [[nodiscard]] static std::size_t ClampToSize(const std::uint64_t value) noexcept {
        return static_cast<std::size_t>(
            std::min<std::uint64_t>(
                value,
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())));
    }
};

inline bool ValidateResourceBudgetControlParams(
    const ResourceBudgetControlParams& params,
    std::string* error = nullptr) {
    if (params.AccessWindowBytes() > params.ActiveWindowBytes()) {
        return validation::AssignError(
            error,
            "resource access window exceeds the active window budget");
    }
    if (params.TopologyStreamBufferBytes() > params.TopologyBufferBudgetBytes()) {
        return validation::AssignError(
            error,
            "topology stream buffer exceeds the topology buffer budget");
    }
    constexpr std::uint64_t kTopologyLogicalStreamBuffersPerBlock = 4u;
    if (validation::SaturatingMulU64(
            params.TopologyStreamBufferBytes(),
            kTopologyLogicalStreamBuffersPerBlock) > params.TopologyBufferBudgetBytes()) {
        return validation::AssignError(
            error,
            "topology buffer budget cannot hold one complete block encoder");
    }
    if (params.RemapMortonLeafBytes() > params.RemapScratchQuotaBytes()) {
        return validation::AssignError(
            error,
            "remap Morton leaf exceeds the remap scratch budget");
    }
    if (params.RemapMortonRunBufferBytes() > params.ActiveWindowBytes()) {
        return validation::AssignError(
            error,
            "remap Morton run buffer exceeds the active window budget");
    }
    if (params.ScratchRetainedBlockCount() != 0u &&
        params.ScratchRetainedBlockBytes() > params.ScratchRetainedTotalBytes()) {
        return validation::AssignError(
            error,
            "scratch retained block exceeds the scratch retained total budget");
    }
    if (params.AttributeEncodeStagingStorageMode() == EncodeStorageMode::Memory &&
        params.AttributeStagingQuotaBytes(EncodeStorageMode::Memory) == 0u) {
        return validation::AssignError(
            error,
            "attribute memory staging requires a positive resident limit");
    }
    if (params.GeometryDecodeCacheStorageMode() == DecodeStorageMode::Memory &&
        params.GeometryDecodeMemoryCacheLimitBytes() == 0u) {
        return validation::AssignError(
            error,
            "geometry memory cache requires a positive local limit");
    }
    if (params.GeometryDecodeReferenceCacheStorageMode() == DecodeStorageMode::Memory &&
        params.GeometryDecodeMemoryReferenceLimitBytes() == 0u) {
        return validation::AssignError(
            error,
            "geometry memory reference cache requires a positive local limit");
    }
    if (params.TopologyDecodeInputStorageMode() == DecodeStorageMode::Memory &&
        params.TopologyDecodeMemoryInputLimitBytes() == 0u) {
        return validation::AssignError(
            error,
            "topology memory input requires a positive local limit");
    }
    if (params.TopologyDecodeCacheStorageMode() == DecodeStorageMode::Memory &&
        params.TopologyDecodeMemoryCacheLimitBytes() == 0u) {
        return validation::AssignError(
            error,
            "topology memory cache requires a positive local limit");
    }
    if (params.TopologyDecodeReferenceCacheStorageMode() == DecodeStorageMode::Memory &&
        params.TopologyDecodeMemoryReferenceLimitBytes() == 0u) {
        return validation::AssignError(
            error,
            "topology memory reference cache requires a positive local limit");
    }
    return true;
}


} // namespace datacodec

#endif
