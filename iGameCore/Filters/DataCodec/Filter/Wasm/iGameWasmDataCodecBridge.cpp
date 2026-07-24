#include "DataCodec/Filter/Wasm/iGameWasmDataCodecBridge.h"

#include "DataCodec/Filter/Adapter/iGameFileByteRangeIO.h"
#include "DataCodec/Filter/Execution/iGameDataCodecThreadPoolTaskRunner.h"
#include "DataCodec/Filter/Execution/iGameRunRecordSink.h"
#include "DataCodec/Filter/Wasm/iGameWasmDataCodecDiagnostics.h"
#include "DataCodec/Platform/Wasm/WasmBrowserFileByteRangeReader.h"
#include "DataCodec/Platform/Wasm/WasmRuntime.h"
#include "DataCodec/Storage/Package/PackageBinaryHeader.h"
#include "iGameThreadPool.h"

#include <exception>
#include <filesystem>
#include <sstream>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {

void CleanupFailedWasmDecodeSession(
    iGameWasmDataCodecDecodeResult& result) noexcept {
    if (result.session != nullptr) {
        try {
            result.session->Reset();
        } catch (...) {
        }
        result.session.reset();
    }
    result.decodeResult.success = false;
    result.decodeResult.output = nullptr;
    result.output = nullptr;
    result.success = false;
}

void SetWasmDecodeError(
    iGameWasmDataCodecDecodeResult& result,
    ::datacodec::IRunRecordSink* runRecordSink,
    std::string text) {
    result.error = std::move(text);
    SubmitiGameRunError(
        runRecordSink,
        "iGameWasmDataCodecBridge",
        result.error);
}

} // 匿名命名空间

std::shared_ptr<::datacodec::IParallelTaskRunner> MakeiGameWasmDataCodecTaskRunner() {
    const auto capabilities = ::datacodec::wasm::DetectWasmRuntimeCapabilities();
    if (capabilities.pthreadsAvailable) {
        return DataCodecTaskRunner();
    }
    return nullptr;
}

std::future<void> SubmitiGameWasmDataCodecTask(std::function<void()> task) {
    return ThreadPool::Instance()->Commit(std::move(task));
}

bool ResolveiGameWasmPackageSourceIdentity(
    ::datacodec::IByteRangeReader& reader,
    ::datacodec::DecodeSourceIdentity& sourceIdentity,
    std::string* error) {
    ::datacodec::PackageInspection inspection;
    if (!::datacodec::InspectPackage(reader, inspection, error)) {
        sourceIdentity = {};
        return false;
    }
    sourceIdentity = std::move(inspection.sourceIdentity);
    if (error != nullptr) { error->clear(); }
    return true;
}

bool ResolveiGameWasmDataCodecFileSourceIdentity(
    const std::string& filePath,
    ::datacodec::DecodeSourceIdentity& sourceIdentity,
    std::string* error) {
    if (filePath.empty()) {
        sourceIdentity = {};
        return ::datacodec::validation::AssignError(
            error,
            "DataCodec WASM file path is empty");
    }
    iGameFileByteRangeReader reader{std::filesystem::path(filePath)};
    return ResolveiGameWasmPackageSourceIdentity(reader, sourceIdentity, error);
}

iGameWasmDataCodecDecodeResult DecodeiGameWasmDataCodec(
    iGameWasmDataCodecDecodeRequest request) {
    iGameWasmDataCodecDecodeResult result;
    bool completedSuccessfully = false;
    struct FailureCleanupScope {
        iGameWasmDataCodecDecodeResult& result;
        bool& completedSuccessfully;

        ~FailureCleanupScope() noexcept {
            if (!completedSuccessfully) {
                CleanupFailedWasmDecodeSession(result);
            }
        }
    } failureCleanup{result, completedSuccessfully};
    if (request.inputReader == nullptr) {
        SetWasmDecodeError(
            result,
            request.runRecordSink.get(),
            "DataCodec WASM decode requires an input reader");
        return result;
    }
    ::datacodec::DecodeSourceIdentity inspectedIdentity;
    if (!ResolveiGameWasmPackageSourceIdentity(
            *request.inputReader,
            inspectedIdentity,
            &result.error)) {
        SubmitiGameRunError(
            request.runRecordSink.get(),
            "iGameWasmDataCodecBridge",
            result.error);
        return result;
    }
    if (request.sourceIdentity.IsStable() && request.sourceIdentity != inspectedIdentity) {
        SetWasmDecodeError(
            result,
            request.runRecordSink.get(),
            "DataCodec WASM source identity does not match the package header");
        return result;
    }
    request.sourceIdentity = std::move(inspectedIdentity);
    result.cacheIdentityAvailable = true;
    if (request.parallelTaskRunner == nullptr) {
        request.parallelTaskRunner = MakeiGameWasmDataCodecTaskRunner();
        if (request.parallelTaskRunner == nullptr) {
            SetWasmDecodeError(
                result,
                request.runRecordSink.get(),
                "DataCodec WASM decode requires pthread support");
            return result;
        }
    }
    const auto browserReader = std::dynamic_pointer_cast<
        ::datacodec::wasm::WasmBrowserFileByteRangeReader>(request.inputReader);

    const auto preparedSurface =
        request.topologyOutputMode == iGameWasmTopologyOutputMode::PreparedSurface;
    std::shared_ptr<iGamePreparedSurfaceDecodeAdapter> surfaceObserver;
    if (preparedSurface) {
        try {
            surfaceObserver = std::make_shared<iGamePreparedSurfaceDecodeAdapter>(
                request.parallelTaskRunner);
        } catch (const std::exception& exception) {
            SetWasmDecodeError(
                result,
                request.runRecordSink.get(),
                std::string("DataCodec WASM prepared surface initialization failed: ") +
                    exception.what());
            return result;
        } catch (...) {
            SetWasmDecodeError(
                result,
                request.runRecordSink.get(),
                "DataCodec WASM prepared surface initialization failed");
            return result;
        }
    }
    auto controls = ::datacodec::wasm::MakeWasmDecodeConfiguration(
        request.enableReuseCache);
    if (preparedSurface) {
        // PreparedSurface 在 DataCodec 会话返回后完成，禁止缓存未附着 surface 的中间结果
        controls.decodedFrameCachePolicy.enabled = false;
    }
    if (request.enableEncodedInputCache.has_value()) {
        controls.encodedInputCachePolicy.enabled = *request.enableEncodedInputCache;
    }
    if (request.enableFullInputPrefetch.has_value()) {
        controls.execution.enableFullInputPrefetch = *request.enableFullInputPrefetch;
    }
    controls.execution.topologyBlockObserver = surfaceObserver;
    controls.execution.topologyOutputMode = preparedSurface
        ? ::datacodec::TopologyDecodeOutputMode::ObserverOnly
        : ::datacodec::TopologyDecodeOutputMode::CommitToAdapter;
    result.encodedInputCacheEnabled = controls.encodedInputCachePolicy.enabled;
    result.fullInputPrefetchEnabled = controls.execution.enableFullInputPrefetch;

    const auto cacheRuntime = ::datacodec::DefaultDecodeCacheRuntime();
    const auto frameCache = cacheRuntime->DefaultFrameCache();
    const auto encodedInputCache = cacheRuntime->DefaultEncodedInputCache();
    result.cacheStatsBefore = frameCache->Statistics();
    result.encodedInputCacheStatsBefore = encodedInputCache->Statistics();
    auto diagnosticsSink = std::make_shared<iGameWasmDataCodecDiagnosticsSink>();
    iGameRunRecordSinkSet recordSinks(false, std::move(request.runRecordSink));
    recordSinks.AddSink(diagnosticsSink);
    auto runRecordSink = recordSinks.Sink();
    result.session = std::make_shared<DataCodecDataObjectDecodeSession>();
    result.decodeResult = result.session->Open({
        .inputReader = std::move(request.inputReader),
        .inputSourceIdentity = request.sourceIdentity,
        .controlParams = &controls.controlParams,
        .executionOptions = &controls.execution,
        .configurationSource = &controls.source,
        .decodedFrameCachePolicy = controls.decodedFrameCachePolicy,
        .encodedInputCachePolicy = controls.encodedInputCachePolicy,
        .cacheRuntime = cacheRuntime,
        .executionResources = ::datacodec::DataCodecExecutionResources{
            .parallelTaskRunner = request.parallelTaskRunner.get(),
        },
        .runRecordSink = runRecordSink,
    });
    result.cacheStatsAfter = frameCache->Statistics();
    result.encodedInputCacheStatsAfter = encodedInputCache->Statistics();
    result.sourceIdentity = request.sourceIdentity;
    result.output = result.decodeResult.success
        ? result.decodeResult.output
        : DataObject::Pointer{};
    result.timingDetail = diagnosticsSink->BuildTopologyTimingDetail();
    if (result.output == nullptr) {
        if (result.decodeResult.messages.empty()) {
            SetWasmDecodeError(
                result,
                runRecordSink.get(),
                "DataCodec WASM decode returned null");
        } else {
            result.error = result.decodeResult.messages.back().text;
        }
        return result;
    }

    if (surfaceObserver != nullptr) {
        std::string surfaceError;
        bool attached = false;
        try {
            attached = surfaceObserver->AttachPreparedSurface(result.output, &surfaceError);
        } catch (const std::exception& exception) {
            surfaceError = std::string("DataCodec WASM prepared surface construction failed: ") +
                exception.what();
        } catch (...) {
            surfaceError = "DataCodec WASM prepared surface construction failed";
        }
        if (!attached) {
            SetWasmDecodeError(
                result,
                runRecordSink.get(),
                surfaceError.empty()
                    ? "DataCodec WASM prepared surface construction failed"
                    : std::move(surfaceError));
            return result;
        } else {
            result.surfaceSummary = surfaceObserver->Summary();
        }
    }

    std::ostringstream cacheTiming;
    cacheTiming << "cache-identity=" << (result.cacheIdentityAvailable ? 1 : 0)
                << "; encoded-input-cache=" << (result.encodedInputCacheEnabled ? 1 : 0)
                << "; full-input-prefetch=" << (result.fullInputPrefetchEnabled ? 1 : 0)
                << "; frame-cache-hit=" << (result.decodeResult.decodedFrameCacheHit ? 1 : 0)
                << "; cache-lookups-delta="
                << (result.cacheStatsAfter.lookups - result.cacheStatsBefore.lookups)
                << "; cache-hits-delta="
                << (result.cacheStatsAfter.hits - result.cacheStatsBefore.hits)
                << "; cache-misses-delta="
                << (result.cacheStatsAfter.misses - result.cacheStatsBefore.misses)
                << "; cache-stores-delta="
                << (result.cacheStatsAfter.stores - result.cacheStatsBefore.stores)
                << "; cache-evictions-delta="
                << (result.cacheStatsAfter.evictions - result.cacheStatsBefore.evictions)
                << "; cache-resident-frames=" << result.cacheStatsAfter.residentFrames
                << "; cache-resident-bytes=" << result.cacheStatsAfter.residentBytes
                << "; cache-peak-bytes=" << result.cacheStatsAfter.peakResidentBytes
                << "; encoded-cache-lookups-delta="
                << (result.encodedInputCacheStatsAfter.lookups -
                    result.encodedInputCacheStatsBefore.lookups)
                << "; encoded-cache-hits-delta="
                << (result.encodedInputCacheStatsAfter.hits -
                    result.encodedInputCacheStatsBefore.hits)
                << "; encoded-cache-misses-delta="
                << (result.encodedInputCacheStatsAfter.misses -
                    result.encodedInputCacheStatsBefore.misses)
                << "; encoded-cache-stores-delta="
                << (result.encodedInputCacheStatsAfter.stores -
                    result.encodedInputCacheStatsBefore.stores)
                << "; encoded-cache-evictions-delta="
                << (result.encodedInputCacheStatsAfter.evictions -
                    result.encodedInputCacheStatsBefore.evictions)
                << "; encoded-cache-resident-inputs="
                << result.encodedInputCacheStatsAfter.residentInputs
                << "; encoded-cache-resident-bytes="
                << result.encodedInputCacheStatsAfter.residentBytes
                << "; encoded-cache-peak-bytes="
                << result.encodedInputCacheStatsAfter.peakResidentBytes;
    if (browserReader != nullptr) {
        const auto prefetchStats = browserReader->PrefetchStats();
        cacheTiming << "; browser-prefetch-requests=" << prefetchStats.requests
                    << "; browser-prefetch-accepted=" << prefetchStats.accepted
                    << "; browser-prefetched-bytes=" << prefetchStats.prefetchedBytes
                    << "; browser-prefetch-skipped-bytes=" << prefetchStats.skippedBytes;
    }
    if (!result.timingDetail.empty()) { result.timingDetail += "; "; }
    result.timingDetail += cacheTiming.str();
    if (!result.surfaceSummary.empty()) {
        result.timingDetail += "; " + result.surfaceSummary;
    }
    result.success = true;
    completedSuccessfully = true;
    return result;
}

iGameWasmDataCodecDecodeResult DecodeiGameWasmDataCodecFile(
    const std::string& filePath,
    const bool enableReuseCache,
    const iGameWasmTopologyOutputMode topologyOutputMode,
    const std::optional<bool> enableEncodedInputCache,
    const std::optional<bool> enableFullInputPrefetch,
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink,
    ::datacodec::DecodeSourceIdentity sourceIdentity) {
    if (filePath.empty()) {
        iGameWasmDataCodecDecodeResult result;
        SetWasmDecodeError(
            result,
            runRecordSink.get(),
            "DataCodec WASM file path is empty");
        return result;
    }
    return DecodeiGameWasmDataCodec(iGameWasmDataCodecDecodeRequest{
        .inputReader = std::make_shared<iGameFileByteRangeReader>(
            std::filesystem::path(filePath)),
        .sourceIdentity = std::move(sourceIdentity),
        .enableReuseCache = enableReuseCache,
        .enableEncodedInputCache = enableEncodedInputCache,
        .enableFullInputPrefetch = enableFullInputPrefetch,
        .topologyOutputMode = topologyOutputMode,
        .runRecordSink = std::move(runRecordSink),
    });
}

iGameWasmDataCodecDecodeResult DecodeiGameWasmDataCodecMemory(
    const std::span<const std::uint8_t> bytes,
    const bool enableReuseCache,
    const iGameWasmTopologyOutputMode topologyOutputMode,
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink) {
    if (bytes.empty()) {
        iGameWasmDataCodecDecodeResult result;
        SetWasmDecodeError(
            result,
            runRecordSink.get(),
            "DataCodec WASM memory input is empty");
        return result;
    }
    return DecodeiGameWasmDataCodec(iGameWasmDataCodecDecodeRequest{
        .inputReader = std::make_shared<::datacodec::MemoryByteRangeReader>(bytes),
        .enableReuseCache = enableReuseCache,
        .topologyOutputMode = topologyOutputMode,
        .runRecordSink = std::move(runRecordSink),
    });
}

iGameWasmDataCodecDecodeResult DecodeiGameWasmBrowserFile(
    const std::uint32_t browserFileId,
    const std::uint64_t browserFileSize,
    const bool enableReuseCache,
    const iGameWasmTopologyOutputMode topologyOutputMode,
    const std::optional<bool> enableEncodedInputCache,
    const std::optional<bool> enableFullInputPrefetch,
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink) {
    std::string readerError;
    auto reader = ::datacodec::wasm::CreateWasmBrowserFileByteRangeReader(
        browserFileId,
        browserFileSize,
        &readerError);
    if (reader == nullptr) {
        iGameWasmDataCodecDecodeResult result;
        SetWasmDecodeError(
            result,
            runRecordSink.get(),
            std::move(readerError));
        return result;
    }
    return DecodeiGameWasmDataCodec(iGameWasmDataCodecDecodeRequest{
        .inputReader = std::move(reader),
        .enableReuseCache = enableReuseCache,
        .enableEncodedInputCache = enableEncodedInputCache,
        .enableFullInputPrefetch = enableFullInputPrefetch,
        .topologyOutputMode = topologyOutputMode,
        .runRecordSink = std::move(runRecordSink),
    });
}

IGAME_NAMESPACE_END
