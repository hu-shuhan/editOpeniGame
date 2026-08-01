#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREOUTPUTSINKS_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREOUTPUTSINKS_H

#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Log/Report/DataCodecProcessReportJson.h"
#include "DataCodec/Log/Telemetry/Sinks/TelemetrySessionSink.h"
#include "DataCodec/Runtime/Output/DataCodecOutputRouter.h"
#include "DataCodec/Runtime/Record/RunRecordEmitter.h"
#include "DataCodec/Test/Common/DataCodecTestResult.h"

#include <cereal/external/rapidjson/document.h>

#include <cmath>
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

    const auto defaultDecodeConfiguration = MakeDefaultDecodeConfigurationParams();
    Require(
        result,
        !defaultDecodeConfiguration.logging.enableFileLog &&
            defaultDecodeConfiguration.logging.enableConsoleLog,
        "outputSinks.decodeLoggingDefaults",
        "decode logging defaults should keep file output off and console output on");
    const auto configuredDecodeLogging = MakeDecodeConfigurationParams(
        DataCodecDecodeOptions{
            .logging = DataCodecDecodeLogParams{
                .enableFileLog = true,
                .enableConsoleLog = false,
            },
        });
    Require(
        result,
        configuredDecodeLogging.logging.enableFileLog &&
            !configuredDecodeLogging.logging.enableConsoleLog,
        "outputSinks.decodeLoggingPropagation",
        "decode logging options should propagate into the reader configuration");

    TelemetrySession packageSession{
        .runId = 10u,
        .runKind = TelemetryRunKind::Decode,
        .objectName = "Package",
        .meshType = "Package",
        .success = true,
        .elapsedMs = 12.0,
        .inputBytes = 1024u,
    };
    TelemetrySession leafSession{
        .runId = 11u,
        .parentRunId = 10u,
        .runKind = TelemetryRunKind::Decode,
        .leafPath = "/root",
        .success = true,
        .elapsedMs = 10.0,
        .inputBytes = 1000u,
        .outputBytes = 4096u,
    };
    for (std::uint64_t index = 0u; index < 32u; ++index) {
        leafSession.stages.push_back({
            .name = "AttrDecodeStage[" + std::to_string(index) + "]",
            .order = index,
            .elapsedMs = 0.25,
            .category = TelemetryStageCategory::General,
        });
    }
    leafSession.stages.push_back({
        .name = "AttrDecodeStage",
        .order = 32u,
        .elapsedMs = 8.0,
        .category = TelemetryStageCategory::Attribute,
    });
    leafSession.stages.push_back({
        .name = "memory.attribute",
        .order = 33u,
        .category = TelemetryStageCategory::Attribute,
        .resource = TelemetryResourceUsage{
            .valid = true,
            .workingSetBytes = 1800u,
            .workingSetBeforeBytes = 1200u,
            .workingSetAfterBytes = 1500u,
            .peakWorkingSetBytes = 1800u,
        },
    });
    const std::vector<TelemetrySession> processSessions{
        packageSession,
        leafSession,
    };
    auto processNodes = BuildTelemetryProcessNodes(
        processSessions,
        TelemetryRunKind::Decode);
    Require(
        result,
        processNodes.size() == 1u && processNodes.front().name == "Package" &&
            processNodes.front().children.size() == 1u &&
            processNodes.front().children.front().name == "Leaf: /root",
        "outputSinks.processTreeHierarchy",
        "process report should preserve package and leaf hierarchy in one tree");
    DataCodecProcessReport processReport{
        .operation = TelemetryRunKind::Decode,
        .generatedAtUtc = "2026-01-01T00:00:00Z",
        .objectName = "fixture.igc",
        .success = true,
        .elapsedMs = 12.0,
        .inputBytes = 1024u,
        .outputBytes = 4096u,
        .details = {
            {"smallScientificMetric", 1.0e-12},
            {"numericItemCount", std::uint64_t{7u}},
        },
        .processes = std::move(processNodes),
    };
    CompleteDataCodecProcessReportMemory(processReport);
    const auto processJson = SerializeDataCodecProcessReportJson(processReport);
    rapidjson::Document processDocument;
    processDocument.Parse(processJson.data(), processJson.size());
    const auto hasTypedDetails = !processDocument.HasParseError() &&
        processDocument.HasMember("details") &&
        processDocument["details"].IsObject() &&
        processDocument["details"].HasMember("smallScientificMetric") &&
        processDocument["details"]["smallScientificMetric"].IsDouble() &&
        std::abs(
            processDocument["details"]["smallScientificMetric"].GetDouble() -
            1.0e-12) < 1.0e-18 &&
        processDocument["details"].HasMember("numericItemCount") &&
        processDocument["details"]["numericItemCount"].IsUint64();
    Require(
        result,
        processJson.find("\"Attributes\"") != std::string::npos &&
            processJson.find("\"measuredTaskCount\": 33") != std::string::npos &&
            processJson.find("\"longestMeasuredTaskMs\": 8.0") != std::string::npos &&
            processJson.find("\"peakWorkingSetBytes\": 1800") != std::string::npos &&
            processJson.find("AttrDecodeStage[") == std::string::npos &&
            hasTypedDetails,
        "outputSinks.processTreeAggregation",
        "process report should label measured task timing, retain peak memory, and preserve numeric JSON values");

    const DataCodecProcessReport compressionReport{
        .operation = TelemetryRunKind::Encode,
        .generatedAtUtc = "2026-01-01T00:00:00Z",
        .objectName = "compression.igc",
        .success = true,
        .inputBytes = 4000u,
        .outputBytes = 1000u,
        .summaryNote = "compressed output size divided by source file size",
    };
    const auto compressionJson = SerializeDataCodecProcessReportJson(compressionReport);
    rapidjson::Document compressionDocument;
    compressionDocument.Parse(compressionJson.data(), compressionJson.size());
    bool hasCompressionStatistics = !compressionDocument.HasParseError() &&
        compressionDocument.IsObject() && compressionDocument.HasMember("summary") &&
        compressionDocument["summary"].IsObject();
    if (hasCompressionStatistics) {
        const auto& summary = compressionDocument["summary"];
        hasCompressionStatistics =
            summary.HasMember("sourceFileSizeBytes") &&
            summary["sourceFileSizeBytes"].IsUint64() &&
            summary["sourceFileSizeBytes"].GetUint64() == 4000u &&
            summary.HasMember("compressedOutputSizeBytes") &&
            summary["compressedOutputSizeBytes"].IsUint64() &&
            summary["compressedOutputSizeBytes"].GetUint64() == 1000u &&
            summary.HasMember("compressionRatio") &&
            summary["compressionRatio"].IsDouble() &&
            std::abs(summary["compressionRatio"].GetDouble() - 0.25) < 1.0e-12 &&
            summary.HasMember("note") && summary["note"].IsString() &&
            std::string(summary["note"].GetString()) ==
                "compressed output size divided by source file size";
    }
    Require(
        result,
        hasCompressionStatistics,
        "outputSinks.encodeCompressionStatistics",
        "encode process report should expose source size, compressed size, decimal ratio, and calculation note");

    DataCodecProcessReport workflowMemoryReport{
        .operation = TelemetryRunKind::Encode,
        .generatedAtUtc = "2026-01-01T00:00:00Z",
        .objectName = "memory.igc",
        .success = true,
        .processes = {
            DataCodecProcessNode{
                .name = "EncodeData",
                .memory = DataCodecProcessMemory{
                    .valid = true,
                    .beforeWorkingSetBytes = 15000u,
                    .afterWorkingSetBytes = 19000u,
                    .peakWorkingSetBytes = 20000u,
                },
            },
            DataCodecProcessNode{
                .name = "Verification",
                .memory = DataCodecProcessMemory{
                    .valid = true,
                    .beforeWorkingSetBytes = 3000u,
                    .afterWorkingSetBytes = 3500u,
                    .peakWorkingSetBytes = 17000u,
                },
            },
        },
    };
    CompleteDataCodecProcessReportMemory(workflowMemoryReport);
    Require(
        result,
        workflowMemoryReport.memory.beforeWorkingSetBytes == 15000u &&
            workflowMemoryReport.memory.afterWorkingSetBytes == 3500u &&
            workflowMemoryReport.memory.peakWorkingSetBytes == 20000u,
        "outputSinks.processMemoryLifecycle",
        "process report memory should preserve workflow start, workflow end, and global peak");
    auto runningReport = processReport;
    runningReport.completed = false;
    const auto runningJson = SerializeDataCodecProcessReportJson(runningReport);
    Require(
        result,
        runningJson.find("\"status\": \"running\"") != std::string::npos,
        "outputSinks.runningProcessReport",
        "process report should expose a running state before completion");

    auto failedSessions = processSessions;
    failedSessions.back().success = false;
    failedSessions.back().messages.push_back({
        .severity = TelemetryMessageSeverity::Error,
        .origin = "AttributeDecode",
        .code = "decode.attribute.invalid",
        .text = "attribute payload is invalid",
        .technicalDetail = "payload range exceeds the package",
    });
    const auto errorReport = BuildDataCodecErrorReport(
        TelemetryRunKind::Decode,
        "2026-01-01T00:00:00Z",
        "fixture.igc",
        failedSessions,
        {},
        "decode failed");
    const auto errorJson = SerializeDataCodecErrorReportJson(errorReport);
    Require(
        result,
        errorReport.errors.size() == 1u &&
            errorJson.find("attribute payload is invalid") != std::string::npos &&
            errorJson.find("Leaf: /root") != std::string::npos,
        "outputSinks.errorReport",
        "error report should contain only actionable failures with their phase path");

    TelemetrySessionSink summarySink(
        kRunLifecycleRecordMask |
            RunRecordKind::StageTiming |
            RunRecordKind::ResourceUsage |
            RunRecordKind::Artifact,
        0u,
        TelemetrySessionDetail::ProcessSummary);
    RunRecordEmitter summaryRecords;
    summaryRecords.Reset(
        RunRecordInfo{
            .runKind = TelemetryRunKind::Decode,
            .objectName = "summary-filter",
        },
        &summarySink);
    summaryRecords.BeginRun();
    summaryRecords.RecordStageTiming(
        "AttrDecodeStage[0]",
        1.0,
        TelemetryStageCategory::General);
    summaryRecords.RecordStageTiming(
        "AttrDecodeStage",
        2.0,
        TelemetryStageCategory::Attribute);
    summaryRecords.RecordResourceUsage("bytestore.logical_bytes", 4096u);
    summaryRecords.RecordResourceUsage(
        "memory.attribute",
        TelemetryResourceUsage{
            .valid = true,
            .workingSetBytes = 2048u,
            .peakWorkingSetBytes = 2048u,
        },
        TelemetryStageCategory::Attribute);
    summaryRecords.AddArtifact({
        .name = "raw_detail",
        .mediaType = "text/plain",
        .preferredExtension = ".txt",
        .text = "raw",
    });
    summaryRecords.EndRun({.success = true});
    const auto summarySession = summarySink.TakeSession(summaryRecords.RunId());
    const auto hasRawStage = summarySession.has_value() && std::any_of(
        summarySession->stages.begin(),
        summarySession->stages.end(),
        [](const auto& stage) { return stage.name == "AttrDecodeStage[0]"; });
    const auto foldedStage = summarySession.has_value()
        ? std::find_if(
              summarySession->stages.begin(),
              summarySession->stages.end(),
              [](const auto& stage) { return stage.name == "folded.attribute"; })
        : std::vector<TelemetryStageRecord>::const_iterator{};
    Require(
        result,
        summarySession.has_value() && summarySession->stages.size() == 3u &&
            !hasRawStage && foldedStage != summarySession->stages.end() &&
            foldedStage->sampleCount == 1u && summarySession->artifacts.empty(),
        "outputSinks.processSummaryCapture",
        "process summary capture should drop per-item stages and raw artifacts");

    return result;
}

} // 命名空间 datacodec::test

#endif
