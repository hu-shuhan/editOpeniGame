#ifndef DATACODEC_PLATFORM_WASM_WASMRUNTIME_H
#define DATACODEC_PLATFORM_WASM_WASMRUNTIME_H

#include "DataCodec/API/Params/CodecPerformancePresetParams.h"

#include <cstddef>
#include <cstdint>

namespace datacodec::wasm {

struct WasmRuntimeCapabilities {
    bool emscripten{false};
    bool pthreadsAvailable{false};
    std::size_t pointerBytes{sizeof(void*)};
    std::size_t maximumDataCodecWorkers{1u};
    DataCodecRuntimeProfile runtimeProfile{DataCodecRuntimeProfile::Wasm4GiB};
};

[[nodiscard]] WasmRuntimeCapabilities DetectWasmRuntimeCapabilities() noexcept;

[[nodiscard]] DataCodecDecodeConfigurationParams MakeWasmDecodeConfiguration(
    bool enableReuseCache = true);

[[nodiscard]] DataCodecEncodeConfigurationParams MakeWasmEncodeConfiguration();

} // namespace datacodec::wasm

#endif
