#ifndef DATACODEC_COMMON_DATACODECCALLBACK_H
#define DATACODEC_COMMON_DATACODECCALLBACK_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
namespace datacodec::callback {

using ProgressCallback = std::function<void(double)>;
using ResourceCallback = std::function<void(std::string_view, std::uint64_t)>;
using PhaseTimingCallback = std::function<void(std::string_view, double)>;
using PhaseTimePoint = std::chrono::steady_clock::time_point;

inline PhaseTimePoint Now() {
    return std::chrono::steady_clock::now();
}

inline double NormalizeProgress(const double normalized) {
    return std::clamp(normalized, 0.0, 1.0);
}

inline void InvokeProgress(
    const ProgressCallback& progressCallback,
    const double normalized) {
    if (!progressCallback) {
        return;
    }
    progressCallback(NormalizeProgress(normalized));
}

inline std::string MakeResourceName(
    const std::string_view prefix,
    const std::string_view suffix) {
    std::string name(prefix);
    if (!name.empty() && !suffix.empty()) {
        name.push_back('.');
    }
    name.append(suffix.data(), suffix.size());
    return name;
}

inline void InvokeResource(
    const ResourceCallback& resourceCallback,
    const std::string_view stepName,
    const std::uint64_t logicalBytes) {
    if (!resourceCallback) {
        return;
    }
    resourceCallback(stepName, logicalBytes);
}

inline PhaseTimePoint StartPhase(const PhaseTimingCallback& phaseTimingCallback) {
    if (!phaseTimingCallback) {
        return {};
    }
    return Now();
}

template <typename TCallback>
inline PhaseTimePoint StartTiming(const TCallback& timingCallback) {
    if (!timingCallback) {
        return {};
    }
    return Now();
}

inline double ElapsedMilliseconds(const PhaseTimePoint startTime) {
    return std::chrono::duration<double, std::milli>(
        Now() - startTime).count();
}

inline double ElapsedMilliseconds(
    const PhaseTimePoint startTime,
    const PhaseTimePoint endTime) {
    return std::chrono::duration<double, std::milli>(endTime - startTime).count();
}

inline std::chrono::nanoseconds ElapsedNanoseconds(const PhaseTimePoint startTime) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Now() - startTime);
}

template <typename TCallback, typename TEvent>
inline void InvokeTimingEvent(
    const TCallback& timingCallback,
    TEvent&& event) {
    if (!timingCallback) {
        return;
    }
    timingCallback(std::forward<TEvent>(event));
}

inline void InvokePhaseTiming(
    const PhaseTimingCallback& phaseTimingCallback,
    const std::string_view phaseName,
    const double elapsedMs) {
    if (!phaseTimingCallback) {
        return;
    }
    phaseTimingCallback(phaseName, elapsedMs);
}

inline PhaseTimePoint MarkPhase(
    const PhaseTimingCallback& phaseTimingCallback,
    const std::string_view phaseName,
    const PhaseTimePoint phaseStart) {
    if (!phaseTimingCallback) {
        return phaseStart;
    }
    const auto now = Now();
    InvokePhaseTiming(
        phaseTimingCallback,
        phaseName,
        std::chrono::duration<double, std::milli>(now - phaseStart).count());
    return now;
}

} // namespace datacodec::callback

#endif
