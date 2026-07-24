#ifndef DATACODEC_RUNTIME_RECORD_RUNRECORDEMITTER_H
#define DATACODEC_RUNTIME_RECORD_RUNRECORDEMITTER_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

namespace datacodec {

[[nodiscard]] inline std::uint64_t NextRunRecordId() noexcept {
    static std::atomic_uint64_t nextId{1u};
    return nextId.fetch_add(1u, std::memory_order_relaxed);
}

class RunRecordEmitter {
public:
    void Reset(RunRecordInfo run, IRunRecordSink* sink) {
        if (run.runId == 0u) {
            run.runId = NextRunRecordId();
        }
        m_run = std::move(run);
        m_sink = sink;
        m_messageOrder.store(0u, std::memory_order_relaxed);
        m_stageOrder.store(0u, std::memory_order_relaxed);
        m_artifactOrder.store(0u, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t RunId() const noexcept {
        return m_run.runId;
    }

    [[nodiscard]] const RunRecordInfo& RunInfo() const noexcept {
        return m_run;
    }

    [[nodiscard]] bool Wants(const RunRecordKind kind) const noexcept {
        return m_sink != nullptr && m_sink->Wants(kind);
    }

    [[nodiscard]] bool Requests(const RunCollectionKind kind) const noexcept {
        return m_sink != nullptr && m_sink->Requests(kind);
    }

    void BeginRun() const {
        Submit(RunRecordKind::RunBegin, RunRecord{RunBeginRecord{m_run}});
    }

    void EndRun(RunEndRecord record) const {
        record.run = m_run;
        Submit(RunRecordKind::RunEnd, RunRecord{std::move(record)});
    }

    void SubmitProgress(RunProgressRecord record) const {
        record.runId = m_run.runId;
        Submit(RunRecordKind::Progress, RunRecord{std::move(record)});
    }

    void AddMessage(TelemetryMessageRecord message) {
        message.order = m_messageOrder.fetch_add(1u, std::memory_order_relaxed);
        Submit(
            RunRecordKind::Message,
            RunRecord{RunMessageRecord{m_run.runId, std::move(message)}});
    }

    void AddInfo(std::string origin, std::string text) {
        AddMessage(TelemetryMessageRecord{
            .severity = TelemetryMessageSeverity::Info,
            .origin = std::move(origin),
            .text = std::move(text),
        });
    }

    void AddWarning(std::string origin, std::string text) {
        AddMessage(TelemetryMessageRecord{
            .severity = TelemetryMessageSeverity::Warning,
            .origin = std::move(origin),
            .text = std::move(text),
        });
    }

    void AddError(std::string origin, std::string text) {
        AddMessage(TelemetryMessageRecord{
            .severity = TelemetryMessageSeverity::Error,
            .origin = std::move(origin),
            .text = std::move(text),
        });
    }

    void RecordStageTiming(
        std::string stageName,
        const double elapsedMs,
        const TelemetryStageCategory category = TelemetryStageCategory::General,
        std::string scope = {}) {
        TelemetryStageRecord stage{
            .name = std::move(stageName),
            .order = m_stageOrder.fetch_add(1u, std::memory_order_relaxed),
            .elapsedMs = elapsedMs,
            .category = category,
            .scope = std::move(scope),
        };
        Submit(
            RunRecordKind::StageTiming,
            RunRecord{RunStageTimingRecord{m_run.runId, std::move(stage)}});
    }

    void RecordResourceUsage(
        std::string stageName,
        const std::uint64_t logicalBytes,
        const TelemetryStageCategory category = TelemetryStageCategory::General,
        std::string scope = {}) {
        TelemetryStageRecord stage{
            .name = std::move(stageName),
            .order = m_stageOrder.fetch_add(1u, std::memory_order_relaxed),
            .category = category,
            .scope = std::move(scope),
            .resource = MakeLogicalTelemetryResourceUsage(logicalBytes),
        };
        Submit(
            RunRecordKind::ResourceUsage,
            RunRecord{RunResourceUsageRecord{m_run.runId, std::move(stage)}});
    }

    void AddArtifact(TelemetryArtifactRecord artifact) {
        artifact.order = m_artifactOrder.fetch_add(1u, std::memory_order_relaxed);
        Submit(
            RunRecordKind::Artifact,
            RunRecord{RunArtifactRecord{m_run.runId, std::move(artifact)}});
    }

    void RecordRemapOrder(
        BlockPath leafPath,
        const RunRemapDomain domain,
        const IRemapProvider* provider) const {
        Submit(
            RunRecordKind::RemapOrder,
            RunRecord{RunRemapOrderRecord{
                .runId = m_run.runId,
                .leafPath = std::move(leafPath),
                .domain = domain,
                .provider = provider,
            }});
    }

private:
    void Submit(const RunRecordKind kind, const RunRecord& record) const {
        if (m_sink != nullptr && m_sink->Wants(kind)) {
            m_sink->Submit(record);
        }
    }

    RunRecordInfo m_run;
    IRunRecordSink* m_sink{nullptr};
    std::atomic_uint64_t m_messageOrder{0u};
    std::atomic_uint64_t m_stageOrder{0u};
    std::atomic_uint64_t m_artifactOrder{0u};
};

} // 命名空间 datacodec

#endif
