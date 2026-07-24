#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKFORMAT_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBLOCKFORMAT_H

#include "DataCodec/Codec/NumericArray/NumericArrayLayout.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockWireFormat.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>
namespace datacodec {
namespace numericarray {

struct NumericArrayBlockParams : public NumericArrayLayout {
    const NumericArrayRegionControlParams* regionControl{nullptr};
    const std::vector<RegionRun>* regionRuns{nullptr};
};

inline NumericArrayBlockParams MakeNumericArrayBlockParams(const NumericArrayLayout& layout) {
    NumericArrayBlockParams params;
    params.dataType = layout.dataType;
    params.valueSize = layout.valueSize;
    params.elementCount = layout.elementCount;
    params.componentCount = layout.componentCount;
    params.regionControl = nullptr;
    params.regionRuns = nullptr;
    return params;
}

inline void ApplyNumericArrayControlParams(
    NumericArrayBlockParams& params,
    const NumericArrayControlParams& control) noexcept {
    params.regionControl = &control.regionControl;
    params.regionRuns = &control.regionRuns;
}

inline bool MakeNumericArrayBlockParamsFromMeta(
    const NumericArrayStorageParams& meta,
    NumericArrayBlockParams& params,
    std::string* error = nullptr) {
    NumericArrayLayout layout;
    if (!MakeNumericArrayLayoutFromMeta(meta, layout, error)) {
        return false;
    }
    params = MakeNumericArrayBlockParams(layout);
    params.regionRuns = nullptr;
    return true;
}

inline bool IsSupportedNumericArrayDataType(const DataType type) {
    switch (type) {
        case DataType::Float32:
        case DataType::Float64:
        case DataType::Int8:
        case DataType::UInt8:
        case DataType::Int16:
        case DataType::UInt16:
        case DataType::Int32:
        case DataType::UInt32:
        case DataType::Int64:
        case DataType::UInt64:
            return true;
    }
    return false;
}

inline bool IsIntegerNumericArrayDataType(const DataType type) noexcept {
    switch (type) {
        case DataType::Int8:
        case DataType::UInt8:
        case DataType::Int16:
        case DataType::UInt16:
        case DataType::Int32:
        case DataType::UInt32:
        case DataType::Int64:
        case DataType::UInt64:
            return true;
        case DataType::Float32:
        case DataType::Float64:
            return false;
    }
    return false;
}

inline std::uint64_t IntegerStorageMask(const std::size_t valueSize) noexcept {
    if (valueSize >= sizeof(std::uint64_t)) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return (std::uint64_t{1u} << (valueSize * 8u)) - 1u;
}

inline bool ValidateNumericArrayBlockParams(const NumericArrayBlockParams& params, std::string* error) {
    const auto expectedValueSize = DataTypeSize(params.dataType);
    if (expectedValueSize == 0u || !IsSupportedNumericArrayDataType(params.dataType)) {
        return validation::AssignError(error, "numeric array block data type is unsupported");
    }
    if (params.valueSize != expectedValueSize) {
        return validation::AssignError(error, "numeric array block value size does not match data type");
    }
    if (params.componentCount == 0u) {
        return validation::AssignError(error, "numeric array block component count is invalid");
    }
    return true;
}

struct EncodedNumericArrayComponentBytes {
    std::uint32_t componentIndex{0};
    NumericArrayBytesCodec bytesCodec{NumericArrayBytesCodec::RawBytes};
    std::vector<std::uint8_t> bytes;
};

struct EncodedNumericArrayComponentByteView {
    std::uint32_t componentIndex{0};
    NumericArrayBytesCodec bytesCodec{NumericArrayBytesCodec::RawBytes};
    std::span<const std::uint8_t> bytes;
};

inline bool ResolveNumericArrayBlockRawByteCount(
    const NumericArrayBlockParams& params,
    const std::uint32_t elementCount,
    std::size_t& rawByteCount,
    std::string* error) {
    rawByteCount = 0u;
    const auto componentCount = params.componentCount;
    if (componentCount == 0u && elementCount != 0u) {
        return validation::AssignError(error, "numeric array component count is invalid");
    }
    std::size_t valueCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(elementCount),
            componentCount,
            valueCount,
            "numeric array block value count",
            error)) {
        return false;
    }
    if (!validation::CheckedMulSizeT(
            valueCount,
            params.valueSize,
            rawByteCount,
            "numeric array block raw bytes",
            error)) {
        return false;
    }
    return true;
}

inline bool ResolveNumericArrayComponentRawByteCount(
    const NumericArrayBlockParams& params,
    const std::uint32_t elementCount,
    std::size_t& rawByteCount,
    std::string* error) {
    rawByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(elementCount),
            params.valueSize,
            rawByteCount,
            "numeric array component raw bytes",
            error)) {
        return false;
    }
    return true;
}

inline bool AppendNumericArrayComponentBundle(
    std::vector<std::uint8_t>& encodedBytes,
    const std::span<const EncodedNumericArrayComponentByteView> components,
    std::vector<NumericArrayComponentLayoutParams>* componentLayouts = nullptr,
    std::string* error = nullptr) {
    encodedBytes.clear();
    if (componentLayouts != nullptr) {
        componentLayouts->clear();
    }
    if (components.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return validation::AssignError(error, "numeric array component bytes count exceeds current block format");
    }
    for (const auto& component : components) {
        if (component.bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
            return validation::AssignError(error, "numeric array component bytes length exceeds current block format");
        }
        if (componentLayouts != nullptr) {
            componentLayouts->push_back(NumericArrayComponentLayoutParams{
                .componentIndex = component.componentIndex,
                .bytesCodec = component.bytesCodec,
                .encodedByteLength = static_cast<ParamSize>(component.bytes.size()),
            });
        }
        encodedBytes.insert(encodedBytes.end(), component.bytes.begin(), component.bytes.end());
    }
    return true;
}

inline bool AppendNumericArrayComponentBundle(
    std::vector<std::uint8_t>& encodedBytes,
    const std::vector<EncodedNumericArrayComponentBytes>& components,
    std::vector<NumericArrayComponentLayoutParams>* componentLayouts = nullptr,
    std::string* error = nullptr) {
    std::vector<EncodedNumericArrayComponentByteView> views;
    views.reserve(components.size());
    for (const auto& component : components) {
        views.push_back(EncodedNumericArrayComponentByteView{
            .componentIndex = component.componentIndex,
            .bytesCodec = component.bytesCodec,
            .bytes = std::span<const std::uint8_t>(component.bytes.data(), component.bytes.size()),
        });
    }
    return AppendNumericArrayComponentBundle(
        encodedBytes,
        std::span<const EncodedNumericArrayComponentByteView>(views.data(), views.size()),
        componentLayouts,
        error);
}

inline bool WriteEncodedNumericArrayComponentBlock(
    std::vector<std::uint8_t>& fragment,
    const NumericArrayBlockParams& params,
    NumericArrayBlockHeader header,
    const std::span<const EncodedNumericArrayComponentByteView> components,
    std::vector<NumericArrayComponentLayoutParams>* componentLayouts,
    std::vector<std::uint8_t>& encodedBytes,
    std::string* error = nullptr) {
    encodedBytes.clear();
    if (!ValidateNumericArrayBlockParams(params, error)) {
        return false;
    }
    const auto componentCount = params.componentCount;
    if (components.size() != componentCount) {
        return validation::AssignError(error, "numeric array component block does not contain every component");
    }

    std::vector<bool> seen(componentCount, false);
    for (const auto& component : components) {
        if (component.componentIndex >= componentCount || seen[component.componentIndex]) {
            return validation::AssignError(error, "numeric array component block has invalid component index");
        }
        seen[component.componentIndex] = true;
    }

    if (!AppendNumericArrayComponentBundle(encodedBytes, components, componentLayouts, error)) {
        return false;
    }
    if (encodedBytes.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return validation::AssignError(error, "numeric array component block bundle exceeds current block format");
    }
    header.bytesCodec = NumericArrayBytesCodec::NumericArrayCodec;
    header.encodedByteLength = static_cast<std::uint32_t>(encodedBytes.size());
    AppendNumericArrayBlock(fragment, header, {}, {}, encodedBytes);
    return true;
}

inline bool WriteEncodedNumericArrayComponentBlock(
    std::vector<std::uint8_t>& fragment,
    const NumericArrayBlockParams& params,
    NumericArrayBlockHeader header,
    const std::vector<EncodedNumericArrayComponentBytes>& components,
    std::string* error = nullptr) {
    std::vector<EncodedNumericArrayComponentByteView> views;
    views.reserve(components.size());
    for (const auto& component : components) {
        views.push_back(EncodedNumericArrayComponentByteView{
            .componentIndex = component.componentIndex,
            .bytesCodec = component.bytesCodec,
            .bytes = std::span<const std::uint8_t>(component.bytes.data(), component.bytes.size()),
        });
    }
    std::vector<std::uint8_t> encodedBytes;
    return WriteEncodedNumericArrayComponentBlock(
        fragment,
        params,
        header,
        std::span<const EncodedNumericArrayComponentByteView>(views.data(), views.size()),
        nullptr,
        encodedBytes,
        error);
}

} // namespace numericarray
} // namespace datacodec

#endif
