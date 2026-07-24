#ifndef DATACODEC_VALIDATION_RESULT_VALIDATIONRESULT_H
#define DATACODEC_VALIDATION_RESULT_VALIDATIONRESULT_H

#include "DataCodec/Common/DataCodecError.h"

#include <string>
#include <utility>

namespace datacodec::validation {

enum class ValidationDomain {
    Common,
    Storage,
    Geometry,
    Topology,
    Attribute,
    Runtime,
    Workflow,
    Filter,
};

struct ValidationResult {
    bool success{true};
    CodecErrorCode code{CodecErrorCode::PipelineFailure};
    ValidationDomain domain{ValidationDomain::Common};
    std::string rule;
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept {
        return success;
    }

    [[nodiscard]] static ValidationResult Success(
        const ValidationDomain domain = ValidationDomain::Common,
        std::string rule = {}) {
        return ValidationResult{
            .success = true,
            .code = CodecErrorCode::PipelineFailure,
            .domain = domain,
            .rule = std::move(rule),
        };
    }

    [[nodiscard]] static ValidationResult Failure(
        const CodecErrorCode code,
        const ValidationDomain domain,
        std::string rule,
        std::string message) {
        return ValidationResult{
            .success = false,
            .code = code,
            .domain = domain,
            .rule = std::move(rule),
            .message = std::move(message),
        };
    }
};

} // namespace datacodec::validation

#endif
