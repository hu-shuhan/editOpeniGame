#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREOUTPUTSINKS_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREOUTPUTSINKS_H

#include "DataCodec/Runtime/Output/DataCodecOutputRouter.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <memory>
#include <utility>
#include <vector>

namespace datacodec::test {

class CapturingUiOutputSink final : public IDataCodecUiSink {
public:
    void SubmitUiStatus(const DataCodecStatusRecord& status) override {
        statuses.push_back(status);
    }

    std::vector<DataCodecStatusRecord> statuses;
};

class CapturingConsoleOutputSink final : public IDataCodecConsoleSink {
public:
    void SubmitConsoleStatus(const DataCodecStatusRecord& status) override {
        statuses.push_back(status);
    }

    std::vector<DataCodecStatusRecord> statuses;
};

class CapturingProgressOutputSink final : public IDataCodecProgressSink {
public:
    void SubmitProgress(const DataCodecProgressUpdate& progress) override {
        updates.push_back(progress);
    }

    std::vector<DataCodecProgressUpdate> updates;
};

class CapturingReportFileOutputSink final : public IDataCodecReportFileSink {
public:
    [[nodiscard]] DataCodecReportWriteResult WriteReportFile(
        const DataCodecReportFile& report) override {
        reports.push_back(report);
        return {.success = true, .path = report.name + report.preferredExtension};
    }

    std::vector<DataCodecReportFile> reports;
};

[[nodiscard]] inline TestResult RunDataCodecFeatureOutputSinks() {
    TestResult result;
    auto ui = std::make_shared<CapturingUiOutputSink>();
    auto console = std::make_shared<CapturingConsoleOutputSink>();
    auto progress = std::make_shared<CapturingProgressOutputSink>();
    auto reportFile = std::make_shared<CapturingReportFileOutputSink>();
    DataCodecOutputRouter router(DataCodecOutputSinks{
        .ui = ui,
        .console = console,
        .progress = progress,
        .reportFile = reportFile,
    });

    router.Submit(RunRecord{RunProgressRecord{
        .runId = 1u,
        .phase = RunProgressPhase::Update,
        .normalized = 0.5,
        .language = DataCodecLanguage::English,
        .messageId = DataCodecMessageId::EncodeGeometry,
        .text = "Compressing coordinates",
    }});
    Require(
        result,
        progress->updates.size() == 1u && ui->statuses.size() == 1u &&
            console->statuses.size() == 1u && reportFile->reports.empty(),
        "outputSinks.progressRouting",
        "progress should update the progress destination and publish its status text");

    router.Submit(RunRecord{RunMessageRecord{
        .runId = 1u,
        .message = TelemetryMessageRecord{
            .severity = TelemetryMessageSeverity::Warning,
            .language = DataCodecLanguage::English,
            .messageId = DataCodecMessageId::CacheHit,
            .text = "Cache hit",
        },
    }});
    Require(
        result,
        progress->updates.size() == 1u && ui->statuses.size() == 2u &&
            console->statuses.size() == 2u && reportFile->reports.empty(),
        "outputSinks.statusRouting",
        "status messages should only reach UI and console destinations");
    Require(
        result,
        ui->statuses.back().severity == DataCodecStatusSeverity::Warning &&
            console->statuses.back().severity == DataCodecStatusSeverity::Warning,
        "outputSinks.severityMapping",
        "internal telemetry severity should map to public output severity");

    router.Submit(RunRecord{RunMessageRecord{
        .runId = 2u,
        .runKind = TelemetryRunKind::Decode,
        .message = TelemetryMessageRecord{
            .severity = TelemetryMessageSeverity::Error,
            .language = DataCodecLanguage::SimplifiedChinese,
            .text = "decoded payload is malformed",
        },
    }});
    Require(
        result,
        ui->statuses.size() == 3u &&
            ui->statuses.back().messageId == DataCodecMessageId::DecodeFailed &&
            ui->statuses.back().text == "解压失败" &&
            ui->statuses.back().technicalDetail == "decoded payload is malformed",
        "outputSinks.codecErrorLocalization",
        "DataCodec failures should localize the user text and preserve technical detail");
    Require(
        result,
        FormatDataCodecStatusText(ui->statuses.back()) ==
            "解压失败：decoded payload is malformed",
        "outputSinks.sharedStatusFormatting",
        "UI and console destinations should share one status text formatter");

    Require(
        result,
        FormatDataCodecStatusText(DataCodecStatusRecord{
            .language = DataCodecLanguage::English,
            .messageId = DataCodecMessageId::EncodeGeometry,
        }) == "Compressing coordinates",
        "outputSinks.statusKeyFormatting",
        "status formatting should resolve an empty text field from its message key");
    Require(
        result,
        FormatDataCodecProgressText(DataCodecProgressUpdate{
            .frameOrdinal = 1u,
            .frameCount = 4u,
            .language = DataCodecLanguage::SimplifiedChinese,
            .messageId = DataCodecMessageId::DecodeTopology,
        }) == "第 2/4 帧：解码拓扑",
        "outputSinks.progressKeyFormatting",
        "progress formatting should resolve message keys and frame parameters once");

    router.Submit(RunRecord{RunArtifactRecord{
        .runId = 1u,
        .artifact = TelemetryArtifactRecord{
            .name = "telemetry",
            .mediaType = "application/json",
            .preferredExtension = ".json",
            .text = "{}",
        },
    }});
    Require(
        result,
        progress->updates.size() == 1u && ui->statuses.size() == 3u &&
            console->statuses.size() == 3u && reportFile->reports.size() == 1u,
        "outputSinks.reportRouting",
        "artifacts should only reach the report file destination");

    return result;
}

} // 命名空间 datacodec::test

#endif
