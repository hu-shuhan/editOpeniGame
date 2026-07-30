#ifndef DATACODEC_LOG_TELEMETRY_SINKS_TELEMETRYSESSIONSINK_H
#define DATACODEC_LOG_TELEMETRY_SINKS_TELEMETRYSESSIONSINK_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/Log/Telemetry/TelemetrySession.h"

#include <algorithm>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace datacodec {

inline constexpr RunRecordMask kTelemetrySessionRecordMask =
    kRunLifecycleRecordMask |
    RunRecordKind::Message |
    RunRecordKind::StageTiming |
    RunRecordKind::ResourceUsage |
    RunRecordKind::Artifact;

class TelemetrySessionSink final : public IRunRecordSink {
public:
    explicit TelemetrySessionSink(
        const RunRecordMask interests =
            kRunLifecycleRecordMask |
            RunRecordKind::Message,
        const RunCollectionMask collectionRequests = 0u)
        : m_interests((interests & kTelemetrySessionRecordMask) |
              kRunLifecycleRecordMask),
          m_collectionRequests(collectionRequests) {}

    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        return m_interests;
    }

    [[nodiscard]] RunCollectionMask CollectionRequests() const noexcept override {
        return m_collectionRequests;
    }

    void Submit(const RunRecord& record) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::visit([this](const auto& value) { Consume(value); }, record);
    }

    [[nodiscard]] std::optional<TelemetrySession> TakeSession(
        const std::uint64_t runId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto iterator = m_sessions.find(runId);
        if (iterator == m_sessions.end()) {
            return std::nullopt;
        }
        auto session = std::move(iterator->second);
        m_sessions.erase(iterator);
        m_completedRunIds.erase(
            std::remove(m_completedRunIds.begin(), m_completedRunIds.end(), runId),
            m_completedRunIds.end());
        return session;
    }

    [[nodiscard]] std::vector<TelemetrySession> SnapshotCompletedSessions() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<TelemetrySession> sessions;
        sessions.reserve(m_completedRunIds.size());
        for (const auto runId : m_completedRunIds) {
            const auto iterator = m_sessions.find(runId);
            if (iterator != m_sessions.end()) {
                sessions.push_back(iterator->second);
            }
        }
        std::sort(
            sessions.begin(),
            sessions.end(),
            [](const auto& left, const auto& right) { return left.runId < right.runId; });
        return sessions;
    }

    [[nodiscard]] std::vector<TelemetryMessageRecord> SnapshotMessages() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto messages = m_unscopedMessages;
        auto runIds = m_completedRunIds;
        std::sort(runIds.begin(), runIds.end());
        for (const auto runId : runIds) {
            const auto iterator = m_sessions.find(runId);
            if (iterator == m_sessions.end()) {
                continue;
            }
            messages.insert(
                messages.end(),
                iterator->second.messages.begin(),
                iterator->second.messages.end());
        }
        return messages;
    }

private:
    void Consume(const RunBeginRecord& record) {
        m_completedRunIds.erase(
            std::remove(
                m_completedRunIds.begin(),
                m_completedRunIds.end(),
                record.run.runId),
            m_completedRunIds.end());
        TelemetrySession session;
        session.runId = record.run.runId;
        session.parentRunId = record.run.parentRunId;
        session.generatedAtUtc = record.run.generatedAtUtc;
        session.runKind = record.run.runKind;
        session.objectName = record.run.objectName;
        session.leafPath = record.run.leafPath;
        session.meshType = record.run.meshType;
        m_sessions[record.run.runId] = std::move(session);
    }

    void Consume(const RunEndRecord& record) {
        auto& session = m_sessions[record.run.runId];
        session.runId = record.run.runId;
        session.parentRunId = record.run.parentRunId;
        session.generatedAtUtc = record.run.generatedAtUtc;
        session.runKind = record.run.runKind;
        session.objectName = record.run.objectName;
        session.leafPath = record.run.leafPath;
        session.meshType = record.run.meshType;
        session.success = record.success;
        session.elapsedMs = record.elapsedMs;
        session.inputBytes = record.inputBytes;
        session.outputBytes = record.outputBytes;
        session.sourceBytes = record.sourceBytes;
        session.topologyBytes = record.topologyBytes;
        session.compressedPayloadBytes = record.compressedPayloadBytes;
        std::stable_sort(
            session.messages.begin(),
            session.messages.end(),
            [](const auto& left, const auto& right) { return left.order < right.order; });
        std::stable_sort(
            session.stages.begin(),
            session.stages.end(),
            [](const auto& left, const auto& right) { return left.order < right.order; });
        std::stable_sort(
            session.artifacts.begin(),
            session.artifacts.end(),
            [](const auto& left, const auto& right) { return left.order < right.order; });
        if (std::find(
                m_completedRunIds.begin(),
                m_completedRunIds.end(),
                record.run.runId) == m_completedRunIds.end()) {
            m_completedRunIds.push_back(record.run.runId);
        }
    }

    void Consume(const RunMessageRecord& record) {
        const auto iterator = m_sessions.find(record.runId);
        if (iterator != m_sessions.end()) {
            iterator->second.AddMessage(record.message);
            return;
        }
        m_unscopedMessages.push_back(record.message);
    }

    void Consume(const RunStageTimingRecord& record) {
        const auto iterator = m_sessions.find(record.runId);
        if (iterator != m_sessions.end()) {
            iterator->second.AddStageRecord(record.stage);
        }
    }

    void Consume(const RunResourceUsageRecord& record) {
        const auto iterator = m_sessions.find(record.runId);
        if (iterator != m_sessions.end()) {
            iterator->second.AddStageRecord(record.stage);
        }
    }

    void Consume(const RunArtifactRecord& record) {
        const auto iterator = m_sessions.find(record.runId);
        if (iterator != m_sessions.end()) {
            iterator->second.AddArtifact(record.artifact);
        }
    }

    void Consume(const RunProgressRecord&) {}
    void Consume(const RunRemapOrderRecord&) {}

    RunRecordMask m_interests{0u};
    RunCollectionMask m_collectionRequests{0u};
    mutable std::mutex m_mutex;
    std::unordered_map<std::uint64_t, TelemetrySession> m_sessions;
    std::vector<std::uint64_t> m_completedRunIds;
    std::vector<TelemetryMessageRecord> m_unscopedMessages;
};

} // 命名空间 datacodec

#endif
