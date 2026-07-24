#ifndef DATACODEC_TEST_ASSERTIONS_WINDOWBUDGETASSERTIONS_H
#define DATACODEC_TEST_ASSERTIONS_WINDOWBUDGETASSERTIONS_H

#include "DataCodec/Storage/ByteIO/Window/WindowBudget.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <cstdint>
#include <string>
namespace datacodec::test {

struct WindowBudgetSnapshot {
    std::uint64_t activeBytes{0u};
    std::uint64_t peakActiveBytes{0u};
    std::uint64_t maxActiveBytes{0u};
};

[[nodiscard]] inline WindowBudgetSnapshot CaptureWindowBudget(
    const window::WindowBudget& budget) noexcept {
    return WindowBudgetSnapshot{
        .activeBytes = budget.ActiveBytes(),
        .peakActiveBytes = budget.PeakActiveBytes(),
        .maxActiveBytes = budget.MaxActiveBytes(),
    };
}

inline void RequireWindowBudgetSnapshot(
    TestResult& result,
    const WindowBudgetSnapshot& snapshot,
    const std::string& path,
    const std::uint64_t expectedActive,
    const std::uint64_t expectedPeak) {
    Require(
        result,
        snapshot.activeBytes == expectedActive,
        path + ".activeBytes",
        "active bytes mismatch");
    Require(
        result,
        snapshot.peakActiveBytes == expectedPeak,
        path + ".peakActiveBytes",
        "peak active bytes mismatch");
    Require(
        result,
        snapshot.activeBytes <= snapshot.maxActiveBytes,
        path + ".activeLimit",
        "active bytes exceed max active bytes");
    Require(
        result,
        snapshot.peakActiveBytes <= snapshot.maxActiveBytes,
        path + ".peakLimit",
        "peak active bytes exceed max active bytes");
}

inline void RequireWindowBudgetLimit(
    TestResult& result,
    const WindowBudgetSnapshot& snapshot,
    const std::string& path,
    const std::uint64_t expectedMaxActiveBytes) {
    Require(
        result,
        snapshot.maxActiveBytes == expectedMaxActiveBytes,
        path + ".maxActiveBytes",
        "max active bytes mismatch");
}

} // namespace datacodec::test

#endif
