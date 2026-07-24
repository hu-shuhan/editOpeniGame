#ifndef DATACODEC_API_ADAPTER_IDECODEDFRAMEATTRIBUTEACCESS_H
#define DATACODEC_API_ADAPTER_IDECODEDFRAMEATTRIBUTEACCESS_H

#include "DataCodec/API/Adapter/IDecodedFrameCache.h"
#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Workflow/Session/DecodeSession.h"

#include <memory>
#include <stop_token>
#include <vector>

namespace datacodec {

struct DecodedFrameAttributeRequest {
    std::vector<AttributeTarget> attributeTargets;
    AttributeDecodeRequestMode mode{AttributeDecodeRequestMode::DecodeAndCommit};
    std::shared_ptr<IRunRecordSink> runRecordSink;
    std::stop_token stopToken;
};

struct DecodedFrameAttributeResult {
    bool success{false};
    bool cancelled{false};
    std::vector<TelemetryMessageRecord> messages;
};

// 将完整解码帧上的属性访问与会话实现隔离
// 框架桥接层只依赖该合同和完整帧租约
class IDecodedFrameAttributeAccess {
public:
    using Pointer = std::shared_ptr<IDecodedFrameAttributeAccess>;

    virtual ~IDecodedFrameAttributeAccess() = default;

    [[nodiscard]] virtual DecodedFrameLease::Pointer Frame() const noexcept = 0;
    [[nodiscard]] virtual DecodedFrameCacheLookupResult FindCachedFrame(
        std::uint32_t frameIndex) const = 0;
    [[nodiscard]] virtual Pointer ForFrame(DecodedFrameLease::Pointer frame) const = 0;
    [[nodiscard]] virtual std::vector<DecodeAttributeDescriptor> AvailableAttributes() const = 0;
    [[nodiscard]] virtual DecodedFrameAttributeResult RequestAttributes(
        const DecodedFrameAttributeRequest& request) = 0;
};

} // namespace datacodec

#endif
