#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYLAYOUT_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYLAYOUT_H

#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Common/Views/ArrayViews.h"
#include "DataCodec/API/Params/NumericArrayParams.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>
namespace datacodec {
namespace numericarray {

struct NumericArrayLayout {
    DataType dataType{DataType::Float32};
    std::size_t valueSize{sizeof(float)};
    std::size_t elementCount{0};
    std::size_t componentCount{0};

    [[nodiscard]] std::size_t ElementBytes() const noexcept {
        return validation::SaturatingMulSizeT(componentCount, valueSize);
    }

    [[nodiscard]] std::size_t ValueCount() const {
        return validation::SaturatingMulSizeT(elementCount, componentCount);
    }

    [[nodiscard]] std::size_t ByteCount() const {
        return validation::SaturatingMulSizeT(ValueCount(), valueSize);
    }

    [[nodiscard]] std::vector<std::size_t> BuildShape() const {
        if (componentCount <= 1u) {
            return {elementCount};
        }
        return {elementCount, componentCount};
    }

    [[nodiscard]] NumericArrayLayout WithElementCount(const std::size_t count) const noexcept {
        auto layout = *this;
        layout.elementCount = count;
        return layout;
    }
};

inline NumericArrayLayout MakeNumericArrayLayout(
    const DataType dataType,
    const std::size_t valueSize,
    const std::size_t elementCount,
    const std::size_t componentCount) {
    return NumericArrayLayout{
        .dataType = dataType,
        .valueSize = valueSize,
        .elementCount = elementCount,
        .componentCount = componentCount,
    };
}

inline NumericArrayLayout MakeNumericArrayLayoutFromView(
    const NumericArrayView& view,
    const std::size_t elementCount,
    const std::size_t componentCount) {
    return MakeNumericArrayLayout(
        ToDataType(view.scalarType),
        view.ComponentSize(),
        elementCount,
        componentCount);
}

inline NumericArrayLayout MakeNumericArrayLayoutFromView(
    const NumericArrayView& view,
    const std::size_t componentCount) {
    return MakeNumericArrayLayoutFromView(view, view.tupleCount, componentCount);
}

inline bool MakeNumericArrayLayoutFromMeta(
    const NumericArrayStorageParams& meta,
    NumericArrayLayout& layout,
    std::string* error = nullptr) {
    std::size_t elementCount = 0u;
    std::size_t valueSize = 0u;
    if (!TryParamSizeToSizeT(meta.elementCount, elementCount) ||
        !TryParamSizeToSizeT(meta.valueSize, valueSize)) {
        return validation::AssignError(error, "numeric array metadata exceeds this platform size limit");
    }
    layout = MakeNumericArrayLayout(
        meta.dataType,
        valueSize,
        elementCount,
        static_cast<std::size_t>(std::max(meta.dimension, 0)));
    return true;
}

} // namespace numericarray
} // namespace datacodec

#endif
