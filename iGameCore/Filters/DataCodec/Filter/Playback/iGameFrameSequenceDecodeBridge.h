#ifndef iGameFrameSequenceDecodeBridge_h
#define iGameFrameSequenceDecodeBridge_h

#include "Attribute/iGameAttributeDataSource.h"
#include "DataCodec/API/Adapter/IDecodedFrameCache.h"
#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Workflow/Session/PlaybackSession.h"
#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/API/Params/DecodedFrameCacheParams.h"
#include "DataCodec/API/Params/EncodedInputCacheParams.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "iGameDataObject.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

// iGame读取层通过该桥接提交已经发现的帧包序列
// DataCodec会话和iGame对象组装仅在Filter实现层相遇
struct FrameSequenceDecodeRequest {
    std::vector<::datacodec::FrameDecodeSource> decodeSources;
    std::vector<std::uint32_t> selectedFrameOrder;
    std::uint32_t targetFrameIndex{0u};
    std::string sourceLabel;
    const ::datacodec::DecodeControlParams* controlParams{nullptr};
    const ::datacodec::DecodeExecutionOptions* executionOptions{nullptr};
    const ::datacodec::DataCodecDecodeConfigurationSource* configurationSource{nullptr};
    ::datacodec::DataCodecLanguage language{
        ::datacodec::DataCodecLanguage::SimplifiedChinese};
    ::datacodec::DecodedFrameCachePolicy decodedFrameCachePolicy;
    std::shared_ptr<::datacodec::IDecodedFrameCache> decodedFrameCache;
    ::datacodec::EncodedInputCachePolicy encodedInputCachePolicy;
    std::shared_ptr<::datacodec::IEncodedInputCache> encodedInputCache;
    std::shared_ptr<::datacodec::IParallelTaskRunner> parallelTaskRunner;
    bool loadAllAvailableAttributes{true};
    bool enableConsoleLog{true};
    std::shared_ptr<::datacodec::IRunRecordSink> runRecordSink;
};

struct FrameSequenceDecodeResult {
    bool success{false};
    DataObject::Pointer output;
    AttributeDataSourcePointer attributeDataSource;
    std::vector<::datacodec::TelemetryMessageRecord> messages;
};

[[nodiscard]] FrameSequenceDecodeResult DecodeFrameSequence(
    const FrameSequenceDecodeRequest& request);

IGAME_NAMESPACE_END

#endif
