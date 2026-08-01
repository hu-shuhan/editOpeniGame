#ifndef DATACODEC_LOG_TELEMETRY_TELEMETRYMEMORYTRACE_H
#define DATACODEC_LOG_TELEMETRY_TELEMETRYMEMORYTRACE_H

#include "DataCodec/API/Adapter/RunRecord.h"
#include "DataCodec/Log/Telemetry/TelemetryResidentSet.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <atomic>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace datacodec {

struct TelemetryMemoryModuleSummary {
    std::string name;
    std::uint64_t beforeWorkingSetBytes{0u};
    std::uint64_t afterWorkingSetBytes{0u};
    std::uint64_t peakWorkingSetBytes{0u};
};

struct TelemetryMemoryTraceSummary {
    bool valid{false};
    std::uint64_t beforeWorkingSetBytes{0u};
    std::uint64_t afterWorkingSetBytes{0u};
    std::uint64_t peakWorkingSetBytes{0u};
    std::vector<TelemetryMemoryModuleSummary> modules;
};

class TelemetryMemoryTraceRecorder {
public:
    TelemetryMemoryTraceRecorder() = default;
    TelemetryMemoryTraceRecorder(const TelemetryMemoryTraceRecorder&) = delete;
    TelemetryMemoryTraceRecorder& operator=(const TelemetryMemoryTraceRecorder&) = delete;

    ~TelemetryMemoryTraceRecorder() {
        std::string ignored;
        (void)Stop(&ignored);
    }

    bool Start(
        const RunRecordInfo&,
        std::string* error = nullptr) {
        std::string stopError;
        if (!Stop(&stopError)) {
            return validation::AssignError(error, stopError);
        }

        std::uint64_t residentSetBytes = 0u;
        if (!SampleResidentSet(residentSetBytes, error)) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_summary = {};
            m_summary.valid = true;
            m_summary.beforeWorkingSetBytes = residentSetBytes;
            m_summary.afterWorkingSetBytes = residentSetBytes;
            m_summary.peakWorkingSetBytes = residentSetBytes;
        }
        m_active.store(true, std::memory_order_release);
        return true;
    }

    bool Stop(std::string* error = nullptr) {
        if (!m_active.exchange(false, std::memory_order_acq_rel)) {
            return true;
        }
        std::uint64_t residentSetBytes = 0u;
        if (!SampleResidentSet(residentSetBytes, error)) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_summary.afterWorkingSetBytes = residentSetBytes;
            m_summary.peakWorkingSetBytes = std::max(
                m_summary.peakWorkingSetBytes,
                residentSetBytes);
        }
        return true;
    }

    void EnterScope(std::string, std::string moduleName) {
        if (!Active()) {
            return;
        }
        std::uint64_t residentSetBytes = 0u;
        if (!SampleResidentSet(residentSetBytes, nullptr)) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        RecordModuleSampleLocked(std::move(moduleName), residentSetBytes);
    }

    void LeaveScope(std::string, std::string moduleName) {
        if (!Active()) {
            return;
        }
        std::uint64_t residentSetBytes = 0u;
        if (!SampleResidentSet(residentSetBytes, nullptr)) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        RecordModuleSampleLocked(std::move(moduleName), residentSetBytes);
    }

    [[nodiscard]] bool Active() const noexcept {
        return m_active.load(std::memory_order_acquire);
    }

    [[nodiscard]] TelemetryMemoryTraceSummary Summary() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_summary;
    }

private:
    static bool SampleResidentSet(std::uint64_t& residentSetBytes, std::string* error) {
        return telemetrydetail::GetResidentSetMemoryBytes(residentSetBytes, error);
    }

    void RecordModuleSampleLocked(
        std::string moduleName,
        const std::uint64_t residentSetBytes) {
        m_summary.afterWorkingSetBytes = residentSetBytes;
        m_summary.peakWorkingSetBytes = std::max(
            m_summary.peakWorkingSetBytes,
            residentSetBytes);
        const auto iterator = std::find_if(
            m_summary.modules.begin(),
            m_summary.modules.end(),
            [&moduleName](const auto& module) { return module.name == moduleName; });
        if (iterator == m_summary.modules.end()) {
            m_summary.modules.push_back({
                .name = std::move(moduleName),
                .beforeWorkingSetBytes = residentSetBytes,
                .afterWorkingSetBytes = residentSetBytes,
                .peakWorkingSetBytes = residentSetBytes,
            });
            return;
        }
        iterator->afterWorkingSetBytes = residentSetBytes;
        iterator->peakWorkingSetBytes = std::max(
            iterator->peakWorkingSetBytes,
            residentSetBytes);
    }

    std::atomic_bool m_active{false};
    TelemetryMemoryTraceSummary m_summary;
    mutable std::mutex m_mutex;
};

inline std::string ResolveTelemetryMemoryTraceModule(const std::string_view stageName) {
    if (stageName.find("Geometry") != std::string_view::npos) {
        return "geometry";
    }
    if (stageName.find("Topo") != std::string_view::npos) {
        return "topology";
    }
    if (stageName.find("Attr") != std::string_view::npos ||
        stageName.find("Attribute") != std::string_view::npos) {
        return "attribute";
    }
    if (stageName.find("Remap") != std::string_view::npos) {
        return "remap";
    }
    if (stageName.find("Params") != std::string_view::npos) {
        return "params";
    }
    if (stageName.find("Commit") != std::string_view::npos) {
        return "commit";
    }
    return "pipeline";
}

[[nodiscard]] inline TelemetryResourceUsage MakeTelemetryMemoryResourceUsage(
    const std::uint64_t beforeWorkingSetBytes,
    const std::uint64_t afterWorkingSetBytes,
    const std::uint64_t peakWorkingSetBytes) {
    return TelemetryResourceUsage{
        .valid = true,
        .workingSetBytes = peakWorkingSetBytes,
        .workingSetBeforeBytes = beforeWorkingSetBytes,
        .workingSetAfterBytes = afterWorkingSetBytes,
        .peakWorkingSetBytes = peakWorkingSetBytes,
    };
}

template <typename TRunRecords>
inline void RecordTelemetryMemoryTraceSummary(
    TRunRecords& records,
    const TelemetryMemoryTraceSummary& summary) {
    if (!summary.valid) {
        return;
    }
    records.RecordResourceUsage(
        "memory.run",
        MakeTelemetryMemoryResourceUsage(
            summary.beforeWorkingSetBytes,
            summary.afterWorkingSetBytes,
            summary.peakWorkingSetBytes));
    for (const auto& module : summary.modules) {
        const auto category = module.name == "topology"
            ? TelemetryStageCategory::Topology
            : module.name == "geometry"
                ? TelemetryStageCategory::Geometry
                : module.name == "attribute"
                    ? TelemetryStageCategory::Attribute
                    : module.name == "remap"
                        ? TelemetryStageCategory::Remap
                        : module.name == "params"
                            ? TelemetryStageCategory::Params
                            : module.name == "commit"
                                ? TelemetryStageCategory::Commit
                                : TelemetryStageCategory::General;
        records.RecordResourceUsage(
            "memory." + module.name,
            MakeTelemetryMemoryResourceUsage(
                module.beforeWorkingSetBytes,
                module.afterWorkingSetBytes,
                module.peakWorkingSetBytes),
            category);
    }
}

template <typename TContext>
inline void RecordMemoryTraceStageEvent(
    TContext& context,
    const std::string_view stageName,
    const bool enter) {
    if (context.memoryTrace == nullptr || !context.memoryTrace->Active()) {
        return;
    }
    std::string stage(stageName);
    auto module = ResolveTelemetryMemoryTraceModule(stageName);
    if (enter) {
        context.memoryTrace->EnterScope(std::move(stage), std::move(module));
        return;
    }
    context.memoryTrace->LeaveScope(std::move(stage), std::move(module));
}

} // namespace datacodec

#endif
