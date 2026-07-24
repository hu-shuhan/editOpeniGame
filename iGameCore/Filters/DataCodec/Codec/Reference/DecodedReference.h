#ifndef DATACODEC_CODEC_REFERENCE_DECODEDREFERENCE_H
#define DATACODEC_CODEC_REFERENCE_DECODEDREFERENCE_H

#include "DataCodec/Runtime/Cache/DecodeCache/DecodedAttributeCacheSet.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h"
#include "DataCodec/Storage/LeafPackage/LeafPackage.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <memory>
namespace datacodec {

struct AttributeReference {
    CodecStorageParams storageParams;
    LeafPackage leafPackage;
};

struct DecodedAttributeReference {
    AttributeReference reference;
    std::shared_ptr<DecodedAttributeCacheSet> store;
    std::shared_ptr<bytestore::ByteStoreSession> byteStoreSession;
};

struct GeometryReference {
    CodecStorageParams storageParams;
    LeafPackage leafPackage;
};

struct DecodedGeometryReference {
    GeometryReference reference;
    std::shared_ptr<DecodedGeometryReferenceCache> store;
    std::shared_ptr<bytestore::ByteStoreSession> byteStoreSession;
};

} // namespace datacodec

#endif
