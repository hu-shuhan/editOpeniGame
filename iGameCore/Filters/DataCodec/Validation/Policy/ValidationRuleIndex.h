#ifndef DATACODEC_VALIDATION_POLICY_VALIDATIONRULEINDEX_H
#define DATACODEC_VALIDATION_POLICY_VALIDATIONRULEINDEX_H

#include <array>
#include <cstdint>
#include <string_view>

namespace datacodec::validation {

enum class ValidationPolicyCoverage : std::uint8_t {
    None = 0u,
    Required = 1u << 0u,
    Strict = 1u << 1u,
    Audit = 1u << 2u,
};

enum class ValidationOperationCoverage : std::uint8_t {
    None = 0u,
    Encode = 1u << 0u,
    Decode = 1u << 1u,
};

[[nodiscard]] constexpr ValidationPolicyCoverage operator|(
    const ValidationPolicyCoverage left,
    const ValidationPolicyCoverage right) noexcept {
    return static_cast<ValidationPolicyCoverage>(
        static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right));
}

[[nodiscard]] constexpr bool HasCoverage(
    const ValidationPolicyCoverage value,
    const ValidationPolicyCoverage expected) noexcept {
    return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(expected)) != 0u;
}

[[nodiscard]] constexpr bool HasCoverage(
    const ValidationOperationCoverage value,
    const ValidationOperationCoverage expected) noexcept {
    return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(expected)) != 0u;
}

struct ValidationRuleDescriptor {
    std::string_view rule;
    ValidationPolicyCoverage policies{ValidationPolicyCoverage::None};
    ValidationOperationCoverage operations{ValidationOperationCoverage::None};
};

inline constexpr std::array<ValidationRuleDescriptor, 25u> kValidationRuleIndex{{
    {"encode.request.contract", ValidationPolicyCoverage::Required |
        ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Encode},
    {"encode.runtime.resources", ValidationPolicyCoverage::Required |
        ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Encode},
    {"encode.pipeline.binding", ValidationPolicyCoverage::Strict |
        ValidationPolicyCoverage::Audit, ValidationOperationCoverage::Encode},
    {"encode.output.required", ValidationPolicyCoverage::Required,
        ValidationOperationCoverage::Encode},
    {"input.reader", ValidationPolicyCoverage::Required | ValidationPolicyCoverage::Strict |
        ValidationPolicyCoverage::Audit, ValidationOperationCoverage::Decode},
    {"package.preamble", ValidationPolicyCoverage::Required | ValidationPolicyCoverage::Strict |
        ValidationPolicyCoverage::Audit, ValidationOperationCoverage::Decode},
    {"runtime.decode.resources", ValidationPolicyCoverage::Required |
        ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"geometry.params.size", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"geometry.cache.shape", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"geometry.values.finite", ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"topology.params.size", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"topology.cache.structured", ValidationPolicyCoverage::Strict |
        ValidationPolicyCoverage::Audit, ValidationOperationCoverage::Decode},
    {"topology.cache.complete", ValidationPolicyCoverage::Strict |
        ValidationPolicyCoverage::Audit, ValidationOperationCoverage::Decode},
    {"topology.cache.kind", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"topology.cache.count", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"topology.cache.shape", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"topology.reference.complete", ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"attribute.cache.shape", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"attribute.cache.coverage", ValidationPolicyCoverage::Strict |
        ValidationPolicyCoverage::Audit, ValidationOperationCoverage::Decode},
    {"attribute.values.finite", ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"decode.output.required", ValidationPolicyCoverage::Required,
        ValidationOperationCoverage::Decode},
    {"decode.attribute.required", ValidationPolicyCoverage::Required,
        ValidationOperationCoverage::Decode},
    {"decode.output.strict", ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"filter.decode.success", ValidationPolicyCoverage::Required |
        ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
    {"filter.output.present", ValidationPolicyCoverage::Required |
        ValidationPolicyCoverage::Strict | ValidationPolicyCoverage::Audit,
        ValidationOperationCoverage::Decode},
}};

[[nodiscard]] constexpr const ValidationRuleDescriptor* FindValidationRule(
    const std::string_view rule) noexcept {
    for (const auto& descriptor : kValidationRuleIndex) {
        if (descriptor.rule == rule) {
            return &descriptor;
        }
    }
    return nullptr;
}

} // namespace datacodec::validation

#endif
