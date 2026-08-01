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
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        writer.Int64(*integer);
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

void WriteReportConfigurationJson(
        ProcessReportJsonWriter& writer,
        const DataCodecReportConfiguration& configuration) {
    writer.StartObject();
    writer.Key("preset");
    WriteJsonString(writer, configuration.preset);
    writer.Key("runtimeProfile");
    WriteJsonString(writer, configuration.runtimeProfile);
    for (const auto& section : configuration.sections) {
        WriteJsonKey(writer, section.name);
        WriteProcessDetailsJson(writer, section.values);
    }
    writer.EndObject();
}

[[nodiscard]] const char* EncodeStorageModeReportName(
        const EncodeStorageMode mode) noexcept {
    return mode == EncodeStorageMode::Memory ? "Memory" : "Managed";
}

[[nodiscard]] const char* DecodeStorageModeReportName(
        const DecodeStorageMode mode) noexcept {
    return mode == DecodeStorageMode::Memory ? "Memory" : "Managed";
}

[[nodiscard]] const char* AttributeDecodePayloadModeReportName(
        const AttributeDecodePayloadMode mode) noexcept {
    switch (mode) {
        case AttributeDecodePayloadMode::Managed:
            return "Managed";
        case AttributeDecodePayloadMode::Memory:
            return "Memory";
        case AttributeDecodePayloadMode::OneShotZstd:
            return "OneShotZstd";
    }
    return "Managed";
}

[[nodiscard]] const char* DecodeValidationModeReportName(
        const DecodeValidationMode mode) noexcept {
    return mode == DecodeValidationMode::Strict ? "Strict" : "Required";
}

[[nodiscard]] const char* TopologyDecodeOutputModeReportName(
        const TopologyDecodeOutputMode mode) noexcept {
    return mode == TopologyDecodeOutputMode::ObserverOnly
        ? "ObserverOnly"
        : "CommitToAdapter";
}

[[nodiscard]] const char* TemporalPredictorSearchStrategyReportName(
        const TemporalPredictorSearchStrategy strategy) noexcept {
    switch (strategy) {
        case TemporalPredictorSearchStrategy::ExhaustiveL2:
            return "ExhaustiveL2";
        case TemporalPredictorSearchStrategy::ExhaustiveEstimatedBytes:
            return "ExhaustiveEstimatedBytes";
        case TemporalPredictorSearchStrategy::CoarseToFineL2:
            return "CoarseToFineL2";
        case TemporalPredictorSearchStrategy::CoarseToFineEstimatedBytes:
            return "CoarseToFineEstimatedBytes";
    }
    return "ExhaustiveL2";
}

[[nodiscard]] std::vector<DataCodecProcessDetail> MakeCommonResourceBudgetDetails(
        const ResourceBudgetControlParams& budget) {
    return {
        {"residentLimitMiB", budget.residentLimitMiB},
        {"accessWindowMiB", budget.accessWindowMiB},
        {"activeWindowMiB", budget.activeWindowMiB},
        {"scratchRetainedBlockCount", static_cast<std::uint64_t>(budget.scratchRetainedBlockCount)},
        {"scratchRetainedBlockMiB", budget.scratchRetainedBlockMiB},
        {"scratchRetainedTotalMiB", budget.scratchRetainedTotalMiB},
    };
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

DataCodecReportConfiguration MakeDataCodecEncodeReportConfiguration(
        const DataCodecEncodeTier preset,
        const bool compressionEnhancementEnabled,
        const DataCodecEncodeConfigurationParams& configuration) {
    const auto& control = configuration.controlParams;
    const auto& budget = control.resourceBudget;
    auto resourceBudget = MakeCommonResourceBudgetDetails(budget);
    resourceBudget.insert(resourceBudget.end(), {
        {"attributeScratchQuotaMiB", budget.attributeScratchQuotaMiB},
        {"attributeMemoryStagingLimitMiB", budget.attributeMemoryStagingLimitMiB},
        {"attributeManagedStagingLogicalLimitMiB", budget.attributeManagedStagingLogicalLimitMiB},
        {"encodeReferenceResidentLimitMiB", budget.encodeReferenceResidentLimitMiB},
        {"remapMortonLeafMiB", budget.remapMortonLeafMiB},
        {"remapMortonRunBufferMiB", budget.remapMortonRunBufferMiB},
        {"remapScratchQuotaMiB", budget.remapScratchQuotaMiB},
    });

    return DataCodecReportConfiguration{
        .preset = DataCodecEncodeTierName(preset),
        .runtimeProfile = DataCodecRuntimeProfileName(
            configuration.source.runtimeProfile),
        .sections = {
            DataCodecReportConfigurationSection{
                .name = "pipeline",
                .values = {
                    {"compressionEnhancementEnabled", compressionEnhancementEnabled},
                    {"pointOrder", EncodePointOrderModeName(configuration.pipelineControl.pointOrder)},
                    {"cellOrder", EncodeCellOrderModeName(configuration.pipelineControl.cellOrder)},
                    {"packageFieldEncodingMode", PackageFieldEncodingModeName(
                        configuration.pipelineControl.packageFields.mode)},
                    {"packageZstdLevel", static_cast<std::int64_t>(
                        configuration.pipelineControl.packageFields.zstdLevel)},
                    {"packageFieldWorkerCount", static_cast<std::uint64_t>(
                        configuration.pipelineControl.packageFields.workerCount)},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "spatialBlocking",
                .values = {
                    {"pointElementCount", static_cast<std::uint64_t>(
                        control.spatialBlockPolicy.pointElementCount)},
                    {"cellElementCount", static_cast<std::uint64_t>(
                        control.spatialBlockPolicy.cellElementCount)},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "references",
                .values = {
                    {"attributeReferenceEnabled", control.attrReference.enabled},
                    {"geometryReferenceEnabled", control.geometryReference.enabled},
                    {"topologyReferenceEnabled", control.topologyReference.enabled},
                    {"attributeKeyFrameInterval", static_cast<std::uint64_t>(
                        control.attrReference.temporalField.keyFrameInterval)},
                    {"geometryKeyFrameInterval", static_cast<std::uint64_t>(
                        control.geometryReference.temporalField.keyFrameInterval)},
                    {"attributeLocalWindowSearchEnabled",
                        control.attrReference.temporalField.predictor.enableLocalWindowSearch},
                    {"geometryLocalWindowSearchEnabled",
                        control.geometryReference.temporalField.predictor.enableLocalWindowSearch},
                    {"attributePredictorSearchStrategy",
                        TemporalPredictorSearchStrategyReportName(
                            control.attrReference.temporalField.predictor.searchStrategy)},
                    {"geometryPredictorSearchStrategy",
                        TemporalPredictorSearchStrategyReportName(
                            control.geometryReference.temporalField.predictor.searchStrategy)},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "parallelism",
                .values = {
                    {"parallelStagesEnabled", configuration.execution.enableParallelStages},
                    {"attributeCompressionLanes", static_cast<std::uint64_t>(
                        budget.AttributePressioLaneCount())},
                    {"attributeReferenceLanes", static_cast<std::uint64_t>(
                        budget.AttributeReferenceLaneCount())},
                    {"topologyBlockLanes", static_cast<std::uint64_t>(
                        budget.TopologyBlockLaneCount())},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "storage",
                .values = {
                    {"geometryTransferCacheMode", EncodeStorageModeReportName(
                        budget.GeometryEncodeTransferCacheStorageMode())},
                    {"geometryStagingMode", EncodeStorageModeReportName(
                        budget.GeometryEncodeStagingStorageMode())},
                    {"attributeTransferCacheMode", EncodeStorageModeReportName(
                        budget.AttributeEncodeTransferCacheStorageMode())},
                    {"attributeStagingMode", EncodeStorageModeReportName(
                        budget.AttributeEncodeStagingStorageMode())},
                    {"topologyTransferCacheMode", EncodeStorageModeReportName(
                        budget.TopologyEncodeTransferCacheStorageMode())},
                    {"remapMode", EncodeStorageModeReportName(
                        budget.RemapEncodeStorageMode())},
                    {"packageFieldStagingMode", EncodeStorageModeReportName(
                        budget.PackageFieldStagingStorageMode())},
                    {"attributeReferenceCacheMode", EncodeStorageModeReportName(
                        budget.AttributeEncodeReferenceCacheStorageMode())},
                    {"geometryReferenceCacheMode", EncodeStorageModeReportName(
                        budget.GeometryEncodeReferenceCacheStorageMode())},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "resourceBudget",
                .values = std::move(resourceBudget),
            },
        },
    };
}

DataCodecReportConfiguration MakeDataCodecDecodeReportConfiguration(
        const std::optional<DataCodecDecodeTier> preset,
        const DataCodecDecodeConfigurationParams& configuration,
        const bool loadAllAvailableAttributes) {
    const auto& control = configuration.controlParams;
    const auto& budget = control.resourceBudget;
    auto resourceBudget = MakeCommonResourceBudgetDetails(budget);
    resourceBudget.insert(resourceBudget.end(), {
        {"attributeDecodeMemoryPayloadLimitMiB", budget.attributeDecodeMemoryPayloadLimitMiB},
        {"attributeDecodeMemoryCacheLimitMiB", budget.attributeDecodeMemoryCacheLimitMiB},
        {"geometryDecodeMemoryCacheLimitMiB", budget.geometryDecodeMemoryCacheLimitMiB},
        {"geometryDecodeMemoryReferenceLimitMiB", budget.geometryDecodeMemoryReferenceLimitMiB},
        {"topologyDecodeMemoryInputLimitMiB", budget.topologyDecodeMemoryInputLimitMiB},
        {"topologyDecodeMemoryCacheLimitMiB", budget.topologyDecodeMemoryCacheLimitMiB},
        {"topologyDecodeMemoryReferenceLimitMiB", budget.topologyDecodeMemoryReferenceLimitMiB},
        {"decodeReferenceResidentLimitMiB", budget.decodeReferenceResidentLimitMiB},
        {"decodeReferenceFrameLimit", static_cast<std::uint64_t>(
            budget.decodeReferenceFrameLimit)},
    });

    return DataCodecReportConfiguration{
        .preset = preset.has_value()
            ? DataCodecDecodeTierName(*preset)
            : "Custom",
        .runtimeProfile = DataCodecRuntimeProfileName(
            configuration.source.runtimeProfile),
        .sections = {
            DataCodecReportConfigurationSection{
                .name = "validation",
                .values = {
                    {"mode", DecodeValidationModeReportName(
                        control.validation.decodeMode)},
                    {"topologyReferencesEnabled",
                        control.validation.validateTopologyReferences},
                    {"floatingPointValuesEnabled",
                        control.validation.validateFloatingPointValues},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "execution",
                .values = {
                    {"parallelStagesEnabled", configuration.execution.enableParallelStages},
                    {"fullInputPrefetchEnabled", configuration.execution.enableFullInputPrefetch},
                    {"topologyOutputMode", TopologyDecodeOutputModeReportName(
                        configuration.execution.topologyOutputMode)},
                    {"loadAllAvailableAttributes", loadAllAvailableAttributes},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "caching",
                .values = {
                    {"decodedResultCacheEnabled", configuration.decodedFrameCachePolicy.enabled},
                    {"decodedResultResidentFrameLimit", static_cast<std::uint64_t>(
                        configuration.decodedFrameCachePolicy.residentFrameLimit)},
                    {"decodedResultResidentLimitBytes",
                        configuration.decodedFrameCachePolicy.residentLimitBytes},
                    {"decodedResultPrefetchFrameCount", static_cast<std::uint64_t>(
                        configuration.decodedFrameCachePolicy.prefetchFrameCount)},
                    {"encodedInputCacheEnabled", configuration.encodedInputCachePolicy.enabled},
                    {"encodedInputResidentLimit", static_cast<std::uint64_t>(
                        configuration.encodedInputCachePolicy.residentInputLimit)},
                    {"encodedInputResidentLimitBytes",
                        configuration.encodedInputCachePolicy.residentLimitBytes},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "parallelism",
                .values = {
                    {"attributeDecodeLanes", static_cast<std::uint64_t>(
                        budget.AttributeDecodeLaneCount())},
                    {"attributeCommitLanes", static_cast<std::uint64_t>(
                        budget.AttributeCommitLaneCount())},
                    {"topologyBlockLanes", static_cast<std::uint64_t>(
                        budget.TopologyBlockLaneCount())},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "storage",
                .values = {
                    {"attributePayloadMode", AttributeDecodePayloadModeReportName(
                        budget.AttributeDecodePayloadStorageMode())},
                    {"attributeCacheMode", DecodeStorageModeReportName(
                        budget.AttributeDecodeCacheStorageMode())},
                    {"geometryCacheMode", DecodeStorageModeReportName(
                        budget.GeometryDecodeCacheStorageMode())},
                    {"geometryReferenceCacheMode", DecodeStorageModeReportName(
                        budget.GeometryDecodeReferenceCacheStorageMode())},
                    {"topologyInputMode", DecodeStorageModeReportName(
                        budget.TopologyDecodeInputStorageMode())},
                    {"topologyCacheMode", DecodeStorageModeReportName(
                        budget.TopologyDecodeCacheStorageMode())},
                    {"topologyReferenceCacheMode", DecodeStorageModeReportName(
                        budget.TopologyDecodeReferenceCacheStorageMode())},
                },
            },
            DataCodecReportConfigurationSection{
                .name = "resourceBudget",
                .values = std::move(resourceBudget),
            },
        },
    };
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
    if (report.operation == TelemetryRunKind::Encode) {
        writer.Key("sourceFileSizeBytes");
        writer.Uint64(report.inputBytes);
        writer.Key("compressedOutputSizeBytes");
        writer.Uint64(report.outputBytes);
        writer.Key("compressionRatio");
        if (report.inputBytes > 0u && report.outputBytes > 0u) {
            writer.Double(
                static_cast<double>(report.outputBytes) /
                static_cast<double>(report.inputBytes));
        } else {
            writer.Null();
        }
        if (!report.summaryNote.empty()) {
            writer.Key("note");
            WriteJsonString(writer, report.summaryNote);
        }
    } else if (report.inputBytes > 0u && report.outputBytes > 0u) {
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
    if (!report.configuration.preset.empty() ||
        !report.configuration.runtimeProfile.empty() ||
        !report.configuration.sections.empty()) {
        writer.Key("configuration");
        WriteReportConfigurationJson(writer, report.configuration);
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
