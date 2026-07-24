#ifndef DATACODEC_VALIDATION_POLICY_CODECVALIDATIONPOLICY_H
#define DATACODEC_VALIDATION_POLICY_CODECVALIDATIONPOLICY_H

namespace datacodec {

// 解码运行期校验策略
// 这里只放开关，不放具体验证实现
enum class DecodeValidationMode {
    Required,
    Strict,
};

struct CodecValidationPolicy {
    DecodeValidationMode decodeMode{DecodeValidationMode::Required};
    bool validateTopologyReferences{false};
    bool validateFloatingPointValues{false};

    [[nodiscard]] bool StrictDecodeEnabled() const noexcept {
        return decodeMode == DecodeValidationMode::Strict;
    }
};

} // namespace datacodec

#endif
