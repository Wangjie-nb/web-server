#include "thread_pool.h"

ThreadPool::ThreadPool(int numThreads) : stop(false) {
    for (int i = 0; i < numThreads; i++) {
        workers.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->mtx);
                    this->cv.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    if (this->stop && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();
    for (std::thread& worker : workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(mtx);
        tasks.emplace(task);
    }
    cv.notify_one();
}
