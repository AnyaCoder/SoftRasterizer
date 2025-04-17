// include/core/threadpool.h
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

#define MultiThreading
// #undef MultiThreading

class ThreadPool {
public:
    
    ThreadPool(uint32_t numThreads = 1u);
    ~ThreadPool();
    void enqueue(std::function<void()>&& task);
    void waitForCompletion();
    int getNumThreads() const { return static_cast<int>(workers.size()); }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable completionCondition;
    std::atomic<bool> stop;
    std::atomic<size_t> activeTasks;
    void workerThread();
};