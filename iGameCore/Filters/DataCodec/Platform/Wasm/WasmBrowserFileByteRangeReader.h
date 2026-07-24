#ifndef DATACODEC_PLATFORM_WASM_WASMBROWSERFILEBYTERANGEREADER_H
#define DATACODEC_PLATFORM_WASM_WASMBROWSERFILEBYTERANGEREADER_H

#include "DataCodec/Storage/ByteIO/ByteRange.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace datacodec::wasm {

struct WasmBrowserPrefetchStats {
    std::uint64_t requests{0u};
    std::uint64_t accepted{0u};
    std::uint64_t rejectedByPolicy{0u};
    std::uint64_t unavailable{0u};
    std::uint64_t errors{0u};
    std::uint64_t skippedBytes{0u};
    std::uint64_t prefetchedBytes{0u};
};

class WasmBrowserFileByteRangeReader final : public IByteRangeReader {
public:
    static constexpr std::uint64_t kDefaultMaximumPrefetchBytes =
        512ull * 1024ull * 1024ull;

    WasmBrowserFileByteRangeReader(
        std::uint32_t fileId,
        std::uint64_t byteSize,
        std::uint64_t maximumPrefetchBytes = kDefaultMaximumPrefetchBytes);

    [[nodiscard]] std::uint64_t ByteSize() const noexcept override;
    [[nodiscard]] ByteRangePrefetchResult PrefetchRange(
        std::uint64_t offset,
        std::uint64_t byteSize) const override;
    bool ReadAt(
        std::uint64_t offset,
        std::span<std::uint8_t> output,
        std::string* error = nullptr) override;

    [[nodiscard]] std::uint32_t FileId() const noexcept;
    [[nodiscard]] WasmBrowserPrefetchStats PrefetchStats() const;

private:
    std::uint32_t m_fileId{0u};
    std::uint64_t m_byteSize{0u};
    std::uint64_t m_maximumPrefetchBytes{kDefaultMaximumPrefetchBytes};
    mutable std::mutex m_statsMutex;
    mutable WasmBrowserPrefetchStats m_prefetchStats;
};

[[nodiscard]] std::shared_ptr<IByteRangeReader> CreateWasmBrowserFileByteRangeReader(
    std::uint32_t fileId,
    std::uint64_t byteSize,
    std::string* error = nullptr);

void ReleaseWasmBrowserFile(std::uint32_t fileId) noexcept;

} // namespace datacodec::wasm

#endif
