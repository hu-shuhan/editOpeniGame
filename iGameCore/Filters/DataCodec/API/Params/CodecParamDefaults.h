#ifndef DATACODEC_API_PARAMS_CODECPARAMDEFAULTS_H
#define DATACODEC_API_PARAMS_CODECPARAMDEFAULTS_H

#include "DataCodec/API/Params/CodecControlParams.h"

namespace datacodec {

[[nodiscard]] inline CompressorConfig MakeAbsoluteErrorNumericArrayCompressor(
    const double absoluteError) {
    CompressorConfig compressor;
    compressor.options["pressio:abs"] = absoluteError;
    return compressor;
}

[[nodiscard]] inline CompressorConfig MakeRelativeErrorNumericArrayCompressor(
    const double relativeError) {
    CompressorConfig compressor;
    compressor.options["pressio:rel"] = relativeError;
    return compressor;
}

[[nodiscard]] inline CompressorConfig MakeLosslessNumericArrayCompressor() {
    return MakeAbsoluteErrorNumericArrayCompressor(0.0);
}

[[nodiscard]] inline CompressorConfig MakeDefaultGeometryValueCompressor() {
    return MakeLosslessNumericArrayCompressor();
}

[[nodiscard]] inline CompressorConfig MakeDefaultAttributeValueCompressor() {
    return MakeLosslessNumericArrayCompressor();
}

[[nodiscard]] inline CodecControlParams MakeDefaultCodecControlParams() {
    CodecControlParams params;
    params.geomControl.regionControl = MakeSingleRegionPrecisionControl(MakeDefaultGeometryValueCompressor());
    params.defaultAttrControl.regionControl = MakeSingleRegionPrecisionControl(MakeDefaultAttributeValueCompressor());
    return params;
}

} // namespace datacodec

#endif
