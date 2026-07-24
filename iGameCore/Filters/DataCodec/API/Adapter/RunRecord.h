#ifndef DATACODEC_API_ADAPTER_RUNRECORD_H
#define DATACODEC_API_ADAPTER_RUNRECORD_H

#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/API/Adapter/RunRecordTypes.h"

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace datacodec {

class IRemapProvider;

enum class RunRecordKind : std::uint32_t {
    None = 0u,
    RunBegin = 1u << 0u,
    RunEnd = 1u << 1u,
    Progress = 1u << 2u,
    Message = 1u << 3u,
    StageTiming = 1u << 4u,
    ResourceUsage = 1u << 5u,
    Artifact = 1u << 6u,
    RemapOrder = 1u << 7u,
};

using RunRecordMask = std::uint32_t;

[[nodiscard]] constexpr RunRecordMask RunRecordBit(const RunRecordKind kind) noexcept {
    return static_cast<RunRecordMask>(kind);
}

[[nodiscard]] constexpr RunRecordMask operator|(
    const RunRecordKind left,
    const RunRecordKind right) noexcept {
    return RunRecordBit(left) | RunRecordBit(right);
}

[[nodiscard]] constexpr RunRecordMask operator|(
    const RunRecordMask left,
    const RunRecordKind right) noexcept {
    return left | RunRecordBit(right);
}

inline constexpr RunRecordMask kRunLifecycleRecordMask =
    RunRecordKind::RunBegin | RunRecordKind::RunEnd;

enum class RunCollectionKind : std::uint32_t {
    None = 0u,
    MemoryTrace = 1u << 0u,
};

using RunCollectionMask = std::uint32_t;

[[nodiscard]] constexpr RunCollectionMask RunCollectionBit(
    const RunCollectionKind kind) noexcept {
    return static_cast<RunCollectionMask>(kind);
}

struct RunRecordInfo {
    std::uint64_t runId{0u};
    std::uint64_t parentRunId{0u};
    std::string generatedAtUtc;
    TelemetryRunKind runKind{TelemetryRunKind::Unknown};
    std::string objectName;
    BlockPath leafPath;
    std::string meshType;
};

struct RunBeginRecord {
    RunRecordInfo run;
};

struct RunEndRecord {
    RunRecordInfo run;
    bool success{false};
    double elapsedMs{0.0};
    std::uint64_t inputBytes{0u};
    std::uint64_t outputBytes{0u};
    std::uint64_t sourceBytes{0u};
    std::uint64_t topologyBytes{0u};
    std::uint64_t compressedPayloadBytes{0u};
};

enum class RunProgressPhase : std::uint8_t {
    Begin,
    Update,
    Finish,
};

struct RunProgressRecord {
    std::uint64_t runId{0u};
    RunProgressPhase phase{RunProgressPhase::Update};
    double normalized{0.0};
    std::string text;
    bool success{false};
    std::uint32_t frameOrdinal{0u};
    std::uint32_t frameCount{0u};
};

struct RunMessageRecord {
    std::uint64_t runId{0u};
    TelemetryMessageRecord message;
};

struct RunStageTimingRecord {
    std::uint64_t runId{0u};
    TelemetryStageRecord stage;
};

struct RunResourceUsageRecord {
    std::uint64_t runId{0u};
    TelemetryStageRecord stage;
};

struct RunArtifactRecord {
    std::uint64_t runId{0u};
    TelemetryArtifactRecord artifact;
};

enum class RunRemapDomain : std::uint8_t {
    Point,
    Cell,
};

struct RunRemapOrderRecord {
    std::uint64_t runId{0u};
    BlockPath leafPath;
    RunRemapDomain domain{RunRemapDomain::Point};
    const IRemapProvider* provider{nullptr};
};

using RunRecord = std::variant<
    RunBeginRecord,
    RunEndRecord,
    RunProgressRecord,
    RunMessageRecord,
    RunStageTimingRecord,
    RunResourceUsageRecord,
    RunArtifactRecord,
    RunRemapOrderRecord>;

[[nodiscard]] inline RunRecordKind GetRunRecordKind(const RunRecord& record) noexcept {
    return std::visit([](const auto& value) noexcept -> RunRecordKind {
        using Record = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Record, RunBeginRecord>) {
            return RunRecordKind::RunBegin;
        } else if constexpr (std::is_same_v<Record, RunEndRecord>) {
            return RunRecordKind::RunEnd;
        } else if constexpr (std::is_same_v<Record, RunProgressRecord>) {
            return RunRecordKind::Progress;
        } else if constexpr (std::is_same_v<Record, RunMessageRecord>) {
            return RunRecordKind::Message;
        } else if constexpr (std::is_same_v<Record, RunStageTimingRecord>) {
            return RunRecordKind::StageTiming;
        } else if constexpr (std::is_same_v<Record, RunResourceUsageRecord>) {
            return RunRecordKind::ResourceUsage;
        } else if constexpr (std::is_same_v<Record, RunArtifactRecord>) {
            return RunRecordKind::Artifact;
        } else {
            static_assert(std::is_same_v<Record, RunRemapOrderRecord>);
            return RunRecordKind::RemapOrder;
        }
    }, record);
}

} // 命名空间 datacodec

#endif
