#ifndef DATACODEC_RUNTIME_FAILURE_ENCODEFAILUREMANAGEMENT_H
#define DATACODEC_RUNTIME_FAILURE_ENCODEFAILUREMANAGEMENT_H

#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Failure/FailureScope.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"

#include <string>
#include <string_view>
namespace datacodec {

inline void FailEncodeStage(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const std::string_view stageName,
    const CodecErrorCode code,
    const std::string_view message) {
    datacodec::FailStage(context, workspace, stageName, code, message);
}

inline void FailEncodeStage(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const std::string_view stageName,
    const CodecErrorCode code,
    const std::string& message) {
    FailEncodeStage(context, workspace, stageName, code, std::string_view(message));
}

inline void FailEncodeStage(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const std::string_view stageName,
    const CodecErrorCode code,
    const char* message) {
    FailEncodeStage(
        context,
        workspace,
        stageName,
        code,
        std::string_view(message != nullptr ? message : "unknown failure"));
}

} // namespace datacodec

#endif
