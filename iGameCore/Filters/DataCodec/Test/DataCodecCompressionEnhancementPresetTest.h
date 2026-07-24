#ifndef DATACODEC_TEST_DATACODECCOMPRESSIONENHANCEMENTPRESETTEST_H
#define DATACODEC_TEST_DATACODECCOMPRESSIONENHANCEMENTPRESETTEST_H

#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Codec/Remap/Common/MortonRemapBuilder.h"

#include <cstddef>
#include <cstdint>

namespace datacodec::test {

// 保持未注册状态 由维护者在需要时手动调用
[[nodiscard]] inline bool TestCompressionEnhancementPresetComposition() noexcept {
    constexpr std::size_t kDriverCellCount = 38000000u;
    constexpr std::uint64_t kMiB = 1024u * 1024u;

    const auto fast = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::TimePriority,
            .enableCompressionEnhancement = true,
        });
    const auto standard = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::Balanced,
            .enableCompressionEnhancement = true,
        });
    const auto lowMemory = CodecControlParamsFactory::MakeEncodeConfiguration(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::MemoryPriority,
            .enableCompressionEnhancement = true,
        });
    const auto usesInMemoryMortonPath = [=](const auto& configuration) {
        return kDriverCellCount <= mortonremap::ResolveLeafBudgetElements(
            configuration.controlParams.resourceBudget.RemapMortonLeafBytes());
    };
    const auto& fastBudget = fast.controlParams.resourceBudget;
    const auto& standardBudget = standard.controlParams.resourceBudget;
    const auto& lowMemoryBudget = lowMemory.controlParams.resourceBudget;

    return
        fast.pipelineControl.cellOrder == EncodeCellOrderMode::Morton &&
        standard.pipelineControl.cellOrder == EncodeCellOrderMode::Morton &&
        lowMemory.pipelineControl.cellOrder == EncodeCellOrderMode::Morton &&
        usesInMemoryMortonPath(fast) &&
        usesInMemoryMortonPath(standard) &&
        !usesInMemoryMortonPath(lowMemory) &&
        fastBudget.RemapEncodeStorageMode() == EncodeStorageMode::Memory &&
        standardBudget.RemapEncodeStorageMode() == EncodeStorageMode::Managed &&
        lowMemoryBudget.RemapEncodeStorageMode() == EncodeStorageMode::Managed &&
        fastBudget.RemapMortonLeafBytes() == 512u * kMiB &&
        fastBudget.RemapMortonRunBufferBytes() == 64u * kMiB &&
        fastBudget.RemapScratchQuotaBytes() == 1024u * kMiB &&
        standardBudget.ResidentLimitBytes() == 2048u * kMiB &&
        standardBudget.EncodeReferenceResidentLimitBytes() == 512u * kMiB &&
        standardBudget.ActiveWindowBytes() == 1600u * kMiB &&
        standardBudget.AttributeScratchQuotaBytes() == 1024u * kMiB &&
        standardBudget.AttributeStagingQuotaBytes(EncodeStorageMode::Managed) ==
            1024u * kMiB &&
        standardBudget.RemapMortonLeafBytes() == 512u * kMiB &&
        standardBudget.RemapMortonRunBufferBytes() == 64u * kMiB &&
        standardBudget.RemapScratchQuotaBytes() == 1024u * kMiB;
}

} // 命名空间 datacodec::test

#endif
