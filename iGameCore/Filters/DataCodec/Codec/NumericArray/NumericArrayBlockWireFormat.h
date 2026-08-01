#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKWIREFORMAT_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKWIREFORMAT_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/NumericArrayParams.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>
namespace datacodec {

struct NumericArrayBlockHeader {
    NumericArrayBlockMode mode{NumericArrayBlockMode::NonReference};
    NumericArrayReferenceKind referenceKind{NumericArrayReferenceKind::None};
    NumericArrayReferenceCodecId codecId{NumericArrayReferenceCodecId::NonReference};
    std::uint16_t localParentFieldIndex{0xFFFFu};
    std::uint32_t elementOffset{0};
    std::uint32_t elementCount{0};
    std::uint32_t encodedByteLength{0};
    NumericArrayBytesCodec bytesCodec{NumericArrayBytesCodec::RawBytes};
    std::int32_t predictorOffset{0};
};

inline NumericArrayBlockLayoutParams MakeNumericArrayBlockLayoutParams(
    const NumericArrayBlockHeader& header,
    const std::span<const double> alpha,
    const std::span<const double> beta) {
    NumericArrayBlockLayoutParams layout;
    layout.mode = header.mode;
    layout.referenceKind = header.referenceKind;
    layout.localParentFieldIndex = header.localParentFieldIndex;
    layout.elementOffset = header.elementOffset;
    layout.elementCount = header.elementCount;
    layout.encodedByteLength = header.encodedByteLength;
    layout.bytesCodec = header.bytesCodec;
    layout.predictorOffset = header.predictorOffset;
    layout.alpha.assign(alpha.begin(), alpha.end());
    layout.beta.assign(beta.begin(), beta.end());
    return layout;
}

inline bool MakeNumericArrayBlockHeader(
    const NumericArrayBlockLayoutParams& layout,
    NumericArrayBlockHeader& header,
    std::string* error = nullptr) {
    if (layout.elementOffset > static_cast<ParamSize>(std::numeric_limits<std::uint32_t>::max()) ||
        layout.elementCount > static_cast<ParamSize>(std::numeric_limits<std::uint32_t>::max()) ||
        layout.encodedByteLength > static_cast<ParamSize>(std::numeric_limits<std::uint32_t>::max())) {
        return validation::AssignError(error, "numeric array block layout exceeds current block wire limits");
    }
    header = {};
    header.mode = layout.mode;
    header.referenceKind = layout.referenceKind;
    header.codecId = NumericArrayBlockModeCodecId(layout.mode);
    header.localParentFieldIndex = layout.localParentFieldIndex;
    header.elementOffset = static_cast<std::uint32_t>(layout.elementOffset);
    header.elementCount = static_cast<std::uint32_t>(layout.elementCount);
    header.encodedByteLength = static_cast<std::uint32_t>(layout.encodedByteLength);
    header.bytesCodec = layout.bytesCodec;
    header.predictorOffset = layout.predictorOffset;
    return true;
}

struct ParsedNumericArrayBlock {
    NumericArrayBlockHeader header;
    CompressorConfig backgroundCompressor;
    ParamSize backgroundEncodedByteLength{0};
    std::vector<NumericArrayComponentLayoutParams> componentLayouts;
    std::vector<NumericArrayRegionLayerLayoutParams> regionLayers;
    std::vector<double> alpha;
    std::vector<double> beta;
    std::span<const std::uint8_t> bytes;
};

inline std::uint16_t EncodePredictorOffsetStorage(const std::int32_t predictorOffset) {
    constexpr std::int32_t kBias = 0x8000;
    constexpr std::int32_t kMaxEncodableOffset = 0x7FFE;
    const auto clampedOffset = std::clamp(predictorOffset, -kBias, kMaxEncodableOffset);
    return static_cast<std::uint16_t>(kBias + clampedOffset);
}

inline std::int32_t DecodePredictorOffsetStorage(const std::uint16_t storedValue) {
    if (storedValue == 0xFFFFu) {
        return 0;
    }
    return static_cast<std::int32_t>(storedValue) - 0x8000;
}

inline void AppendNumericArrayBlock(
    std::vector<std::uint8_t>& output,
    const NumericArrayBlockHeader& header,
    const std::vector<double>& alpha,
    const std::vector<double>& beta,
    const std::span<const std::uint8_t> bytes) {
    (void)header;
    (void)alpha;
    (void)beta;
    output.insert(output.end(), bytes.begin(), bytes.end());
}

template<typename TValue>
inline bool WriteNumericArrayBlockScalar(
    bytestore::IByteWriter& writer,
    const TValue& value,
    std::string* error = nullptr) {
    using Storage = detail::WireStorageTypeT<TValue>;
    auto storage = detail::ToWireStorage(value);
    std::array<std::uint8_t, sizeof(Storage)> bytes{};
    for (std::size_t byteIndex = 0; byteIndex < sizeof(Storage); ++byteIndex) {
        bytes[byteIndex] =
            static_cast<std::uint8_t>((storage >> (byteIndex * 8u)) & Storage{0xFFu});
    }
    return writer.Write(std::span<const std::uint8_t>(bytes.data(), bytes.size()), error);
}

template<typename TValue>
inline bool WriteNumericArrayBlockArray(
    bytestore::IByteWriter& writer,
    const std::span<const TValue> values,
    std::string* error = nullptr) {
    for (const auto& value : values) {
        if (!WriteNumericArrayBlockScalar(writer, value, error)) {
            return false;
        }
    }
    return true;
}

inline bool WriteNumericArrayBlock(
    bytestore::IByteWriter& writer,
    const NumericArrayBlockHeader& header,
    const std::span<const double> alpha,
    const std::span<const double> beta,
    const std::span<const std::uint8_t> bytes,
    std::string* error = nullptr) {
    (void)header;
    (void)alpha;
    (void)beta;
    return writer.Write(bytes, error);
}

} // namespace datacodec

#endif
