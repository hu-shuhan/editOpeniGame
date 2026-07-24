#ifndef DATACODEC_RUNTIME_FAILURE_FAILURESCOPE_H
#define DATACODEC_RUNTIME_FAILURE_FAILURESCOPE_H

#include "DataCodec/Common/DataCodecError.h"

#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
namespace datacodec {

template <typename TContext, typename TWorkspace>
struct DefaultFailureCleanup {
    void operator()(TContext& context, TWorkspace& workspace) const noexcept {
        workspace.CleanupOnFailure();
        context.CleanupOnFailure();
    }
};

template <
    typename TContext,
    typename TWorkspace,
    typename TCleanup = DefaultFailureCleanup<TContext, TWorkspace>>
class FailureScope {
public:
    FailureScope(TContext& context, TWorkspace& workspace, TCleanup cleanup = {}) noexcept
        : m_context(context),
          m_workspace(workspace),
          m_cleanup(std::move(cleanup)) {}

    ~FailureScope() noexcept { CleanupIfFailed(); }

    FailureScope(const FailureScope&) = delete;
    FailureScope& operator=(const FailureScope&) = delete;

    bool Fail(
        const std::string_view origin,
        const CodecErrorCode code,
        const std::string_view message) noexcept {
        try {
            m_context.RecordFailure(origin, code, message);
            m_workspace.RequestStop();
        } catch (...) {
            m_forceCleanup = true;
            try {
                m_workspace.RequestStop();
            } catch (...) {
            }
        }
        return false;
    }

    bool Fail(
        const std::string_view origin,
        const CodecErrorCode code,
        const std::string& message) noexcept {
        return Fail(origin, code, std::string_view(message));
    }

    bool Fail(
        const std::string_view origin,
        const CodecErrorCode code,
        const char* message) noexcept {
        return Fail(origin, code, std::string_view(message != nullptr ? message : "unknown failure"));
    }

    template <typename TFn>
    bool Run(
        const std::string_view origin,
        const CodecErrorCode code,
        TFn&& fn) noexcept {
        try {
            bool result = true;
            if constexpr (std::is_void_v<std::invoke_result_t<TFn>>) {
                std::invoke(std::forward<TFn>(fn));
            } else {
                result = static_cast<bool>(std::invoke(std::forward<TFn>(fn)));
            }
            if (!result) {
                // 阶段已经明确失败时，即使没有留下 failure 记录也必须执行资源清理
                m_forceCleanup = true;
                return false;
            }
            return !FailedOrStopped();
        } catch (const std::exception& exception) {
            return Fail(origin, code, exception.what());
        } catch (...) {
            return Fail(origin, code, "unknown exception");
        }
    }

    void CleanupIfFailed() noexcept {
        if (m_cleaned) {
            return;
        }
        if (!FailedOrStopped()) {
            return;
        }
        m_cleaned = true;
        try {
            m_cleanup(m_context, m_workspace);
        } catch (...) {
        }
    }

private:
    TContext& m_context;
    TWorkspace& m_workspace;
    TCleanup m_cleanup;
    bool m_cleaned{false};
    bool m_forceCleanup{false};

    [[nodiscard]] bool FailedOrStopped() noexcept {
        bool failed = m_forceCleanup;
        try {
            failed = failed || m_context.HasFailure();
        } catch (...) {
            failed = true;
        }
        try {
            failed = failed || m_workspace.StopRequested();
        } catch (...) {
            failed = true;
        }
        return failed;
    }
};

template <typename TContext, typename TWorkspace>
inline bool RecordFailureAndStop(
    TContext& context,
    TWorkspace& workspace,
    const std::string_view origin,
    const CodecErrorCode code,
    const std::string_view message) noexcept {
    try {
        context.RecordFailure(origin, code, message);
        workspace.RequestStop();
    } catch (...) {
        try {
            workspace.RequestStop();
        } catch (...) {
        }
    }
    return false;
}

template <typename TContext, typename TWorkspace>
inline void FailStage(
    TContext& context,
    TWorkspace& workspace,
    const std::string_view stageName,
    const CodecErrorCode code,
    const std::string_view message) noexcept {
    (void)RecordFailureAndStop(context, workspace, stageName, code, message);
}

} // namespace datacodec

#endif
