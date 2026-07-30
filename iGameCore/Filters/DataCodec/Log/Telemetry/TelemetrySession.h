#ifndef DATACODEC_LOG_TELEMETRY_TELEMETRYSESSION_H
#define DATACODEC_LOG_TELEMETRY_TELEMETRYSESSION_H

#include "DataCodec/API/Adapter/RunRecordTypes.h"
#include "DataCodec/Common/DataCodecTypes.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

struct TelemetrySession {
    std::uint64_t runId{0u};
    std::uint64_t parentRunId{0u};
    std::string generatedAtUtc;
    TelemetryRunKind runKind{TelemetryRunKind::Unknown};
    std::string objectName;
    BlockPath leafPath;
    std::string meshType;
    bool success{false};
    double elapsedMs{0.0};
    std::uint64_t inputBytes{0};
    std::uint64_t outputBytes{0};
    std::uint64_t sourceBytes{0};
    std::uint64_t topologyBytes{0};
    std::uint64_t compressedPayloadBytes{0};
    std::vector<TelemetryStageRecord> stages;
    std::vector<TelemetryMessageRecord> messages;
    std::vector<TelemetryArtifactRecord> artifacts;

    void AddStageRecord(TelemetryStageRecord record) {
        stages.push_back(std::move(record));
    }

    void AddMessage(TelemetryMessageRecord record) {
        messages.push_back(std::move(record));
    }

    void AddArtifact(TelemetryArtifactRecord record) {
        artifacts.push_back(std::move(record));
    }

    [[nodiscard]] double SourceRatio() const {
        if (sourceBytes == 0) {
            return 0.0;
        }
        return static_cast<double>(outputBytes) / static_cast<double>(sourceBytes);
    }
};

} // namespace datacodec

#endif
