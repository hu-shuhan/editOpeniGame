#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATURETELEMETRY_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATURETELEMETRY_H

#include <DataCodec/Log/Report/DataCodecProcessReportJson.h>
#include <DataCodec/Log/Telemetry/Sinks/TelemetrySessionSink.h>
#include <DataCodec/Runtime/Record/RunRecordEmitter.h>
#include <DataCodec/Test/Assertions/ProcessReportAssertions.h>
#include <DataCodec/Test/Common/DataCodecTestResult.h>

#include <cereal/external/rapidjson/document.h>

#include <string>
#include <utility>
#include <vector>

namespace datacodec::test::feature_telemetry {

using datacodec::test::Require;
using datacodec::test::TestResult;

inline TestResult TestInterestDrivenCollection() {
    TestResult result;
    TelemetrySessionSink statusSink;
    TelemetrySessionSink reportSink(
        kRunLifecycleRecordMask |
        RunRecordKind::Message |
        RunRecordKind::StageTiming |
        RunRecordKind::ResourceUsage |
        RunRecordKind::Artifact,
        RunCollectionBit(RunCollectionKind::MemoryTrace));

    Require(result, statusSink.Wants(RunRecordKind::Message),
            "telemetry.interest.statusMessage", "status sink should request messages");
    Require(result, !statusSink.Wants(RunRecordKind::StageTiming),
            "telemetry.interest.statusTiming", "status sink should not request stage timing");
    Require(result, !statusSink.Wants(RunRecordKind::ResourceUsage),
            "telemetry.interest.statusResource", "status sink should not request resource usage");
    Require(result, reportSink.Wants(RunRecordKind::StageTiming),
            "telemetry.interest.reportTiming", "report sink should request stage timing");
    Require(result, reportSink.Wants(RunRecordKind::ResourceUsage),
            "telemetry.interest.reportResource", "report sink should request resource usage");
    Require(result, reportSink.Requests(RunCollectionKind::MemoryTrace),
            "telemetry.interest.reportMemory", "report sink should request memory tracing");

    TelemetrySessionSink boundedSink(
        kRunLifecycleRecordMask |
        RunRecordKind::Progress |
        RunRecordKind::RemapOrder);
    Require(result, !boundedSink.Wants(RunRecordKind::Progress),
            "telemetry.interest.progress", "session sink should reject progress records");
    Require(result, !boundedSink.Wants(RunRecordKind::RemapOrder),
            "telemetry.interest.remap", "session sink should reject remap records");

    return result;
}

inline TestResult TestSessionCaptureAndProcessReport() {
    TestResult result;
    TelemetrySessionSink sessionSink(
        kRunLifecycleRecordMask |
        RunRecordKind::Message |
        RunRecordKind::StageTiming |
        RunRecordKind::ResourceUsage |
        RunRecordKind::Artifact);
    RunRecordEmitter records;
    records.Reset(
        RunRecordInfo{
            .generatedAtUtc = "2026-01-01T00:00:00Z",
            .runKind = TelemetryRunKind::Encode,
            .objectName = "telemetry-test",
            .leafPath = "/0",
            .meshType = "SurfaceMesh",
        },
        &sessionSink);
    records.BeginRun();
    records.AddInfo("TelemetryTest", "message");
    records.RecordStageTiming("TopoStage", 3.5, TelemetryStageCategory::Topology);
    records.RecordResourceUsage("AttrStage", 4096u, TelemetryStageCategory::Attribute);
    records.AddArtifact(TelemetryArtifactRecord{
        .name = "diagnostic_note",
        .mediaType = "text/plain",
        .preferredExtension = ".txt",
        .text = "telemetry artifact",
    });
    records.EndRun(RunEndRecord{
        .success = true,
        .elapsedMs = 8.0,
        .inputBytes = 100u,
        .outputBytes = 40u,
        .sourceBytes = 100u,
    });

    const auto emittedMessages = records.TakeMessages();
    Require(result, emittedMessages.size() == 1u,
            "telemetry.messages.emitter", "run emitter message count mismatch");
    const auto capturedMessages = sessionSink.SnapshotMessages();
    Require(result, capturedMessages.size() == 1u,
            "telemetry.messages.snapshot", "session message snapshot count mismatch");
    const auto session = sessionSink.TakeSession(records.RunId());
    Require(result, session.has_value(),
            "telemetry.session.present", "completed run should produce a session");
    if (session.has_value()) {
        const std::vector<TelemetrySession> sessions{*session};
        Require(result, session->messages.size() == 1u,
                "telemetry.session.messages", "session message count mismatch");
        Require(result, session->stages.size() == 2u,
                "telemetry.session.stages", "session stage count mismatch");
        Require(result, session->artifacts.size() == 1u,
                "telemetry.session.artifacts", "session artifact count mismatch");
        Require(result, HasSerializableDataCodecProcessReport(sessions),
                "telemetry.processReport.sharedAssertion",
                "captured session should produce a valid process report");
        auto processNodes = BuildTelemetryProcessNodes(
            sessions,
            TelemetryRunKind::Encode);
        DataCodecProcessReport processReport{
            .operation = TelemetryRunKind::Encode,
            .generatedAtUtc = session->generatedAtUtc,
            .objectName = session->objectName,
            .success = session->success,
            .elapsedMs = session->elapsedMs,
            .inputBytes = session->inputBytes,
            .outputBytes = session->outputBytes,
            .processes = std::move(processNodes),
        };
        CompleteDataCodecProcessReportMemory(processReport);
        const auto json = SerializeDataCodecProcessReportJson(processReport);
        rapidjson::Document document;
        document.Parse(json.data(), json.size());
        Require(result, !document.HasParseError() && document.IsObject(),
                "telemetry.processReport.validJson", "process report should be valid json");
        Require(result, json.find("telemetry-test") != std::string::npos,
                "telemetry.processReport.object", "process report should contain the object name");
        Require(result, json.find("\"Topology\"") != std::string::npos,
                "telemetry.processReport.stage", "process report should contain grouped stage timing");
        Require(result, json.find("diagnostic_note") == std::string::npos &&
                json.find("telemetry artifact") == std::string::npos,
                "telemetry.processReport.artifact",
                "process report should not expose raw telemetry artifacts");
    }

    return result;
}

inline TestResult TestStageCategoryResolution() {
    TestResult result;
    Require(result, ResolveTelemetryStageCategory("ParamsEncodeStage") == TelemetryStageCategory::Params,
            "telemetry.category.params", "params stage category mismatch");
    Require(result, ResolveTelemetryStageCategory("GeometryStage") == TelemetryStageCategory::Geometry,
            "telemetry.category.geometry", "geometry stage category mismatch");
    Require(result, ResolveTelemetryStageCategory("TopoDecodeStage") == TelemetryStageCategory::Topology,
            "telemetry.category.topology", "topology stage category mismatch");
    Require(result, ResolveTelemetryStageCategory("PointAttributeStage") == TelemetryStageCategory::Attribute,
            "telemetry.category.attribute", "attribute stage category mismatch");
    Require(result, ResolveTelemetryStageCategory("CellSpatialPartition.Morton") == TelemetryStageCategory::Remap,
            "telemetry.category.remap", "remap stage category mismatch");
    Require(result, ResolveTelemetryStageCategory("DecodeCommitStage") == TelemetryStageCategory::Commit,
            "telemetry.category.commit", "commit stage category mismatch");

    return result;
}

} // namespace datacodec::test::feature_telemetry

namespace datacodec::test {

inline TestResult RunDataCodecFeatureTelemetry() {
    auto result = feature_telemetry::TestInterestDrivenCollection();
    const auto appendResult = [&result](const TestResult& addition) {
        if (!addition.passed) {
            result.passed = false;
            result.failures.insert(
                result.failures.end(),
                addition.failures.begin(),
                addition.failures.end());
        }
        result.AppendDiagnostics(addition.diagnostics);
    };
    appendResult(feature_telemetry::TestSessionCaptureAndProcessReport());
    appendResult(feature_telemetry::TestStageCategoryResolution());
    return result;
}

} // namespace datacodec::test

#endif
