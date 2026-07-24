#ifndef DATACODEC_COMMON_DATACODECERROR_H
#define DATACODEC_COMMON_DATACODECERROR_H

#include "DataCodec/API/Adapter/RunRecordTypes.h"
#include "DataCodec/Common/DataCodecTypes.h"

#include <cassert>
#include <string>
#include <string_view>
#include <utility>
namespace datacodec {

enum class CodecErrorCode {
    InvalidInput,
    MissingInput,
    InvalidTopology,
    InvalidRemap,
    PipelineFailure,
    EncodeFailure,
    DecodeFailure,
    UnsupportedFormat,
    UnsupportedPlatform,
};

struct CodecStatus {
    bool success{true};
    CodecErrorCode code{CodecErrorCode::PipelineFailure};
    std::string message;

    explicit operator bool() const noexcept { return success; }

    [[nodiscard]] static CodecStatus Ok() { return {}; }

    [[nodiscard]] static CodecStatus Failure(
        const CodecErrorCode code,
        std::string message) {
        return CodecStatus{
            .success = false,
            .code = code,
            .message = std::move(message),
        };
    }
};

inline const char* CodecErrorCodeName(const CodecErrorCode code) noexcept {
    switch (code) {
        case CodecErrorCode::InvalidInput:
            return "invalid-input";
        case CodecErrorCode::MissingInput:
            return "missing-input";
        case CodecErrorCode::InvalidTopology:
            return "invalid-topology";
        case CodecErrorCode::InvalidRemap:
            return "invalid-remap";
        case CodecErrorCode::PipelineFailure:
            return "pipeline-failure";
        case CodecErrorCode::EncodeFailure:
            return "encode-failure";
        case CodecErrorCode::DecodeFailure:
            return "decode-failure";
        case CodecErrorCode::UnsupportedFormat:
            return "unsupported-format";
        case CodecErrorCode::UnsupportedPlatform:
            return "unsupported-platform";
    }
    return "unknown";
}

inline std::string FormatCodecError(
    const CodecErrorCode code,
    const std::string_view message) {
    std::string output;
    output.reserve(message.size() + 32u);
    output.push_back('[');
    output.append(CodecErrorCodeName(code));
    output.append("] ");
    output.append(message);
    return output;
}

inline TelemetryMessageRecord MakeCodecTelemetryMessage(
    std::string origin,
    const CodecErrorCode code,
    std::string text) {
    return TelemetryMessageRecord{
        .severity = TelemetryMessageSeverity::Error,
        .origin = std::move(origin),
        .code = CodecErrorCodeName(code),
        .text = std::move(text),
    };
}

inline bool SetCodecError(
    std::string* error,
    const CodecErrorCode code,
    const std::string_view message) {
    if (error != nullptr) {
        *error = FormatCodecError(code, message);
    }
    return false;
}

inline bool SetCodecError(
    std::string* error,
    const CodecErrorCode code,
    const char* message) {
    assert(message != nullptr);
    return SetCodecError(error, code, std::string_view(message));
}

inline bool SetCodecError(
    std::string* error,
    const CodecErrorCode code,
    const std::string& message) {
    return SetCodecError(error, code, std::string_view(message));
}

inline std::string FormatStageCodecError(
    const std::string_view stageName,
    const CodecErrorCode code,
    const std::string_view message) {
    std::string output;
    output.reserve(stageName.size() + message.size() + 36u);
    output.append(stageName);
    output.append(": ");
    output.append(FormatCodecError(code, message));
    return output;
}

inline std::string FormatStageCodecError(
    const std::string_view stageName,
    const CodecErrorCode code,
    const std::string& message) {
    return FormatStageCodecError(stageName, code, std::string_view(message));
}

inline std::string FormatStageCodecError(
    const std::string_view stageName,
    const CodecErrorCode code,
    const char* message) {
    assert(message != nullptr);
    return FormatStageCodecError(stageName, code, std::string_view(message));
}

} // namespace datacodec

#endif
