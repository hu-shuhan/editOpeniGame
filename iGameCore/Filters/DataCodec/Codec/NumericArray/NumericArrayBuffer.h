#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBUFFER_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYBUFFER_H

#include "DataCodec/Codec/NumericArray/NumericArrayLayout.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <libpressio.h>
namespace datacodec::numericarray {

struct PressioDataDeleter {
    void operator()(pressio_data* data) const {
        if (data != nullptr) {
            pressio_data_free(data);
        }
    }
};

using PressioDataHandle = std::unique_ptr<pressio_data, PressioDataDeleter>;

struct NumericArrayBufferLayout : public NumericArrayLayout {
    std::vector<std::size_t> shape;

    [[nodiscard]] std::size_t ValueCount() const {
        if (!this->shape.empty()) {
            std::size_t count = 1u;
            for (const auto extent : this->shape) {
                if (extent == 0u) {
                    return 0u;
                }
                count = validation::SaturatingMulSizeT(count, extent);
            }
            return count;
        }
        return NumericArrayLayout::ValueCount();
    }

    [[nodiscard]] std::size_t ByteCount() const {
        return validation::SaturatingMulSizeT(ValueCount(), valueSize);
    }

    [[nodiscard]] std::vector<std::size_t> BuildShape() const {
        if (!this->shape.empty()) {
            return this->shape;
        }
        return NumericArrayLayout::BuildShape();
    }
};

struct NumericArrayBufferView {
    const void* data{nullptr};
    NumericArrayBufferLayout layout;
};

struct MutableNumericArrayBufferView {
    void* data{nullptr};
    NumericArrayBufferLayout layout;
};

struct NumericArraySegmentSpec {
    std::size_t elementOffset{0};
    std::size_t elementCount{0};
    CompressorConfig compressor;
};

class NumericArrayEncodedBytes {
public:
    [[nodiscard]] std::span<const std::uint8_t> Bytes() const noexcept {
        if (this->pressioData != nullptr) {
            return this->pressioBytes;
        }
        return std::span<const std::uint8_t>(this->ownedBytes.data(), this->ownedBytes.size());
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        return this->Bytes().size();
    }

    [[nodiscard]] bool Empty() const noexcept {
        return this->Bytes().empty();
    }

    void Reset() {
        std::vector<std::uint8_t>().swap(this->ownedBytes);
        this->pressioBytes = {};
        this->pressioData.reset();
    }

    void TakeVector(std::vector<std::uint8_t>&& bytes) {
        this->Reset();
        this->ownedBytes = std::move(bytes);
    }

    void CopyFrom(const std::span<const std::uint8_t> bytes) {
        this->Reset();
        this->ownedBytes.assign(bytes.begin(), bytes.end());
    }

    [[nodiscard]] std::vector<std::uint8_t> MaterializeVector() const {
        const auto bytes = this->Bytes();
        return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
    }

    void TakePressioData(PressioDataHandle data, const std::span<const std::uint8_t> bytes) {
        this->Reset();
        this->pressioBytes = bytes;
        this->pressioData = std::move(data);
    }

private:
    std::vector<std::uint8_t> ownedBytes;
    PressioDataHandle pressioData;
    std::span<const std::uint8_t> pressioBytes;
};

struct NumericArrayCompressedSegment {
    std::size_t elementOffset{0};
    NumericArrayBufferLayout layout;
    CompressorConfig compressor;
    std::vector<std::uint8_t> bytes;
};

inline NumericArrayBufferLayout MakeNumericArrayComponentLayout(
    const DataType dataType,
    const std::size_t valueSize,
    const std::size_t elementCount) {
    NumericArrayBufferLayout layout;
    layout.dataType = dataType;
    layout.valueSize = valueSize;
    layout.elementCount = elementCount;
    layout.componentCount = 1u;
    layout.shape = {elementCount};
    return layout;
}

inline NumericArrayBufferLayout MakeNumericArrayBufferLayout(
    const NumericArrayLayout& logical,
    std::vector<std::size_t> shape = {}) {
    NumericArrayBufferLayout layout;
    layout.dataType = logical.dataType;
    layout.valueSize = logical.valueSize;
    layout.elementCount = logical.elementCount;
    layout.componentCount = logical.componentCount;
    layout.shape = std::move(shape);
    return layout;
}

} // namespace datacodec::numericarray

#endif
