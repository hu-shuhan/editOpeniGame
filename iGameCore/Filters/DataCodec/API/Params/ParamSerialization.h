#ifndef DATACODEC_API_PARAMS_PARAMSERIALIZATION_H
#define DATACODEC_API_PARAMS_PARAMSERIALIZATION_H

#include "DataCodec/Common/DataCodecTypes.h"

#include <type_traits>
namespace datacodec {

namespace detail {

template<typename TEnum>
using EnumUnderlyingType = std::underlying_type_t<TEnum>;

// 用固定底层整数保存枚举，保证 params 二进制布局稳定
template<class Archive, typename TEnum>
inline void SaveEnum(Archive& archive, const TEnum value) {
    const auto rawValue = static_cast<EnumUnderlyingType<TEnum>>(value);
    archive(rawValue);
}

template<class Archive, typename TEnum>
inline void LoadEnum(Archive& archive, TEnum& value) {
    EnumUnderlyingType<TEnum> rawValue{};
    archive(rawValue);
    value = static_cast<TEnum>(rawValue);
}

} // namespace detail

} // namespace datacodec

#endif
