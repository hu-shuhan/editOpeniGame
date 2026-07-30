#ifndef DATACODEC_API_ADAPTER_RUNRECORDTYPES_H
#define DATACODEC_API_ADAPTER_RUNRECORDTYPES_H

#include "DataCodec/Localization/DataCodecMessageCatalog.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace datacodec {

enum class TelemetryRunKind : std::uint8_t {
    Unknown = 0,
    Encode = 1,
    Decode = 2,
};

inline const char* TelemetryRunKindName(const TelemetryRunKind kind) {
    switch (kind) {
        case TelemetryRunKind::Encode:
            return "encode";
        case TelemetryRunKind::Decode:
            return "decode";
        case TelemetryRunKind::Unknown:
        default:
            return "unknown";
    }
}

enum class TelemetryStageCategory : std::uint8_t {
    General = 0,
    Topology = 1,
    Params = 2,
    Geometry = 3,
    Attribute = 4,
    Remap = 5,
    Commit = 6,
};

inline const char* TelemetryStageCategoryName(const TelemetryStageCategory category) {
    switch (category) {
        case TelemetryStageCategory::Topology:
            return "topology";
        case TelemetryStageCategory::Params:
            return "params";
        case TelemetryStageCategory::Geometry:
            return "geometry";
        case TelemetryStageCategory::Attribute:
            return "attribute";
        case TelemetryStageCategory::Remap:
            return "remap";
        case TelemetryStageCategory::Commit:
            return "commit";
        case TelemetryStageCategory::General:
        default:
            return "general";
    }
}

inline TelemetryStageCategory ResolveTelemetryStageCategory(const std::string_view stageName) {
    if (stageName.find("Params") != std::string_view::npos) {
        return TelemetryStageCategory::Params;
    }
    if (stageName.find("Geometry") != std::string_view::npos) {
        return TelemetryStageCategory::Geometry;
    }
    if (stageName.find("Topo") != std::string_view::npos) {
        return TelemetryStageCategory::Topology;
    }
    if (stageName.find("Attr") != std::string_view::npos ||
        stageName.find("Attribute") != std::string_view::npos) {
        return TelemetryStageCategory::Attribute;
    }
    if (stageName.find("Remap") != std::string_view::npos ||
        stageName.find("SpatialPartition") != std::string_view::npos) {
        return TelemetryStageCategory::Remap;
    }
    if (stageName.find("Commit") != std::string_view::npos) {
        return TelemetryStageCategory::Commit;
    }
    return TelemetryStageCategory::General;
}

enum class TelemetryMessageSeverity : std::uint8_t {
    Info = 0,
    Warning = 1,
    Error = 2,
    Critical = 3,
};

inline const char* TelemetryMessageSeverityName(const TelemetryMessageSeverity severity) noexcept {
    switch (severity) {
        case TelemetryMessageSeverity::Warning:
            return "warning";
        case TelemetryMessageSeverity::Error:
            return "error";
        case TelemetryMessageSeverity::Critical:
            return "critical";
        case TelemetryMessageSeverity::Info:
        default:
            return "info";
    }
}

struct TelemetryMessageRecord {
    std::uint64_t order{0};
    TelemetryMessageSeverity severity{TelemetryMessageSeverity::Info};
    std::string origin;
    std::string code;
    DataCodecLanguage language{DataCodecLanguage::SimplifiedChinese};
    DataCodecMessageId messageId{DataCodecMessageId::None};
    std::vector<DataCodecMessageArgument> messageArguments;
    std::string text;
    std::string technicalDetail;
};

struct TelemetryArtifactRecord {
    std::uint64_t order{0};
    std::string name;
    std::string mediaType;
    std::string preferredExtension;
    std::string text;
};

struct TelemetryResourceUsage {
    bool valid{false};
    std::uint64_t logicalBytes{0};
    std::uint64_t privateBytes{0};
    std::uint64_t workingSetBytes{0};
};

inline TelemetryResourceUsage MakeLogicalTelemetryResourceUsage(const std::uint64_t logicalBytes) {
    TelemetryResourceUsage usage;
    usage.valid = true;
    usage.logicalBytes = logicalBytes;
    return usage;
}

struct TelemetryStageRecord {
    std::string name;
    std::uint64_t order{0};
    double elapsedMs{0.0};
    TelemetryStageCategory category{TelemetryStageCategory::General};
    std::string scope;
    TelemetryResourceUsage resource;
};

} // 命名空间 datacodec

#endif
