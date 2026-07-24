#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYSOURCE_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYSOURCE_H

#include "DataCodec/Codec/NumericArray/NumericArrayLayout.h"
#include "DataCodec/Codec/Remap/RemapProvider.h"
#include "DataCodec/Common/Views/ArrayViews.h"

#include <cstddef>
#include <memory>
#include <vector>
namespace datacodec {
namespace numericarray {

struct NumericArraySource {
    NumericArrayView values;
    const std::vector<IndexType>* order{nullptr};
    const IRemapProvider* orderProvider{nullptr};
    NumericArrayLayout layout;
    std::shared_ptr<const void> owner;

    [[nodiscard]] std::size_t ElementBytes() const noexcept {
        return layout.ElementBytes();
    }
};

} // namespace numericarray
} // namespace datacodec

#endif
