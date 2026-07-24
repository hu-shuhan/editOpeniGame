#ifndef iGameWasmDataCodecBridge_h
#define iGameWasmDataCodecBridge_h

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Adapter/IEncodedInputCache.h"
#include "DataCodec/Filter/Adapter/iGameDataCodecDataObjectBridge.h"
#include "DataCodec/Filter/Adapter/iGamePreparedSurfaceDecodeAdapter.h"
#include "DataCodec/Runtime/Cache/DecodedFrameLruCache.h"

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>

IGAME_NAMESPACE_BEGIN

enum class iGameWasmTopologyOutputMode : std::uint8_t {
    CommitToAdapter = 0,
    PreparedSurface = 1,
};

struct iGameWasmDataCodecDecodeRequest {
    std::shared_ptr<::datacodec::IByteRangeReader> inputReader;
    ::datacodec::DecodeSourceIdentity sourceIdentity;
    bool enableReuseCache{true};
    std::optional<bool> enableEncodedInputCache;
    std::optional<bool> enableFullInputPrefetch;
    iGameWasmTopologyOutputMode topologyOutputMode{
        iGameWasmTopologyOutputMode::CommitToAdapter};
    std::shared_ptr<::datacodec::IParallelTaskRunner> parallelTaskRunner;
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink;
};

struct iGameWasmDataCodecDecodeResult {
    bool success{false};
    DataObject::Pointer output;
    std::shared_ptr<DataCodecDataObjectDecodeSession> session;
    ::datacodec::DecodeSourceIdentity sourceIdentity;
    DataCodecDataObjectDecodeResult decodeResult;
    ::datacodec::DecodedFrameCacheStats cacheStatsBefore;
    ::datacodec::DecodedFrameCacheStats cacheStatsAfter;
    ::datacodec::EncodedInputCacheStats encodedInputCacheStatsBefore;
    ::datacodec::EncodedInputCacheStats encodedInputCacheStatsAfter;
    bool cacheIdentityAvailable{false};
    bool encodedInputCacheEnabled{false};
    bool fullInputPrefetchEnabled{false};
    std::string timingDetail;
    std::string surfaceSummary;
    std::string error;
};

[[nodiscard]] std::shared_ptr<::datacodec::IParallelTaskRunner>
MakeiGameWasmDataCodecTaskRunner();

[[nodiscard]] std::future<void> SubmitiGameWasmDataCodecTask(
    std::function<void()> task);

[[nodiscard]] bool ResolveiGameWasmPackageSourceIdentity(
    ::datacodec::IByteRangeReader& reader,
    ::datacodec::DecodeSourceIdentity& sourceIdentity,
    std::string* error = nullptr);

[[nodiscard]] bool ResolveiGameWasmDataCodecFileSourceIdentity(
    const std::string& filePath,
    ::datacodec::DecodeSourceIdentity& sourceIdentity,
    std::string* error = nullptr);

[[nodiscard]] iGameWasmDataCodecDecodeResult DecodeiGameWasmDataCodec(
    iGameWasmDataCodecDecodeRequest request);

[[nodiscard]] iGameWasmDataCodecDecodeResult DecodeiGameWasmDataCodecFile(
    const std::string& filePath,
    bool enableReuseCache = true,
    iGameWasmTopologyOutputMode topologyOutputMode =
        iGameWasmTopologyOutputMode::CommitToAdapter,
    std::optional<bool> enableEncodedInputCache = {},
    std::optional<bool> enableFullInputPrefetch = {},
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink = {},
    ::datacodec::DecodeSourceIdentity sourceIdentity = {});

[[nodiscard]] iGameWasmDataCodecDecodeResult DecodeiGameWasmDataCodecMemory(
    std::span<const std::uint8_t> bytes,
    bool enableReuseCache = true,
    iGameWasmTopologyOutputMode topologyOutputMode =
        iGameWasmTopologyOutputMode::CommitToAdapter,
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink = {});

[[nodiscard]] iGameWasmDataCodecDecodeResult DecodeiGameWasmBrowserFile(
    std::uint32_t browserFileId,
    std::uint64_t browserFileSize,
    bool enableReuseCache = true,
    iGameWasmTopologyOutputMode topologyOutputMode =
        iGameWasmTopologyOutputMode::CommitToAdapter,
    std::optional<bool> enableEncodedInputCache = {},
    std::optional<bool> enableFullInputPrefetch = {},
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink = {});

IGAME_NAMESPACE_END

#endif
