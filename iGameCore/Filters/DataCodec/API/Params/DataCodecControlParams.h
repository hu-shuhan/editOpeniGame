#ifndef DATACODEC_API_PARAMS_DATACODECCONTROLPARAMS_H
#define DATACODEC_API_PARAMS_DATACODECCONTROLPARAMS_H

#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/API/Params/CodecPerformanceParams.h"
#include "DataCodec/Validation/Policy/CodecValidationPolicy.h"

namespace datacodec {

// encode 控制参数，沿用现有 CodecControlParams 全字段
// 后续阶段按需剔除 decode-only 字段
using EncodeCodecControlParams = CodecControlParams;

// decode 控制参数，只含 decode 真正需要的控制项
// 不接收区域精度表、region runs、encode reference/remap 策略
struct DecodeControlParams {
    CodecValidationPolicy validation;
    ResourceBudgetControlParams resourceBudget;

    DecodeControlParams& SetDecodeValidationMode(const DecodeValidationMode mode) noexcept {
        validation.decodeMode = mode;
        return *this;
    }
};

} // namespace datacodec

#endif
