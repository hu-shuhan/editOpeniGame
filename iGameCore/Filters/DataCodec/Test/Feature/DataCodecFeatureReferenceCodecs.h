#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREREFERENCECODECS_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREREFERENCECODECS_H

#include "DataCodec/API/Params/CodecParamDefaults.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"
#include "DataCodec/Runtime/Cache/TransferCache/ReferenceTransferCacheBuilder.h"
#include "DataCodec/Test/Common/ReferenceCodecTestHarness.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <string>
#include <vector>

namespace datacodec::test {

template<typename TValue>
inline std::vector<TValue> MakeReferencePrecisionValues(
    const std::size_t tupleCount,
    const std::size_t componentCount,
    const bool currentValues) {
    std::vector<TValue> values(tupleCount * componentCount, TValue{});
    for (std::size_t tupleIndex = 0u; tupleIndex < tupleCount; ++tupleIndex) {
        const auto position = static_cast<double>(tupleIndex);
        for (std::size_t componentIndex = 0u;
             componentIndex < componentCount;
             ++componentIndex) {
            const auto scale = componentIndex == 0u
                ? 120.0
                : (componentIndex == 1u ? 1.5 : 0.015);
            const auto reference = scale * (
                std::sin(position * (0.009 + componentIndex * 0.002)) +
                0.25 * std::cos(position * (0.003 + componentIndex * 0.001)));
            const auto value = currentValues
                ? (1.15 + componentIndex * 0.03) * reference +
                    scale * 0.07 +
                    scale * 0.004 * std::sin(position * 0.017)
                : reference;
            values[tupleIndex * componentCount + componentIndex] =
                static_cast<TValue>(value);
        }
    }
    return values;
}

template<typename TValue>
inline bool CheckReferencePrecision(
    TestResult& result,
    const std::vector<TValue>& expected,
    const std::vector<std::uint8_t>& decodedBytes,
    const std::size_t componentCount,
    const CompressorConfig& compressor,
    const std::string& caseName) {
    if (!Require(
            result,
            decodedBytes.size() == expected.size() * sizeof(TValue),
            caseName + ".decodeSize",
            "reference decoded byte size is invalid")) {
        return false;
    }
    const auto* decoded = reinterpret_cast<const TValue*>(decodedBytes.data());
    std::vector<double> componentMinimum(
        componentCount,
        std::numeric_limits<double>::infinity());
    std::vector<double> componentMaximum(
        componentCount,
        -std::numeric_limits<double>::infinity());
    for (std::size_t valueIndex = 0u; valueIndex < expected.size(); ++valueIndex) {
        const auto componentIndex = valueIndex % componentCount;
        const auto value = static_cast<double>(expected[valueIndex]);
        componentMinimum[componentIndex] = std::min(componentMinimum[componentIndex], value);
        componentMaximum[componentIndex] = std::max(componentMaximum[componentIndex], value);
    }
    const auto absoluteIterator = compressor.options.find("pressio:abs");
    const auto relativeIterator = compressor.options.find("pressio:rel");
    std::vector<double> maximumErrors(componentCount, 0.0);
    bool precise = true;
    for (std::size_t valueIndex = 0u; valueIndex < expected.size(); ++valueIndex) {
        const auto componentIndex = valueIndex % componentCount;
        const auto absoluteError = std::abs(
            static_cast<double>(decoded[valueIndex]) -
            static_cast<double>(expected[valueIndex]));
        maximumErrors[componentIndex] = std::max(
            maximumErrors[componentIndex],
            absoluteError);
        double errorBound = std::numeric_limits<double>::infinity();
        if (absoluteIterator != compressor.options.end()) {
            errorBound = absoluteIterator->second;
        }
        if (relativeIterator != compressor.options.end()) {
            errorBound = std::min(
                errorBound,
                relativeIterator->second *
                    (componentMaximum[componentIndex] - componentMinimum[componentIndex]));
        }
        precise = precise && absoluteError <= std::nextafter(
            errorBound,
            std::numeric_limits<double>::infinity());
    }
    for (std::size_t componentIndex = 0u;
         componentIndex < componentCount;
         ++componentIndex) {
        result.AddDiagnostic(
            caseName + ".component" + std::to_string(componentIndex) +
            ".max_abs_error=" + std::to_string(maximumErrors[componentIndex]));
    }
    return Require(
        result,
        precise,
        caseName + ".precision",
        "reference codec exceeded a component precision bound");
}

template<typename TValue>
inline bool RunReferenceCodecPrecisionCase(
    TestResult& result,
    const NumericArrayReferenceCodecId codecId,
    const CompressorConfig& compressor,
    const std::size_t tupleCount,
    const std::size_t componentCount,
    const std::string& caseName) {
    const auto reference = MakeReferencePrecisionValues<TValue>(
        tupleCount,
        componentCount,
        false);
    const auto current = MakeReferencePrecisionValues<TValue>(
        tupleCount,
        componentCount,
        true);
    ScratchByteBufferPool scratchBytePool;
    NumericArrayReferenceEncodedBlock encoded;
    std::vector<std::uint8_t> decodedBytes;
    std::string error;
    const bool encodedOk = EncodeDecodeReferenceTestBlock(
        codecId,
        compressor,
        scratchBytePool,
        current,
        reference,
        componentCount,
        NumericArrayReferenceKind::IntraArray,
        encoded,
        decodedBytes,
        &error);
    if (!Require(
            result,
            encodedOk,
            caseName + ".roundTrip",
            error.empty() ? "reference codec round trip failed" : error)) {
        return false;
    }
    return CheckReferencePrecision(
        result,
        current,
        decodedBytes,
        componentCount,
        compressor,
        caseName);
}

inline bool RunReferenceCodecDefaultCase(TestResult& result) {
    const IntraFieldReferenceControlParams intra;
    const TemporalFieldReferenceControlParams temporal;
    return Require(
               result,
               intra.codec == IntraFieldReferenceCodec::Affine,
               "referenceCodec.defaults.intra",
               "intra-field reference default is not affine") &&
        Require(
            result,
            temporal.codec == TemporalFieldReferenceCodec::Predictor,
            "referenceCodec.defaults.temporal",
            "temporal reference default is not predictor") &&
        Require(
            result,
            !temporal.predictor.enableLocalWindowSearch,
            "referenceCodec.defaults.offsetSearch",
            "temporal predictor offset search is enabled by default");
}

inline bool RunReferenceCodecNonFiniteCase(TestResult& result) {
    auto reference = MakeReferencePrecisionValues<float>(33u, 1u, false);
    auto current = MakeReferencePrecisionValues<float>(33u, 1u, true);
    current[7] = std::numeric_limits<float>::quiet_NaN();
    bool rejectedAll = true;
    for (const auto codecId : {
             NumericArrayReferenceCodecId::Predictor,
             NumericArrayReferenceCodecId::Affine,
             NumericArrayReferenceCodecId::Wavelet}) {
        ScratchByteBufferPool scratchBytePool;
        NumericArrayReferenceEncodedBlock encoded;
        std::vector<std::uint8_t> decodedBytes;
        std::string error;
        rejectedAll = rejectedAll && !EncodeDecodeReferenceTestBlock(
            codecId,
            MakeRelativeErrorNumericArrayCompressor(1.0e-4),
            scratchBytePool,
            current,
            reference,
            1u,
            NumericArrayReferenceKind::IntraArray,
            encoded,
            decodedBytes,
            &error);
    }
    return Require(
        result,
        rejectedAll,
        "referenceCodec.nonFinite",
        "a reference codec accepted a non-finite input value");
}

inline bool RunReferenceCodecTypeDispatchCase(TestResult& result) {
    std::vector<std::int32_t> reference(33u, 0);
    std::vector<std::int32_t> current(33u, 0);
    for (std::size_t index = 0u; index < current.size(); ++index) {
        reference[index] = static_cast<std::int32_t>(index) * 7 - 80;
        current[index] = reference[index] + static_cast<std::int32_t>(index % 5u);
    }
    bool rejectedAll = true;
    for (const auto codecId : {
             NumericArrayReferenceCodecId::Predictor,
             NumericArrayReferenceCodecId::Affine}) {
        ScratchByteBufferPool scratchBytePool;
        NumericArrayReferenceEncodedBlock encoded;
        std::vector<std::uint8_t> decodedBytes;
        std::string error;
        rejectedAll = rejectedAll && !EncodeDecodeReferenceTestBlock(
            codecId,
            MakeLosslessNumericArrayCompressor(),
            scratchBytePool,
            current,
            reference,
            1u,
            NumericArrayReferenceKind::IntraArray,
            encoded,
            decodedBytes,
            &error);
    }
    return Require(
        result,
        rejectedAll,
        "referenceCodec.typeDispatch",
        "a floating-point reference codec accepted int32 data");
}

inline bool RunIntegerWaveletCase(TestResult& result) {
    constexpr std::size_t kTupleCount = 4097u;
    constexpr std::size_t kComponentCount = 2u;
    std::vector<std::int32_t> reference(kTupleCount * kComponentCount, 0);
    std::vector<std::int32_t> current(reference.size(), 0);
    for (std::size_t valueIndex = 0u; valueIndex < current.size(); ++valueIndex) {
        reference[valueIndex] = static_cast<std::int32_t>(
            (valueIndex * 7919u) % 1000003u) - 500001;
        current[valueIndex] = reference[valueIndex] +
            static_cast<std::int32_t>(valueIndex % 17u) - 8;
    }
    ScratchByteBufferPool scratchBytePool;
    NumericArrayReferenceEncodedBlock encoded;
    std::vector<std::uint8_t> decodedBytes;
    std::string error;
    const bool roundTrip = EncodeDecodeReferenceTestBlock(
        NumericArrayReferenceCodecId::Wavelet,
        MakeLosslessNumericArrayCompressor(),
        scratchBytePool,
        current,
        reference,
        kComponentCount,
        NumericArrayReferenceKind::IntraArray,
        encoded,
        decodedBytes,
        &error);
    return Require(
               result,
               roundTrip,
               "referenceCodec.wavelet.int32.roundTrip",
               error.empty() ? "integer wavelet round trip failed" : error) &&
        Require(
            result,
            decodedBytes == std::vector<std::uint8_t>(
                NumericValueBytes(current).begin(),
                NumericValueBytes(current).end()),
            "referenceCodec.wavelet.int32.lossless",
            "integer wavelet reconstruction is not lossless");
}

inline bool RunBoundedProbePreparedPayloadCase(TestResult& result) {
    constexpr std::size_t kTupleCount = 8193u;
    constexpr std::size_t kComponentCount = 3u;
    const auto reference = MakeReferencePrecisionValues<float>(
        kTupleCount,
        kComponentCount,
        false);
    const auto current = MakeReferencePrecisionValues<float>(
        kTupleCount,
        kComponentCount,
        true);
    const auto meta = MakeReferenceTestMeta<float>(kTupleCount, kComponentCount);

    numericarray::NumericArraySource currentSource;
    numericarray::NumericArraySource referenceSource;
    std::string error;
    if (!numericarray::MakeNumericArrayLayoutFromMeta(meta, currentSource.layout, &error) ||
        !numericarray::MakeNumericArrayLayoutFromMeta(meta, referenceSource.layout, &error)) {
        return Require(
            result,
            false,
            "referenceCodec.boundedProbe.layout",
            error.empty() ? "failed to prepare bounded probe layouts" : error);
    }
    currentSource.values = MakeCompactNumericArrayView(
        current.data(),
        ScalarType::Float32,
        kTupleCount,
        static_cast<int>(kComponentCount));
    referenceSource.values = MakeCompactNumericArrayView(
        reference.data(),
        ScalarType::Float32,
        kTupleCount,
        static_cast<int>(kComponentCount));

    numericarrayreference::NumericArrayReferenceSourceData referenceData{
        .candidate = NumericArrayReferenceCandidate{
            .scope = NumericArrayReferenceScope::IntraArray,
            .localParentFieldIndex = 0u,
        },
        .meta = meta,
        .source = referenceSource,
    };
    ScratchByteBufferPool scratchBytePool;
    window::WindowBudget windowBudget(64u * 1024u * 1024u);
    bytestore::ByteStoreSession byteStoreSession;
    std::shared_ptr<bytestore::IByteSource> transferCache;
    std::vector<NumericArrayBlockLayoutParams> blockLayouts;
    const auto built = numericarrayreference::BuildNumericArrayReferenceTransferCache(
        meta,
        MakeAbsoluteErrorNumericArrayCompressor(1.0e-3),
        currentSource,
        referenceData,
        NumericArrayReferenceCodecId::Affine,
        numericarrayreference::NumericArrayReferenceTransferControl{
            .affineBlockRSquared = 0.0,
            .selectionMode = ReferenceSelectionMode::Auto,
            .autoSelectionStrategy = ReferenceAutoSelectionStrategy::BoundedProbe,
            .spatialBlockElementCount = static_cast<std::uint32_t>(kTupleCount),
            .useMemoryStaging = true,
            .useMemoryTransferCache = true,
        },
        scratchBytePool,
        windowBudget,
        1u * 1024u * 1024u,
        transferCache,
        byteStoreSession,
        &blockLayouts,
        &error,
        "reference_bounded_probe_test");
    return Require(
               result,
               built,
               "referenceCodec.boundedProbe.build",
               error.empty() ? "bounded probe transfer build failed" : error) &&
        Require(
            result,
            transferCache != nullptr && transferCache->CanRead(),
            "referenceCodec.boundedProbe.transfer",
            "bounded probe transfer cache is unavailable") &&
        Require(
            result,
            blockLayouts.size() == 1u,
            "referenceCodec.boundedProbe.blockCount",
            "bounded probe transfer produced an unexpected block count");
}

[[nodiscard]] inline TestResult RunDataCodecFeatureReferenceCodecs() noexcept {
    TestResult result;
    try {
        RunReferenceCodecDefaultCase(result);
        const std::vector<NumericArrayReferenceCodecId> codecs{
            NumericArrayReferenceCodecId::Predictor,
            NumericArrayReferenceCodecId::Affine,
            NumericArrayReferenceCodecId::Wavelet,
        };
        for (const auto codecId : codecs) {
            const auto codecName = std::to_string(static_cast<std::uint16_t>(codecId));
            RunReferenceCodecPrecisionCase<float>(
                result,
                codecId,
                MakeAbsoluteErrorNumericArrayCompressor(1.0e-3),
                4097u,
                3u,
                "referenceCodec." + codecName + ".float32.absolute");
            RunReferenceCodecPrecisionCase<double>(
                result,
                codecId,
                MakeRelativeErrorNumericArrayCompressor(1.0e-5),
                4097u,
                3u,
                "referenceCodec." + codecName + ".float64.relative");
        }
        RunReferenceCodecNonFiniteCase(result);
        RunReferenceCodecTypeDispatchCase(result);
        RunIntegerWaveletCase(result);
        RunBoundedProbePreparedPayloadCase(result);
    } catch (const std::exception& exception) {
        result.AddFailure("referenceCodec.exception", exception.what());
    } catch (...) {
        result.AddFailure("referenceCodec.exception", "unknown exception");
    }
    return result;
}

} // namespace datacodec::test

#endif
