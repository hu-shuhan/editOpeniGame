#ifndef DATACODEC_TEST_ASSERTIONS_PROCESSREPORTASSERTIONS_H
#define DATACODEC_TEST_ASSERTIONS_PROCESSREPORTASSERTIONS_H

#include "DataCodec/Log/Report/DataCodecProcessReportJson.h"

#include <cereal/external/rapidjson/document.h>

#include <algorithm>
#include <vector>

namespace datacodec::test {

[[nodiscard]] inline bool HasSerializableDataCodecProcessReport(
        const std::vector<TelemetrySession>& sessions) {
    if (sessions.empty()) {
        return false;
    }
    const auto operation = sessions.front().runKind;
    auto processes = BuildTelemetryProcessNodes(sessions, operation);
    if (processes.empty()) {
        return false;
    }

    DataCodecProcessReport report{
        .operation = operation,
        .generatedAtUtc = sessions.front().generatedAtUtc,
        .objectName = sessions.front().objectName,
        .success = true,
        .processes = std::move(processes),
    };
    for (const auto& session : sessions) {
        if (session.runKind != operation) {
            continue;
        }
        report.success = report.success && session.success;
        report.elapsedMs = std::max(report.elapsedMs, session.elapsedMs);
        report.inputBytes = std::max(report.inputBytes, session.inputBytes);
        report.outputBytes = std::max(report.outputBytes, session.outputBytes);
    }
    CompleteDataCodecProcessReportMemory(report);

    const auto json = SerializeDataCodecProcessReportJson(report);
    rapidjson::Document document;
    document.Parse(json.data(), json.size());
    return !document.HasParseError() && document.IsObject() &&
        document.HasMember("operation") && document["operation"].IsString() &&
        document.HasMember("summary") && document["summary"].IsObject() &&
        document.HasMember("processes") && document["processes"].IsArray() &&
        !document["processes"].Empty();
}

} // 命名空间 datacodec::test

#endif
