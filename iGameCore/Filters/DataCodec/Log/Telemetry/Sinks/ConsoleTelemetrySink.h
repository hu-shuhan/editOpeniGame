#ifndef DATACODEC_LOG_TELEMETRY_SINKS_CONSOLETELEMETRYSINK_H
#define DATACODEC_LOG_TELEMETRY_SINKS_CONSOLETELEMETRYSINK_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"

#include <iostream>
#include <mutex>

namespace datacodec {

class ConsoleTelemetrySink final : public IRunRecordSink {
public:
    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        return kRunLifecycleRecordMask |
            RunRecordKind::Message |
            RunRecordKind::StageTiming |
            RunRecordKind::ResourceUsage;
    }

    void Submit(const RunRecord& record) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (const auto* end = std::get_if<RunEndRecord>(&record)) {
            std::clog
                << "DataCodec " << TelemetryRunKindName(end->run.runKind)
                << " '" << end->run.objectName << "'"
                << " elapsed=" << end->elapsedMs << "ms"
                << " input=" << end->inputBytes
                << " output=" << end->outputBytes
                << " source=" << end->sourceBytes
                << '\n';
            return;
        }
        if (const auto* stage = std::get_if<RunStageTimingRecord>(&record)) {
            PrintStage(stage->stage);
            return;
        }
        if (const auto* resource = std::get_if<RunResourceUsageRecord>(&record)) {
            PrintStage(resource->stage);
            return;
        }
        if (const auto* message = std::get_if<RunMessageRecord>(&record)) {
            std::cerr << "  " << TelemetryMessageSeverityName(message->message.severity) << " ";
            if (!message->message.origin.empty()) {
                std::cerr << message->message.origin << ": ";
            }
            if (!message->message.code.empty()) {
                std::cerr << "[" << message->message.code << "] ";
            }
            std::cerr << message->message.text << '\n';
        }
    }

private:
    static void PrintStage(const TelemetryStageRecord& stage) {
        std::clog << "  stage[" << stage.order << "] "
                  << stage.name << " " << stage.elapsedMs << "ms";
        if (!stage.scope.empty()) {
            std::clog << " scope=" << stage.scope;
        }
        if (stage.resource.valid) {
            std::clog << " logicalBytes=" << stage.resource.logicalBytes;
        }
        std::clog << '\n';
    }

    std::mutex m_mutex;
};

} // namespace datacodec

#endif
