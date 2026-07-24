#ifndef DATACODEC_RUNTIME_CACHE_CACHERESOURCES_H
#define DATACODEC_RUNTIME_CACHE_CACHERESOURCES_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Storage/ByteIO/Window/WindowBudget.h"
#include "DataCodec/Storage/ByteIO/Window/WindowRuntimeParams.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
namespace datacodec {

struct CacheResources {
    std::size_t accessWindowBytes{kDefaultEncodeAccessWindowBytes};
    std::uint64_t activeWindowBytes{kDefaultEncodeActiveWindowBytes};
    mutable ScratchByteBufferPool scratchBytePool;
    mutable window::WindowBudget windowBudget{kDefaultEncodeActiveWindowBytes};
    mutable resource::ActiveByteBudget topologyBufferBudget{4u * 1024u * 1024u};
    mutable resource::ActiveByteBudget remapScratchBudget{256u * 1024u * 1024u};

    void Configure(
        const std::size_t requestedAccessWindowBytes,
        const std::uint64_t requestedActiveWindowBytes,
        const std::size_t scratchRetainedBlockCount = 16u,
        const std::size_t scratchRetainedBlockBytes = 64u * 1024u * 1024u,
        const std::uint64_t scratchRetainedTotalBytes = 1024ull * 1024ull * 1024ull,
        const std::uint64_t topologyBufferBudgetBytes = 4u * 1024u * 1024u,
        const std::uint64_t remapScratchBudgetBytes = 256u * 1024u * 1024u) {
        accessWindowBytes = std::max<std::size_t>(requestedAccessWindowBytes, 1u);
        activeWindowBytes = std::max<std::uint64_t>(requestedActiveWindowBytes, 1u);
        scratchBytePool.Clear();
        scratchBytePool.Configure(
            scratchRetainedBlockCount,
            scratchRetainedBlockBytes,
            scratchRetainedTotalBytes);
        windowBudget.Reset(activeWindowBytes);
        topologyBufferBudget.Reset(topologyBufferBudgetBytes);
        remapScratchBudget.Reset(remapScratchBudgetBytes);
    }

    void Clear() {
        scratchBytePool.Clear();
        windowBudget.Reset(activeWindowBytes);
        topologyBufferBudget.Reset(topologyBufferBudget.MaxActiveBytes());
        remapScratchBudget.Reset(remapScratchBudget.MaxActiveBytes());
    }

    template<typename TValue>
    [[nodiscard]] std::size_t ValuesPerWindow(const std::size_t valuesPerElement = 1u) const noexcept {
        return std::max<std::size_t>(
            1u,
            accessWindowBytes /
                std::max<std::size_t>(sizeof(TValue) * valuesPerElement, sizeof(TValue)));
    }
};

} // namespace datacodec

#endif
