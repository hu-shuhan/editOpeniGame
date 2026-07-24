#ifndef DATACODEC_LOG_TELEMETRY_TELEMETRYMEMORYTRACE_H
#define DATACODEC_LOG_TELEMETRY_TELEMETRYMEMORYTRACE_H

#include "DataCodec/API/Adapter/RunRecord.h"
#include "DataCodec/Log/Telemetry/TelemetryResidentSet.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <locale>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace datacodec {

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
        const RunRecordInfo& run,
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
            m_csvText.clear();
            m_runKind = TelemetryRunKindName(run.runKind);
            m_objectName = run.objectName;
            m_leafPath = run.leafPath;
            m_startTime = std::chrono::steady_clock::now();
            m_baselineResidentSetBytes = residentSetBytes;
            m_csvText =
                "elapsedMs,event,runKind,objectName,leafPath,stageName,moduleName,"
                "residentSetBytes,residentSetDeltaBytes\n";
            WriteRowLocked("begin", "run", "run", residentSetBytes);
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
            WriteRowLocked("end", "run", "run", residentSetBytes);
        }
        return true;
    }

    void EnterScope(std::string stageName, std::string moduleName) {
        if (!Active()) {
            return;
        }
        std::uint64_t residentSetBytes = 0u;
        if (!SampleResidentSet(residentSetBytes, nullptr)) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        WriteRowLocked("stage_enter", stageName, moduleName, residentSetBytes);
    }

    void LeaveScope(std::string stageName, std::string moduleName) {
        if (!Active()) {
            return;
        }
        std::uint64_t residentSetBytes = 0u;
        if (!SampleResidentSet(residentSetBytes, nullptr)) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        WriteRowLocked("stage_leave", stageName, moduleName, residentSetBytes);
    }

    [[nodiscard]] bool Active() const noexcept {
        return m_active.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::string CsvText() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_csvText;
    }

private:
    static bool SampleResidentSet(std::uint64_t& residentSetBytes, std::string* error) {
        return telemetrydetail::GetResidentSetMemoryBytes(residentSetBytes, error);
    }

    static void AppendCsvString(std::string& output, const std::string& value) {
        const bool needsQuotes =
            value.find_first_of(",\"\r\n") != std::string::npos;
        if (!needsQuotes) {
            output += value;
            return;
        }
        output.push_back('"');
        for (const char ch : value) {
            if (ch == '"') {
                output += "\"\"";
            } else {
                output.push_back(ch);
            }
        }
        output.push_back('"');
    }

    void WriteRowLocked(
        const std::string& event,
        const std::string& stageName,
        const std::string& moduleName,
        const std::uint64_t residentSetBytes) {
        const auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - m_startTime);
        const auto delta =
            static_cast<std::int64_t>(residentSetBytes) -
            static_cast<std::int64_t>(m_baselineResidentSetBytes);
        std::ostringstream numberStream;
        numberStream.imbue(std::locale::classic());
        numberStream.setf(std::ios::fixed, std::ios::floatfield);
        numberStream.precision(6);
        numberStream << elapsed.count();
        m_csvText += numberStream.str();
        m_csvText.push_back(',');
        AppendCsvString(m_csvText, event);
        m_csvText.push_back(',');
        AppendCsvString(m_csvText, m_runKind);
        m_csvText.push_back(',');
        AppendCsvString(m_csvText, m_objectName);
        m_csvText.push_back(',');
        AppendCsvString(m_csvText, m_leafPath);
        m_csvText.push_back(',');
        AppendCsvString(m_csvText, stageName);
        m_csvText.push_back(',');
        AppendCsvString(m_csvText, moduleName);
        m_csvText.push_back(',');
        m_csvText += std::to_string(residentSetBytes);
        m_csvText.push_back(',');
        m_csvText += std::to_string(delta);
        m_csvText.push_back('\n');
    }

    std::atomic_bool m_active{false};
    std::string m_csvText;
    std::string m_runKind;
    std::string m_objectName;
    BlockPath m_leafPath;
    std::uint64_t m_baselineResidentSetBytes{0u};
    std::chrono::steady_clock::time_point m_startTime{};
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
