#ifndef DATACODEC_LOG_TELEMETRY_SINKS_JSONTELEMETRYSINK_H
#define DATACODEC_LOG_TELEMETRY_SINKS_JSONTELEMETRYSINK_H

#include "DataCodec/Log/Telemetry/TelemetrySession.h"

#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
namespace datacodec {

inline void AppendIndent(std::string& output, const int indent) {
    output.append(static_cast<std::size_t>(std::max(indent, 0)) * 2u, ' ');
}

inline void AppendEscapedString(std::string& output, const std::string& value) {
    output.push_back('"');
    for (const unsigned char ch : value) {
        switch (ch) {
            case '\\':
                output += "\\\\";
                break;
            case '"':
                output += "\\\"";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                if (ch < 0x20u) {
                    std::ostringstream stream;
                    stream.imbue(std::locale::classic());
                    stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
                    output += stream.str();
                } else {
                    output.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    output.push_back('"');
}

inline void AppendBool(std::string& output, const bool value) {
    output += value ? "true" : "false";
}

inline void AppendNumber(std::string& output, const std::uint64_t value) {
    output += std::to_string(value);
}

inline void AppendNumber(std::string& output, const double value) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(6) << value;
    output += stream.str();
}

template <typename FillFields>
void WriteObject(std::string& output, const int indent, FillFields&& fillFields) {
    output += "{";
    bool first = true;
    auto field = [&](const std::string& name, auto&& writeValue) {
        if (first) {
            output += "\n";
            first = false;
        } else {
            output += ",\n";
        }
        AppendIndent(output, indent + 1);
        AppendEscapedString(output, name);
        output += ": ";
        writeValue();
    };

    fillFields(field);

    if (!first) {
        output += "\n";
        AppendIndent(output, indent);
    }
    output += "}";
}

template <typename FillItems>
void WriteArray(std::string& output, const int indent, FillItems&& fillItems) {
    output += "[";
    bool first = true;
    auto item = [&](auto&& writeValue) {
        if (first) {
            output += "\n";
            first = false;
        } else {
            output += ",\n";
        }
        AppendIndent(output, indent + 1);
        writeValue();
    };

    fillItems(item);

    if (!first) {
        output += "\n";
        AppendIndent(output, indent);
    }
    output += "]";
}

[[nodiscard]] inline std::string SerializeTelemetrySessionJson(
    const TelemetrySession& session) {
    std::string output;
    WriteObject(output, 0, [&](auto&& field) {
        field("runId", [&]() { AppendNumber(output, session.runId); });
        field("parentRunId", [&]() { AppendNumber(output, session.parentRunId); });
        field("generatedAtUtc", [&]() { AppendEscapedString(output, session.generatedAtUtc); });
        field("runKind", [&]() { AppendEscapedString(output, TelemetryRunKindName(session.runKind)); });
        field("objectName", [&]() { AppendEscapedString(output, session.objectName); });
        field("leafPath", [&]() { AppendEscapedString(output, session.leafPath); });
        if (!session.meshType.empty()) {
            field("meshType", [&]() { AppendEscapedString(output, session.meshType); });
        }
        field("success", [&]() { AppendBool(output, session.success); });
        field("summary", [&]() {
            WriteObject(output, 1, [&](auto&& summaryField) {
                summaryField("elapsedMs", [&]() { AppendNumber(output, session.elapsedMs); });
                summaryField("inputBytes", [&]() { AppendNumber(output, session.inputBytes); });
                summaryField("outputBytes", [&]() { AppendNumber(output, session.outputBytes); });
                summaryField("sourceBytes", [&]() { AppendNumber(output, session.sourceBytes); });
                if (session.topologyBytes > 0u) {
                    summaryField("topologyBytes", [&]() { AppendNumber(output, session.topologyBytes); });
                }
                if (session.compressedPayloadBytes > 0u) {
                    summaryField("compressedPayloadBytes", [&]() {
                        AppendNumber(output, session.compressedPayloadBytes);
                    });
                }
                summaryField("sourceRatio", [&]() { AppendNumber(output, session.SourceRatio()); });
            });
        });
        field("messages", [&]() {
            WriteArray(output, 1, [&](auto&& item) {
                for (const auto& message : session.messages) {
                    item([&]() {
                        WriteObject(output, 2, [&](auto&& messageField) {
                            messageField("order", [&]() { AppendNumber(output, message.order); });
                            messageField("level", [&]() {
                                AppendEscapedString(output, TelemetryMessageSeverityName(message.severity));
                            });
                            if (!message.origin.empty()) {
                                messageField("origin", [&]() { AppendEscapedString(output, message.origin); });
                            }
                            if (!message.code.empty()) {
                                messageField("code", [&]() { AppendEscapedString(output, message.code); });
                            }
                            messageField("text", [&]() { AppendEscapedString(output, message.text); });
                        });
                    });
                }
            });
        });
        field("stages", [&]() {
            WriteArray(output, 1, [&](auto&& item) {
                for (const auto& stage : session.stages) {
                    item([&]() {
                        WriteObject(output, 2, [&](auto&& stageField) {
                            stageField("name", [&]() { AppendEscapedString(output, stage.name); });
                            stageField("order", [&]() { AppendNumber(output, stage.order); });
                            stageField("elapsedMs", [&]() { AppendNumber(output, stage.elapsedMs); });
                            stageField("category", [&]() {
                                AppendEscapedString(output, TelemetryStageCategoryName(stage.category));
                            });
                            if (!stage.scope.empty()) {
                                stageField("scope", [&]() { AppendEscapedString(output, stage.scope); });
                            }
                            if (stage.resource.valid) {
                                stageField("resource", [&]() {
                                    WriteObject(output, 3, [&](auto&& resourceField) {
                                        resourceField("logicalBytes", [&]() {
                                            AppendNumber(output, stage.resource.logicalBytes);
                                        });
                                        resourceField("privateBytes", [&]() {
                                            AppendNumber(output, stage.resource.privateBytes);
                                        });
                                        resourceField("workingSetBytes", [&]() {
                                            AppendNumber(output, stage.resource.workingSetBytes);
                                        });
                                    });
                                });
                            }
                        });
                    });
                }
            });
        });
        field("artifacts", [&]() {
            WriteArray(output, 1, [&](auto&& item) {
                for (const auto& artifact : session.artifacts) {
                    item([&]() {
                        WriteObject(output, 2, [&](auto&& artifactField) {
                            artifactField("order", [&]() {
                                AppendNumber(output, artifact.order);
                            });
                            artifactField("name", [&]() {
                                AppendEscapedString(output, artifact.name);
                            });
                            artifactField("mediaType", [&]() {
                                AppendEscapedString(output, artifact.mediaType);
                            });
                            artifactField("preferredExtension", [&]() {
                                AppendEscapedString(output, artifact.preferredExtension);
                            });
                            artifactField("byteCount", [&]() {
                                AppendNumber(
                                    output,
                                    static_cast<std::uint64_t>(artifact.text.size()));
                            });
                        });
                    });
                }
            });
        });
    });
    return output;
}

} // namespace datacodec

#endif
