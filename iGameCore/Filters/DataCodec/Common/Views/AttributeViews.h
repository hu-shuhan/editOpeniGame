#ifndef DATACODEC_COMMON_VIEWS_ATTRIBUTEVIEWS_H
#define DATACODEC_COMMON_VIEWS_ATTRIBUTEVIEWS_H

#include "DataCodec/Common/Views/ArrayViews.h"
#include "DataCodec/Common/DataCodecTypes.h"

#include <string>
namespace datacodec {

struct EncodeAttributeView {
    std::string name;
    AttrRole role{AttrRole::Unknown};
    AttrAttachment attachment{AttrAttachment::Point};
    NumericArrayView values;
};

} // namespace datacodec

#endif
