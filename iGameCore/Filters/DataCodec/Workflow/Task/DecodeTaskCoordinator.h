#ifndef DATACODEC_WORKFLOW_TASK_DECODETASKCOORDINATOR_H
#define DATACODEC_WORKFLOW_TASK_DECODETASKCOORDINATOR_H

#include "DataCodec/Runtime/Execution/ParallelExecution.h"
#include "DataCodec/Workflow/Task/DecodeTaskTypes.h"

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <unordered_map>
#include <utility>
#include <vector>

namespace datacodec {

template<typename Result>
class DecodeTaskCoordinator final {
private:
    struct Entry;
    struct Interest;

public:
    using Task = std::function<Result(std::stop_token)>;

    class Handle final {
    public:
        Handle() = default;

        [[nodiscard]] bool Valid() const noexcept { return m_entry != nullptr; }

        [[nodiscard]] const DecodeTaskKey* Key() const noexcept {
            return m_entry != nullptr ? &m_entry->key : nullptr;
        }

        [[nodiscard]] DecodeTaskState State() const noexcept {
            if (m_entry == nullptr) {
                return DecodeTaskState::Cancelled;
            }
            std::lock_guard<std::mutex> lock(m_entry->mutex);
            return m_entry->state;
        }

        [[nodiscard]] bool Wait(Result& result, const std::stop_token stopToken = {}) const {
            if (m_entry == nullptr) {
                return false;
            }
            std::unique_lock<std::mutex> lock(m_entry->mutex);
            if (!m_entry->condition.wait(
                    lock,
                    stopToken,
                    [this]() { return IsTerminalDecodeTaskState(m_entry->state); })) {
                return false;
            }
            if (m_entry->exception != nullptr) {
                std::rethrow_exception(m_entry->exception);
            }
            if (!m_entry->result.has_value()) {
                return false;
            }
            result = *m_entry->result;
            return true;
        }

    private:
        friend class DecodeTaskCoordinator<Result>;

        Handle(std::shared_ptr<Entry> entry, std::shared_ptr<Interest> interest)
            : m_entry(std::move(entry)), m_interest(std::move(interest)) {}

        std::shared_ptr<Entry> m_entry;
        std::shared_ptr<Interest> m_interest;
    };

    explicit DecodeTaskCoordinator(std::shared_ptr<IParallelTaskRunner> taskRunner = {})
        : m_taskRunner(std::move(taskRunner)) {
        if (m_taskRunner == nullptr) {
            throw std::invalid_argument("decode task coordinator requires a task runner");
        }
    }

    DecodeTaskCoordinator(const DecodeTaskCoordinator&) = delete;
    DecodeTaskCoordinator& operator=(const DecodeTaskCoordinator&) = delete;

    ~DecodeTaskCoordinator() { Stop(); }

    [[nodiscard]] Handle Submit(const DecodeTaskKey& key, Task task) {
        return SubmitImpl(key, std::move(task), false);
    }

    [[nodiscard]] Handle SubmitInline(const DecodeTaskKey& key, Task task) {
        return SubmitImpl(key, std::move(task), true);
    }

    void CancelAll() noexcept {
        std::vector<std::shared_ptr<Entry>> entries;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            entries.reserve(m_entries.size());
            for (const auto& [key, entry] : m_entries) {
                (void) key;
                entries.push_back(entry);
            }
        }
        for (const auto& entry : entries) {
            if (entry != nullptr) {
                entry->stopSource.request_stop();
            }
        }
    }

    void WaitIdle() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_idle.wait(lock, [this]() { return m_entries.empty(); });
    }

    [[nodiscard]] std::size_t Concurrency() const noexcept {
        return m_taskRunner->Concurrency();
    }

    [[nodiscard]] std::size_t InFlightTaskCount() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries.size();
    }

private:
    [[nodiscard]] Handle SubmitImpl(const DecodeTaskKey& key, Task task, const bool executeInline) {
        if (!task) {
            throw std::invalid_argument("decode task must not be empty");
        }

        std::shared_ptr<Entry> entry;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopping) {
                throw std::runtime_error("decode task coordinator is stopping");
            }
            const auto iterator = m_entries.find(key);
            if (iterator != m_entries.end()) {
                entry = iterator->second;
                bool replaceStoppedEntry = false;
                {
                    std::lock_guard<std::mutex> entryLock(entry->mutex);
                    replaceStoppedEntry = entry->interestCount == 0u &&
                        entry->stopSource.stop_requested() &&
                        !IsTerminalDecodeTaskState(entry->state);
                    if (!replaceStoppedEntry) {
                        ++entry->interestCount;
                    }
                }
                if (!replaceStoppedEntry) {
                    return Handle(entry, std::make_shared<Interest>(entry));
                }
                m_entries.erase(iterator);
                entry.reset();
            }

            entry = std::make_shared<Entry>();
            entry->key = key;
            entry->task = std::move(task);
            entry->interestCount = 1u;
            entry->state = DecodeTaskState::Queued;
            m_entries.emplace(key, entry);
        }

        try {
            {
                std::lock_guard<std::mutex> entryLock(entry->mutex);
                entry->state = DecodeTaskState::Running;
            }
            if (executeInline) {
                // 前台调用保持任务去重并在调用线程执行，内部阶段可使用完整外部执行资源
                ExecuteEntry(entry);
            } else {
                entry->taskGroup = m_taskRunner->CreateGroup(entry->stopSource.get_token());
                if (entry->taskGroup == nullptr) {
                    throw std::runtime_error("decode task coordinator task group is unavailable");
                }
                entry->taskGroup->Submit([this, entry]() { ExecuteEntry(entry); });
            }
        } catch (...) {
            CompleteEntry(entry, std::nullopt, std::current_exception());
        }
        return Handle(entry, std::make_shared<Interest>(entry));
    }
    struct Entry {
        DecodeTaskKey key;
        DecodeTaskState state{DecodeTaskState::Queued};
        Task task;
        std::stop_source stopSource;
        std::optional<Result> result;
        std::exception_ptr exception;
        std::unique_ptr<IParallelTaskGroup> taskGroup;
        mutable std::mutex mutex;
        std::condition_variable_any condition;
        std::size_t interestCount{0u};
    };

    struct Interest final {
        explicit Interest(std::shared_ptr<Entry> target) : entry(std::move(target)) {}

        ~Interest() {
            if (entry == nullptr) {
                return;
            }
            bool requestStop = false;
            {
                std::lock_guard<std::mutex> lock(entry->mutex);
                if (entry->interestCount > 0u) {
                    --entry->interestCount;
                }
                requestStop = entry->interestCount == 0u &&
                    !IsTerminalDecodeTaskState(entry->state);
            }
            if (requestStop) {
                entry->stopSource.request_stop();
            }
        }

        std::shared_ptr<Entry> entry;
    };

    void ExecuteEntry(const std::shared_ptr<Entry>& entry) noexcept {
        std::optional<Result> result;
        std::exception_ptr exception;
        try {
            result.emplace(entry->task(entry->stopSource.get_token()));
        } catch (...) {
            exception = std::current_exception();
        }
        CompleteEntry(entry, std::move(result), std::move(exception));
    }

    void CompleteEntry(
        const std::shared_ptr<Entry>& entry,
        std::optional<Result> result,
        std::exception_ptr exception) noexcept {
        {
            std::lock_guard<std::mutex> lock(entry->mutex);
            entry->result = std::move(result);
            entry->exception = std::move(exception);
            if (entry->exception != nullptr) {
                entry->state = DecodeTaskState::Failed;
            } else if (entry->stopSource.stop_requested()) {
                entry->state = DecodeTaskState::Cancelled;
            } else {
                entry->state = DecodeTaskState::Succeeded;
            }
        }
        entry->condition.notify_all();

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            const auto iterator = m_entries.find(entry->key);
            if (iterator != m_entries.end() && iterator->second == entry) {
                m_entries.erase(iterator);
            }
            if (m_entries.empty()) {
                m_idle.notify_all();
            }
        }
    }

    void Stop() noexcept {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopping) {
                return;
            }
            m_stopping = true;
        }
        CancelAll();
        WaitIdle();
    }

    std::shared_ptr<IParallelTaskRunner> m_taskRunner;
    mutable std::mutex m_mutex;
    std::condition_variable m_idle;
    std::unordered_map<DecodeTaskKey, std::shared_ptr<Entry>, DecodeTaskKeyHash> m_entries;
    bool m_stopping{false};
};

} // namespace datacodec

#endif
