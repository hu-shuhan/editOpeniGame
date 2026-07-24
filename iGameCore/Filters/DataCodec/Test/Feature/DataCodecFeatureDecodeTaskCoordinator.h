#ifndef DATACODEC_TEST_FEATURE_DATACODECFEATUREDECODETASKCOORDINATOR_H
#define DATACODEC_TEST_FEATURE_DATACODECFEATUREDECODETASKCOORDINATOR_H

#include "DataCodec/Workflow/Task/DecodeTaskCoordinator.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

namespace datacodec::test::feature_decode_task_coordinator
{

class TestParallelTaskGroup final : public IParallelTaskGroup {
public:
    explicit TestParallelTaskGroup(std::stop_token) {}

    void Submit(std::function<void()> task) override {
        if (!task) {
            return;
        }
        m_workers.emplace_back([task = std::move(task)]() mutable {
            task();
        });
    }

    void Wait() override {
        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        m_workers.clear();
    }

    ~TestParallelTaskGroup() override { Wait(); }

private:
    std::vector<std::jthread> m_workers;
};

class TestParallelTaskRunner final : public IParallelTaskRunner {
public:
    [[nodiscard]] std::unique_ptr<IParallelTaskGroup> CreateGroup(
        const std::stop_token stopToken = {}) override {
        m_groupCount.fetch_add(1u, std::memory_order_relaxed);
        return std::make_unique<TestParallelTaskGroup>(stopToken);
    }

    [[nodiscard]] std::size_t Concurrency() const noexcept override { return 2u; }

    [[nodiscard]] std::uint32_t GroupCount() const noexcept {
        return m_groupCount.load(std::memory_order_relaxed);
    }

private:
    std::atomic_uint32_t m_groupCount{0u};
};

class UnavailableTaskGroupRunner final : public IParallelTaskRunner {
public:
    [[nodiscard]] std::unique_ptr<IParallelTaskGroup> CreateGroup(
        std::stop_token = {}) override {
        return nullptr;
    }

    [[nodiscard]] std::size_t Concurrency() const noexcept override { return 2u; }
};

inline bool TestDuplicateTargetUsesSingleTask() {
    auto runner = std::make_shared<TestParallelTaskRunner>();
    DecodeTaskCoordinator<int> coordinator(runner);
    std::atomic_uint32_t executionCount{0u};
    std::mutex mutex;
    std::condition_variable condition;
    bool started = false;
    bool released = false;
    const auto task = [&](const std::stop_token stopToken) {
        executionCount.fetch_add(1u, std::memory_order_relaxed);
        std::unique_lock<std::mutex> lock(mutex);
        started = true;
        condition.notify_all();
        condition.wait(lock, [&]() { return released || stopToken.stop_requested(); });
        return stopToken.stop_requested() ? -1 : 42;
    };
    const DecodeTaskKey key{
        .scope = 1u,
        .frameIndex = 7u,
        .variant = "all-attributes",
    };
    const auto first = coordinator.Submit(key, task);
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (!condition.wait_for(lock, std::chrono::seconds(2), [&]() { return started; })) {
            released = true;
            condition.notify_all();
            return false;
        }
    }
    const auto second = coordinator.Submit(key, task);
    {
        std::lock_guard<std::mutex> lock(mutex);
        released = true;
    }
    condition.notify_all();

    int firstResult = 0;
    int secondResult = 0;
    return first.Wait(firstResult) && second.Wait(secondResult) && firstResult == 42 &&
           secondResult == 42 && executionCount.load(std::memory_order_relaxed) == 1u &&
           runner->GroupCount() == 1u;
}

inline bool TestIndependentTargetsUseExternalRunner() {
    auto runner = std::make_shared<TestParallelTaskRunner>();
    DecodeTaskCoordinator<std::uint32_t> coordinator(runner);
    std::mutex mutex;
    std::condition_variable condition;
    std::size_t startedCount = 0u;
    bool release = false;

    const auto makeTask = [&](const std::uint32_t value) {
        return [&, value](const std::stop_token stopToken) {
            std::unique_lock<std::mutex> lock(mutex);
            ++startedCount;
            condition.notify_all();
            condition.wait(lock, [&]() { return release || stopToken.stop_requested(); });
            return value;
        };
    };

    const auto first = coordinator.Submit({.frameIndex = 10u}, makeTask(10u));
    const auto second = coordinator.Submit({.frameIndex = 20u}, makeTask(20u));
    bool bothStarted = false;
    {
        std::unique_lock<std::mutex> lock(mutex);
        bothStarted = condition.wait_for(lock, std::chrono::seconds(2), [&]() { return startedCount == 2u; });
        release = true;
    }
    condition.notify_all();

    std::uint32_t firstResult = 0u;
    std::uint32_t secondResult = 0u;
    return bothStarted && first.Wait(firstResult) && second.Wait(secondResult) &&
           firstResult == 10u && secondResult == 20u && runner->GroupCount() == 2u;
}

inline bool TestMissingRunnerIsRejected() {
    try {
        DecodeTaskCoordinator<std::uint32_t> coordinator;
        (void)coordinator;
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

inline bool TestUnavailableTaskGroupIsRejected() {
    auto runner = std::make_shared<UnavailableTaskGroupRunner>();
    DecodeTaskCoordinator<std::uint32_t> coordinator(runner);
    const auto task = coordinator.Submit(
        {.frameIndex = 8u},
        [](std::stop_token) { return 8u; });
    std::uint32_t result = 0u;
    try {
        (void)task.Wait(result);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

inline bool TestTaskGroupDoesNotDropAcceptedTaskAfterStop() {
    TestParallelTaskRunner runner;
    std::stop_source stopSource;
    stopSource.request_stop();
    auto group = runner.CreateGroup(stopSource.get_token());
    std::atomic_uint32_t executionCount{0u};
    group->Submit([&executionCount]() {
        executionCount.fetch_add(1u, std::memory_order_relaxed);
    });
    group->Wait();
    return executionCount.load(std::memory_order_relaxed) == 1u;
}

inline bool TestTaskGroupDestructorWaitsForAcceptedTask() {
    TestParallelTaskRunner runner;
    std::atomic_uint32_t executionCount{0u};
    {
        auto group = runner.CreateGroup();
        group->Submit([&executionCount]() {
            executionCount.fetch_add(1u, std::memory_order_relaxed);
        });
    }
    return executionCount.load(std::memory_order_relaxed) == 1u;
}

inline bool TestInlineSubmissionUsesCallerThread() {
    auto runner = std::make_shared<TestParallelTaskRunner>();
    DecodeTaskCoordinator<std::thread::id> coordinator(runner);
    const auto callerThread = std::this_thread::get_id();
    const auto task = coordinator.SubmitInline(
        {.frameIndex = 4u},
        [](const std::stop_token) {
            return std::this_thread::get_id();
        });
    std::thread::id taskThread;
    return task.Wait(taskThread) && taskThread == callerThread && runner->GroupCount() == 0u;
}

} // namespace datacodec::test::feature_decode_task_coordinator

namespace datacodec::test
{

inline int RunDataCodecFeatureDecodeTaskCoordinator() {
    using namespace feature_decode_task_coordinator;
    if (!TestDuplicateTargetUsesSingleTask() || !TestIndependentTargetsUseExternalRunner() ||
        !TestMissingRunnerIsRejected() || !TestUnavailableTaskGroupIsRejected() ||
        !TestTaskGroupDoesNotDropAcceptedTaskAfterStop() ||
        !TestTaskGroupDestructorWaitsForAcceptedTask() ||
        !TestInlineSubmissionUsesCallerThread()) {
        std::cerr << "DataCodec decode task coordinator feature test failed\n";
        return 1;
    }
    std::cout << "DataCodec decode task coordinator feature test passed\n";
    return 0;
}

} // namespace datacodec::test

#endif
