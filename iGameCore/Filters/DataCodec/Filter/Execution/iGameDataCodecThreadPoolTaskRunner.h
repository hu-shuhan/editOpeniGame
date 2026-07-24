#ifndef iGameDataCodecThreadPoolTaskRunner_h
#define iGameDataCodecThreadPoolTaskRunner_h

#include "DataCodec/Runtime/Execution/DataCodecExecutionResources.h"
#include "iGameThreadPool.h"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <stdexcept>
#include <utility>

IGAME_NAMESPACE_BEGIN

class DataCodecThreadPoolTaskGroup final : public ::datacodec::IParallelTaskGroup {
public:
    explicit DataCodecThreadPoolTaskGroup(std::stop_token) {}

    ~DataCodecThreadPoolTaskGroup() override {
        try {
            Wait();
        } catch (...) {
        }
    }

    void Submit(std::function<void()> task) override {
        if (!task) {
            return;
        }
        std::size_t workerIndex = 0u;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_pendingTasks;
            workerIndex = m_nextWorkerIndex++;
        }
        bool callerOwnsPendingCount = true;
        try {
            auto future = ThreadPool::Instance()->Commit(
                [this, task = std::move(task), workerIndex]() mutable {
                    try {
                        ::datacodec::ParallelWorkerIndexScope workerIndexScope(workerIndex);
                        task();
                    } catch (...) {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        if (m_firstException == nullptr) {
                            m_firstException = std::current_exception();
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        if (m_pendingTasks > 0u) {
                            --m_pendingTasks;
                        }
                    }
                    m_completed.notify_all();
                });
            if (!future.valid()) {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_pendingTasks > 0u) {
                        --m_pendingTasks;
                    }
                    if (m_firstException == nullptr) {
                        m_firstException = std::make_exception_ptr(
                            std::runtime_error("iGame thread pool rejected a DataCodec task"));
                    }
                }
                callerOwnsPendingCount = false;
                m_completed.notify_all();
                throw std::runtime_error("iGame thread pool rejected a DataCodec task");
            }
            callerOwnsPendingCount = false;
        } catch (...) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (callerOwnsPendingCount && m_pendingTasks > 0u) {
                    --m_pendingTasks;
                }
                if (m_firstException == nullptr) {
                    m_firstException = std::current_exception();
                }
            }
            m_completed.notify_all();
            throw;
        }
    }

    void Wait() override {
        std::exception_ptr exception;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_completed.wait(lock, [this]() { return m_pendingTasks == 0u; });
            exception = m_firstException;
        }
        if (exception != nullptr) {
            std::rethrow_exception(exception);
        }
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_completed;
    std::size_t m_pendingTasks{0u};
    std::size_t m_nextWorkerIndex{0u};
    std::exception_ptr m_firstException;
};

class DataCodecThreadPoolTaskRunner final : public ::datacodec::IParallelTaskRunner {
public:
    [[nodiscard]] std::unique_ptr<::datacodec::IParallelTaskGroup> CreateGroup(
        const std::stop_token stopToken = {}) override {
        return std::make_unique<DataCodecThreadPoolTaskGroup>(stopToken);
    }

    [[nodiscard]] std::size_t Concurrency() const noexcept override {
#if IGAME_EMSCRIPTEN_SINGLE_THREAD
        return 1u;
#else
        return std::max<std::size_t>(ThreadPool::Instance()->WorkerCount(), 1u);
#endif
    }
};

inline std::shared_ptr<::datacodec::IParallelTaskRunner> DataCodecTaskRunner() {
    static auto runner = std::make_shared<DataCodecThreadPoolTaskRunner>();
    return runner;
}

inline ::datacodec::DataCodecExecutionResources MakeDataCodecExecutionResources() {
    return ::datacodec::DataCodecExecutionResources{
        .parallelTaskRunner = DataCodecTaskRunner().get(),
    };
}

IGAME_NAMESPACE_END

#endif
