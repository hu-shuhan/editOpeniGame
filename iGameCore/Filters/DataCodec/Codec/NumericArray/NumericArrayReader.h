#ifndef DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYREADER_H
#define DATACODEC_CODEC_NUMERICARRAY_NUMERICARRAYREADER_H

#include "DataCodec/Storage/ByteIO/ScratchByteBuffer.h"
#include "DataCodec/Codec/NumericArray/NumericArraySource.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>
namespace datacodec {
namespace numericarray {

struct NumericArrayReader {
    NumericArraySource source;

    [[nodiscard]] std::size_t ElementBytes() const noexcept { return source.ElementBytes(); }

    [[nodiscard]] bool ReadElements(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        std::vector<std::uint8_t>& output,
        std::string* error) const {
        output.clear();
        std::size_t outputBytes = 0u;
        if (!PrepareElementRead(elementOffset, elementCount, outputBytes, error)) {
            return false;
        }
        output.resize(outputBytes);
        if (outputBytes == 0u) {
            return true;
        }
        return ReadElementsInto(elementOffset, elementCount, std::span<std::uint8_t>(output.data(), output.size()), error);
    }

    [[nodiscard]] bool ReadElements(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        ScratchByteBufferPool& scratchBytePool,
        ScratchByteBuffer& output,
        std::string* error) const {
        return ReadElements(elementOffset, elementCount, scratchBytePool, {}, output, error);
    }

    [[nodiscard]] bool ReadElements(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        ScratchByteBufferPool& scratchBytePool,
        const ScratchByteQuotaAcquire& acquireScratchQuota,
        ScratchByteBuffer& output,
        std::string* error) const {
        output.Release();
        std::size_t outputBytes = 0u;
        if (!PrepareElementRead(elementOffset, elementCount, outputBytes, error)) {
            return false;
        }
        const auto outputByteCount = static_cast<std::uint64_t>(outputBytes);
        output = scratchBytePool.Acquire(
            outputBytes,
            acquireScratchQuota ? acquireScratchQuota(outputByteCount) : ScratchByteQuotaLease{});
        if (outputBytes == 0u) {
            return true;
        }
        return ReadElementsInto(elementOffset, elementCount, output.Span(), error);
    }

    [[nodiscard]] bool ReadComponentElements(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        const std::size_t componentIndex,
        ScratchByteBufferPool& scratchBytePool,
        ScratchByteBuffer& output,
        std::string* error) const {
        output.Release();
        std::size_t outputBytes = 0u;
        if (!PrepareComponentRead(elementOffset, elementCount, componentIndex, outputBytes, error)) {
            return false;
        }
        output = scratchBytePool.Acquire(outputBytes);
        if (outputBytes == 0u) {
            return true;
        }
        return ReadComponentElementsInto(
            elementOffset,
            elementCount,
            componentIndex,
            output.Span(),
            error);
    }

    [[nodiscard]] bool TryResolveContiguousComponentBytes(
        const std::size_t componentIndex,
        std::span<const std::uint8_t>& bytes) const noexcept {
        bytes = {};
        const bool hasOrderProvider = source.orderProvider != nullptr && !source.orderProvider->IsIdentity();
        const bool hasVectorOrder = source.order != nullptr && !source.order->empty();
        if (hasOrderProvider || hasVectorOrder ||
            source.layout.valueSize == 0u ||
            componentIndex >= source.layout.componentCount ||
            source.layout.elementCount > source.values.tupleCount ||
            !validation::CanMulSizeT(source.layout.elementCount, source.layout.valueSize)) {
            return false;
        }
        const auto byteCount = static_cast<std::size_t>(source.layout.elementCount) * source.layout.valueSize;
        if (source.layout.elementCount == 0u) {
            return true;
        }
        if (source.values.layout == ArrayLayout::SOA) {
            if (source.layout.componentCount > source.values.ComponentSlotCount()) {
                return false;
            }
            const auto* componentData = static_cast<const std::uint8_t*>(source.values.ComponentData(componentIndex));
            if (componentData == nullptr) {
                return false;
            }
            const auto componentStrideBytes = source.values.ComponentStrideBytes(componentIndex);
            const auto stride = componentStrideBytes == 0u
                ? source.layout.valueSize
                : componentStrideBytes;
            if (stride != source.layout.valueSize) {
                return false;
            }
            bytes = std::span<const std::uint8_t>(componentData, byteCount);
            return true;
        }
        if (source.values.layout == ArrayLayout::CompactAOS &&
            source.values.data != nullptr &&
            source.layout.componentCount == 1u) {
            bytes = std::span<const std::uint8_t>(
                static_cast<const std::uint8_t*>(source.values.data),
                byteCount);
            return true;
        }
        if (source.values.layout == ArrayLayout::StridedAOS &&
            source.values.data != nullptr &&
            source.layout.componentCount == 1u &&
            source.values.tupleStrideBytes == source.layout.valueSize) {
            bytes = std::span<const std::uint8_t>(
                static_cast<const std::uint8_t*>(source.values.data),
                byteCount);
            return true;
        }
        return false;
    }

private:
    [[nodiscard]] bool PrepareElementRead(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        std::size_t& outputBytes,
        std::string* error) const {
        outputBytes = 0u;
        if (elementOffset > source.layout.elementCount || elementCount > source.layout.elementCount - elementOffset) {
            return validation::AssignError(error, "numeric array block range is out of bounds");
        }
        if (source.layout.componentCount == 0u || source.layout.valueSize == 0u) {
            if (elementCount == 0u) {
                return true;
            }
            return validation::AssignError(error, "numeric array block reader received an empty element layout");
        }

        const auto tupleBytes = ElementBytes();
        std::size_t localElementCount = 0u;
        if (!validation::CheckedCastSizeT(
                elementCount,
                localElementCount,
                "numeric array block reader element count",
                error) ||
            !validation::CheckedMulSizeT(
                localElementCount,
                tupleBytes,
                outputBytes,
                "numeric array block reader output",
                error)) {
            return false;
        }
        return true;
    }

    [[nodiscard]] bool PrepareComponentRead(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        const std::size_t componentIndex,
        std::size_t& outputBytes,
        std::string* error) const {
        outputBytes = 0u;
        if (elementOffset > source.layout.elementCount || elementCount > source.layout.elementCount - elementOffset) {
            return validation::AssignError(error, "numeric array component range is out of bounds");
        }
        if (source.layout.componentCount == 0u || source.layout.valueSize == 0u) {
            if (elementCount == 0u) {
                return true;
            }
            return validation::AssignError(error, "numeric array component reader received an empty element layout");
        }
        if (componentIndex >= source.layout.componentCount) {
            return validation::AssignError(error, "numeric array component index is out of bounds");
        }
        std::size_t localElementCount = 0u;
        if (!validation::CheckedCastSizeT(
                elementCount,
                localElementCount,
                "numeric array component reader element count",
                error) ||
            !validation::CheckedMulSizeT(
                localElementCount,
                source.layout.valueSize,
                outputBytes,
                "numeric array component reader output",
                error)) {
            return false;
        }
        return true;
    }

    [[nodiscard]] bool ReadElementsInto(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        const std::span<std::uint8_t> output,
        std::string* error) const {
        const auto tupleBytes = ElementBytes();
        std::size_t localElementOffset = 0u;
        std::size_t localElementCount = 0u;
        std::size_t expectedBytes = 0u;
        if (!validation::CheckedCastSizeT(
                elementOffset,
                localElementOffset,
                "numeric array block reader element offset",
                error) ||
            !validation::CheckedCastSizeT(
                elementCount,
                localElementCount,
                "numeric array block reader element count",
                error) ||
            !validation::CheckedMulSizeT(
                localElementCount,
                tupleBytes,
                expectedBytes,
                "numeric array block reader expected bytes",
                error)) {
            return false;
        }
        if (output.size() != expectedBytes) {
            return validation::AssignError(error, "numeric array block reader output size is invalid");
        }
        const auto begin = localElementOffset;
        const auto count = localElementCount;
        const bool hasOrderProvider = source.orderProvider != nullptr && !source.orderProvider->IsIdentity();
        const bool hasVectorOrder = source.order != nullptr && !source.order->empty();
        std::vector<IndexType> orderBlock;
        if (hasOrderProvider) {
            if (!source.orderProvider->ReadRange(elementOffset, elementCount, orderBlock, error)) {
                return false;
            }
        }
        const auto resolveSourceIndex = [&](const std::size_t newIndex, const std::size_t localIndex) -> std::size_t {
            if (!orderBlock.empty()) {
                return static_cast<std::size_t>(orderBlock[localIndex]);
            }
            if (source.order == nullptr || source.order->empty()) {
                return newIndex;
            }
            return static_cast<std::size_t>((*source.order)[newIndex]);
        };
        const auto validateSourceIndex = [&](const std::size_t sourceIndex) -> bool {
            return sourceIndex < source.values.tupleCount;
        };

        if (source.values.layout == ArrayLayout::CompactAOS && source.values.data != nullptr) {
            const auto* sourceBytes = static_cast<const std::uint8_t*>(source.values.data);
            if (!hasOrderProvider && !hasVectorOrder) {
                const auto sourceOffset = begin * tupleBytes;
                if (begin + count > source.values.tupleCount) {
                    return validation::AssignError(error, "compact numeric array block range exceeds source element count");
                }
                std::memcpy(output.data(), sourceBytes + sourceOffset, output.size());
                return true;
            }
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto newIndex = begin + localIndex;
                const auto sourceIndex = resolveSourceIndex(newIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(error, "compact numeric array remap index exceeds source element count");
                }
                std::memcpy(output.data() + localIndex * tupleBytes, sourceBytes + sourceIndex * tupleBytes, tupleBytes);
            }
            return true;
        }

        if (source.values.layout == ArrayLayout::StridedAOS &&
            source.values.data != nullptr &&
            source.values.tupleStrideBytes >= tupleBytes) {
            const auto* sourceBytes = static_cast<const std::uint8_t*>(source.values.data);
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto sourceIndex = resolveSourceIndex(begin + localIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(error, "strided numeric array remap index exceeds source element count");
                }
                std::memcpy(
                    output.data() + localIndex * tupleBytes,
                    sourceBytes + sourceIndex * source.values.tupleStrideBytes,
                    tupleBytes);
            }
            return true;
        }

        if (source.values.layout == ArrayLayout::SOA) {
            if (source.layout.componentCount > source.values.ComponentSlotCount()) {
                return validation::AssignError(error, "soa numeric array component count exceeds view storage");
            }
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto sourceIndex = resolveSourceIndex(begin + localIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(error, "soa numeric array remap index exceeds source element count");
                }
                auto* target = output.data() + localIndex * tupleBytes;
                for (std::size_t componentIndex = 0; componentIndex < source.layout.componentCount; ++componentIndex) {
                    const auto* componentData = static_cast<const std::uint8_t*>(source.values.ComponentData(componentIndex));
                    if (componentData == nullptr) {
                        return validation::AssignError(error, "soa numeric array component data is null");
                    }
                    const auto componentStrideBytes = source.values.ComponentStrideBytes(componentIndex);
                    const auto stride = componentStrideBytes == 0u
                        ? source.layout.valueSize
                        : componentStrideBytes;
                    std::memcpy(
                        target + componentIndex * source.layout.valueSize,
                        componentData + sourceIndex * stride,
                        source.layout.valueSize);
                }
            }
            return true;
        }

        if (source.values.layout == ArrayLayout::GetterOnly &&
            source.values.getTupleRangeBytes != nullptr &&
            !hasOrderProvider &&
            !hasVectorOrder) {
            return source.values.getTupleRangeBytes(
                source.values.userData,
                begin,
                count,
                output.data(),
                output.size(),
                error);
        }

        if (source.values.layout == ArrayLayout::GetterOnly && source.values.getTupleBytes != nullptr) {
            std::vector<std::uint8_t> elementValues(source.ElementBytes(), 0u);
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto sourceIndex = resolveSourceIndex(begin + localIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(error, "getter numeric array remap index exceeds source element count");
                }
                if (!ReadNumericArrayTupleBytes(
                        source.values,
                        sourceIndex,
                        source.layout.componentCount,
                        elementValues.data(),
                        error)) {
                    return false;
                }
                std::memcpy(output.data() + localIndex * tupleBytes, elementValues.data(), tupleBytes);
            }
            return true;
        }

        return validation::AssignError(error, "unsupported numeric array block reader layout");
    }

    [[nodiscard]] bool ReadComponentElementsInto(
        const std::uint64_t elementOffset,
        const std::uint64_t elementCount,
        const std::size_t componentIndex,
        const std::span<std::uint8_t> output,
        std::string* error) const {
        std::size_t localElementOffset = 0u;
        std::size_t localElementCount = 0u;
        std::size_t expectedBytes = 0u;
        if (!validation::CheckedCastSizeT(
                elementOffset,
                localElementOffset,
                "numeric array component reader element offset",
                error) ||
            !validation::CheckedCastSizeT(
                elementCount,
                localElementCount,
                "numeric array component reader element count",
                error) ||
            !validation::CheckedMulSizeT(
                localElementCount,
                source.layout.valueSize,
                expectedBytes,
                "numeric array component reader expected bytes",
                error)) {
            return false;
        }
        if (output.size() != expectedBytes) {
            return validation::AssignError(error, "numeric array component reader output size is invalid");
        }
        const auto begin = localElementOffset;
        const auto count = localElementCount;
        const auto tupleBytes = ElementBytes();
        const auto componentByteOffset = componentIndex * source.layout.valueSize;
        const bool hasOrderProvider = source.orderProvider != nullptr && !source.orderProvider->IsIdentity();
        const bool hasVectorOrder = source.order != nullptr && !source.order->empty();
        std::vector<IndexType> orderBlock;
        if (hasOrderProvider) {
            if (!source.orderProvider->ReadRange(elementOffset, elementCount, orderBlock, error)) {
                return false;
            }
        }
        const auto resolveSourceIndex = [&](const std::size_t newIndex, const std::size_t localIndex) -> std::size_t {
            if (!orderBlock.empty()) {
                return static_cast<std::size_t>(orderBlock[localIndex]);
            }
            if (source.order == nullptr || source.order->empty()) {
                return newIndex;
            }
            return static_cast<std::size_t>((*source.order)[newIndex]);
        };
        const auto validateSourceIndex = [&](const std::size_t sourceIndex) -> bool {
            return sourceIndex < source.values.tupleCount;
        };

        if (source.values.layout == ArrayLayout::CompactAOS && source.values.data != nullptr) {
            const auto* sourceBytes = static_cast<const std::uint8_t*>(source.values.data);
            if (!hasOrderProvider && !hasVectorOrder && source.layout.componentCount == 1u) {
                const auto sourceOffset = begin * tupleBytes;
                if (begin + count > source.values.tupleCount) {
                    return validation::AssignError(error, "compact numeric array component range exceeds source element count");
                }
                std::memcpy(output.data(), sourceBytes + sourceOffset, output.size());
                return true;
            }
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto newIndex = begin + localIndex;
                const auto sourceIndex = resolveSourceIndex(newIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(
                        error,
                        "compact numeric array component remap index exceeds source element count");
                }
                std::memcpy(
                    output.data() + localIndex * source.layout.valueSize,
                    sourceBytes + sourceIndex * tupleBytes + componentByteOffset,
                    source.layout.valueSize);
            }
            return true;
        }

        if (source.values.layout == ArrayLayout::StridedAOS &&
            source.values.data != nullptr &&
            source.values.tupleStrideBytes >= tupleBytes) {
            const auto* sourceBytes = static_cast<const std::uint8_t*>(source.values.data);
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto sourceIndex = resolveSourceIndex(begin + localIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(
                        error,
                        "strided numeric array component remap index exceeds source element count");
                }
                std::memcpy(
                    output.data() + localIndex * source.layout.valueSize,
                    sourceBytes + sourceIndex * source.values.tupleStrideBytes + componentByteOffset,
                    source.layout.valueSize);
            }
            return true;
        }

        if (source.values.layout == ArrayLayout::SOA) {
            if (source.layout.componentCount > source.values.ComponentSlotCount()) {
                return validation::AssignError(error, "soa numeric array component count exceeds view storage");
            }
            const auto* componentData = static_cast<const std::uint8_t*>(source.values.ComponentData(componentIndex));
            if (componentData == nullptr) {
                return validation::AssignError(error, "soa numeric array component data is null");
            }
            const auto componentStrideBytes = source.values.ComponentStrideBytes(componentIndex);
            const auto stride = componentStrideBytes == 0u
                ? source.layout.valueSize
                : componentStrideBytes;
            if (!hasOrderProvider && !hasVectorOrder && stride == source.layout.valueSize) {
                std::memcpy(output.data(), componentData + begin * stride, output.size());
                return true;
            }
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto sourceIndex = resolveSourceIndex(begin + localIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(
                        error,
                        "soa numeric array component remap index exceeds source element count");
                }
                std::memcpy(
                    output.data() + localIndex * source.layout.valueSize,
                    componentData + sourceIndex * stride,
                    source.layout.valueSize);
            }
            return true;
        }

        if (source.values.layout == ArrayLayout::GetterOnly && source.values.getTupleBytes != nullptr) {
            std::vector<std::uint8_t> elementValues(source.ElementBytes(), 0u);
            for (std::size_t localIndex = 0; localIndex < count; ++localIndex) {
                const auto sourceIndex = resolveSourceIndex(begin + localIndex, localIndex);
                if (!validateSourceIndex(sourceIndex)) {
                    return validation::AssignError(
                        error,
                        "getter numeric array component remap index exceeds source element count");
                }
                if (!ReadNumericArrayTupleBytes(
                        source.values,
                        sourceIndex,
                        source.layout.componentCount,
                        elementValues.data(),
                        error)) {
                    return false;
                }
                std::memcpy(
                    output.data() + localIndex * source.layout.valueSize,
                    elementValues.data() + componentByteOffset,
                    source.layout.valueSize);
            }
            return true;
        }

        return validation::AssignError(error, "unsupported numeric array component reader layout");
    }
};

inline bool BuildNumericArrayReader(
    const NumericArraySource& source,
    NumericArrayReader& reader,
    std::string* error) {
    if (!source.values.IsValid()) {
        return validation::AssignError(error, "numeric array source view is invalid");
    }
    reader = NumericArrayReader{.source = source};
    return true;
}

} // namespace numericarray
} // namespace datacodec

#endif
