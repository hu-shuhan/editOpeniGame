#ifndef DATACODEC_RUNTIME_RECORD_RUNRECORDEMITTER_H
#define DATACODEC_RUNTIME_RECORD_RUNRECORDEMITTER_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

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
        std::lock_guard<std::mutex> lock(m_messageMutex);
        m_messages.clear();
    }

    [[nodiscard]] std::uint64_t RunId() const noexcept {
        return m_run.runId;
    }

    [[nodiscard]] const RunRecordInfo& RunInfo() const noexcept {
        return m_run;
    }

    [[nodiscard]] DataCodecLanguage Language() const noexcept {
        return m_run.language;
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

    void SubmitProgress(
        const RunProgressPhase phase,
        const double normalized,
        const DataCodecMessageId messageId,
        std::initializer_list<DataCodecMessageArgument> arguments = {},
        const bool success = false,
        std::string technicalDetail = {}) const {
        auto message = LocalizeDataCodecMessage(
            m_run.language,
            messageId,
            arguments,
            std::move(technicalDetail));
        SubmitProgress(RunProgressRecord{
            .phase = phase,
            .normalized = normalized,
            .language = message.language,
            .messageId = message.id,
            .messageArguments = std::move(message.arguments),
            .text = std::move(message.text),
            .technicalDetail = std::move(message.technicalDetail),
            .success = success,
        });
    }

    void AddLocalizedMessage(
        const TelemetryMessageSeverity severity,
        std::string origin,
        const DataCodecMessageId messageId,
        std::initializer_list<DataCodecMessageArgument> arguments = {},
        std::string code = {},
        std::string technicalDetail = {}) {
        auto message = LocalizeDataCodecMessage(
            m_run.language,
            messageId,
            arguments,
            std::move(technicalDetail));
        AddMessage(TelemetryMessageRecord{
            .severity = severity,
            .origin = std::move(origin),
            .code = std::move(code),
            .language = message.language,
            .messageId = message.id,
            .messageArguments = std::move(message.arguments),
            .text = std::move(message.text),
            .technicalDetail = std::move(message.technicalDetail),
        });
    }

    void AddMessage(TelemetryMessageRecord message) {
        message.order = m_messageOrder.fetch_add(1u, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(m_messageMutex);
            m_messages.push_back(message);
        }
        Submit(
            RunRecordKind::Message,
            RunRecord{RunMessageRecord{
                .runId = m_run.runId,
                .runKind = m_run.runKind,
                .message = std::move(message),
            }});
    }

    [[nodiscard]] std::vector<TelemetryMessageRecord> TakeMessages() {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        std::stable_sort(
            m_messages.begin(),
            m_messages.end(),
            [](const auto& left, const auto& right) { return left.order < right.order; });
        auto messages = std::move(m_messages);
        m_messages.clear();
        return messages;
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
        RecordResourceUsage(
            std::move(stageName),
            MakeLogicalTelemetryResourceUsage(logicalBytes),
            category,
            std::move(scope));
    }

    void RecordResourceUsage(
        std::string stageName,
        TelemetryResourceUsage resource,
        const TelemetryStageCategory category = TelemetryStageCategory::General,
        std::string scope = {}) {
        TelemetryStageRecord stage{
            .name = std::move(stageName),
            .order = m_stageOrder.fetch_add(1u, std::memory_order_relaxed),
            .category = category,
            .scope = std::move(scope),
            .resource = std::move(resource),
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
    std::mutex m_messageMutex;
    std::vector<TelemetryMessageRecord> m_messages;
};

} // 命名空间 datacodec

#endif
