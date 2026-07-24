#ifndef DATACODEC_WORKFLOW_FRAMESEQUENCE_FRAMEDECODESOURCE_H
#define DATACODEC_WORKFLOW_FRAMESEQUENCE_FRAMEDECODESOURCE_H

#include "DataCodec/Storage/ByteIO/ByteRange.h"
#include "DataCodec/Runtime/Cache/DecodeCacheIdentity.h"
#include "DataCodec/Storage/FramePackage/FramePackageFormat.h"

#include <cstdint>
#include <memory>

namespace datacodec {

// 描述已发现的一帧包及其稳定缓存身份
struct FrameDecodeSource {
    std::uint32_t frameIndex{0u};
    float timeValue{0.0f};
    std::shared_ptr<IByteRangeReader> frameReader;
    DecodeSourceIdentity sourceIdentity;
    std::shared_ptr<const FramePackage> framePackage;
};

} // namespace datacodec

#endif
