#ifndef DATACODEC_RUNTIME_EXECUTION_PARALLELEXECUTION_H
#define DATACODEC_RUNTIME_EXECUTION_PARALLELEXECUTION_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <stop_token>
#include <stdexcept>
#include <utility>

namespace datacodec {

inline constexpr std::size_t kInvalidParallelWorkerIndex = std::numeric_limits<std::size_t>::max();
inline thread_local std::size_t g_currentParallelWorkerIndex = kInvalidParallelWorkerIndex;

inline std::size_t CurrentParallelWorkerIndex() noexcept {
    return g_currentParallelWorkerIndex;
}

class ParallelWorkerIndexScope final {
public:
    explicit ParallelWorkerIndexScope(const std::size_t workerIndex) noexcept
        : m_previous(g_currentParallelWorkerIndex) {
        g_currentParallelWorkerIndex = workerIndex;
    }

    ParallelWorkerIndexScope(const ParallelWorkerIndexScope&) = delete;
    ParallelWorkerIndexScope& operator=(const ParallelWorkerIndexScope&) = delete;

    ~ParallelWorkerIndexScope() {
        g_currentParallelWorkerIndex = m_previous;
    }

private:
    std::size_t m_previous{kInvalidParallelWorkerIndex};
};

inline std::size_t ClampDataCodecWorkerCount(std::size_t workerCount) noexcept {
    workerCount = std::max<std::size_t>(workerCount, 1u);
#if defined(DATACODEC_MAX_PARALLEL_WORKERS) && DATACODEC_MAX_PARALLEL_WORKERS > 0
    workerCount = std::min<std::size_t>(
        workerCount,
        static_cast<std::size_t>(DATACODEC_MAX_PARALLEL_WORKERS));
#endif
    return workerCount;
}

class IParallelTaskGroup {
public:
    // 实现析构前必须收束所有已接受任务
    virtual ~IParallelTaskGroup() = default;

    // Submit 必须执行已接受的任务，取消语义由任务自身处理
    virtual void Submit(std::function<void()> task) = 0;
    virtual void Wait() = 0;
};

class IParallelTaskRunner {
public:
    virtual ~IParallelTaskRunner() = default;

    [[nodiscard]] virtual std::unique_ptr<IParallelTaskGroup> CreateGroup(
        std::stop_token stopToken = {}) = 0;
    [[nodiscard]] virtual std::size_t Concurrency() const noexcept = 0;
};

class InlineParallelTaskGroup final : public IParallelTaskGroup {
public:
    explicit InlineParallelTaskGroup(std::stop_token = {}) {}

    void Submit(std::function<void()> task) override {
        if (!task) {
            return;
        }
        ParallelWorkerIndexScope workerIndexScope(0u);
        task();
    }

    void Wait() override {}
};

class InlineParallelTaskRunner final : public IParallelTaskRunner {
public:
    [[nodiscard]] std::unique_ptr<IParallelTaskGroup> CreateGroup(
        const std::stop_token stopToken = {}) override {
        return std::make_unique<InlineParallelTaskGroup>(stopToken);
    }

    [[nodiscard]] std::size_t Concurrency() const noexcept override { return 1u; }
};

inline std::size_t ResolveParallelTaskCount(
    const std::size_t taskCount,
    const IParallelTaskRunner* runner,
    const std::size_t workerLimit = 0u) noexcept {
    if (taskCount == 0u || runner == nullptr || CurrentParallelWorkerIndex() != kInvalidParallelWorkerIndex) {
        return taskCount == 0u ? 0u : 1u;
    }
    auto concurrency = ClampDataCodecWorkerCount(runner->Concurrency());
    if (workerLimit != 0u) {
        concurrency = std::min(concurrency, ClampDataCodecWorkerCount(workerLimit));
    }
    return std::min(taskCount, concurrency);
}

inline std::size_t ResolveNestedParallelTaskCount(
    const std::size_t taskCount,
    const IParallelTaskRunner* runner,
    const std::size_t workerLimit = 0u) noexcept {
    if (taskCount == 0u || runner == nullptr) {
        return taskCount == 0u ? 0u : 1u;
    }
    auto concurrency = ClampDataCodecWorkerCount(runner->Concurrency());
    if (workerLimit != 0u) {
        concurrency = std::min(concurrency, ClampDataCodecWorkerCount(workerLimit));
    }
    return std::min(taskCount, concurrency);
}

inline bool ShouldParallelizeRange(
    const std::size_t taskCount,
    const std::size_t minTaskCount,
    const IParallelTaskRunner* runner,
    const std::size_t workerLimit = 0u) noexcept {
    return taskCount >= minTaskCount &&
        ResolveParallelTaskCount(taskCount, runner, workerLimit) > 1u;
}

template<typename Func>
inline void ParallelForChunks(
    const std::size_t begin,
    const std::size_t end,
    Func&& process,
    IParallelTaskRunner* runner = nullptr,
    const std::size_t workerLimit = 0u,
    const std::stop_token stopToken = {}) {
    if (end <= begin || stopToken.stop_requested()) {
        return;
    }

    const auto taskCount = end - begin;
    const auto resolvedWorkerCount = ResolveParallelTaskCount(taskCount, runner, workerLimit);
    if (resolvedWorkerCount <= 1u) {
        process(begin, end);
        return;
    }

    const auto chunkSize = (taskCount + resolvedWorkerCount - 1u) / resolvedWorkerCount;
    auto group = runner->CreateGroup(stopToken);
    if (group == nullptr) {
        throw std::runtime_error("parallel task runner returned an unavailable task group");
    }
    for (std::size_t chunkBegin = begin; chunkBegin < end; chunkBegin += chunkSize) {
        const auto chunkEnd = std::min(end, chunkBegin + chunkSize);
        group->Submit([chunkBegin, chunkEnd, &process, stopToken]() {
            if (!stopToken.stop_requested()) {
                process(chunkBegin, chunkEnd);
            }
        });
    }
    group->Wait();
}

template<typename Func>
inline void ParallelForChunksThreshold(
    const std::size_t begin,
    const std::size_t end,
    const std::size_t minTaskCount,
    Func&& process,
    IParallelTaskRunner* runner = nullptr,
    const std::size_t workerLimit = 0u,
    const std::stop_token stopToken = {}) {
    if (end <= begin || stopToken.stop_requested()) {
        return;
    }

    if (!ShouldParallelizeRange(end - begin, minTaskCount, runner, workerLimit)) {
        process(begin, end);
        return;
    }

    ParallelForChunks(
        begin,
        end,
        std::forward<Func>(process),
        runner,
        workerLimit,
        stopToken);
}

template<typename Func>
inline void ParallelForChunksAllowNested(
    const std::size_t begin,
    const std::size_t end,
    Func&& process,
    IParallelTaskRunner* runner = nullptr,
    const std::size_t workerLimit = 0u,
    const std::stop_token stopToken = {}) {
    if (end <= begin || stopToken.stop_requested()) {
        return;
    }
    if (CurrentParallelWorkerIndex() == kInvalidParallelWorkerIndex) {
        ParallelForChunks(
            begin,
            end,
            std::forward<Func>(process),
            runner,
            workerLimit,
            stopToken);
        return;
    }
    const auto taskCount = end - begin;
    const auto workerCount = ResolveNestedParallelTaskCount(taskCount, runner, workerLimit);
    if (workerCount <= 1u) {
        process(begin, end);
        return;
    }

    const auto chunkSize = (taskCount + workerCount - 1u) / workerCount;
    auto group = runner->CreateGroup(stopToken);
    if (group == nullptr) {
        throw std::runtime_error("parallel task runner returned an unavailable task group");
    }
    const auto callerEnd = std::min(end, begin + chunkSize);
    for (std::size_t chunkBegin = callerEnd; chunkBegin < end; chunkBegin += chunkSize) {
        const auto chunkEnd = std::min(end, chunkBegin + chunkSize);
        group->Submit([chunkBegin, chunkEnd, &process, stopToken]() {
            if (!stopToken.stop_requested()) {
                process(chunkBegin, chunkEnd);
            }
        });
    }
    process(begin, callerEnd);
    group->Wait();
}

} // namespace datacodec

#endif
