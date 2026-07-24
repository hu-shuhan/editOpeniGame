#ifndef DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_COMMON_ENCODETRANSFERCACHE_H
#define DATACODEC_RUNTIME_CACHE_TRANSFERCACHE_COMMON_ENCODETRANSFERCACHE_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Storage/ByteIO/ByteSource.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
namespace datacodec {

struct EncodeTransferCacheSchedule {
    FieldType fieldType{FieldType::Params};
    std::size_t ordinal{0};
    AttrAttachment attachment{AttrAttachment::Point};
    EncodedFieldCodecType codecType{EncodedFieldCodecType::Unknown};
    std::string label;
    std::uint64_t rawSize{0};
    EncodedFieldCompressionType compressionType{EncodedFieldCompressionType::None};
};

struct EncodeTransferUnit {
    EncodeTransferCacheSchedule schedule;
    std::shared_ptr<bytestore::IByteSource> transferCache;
};

} // namespace datacodec

#endif
