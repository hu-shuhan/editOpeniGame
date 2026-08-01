#include "DataCodec/Log/Report/DataCodecProcessReportJson.h"

#include <cereal/external/rapidjson/prettywriter.h>
#include <cereal/external/rapidjson/stringbuffer.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>

namespace datacodec {
namespace {

using ProcessReportJsonBuffer = rapidjson::StringBuffer;
using ProcessReportJsonWriter = rapidjson::PrettyWriter<ProcessReportJsonBuffer>;

void WriteJsonString(
        ProcessReportJsonWriter& writer,
        const std::string& value) {
    writer.String(
        value.data(),
        static_cast<rapidjson::SizeType>(value.size()));
}

void WriteJsonKey(
        ProcessReportJsonWriter& writer,
        const std::string& value) {
    writer.Key(
        value.data(),
        static_cast<rapidjson::SizeType>(value.size()));
}

void WriteProcessMemoryJson(
        ProcessReportJsonWriter& writer,
        const DataCodecProcessMemory& memory) {
    writer.StartObject();
    if (memory.beforeWorkingSetBytes > 0u) {
        writer.Key("beforeWorkingSetBytes");
        writer.Uint64(memory.beforeWorkingSetBytes);
    }
    if (memory.afterWorkingSetBytes > 0u) {
        writer.Key("afterWorkingSetBytes");
        writer.Uint64(memory.afterWorkingSetBytes);
    }
    if (memory.peakWorkingSetBytes > 0u) {
        writer.Key("peakWorkingSetBytes");
        writer.Uint64(memory.peakWorkingSetBytes);
    }
    writer.EndObject();
}

void WriteProcessDetailValueJson(
        ProcessReportJsonWriter& writer,
        const DataCodecProcessDetail::Value& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        WriteJsonString(writer, *text);
        return;
    }
    if (const auto* integer = std::get_if<std::uint64_t>(&value)) {
        writer.Uint64(*integer);
        return;
    }
    if (const auto* number = std::get_if<double>(&value)) {
        if (std::isfinite(*number)) {
            writer.Double(*number);
        } else {
            writer.Null();
        }
        return;
    }
    writer.Bool(std::get<bool>(value));
}

void WriteProcessDetailsJson(
        ProcessReportJsonWriter& writer,
        const std::vector<DataCodecProcessDetail>& details) {
    writer.StartObject();
    for (const auto& detail: details) {
        WriteJsonKey(writer, detail.name);
        WriteProcessDetailValueJson(writer, detail.value);
    }
    writer.EndObject();
}

void WriteProcessNodeJson(
        ProcessReportJsonWriter& writer,
        const DataCodecProcessNode& node) {
    writer.StartObject();
    writer.Key("name");
    WriteJsonString(writer, node.name);
    writer.Key("status");
    writer.String(!node.completed ? "running" : node.success ? "success" : "failed");
    if (node.elapsedMsValid) {
        writer.Key("elapsedMs");
        writer.Double(node.elapsedMs);
    }
    if (node.inputBytes > 0u) {
        writer.Key("inputBytes");
        writer.Uint64(node.inputBytes);
    }
    if (node.outputBytes > 0u) {
        writer.Key("outputBytes");
        writer.Uint64(node.outputBytes);
    }
    if (node.measuredTaskCount > 0u) {
        writer.Key("taskTiming");
        writer.StartObject();
        writer.Key("measuredTaskCount");
        writer.Uint64(node.measuredTaskCount);
        writer.Key("longestMeasuredTaskMs");
        writer.Double(node.longestMeasuredTaskMs);
        writer.EndObject();
    }
    if (node.memory.valid) {
        writer.Key("memory");
        WriteProcessMemoryJson(writer, node.memory);
    }
    if (!node.details.empty()) {
        writer.Key("details");
        WriteProcessDetailsJson(writer, node.details);
    }
    if (!node.children.empty()) {
        writer.Key("children");
        writer.StartArray();
        for (const auto& child: node.children) {
            WriteProcessNodeJson(writer, child);
        }
        writer.EndArray();
    }
    writer.EndObject();
}

std::string FinishJson(ProcessReportJsonBuffer& buffer) {
    auto output = std::string(buffer.GetString(), buffer.GetSize());
    output.push_back('\n');
    return output;
}

} // 匿名命名空间

std::string MakeDataCodecReportFileTimestampUtc() {
    const auto now = std::chrono::system_clock::now();
    const auto wholeSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto milliseconds = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - wholeSeconds).count());
    const auto time = std::chrono::system_clock::to_time_t(wholeSeconds);
    std::tm utcTime{};
#if defined(_WIN32)
    (void)gmtime_s(&utcTime, &time);
#else
    (void)gmtime_r(&time, &utcTime);
#endif
    char timestamp[32]{};
    std::snprintf(
        timestamp,
        sizeof(timestamp),
        "%04d%02d%02d_%02d%02d%02d_%03dZ",
        utcTime.tm_year + 1900,
        utcTime.tm_mon + 1,
        utcTime.tm_mday,
        utcTime.tm_hour,
        utcTime.tm_min,
        utcTime.tm_sec,
        milliseconds);
    return timestamp;
}

std::string SerializeDataCodecProcessReportJson(
        const DataCodecProcessReport& report) {
    ProcessReportJsonBuffer buffer;
    ProcessReportJsonWriter writer(buffer);
    writer.SetIndent(' ', 2u);
    writer.StartObject();
    writer.Key("operation");
    WriteJsonString(writer, std::string(TelemetryRunKindName(report.operation)));
    writer.Key("generatedAtUtc");
    WriteJsonString(writer, report.generatedAtUtc);
    writer.Key("objectName");
    WriteJsonString(writer, report.objectName);
    writer.Key("status");
    writer.String(!report.completed ? "running" : report.success ? "success" : "failed");
    writer.Key("summary");
    writer.StartObject();
    writer.Key("elapsedMs");
    writer.Double(report.elapsedMs);
    writer.Key("inputBytes");
    writer.Uint64(report.inputBytes);
    writer.Key("outputBytes");
    writer.Uint64(report.outputBytes);
    if (report.inputBytes > 0u && report.outputBytes > 0u) {
        writer.Key("outputToInputRatio");
        writer.Double(
            static_cast<double>(report.outputBytes) /
            static_cast<double>(report.inputBytes));
    }
    if (report.memory.valid) {
        writer.Key("memory");
        WriteProcessMemoryJson(writer, report.memory);
    }
    writer.EndObject();
    if (!report.details.empty()) {
        writer.Key("details");
        WriteProcessDetailsJson(writer, report.details);
    }
    writer.Key("processes");
    writer.StartArray();
    for (const auto& process: report.processes) {
        WriteProcessNodeJson(writer, process);
    }
    writer.EndArray();
    writer.EndObject();
    return FinishJson(buffer);
}

std::string SerializeDataCodecErrorReportJson(
        const DataCodecErrorReport& report) {
    ProcessReportJsonBuffer buffer;
    ProcessReportJsonWriter writer(buffer);
    writer.SetIndent(' ', 2u);
    writer.StartObject();
    writer.Key("operation");
    WriteJsonString(writer, std::string(TelemetryRunKindName(report.operation)));
    writer.Key("generatedAtUtc");
    WriteJsonString(writer, report.generatedAtUtc);
    writer.Key("objectName");
    WriteJsonString(writer, report.objectName);
    writer.Key("status");
    writer.String("failed");
    writer.Key("errors");
    writer.StartArray();
    for (const auto& error: report.errors) {
        writer.StartObject();
        writer.Key("phasePath");
        writer.StartArray();
        for (const auto& phase: error.phasePath) {
            WriteJsonString(writer, phase);
        }
        writer.EndArray();
        if (!error.origin.empty()) {
            writer.Key("origin");
            WriteJsonString(writer, error.origin);
        }
        if (!error.code.empty()) {
            writer.Key("code");
            WriteJsonString(writer, error.code);
        }
        writer.Key("message");
        WriteJsonString(writer, error.message);
        if (!error.technicalDetail.empty()) {
            writer.Key("technicalDetail");
            WriteJsonString(writer, error.technicalDetail);
        }
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
    return FinishJson(buffer);
}

} // 命名空间 datacodec
