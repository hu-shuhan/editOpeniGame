#ifndef iGameMeshCodecThread_h
#define iGameMeshCodecThread_h

// 定义此宏启用单线程模式
// #define IGAME_CODEC_SINGLE_THREAD

#include "iGameThreadPool.h"
#include <future>
#include <vector>

IGAME_NAMESPACE_BEGIN

#ifdef IGAME_CODEC_SINGLE_THREAD

inline int GetCodecThreadCount(int = 1) { return 1; }

class CodecThreadPool {
public:
    static CodecThreadPool* Instance() {
        static CodecThreadPool ins;
        return &ins;
    }

    static int GetDefaultThreadCount() { return 1; }
    static void SetDefaultThreadCount(int) {}

    template<typename Func>
    static void parallelFor(int start, int end, Func&& process, int = 1) {
        process(start, end);
    }

    template<typename Func>
    static void parallelFor(int start, int end, int, Func&& process, int = 1) {
        process(start, end, 0);
    }

    template<class F, class... Args>
    auto Commit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using RetType = decltype(f(args...));
        std::promise<RetType> promise;
        auto future = promise.get_future();
        if constexpr (std::is_void_v<RetType>) {
            f(std::forward<Args>(args)...);
            promise.set_value();
        } else {
            promise.set_value(f(std::forward<Args>(args)...));
        }
        return future;
    }

private:
    CodecThreadPool() = default;
};

template<typename Owner, typename Func>
inline void CodecProgressParallelFor(Owner* owner, int start, int end, float startProgress, float endProgress,
                                     Func&& process, int = 1) {
    if (!owner || start >= end) { return; }
    process(start, end);
    owner->UpdateProgress(endProgress);
}

#else

inline int GetCodecThreadCount(int requested = ThreadPool::GetDefaultThreadCount()) { return requested; }

class CodecThreadPool {
public:
    static ThreadPool::Pointer Instance() {
        return ThreadPool::Instance();
    }

    static int GetDefaultThreadCount() {
        return ThreadPool::GetDefaultThreadCount();
    }

    static void SetDefaultThreadCount(int n) {
        ThreadPool::SetDefaultThreadCount(n);
    }

    template<typename Func>
    static void parallelFor(int start, int end, Func&& process, int numThreads = GetDefaultThreadCount()) {
        ThreadPool::parallelFor(start, end, std::forward<Func>(process), numThreads);
    }

    template<typename Func>
    static void parallelFor(int start, int end, int maxThreadSize, Func&& process, int numThreads = GetDefaultThreadCount()) {
        ThreadPool::parallelFor(start, end, maxThreadSize, std::forward<Func>(process), numThreads);
    }
};

template<typename Owner, typename Func>
inline void CodecProgressParallelFor(Owner* owner, int start, int end, float startProgress, float endProgress,
                                     Func&& process,
                                     int numThreads = CodecThreadPool::GetDefaultThreadCount()) {
    if (!owner || start >= end) { return; }

    int range = end - start;
    int chunkSize = range / numThreads;
    if (range < numThreads) {
        numThreads = range;
        chunkSize = 1;
    }

    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<size_t>(numThreads));

    for (int i = 0; i < numThreads; ++i) {
        int chunkStart = start + i * chunkSize;
        int chunkEnd = (i == numThreads - 1) ? end : chunkStart + chunkSize;
        if (chunkStart == chunkEnd) continue;

        futures.emplace_back(ThreadPool::Instance()->Commit([=]() { process(chunkStart, chunkEnd); }));
    }

    for (size_t i = 0; i < futures.size(); ++i) {
        futures[i].get();
        const float progress = startProgress +
                               (static_cast<float>(i + 1) / static_cast<float>(futures.size())) *
                                       (endProgress - startProgress);
        owner->UpdateProgress(progress);
    }
}

#endif

IGAME_NAMESPACE_END

#endif
