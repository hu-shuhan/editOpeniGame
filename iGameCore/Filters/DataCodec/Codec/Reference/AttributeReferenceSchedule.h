#ifndef DATACODEC_CODEC_REFERENCE_ATTRIBUTEREFERENCESCHEDULE_H
#define DATACODEC_CODEC_REFERENCE_ATTRIBUTEREFERENCESCHEDULE_H

#include "DataCodec/Codec/NumericArray/NumericArraySource.h"
#include "DataCodec/API/Params/CodecStorageParams.h"

#include <cstdint>
#include <vector>
namespace datacodec {

struct EncodeAttributeReferenceScheduleEntry {
    bool hasIntraParent{false};
    std::uint16_t parentMetaIndex{0xFFFFu};
    AttrStorageParams parentMeta;
    numericarray::NumericArraySource parentSource;
};

struct EncodeAttributeReferenceSchedule {
    bool initialized{false};
    std::vector<std::size_t> topologyOrder;
    std::vector<EncodeAttributeReferenceScheduleEntry> entries;
};

} // namespace datacodec

#endif
