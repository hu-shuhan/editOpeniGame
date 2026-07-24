#ifndef DATACODEC_API_PARAMS_CODECPERFORMANCEPRESETPARAMS_H
#define DATACODEC_API_PARAMS_CODECPERFORMANCEPRESETPARAMS_H

#include "DataCodec/API/Adapter/IDecodeTopologyBlockObserver.h"
#include "DataCodec/API/Params/CodecParamDefaults.h"
#include "DataCodec/API/Params/DataCodecControlParams.h"
#include "DataCodec/API/Params/DecodedFrameCacheParams.h"
#include "DataCodec/API/Params/EncodedInputCacheParams.h"
#include "DataCodec/API/Params/EncodePipelineParams.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
namespace datacodec {

enum class DataCodecEncodeTier {
    TimePriority,
    Balanced,
    MemoryPriority
};

enum class DataCodecDecodeTier {
    Fast,
    Balanced,
    LowMemory
};

enum class DataCodecDecodeValidationProfile {
    Required,
    Audit
};

// 每个档位分支的注释依次说明流程语义、资源策略和运行环境约束
// 这些分类只用于维护说明，不形成额外类型或运行期分派

// Runtime Profile 表达运行环境硬约束
// 它在性能配置之后限制存储模式、窗口和并行度
enum class DataCodecRuntimeProfile {
    Native,
    Wasm4GiB,
    Wasm16GiB
};

struct DataCodecEncodeOptions {
    DataCodecEncodeTier tier{DataCodecEncodeTier::Balanced};
    bool enableCompressionEnhancement{false};
    std::optional<int> packageZstdLevel;
    std::optional<std::uint32_t> temporalKeyFrameInterval;
};

struct DataCodecDecodeOptions {
    DataCodecDecodeTier tier{DataCodecDecodeTier::Balanced};
    DataCodecDecodeValidationProfile validationProfile{
        DataCodecDecodeValidationProfile::Required};
    // 未指定时沿用性能档位的默认解码结果缓存策略
    // true 或 false 时明确覆盖该默认策略
    std::optional<bool> enableDecodedResultCache;
    // 未指定时沿用性能档位的默认完整帧缓存容量
    std::optional<std::size_t> decodedResultCacheFrameLimit;
    // 未指定时沿用性能档位的完整输入预读策略
    std::optional<bool> enableFullInputPrefetch;
};

struct DataCodecEncodeConfigurationSource {
    DataCodecEncodeTier performanceTier{DataCodecEncodeTier::Balanced};
    DataCodecRuntimeProfile runtimeProfile{DataCodecRuntimeProfile::Native};
    bool compressionEnhancementEnabled{false};
    bool customControlParams{false};
};

struct DataCodecDecodeConfigurationSource {
    DataCodecDecodeTier performanceTier{DataCodecDecodeTier::Balanced};
    DataCodecRuntimeProfile runtimeProfile{DataCodecRuntimeProfile::Native};
    DataCodecDecodeValidationProfile validationProfile{
        DataCodecDecodeValidationProfile::Required};
    bool customControlParams{false};
};

struct EncodeExecutionOptions {
    bool enableParallelStages{true};
};

enum class TopologyDecodeOutputMode : std::uint8_t {
    CommitToAdapter = 0,
    ObserverOnly = 1,
};

struct DecodeExecutionOptions {
    bool enableParallelStages{true};
    bool enableFullInputPrefetch{false};
    TopologyDecodeOutputMode topologyOutputMode{TopologyDecodeOutputMode::CommitToAdapter};
    std::shared_ptr<IDecodeTopologyBlockObserver> topologyBlockObserver;
};

struct DataCodecEncodeConfigurationParams {
    EncodeCodecControlParams controlParams;
    EncodePipelineControlParams pipelineControl;
    EncodeExecutionOptions execution;
    DataCodecEncodeConfigurationSource source;
};

struct DataCodecDecodeConfigurationParams {
    DecodeControlParams controlParams;
    DecodeExecutionOptions execution;
    // 完整帧与编码输入缓存分别表达两种不同的数据驻留形式
    DecodedFrameCachePolicy decodedFrameCachePolicy;
    EncodedInputCachePolicy encodedInputCachePolicy;
    DataCodecDecodeConfigurationSource source;
};

[[nodiscard]] inline const char* DataCodecEncodeTierName(
    const DataCodecEncodeTier tier) noexcept {
    switch (tier) {
        case DataCodecEncodeTier::TimePriority:
            return "TimePriority";
        case DataCodecEncodeTier::Balanced:
            return "Balanced";
        case DataCodecEncodeTier::MemoryPriority:
            return "MemoryPriority";
    }
    return "Balanced";
}

[[nodiscard]] inline const char* DataCodecDecodeTierName(
    const DataCodecDecodeTier tier) noexcept {
    switch (tier) {
        case DataCodecDecodeTier::Fast:
            return "Fast";
        case DataCodecDecodeTier::Balanced:
            return "Balanced";
        case DataCodecDecodeTier::LowMemory:
            return "LowMemory";
    }
    return "Balanced";
}

[[nodiscard]] inline const char* DataCodecRuntimeProfileName(
    const DataCodecRuntimeProfile profile) noexcept {
    switch (profile) {
        case DataCodecRuntimeProfile::Native:
            return "Native";
        case DataCodecRuntimeProfile::Wasm4GiB:
            return "Wasm4GiB";
        case DataCodecRuntimeProfile::Wasm16GiB:
            return "Wasm16GiB";
    }
    return "Native";
}

class CodecControlParamsFactory {
public:
    [[nodiscard]] static EncodeCodecControlParams MakeDefault() {
        return MakeDefaultCodecControlParams();
    }

    [[nodiscard]] static DataCodecEncodeConfigurationParams MakeEncodeConfiguration(
        const DataCodecEncodeOptions& options,
        const DataCodecRuntimeProfile runtimeProfile = DataCodecRuntimeProfile::Native) {
        DataCodecEncodeConfigurationParams result;
        result.controlParams = MakeDefault();
        const bool compressionEnhancementEnabled = options.enableCompressionEnhancement;
        switch (options.tier) {
            case DataCodecEncodeTier::TimePriority:
                // Point 使用 Morton 顺序，Cell 保持原序
                // Cell 属性可独立编码，Topology 等待 Point Morton 完成
                // 大型中间结果优先驻留内存，Memory Store 上限覆盖实测 3.1 GiB 峰值
                // Native 时间档使用 4 路 ATTR 为单线程 Topology 保留 CPU，最终 Zstd 使用 8 个 worker
                // Remap 预算覆盖 3800 万单元的 Morton 内存路径
                ApplyEncodeBaseControl(result.controlParams);
                ApplyEncodeTimePriorityStorageControl(result.controlParams);
                result.controlParams.resourceBudget.SetResidentLimitMiB(4096u);
                result.controlParams.resourceBudget.SetEncodeReferenceResidentLimitMiB(2048u);
                result.controlParams.resourceBudget.SetScratchPoolRetention(4u, 64u, 256u);
                result.controlParams.resourceBudget.SetActiveWindowMiB(512u);
                result.controlParams.resourceBudget.SetAttributeScratchQuotaMiB(256u);
                result.controlParams.resourceBudget.SetAttributeMemoryStagingLimitMiB(256u);
                result.controlParams.resourceBudget.SetAttributePressioLanes(4u);
                result.controlParams.resourceBudget.SetAttributeReferenceLanes(4u);
                result.controlParams.resourceBudget.SetRemapMortonLeafMiB(512u);
                result.controlParams.resourceBudget.SetRemapMortonRunBufferMiB(64u);
                result.controlParams.resourceBudget.SetRemapScratchQuotaMiB(1024u);
                result.pipelineControl.pointOrder = EncodePointOrderMode::Morton;
                result.pipelineControl.cellOrder = EncodeCellOrderMode::Original;
                result.pipelineControl.packageFields.zstdLevel = 1;
                result.pipelineControl.packageFields.workerCount = 8u;
                break;
            case DataCodecEncodeTier::Balanced:
                // Point Morton 完成后允许 Point 属性先行编码，Cell 保持原序
                // Cell Morton 在目标数据上只减少约 1% 输出并显著延长 Topology，因此不参与平衡档
                // 大型中间结果使用 Managed Store，顺序追加由 Stream Store 承担
                // 标准档使用 2 GiB 资源包络
                // Native 标准档使用 4 路 ATTR、Zstd 3 和中等窗口
                ApplyEncodeBaseControl(result.controlParams);
                result.controlParams.resourceBudget.SetResidentLimitMiB(2048u);
                result.controlParams.resourceBudget.SetEncodeReferenceResidentLimitMiB(512u);
                result.controlParams.resourceBudget.SetScratchPoolRetention(4u, 64u, 256u);
                result.controlParams.resourceBudget.SetActiveWindowMiB(1600u);
                result.controlParams.resourceBudget.SetAttributeScratchQuotaMiB(1024u);
                result.controlParams.resourceBudget.SetAttributeManagedStagingLogicalLimitMiB(1024u);
                result.controlParams.resourceBudget.SetAttributePressioLanes(4u);
                result.controlParams.resourceBudget.SetAttributeReferenceLanes(4u);
                result.controlParams.resourceBudget.SetRemapMortonLeafMiB(512u);
                result.controlParams.resourceBudget.SetRemapMortonRunBufferMiB(64u);
                result.controlParams.resourceBudget.SetRemapScratchQuotaMiB(1024u);
                result.pipelineControl.pointOrder = EncodePointOrderMode::Morton;
                result.pipelineControl.cellOrder = EncodeCellOrderMode::Original;
                result.pipelineControl.packageFields.zstdLevel = 3;
                result.pipelineControl.packageFields.workerCount = 4u;
                break;
            case DataCodecEncodeTier::MemoryPriority:
                // Point 使用 Morton 顺序，Cell 保持原序，所有大型中间结果使用 Managed Store
                // Stage 保持并行，2 路 ATTR 和较小窗口限制 DataCodec 活跃内存
                ApplyEncodeLowMemoryControl(result.controlParams);
                result.pipelineControl.pointOrder = EncodePointOrderMode::Morton;
                result.pipelineControl.cellOrder = EncodeCellOrderMode::Original;
                result.pipelineControl.packageFields.zstdLevel = 1;
                result.pipelineControl.packageFields.workerCount = 1u;
                break;
        }
        if (compressionEnhancementEnabled) {
            ApplyEncodeCompressionEnhancement(result);
        }
        ApplyEncodeRuntimeProfile(result, runtimeProfile);
        if (options.packageZstdLevel.has_value()) {
            result.pipelineControl.packageFields.zstdLevel = *options.packageZstdLevel;
        }
        if (options.temporalKeyFrameInterval.has_value() &&
            *options.temporalKeyFrameInterval > 0u) {
            result.controlParams.attrReference.temporalField.keyFrameInterval =
                *options.temporalKeyFrameInterval;
            result.controlParams.geometryReference.temporalField.keyFrameInterval =
                *options.temporalKeyFrameInterval;
            result.controlParams.attrReference.temporalField.forcePredFrames = false;
            result.controlParams.geometryReference.temporalField.forcePredFrames = false;
        }
        result.source = DataCodecEncodeConfigurationSource{
            .performanceTier = options.tier,
            .runtimeProfile = runtimeProfile,
            .compressionEnhancementEnabled = compressionEnhancementEnabled,
            .customControlParams =
                options.packageZstdLevel.has_value() ||
                options.temporalKeyFrameInterval.has_value(),
        };
        return result;
    }

    [[nodiscard]] static DataCodecDecodeConfigurationParams MakeDecodeConfiguration(
        const DataCodecDecodeOptions& options,
        const DataCodecRuntimeProfile runtimeProfile = DataCodecRuntimeProfile::Native) {
        DataCodecDecodeConfigurationParams result;
        auto& params = result.controlParams;
        switch (options.tier) {
            case DataCodecDecodeTier::Fast:
                // 完整帧 Cache 优先使用内存，ByteStoreSession 统一限制为 16 GiB
                // ATTR 阶段在调用线程调度内部并行，Geometry 和 Topology 使用外部线程池
                // 16 路 ATTR 与外部执行器并发能力取较小值
                params.resourceBudget.SetResidentLimitMiB(16384u);
                params.resourceBudget.SetScratchPoolRetention(16u, 64u, 1024u);
                params.resourceBudget.SetDecodeReferenceRetention(12288u, 12u);
                params.resourceBudget.SetAccessWindowMiB(512u);
                params.resourceBudget.SetActiveWindowMiB(4096u);
                params.resourceBudget.SetAttributeDecodePayloadMode(AttributeDecodePayloadMode::OneShotZstd);
                params.resourceBudget.SetAttributeDecodeCacheMode(DecodeStorageMode::Memory);
                params.resourceBudget.SetGeometryDecodeCacheMode(DecodeStorageMode::Memory);
                params.resourceBudget.SetGeometryDecodeReferenceCacheMode(DecodeStorageMode::Memory);
                params.resourceBudget.SetTopologyDecodeInputMode(DecodeStorageMode::Memory);
                params.resourceBudget.SetTopologyDecodeCacheMode(DecodeStorageMode::Memory);
                params.resourceBudget.SetTopologyDecodeReferenceCacheMode(DecodeStorageMode::Memory);
                params.resourceBudget.SetAttributeDecodeMemoryPayloadLimitMiB(12288u);
                params.resourceBudget.SetAttributeDecodeMemoryCacheLimitMiB(12288u);
                params.resourceBudget.SetGeometryDecodeMemoryCacheLimitMiB(4096u);
                params.resourceBudget.SetGeometryDecodeMemoryReferenceLimitMiB(4096u);
                params.resourceBudget.SetTopologyDecodeMemoryInputLimitMiB(4096u);
                params.resourceBudget.SetTopologyDecodeMemoryCacheLimitMiB(4096u);
                params.resourceBudget.SetTopologyDecodeMemoryReferenceLimitMiB(4096u);
                params.resourceBudget.SetAttributeDecodeLanes(16u);
                params.resourceBudget.SetAttributeCommitLanes(4u);
                result.execution.enableParallelStages = true;
                // 完整帧会消费整个输入包，提前提交映射预取以覆盖首次读取延迟
                result.execution.enableFullInputPrefetch = true;
                break;
            case DataCodecDecodeTier::Balanced:
                // Cache 默认使用 Managed Store，Memory Store 总量限制为 4 GiB
                params.resourceBudget.SetResidentLimitMiB(4096u);
                params.resourceBudget.SetScratchPoolRetention(4u, 64u, 256u);
                params.resourceBudget.SetDecodeReferenceRetention(2048u, 4u);
                params.resourceBudget.SetAccessWindowMiB(256u);
                params.resourceBudget.SetActiveWindowMiB(1024u);
                params.resourceBudget.SetAttributeDecodeLanes(4u);
                params.resourceBudget.SetAttributeCommitLanes(2u);
                break;
            case DataCodecDecodeTier::LowMemory:
                // 大型 Cache 使用 Managed Store，Memory Store 总量限制为 512 MiB
                params.resourceBudget.SetResidentLimitMiB(512u);
                params.resourceBudget.SetScratchPoolRetention(1u, 32u, 32u);
                params.resourceBudget.SetDecodeReferenceRetention(256u, 2u);
                params.resourceBudget.SetAccessWindowMiB(128u);
                params.resourceBudget.SetActiveWindowMiB(256u);
                params.resourceBudget.SetAttributeDecodeLanes(1u);
                params.resourceBudget.SetAttributeCommitLanes(1u);
                result.execution.enableParallelStages = false;
                break;
        }
        if (options.validationProfile == DataCodecDecodeValidationProfile::Audit) {
            params.SetDecodeValidationMode(DecodeValidationMode::Strict);
            params.validation.validateTopologyReferences = true;
            params.validation.validateFloatingPointValues = true;
        }
        ApplyDecodeRuntimeProfile(result, runtimeProfile);
        if (options.enableFullInputPrefetch.has_value()) {
            result.execution.enableFullInputPrefetch = *options.enableFullInputPrefetch;
        }
        const bool defaultDecodedResultCache = !(
            runtimeProfile != DataCodecRuntimeProfile::Native &&
            options.tier == DataCodecDecodeTier::LowMemory);
        result.decodedFrameCachePolicy.enabled =
            options.enableDecodedResultCache.value_or(defaultDecodedResultCache);
        if (options.decodedResultCacheFrameLimit.has_value()) {
            result.decodedFrameCachePolicy.residentFrameLimit =
                *options.decodedResultCacheFrameLimit;
        }
        if (runtimeProfile != DataCodecRuntimeProfile::Native &&
            options.tier == DataCodecDecodeTier::LowMemory) {
            // Wasm 低内存读取保留编码输入，不保留完整解码帧
            // 这两个默认策略二选一，调用方仍可按自己的缓存实现重新组合
            result.encodedInputCachePolicy.enabled = true;
            result.encodedInputCachePolicy.residentInputLimit = 3u;
            result.encodedInputCachePolicy.residentLimitBytes =
                runtimeProfile == DataCodecRuntimeProfile::Wasm4GiB
                    ? 768u * 1024u * 1024u
                    : 2u * 1024u * 1024u * 1024u;
        }
        result.source = DataCodecDecodeConfigurationSource{
            .performanceTier = options.tier,
            .runtimeProfile = runtimeProfile,
            .validationProfile = options.validationProfile,
            .customControlParams =
                options.enableDecodedResultCache.has_value() ||
                options.decodedResultCacheFrameLimit.has_value() ||
                options.enableFullInputPrefetch.has_value(),
        };
        return result;
    }

    static void ApplyEncodeRuntimeConstraint(
        DataCodecEncodeConfigurationParams& configuration,
        const DataCodecRuntimeProfile runtimeProfile) noexcept {
        ApplyEncodeRuntimeProfile(configuration, runtimeProfile);
    }

    static void ApplyDecodeRuntimeConstraint(
        DataCodecDecodeConfigurationParams& configuration,
        const DataCodecRuntimeProfile runtimeProfile) noexcept {
        ApplyDecodeRuntimeProfile(configuration, runtimeProfile);
    }

    static bool ValidateEncodeRuntimeConstraint(
        const ResourceBudgetControlParams& params,
        const DataCodecRuntimeProfile runtimeProfile,
        std::string* error = nullptr) {
        const auto capacityBytes = RuntimeProfileCapacityBytes(runtimeProfile);
        if (capacityBytes == std::numeric_limits<std::uint64_t>::max()) {
            return true;
        }
        std::uint64_t upperBoundBytes = 0u;
        AddRuntimeBound(upperBoundBytes, params.ResidentLimitBytes());
        AddRuntimeBound(upperBoundBytes, params.EncodeReferenceResidentLimitBytes());
        AddRuntimeBound(upperBoundBytes, params.ActiveWindowBytes());
        AddRuntimeBound(upperBoundBytes, params.AttributeScratchQuotaBytes());
        AddRuntimeBound(upperBoundBytes, params.RemapScratchQuotaBytes());
        AddRuntimeBound(upperBoundBytes, params.TopologyBufferBudgetBytes());
        AddRuntimeBound(upperBoundBytes, params.ScratchRetainedCapacityBytes());
        if (upperBoundBytes > capacityBytes) {
            return validation::AssignError(
                error,
                "encode resource upper bound exceeds the runtime profile capacity");
        }
        return true;
    }

    static bool ValidateDecodeRuntimeConstraint(
        const ResourceBudgetControlParams& params,
        const DataCodecRuntimeProfile runtimeProfile,
        std::string* error = nullptr) {
        const auto capacityBytes = RuntimeProfileCapacityBytes(runtimeProfile);
        if (capacityBytes == std::numeric_limits<std::uint64_t>::max()) {
            return true;
        }
        std::uint64_t upperBoundBytes = 0u;
        AddRuntimeBound(upperBoundBytes, params.ResidentLimitBytes());
        AddRuntimeBound(upperBoundBytes, params.DecodeReferenceResidentLimitBytes());
        AddRuntimeBound(upperBoundBytes, params.ActiveWindowBytes());
        AddRuntimeBound(upperBoundBytes, params.ScratchRetainedCapacityBytes());
        if (upperBoundBytes > capacityBytes) {
            return validation::AssignError(
                error,
                "decode resource upper bound exceeds the runtime profile capacity");
        }
        return true;
    }

private:
    [[nodiscard]] static constexpr std::uint64_t RuntimeProfileCapacityBytes(
        const DataCodecRuntimeProfile profile) noexcept {
        constexpr std::uint64_t kMiB = 1024u * 1024u;
        switch (profile) {
            case DataCodecRuntimeProfile::Native:
                return std::numeric_limits<std::uint64_t>::max();
            case DataCodecRuntimeProfile::Wasm4GiB:
                return 4096ull * kMiB;
            case DataCodecRuntimeProfile::Wasm16GiB:
                return 16384ull * kMiB;
        }
        return std::numeric_limits<std::uint64_t>::max();
    }

    static void AddRuntimeBound(
        std::uint64_t& totalBytes,
        const std::uint64_t valueBytes) noexcept {
        totalBytes = validation::SaturatingAddU64(totalBytes, valueBytes);
    }

    static void ApplyEncodeBaseControl(EncodeCodecControlParams& params) noexcept {
        params.resourceBudget.SetAccessWindowMiB(128u);
        params.resourceBudget.SetActiveWindowMiB(1600u);
        params.resourceBudget.SetAttributeScratchQuotaMiB(1024u);
        params.resourceBudget.SetAttributeManagedStagingLogicalLimitMiB(1024u);
        params.resourceBudget.SetAttributePressioLanes(4u);
        params.resourceBudget.SetAttributeReferenceLanes(4u);
        params.resourceBudget.SetAttributeDecodeLanes(4u);
        params.resourceBudget.SetTopologyStreamBufferMiB(1u);
        // 每个拓扑块编码器同时持有四个逻辑流缓冲，16 MiB 支持四路块并行
        params.resourceBudget.SetTopologyBufferBudgetMiB(16u);
        params.resourceBudget.SetTopologyBlockLanes(4u);
        params.resourceBudget.SetRemapMortonLeafMiB(512u);
        params.resourceBudget.SetRemapMortonRunBufferMiB(64u);
        params.resourceBudget.SetRemapScratchQuotaMiB(1024u);
        params.SetSpatialBlockElementCounts(262144u, 262144u);
    }

    static void ApplyEncodeTimePriorityStorageControl(EncodeCodecControlParams& params) noexcept {
        params.resourceBudget.SetEncodeStorageModes(
            EncodeStorageMode::Memory,
            EncodeStorageMode::Memory,
            EncodeStorageMode::Memory,
            EncodeStorageMode::Memory,
            EncodeStorageMode::Memory);
        params.resourceBudget.SetTopologyEncodeTransferCacheMode(EncodeStorageMode::Memory);
        params.resourceBudget.SetPackageFieldStagingMode(EncodeStorageMode::Memory);
        params.resourceBudget.SetEncodeReferenceCacheModes(
            EncodeStorageMode::Memory,
            EncodeStorageMode::Memory);
    }

    static void ApplyEncodeCompressionEnhancement(
        DataCodecEncodeConfigurationParams& result) noexcept {
        // 压缩率增强只改变重排与预测搜索，不改变资源预算和 ZSTD 配置
        result.controlParams.attrReference.temporalField.predictor.enableLocalWindowSearch = true;
        result.controlParams.attrReference.temporalField.predictor.searchStrategy =
            TemporalPredictorSearchStrategy::ExhaustiveEstimatedBytes;
        result.controlParams.geometryReference.temporalField.predictor.enableLocalWindowSearch = true;
        result.controlParams.geometryReference.temporalField.predictor.searchStrategy =
            TemporalPredictorSearchStrategy::ExhaustiveEstimatedBytes;
        result.pipelineControl.cellOrder = EncodeCellOrderMode::Morton;
    }

    static void ApplyEncodeLowMemoryControl(EncodeCodecControlParams& params) noexcept {
        params.resourceBudget.SetEncodeStorageModes(
            EncodeStorageMode::Managed,
            EncodeStorageMode::Managed,
            EncodeStorageMode::Managed,
            EncodeStorageMode::Managed,
            EncodeStorageMode::Managed);
        params.resourceBudget.SetTopologyEncodeTransferCacheMode(EncodeStorageMode::Managed);
        params.resourceBudget.SetPackageFieldStagingMode(EncodeStorageMode::Managed);
        params.resourceBudget.SetEncodeReferenceCacheModes(
            EncodeStorageMode::Managed,
            EncodeStorageMode::Managed);
        params.resourceBudget.SetResidentLimitMiB(512u);
        params.resourceBudget.SetEncodeReferenceResidentLimitMiB(256u);
        params.resourceBudget.SetScratchPoolRetention(2u, 32u, 64u);
        params.resourceBudget.SetAccessWindowMiB(64u);
        params.resourceBudget.SetActiveWindowMiB(256u);
        params.resourceBudget.SetAttributeScratchQuotaMiB(128u);
        params.resourceBudget.SetAttributeManagedStagingLogicalLimitMiB(128u);
        params.resourceBudget.SetAttributePressioLanes(2u);
        params.resourceBudget.SetAttributeReferenceLanes(2u);
        params.resourceBudget.SetAttributeDecodeLanes(1u);
        params.resourceBudget.SetTopologyStreamBufferMiB(1u);
        params.resourceBudget.SetTopologyBufferBudgetMiB(4u);
        params.resourceBudget.SetTopologyBlockLanes(1u);
        params.resourceBudget.SetRemapMortonLeafMiB(128u);
        params.resourceBudget.SetRemapMortonRunBufferMiB(8u);
        params.resourceBudget.SetRemapScratchQuotaMiB(256u);
        params.SetSpatialBlockElementCounts(262144u, 262144u);
    }

    static void ApplyEncodeRuntimeProfile(
        DataCodecEncodeConfigurationParams& result,
        const DataCodecRuntimeProfile profile) noexcept {
        switch (profile) {
            case DataCodecRuntimeProfile::Native:
                return;
            case DataCodecRuntimeProfile::Wasm4GiB:
                // 大型 transfer cache 和 staging 交给 WasmFS 管理以控制线性内存峰值
                // 线性内存为调用方输出和 Runtime 预留空间，Memory Store 限制为 1.5 GiB
                // 当前编码资源上界为 3396 MiB
                result.controlParams.resourceBudget.SetEncodeStorageModes(
                    EncodeStorageMode::Managed,
                    EncodeStorageMode::Managed,
                    EncodeStorageMode::Managed,
                    EncodeStorageMode::Managed,
                    EncodeStorageMode::Managed);
                result.controlParams.resourceBudget.SetTopologyEncodeTransferCacheMode(
                    EncodeStorageMode::Managed);
                result.controlParams.resourceBudget.SetPackageFieldStagingMode(
                    EncodeStorageMode::Managed);
                result.controlParams.resourceBudget.SetResidentLimitMiB(1536u);
                result.controlParams.resourceBudget.SetEncodeReferenceResidentLimitMiB(256u);
                result.controlParams.resourceBudget.SetEncodeReferenceCacheModes(
                    EncodeStorageMode::Managed,
                    EncodeStorageMode::Managed);
                result.controlParams.resourceBudget.SetScratchPoolRetention(2u, 32u, 64u);
                result.controlParams.resourceBudget.SetTopologyBufferBudgetMiB(4u);
                result.controlParams.resourceBudget.SetRemapMortonLeafMiB(128u);
                result.controlParams.resourceBudget.SetRemapMortonRunBufferMiB(8u);
                result.controlParams.resourceBudget.SetRemapScratchQuotaMiB(256u);
                result.controlParams.resourceBudget.SetAccessWindowMiB(128u);
                result.controlParams.resourceBudget.SetActiveWindowMiB(768u);
                result.controlParams.resourceBudget.SetAttributeScratchQuotaMiB(512u);
                result.controlParams.resourceBudget.SetAttributeManagedStagingLogicalLimitMiB(512u);
                result.controlParams.resourceBudget.SetAttributePressioLanes(4u);
                result.controlParams.resourceBudget.SetAttributeReferenceLanes(4u);
                result.pipelineControl.packageFields.workerCount = 1u;
                result.execution.enableParallelStages = false;
                return;
            case DataCodecRuntimeProfile::Wasm16GiB:
                // Memory Store 限制为 8 GiB，剩余空间用于活跃窗口、算法 Scratch 和输出
                // 各编码档位叠加后的最大资源上界为 14596 MiB
                result.controlParams.resourceBudget.SetResidentLimitMiB(8192u);
                result.controlParams.resourceBudget.SetEncodeReferenceResidentLimitMiB(2048u);
                result.controlParams.resourceBudget.SetScratchPoolRetention(4u, 64u, 256u);
                result.controlParams.resourceBudget.SetActiveWindowMiB(2048u);
                result.controlParams.resourceBudget.SetAttributeScratchQuotaMiB(1024u);
                result.controlParams.resourceBudget.SetAttributeMemoryStagingLimitMiB(1024u);
                result.controlParams.resourceBudget.SetAttributeManagedStagingLogicalLimitMiB(1024u);
                result.controlParams.resourceBudget.SetAttributePressioLanes(4u);
                result.controlParams.resourceBudget.SetAttributeReferenceLanes(4u);
                result.pipelineControl.packageFields.workerCount = 4u;
                return;
        }
    }

    static void ApplyDecodeRuntimeProfile(
        DataCodecDecodeConfigurationParams& result,
        const DataCodecRuntimeProfile profile) noexcept {
        auto& params = result.controlParams;
        switch (profile) {
            case DataCodecRuntimeProfile::Native:
                return;
            case DataCodecRuntimeProfile::Wasm4GiB:
                // 属性按 target 延迟解码并复用 managed cache
                // Memory Store 限制为 1.5 GiB，全部大型 Cache 使用 Managed Store
                // 当前解码资源上界为 2880 MiB
                params.resourceBudget.SetResidentLimitMiB(1536u);
                params.resourceBudget.SetScratchPoolRetention(2u, 32u, 64u);
                params.resourceBudget.SetDecodeReferenceRetention(512u, 2u);
                params.resourceBudget.SetAttributeDecodePayloadMode(AttributeDecodePayloadMode::Managed);
                params.resourceBudget.SetAttributeDecodeCacheMode(DecodeStorageMode::Managed);
                params.resourceBudget.SetGeometryDecodeCacheMode(DecodeStorageMode::Managed);
                params.resourceBudget.SetGeometryDecodeReferenceCacheMode(DecodeStorageMode::Managed);
                params.resourceBudget.SetTopologyDecodeInputMode(DecodeStorageMode::Managed);
                params.resourceBudget.SetTopologyDecodeCacheMode(DecodeStorageMode::Managed);
                params.resourceBudget.SetTopologyDecodeReferenceCacheMode(DecodeStorageMode::Managed);
                params.resourceBudget.SetAccessWindowMiB(128u);
                params.resourceBudget.SetActiveWindowMiB(768u);
                params.resourceBudget.SetAttributeDecodeMemoryPayloadLimitMiB(0u);
                params.resourceBudget.SetAttributeDecodeMemoryCacheLimitMiB(0u);
                params.resourceBudget.SetGeometryDecodeMemoryCacheLimitMiB(0u);
                params.resourceBudget.SetGeometryDecodeMemoryReferenceLimitMiB(0u);
                params.resourceBudget.SetTopologyDecodeMemoryInputLimitMiB(0u);
                params.resourceBudget.SetTopologyDecodeMemoryCacheLimitMiB(0u);
                params.resourceBudget.SetTopologyDecodeMemoryReferenceLimitMiB(0u);
                params.resourceBudget.SetAttributeDecodeLanes(1u);
                params.resourceBudget.SetAttributeCommitLanes(1u);
                result.execution.enableParallelStages = false;
                return;
            case DataCodecRuntimeProfile::Wasm16GiB:
                // ByteStoreSession 统一限制所有 Memory Cache 为 8 GiB
                // 当前解码资源上界为 14592 MiB
                params.resourceBudget.SetResidentLimitMiB(8192u);
                params.resourceBudget.SetScratchPoolRetention(4u, 64u, 256u);
                params.resourceBudget.SetDecodeReferenceRetention(4096u, 4u);
                params.resourceBudget.SetActiveWindowMiB(2048u);
                params.resourceBudget.SetAttributeDecodeMemoryPayloadLimitMiB(8192u);
                params.resourceBudget.SetAttributeDecodeMemoryCacheLimitMiB(8192u);
                params.resourceBudget.SetGeometryDecodeMemoryCacheLimitMiB(4096u);
                params.resourceBudget.SetGeometryDecodeMemoryReferenceLimitMiB(4096u);
                params.resourceBudget.SetTopologyDecodeMemoryInputLimitMiB(4096u);
                params.resourceBudget.SetTopologyDecodeMemoryCacheLimitMiB(4096u);
                params.resourceBudget.SetTopologyDecodeMemoryReferenceLimitMiB(4096u);
                params.resourceBudget.SetAttributeDecodeLanes(1u);
                params.resourceBudget.SetAttributeCommitLanes(1u);
                result.execution.enableParallelStages = false;
                return;
        }
    }

};

[[nodiscard]] inline DataCodecEncodeConfigurationParams MakeDefaultEncodeConfigurationParams() {
    return CodecControlParamsFactory::MakeEncodeConfiguration(DataCodecEncodeOptions{});
}

[[nodiscard]] inline DataCodecDecodeConfigurationParams MakeDefaultDecodeConfigurationParams() {
    return CodecControlParamsFactory::MakeDecodeConfiguration(DataCodecDecodeOptions{});
}

[[nodiscard]] inline DataCodecEncodeConfigurationParams MakeEncodeConfigurationParams(
    const DataCodecEncodeOptions& options,
    const DataCodecRuntimeProfile runtimeProfile = DataCodecRuntimeProfile::Native) {
    return CodecControlParamsFactory::MakeEncodeConfiguration(options, runtimeProfile);
}

[[nodiscard]] inline DataCodecDecodeConfigurationParams MakeDecodeConfigurationParams(
    const DataCodecDecodeOptions& options,
    const DataCodecRuntimeProfile runtimeProfile = DataCodecRuntimeProfile::Native) {
    return CodecControlParamsFactory::MakeDecodeConfiguration(options, runtimeProfile);
}

[[nodiscard]] inline EncodeCodecControlParams MakeDefaultEncodeControlParams() {
    return MakeDefaultEncodeConfigurationParams().controlParams;
}

[[nodiscard]] inline DecodeControlParams MakeDefaultDecodeControlParams() {
    return MakeDefaultDecodeConfigurationParams().controlParams;
}

[[nodiscard]] inline EncodeExecutionOptions MakeDefaultEncodeExecutionOptions() {
    return MakeDefaultEncodeConfigurationParams().execution;
}

[[nodiscard]] inline DecodeExecutionOptions MakeDefaultDecodeExecutionOptions() {
    return MakeDefaultDecodeConfigurationParams().execution;
}

} // namespace datacodec

#endif
