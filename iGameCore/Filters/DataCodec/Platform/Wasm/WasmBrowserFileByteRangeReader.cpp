#include "DataCodec/Platform/Wasm/WasmBrowserFileByteRangeReader.h"

#include "DataCodec/Validation/Common/DataCodecValidation.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/em_js.h>
#endif

namespace datacodec::wasm {

#if defined(__EMSCRIPTEN__)
EM_ASYNC_JS(int, datacodec_wasm_read_browser_file_range,
            (std::uint32_t fileId, std::uint64_t offset, std::uint8_t* output, std::size_t byteCount), {
    try {
        const registry = Module.igameBrowserFiles;
        const file = registry && registry.get(fileId);
        if (!file) {
            Module.igameBrowserFileLastError = `browser file ${fileId} is unavailable`;
            return -1;
        }
        const start = Number(offset);
        const count = Number(byteCount);
        const destination = Number(output);
        if (!Number.isSafeInteger(start) || start < 0 ||
            !Number.isSafeInteger(count) || count < 0 ||
            !Number.isSafeInteger(destination) || destination < 0 ||
            !Number.isSafeInteger(start + count) || start + count > file.size) {
            Module.igameBrowserFileLastError =
                `invalid browser file range offset=${start} bytes=${count} size=${file.size}`;
            return -2;
        }
        let bytes = null;
        const prefetchRegistry = Module.dataCodecBrowserPrefetchRanges;
        const prefetched = prefetchRegistry && prefetchRegistry.get(fileId);
        if (prefetched && start >= prefetched.offset &&
            start + count <= prefetched.offset + prefetched.bytes.byteLength) {
            const localOffset = start - prefetched.offset;
            bytes = prefetched.bytes.subarray(localOffset, localOffset + count);
        } else {
            bytes = new Uint8Array(await file.slice(start, start + count).arrayBuffer());
        }
        if (bytes.byteLength !== count) {
            Module.igameBrowserFileLastError =
                `browser file range returned ${bytes.byteLength} bytes, expected ${count}`;
            return -3;
        }
        HEAPU8.set(bytes, destination);
        if (typeof Module.igameBrowserFileReadObserver === 'function') {
            Module.igameBrowserFileReadObserver(fileId, start, count);
        }
        Module.igameBrowserFileLastError = String();
        return 1;
    } catch (error) {
        Module.igameBrowserFileLastError = error && error.message
            ? error.message
            : String(error);
        return -4;
    }
});

EM_ASYNC_JS(int, datacodec_wasm_prefetch_browser_file_range,
            (std::uint32_t fileId, std::uint64_t offset, std::uint64_t byteCount), {
    try {
        const registry = Module.igameBrowserFiles;
        const file = registry && registry.get(fileId);
        if (!file) return -1;
        const start = Number(offset);
        const count = Number(byteCount);
        if (!Number.isSafeInteger(start) || !Number.isSafeInteger(count) ||
            start < 0 || count < 0 || start + count > file.size) return -2;
        const bytes = new Uint8Array(await file.slice(start, start + count).arrayBuffer());
        if (!Module.dataCodecBrowserPrefetchRanges) {
            Module.dataCodecBrowserPrefetchRanges = new Map();
        }
        Module.dataCodecBrowserPrefetchRanges.set(fileId, {offset: start, bytes});
        return 1;
    } catch (error) {
        Module.igameBrowserFileLastError = error && error.message
            ? error.message
            : String(error);
        return -3;
    }
});

EM_JS(void, datacodec_wasm_release_browser_file, (std::uint32_t fileId), {
    if (Module.igameBrowserFiles) Module.igameBrowserFiles.delete(fileId);
    if (Module.igameBrowserFileStats) Module.igameBrowserFileStats.delete(fileId);
    if (Module.dataCodecBrowserPrefetchRanges) {
        Module.dataCodecBrowserPrefetchRanges.delete(fileId);
    }
});
#endif

WasmBrowserFileByteRangeReader::WasmBrowserFileByteRangeReader(
    const std::uint32_t fileId,
    const std::uint64_t byteSize,
    const std::uint64_t maximumPrefetchBytes)
    : m_fileId(fileId),
      m_byteSize(byteSize),
      m_maximumPrefetchBytes(maximumPrefetchBytes) {}

std::uint64_t WasmBrowserFileByteRangeReader::ByteSize() const noexcept {
    return m_byteSize;
}

ByteRangePrefetchResult WasmBrowserFileByteRangeReader::PrefetchRange(
    const std::uint64_t offset,
    const std::uint64_t byteSize) const {
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_prefetchStats.requests;
    }
    if (m_fileId == 0u || offset > m_byteSize || byteSize > m_byteSize - offset) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_prefetchStats.errors;
        m_prefetchStats.skippedBytes = validation::SaturatingAddU64(
            m_prefetchStats.skippedBytes,
            byteSize);
        return {
            .status = ByteRangePrefetchStatus::Error,
            .error = "browser file prefetch range is invalid",
        };
    }
    if (byteSize == 0u) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_prefetchStats.accepted;
        return {.status = ByteRangePrefetchStatus::Accepted};
    }
    if (byteSize > m_maximumPrefetchBytes) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_prefetchStats.rejectedByPolicy;
        m_prefetchStats.skippedBytes = validation::SaturatingAddU64(
            m_prefetchStats.skippedBytes,
            byteSize);
        return {.status = ByteRangePrefetchStatus::RejectedByPolicy};
    }
#if defined(__EMSCRIPTEN__)
    const auto status = datacodec_wasm_prefetch_browser_file_range(m_fileId, offset, byteSize);
    if (status == 1) {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_prefetchStats.accepted;
        m_prefetchStats.prefetchedBytes = validation::SaturatingAddU64(
            m_prefetchStats.prefetchedBytes,
            byteSize);
        return {.status = ByteRangePrefetchStatus::Accepted};
    } else {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_prefetchStats.errors;
        m_prefetchStats.skippedBytes = validation::SaturatingAddU64(
            m_prefetchStats.skippedBytes,
            byteSize);
        return {
            .status = ByteRangePrefetchStatus::Error,
            .error = "browser file prefetch operation failed with status " + std::to_string(status),
        };
    }
#else
    std::lock_guard<std::mutex> lock(m_statsMutex);
    ++m_prefetchStats.unavailable;
    m_prefetchStats.skippedBytes = validation::SaturatingAddU64(
        m_prefetchStats.skippedBytes,
        byteSize);
    return {.status = ByteRangePrefetchStatus::Unavailable};
#endif
}

bool WasmBrowserFileByteRangeReader::ReadAt(
    const std::uint64_t offset,
    const std::span<std::uint8_t> output,
    std::string* error) {
    if (m_fileId == 0u || offset > m_byteSize || output.size() > m_byteSize - offset) {
        return validation::AssignError(error, "browser file read range is invalid");
    }
    if (output.empty()) { return true; }
#if defined(__EMSCRIPTEN__)
    const auto status = datacodec_wasm_read_browser_file_range(
        m_fileId,
        offset,
        output.data(),
        output.size());
    if (status == 1) { return true; }
    return validation::AssignError(
        error,
        "browser file range read failed with status " + std::to_string(status));
#else
    return validation::AssignError(error, "browser file reader requires Emscripten");
#endif
}

std::uint32_t WasmBrowserFileByteRangeReader::FileId() const noexcept {
    return m_fileId;
}

WasmBrowserPrefetchStats WasmBrowserFileByteRangeReader::PrefetchStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_prefetchStats;
}

std::shared_ptr<IByteRangeReader> CreateWasmBrowserFileByteRangeReader(
    const std::uint32_t fileId,
    const std::uint64_t byteSize,
    std::string* error) {
#if defined(__EMSCRIPTEN__)
    if (fileId == 0u) {
        validation::AssignError(error, "browser file id is invalid");
        return nullptr;
    }
    if (byteSize == 0u) {
        validation::AssignError(error, "browser file is empty");
        return nullptr;
    }
    return std::make_shared<WasmBrowserFileByteRangeReader>(fileId, byteSize);
#else
    (void)fileId;
    (void)byteSize;
    validation::AssignError(error, "browser file reader requires Emscripten");
    return nullptr;
#endif
}

void ReleaseWasmBrowserFile(const std::uint32_t fileId) noexcept {
#if defined(__EMSCRIPTEN__)
    if (fileId != 0u) {
        datacodec_wasm_release_browser_file(fileId);
    }
#else
    (void)fileId;
#endif
}

} // namespace datacodec::wasm
