#ifndef DATACODEC_VALIDATION_WORKFLOW_DECODEVALIDATIONLIFECYCLE_H
#define DATACODEC_VALIDATION_WORKFLOW_DECODEVALIDATIONLIFECYCLE_H

#include <array>
#include <cstdint>
#include <string_view>

namespace datacodec::validation {

enum class DecodeValidationNode : std::uint8_t {
    InputBoundary,
    FormatAndParams,
    AlgorithmPrecondition,
    AlgorithmExecution,
    OutputInvariant,
    Commit,
};

inline constexpr std::array<DecodeValidationNode, 6u> kDecodeValidationLifecycle{
    DecodeValidationNode::InputBoundary,
    DecodeValidationNode::FormatAndParams,
    DecodeValidationNode::AlgorithmPrecondition,
    DecodeValidationNode::AlgorithmExecution,
    DecodeValidationNode::OutputInvariant,
    DecodeValidationNode::Commit,
};

[[nodiscard]] inline std::string_view DecodeValidationNodeName(
    const DecodeValidationNode node) noexcept {
    switch (node) {
        case DecodeValidationNode::InputBoundary:
            return "DecodeValidation.InputBoundary";
        case DecodeValidationNode::FormatAndParams:
            return "DecodeValidation.FormatAndParams";
        case DecodeValidationNode::AlgorithmPrecondition:
            return "DecodeValidation.AlgorithmPrecondition";
        case DecodeValidationNode::AlgorithmExecution:
            return "DecodeValidation.AlgorithmExecution";
        case DecodeValidationNode::OutputInvariant:
            return "DecodeValidation.OutputInvariant";
        case DecodeValidationNode::Commit:
            return "DecodeValidation.Commit";
    }
    return "DecodeValidation.Unknown";
}

} // namespace datacodec::validation

#endif
