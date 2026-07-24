#ifndef DATACODEC_RUNTIME_FAILURE_DECODEFAILUREMANAGEMENT_H
#define DATACODEC_RUNTIME_FAILURE_DECODEFAILUREMANAGEMENT_H

#include "DataCodec/Runtime/Context/DecodeContext.h"
#include "DataCodec/Runtime/Failure/FailureScope.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Runtime/Workspace/DecodeLeafWorkspace.h"

#include <string>
#include <string_view>
namespace datacodec {

inline void FailDecodeStage(
    DecodeContext& context,
    DecodeLeafWorkspace& workspace,
    const std::string_view stageName,
    const CodecErrorCode code,
    const std::string_view message) {
    datacodec::FailStage(context, workspace, stageName, code, message);
}

inline void FailDecodeStage(
    DecodeContext& context,
    DecodeLeafWorkspace& workspace,
    const std::string_view stageName,
    const CodecErrorCode code,
    const std::string& message) {
    FailDecodeStage(context, workspace, stageName, code, std::string_view(message));
}

inline void FailDecodeStage(
    DecodeContext& context,
    DecodeLeafWorkspace& workspace,
    const std::string_view stageName,
    const CodecErrorCode code,
    const char* message) {
    FailDecodeStage(
        context,
        workspace,
        stageName,
        code,
        std::string_view(message != nullptr ? message : "unknown failure"));
}

} // namespace datacodec

#endif
