#ifndef DATACODEC_API_PARAMS_TIMESERIESCONTROLPARAMS_H
#define DATACODEC_API_PARAMS_TIMESERIESCONTROLPARAMS_H

#include "DataCodec/API/Params/CodecControlParams.h"

#include <cstdint>

namespace datacodec
{

inline void RemoveAttributeRegionPrecisionForTimeSeries(NumericArrayControlParams& control) {
    control.regionControl.regions.clear();
    control.regionRuns.clear();
}

inline void PrepareTimeSeriesControlParamsForRandomAccess(CodecControlParams& params) {
    constexpr std::uint32_t kDefaultRandomAccessKeyFrameInterval = 8u;
    RemoveAttributeRegionPrecisionForTimeSeries(params.defaultAttrControl);
    for (auto& [name, control]: params.attrControl) {
        (void) name;
        RemoveAttributeRegionPrecisionForTimeSeries(control);
    }

    params.attrReference.temporalField.forcePredFrames = false;
    params.geometryReference.temporalField.forcePredFrames = false;
    if (params.attrReference.temporalField.keyFrameInterval == 0u) {
        params.attrReference.temporalField.keyFrameInterval = kDefaultRandomAccessKeyFrameInterval;
    }
    if (params.geometryReference.temporalField.keyFrameInterval == 0u) {
        params.geometryReference.temporalField.keyFrameInterval = kDefaultRandomAccessKeyFrameInterval;
    }
}

} // datacodec命名空间

#endif
