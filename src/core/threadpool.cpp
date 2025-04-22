// src/core/threadpool.cpp
#include "core/threadpool.h"
#include <iostream>

// Constructor: Initializes the thread pool with a specified number of threads
ThreadPool::ThreadPool(uint32_t numThreads) 
    : numThreads(numThreads), stop(false), activeTasks(0) {
    for (uint32_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

// Destructor: Cleans up by stopping the thread pool and joining all threads
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) {
        worker.join();
    }
}

// Enqueue a task (function) to be executed by the thread pool
void ThreadPool::enqueue(std::function<void()>&& task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) {
            throw std::runtime_error("Cannot enqueue task: ThreadPool is stopped");
        }
        tasks.emplace(std::forward<std::function<void()>>(task));
    }
    condition.notify_one();
}

// Wait until all enqueued tasks are completed
void ThreadPool::waitForCompletion() {
    std::unique_lock<std::mutex> lock(queueMutex);
    completionCondition.wait(lock, [this] { return activeTasks == 0 && tasks.empty(); });
}


// Worker thread function that processes tasks from the queue
void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        activeTasks++;
        task();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            activeTasks--;
            if (activeTasks == 0 && tasks.empty()) {
                completionCondition.notify_all(); // 通知所有任务完成
            }
        }
    }
}
