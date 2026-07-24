#ifndef DATACODEC_VALIDATION_RUNTIME_RUNTIMEVALIDATOR_H
#define DATACODEC_VALIDATION_RUNTIME_RUNTIMEVALIDATOR_H

#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Validation/Result/ValidationResult.h"

#include <string>
#include <utility>

namespace datacodec::validation {

class RuntimeValidator final {
public:
    [[nodiscard]] static ValidationResult ValidateDecodeConfiguration(
        const DecodeControlParams& controlParams,
        const DataCodecRuntimeProfile runtimeProfile) {
        std::string error;
        if (!ValidateResourceBudgetControlParams(
                controlParams.resourceBudget,
                &error) ||
            !CodecControlParamsFactory::ValidateDecodeRuntimeConstraint(
                controlParams.resourceBudget,
                runtimeProfile,
                &error)) {
            return ValidationResult::Failure(
                CodecErrorCode::InvalidInput,
                ValidationDomain::Runtime,
                "runtime.decode.resources",
                std::move(error));
        }
        return ValidationResult::Success(
            ValidationDomain::Runtime,
            "runtime.decode.resources");
    }
};

} // namespace datacodec::validation

#endif
