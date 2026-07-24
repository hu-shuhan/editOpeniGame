#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREBYTERANGE_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREBYTERANGE_H

#include "DataCodec/Storage/ByteIO/CallbackByteRangeReader.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace datacodec::test {

[[nodiscard]] inline TestResult RunDataCodecFeatureByteRange() noexcept {
    TestResult result;
    std::uint64_t observedOffset = 0u;
    std::size_t observedSize = 0u;
    std::size_t callbackCount = 0u;
    CallbackByteRangeReader reader(
        64u,
        [&](const std::uint64_t offset,
            const std::span<std::uint8_t> output,
            std::string*) {
            ++callbackCount;
            observedOffset = offset;
            observedSize = output.size();
            for (std::size_t index = 0u; index < output.size(); ++index) {
                output[index] = static_cast<std::uint8_t>(offset + index);
            }
            return true;
        });

    std::array<std::uint8_t, 4u> bytes{};
    std::string error;
    const auto read = reader.ReadAt(9u, bytes, &error);
    Require(result, read, "byteRange.callback.read", error.empty() ? "callback read failed" : error);
    Require(result, reader.ByteSize() == 64u, "byteRange.callback.byteSize", "byte size mismatch");
    Require(result, callbackCount == 1u, "byteRange.callback.count", "callback count mismatch");
    Require(result, observedOffset == 9u, "byteRange.callback.offset", "offset was not forwarded");
    Require(result, observedSize == bytes.size(), "byteRange.callback.size", "size was not forwarded");
    Require(
        result,
        bytes == std::array<std::uint8_t, 4u>{9u, 10u, 11u, 12u},
        "byteRange.callback.bytes",
        "callback output bytes mismatch");

    error.clear();
    const auto invalidRead = reader.ReadAt(62u, bytes, &error);
    Require(result, !invalidRead, "byteRange.callback.bounds", "out-of-range read was accepted");
    Require(result, !error.empty(), "byteRange.callback.boundsError", "out-of-range read did not report an error");
    Require(result, callbackCount == 1u, "byteRange.callback.boundsCount", "invalid read invoked the callback");

    error.clear();
    const auto emptyRead = reader.ReadAt(64u, std::span<std::uint8_t>{}, &error);
    Require(result, emptyRead, "byteRange.callback.empty", error.empty() ? "empty tail read failed" : error);
    Require(result, callbackCount == 1u, "byteRange.callback.emptyCount", "empty read invoked the callback");

    CallbackByteRangeReader missingCallbackReader(8u, {});
    std::array<std::uint8_t, 1u> missingBytes{};
    error.clear();
    const auto missingRead = missingCallbackReader.ReadAt(0u, missingBytes, &error);
    Require(result, !missingRead, "byteRange.callback.missing", "missing callback read was accepted");
    Require(result, !error.empty(), "byteRange.callback.missingError", "missing callback did not report an error");

    MemoryByteRangeReader memoryReader(std::vector<std::uint8_t>{1u, 2u, 3u, 4u});
    std::span<const std::uint8_t> contiguousBytes;
    error.clear();
    const auto contiguousReady = memoryReader.PrepareContiguousRange(
        1u,
        2u,
        contiguousBytes,
        &error);
    Require(
        result,
        contiguousReady == ContiguousViewStatus::Ready &&
            contiguousBytes.size() == 2u &&
            contiguousBytes[0] == 2u &&
            contiguousBytes[1] == 3u,
        "byteRange.contiguous.ready",
        error.empty() ? "memory contiguous view was not returned" : error);

    error.clear();
    const auto contiguousError = memoryReader.PrepareContiguousRange(
        3u,
        2u,
        contiguousBytes,
        &error);
    Require(
        result,
        contiguousError == ContiguousViewStatus::Error && !error.empty(),
        "byteRange.contiguous.error",
        "invalid contiguous range did not produce an error status");

    error.clear();
    const auto contiguousUnavailable = reader.PrepareContiguousRange(
        0u,
        4u,
        contiguousBytes,
        &error);
    Require(
        result,
        contiguousUnavailable == ContiguousViewStatus::Unavailable && error.empty(),
        "byteRange.contiguous.unavailable",
        "reader without contiguous capability did not report unavailable");

    const auto prefetchUnavailable = memoryReader.PrefetchRange(0u, 4u);
    Require(
        result,
        prefetchUnavailable.IsUnavailable(),
        "byteRange.prefetch.unavailable",
        "reader without prefetch capability did not report unavailable");

    auto sharedMemoryReader = std::make_shared<MemoryByteRangeReader>(
        std::vector<std::uint8_t>{1u, 2u, 3u, 4u});
    SubrangeByteRangeReader subrangeReader(sharedMemoryReader, 1u, 2u);
    const auto prefetchError = subrangeReader.PrefetchRange(2u, 1u);
    Require(
        result,
        prefetchError.IsError() && !prefetchError.error.empty(),
        "byteRange.prefetch.error",
        "invalid prefetch range did not produce an error status");
    return result;
}

} // namespace datacodec::test

#endif
