#ifndef DATACODEC_CODEC_REFERENCE_ATTRIBUTEREFERENCESCHEDULEBUILDER_H
#define DATACODEC_CODEC_REFERENCE_ATTRIBUTEREFERENCESCHEDULEBUILDER_H

#include "DataCodec/Codec/Reference/AttributeReferenceSchedule.h"
#include "DataCodec/Codec/Reference/IntraFieldReference.h"
#include "DataCodec/Codec/NumericArray/IntegerResidualCodec.h"
#include "DataCodec/Codec/NumericArray/NumericArrayReader.h"
#include "DataCodec/Codec/Reference/TemporalReference.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
namespace datacodec {

inline bool IsReferenceEligibleAttributeField(const AttrStorageParams& meta) {
    const auto expectedValueSize = DataTypeSize(meta.dataType);
    const auto isFloat =
        (meta.dataType == DataType::Float32 || meta.dataType == DataType::Float64) &&
        (meta.valueSize == sizeof(float) || meta.valueSize == sizeof(double));
    const auto isInteger =
        numericarray::IsIntegerNumericArrayDataType(meta.dataType) &&
        expectedValueSize != 0u &&
        meta.valueSize == expectedValueSize;
    return (isFloat || isInteger) &&
        meta.dimension > 0 &&
        meta.elementCount > 0;
}

inline bool HasMatchingAttributeReferenceSampleLayout(
    const AttrStorageParams& lhs,
    const AttrStorageParams& rhs) noexcept {
    return lhs.dataType == rhs.dataType &&
        lhs.valueSize == rhs.valueSize &&
        lhs.elementCount == rhs.elementCount &&
        lhs.dimension == rhs.dimension;
}

inline bool IsAttributeReferenceSampleFieldEligible(
    const AttrStorageParams& meta,
    const IntraFieldReferenceCodec codec) noexcept {
    if (!IsReferenceEligibleAttributeField(meta)) {
        return false;
    }
    if (numericarray::IsIntegerNumericArrayDataType(meta.dataType)) {
        return codec == IntraFieldReferenceCodec::Wavelet;
    }
    return codec == IntraFieldReferenceCodec::Affine ||
        codec == IntraFieldReferenceCodec::Predictor ||
        codec == IntraFieldReferenceCodec::Wavelet;
}

inline bool BuildAttributeReferenceSampleIndices(
    const ParamSize elementCountParam,
    const std::size_t requestedSampleCount,
    std::vector<std::size_t>& indices,
    std::string* error = nullptr) {
    indices.clear();
    std::size_t elementCount = 0u;
    if (!TryParamSizeToSizeT(elementCountParam, elementCount)) {
        return validation::AssignError(
            error,
            "attribute reference sample element count exceeds this platform size limit");
    }
    if (elementCount == 0u) {
        return true;
    }
    const auto sampleCount = std::min(
        elementCount,
        std::max<std::size_t>(1u, requestedSampleCount));
    indices.reserve(sampleCount);
    if (sampleCount == elementCount) {
        for (std::size_t index = 0u; index < elementCount; ++index) {
            indices.push_back(index);
        }
        return true;
    }
    if (sampleCount == 1u) {
        indices.push_back(0u);
        return true;
    }
    const auto lastIndex = static_cast<long double>(elementCount - 1u);
    const auto denominator = static_cast<long double>(sampleCount - 1u);
    for (std::size_t index = 0u; index < sampleCount; ++index) {
        const auto resolved = static_cast<std::size_t>(std::llround(
            lastIndex * static_cast<long double>(index) / denominator));
        indices.push_back(std::min(resolved, elementCount - 1u));
    }
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return true;
}

struct AttributeReferenceFieldSample {
    std::vector<std::uint8_t> bytes;
    std::size_t tupleCount{0u};
    std::size_t componentCount{0u};
    std::size_t valueSize{0u};
};

struct AttributeReferenceSampleGroup {
    std::size_t representativeFieldIndex{0u};
    std::vector<std::size_t> fieldIndices;
    std::vector<std::size_t> sampleIndices;
};

inline bool BuildAttributeReferenceFieldSample(
    const AttrStorageParams& meta,
    const numericarray::NumericArraySource& source,
    const std::vector<std::size_t>& sampleIndices,
    ScratchByteBufferPool& scratchBytePool,
    AttributeReferenceFieldSample& sample,
    std::string* error = nullptr) {
    sample = {};
    std::size_t valueSize = 0u;
    if (!TryParamSizeToSizeT(meta.valueSize, valueSize)) {
        return validation::AssignError(
            error,
            "attribute reference sample value size exceeds this platform size limit");
    }
    const auto componentCount = static_cast<std::size_t>(std::max(meta.dimension, 0));
    std::size_t tupleBytes = 0u;
    std::size_t sampleByteCount = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            valueSize,
            tupleBytes,
            "attribute reference sample tuple bytes",
            error) ||
        !validation::CheckedMulSizeT(
            sampleIndices.size(),
            tupleBytes,
            sampleByteCount,
            "attribute reference sample bytes",
            error)) {
        return false;
    }
    numericarray::NumericArrayReader reader;
    if (!numericarray::BuildNumericArrayReader(source, reader, error)) {
        return false;
    }
    sample.bytes.reserve(sampleByteCount);
    std::size_t cursor = 0u;
    while (cursor < sampleIndices.size()) {
        const auto runStart = sampleIndices[cursor];
        std::size_t runCount = 1u;
        while (cursor + runCount < sampleIndices.size() &&
               sampleIndices[cursor + runCount] == runStart + runCount) {
            ++runCount;
        }
        ScratchByteBuffer rangeBytes;
        if (!reader.ReadElements(
                runStart,
                runCount,
                scratchBytePool,
                rangeBytes,
                error)) {
            return false;
        }
        sample.bytes.insert(
            sample.bytes.end(),
            rangeBytes.Bytes().begin(),
            rangeBytes.Bytes().end());
        cursor += runCount;
    }
    if (sample.bytes.size() != sampleByteCount) {
        return validation::AssignError(error, "attribute reference sample byte size does not match");
    }
    sample.tupleCount = sampleIndices.size();
    sample.componentCount = componentCount;
    sample.valueSize = valueSize;
    return true;
}

template<typename TValue>
inline TValue ReadAttributeReferenceSampleValue(
    const AttributeReferenceFieldSample& sample,
    const std::size_t tupleIndex,
    const std::size_t componentIndex) noexcept {
    TValue value{};
    const auto valueIndex = tupleIndex * sample.componentCount + componentIndex;
    std::memcpy(
        &value,
        sample.bytes.data() + valueIndex * sizeof(TValue),
        sizeof(TValue));
    return value;
}

template<typename TValue>
inline double ComputeAffineAttributeReferenceSampleScore(
    const AttributeReferenceFieldSample& current,
    const AttributeReferenceFieldSample& reference) noexcept {
    constexpr long double kEpsilon = 1.0e-18L;
    long double totalScore = 0.0L;
    std::size_t validComponentCount = 0u;
    for (std::size_t componentIndex = 0u;
         componentIndex < current.componentCount;
         ++componentIndex) {
        long double sumX = 0.0L;
        long double sumY = 0.0L;
        long double sumXX = 0.0L;
        long double sumYY = 0.0L;
        long double sumXY = 0.0L;
        std::size_t count = 0u;
        for (std::size_t tupleIndex = 0u; tupleIndex < current.tupleCount; ++tupleIndex) {
            const auto x = static_cast<long double>(
                ReadAttributeReferenceSampleValue<TValue>(reference, tupleIndex, componentIndex));
            const auto y = static_cast<long double>(
                ReadAttributeReferenceSampleValue<TValue>(current, tupleIndex, componentIndex));
            if (!std::isfinite(static_cast<double>(x)) ||
                !std::isfinite(static_cast<double>(y))) {
                continue;
            }
            sumX += x;
            sumY += y;
            sumXX += x * x;
            sumYY += y * y;
            sumXY += x * y;
            ++count;
        }
        if (count == 0u) {
            continue;
        }
        const auto localCount = static_cast<long double>(count);
        const auto sxx = sumXX - sumX * sumX / localCount;
        const auto syy = sumYY - sumY * sumY / localCount;
        const auto sxy = sumXY - sumX * sumY / localCount;
        if (syy <= kEpsilon) {
            totalScore += 1.0L;
            ++validComponentCount;
            continue;
        }
        if (sxx <= kEpsilon) {
            continue;
        }
        totalScore += std::clamp((sxy * sxy) / (sxx * syy), 0.0L, 1.0L);
        ++validComponentCount;
    }
    return validComponentCount == 0u
        ? 0.0
        : static_cast<double>(totalScore / static_cast<long double>(validComponentCount));
}

template<typename TValue>
inline double ComputeDeltaAttributeReferenceSampleScore(
    const AttributeReferenceFieldSample& current,
    const AttributeReferenceFieldSample& reference) noexcept {
    constexpr long double kEpsilon = 1.0e-18L;
    long double totalScore = 0.0L;
    std::size_t validComponentCount = 0u;
    for (std::size_t componentIndex = 0u;
         componentIndex < current.componentCount;
         ++componentIndex) {
        long double sumCurrent = 0.0L;
        long double sumResidual = 0.0L;
        long double sumCurrentSquared = 0.0L;
        long double sumResidualSquared = 0.0L;
        std::size_t count = 0u;
        for (std::size_t tupleIndex = 0u; tupleIndex < current.tupleCount; ++tupleIndex) {
            const auto currentValue = static_cast<long double>(
                ReadAttributeReferenceSampleValue<TValue>(current, tupleIndex, componentIndex));
            const auto referenceValue = static_cast<long double>(
                ReadAttributeReferenceSampleValue<TValue>(reference, tupleIndex, componentIndex));
            if (!std::isfinite(static_cast<double>(currentValue)) ||
                !std::isfinite(static_cast<double>(referenceValue))) {
                continue;
            }
            const auto residual = currentValue - referenceValue;
            sumCurrent += currentValue;
            sumResidual += residual;
            sumCurrentSquared += currentValue * currentValue;
            sumResidualSquared += residual * residual;
            ++count;
        }
        if (count == 0u) {
            continue;
        }
        const auto localCount = static_cast<long double>(count);
        const auto currentVariance =
            sumCurrentSquared - sumCurrent * sumCurrent / localCount;
        const auto residualVariance =
            sumResidualSquared - sumResidual * sumResidual / localCount;
        long double componentScore = 0.0L;
        if (residualVariance <= kEpsilon) {
            componentScore = 1.0L;
        } else if (currentVariance > kEpsilon) {
            componentScore = std::clamp(
                1.0L - residualVariance / currentVariance,
                0.0L,
                1.0L);
        }
        totalScore += componentScore;
        ++validComponentCount;
    }
    return validComponentCount == 0u
        ? 0.0
        : static_cast<double>(totalScore / static_cast<long double>(validComponentCount));
}

inline std::size_t AttributeReferenceResidualBitWidth(std::uint64_t value) noexcept {
    std::size_t bitWidth = 0u;
    while (value != 0u) {
        ++bitWidth;
        value >>= 1u;
    }
    return bitWidth;
}

inline double ComputeIntegerAttributeReferenceSampleScore(
    const AttrStorageParams& meta,
    const AttributeReferenceFieldSample& current,
    const AttributeReferenceFieldSample& reference) noexcept {
    const auto storageBitWidth = current.valueSize * 8u;
    const auto valueCount = current.tupleCount * current.componentCount;
    if (storageBitWidth == 0u || valueCount == 0u) {
        return 0.0;
    }
    const auto mask = numericarray::IntegerStorageMask(current.valueSize);
    long double totalResidualBitWidth = 0.0L;
    for (std::size_t valueIndex = 0u; valueIndex < valueCount; ++valueIndex) {
        const auto byteOffset = valueIndex * current.valueSize;
        const auto currentRaw = numericarray::ReadIntegerStorageValue(
            current.bytes.data() + byteOffset,
            current.valueSize);
        const auto referenceRaw = numericarray::ReadIntegerStorageValue(
            reference.bytes.data() + byteOffset,
            reference.valueSize);
        const auto currentCode = numericarray::ToIntegerOrderCode(
            meta.dataType,
            currentRaw,
            current.valueSize);
        const auto referenceCode = numericarray::ToIntegerOrderCode(
            meta.dataType,
            referenceRaw,
            reference.valueSize);
        const auto delta = (currentCode - referenceCode) & mask;
        const auto zigZag = numericarray::SignedModuloToZigZag(delta, current.valueSize);
        totalResidualBitWidth += static_cast<long double>(
            AttributeReferenceResidualBitWidth(zigZag));
    }
    const auto maximumBitWidth =
        static_cast<long double>(valueCount) * static_cast<long double>(storageBitWidth);
    return static_cast<double>(std::clamp(
        1.0L - totalResidualBitWidth / maximumBitWidth,
        0.0L,
        1.0L));
}

inline bool ComputeAttributeReferenceSampleScore(
    const AttrStorageParams& meta,
    const IntraFieldReferenceCodec codec,
    const AttributeReferenceFieldSample& current,
    const AttributeReferenceFieldSample& reference,
    double& score,
    std::string* error = nullptr) {
    score = 0.0;
    if (current.tupleCount != reference.tupleCount ||
        current.componentCount != reference.componentCount ||
        current.valueSize != reference.valueSize ||
        current.bytes.size() != reference.bytes.size()) {
        return validation::AssignError(error, "attribute reference samples do not share a common layout");
    }
    if (numericarray::IsIntegerNumericArrayDataType(meta.dataType)) {
        if (codec != IntraFieldReferenceCodec::Wavelet) {
            return validation::AssignError(error, "integer attribute reference sampling requires wavelet codec");
        }
        score = ComputeIntegerAttributeReferenceSampleScore(meta, current, reference);
        return true;
    }
    if (meta.dataType == DataType::Float32 && meta.valueSize == sizeof(float)) {
        score = codec == IntraFieldReferenceCodec::Affine
            ? ComputeAffineAttributeReferenceSampleScore<float>(current, reference)
            : ComputeDeltaAttributeReferenceSampleScore<float>(current, reference);
        return true;
    }
    if (meta.dataType == DataType::Float64 && meta.valueSize == sizeof(double)) {
        score = codec == IntraFieldReferenceCodec::Affine
            ? ComputeAffineAttributeReferenceSampleScore<double>(current, reference)
            : ComputeDeltaAttributeReferenceSampleScore<double>(current, reference);
        return true;
    }
    return validation::AssignError(error, "attribute reference sampling uses an unsupported numeric type");
}

inline double ResolveAttributeReferenceMinimumSampleScore(
    const IntraFieldReferenceControlParams& control) noexcept {
    return control.codec == IntraFieldReferenceCodec::Affine
        ? control.affine.precheckRSquared
        : control.minimumSampleScore;
}

inline bool BuildAttributeIntraFieldReferenceSchedule(
    const std::vector<AttrStorageParams>& metas,
    const std::vector<numericarray::NumericArraySource>& sources,
    const std::vector<std::size_t>& metaIndices,
    const std::vector<std::uint8_t>& referenceAllowed,
    const AttrReferenceControlParams& dependency,
    ScratchByteBufferPool& scratchBytePool,
    EncodeAttributeReferenceSchedule& schedule,
    std::string* error = nullptr) {
    const auto attrCount = metas.size();
    if (sources.size() != attrCount ||
        metaIndices.size() != attrCount ||
        referenceAllowed.size() != attrCount) {
        return validation::AssignError(error, "attribute reference schedule input sizes do not match");
    }
    schedule.initialized = false;
    schedule.topologyOrder.resize(attrCount);
    schedule.entries.clear();
    schedule.entries.resize(attrCount);
    for (std::size_t index = 0; index < attrCount; ++index) {
        schedule.topologyOrder[index] = index;
    }
    if (attrCount == 0u || !IsIntraFieldReferenceEnabled(dependency)) {
        schedule.initialized = true;
        return true;
    }

    const auto minimumSampleScore = ResolveAttributeReferenceMinimumSampleScore(
        dependency.intraField);
    if (!std::isfinite(minimumSampleScore) ||
        minimumSampleScore < 0.0 ||
        minimumSampleScore > 1.0) {
        return validation::AssignError(
            error,
            "attribute reference minimum sample score must be within [0, 1]");
    }

    std::vector<AttributeReferenceSampleGroup> sampleGroups;
    for (std::size_t fieldIndex = 0u; fieldIndex < attrCount; ++fieldIndex) {
        if (!IsAttributeReferenceSampleFieldEligible(
                metas[fieldIndex],
                dependency.intraField.codec)) {
            continue;
        }
        auto groupIt = std::find_if(
            sampleGroups.begin(),
            sampleGroups.end(),
            [&](const AttributeReferenceSampleGroup& group) {
                return HasMatchingAttributeReferenceSampleLayout(
                    metas[group.representativeFieldIndex],
                    metas[fieldIndex]);
            });
        if (groupIt == sampleGroups.end()) {
            AttributeReferenceSampleGroup group;
            group.representativeFieldIndex = fieldIndex;
            group.fieldIndices.push_back(fieldIndex);
            if (!BuildAttributeReferenceSampleIndices(
                    metas[fieldIndex].elementCount,
                    dependency.intraField.sampleCount,
                    group.sampleIndices,
                    error)) {
                return false;
            }
            sampleGroups.push_back(std::move(group));
            continue;
        }
        groupIt->fieldIndices.push_back(fieldIndex);
    }

    std::vector<AttributeReferenceFieldSample> fieldSamples(attrCount);
    for (const auto& group : sampleGroups) {
        if (group.fieldIndices.size() < 2u) {
            continue;
        }
        for (const auto fieldIndex : group.fieldIndices) {
            if (!BuildAttributeReferenceFieldSample(
                    metas[fieldIndex],
                    sources[fieldIndex],
                    group.sampleIndices,
                    scratchBytePool,
                    fieldSamples[fieldIndex],
                    error)) {
                return false;
            }
        }
    }

    std::vector<IntraFieldEdge> edges;
    for (const auto& group : sampleGroups) {
        if (group.fieldIndices.size() < 2u) {
            continue;
        }
        for (const auto child : group.fieldIndices) {
            if (referenceAllowed[child] == 0u) {
                continue;
            }
            for (const auto parent : group.fieldIndices) {
                if (parent == child) {
                    continue;
                }
                double sampleScore = 0.0;
                std::string localError;
                if (!ComputeAttributeReferenceSampleScore(
                        metas[child],
                        dependency.intraField.codec,
                        fieldSamples[child],
                        fieldSamples[parent],
                        sampleScore,
                        &localError)) {
                    return validation::AssignError(
                        error,
                        "failed to score intra-field reference samples: " + localError);
                }
                if (dependency.intraField.selectionMode != ReferenceSelectionMode::Forced &&
                    sampleScore < minimumSampleScore) {
                    continue;
                }
                edges.push_back(IntraFieldEdge{
                    .parent = parent,
                    .child = child,
                    .score = sampleScore,
                });
            }
        }
    }
    std::sort(edges.begin(), edges.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.score != rhs.score) {
            return lhs.score > rhs.score;
        }
        if (lhs.child != rhs.child) {
            return lhs.child < rhs.child;
        }
        return lhs.parent < rhs.parent;
    });

    const auto noParent = static_cast<std::size_t>(-1);
    std::vector<std::size_t> parentOf(attrCount, noParent);
    for (const auto& edge : edges) {
        if (parentOf[edge.child] != noParent ||
            WouldCreateIntraFieldCycle(edge.parent, edge.child, parentOf, noParent)) {
            continue;
        }
        parentOf[edge.child] = edge.parent;
    }

    for (std::size_t child = 0; child < attrCount; ++child) {
        const auto parent = parentOf[child];
        if (parent == noParent) {
            continue;
        }
        if (metaIndices[parent] > std::numeric_limits<std::uint16_t>::max()) {
            return validation::AssignError(error, "attribute reference parent meta index exceeds uint16 range");
        }
        auto& entry = schedule.entries[child];
        entry.hasIntraParent = true;
        entry.parentMetaIndex = static_cast<std::uint16_t>(metaIndices[parent]);
        entry.parentMeta = metas[parent];
        entry.parentSource = sources[parent];
    }
    if (!BuildIntraFieldRecordOrder(parentOf, noParent, schedule.topologyOrder, error)) {
        return false;
    }
    schedule.initialized = true;
    return true;
}

} // namespace datacodec

#endif
