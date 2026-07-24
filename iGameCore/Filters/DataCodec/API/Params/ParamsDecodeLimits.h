#ifndef DATACODEC_API_PARAMS_PARAMSDECODELIMITS_H
#define DATACODEC_API_PARAMS_PARAMSDECODELIMITS_H

#include <cstdint>
namespace datacodec {

// params 属于小 metadata，超过该值视为异常输入
inline constexpr std::uint64_t kMaxDecodedParamsBytes = 16u * 1024u * 1024u;

} // namespace datacodec

#endif
