#ifndef DATACODEC_LOG_REPORT_DATACODECPROCESSREPORTJSON_H
#define DATACODEC_LOG_REPORT_DATACODECPROCESSREPORTJSON_H

#include "DataCodec/API/Params/CodecPerformancePresetParams.h"
#include "DataCodec/Log/Telemetry/TelemetrySession.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace datacodec
{

struct DataCodecProcessMemory {
    bool valid{false};
    std::uint64_t beforeWorkingSetBytes{0u};
    std::uint64_t afterWorkingSetBytes{0u};
    std::uint64_t peakWorkingSetBytes{0u};
};

struct DataCodecProcessDetail {
    using Value = std::variant<std::string, std::int64_t, std::uint64_t, double, bool>;

    std::string name;
    Value value;

    DataCodecProcessDetail(std::string detailName, Value detailValue)
        : name(std::move(detailName)), value(std::move(detailValue)) {}

    DataCodecProcessDetail(std::string detailName, std::string detailValue)
        : name(std::move(detailName)), value(std::move(detailValue)) {}

    DataCodecProcessDetail(std::string detailName, const char* detailValue)
        : name(std::move(detailName)), value(std::string(detailValue)) {}

    DataCodecProcessDetail(std::string detailName, const std::uint64_t detailValue)
        : name(std::move(detailName)), value(detailValue) {}

    DataCodecProcessDetail(std::string detailName, const std::int64_t detailValue)
        : name(std::move(detailName)), value(detailValue) {}

    DataCodecProcessDetail(std::string detailName, const double detailValue)
        : name(std::move(detailName)), value(detailValue) {}

    DataCodecProcessDetail(std::string detailName, const bool detailValue)
        : name(std::move(detailName)), value(detailValue) {}
};

struct DataCodecReportConfigurationSection {
    std::string name;
    std::vector<DataCodecProcessDetail> values;
};

struct DataCodecReportConfiguration {
    std::string preset;
    std::string runtimeProfile;
    std::vector<DataCodecReportConfigurationSection> sections;
};

struct DataCodecProcessNode {
    std::string name;
    bool completed{true};
    bool success{true};
    bool elapsedMsValid{true};
    double elapsedMs{0.0};
    std::uint64_t inputBytes{0u};
    std::uint64_t outputBytes{0u};
    std::uint64_t measuredTaskCount{0u};
    double longestMeasuredTaskMs{0.0};
    DataCodecProcessMemory memory;
    std::vector<DataCodecProcessDetail> details;
    std::vector<DataCodecProcessNode> children;
};

struct DataCodecProcessReport {
    TelemetryRunKind operation{TelemetryRunKind::Unknown};
    std::string generatedAtUtc;
    std::string objectName;
    bool completed{true};
    bool success{false};
    double elapsedMs{0.0};
    std::uint64_t inputBytes{0u};
    std::uint64_t outputBytes{0u};
    std::string summaryNote;
    DataCodecProcessMemory memory;
    std::vector<DataCodecProcessDetail> details;
    std::vector<DataCodecProcessNode> processes;
    DataCodecReportConfiguration configuration;
};

struct DataCodecErrorReportEntry {
    std::vector<std::string> phasePath;
    std::string origin;
    std::string code;
    std::string message;
    std::string technicalDetail;
};

struct DataCodecErrorReport {
    TelemetryRunKind operation{TelemetryRunKind::Unknown};
    std::string generatedAtUtc;
    std::string objectName;
    std::vector<DataCodecErrorReportEntry> errors;
};

namespace processreportdetail
{

inline void AppendProcessMemorySpan(
        DataCodecProcessMemory& destination,
        const DataCodecProcessMemory& source) {
    if (!source.valid) { return; }
    if (!destination.valid) {
        destination = source;
        return;
    }
    if (destination.beforeWorkingSetBytes == 0u && source.beforeWorkingSetBytes > 0u) {
        destination.beforeWorkingSetBytes = source.beforeWorkingSetBytes;
    }
    if (source.afterWorkingSetBytes > 0u) { destination.afterWorkingSetBytes = source.afterWorkingSetBytes; }
    destination.peakWorkingSetBytes = std::max(destination.peakWorkingSetBytes, source.peakWorkingSetBytes);
}

inline void MergeProcessMemoryPeak(
        DataCodecProcessMemory& destination,
        const DataCodecProcessMemory& source) {
    if (!source.valid) { return; }
    destination.valid = true;
    destination.peakWorkingSetBytes = std::max(
        destination.peakWorkingSetBytes,
        source.peakWorkingSetBytes);
}

[[nodiscard]] inline DataCodecProcessMemory ProcessMemoryFromResource(const TelemetryResourceUsage& resource) {
    DataCodecProcessMemory memory;
    if (!resource.valid || (resource.workingSetBytes == 0u && resource.workingSetBeforeBytes == 0u &&
                            resource.workingSetAfterBytes == 0u && resource.peakWorkingSetBytes == 0u)) {
        return memory;
    }
    memory.valid = true;
    memory.beforeWorkingSetBytes = resource.workingSetBeforeBytes;
    memory.afterWorkingSetBytes = resource.workingSetAfterBytes;
    memory.peakWorkingSetBytes = std::max(resource.peakWorkingSetBytes, resource.workingSetBytes);
    return memory;
}

[[nodiscard]] inline DataCodecProcessMemory ProcessPeakMemoryFromResource(
        const TelemetryResourceUsage& resource) {
    auto memory = ProcessMemoryFromResource(resource);
    if (!memory.valid) { return memory; }
    memory.beforeWorkingSetBytes = 0u;
    memory.afterWorkingSetBytes = 0u;
    memory.peakWorkingSetBytes = std::max({
        resource.workingSetBytes,
        resource.workingSetBeforeBytes,
        resource.workingSetAfterBytes,
        resource.peakWorkingSetBytes,
    });
    return memory;
}

inline void SetProcessDetail(
        std::vector<DataCodecProcessDetail>& details,
        std::string name,
        DataCodecProcessDetail::Value value) {
    const auto iterator =
            std::find_if(details.begin(), details.end(), [&name](const auto& detail) { return detail.name == name; });
    if (iterator != details.end()) {
        iterator->value = std::move(value);
        return;
    }
    details.push_back({std::move(name), std::move(value)});
}

enum class ProcessStageGroup : std::uint8_t {
    Preparation = 0u,
    Remap = 1u,
    Topology = 2u,
    Geometry = 3u,
    Attributes = 4u,
    Commit = 5u,
    Packaging = 6u,
    Count = 7u,
};

[[nodiscard]] inline const char* ProcessStageGroupName(const ProcessStageGroup group) noexcept {
    switch (group) {
        case ProcessStageGroup::Preparation:
            return "Preparation";
        case ProcessStageGroup::Remap:
            return "Remap";
        case ProcessStageGroup::Topology:
            return "Topology";
        case ProcessStageGroup::Geometry:
            return "Geometry";
        case ProcessStageGroup::Attributes:
            return "Attributes";
        case ProcessStageGroup::Commit:
            return "Commit";
        case ProcessStageGroup::Packaging:
            return "Packaging";
        case ProcessStageGroup::Count:
        default:
            return "";
    }
}

[[nodiscard]] inline ProcessStageGroup ResolveProcessStageGroup(const TelemetryStageRecord& stage) noexcept {
    auto category = stage.category;
    if (category == TelemetryStageCategory::General) { category = ResolveTelemetryStageCategory(stage.name); }
    switch (category) {
        case TelemetryStageCategory::Params:
            return ProcessStageGroup::Preparation;
        case TelemetryStageCategory::Remap:
            return ProcessStageGroup::Remap;
        case TelemetryStageCategory::Topology:
            return ProcessStageGroup::Topology;
        case TelemetryStageCategory::Geometry:
            return ProcessStageGroup::Geometry;
        case TelemetryStageCategory::Attribute:
            return ProcessStageGroup::Attributes;
        case TelemetryStageCategory::Commit:
            return ProcessStageGroup::Commit;
        case TelemetryStageCategory::General:
        default:
            break;
    }
    if (stage.name.find("Package") != std::string::npos || stage.name.find("Write") != std::string::npos ||
        stage.name.find("Finalize") != std::string::npos || stage.name.find("Output") != std::string::npos) {
        return ProcessStageGroup::Packaging;
    }
    return ProcessStageGroup::Count;
}

[[nodiscard]] inline bool ShouldExposeLogicalMetric(const std::string& name) noexcept {
    return name.find("peak_") != std::string::npos || name.find("resident_limit_bytes") != std::string::npos ||
           name == "adapter.native_resident_bytes";
}

[[nodiscard]] inline std::string SessionProcessName(const TelemetrySession& session) {
    if (session.objectName == "Package" || session.meshType == "Package") { return "Package"; }
    if (!session.leafPath.empty()) { return "Leaf: " + session.leafPath; }
    if (!session.objectName.empty()) { return session.objectName; }
    return session.parentRunId == 0u ? "Process" : "Leaf";
}

[[nodiscard]] inline std::vector<std::string>
SessionPhasePath(const TelemetrySession& session,
                 const std::unordered_map<std::uint64_t, const TelemetrySession*>& sessionsById,
                 const std::string& rootName) {
    std::vector<std::string> reversed;
    std::unordered_set<std::uint64_t> visited;
    auto* current = &session;
    while (current != nullptr && visited.insert(current->runId).second) {
        reversed.push_back(SessionProcessName(*current));
        const auto parent = sessionsById.find(current->parentRunId);
        current = parent != sessionsById.end() ? parent->second : nullptr;
    }
    std::vector<std::string> path;
    path.reserve(reversed.size() + 1u);
    path.push_back(rootName);
    for (auto iterator = reversed.rbegin(); iterator != reversed.rend(); ++iterator) { path.push_back(*iterator); }
    return path;
}

inline void AppendUniqueError(DataCodecErrorReport& report, DataCodecErrorReportEntry error,
                               std::unordered_set<std::string>& keys) {
    auto key = error.code;
    key.push_back('\n');
    key += error.message;
    key.push_back('\n');
    key += error.technicalDetail;
    if (!keys.insert(std::move(key)).second) { return; }
    report.errors.push_back(std::move(error));
}

inline void CompleteNodeMemoryFromChildren(DataCodecProcessNode& node) {
    DataCodecProcessMemory childrenMemory;
    for (const auto& child: node.children) {
        AppendProcessMemorySpan(childrenMemory, child.memory);
    }
    if (!childrenMemory.valid) { return; }
    if (!node.memory.valid) {
        node.memory = childrenMemory;
        return;
    }
    if (node.memory.beforeWorkingSetBytes == 0u) {
        node.memory.beforeWorkingSetBytes = childrenMemory.beforeWorkingSetBytes;
    }
    if (node.memory.afterWorkingSetBytes == 0u) {
        node.memory.afterWorkingSetBytes = childrenMemory.afterWorkingSetBytes;
    }
    node.memory.peakWorkingSetBytes = std::max(
        node.memory.peakWorkingSetBytes,
        childrenMemory.peakWorkingSetBytes);
}

} // namespace processreportdetail

[[nodiscard]] inline std::vector<DataCodecProcessNode>
BuildTelemetryProcessNodes(const std::vector<TelemetrySession>& sessions, const TelemetryRunKind operation) {
    std::vector<std::size_t> selected;
    std::unordered_map<std::uint64_t, std::size_t> selectedById;
    for (std::size_t index = 0u; index < sessions.size(); ++index) {
        if (sessions[index].runKind != operation) { continue; }
        selectedById[sessions[index].runId] = index;
        selected.push_back(index);
    }

    std::unordered_map<std::uint64_t, std::vector<std::size_t>> childrenByParent;
    std::vector<std::size_t> roots;
    for (const auto index: selected) {
        const auto parentId = sessions[index].parentRunId;
        if (parentId == 0u || !selectedById.contains(parentId)) {
            roots.push_back(index);
        } else {
            childrenByParent[parentId].push_back(index);
        }
    }

    std::unordered_set<std::uint64_t> active;
    const auto buildNode = [&](const auto& self, const std::size_t index) -> DataCodecProcessNode {
        const auto& session = sessions[index];
        DataCodecProcessNode node{
                .name = processreportdetail::SessionProcessName(session),
                .success = session.success,
                .elapsedMs = session.elapsedMs,
                .inputBytes = session.inputBytes,
                .outputBytes = session.outputBytes,
        };
        if (!active.insert(session.runId).second) { return node; }

        std::array<DataCodecProcessNode, static_cast<std::size_t>(processreportdetail::ProcessStageGroup::Count)>
                groups;
        std::array<bool, static_cast<std::size_t>(processreportdetail::ProcessStageGroup::Count)> used{};
        for (std::size_t groupIndex = 0u; groupIndex < groups.size(); ++groupIndex) {
            groups[groupIndex].name = processreportdetail::ProcessStageGroupName(
                    static_cast<processreportdetail::ProcessStageGroup>(groupIndex));
            groups[groupIndex].success = session.success;
            groups[groupIndex].elapsedMsValid = false;
        }

        for (const auto& stage: session.stages) {
            if (stage.name == "memory.run") {
                processreportdetail::AppendProcessMemorySpan(
                    node.memory,
                    processreportdetail::ProcessMemoryFromResource(stage.resource));
                continue;
            }
            const auto group = processreportdetail::ResolveProcessStageGroup(stage);
            if (group == processreportdetail::ProcessStageGroup::Count) { continue; }
            const auto groupIndex = static_cast<std::size_t>(group);
            auto& groupNode = groups[groupIndex];
            used[groupIndex] = true;
            if (stage.elapsedMs > 0.0) {
                groupNode.longestMeasuredTaskMs = std::max(
                    groupNode.longestMeasuredTaskMs,
                    stage.elapsedMs);
                groupNode.measuredTaskCount += std::max<std::uint64_t>(
                    stage.sampleCount,
                    1u);
            }
            processreportdetail::MergeProcessMemoryPeak(
                groupNode.memory,
                processreportdetail::ProcessPeakMemoryFromResource(stage.resource));
            if (stage.resource.valid && stage.resource.logicalBytes > 0u &&
                processreportdetail::ShouldExposeLogicalMetric(stage.name)) {
                processreportdetail::SetProcessDetail(
                    groupNode.details,
                    stage.name,
                    stage.resource.logicalBytes);
            }
        }

        const auto childIterator = childrenByParent.find(session.runId);
        if (childIterator != childrenByParent.end()) {
            for (const auto childIndex: childIterator->second) { node.children.push_back(self(self, childIndex)); }
        }
        for (std::size_t groupIndex = 0u; groupIndex < groups.size(); ++groupIndex) {
            if (used[groupIndex] && (groups[groupIndex].measuredTaskCount > 0u || groups[groupIndex].memory.valid ||
                                     !groups[groupIndex].details.empty())) {
                node.children.push_back(std::move(groups[groupIndex]));
            }
        }
        processreportdetail::CompleteNodeMemoryFromChildren(node);
        active.erase(session.runId);
        return node;
    };

    std::vector<DataCodecProcessNode> nodes;
    nodes.reserve(roots.size());
    for (const auto index: roots) { nodes.push_back(buildNode(buildNode, index)); }
    return nodes;
}

inline void CompleteDataCodecProcessReportMemory(DataCodecProcessReport& report) {
    report.memory = {};
    for (const auto& process: report.processes) {
        processreportdetail::AppendProcessMemorySpan(report.memory, process.memory);
    }
}

inline void CompleteDataCodecProcessNodeMemory(DataCodecProcessNode& node) {
    processreportdetail::CompleteNodeMemoryFromChildren(node);
}

[[nodiscard]] inline DataCodecErrorReport
BuildDataCodecErrorReport(const TelemetryRunKind operation, std::string generatedAtUtc, std::string objectName,
                          const std::vector<TelemetrySession>& sessions,
                          const std::vector<TelemetryMessageRecord>& additionalMessages,
                          std::string fallbackMessage = {}) {
    DataCodecErrorReport report{
            .operation = operation,
            .generatedAtUtc = std::move(generatedAtUtc),
            .objectName = std::move(objectName),
    };
    const std::string rootName = operation == TelemetryRunKind::Encode   ? "Encode"
                                 : operation == TelemetryRunKind::Decode ? "Decode"
                                                                         : "Operation";
    std::unordered_map<std::uint64_t, const TelemetrySession*> sessionsById;
    for (const auto& session: sessions) {
        if (session.runKind == operation) { sessionsById[session.runId] = &session; }
    }
    std::unordered_set<std::string> keys;
    for (const auto& session: sessions) {
        if (session.runKind != operation) { continue; }
        const auto phasePath = processreportdetail::SessionPhasePath(session, sessionsById, rootName);
        for (const auto& message: session.messages) {
            if (message.severity != TelemetryMessageSeverity::Error &&
                message.severity != TelemetryMessageSeverity::Critical) {
                continue;
            }
            processreportdetail::AppendUniqueError(report,
                                                   DataCodecErrorReportEntry{
                                                           .phasePath = phasePath,
                                                           .origin = message.origin,
                                                           .code = message.code,
                                                           .message = message.text,
                                                           .technicalDetail = message.technicalDetail,
                                                   },
                                                   keys);
        }
    }
    for (const auto& message: additionalMessages) {
        if (message.severity != TelemetryMessageSeverity::Error &&
            message.severity != TelemetryMessageSeverity::Critical) {
            continue;
        }
        processreportdetail::AppendUniqueError(report,
                                               DataCodecErrorReportEntry{
                                                       .phasePath = {rootName},
                                                       .origin = message.origin,
                                                       .code = message.code,
                                                       .message = message.text,
                                                       .technicalDetail = message.technicalDetail,
                                               },
                                               keys);
    }
    if (report.errors.empty() && !fallbackMessage.empty()) {
        report.errors.push_back({
                .phasePath = {rootName},
                .message = std::move(fallbackMessage),
        });
    }
    return report;
}

[[nodiscard]] std::string SerializeDataCodecProcessReportJson(
    const DataCodecProcessReport& report);

[[nodiscard]] DataCodecReportConfiguration MakeDataCodecEncodeReportConfiguration(
    DataCodecEncodeTier preset,
    bool compressionEnhancementEnabled,
    const DataCodecEncodeConfigurationParams& configuration);

[[nodiscard]] DataCodecReportConfiguration MakeDataCodecDecodeReportConfiguration(
    std::optional<DataCodecDecodeTier> preset,
    const DataCodecDecodeConfigurationParams& configuration,
    bool loadAllAvailableAttributes);

[[nodiscard]] std::string SerializeDataCodecErrorReportJson(
    const DataCodecErrorReport& report);

[[nodiscard]] std::string MakeDataCodecReportFileTimestampUtc();

} // namespace datacodec

#endif
