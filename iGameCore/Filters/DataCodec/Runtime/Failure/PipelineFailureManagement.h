#ifndef DATACODEC_RUNTIME_FAILURE_PIPELINEFAILUREMANAGEMENT_H
#define DATACODEC_RUNTIME_FAILURE_PIPELINEFAILUREMANAGEMENT_H

#include "DataCodec/Runtime/Context/DecodeContext.h"
#include "DataCodec/Runtime/Context/EncodeContext.h"
#include "DataCodec/Runtime/Failure/FailureCleanable.h"
#include "DataCodec/Runtime/Failure/FailureScope.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/Runtime/Workspace/DecodeLeafWorkspace.h"
#include "DataCodec/Runtime/Workspace/EncodeLeafWorkspace.h"

#include <string_view>
#include <utility>
namespace datacodec {

// --- pipeline 级别的失败记录 ---

inline void FailEncodePipeline(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace,
    const CodecErrorCode code,
    const std::string_view message) {
    RecordFailureAndStop(context, workspace, "EncodePipeline", code, message);
}

inline void FailDecodePipeline(
    DecodeContext& context,
    DecodeLeafWorkspace& workspace,
    const CodecErrorCode code,
    const std::string_view message) {
    RecordFailureAndStop(context, workspace, "DecodePipeline", code, message);
}

template <typename TContext>
inline void AssignFailureOrError(
    const TContext& context,
    std::string* error,
    std::string defaultMessage) {
    if (error == nullptr) {
        return;
    }
    if (const auto failure = context.FirstFailure(); failure.has_value()) {
        validation::AssignError(error, failure->formattedMessage);
        return;
    }
    validation::AssignError(error, std::move(defaultMessage));
}

// --- 失败后的统一清理入口 ---

inline void PublishByteStoreCleanupDiagnostics(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace) noexcept {
    try {
        for (auto& diagnostic : workspace.TakeByteStoreDiagnostics()) {
            context.AddWarning("ByteStoreSession", std::move(diagnostic));
        }
    } catch (...) {
    }
}

inline void CleanupAfterEncodeFailure(
    EncodeContext& context,
    EncodeLeafWorkspace& workspace) noexcept {
    workspace.CleanupOnFailure();
    PublishByteStoreCleanupDiagnostics(context, workspace);
    context.CleanupOnFailure();
}

inline void CleanupAfterDecodeFailure(
    DecodeContext& context,
    DecodeLeafWorkspace& workspace) noexcept {
    workspace.CleanupOnFailure();
    context.CleanupOnFailure();
}

struct EncodeFailureCleanup {
    void operator()(EncodeContext& context, EncodeLeafWorkspace& workspace) const noexcept {
        CleanupAfterEncodeFailure(context, workspace);
    }
};

struct DecodeFailureCleanup {
    void operator()(DecodeContext& context, DecodeLeafWorkspace& workspace) const noexcept {
        CleanupAfterDecodeFailure(context, workspace);
    }
};

// 在 pipeline 入口构造，析构时根据 failure 状态自动清理
using EncodeFailureGuard = FailureScope<EncodeContext, EncodeLeafWorkspace, EncodeFailureCleanup>;
using DecodeFailureGuard = FailureScope<DecodeContext, DecodeLeafWorkspace, DecodeFailureCleanup>;

} // namespace datacodec

#endif
