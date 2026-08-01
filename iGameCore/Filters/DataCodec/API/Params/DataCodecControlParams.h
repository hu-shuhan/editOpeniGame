#ifndef DATACODEC_API_PARAMS_DATACODECCONTROLPARAMS_H
#define DATACODEC_API_PARAMS_DATACODECCONTROLPARAMS_H

#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecPerformanceParams.h"
#include "DataCodec/Validation/Policy/CodecValidationPolicy.h"

namespace datacodec {

// encode 控制参数
using EncodeCodecControlParams = CodecControlParams;

// decode 控制参数
struct DecodeControlParams {
    CodecValidationPolicy validation;
    DecodeResourceBudgetControlParams resourceBudget;

    DecodeControlParams& SetDecodeValidationMode(const DecodeValidationMode mode) noexcept {
        validation.decodeMode = mode;
        return *this;
    }
};

} // namespace datacodec

#endif
