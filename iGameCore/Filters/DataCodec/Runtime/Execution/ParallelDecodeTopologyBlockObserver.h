#ifndef DATACODEC_RUNTIME_EXECUTION_PARALLELDECODETOPOLOGYBLOCKOBSERVER_H
#define DATACODEC_RUNTIME_EXECUTION_PARALLELDECODETOPOLOGYBLOCKOBSERVER_H

#include "DataCodec/API/Adapter/IDecodeTopologyBlockObserver.h"
#include "DataCodec/Runtime/Execution/ParallelExecution.h"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace datacodec {

struct DecodeTopologyBlockObserverStats {
    std::size_t observedBlockCount{0u};
    std::size_t completedBlockCount{0u};
};

class ParallelDecodeTopologyBlockObserver final : public IDecodeTopologyBlockObserver {
public:
    using BeginHandler = std::function<bool(
        const ConnectivityTopologyDecodeInfo&,
        std::size_t,
        std::string*)>;
    using BlockHandler = std::function<bool(
        std::size_t,
        DecodedConnectivityTopologyBlock,
        std::string*)>;
    using EndHandler = std::function<bool(std::string*)>;

    struct Options {
        std::shared_ptr<IParallelTaskRunner> taskRunner;
        std::size_t workerCount{1u};
        std::size_t maxPendingBlockCount{2u};
    };

    ParallelDecodeTopologyBlockObserver(
        Options options,
        BeginHandler beginHandler,
        BlockHandler blockHandler,
        EndHandler endHandler = {})
        : m_options(std::move(options)),
          m_beginHandler(std::move(beginHandler)),
          m_blockHandler(std::move(blockHandler)),
          m_endHandler(std::move(endHandler)) {
        m_options.workerCount = ClampDataCodecWorkerCount(m_options.workerCount);
        m_options.maxPendingBlockCount = std::max<std::size_t>(
            m_options.maxPendingBlockCount,
            m_options.workerCount);
    }

    ~ParallelDecodeTopologyBlockObserver() override {
        std::unique_ptr<IParallelTaskGroup> taskGroup;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            taskGroup = std::move(m_taskGroup);
        }
        try {
            if (taskGroup != nullptr) {
                taskGroup->Wait();
            }
        } catch (...) {
        }
    }

    bool BeginConnectivityTopology(
        const ConnectivityTopologyDecodeInfo& info,
        std::string* error = nullptr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_failed = false;
        m_finished = false;
        m_error.clear();
        m_pendingBlockCount = 0u;
        m_stats = {};
        try {
            if (m_options.taskRunner == nullptr || !m_blockHandler) {
                FailLocked("parallel topology observer is not configured");
                return CopyStatusLocked(error);
            }
            if (m_beginHandler && !m_beginHandler(info, m_options.workerCount, &m_error)) {
                FailLocked(m_error.empty()
                    ? "parallel topology observer initialization failed"
                    : std::move(m_error));
                return CopyStatusLocked(error);
            }
            m_taskGroup = m_options.taskRunner->CreateGroup();
            if (m_taskGroup == nullptr) {
                FailLocked("parallel topology observer task group is unavailable");
            }
        } catch (const std::exception& exception) {
            FailLocked(std::string("parallel topology observer initialization failed: ") + exception.what());
        } catch (...) {
            FailLocked("parallel topology observer initialization failed");
        }
        return CopyStatusLocked(error);
    }

    bool ObserveConnectivityBlock(
        DecodedConnectivityTopologyBlock block,
        std::string* error = nullptr) override {
        std::size_t workerIndex = 0u;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_available.wait(lock, [this]() {
                return m_failed ||
                    m_pendingBlockCount < m_options.maxPendingBlockCount;
            });
            if (m_failed || m_taskGroup == nullptr) {
                return CopyStatusLocked(error);
            }
            workerIndex = block.blockIndex % m_options.workerCount;
            ++m_pendingBlockCount;
            ++m_stats.observedBlockCount;
        }

        try {
            m_taskGroup->Submit(
                [this, workerIndex, block = std::move(block)]() mutable {
                    std::string error;
                    bool success = false;
                    try {
                        success = m_blockHandler(
                            workerIndex,
                            std::move(block),
                            &error);
                    } catch (const std::exception& exception) {
                        error = std::string("parallel topology block handler failed: ") + exception.what();
                    } catch (...) {
                        error = "parallel topology block handler failed";
                    }
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        if (success) {
                            ++m_stats.completedBlockCount;
                        } else {
                            FailLocked(error.empty()
                                ? "parallel topology block handler failed"
                                : std::move(error));
                        }
                        --m_pendingBlockCount;
                    }
                    m_available.notify_all();
                });
        } catch (const std::exception& exception) {
            std::lock_guard<std::mutex> lock(m_mutex);
            FailLocked(std::string("parallel topology task submission failed: ") + exception.what());
            --m_pendingBlockCount;
            m_available.notify_all();
        } catch (...) {
            std::lock_guard<std::mutex> lock(m_mutex);
            FailLocked("parallel topology task submission failed");
            --m_pendingBlockCount;
            m_available.notify_all();
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        return CopyStatusLocked(error);
    }

    bool EndConnectivityTopology(std::string* error = nullptr) override {
        std::unique_ptr<IParallelTaskGroup> taskGroup;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            taskGroup = std::move(m_taskGroup);
        }
        try {
            if (taskGroup != nullptr) {
                taskGroup->Wait();
            }
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_failed && m_endHandler && !m_endHandler(&m_error)) {
                FailLocked(m_error.empty()
                    ? "parallel topology observer finalization failed"
                    : std::move(m_error));
            }
            m_finished = true;
        } catch (const std::exception& exception) {
            std::lock_guard<std::mutex> lock(m_mutex);
            FailLocked(std::string("parallel topology observer finalization failed: ") + exception.what());
            m_finished = true;
        } catch (...) {
            std::lock_guard<std::mutex> lock(m_mutex);
            FailLocked("parallel topology observer finalization failed");
            m_finished = true;
        }
        m_available.notify_all();
        std::lock_guard<std::mutex> lock(m_mutex);
        return CopyStatusLocked(error);
    }

    [[nodiscard]] bool Succeeded() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_finished && !m_failed;
    }

    [[nodiscard]] std::string Error() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_error;
    }

    [[nodiscard]] DecodeTopologyBlockObserverStats Stats() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stats;
    }

private:
    [[nodiscard]] bool CopyStatusLocked(std::string* error) const {
        if (!m_failed) {
            return true;
        }
        if (error != nullptr) {
            *error = m_error;
        }
        return false;
    }

    void FailLocked(std::string error) {
        m_failed = true;
        if (m_error.empty()) {
            m_error = std::move(error);
        }
    }

    Options m_options;
    BeginHandler m_beginHandler;
    BlockHandler m_blockHandler;
    EndHandler m_endHandler;
    mutable std::mutex m_mutex;
    std::condition_variable m_available;
    std::unique_ptr<IParallelTaskGroup> m_taskGroup;
    std::size_t m_pendingBlockCount{0u};
    DecodeTopologyBlockObserverStats m_stats;
    bool m_failed{false};
    bool m_finished{false};
    std::string m_error;
};

} // namespace datacodec

#endif
