#ifndef DATACODEC_COMMON_VIEWS_ARRAYVIEWS_H
#define DATACODEC_COMMON_VIEWS_ARRAYVIEWS_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace datacodec {

enum class ScalarType : std::uint8_t {
    Float32 = 0,
    Float64 = 1,
    Int8 = 2,
    UInt8 = 3,
    Int16 = 4,
    UInt16 = 5,
    Int32 = 6,
    UInt32 = 7,
    Int64 = 8,
    UInt64 = 9,
};

enum class IndexValueType : std::uint8_t {
    UInt32 = 0,
    UInt64 = 1,
};

enum class ArrayLayout : std::uint8_t {
    CompactAOS = 0,
    StridedAOS = 1,
    SOA = 2,
    GetterOnly = 3,
};

enum class ViewBufferOrigin : std::uint8_t {
    Borrowed = 0,
    Converted = 1,
    CodecCache = 2,
};

inline std::size_t ScalarTypeSize(const ScalarType type) {
    switch (type) {
        case ScalarType::Float32:
            return sizeof(float);
        case ScalarType::Float64:
            return sizeof(double);
        case ScalarType::Int8:
            return sizeof(std::int8_t);
        case ScalarType::UInt8:
            return sizeof(std::uint8_t);
        case ScalarType::Int16:
            return sizeof(std::int16_t);
        case ScalarType::UInt16:
            return sizeof(std::uint16_t);
        case ScalarType::Int32:
            return sizeof(std::int32_t);
        case ScalarType::UInt32:
            return sizeof(std::uint32_t);
        case ScalarType::Int64:
            return sizeof(std::int64_t);
        case ScalarType::UInt64:
            return sizeof(std::uint64_t);
    }
    return 0;
}

inline ScalarType ToScalarType(const DataType type) {
    switch (type) {
        case DataType::Float32:
            return ScalarType::Float32;
        case DataType::Float64:
            return ScalarType::Float64;
        case DataType::Int8:
            return ScalarType::Int8;
        case DataType::UInt8:
            return ScalarType::UInt8;
        case DataType::Int16:
            return ScalarType::Int16;
        case DataType::UInt16:
            return ScalarType::UInt16;
        case DataType::Int32:
            return ScalarType::Int32;
        case DataType::Int64:
            return ScalarType::Int64;
        case DataType::UInt32:
            return ScalarType::UInt32;
        case DataType::UInt64:
            return ScalarType::UInt64;
    }
    return ScalarType::Float64;
}

inline DataType ToDataType(const ScalarType type) {
    switch (type) {
        case ScalarType::Float32:
            return DataType::Float32;
        case ScalarType::Float64:
            return DataType::Float64;
        case ScalarType::Int8:
            return DataType::Int8;
        case ScalarType::UInt8:
            return DataType::UInt8;
        case ScalarType::Int16:
            return DataType::Int16;
        case ScalarType::UInt16:
            return DataType::UInt16;
        case ScalarType::Int32:
            return DataType::Int32;
        case ScalarType::UInt32:
            return DataType::UInt32;
        case ScalarType::Int64:
            return DataType::Int64;
        case ScalarType::UInt64:
            return DataType::UInt64;
    }
    return DataType::Float64;
}

struct NumericArrayView {
    static constexpr std::size_t kInlineComponentSlotCount = 16u;

    using GetTupleBytesFn = bool (*)(
        const void* userData,
        std::size_t tupleIndex,
        void* output,
        std::string* error);
    using GetTupleRangeBytesFn = bool (*)(
        const void* userData,
        std::size_t tupleIndex,
        std::size_t tupleCount,
        void* output,
        std::size_t outputByteSize,
        std::string* error);

    ScalarType scalarType{ScalarType::Float32};
    ArrayLayout layout{ArrayLayout::GetterOnly};
    ViewBufferOrigin origin{ViewBufferOrigin::Borrowed};
    const void* data{nullptr};
    std::size_t tupleCount{0};
    int componentCount{0};
    std::size_t tupleStrideBytes{0};
    std::array<const void*, kInlineComponentSlotCount> inlineComponentData{};
    std::array<std::size_t, kInlineComponentSlotCount> inlineComponentStrideBytes{};
    std::vector<const void*> dynamicComponentData;
    std::vector<std::size_t> dynamicComponentStrideBytes;
    const void* userData{nullptr};
    GetTupleBytesFn getTupleBytes{nullptr};
    GetTupleRangeBytesFn getTupleRangeBytes{nullptr};

    [[nodiscard]] std::size_t ComponentSize() const { return ScalarTypeSize(scalarType); }

    [[nodiscard]] std::size_t ComponentSlotCount() const noexcept {
        return dynamicComponentData.empty() ? inlineComponentData.size() : dynamicComponentData.size();
    }

    [[nodiscard]] const void* ComponentData(const std::size_t componentIndex) const noexcept {
        if (!dynamicComponentData.empty()) {
            return componentIndex < dynamicComponentData.size() ? dynamicComponentData[componentIndex] : nullptr;
        }
        return componentIndex < inlineComponentData.size() ? inlineComponentData[componentIndex] : nullptr;
    }

    [[nodiscard]] std::size_t ComponentStrideBytes(const std::size_t componentIndex) const noexcept {
        if (!dynamicComponentStrideBytes.empty()) {
            return componentIndex < dynamicComponentStrideBytes.size() ? dynamicComponentStrideBytes[componentIndex] : 0u;
        }
        return componentIndex < inlineComponentStrideBytes.size() ? inlineComponentStrideBytes[componentIndex] : 0u;
    }

    [[nodiscard]] bool EnsureComponentSlots(const std::size_t requiredCount) {
        inlineComponentData.fill(nullptr);
        inlineComponentStrideBytes.fill(0u);
        dynamicComponentData.clear();
        dynamicComponentStrideBytes.clear();
        if (requiredCount <= inlineComponentData.size()) {
            return true;
        }
        try {
            dynamicComponentData.assign(requiredCount, nullptr);
            dynamicComponentStrideBytes.assign(requiredCount, 0u);
        } catch (...) {
            dynamicComponentData.clear();
            dynamicComponentStrideBytes.clear();
            return false;
        }
        return true;
    }

    [[nodiscard]] bool SetComponentSlot(
        const std::size_t componentIndex,
        const void* componentPointer,
        const std::size_t strideBytes) {
        if (!dynamicComponentData.empty()) {
            if (componentIndex >= dynamicComponentData.size()) {
                return false;
            }
            dynamicComponentData[componentIndex] = componentPointer;
            dynamicComponentStrideBytes[componentIndex] = strideBytes;
            return true;
        }
        if (componentIndex >= inlineComponentData.size()) {
            return false;
        }
        inlineComponentData[componentIndex] = componentPointer;
        inlineComponentStrideBytes[componentIndex] = strideBytes;
        return true;
    }

    [[nodiscard]] std::size_t CompactTupleSize() const {
        return validation::SaturatingMulSizeT(
            static_cast<std::size_t>(componentCount > 0 ? componentCount : 0),
            ComponentSize());
    }

    [[nodiscard]] bool HasRawData() const noexcept {
        return layout == ArrayLayout::CompactAOS || layout == ArrayLayout::StridedAOS;
    }

    [[nodiscard]] bool IsCompact() const noexcept {
        return layout == ArrayLayout::CompactAOS && data != nullptr;
    }

    [[nodiscard]] bool IsValid() const noexcept {
        if (tupleCount == 0) {
            return true;
        }
        if (componentCount <= 0) {
            return false;
        }
        const auto componentSize = ComponentSize();
        if (componentSize == 0u) {
            return false;
        }
        if (layout == ArrayLayout::GetterOnly) {
            return getTupleBytes != nullptr;
        }
        if (layout == ArrayLayout::SOA) {
            const auto requiredComponents = static_cast<std::size_t>(componentCount);
            if (requiredComponents > ComponentSlotCount()) {
                return false;
            }
            for (std::size_t componentIndex = 0u; componentIndex < requiredComponents; ++componentIndex) {
                const auto strideBytes = ComponentStrideBytes(componentIndex);
                if (ComponentData(componentIndex) == nullptr ||
                    (strideBytes != 0u && strideBytes < componentSize)) {
                    return false;
                }
            }
            return true;
        }
        if (layout == ArrayLayout::StridedAOS) {
            return data != nullptr && tupleStrideBytes >= CompactTupleSize();
        }
        return layout == ArrayLayout::CompactAOS && data != nullptr;
    }
};

template<typename TScalar>
inline void WriteDoubleTupleAsScalarBytes(
    const double* input,
    const std::size_t componentCount,
    void* output) {
    auto* target = static_cast<TScalar*>(output);
    for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
        target[componentIndex] = static_cast<TScalar>(input[componentIndex]);
    }
}

inline bool WriteDoubleTupleAsScalarBytes(
    const ScalarType scalarType,
    const double* input,
    const std::size_t componentCount,
    void* output,
    std::string* error = nullptr) {
    if (componentCount == 0u) {
        return true;
    }
    if (input == nullptr || output == nullptr) {
        return validation::AssignError(error, "numeric tuple conversion received a null buffer");
    }
    switch (scalarType) {
        case ScalarType::Float32:
            WriteDoubleTupleAsScalarBytes<float>(input, componentCount, output);
            return true;
        case ScalarType::Float64:
            WriteDoubleTupleAsScalarBytes<double>(input, componentCount, output);
            return true;
        case ScalarType::Int8:
            WriteDoubleTupleAsScalarBytes<std::int8_t>(input, componentCount, output);
            return true;
        case ScalarType::UInt8:
            WriteDoubleTupleAsScalarBytes<std::uint8_t>(input, componentCount, output);
            return true;
        case ScalarType::Int16:
            WriteDoubleTupleAsScalarBytes<std::int16_t>(input, componentCount, output);
            return true;
        case ScalarType::UInt16:
            WriteDoubleTupleAsScalarBytes<std::uint16_t>(input, componentCount, output);
            return true;
        case ScalarType::Int32:
            WriteDoubleTupleAsScalarBytes<std::int32_t>(input, componentCount, output);
            return true;
        case ScalarType::UInt32:
            WriteDoubleTupleAsScalarBytes<std::uint32_t>(input, componentCount, output);
            return true;
        case ScalarType::Int64:
            WriteDoubleTupleAsScalarBytes<std::int64_t>(input, componentCount, output);
            return true;
        case ScalarType::UInt64:
            WriteDoubleTupleAsScalarBytes<std::uint64_t>(input, componentCount, output);
            return true;
    }
    return validation::AssignError(error, "numeric tuple scalar type is unsupported");
}

inline bool ReadNumericArrayTupleBytes(
    const NumericArrayView& view,
    const std::size_t tupleIndex,
    const std::size_t componentCount,
    void* output,
    std::string* error = nullptr) {
    if (componentCount == 0u) {
        return true;
    }
    if (output == nullptr) {
        return validation::AssignError(error, "numeric tuple output buffer is null");
    }
    if (tupleIndex >= view.tupleCount || componentCount > static_cast<std::size_t>(view.componentCount)) {
        return validation::AssignError(error, "numeric tuple read is out of range");
    }
    const auto valueSize = view.ComponentSize();
    if (valueSize == 0u) {
        return validation::AssignError(error, "numeric tuple scalar type is unsupported");
    }
    std::size_t tupleByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            valueSize,
            tupleByteCount,
            "numeric tuple byte count",
            error)) {
        return false;
    }

    if (view.layout == ArrayLayout::CompactAOS && view.data != nullptr) {
        const auto* source = static_cast<const std::uint8_t*>(view.data) +
            tupleIndex * view.CompactTupleSize();
        std::memcpy(output, source, tupleByteCount);
        return true;
    }

    if (view.layout == ArrayLayout::StridedAOS && view.data != nullptr) {
        if (view.tupleStrideBytes < tupleByteCount) {
            return validation::AssignError(error, "strided numeric tuple stride is too small");
        }
        const auto* source = static_cast<const std::uint8_t*>(view.data) +
            tupleIndex * view.tupleStrideBytes;
        std::memcpy(output, source, tupleByteCount);
        return true;
    }

    if (view.layout == ArrayLayout::SOA) {
        if (componentCount > view.ComponentSlotCount()) {
            return validation::AssignError(error, "soa numeric tuple component count exceeds view storage");
        }
        auto* target = static_cast<std::uint8_t*>(output);
        for (std::size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
            const auto* componentData = static_cast<const std::uint8_t*>(view.ComponentData(componentIndex));
            if (componentData == nullptr) {
                return validation::AssignError(error, "soa numeric tuple component data is null");
            }
            const auto componentStrideBytes = view.ComponentStrideBytes(componentIndex);
            const auto stride = componentStrideBytes == 0u
                ? valueSize
                : componentStrideBytes;
            std::memcpy(
                target + componentIndex * valueSize,
                componentData + tupleIndex * stride,
                valueSize);
        }
        return true;
    }

    if (view.layout == ArrayLayout::GetterOnly) {
        if (view.getTupleBytes != nullptr) {
            const auto sourceComponentCount = static_cast<std::size_t>(view.componentCount);
            if (componentCount == sourceComponentCount) {
                return view.getTupleBytes(view.userData, tupleIndex, output, error);
            }
            std::size_t sourceTupleByteCount = 0u;
            if (!validation::CheckedMulSizeT(
                    sourceComponentCount,
                    valueSize,
                    sourceTupleByteCount,
                    "numeric tuple source byte count",
                    error)) {
                return false;
            }
            std::vector<std::uint8_t> tupleBytes(sourceTupleByteCount, 0u);
            if (!view.getTupleBytes(view.userData, tupleIndex, tupleBytes.data(), error)) {
                return false;
            }
            std::memcpy(output, tupleBytes.data(), tupleByteCount);
            return true;
        }
    }

    return validation::AssignError(error, "unsupported numeric tuple layout");
}

struct IndexArrayView {
    using GetIndexFn = std::uint64_t (*)(const void* userData, std::size_t index);

    IndexValueType indexType{IndexValueType::UInt32};
    ArrayLayout layout{ArrayLayout::CompactAOS};
    ViewBufferOrigin origin{ViewBufferOrigin::Borrowed};
    const void* data{nullptr};
    std::size_t count{0};
    std::size_t strideBytes{0};
    const void* userData{nullptr};
    GetIndexFn getIndex{nullptr};

    [[nodiscard]] std::size_t ValueSize() const {
        return indexType == IndexValueType::UInt64 ? sizeof(std::uint64_t) : sizeof(std::uint32_t);
    }

    [[nodiscard]] bool IsCompact() const noexcept {
        return layout == ArrayLayout::CompactAOS && data != nullptr;
    }

    [[nodiscard]] bool IsValid() const noexcept {
        if (count == 0) {
            return true;
        }
        const auto valueSize = ValueSize();
        if (valueSize == 0u) {
            return false;
        }
        if (layout == ArrayLayout::GetterOnly) {
            return getIndex != nullptr;
        }
        if (layout == ArrayLayout::CompactAOS) {
            return data != nullptr;
        }
        if (layout == ArrayLayout::StridedAOS) {
            return data != nullptr && strideBytes >= valueSize;
        }
        return false;
    }
};

inline NumericArrayView MakeCompactNumericArrayView(
    const void* data,
    const ScalarType scalarType,
    const std::size_t tupleCount,
    const int componentCount,
    const ViewBufferOrigin origin = ViewBufferOrigin::Borrowed) {
    NumericArrayView view;
    view.scalarType = scalarType;
    view.layout = ArrayLayout::CompactAOS;
    view.origin = origin;
    view.data = data;
    view.tupleCount = tupleCount;
    view.componentCount = componentCount;
    view.tupleStrideBytes = view.CompactTupleSize();
    return view;
}

inline IndexArrayView MakeCompactIndexArrayView(
    const void* data,
    const IndexValueType indexType,
    const std::size_t count,
    const ViewBufferOrigin origin = ViewBufferOrigin::Borrowed) {
    IndexArrayView view;
    view.indexType = indexType;
    view.layout = ArrayLayout::CompactAOS;
    view.origin = origin;
    view.data = data;
    view.count = count;
    view.strideBytes = view.ValueSize();
    return view;
}

} // namespace datacodec

#endif
