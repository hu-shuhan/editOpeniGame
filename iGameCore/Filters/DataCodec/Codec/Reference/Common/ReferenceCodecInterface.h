#ifndef DATACODEC_CODEC_REFERENCE_COMMON_REFERENCECODECINTERFACE_H
#define DATACODEC_CODEC_REFERENCE_COMMON_REFERENCECODECINTERFACE_H

#include "DataCodec/Codec/Reference/Common/ReferenceTypes.h"

#include <cstdint>
#include <string>
#include <vector>
namespace datacodec {

class INumericArrayReferenceCodec {
public:
    virtual ~INumericArrayReferenceCodec() = default;
    [[nodiscard]] virtual NumericArrayReferenceCodecId CodecId() const noexcept = 0;
    [[nodiscard]] virtual NumericArrayReferenceEncodeResult PrepareBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        NumericArrayReferencePreparedBlock& prepared,
        std::string* error = nullptr) const = 0;
    [[nodiscard]] virtual NumericArrayReferenceEncodeResult EncodePreparedBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        const NumericArrayReferencePreparedBlock& prepared,
        NumericArrayReferenceEncodedBlock& output,
        std::string* error = nullptr) const = 0;
    [[nodiscard]] NumericArrayReferenceEncodeResult EncodeBlock(
        const NumericArrayReferenceCodecEncodeInput& input,
        NumericArrayReferenceEncodedBlock& output,
        std::string* error = nullptr) const {
        NumericArrayReferencePreparedBlock prepared;
        const auto prepareResult = PrepareBlock(input, prepared, error);
        if (!prepareResult.IsEncoded()) {
            output = {};
            return prepareResult;
        }
        return EncodePreparedBlock(input, prepared, output, error);
    }
    [[nodiscard]] virtual bool DecodeBlock(
        const NumericArrayReferenceCodecDecodeInput& input,
        std::vector<std::uint8_t>& decodedBlockBytes,
        std::string* error = nullptr) const = 0;
};

} // namespace datacodec

#endif
