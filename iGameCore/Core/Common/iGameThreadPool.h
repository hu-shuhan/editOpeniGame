#ifndef iGameThreadPool_h
#define iGameThreadPool_h

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
class ThreadPool : public Object {
public:
    I_OBJECT(ThreadPool);
    // 单例
    static Pointer Instance() {
        static Pointer ins = new ThreadPool;
        return ins;
    }

    // 静态方法用于设置默认线程数量
    static void SetDefaultThreadCount(int numThreads) { defaultThreadCount = numThreads; }

    // 获取默认线程数量
    static int GetDefaultThreadCount() { return defaultThreadCount; }
    // static parallelFor 函数，numThreads 默认值为 12
    // numThreads可人为调节，
    // 调用的是func(int start,int end)，处理start到end的部分
    // parallelFor 函数
    template<typename Func>
    static void parallelFor(int start, int end, Func&& process, int numThreads = GetDefaultThreadCount()) {
        int range = end - start;
        int chunkSize = range / numThreads;
        if (range < numThreads) {
            numThreads = range;
            chunkSize = 1;
        }
        // std::cout << "The number of threads uesd  is " << numThreads << '\n';
        std::vector<std::future<void>> futures;
        for (int i = 0; i < numThreads; ++i) {
            int chunkStart = start + i * chunkSize;
            int chunkEnd = (i == numThreads - 1) ? end : chunkStart + chunkSize;
            if (chunkStart == chunkEnd) continue;
            // 使用线程池提交任务
            futures.emplace_back(ThreadPool::Instance()->Commit([=]() { process(chunkStart, chunkEnd); }));
        }
        // 等待所有任务完成
        for (auto& future: futures) { future.get(); }
    }

    // static parallelFor 函数，numThreads 默认值为 12,
    //maxThreadSize 表示的是该func允许的最大线程数量，由用户在自定义程序里面设定
    //这个模式调用的是func(int start,int end,int id), id用于在每个func中开辟独立的数据空间，因此id不能>=maxThreadSize
    template<typename Func>
    static void parallelFor(int start, int end, int maxThreadSize, Func&& process,
                            int numThreads = GetDefaultThreadCount()) {
        if (numThreads > maxThreadSize) numThreads = maxThreadSize;
        int range = end - start;
        int chunkSize = range / numThreads;
        if (range < numThreads) {
            numThreads = range;
            chunkSize = 1;
        }
        // std::cout << "The number of threads uesd  is " << numThreads << '\n';
        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i) {
            int chunkStart = start + i * chunkSize;
            int chunkEnd = (i == numThreads - 1) ? end : chunkStart + chunkSize;
            if (chunkStart == chunkEnd) continue;
            // 创建线程执行任务
            threads.emplace_back([=]() { process(chunkStart, chunkEnd, i); });
        }
        // 等待所有线程完成
        for (auto& thread: threads) {
            if (thread.joinable()) { thread.join(); }
        }
    }
    using Task = std::packaged_task<void()>;

    template<class F, class... Args>
    auto Commit(F&& f, Args&&... args) -> std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))> {
        using RetType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
        if (stop_.load()) return std::future<RetType>{};

        auto task = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<RetType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> cv_mt(cv_mt_);
            tasks_.emplace([task] { (*task)(); });
        }
        cv_lock_.notify_one();
        return ret;
    }
    int IdleThreadCount() { return thread_num_; }

private:
    ThreadPool(unsigned int num = std::thread::hardware_concurrency()) : stop_(false) {
        if (num <= 1) thread_num_ = 2;
        else
            thread_num_ = num;
        this->Start();
    }

    ~ThreadPool() { this->Stop(); }

    void Start();

    void Stop();

private:
    std::mutex cv_mt_;
    std::condition_variable cv_lock_;
    std::atomic_bool stop_;                  // 线程池是否退出
    std::atomic_int thread_num_;             // 空闲的线程数
    std::queue<Task> tasks_;         // 任务队列
	std::vector<std::thread> pool_;          // 线程队列

	// 静态变量存储默认线程数量
	static int defaultThreadCount;
};

IGAME_NAMESPACE_END
#endif