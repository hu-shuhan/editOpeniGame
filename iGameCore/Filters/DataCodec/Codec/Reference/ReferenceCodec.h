#ifndef DATACODEC_CODEC_REFERENCE_REFERENCECODEC_H
#define DATACODEC_CODEC_REFERENCE_REFERENCECODEC_H

#include "DataCodec/Codec/Reference/Common/ReferenceTypes.h"
#include "DataCodec/Codec/Reference/Common/ReferenceCodecInterface.h"
#include "DataCodec/Codec/Reference/Common/ReferenceBlockIO.h"
#include "DataCodec/Codec/Reference/Common/ReferenceDelta.h"
#include "DataCodec/Codec/Reference/Common/AffineReferenceCodec.h"
#include "DataCodec/Codec/Reference/Common/PredictorReferenceCodec.h"
#include "DataCodec/Codec/Reference/Common/WaveletReferenceCodec.h"
namespace datacodec {

class ReferenceCodecRegistry {
public:
    [[nodiscard]] static const INumericArrayReferenceCodec* Resolve(const NumericArrayReferenceCodecId codecId) noexcept {
        switch (codecId) {
            case NumericArrayReferenceCodecId::Affine:
                return &Affine();
            case NumericArrayReferenceCodecId::Wavelet:
                return &Wavelet();
            case NumericArrayReferenceCodecId::Predictor:
                return &Predictor();
            case NumericArrayReferenceCodecId::NonReference:
            default:
                return nullptr;
        }
    }

private:
    [[nodiscard]] static const INumericArrayReferenceCodec& Affine() noexcept {
        static const AffineReferenceCodec codec;
        return codec;
    }

    [[nodiscard]] static const INumericArrayReferenceCodec& Wavelet() noexcept {
        static const WaveletReferenceCodec codec;
        return codec;
    }

    [[nodiscard]] static const INumericArrayReferenceCodec& Predictor() noexcept {
        static const PredictorReferenceCodec codec;
        return codec;
    }
};

inline const INumericArrayReferenceCodec* ResolveNumericArrayReferenceCodec(const NumericArrayReferenceCodecId codecId) noexcept {
    return ReferenceCodecRegistry::Resolve(codecId);
}

} // namespace datacodec

#endif
