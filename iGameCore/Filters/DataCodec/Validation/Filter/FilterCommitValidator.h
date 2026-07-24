#ifndef DATACODEC_VALIDATION_FILTER_FILTERCOMMITVALIDATOR_H
#define DATACODEC_VALIDATION_FILTER_FILTERCOMMITVALIDATOR_H

#include "DataCodec/Validation/Result/ValidationResult.h"

namespace datacodec::validation {

class FilterCommitValidator final {
public:
    [[nodiscard]] static ValidationResult ValidateDecodedOutput(
        const bool decodeSucceeded,
        const bool hasOutput) {
        if (!decodeSucceeded) {
            return ValidationResult::Failure(
                CodecErrorCode::DecodeFailure,
                ValidationDomain::Filter,
                "filter.decode.success",
                "DataCodec decode did not complete successfully");
        }
        if (!hasOutput) {
            return ValidationResult::Failure(
                CodecErrorCode::DecodeFailure,
                ValidationDomain::Filter,
                "filter.output.present",
                "decoded package did not produce an output object");
        }
        return ValidationResult::Success(
            ValidationDomain::Filter,
            "filter.output.present");
    }
};

} // namespace datacodec::validation

#endif
