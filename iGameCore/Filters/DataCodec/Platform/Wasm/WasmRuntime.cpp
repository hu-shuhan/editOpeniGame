#include "DataCodec/Platform/Wasm/WasmRuntime.h"

#include <algorithm>

namespace datacodec::wasm {

WasmRuntimeCapabilities DetectWasmRuntimeCapabilities() noexcept {
    WasmRuntimeCapabilities capabilities;
#if defined(__EMSCRIPTEN__)
    capabilities.emscripten = true;
#endif
#if defined(__EMSCRIPTEN_PTHREADS__)
    capabilities.pthreadsAvailable = true;
#endif
    capabilities.pointerBytes = sizeof(void*);
    capabilities.runtimeProfile = sizeof(void*) > 4u
        ? DataCodecRuntimeProfile::Wasm16GiB
        : DataCodecRuntimeProfile::Wasm4GiB;
#if defined(DATACODEC_MAX_PARALLEL_WORKERS) && DATACODEC_MAX_PARALLEL_WORKERS > 0
    capabilities.maximumDataCodecWorkers = capabilities.pthreadsAvailable
        ? static_cast<std::size_t>(DATACODEC_MAX_PARALLEL_WORKERS)
        : 1u;
#else
    capabilities.maximumDataCodecWorkers = capabilities.pthreadsAvailable ? 4u : 1u;
#endif
    return capabilities;
}

DataCodecDecodeConfigurationParams MakeWasmDecodeConfiguration(
    const bool enableReuseCache) {
    const auto capabilities = DetectWasmRuntimeCapabilities();
    auto controls = MakeDecodeConfigurationParams(
        DataCodecDecodeOptions{
            .tier = DataCodecDecodeTier::Fast,
            .enableDecodedResultCache = enableReuseCache,
            .decodedResultCacheFrameLimit = 1u,
            .enableFullInputPrefetch = enableReuseCache,
        },
        capabilities.runtimeProfile);
    controls.controlParams.resourceBudget.SetTopologyBlockLanes(
        static_cast<std::uint32_t>(capabilities.maximumDataCodecWorkers));
    controls.decodedFrameCachePolicy.enabled = enableReuseCache;
    controls.decodedFrameCachePolicy.residentFrameLimit = enableReuseCache ? 1u : 0u;
    controls.decodedFrameCachePolicy.prefetchFrameCount = 0u;
    controls.decodedFrameCachePolicy.residentLimitBytes = sizeof(void*) > 4u
        ? 8ull * 1024ull * 1024ull * 1024ull
        : 2ull * 1024ull * 1024ull * 1024ull;
    if (!enableReuseCache) {
        controls.execution.enableFullInputPrefetch = false;
        controls.encodedInputCachePolicy.enabled = false;
    }
    return controls;
}

DataCodecEncodeConfigurationParams MakeWasmEncodeConfiguration() {
    const auto capabilities = DetectWasmRuntimeCapabilities();
    auto controls = MakeEncodeConfigurationParams(
        DataCodecEncodeOptions{
            .tier = DataCodecEncodeTier::TimePriority,
        },
        capabilities.runtimeProfile);
    return controls;
}

} // namespace datacodec::wasm
