#ifndef DATACODEC_RUNTIME_OUTPUT_DATACODECOUTPUTROUTER_H
#define DATACODEC_RUNTIME_OUTPUT_DATACODECOUTPUTROUTER_H

#include "DataCodec/API/Adapter/IRunRecordSink.h"
#include "DataCodec/API/Output/DataCodecOutputSinks.h"

#include <mutex>
#include <string>
#include <utility>

namespace datacodec {

class DataCodecOutputRouter final : public IRunRecordSink {
public:
    explicit DataCodecOutputRouter(DataCodecOutputSinks sinks)
        : m_sinks(std::move(sinks)) {}

    [[nodiscard]] RunRecordMask Interests() const noexcept override {
        RunRecordMask interests = 0u;
        if (m_sinks.ui != nullptr || m_sinks.console != nullptr ||
            m_sinks.progress != nullptr) {
            interests |= RunRecordBit(RunRecordKind::Progress);
        }
        if (m_sinks.ui != nullptr || m_sinks.console != nullptr) {
            interests |= RunRecordBit(RunRecordKind::Message);
        }
        if (m_sinks.reportFile != nullptr) {
            interests |= RunRecordBit(RunRecordKind::Artifact);
        }
        return interests;
    }

    void Submit(const RunRecord& record) override {
        if (const auto* progress = std::get_if<RunProgressRecord>(&record)) {
            SubmitProgressRecord(*progress);
            return;
        }
        if (const auto* message = std::get_if<RunMessageRecord>(&record)) {
            SubmitMessageRecord(*message);
            return;
        }
        if (const auto* artifact = std::get_if<RunArtifactRecord>(&record)) {
            SubmitArtifactRecord(*artifact);
        }
    }

    [[nodiscard]] std::string LastReportError() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_lastReportError;
    }

private:
    [[nodiscard]] static DataCodecStatusSeverity StatusSeverity(
        const TelemetryMessageSeverity severity) noexcept {
        switch (severity) {
            case TelemetryMessageSeverity::Warning:
                return DataCodecStatusSeverity::Warning;
            case TelemetryMessageSeverity::Error:
                return DataCodecStatusSeverity::Error;
            case TelemetryMessageSeverity::Critical:
                return DataCodecStatusSeverity::Critical;
            case TelemetryMessageSeverity::Info:
            default:
                return DataCodecStatusSeverity::Info;
        }
    }

    void SubmitStatus(const DataCodecStatusRecord& status) const {
        if (m_sinks.ui != nullptr) {
            m_sinks.ui->SubmitUiStatus(status);
        }
        if (m_sinks.console != nullptr) {
            m_sinks.console->SubmitConsoleStatus(status);
        }
    }

    void SubmitProgressRecord(const RunProgressRecord& record) const {
        const DataCodecProgressUpdate progress{
            .runId = record.runId,
            .phase = static_cast<DataCodecProgressPhase>(record.phase),
            .normalized = record.normalized,
            .success = record.success,
            .frameOrdinal = record.frameOrdinal,
            .frameCount = record.frameCount,
            .language = record.language,
            .messageId = record.messageId,
            .messageArguments = record.messageArguments,
            .text = record.text,
            .technicalDetail = record.technicalDetail,
        };
        if (m_sinks.progress != nullptr) {
            m_sinks.progress->SubmitProgress(progress);
        }
        auto statusText = FormatDataCodecProgressText(progress);
        if (!statusText.empty()) {
            SubmitStatus(DataCodecStatusRecord{
                .runId = record.runId,
                .severity = DataCodecStatusSeverity::Info,
                .language = record.language,
                .messageId = record.messageId,
                .messageArguments = record.messageArguments,
                .text = std::move(statusText),
                .technicalDetail = record.technicalDetail,
            });
        }
    }

    void SubmitMessageRecord(const RunMessageRecord& record) const {
        const auto& message = record.message;
        if (message.severity == TelemetryMessageSeverity::Info &&
            message.messageId == DataCodecMessageId::None) {
            return;
        }
        auto outputMessageId = message.messageId;
        auto outputText = message.text;
        auto outputTechnicalDetail = message.technicalDetail;
        if (outputMessageId == DataCodecMessageId::None &&
            record.runKind != TelemetryRunKind::Unknown &&
            message.severity != TelemetryMessageSeverity::Info) {
            if (outputTechnicalDetail.empty()) {
                outputTechnicalDetail = outputText;
            }
            const bool warning =
                message.severity == TelemetryMessageSeverity::Warning;
            if (record.runKind == TelemetryRunKind::Encode) {
                outputMessageId = warning
                    ? DataCodecMessageId::EncodeWarning
                    : DataCodecMessageId::EncodeFailed;
            } else {
                outputMessageId = warning
                    ? DataCodecMessageId::DecodeWarning
                    : DataCodecMessageId::DecodeFailed;
            }
            outputText = FormatDataCodecMessage(
                message.language,
                outputMessageId);
        }
        SubmitStatus(DataCodecStatusRecord{
            .runId = record.runId,
            .severity = StatusSeverity(message.severity),
            .language = message.language,
            .messageId = outputMessageId,
            .messageArguments = message.messageArguments,
            .text = std::move(outputText),
            .technicalDetail = std::move(outputTechnicalDetail),
        });
    }

    void SubmitArtifactRecord(const RunArtifactRecord& record) {
        if (m_sinks.reportFile == nullptr || record.artifact.text.empty()) {
            return;
        }
        const auto result = m_sinks.reportFile->WriteReportFile(DataCodecReportFile{
            .name = record.artifact.name,
            .mediaType = record.artifact.mediaType,
            .preferredExtension = record.artifact.preferredExtension,
            .content = record.artifact.text,
        });
        if (!result.success) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastReportError = result.error;
        }
    }

    DataCodecOutputSinks m_sinks;
    mutable std::mutex m_mutex;
    std::string m_lastReportError;
};

} // namespace datacodec

#endif
