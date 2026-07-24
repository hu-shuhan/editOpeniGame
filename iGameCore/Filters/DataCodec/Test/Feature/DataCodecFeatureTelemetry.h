#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATURETELEMETRY_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATURETELEMETRY_H

#include <DataCodec/Log/Telemetry/Sinks/JsonTelemetrySink.h>
#include <DataCodec/Log/Telemetry/Sinks/TelemetrySessionSink.h>
#include <DataCodec/Runtime/Record/RunRecordEmitter.h>
#include <DataCodec/Test/Common/DataCodecTestResult.h>

#include <iostream>
#include <string>

namespace datacodec::test::feature_telemetry {

using datacodec::test::Require;
using datacodec::test::TestResult;

inline void PrintResult(const TestResult& result) {
    for (const auto& failure : result.failures) {
        std::cerr << failure.check << ": " << failure.message << '\n';
    }
}

inline bool TestInterestDrivenCollection() {
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

    PrintResult(result);
    return result.passed;
}

inline bool TestSessionAndJsonSink() {
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
        .name = "memory_trace",
        .mediaType = "text/csv",
        .preferredExtension = ".csv",
        .text = "elapsedMs,event\n0,begin\n",
    });
    records.EndRun(RunEndRecord{
        .success = true,
        .elapsedMs = 8.0,
        .inputBytes = 100u,
        .outputBytes = 40u,
        .sourceBytes = 100u,
    });

    const auto session = sessionSink.TakeSession(records.RunId());
    Require(result, session.has_value(),
            "telemetry.session.present", "completed run should produce a session");
    if (session.has_value()) {
        Require(result, session->messages.size() == 1u,
                "telemetry.session.messages", "session message count mismatch");
        Require(result, session->stages.size() == 2u,
                "telemetry.session.stages", "session stage count mismatch");
        Require(result, session->artifacts.size() == 1u,
                "telemetry.session.artifacts", "session artifact count mismatch");
        const auto json = SerializeTelemetrySessionJson(*session);
        Require(result, json.find("telemetry-test") != std::string::npos,
                "telemetry.json.object", "json should contain the object name");
        Require(result, json.find("TopoStage") != std::string::npos,
                "telemetry.json.stage", "json should contain stage timing");
        Require(result, json.find("message") != std::string::npos,
                "telemetry.json.message", "json should contain messages");
        Require(result, json.find("memory_trace") != std::string::npos,
                "telemetry.json.artifact", "json should contain artifact metadata");
    }

    PrintResult(result);
    return result.passed;
}

inline bool TestStageCategoryResolution() {
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

    PrintResult(result);
    return result.passed;
}

} // namespace datacodec::test::feature_telemetry

namespace datacodec::test {

inline int RunDataCodecFeatureTelemetry() {
    if (!feature_telemetry::TestInterestDrivenCollection() ||
        !feature_telemetry::TestSessionAndJsonSink() ||
        !feature_telemetry::TestStageCategoryResolution()) {
        return 1;
    }
    std::cout << "DataCodec telemetry feature tests passed\n";
    return 0;
}

} // namespace datacodec::test

#endif
