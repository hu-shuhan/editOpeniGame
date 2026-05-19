#include "iGameThreadPool.h"
IGAME_NAMESPACE_BEGIN

// 初始化静态变量
int ThreadPool::defaultThreadCount = 12; // 默认值为12

void ThreadPool::Start() {
#if IGAME_EMSCRIPTEN_SINGLE_THREAD
    return;
#endif
    for (int i = 0; i < thread_num_; ++i) {
        pool_.emplace_back([this]() {
            // 如果停止则退出循环
            while (!this->stop_.load()) { // while(stop_ == false)
                Task task;
                {
                    std::unique_lock<std::mutex> cv_mt(cv_mt_);
                    // 如果stop == false && tasks_.empty()，条件变量wait不会返回，线程将挂起。
                    this->cv_lock_.wait(cv_mt, [this] { return this->stop_.load() || !this->tasks_.empty(); });
                    if (this->tasks_.empty()) return;

                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                this->thread_num_--;
                task();
                this->thread_num_++;
            }
        });
    }
}

void ThreadPool::Stop() {
    {
        stop_.store(true);
        cv_lock_.notify_all();
        for (auto& td: pool_) {
            if (td.joinable()) {
                //std::cout << "join thread " << td.get_id() << std::endl;
                td.join();
            }
        }
    }
}
IGAME_NAMESPACE_END
