#ifndef iGameDataCodecDataObjectBridge_h
#define iGameDataCodecDataObjectBridge_h

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Adapter/IDecodedFrameCache.h"
#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Workflow/Session/DecodeSession.h"
#include "DataCodec/API/Params/DataCodecControlParams.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/API/Params/EncodedInputCacheParams.h"
#include "DataCodec/Runtime/Cache/DecodeCacheRuntime.h"
#include "DataCodec/Runtime/Execution/DataCodecExecutionResources.h"
#include "iGameDataObject.h"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

struct DataCodecDataObjectDecodeRequest {
    std::shared_ptr<::datacodec::IByteRangeReader> inputReader;
    // 解码结果缓存和编码输入缓存需要调用方提供稳定身份和内容版本
    ::datacodec::DecodeSourceIdentity inputSourceIdentity;
    const ::datacodec::DecodeControlParams* controlParams{nullptr};
    const ::datacodec::DecodeExecutionOptions* executionOptions{nullptr};
    const ::datacodec::DataCodecDecodeConfigurationSource* configurationSource{nullptr};
    ::datacodec::DataCodecLanguage language{
        ::datacodec::DataCodecLanguage::SimplifiedChinese};
    ::datacodec::DecodedFrameCachePolicy decodedFrameCachePolicy;
    std::shared_ptr<::datacodec::IDecodedFrameCache> decodedFrameCache;
    ::datacodec::EncodedInputCachePolicy encodedInputCachePolicy;
    std::shared_ptr<::datacodec::IEncodedInputCache> encodedInputCache;
    std::shared_ptr<::datacodec::DecodeCacheRuntime> cacheRuntime;
    ::datacodec::DataCodecExecutionResources executionResources;
    std::optional<std::uint32_t> requestedFrameIndex;
    std::vector<::datacodec::AttributeTarget> attributeTargets;
    // true 表示一次完成几何、拓扑和全部属性解压
    bool loadAllAvailableAttributes{false};
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink;
    std::stop_token stopToken;
};

struct DataCodecDataObjectAttributeRequest {
    std::vector<::datacodec::AttributeTarget> attributeTargets;
    ::datacodec::AttributeDecodeRequestMode mode{
        ::datacodec::AttributeDecodeRequestMode::DecodeAndCommit};
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink;
    std::stop_token stopToken;
};

struct DataCodecDataObjectDecodeResult {
    bool success{false};
    bool decodedFrameCacheHit{false};
    DataObject::Pointer output;
    std::uint64_t inputBytes{0u};
    std::vector<::datacodec::TelemetryMessageRecord> messages;
};

class DataCodecDataObjectDecodeSession {
public:
    DataCodecDataObjectDecodeSession();
    ~DataCodecDataObjectDecodeSession();

    DataCodecDataObjectDecodeSession(const DataCodecDataObjectDecodeSession&) = delete;
    DataCodecDataObjectDecodeSession& operator=(const DataCodecDataObjectDecodeSession&) = delete;
    DataCodecDataObjectDecodeSession(DataCodecDataObjectDecodeSession&&) noexcept;
    DataCodecDataObjectDecodeSession& operator=(DataCodecDataObjectDecodeSession&&) noexcept;

    [[nodiscard]] DataCodecDataObjectDecodeResult Open(
        const DataCodecDataObjectDecodeRequest& request);
    [[nodiscard]] DataCodecDataObjectDecodeResult RequestAttributes(
        const DataCodecDataObjectAttributeRequest& request);
    [[nodiscard]] std::vector<::datacodec::DecodeAttributeDescriptor> AvailableAttributes() const;
    [[nodiscard]] int NativeAttributeIndex(
        const ::datacodec::AttributeTarget& target) const;
    [[nodiscard]] DataObject::Pointer OutputForTarget(
        const ::datacodec::AttributeTarget& target) const;
    [[nodiscard]] DataObject::Pointer GetOutput() const;
    [[nodiscard]] bool IsOpen() const;
    void Reset();

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

[[nodiscard]] DataCodecDataObjectDecodeResult DecodeDataCodecDataObject(
    const DataCodecDataObjectDecodeRequest& request);

IGAME_NAMESPACE_END

#endif
