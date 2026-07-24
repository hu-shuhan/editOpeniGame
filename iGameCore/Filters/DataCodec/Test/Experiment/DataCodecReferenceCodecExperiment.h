#ifndef DATACODEC_TEST_EXPERIMENT_DATACODECREFERENCECODECEXPERIMENT_H
#define DATACODEC_TEST_EXPERIMENT_DATACODECREFERENCECODECEXPERIMENT_H

#include "DataCodec/API/Params/CodecParamDefaults.h"
#include "DataCodec/Test/Common/ReferenceCodecTestHarness.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace datacodec::test {

inline std::vector<double> MakeTemporalReferenceExperimentFrame(
    const std::size_t frameIndex) {
    constexpr std::size_t kTupleCount = 32769u;
    constexpr std::size_t kComponentCount = 3u;
    std::vector<double> values(kTupleCount * kComponentCount, 0.0);
    for (std::size_t tupleIndex = 0u; tupleIndex < kTupleCount; ++tupleIndex) {
        const auto position = static_cast<double>(tupleIndex);
        const auto time = static_cast<double>(frameIndex);
        values[tupleIndex * kComponentCount] =
            120.0 * std::sin(position * 0.0017) +
            0.06 * time * std::cos(position * 0.0031);
        values[tupleIndex * kComponentCount + 1u] =
            2.0 * std::cos(position * 0.0023) +
            0.003 * time * std::sin(position * 0.0047);
        values[tupleIndex * kComponentCount + 2u] =
            0.02 * std::sin(position * 0.0037) +
            0.00004 * time * std::cos(position * 0.0053);
    }
    return values;
}

inline bool RunTemporalReferenceExperimentCodec(
    TestResult& result,
    const NumericArrayReferenceCodecId codecId,
    const std::vector<double>& keyFrame,
    const std::string& codecName) {
    constexpr std::size_t kComponentCount = 3u;
    std::size_t totalPayloadBytes = 0u;
    ScratchByteBufferPool scratchBytePool;
    const auto begin = std::chrono::steady_clock::now();
    for (std::size_t frameIndex = 1u; frameIndex < 5u; ++frameIndex) {
        const auto current = MakeTemporalReferenceExperimentFrame(frameIndex);
        NumericArrayReferenceEncodedBlock encoded;
        std::vector<std::uint8_t> decodedBytes;
        std::string error;
        if (!EncodeDecodeReferenceTestBlock(
                codecId,
                MakeRelativeErrorNumericArrayCompressor(1.0e-5),
                scratchBytePool,
                current,
                keyFrame,
                kComponentCount,
                NumericArrayReferenceKind::TemporalKeyFrame,
                encoded,
                decodedBytes,
                &error)) {
            result.AddFailure(
                "referenceExperiment." + codecName + ".frame" +
                    std::to_string(frameIndex),
                error.empty() ? "temporal reference experiment failed" : error);
            return false;
        }
        totalPayloadBytes += ReferenceEncodedPayloadBytes(encoded);
    }
    const auto elapsedMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - begin).count();
    result.AddDiagnostic(
        "referenceExperiment." + codecName +
        ".payload_bytes=" + std::to_string(totalPayloadBytes));
    result.AddDiagnostic(
        "referenceExperiment." + codecName +
        ".elapsed_us=" + std::to_string(elapsedMicroseconds));
    return true;
}

[[nodiscard]] inline TestResult RunDataCodecReferenceCodecExperiment() noexcept {
    TestResult result;
    try {
        const auto keyFrame = MakeTemporalReferenceExperimentFrame(0u);
        RunTemporalReferenceExperimentCodec(
            result,
            NumericArrayReferenceCodecId::Wavelet,
            keyFrame,
            "wavelet");
        RunTemporalReferenceExperimentCodec(
            result,
            NumericArrayReferenceCodecId::Predictor,
            keyFrame,
            "predictor");
    } catch (const std::exception& exception) {
        result.AddFailure("referenceExperiment.exception", exception.what());
    } catch (...) {
        result.AddFailure("referenceExperiment.exception", "unknown exception");
    }
    return result;
}

} // namespace datacodec::test

#endif
