#ifndef DATACODEC_CODEC_REFERENCE_ENCODEREFERENCEFRAME_H
#define DATACODEC_CODEC_REFERENCE_ENCODEREFERENCEFRAME_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedAttributeCacheSet.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h"
#include "DataCodec/Codec/Remap/RemapProvider.h"

#include <memory>
namespace datacodec {

// 编码时的属性关键帧引用信息
struct EncodeAttributeReferenceFrame {
    IEncodeAdapter* adapter{nullptr};
    std::shared_ptr<DecodedAttributeCacheSet> attrReferenceCache;
    std::shared_ptr<IRemapProvider> pointOrderProvider;
    std::shared_ptr<IRemapProvider> pointInverseOrderProvider;
    std::shared_ptr<IRemapProvider> cellOrderProvider;
};

// 编码时的几何关键帧引用信息
struct EncodeGeometryReferenceFrame {
    std::shared_ptr<DecodedGeometryReferenceCache> geometryReferenceCache;
};

} // namespace datacodec

#endif
