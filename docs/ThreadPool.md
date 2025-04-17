---
title: 优化软渲染器：线程池设计与性能提升
date: 2025-04-18
tags: [软渲染器, 线程池, 多线程, 性能优化, C++]
categories: 技术
---

在开发软渲染器时，性能优化是核心目标。单线程渲染受限于 CPU 单核性能，特别是在高分辨率或复杂场景下，帧率（FPS）往往较低。通过引入线程池（`ThreadPool`）并对关键模块进行多线程优化，我们成功将软渲染器的平均 FPS 从约 30 提升至 145（最大线程数 32，分辨率 800x800）。本文将详细介绍线程池的设计思路、实现方法，以及这些优化如何显著提升渲染性能。

## 背景

软渲染器是一个在 CPU 上运行的图形渲染系统，涉及顶点处理、光栅化、像素着色等计算密集型阶段。单线程实现中，平均 FPS 约为 30，难以满足实时渲染需求。性能瓶颈主要集中在：
- **像素填充**：逐像素的颜色和深度测试耗时长。
- **顶点处理与光栅化**：复杂模型的三角形处理计算量大。
- **纹理更新**：将帧缓冲区数据传输到 SDL 纹理的单线程操作效率低。

为解决这些问题，我们设计并实现了线程池，将计算密集型任务并行化，充分利用多核 CPU 的性能。以下将重点阐述线程池的设计与实现，以及其在软渲染器中的应用。

## 线程池的设计思路与实现方法

线程池是一种高效的多线程任务管理机制，通过复用固定数量的线程执行任务，避免频繁创建和销毁线程的开销。我们的线程池（`ThreadPool` 类）专为软渲染器的并行需求设计，核心目标是高效分配任务、确保线程安全，并提供任务完成同步机制。以下是线程池的详细设计思路和实现方法。

### 1. **设计思路**
线程池的设计围绕以下几个关键点：
- **任务队列**：使用线程安全的队列存储待执行任务（`std::function<void()>`），支持动态添加任务。
- **线程管理**：创建固定数量的线程（通常基于硬件并发性），每个线程持续从队列中获取任务执行。
- **线程同步**：通过互斥锁（`std::mutex`）和条件变量（`std::condition_variable`）实现任务分配和任务完成通知的线程安全。
- **任务完成等待**：提供机制让主线程等待所有任务完成，确保渲染流水线的同步。
- **异常安全**：确保线程池在异常情况下（如停止时添加任务）能正确处理。
- **性能优化**：最小化锁竞争和上下文切换，提升任务分配和执行效率。

### 2. **实现方法**
`ThreadPool` 类的实现基于 C++11 的线程库（`std::thread`、`std::mutex` 等），代码结构清晰且高效。以下是核心组件的详细实现说明。

#### a. **类定义与成员**
线程池的核心数据结构包括：
- **任务队列**：`std::queue<std::function<void()>> tasks` 存储待执行任务。
- **工作线程**：`std::vector<std::thread> workers` 管理线程池中的线程。
- **同步机制**：
  - `std::mutex queueMutex`：保护任务队列的访问。
  - `std::condition_variable condition`：通知线程有新任务或停止信号。
  - `std::condition_variable completionCondition`：通知主线程所有任务完成。
- **状态变量**：
  - `bool stop`：控制线程池的停止状态。
  - `std::atomic<uint32_t> activeTasks`：跟踪当前正在执行的任务数。

- **头文件定义**：
```cpp
class ThreadPool {
public:
    ThreadPool(uint32_t numThreads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);
    void waitForCompletion();
    uint32_t getNumThreads() const { return workers.size(); }
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable completionCondition;
    bool stop;
    std::atomic<uint32_t> activeTasks;
    void workerThread();
};
```

#### b. **构造函数**

构造函数初始化线程池，创建指定数量的工作线程（numThreads），并将每个线程绑定到 workerThread 方法。

实现：

```cpp
ThreadPool::ThreadPool(uint32_t numThreads) : stop(false), activeTasks(0) {
    for (uint32_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}
```

- **线程数选择**： `numThreads` 通常设置为 `std::thread::hardware_concurrency() - 1`，以保留一个核心给主线程和其他系统任务。测试中最大线程数为 32。
- **线程创建**：使用 `emplace_back` 直接构造 `std::thread` 对象，绑定到 `workerThread` 方法，减少拷贝开销。

#### c. **析构函数**

析构函数负责安全停止线程池，确保所有线程正确退出。

- **实现**：

```cpp
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
```

- **停止信号**：设置 `stop = true`，通知所有线程退出。
- **通知线程**：调用 `condition.notify_all()` 唤醒所有等待任务的线程。
- **线程回收**：通过 `worker.join()` 等待每个线程退出，确保资源正确释放。

#### d. **任务入队**

enqueue 方法将任务添加到任务队列，并通知一个空闲线程执行。

实现：

```cpp
void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) {
            throw std::runtime_error("Cannot enqueue task: ThreadPool is stopped");
        }
        tasks.emplace(task);
    }
    condition.notify_one();
}
```

- **线程安全**：使用 `std::unique_lock `保护任务队列，防止多线程同时修改。
- **异常检查**：如果线程池已停止`（stop == true）`，抛出异常以避免无效操作。
- **高效通知**：`condition.notify_one()` 只唤醒一个等待的线程，减少不必要的上下文切换。

#### e. **工作线程**

workerThread 是每个线程执行的循环，持续从任务队列中获取并执行任务。

实现：

```cpp
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
                completionCondition.notify_all();
            }
        }
    }
}
```

- **任务获取**：
  - 使用 `condition.wait` 等待任务或停止信号，条件为 stop || !tasks.empty()。
  - 如果 `stop == true` 且队列为空，线程退出。
  - 使用 `std::move` 高效转移任务，减少拷贝开销。
- **任务执行**：
  - 在临界区外执行任务（`task()`），避免持有锁时间过长。
  - 通过 `activeTasks` 跟踪正在执行的任务数。
- **完成通知**：
  - 任务完成后，减少 `activeTasks` 计数。
  - 如果 `activeTasks == 0` 且队列为空，通知主线程所有任务完成。

#### f. **任务完成等待**

`waitForCompletion` 方法让主线程等待所有任务完成。

实现：

```cpp
void ThreadPool::waitForCompletion() {
    std::unique_lock<std::mutex> lock(queueMutex);
    completionCondition.wait(lock, [this] { return activeTasks == 0 && tasks.empty(); });
}
```

- 等待条件：等待 `activeTasks == 0`（无任务在执行）且 `tasks.empty()`（队列为空）。
- 高效同步：使用 `completionCondition` 避免主线程忙等待，提升性能。

### 3. **线程池在软渲染器中的应用**

线程池被集成到渲染流水线的多个模块，以并行化计算密集型任务。以下是主要应用场景的详细说明：

#### a. **Framebuffer 的多线程优化**

Framebuffer 负责像素填充、深度测试和帧缓冲区翻转。我们引入了以下优化：

- **像素锁机制**：为避免多线程写入同一像素的竞争，设计了一个固定大小的锁池（`std::vector<std::mutex>`，大小为 `LOCK_POOL_SIZE = 2047`）。通过哈希函数 `getLockIndex(x, y)` 将像素坐标映射到锁池中的互斥锁，实现细粒度同步。
- **垂直翻转并行化**： `flipVertical` 方法将帧缓冲区的行分成若干组（`rowsPerThread`），每个线程处理一部分行，通过线程池并行执行。
- **任务分配**：线程池将翻转任务分解为小块（每线程处理 `rowsPerThread` 行），确保负载均衡。

代码示例：

```cpp
void Framebuffer::flipVertical() {
#ifdef MultiThreading
    uint32_t numThreads = threadPool.getNumThreads();
    numThreads = std::max(1u, numThreads);
    int rowsPerThread = (height / 2 + numThreads - 1) / numThreads;
    for (int startY = 0; startY < height / 2; startY += rowsPerThread) {
        int endY = std::min(startY + rowsPerThread, height / 2);
        threadPool.enqueue([this, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                for (int x = 0; x < width; ++x) {
                    std::swap(pixels[y * width + x], pixels[(height - 1 - y) * width + x]);
                }
            }
        });
    }
    threadPool.waitForCompletion();
#else
    // 单线程实现
#endif
}
```

b. **Renderer 的多线程优化**

Renderer 负责顶点处理、三角形光栅化和绘制。我们将三角形处理并行化：

- 三角形分配：将模型的三角形（`numFaces`）分成若干组（`facesPerThread`），通过线程池分配给多个线程。
- 任务划分：每个任务处理一定范围的三角形（`startFace 到 endFace`），包括顶点处理、透视除法和光栅化。
- 同步：通过 `waitForCompletion` 确保所有三角形处理完成后再进入下一阶段。

代码示例：


```cpp
void Renderer::drawModel(const Model& model, const Transform& transform, const Material& material) {
    int numFaces = static_cast<int>(model.numFaces());
#ifdef MultiThreading
    int maxThreads = threadPool.getNumThreads();
    int numThreads = std::max(1, std::min(maxThreads, numFaces));
    int facesPerThread = (numFaces + numThreads - 1) / numThreads;
    for (int startFace = 0; startFace < numFaces; startFace += facesPerThread) {
        int endFace = std::min(startFace + facesPerThread, numFaces);
        threadPool.enqueue([this, &model, &material, startFace, endFace]() {
            for (int i = startFace; i < endFace; ++i) {
                // 三角形处理逻辑
                drawTriangle(screenVertices[0], screenVertices[1], screenVertices[2], material);
            }
        });
    }
    threadPool.waitForCompletion();
#else
    // 单线程实现
#endif
}
```

#### c. **SDLApp 的纹理更新并行化**

SDLApp 负责将帧缓冲区数据传输到 SDL 纹理。我们将 `updateTextureFromFramebuffer` 并行化：

- **行分配**：将帧缓冲区的行分成若干组（`rowsPerThread`），每个线程处理一部分行。
- **颜色转换**：线程池并行执行颜色值从浮点（`vec3f`）到 `Uint8` 的转换。

代码示例：

```cpp
void SDLApp::updateTextureFromFramebuffer(const Framebuffer& framebuffer) {
    // ... SDL 纹理锁定 ...
#ifdef MultiThreading
    uint32_t numThreads = threadPool.getNumThreads();
    numThreads = std::max(1u, numThreads);
    int rowsPerThread = (height + numThreads - 1) / numThreads;
    for (int startY = 0; startY < height; startY += rowsPerThread) {
        int endY = std::min(startY + rowsPerThread, height);
        threadPool.enqueue([this, &framebuffer, dstPixels, pitch, &pixels, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                for (int x = 0; x < width; ++x) {
                    const vec3f& color = pixels[y * width + x];
                    Uint8* dstPixel = dstPixels + y * pitch + x * 3;
                    dstPixel[0] = static_cast<Uint8>(std::round(color.x * 255.0f));
                    dstPixel[1] = static_cast<Uint8>(std::round(color.y * 255.0f));
                    dstPixel[2] = static_cast<Uint8>(std::round(color.z * 255.0f));
                }
            }
        });
    }
    threadPool.waitForCompletion();
#else
    // 单线程实现
#endif
    // ... SDL 纹理解锁 ...
}
```

### 4. **实现细节与优化**

- **任务粒度**：任务被划分为较小单元（例如每线程处理若干行或三角形），通过向上取整（`ceiling division`）确保负载均衡。
- **锁优化**：任务队列操作使用 `std::unique_lock` 最小化锁持有时间，任务执行在临界区外进行，降低锁竞争。
- **条件变量**：`condition` 和 `completionCondition` 分别用于任务分配和完成通知，避免忙等待。
- **异常处理**：在 `enqueue` 中检查 `stop` 状态，防止向已停止的线程池添加任务。
- **条件编译**：通过 `#ifdef MultiThreading` 支持单线程和多线程模式，便于调试和兼容性测试。

## 性能提升分析

通过线程池和多线程优化，软渲染器的平均 FPS 从 30 提升至约 145（分辨率 800x800，最大线程数 32）。以下是性能提升的关键因素：

1. 并行化计算密集型任务：
   - 顶点处理和光栅化通过线程池并行执行，显著减少了 `Renderer::drawModel` 的耗时。
   - 帧缓冲区翻转和纹理更新并行化，降低了 `Framebuffer::flipVertical` 和 `SDLApp::updateTextureFromFramebuffer` 的延迟。
2. 细粒度锁机制：
   - `Framebuffer` 的锁池（`pixelLocks`）通过哈希映射减少锁竞争，确保像素写入的线程安全。
3. 负载均衡：
   - 任务按行或三角形均匀分配，最大化利用多核 CPU。
4. 线程复用：
   - 线程池避免频繁创建和销毁线程，减少上下文切换开销。

## 性能数据

- 测试环境：分辨率 `800x800`，`32` 线程，复杂场景（多个模型、灯光和纹理）。
- 单线程 FPS：约 `30`。
- 多线程 FPS：约 `145`（提升约 `4.83` 倍）。
- 瓶颈分析：多线程模式下，`SDL` 的 `SDL_RenderPresent` 成为新瓶颈，受限于其单线程设计。


## 结论

通过设计高效的线程池并优化 Framebuffer、Renderer 和 SDLApp，我们将软渲染器的平均 FPS 从 30 提升至 145，性能提升约 4.83 倍。线程池通过任务队列、线程复用、细粒度同步和负载均衡，充分发挥了多核 CPU 的潜力。这些优化展示了多线程编程在软渲染器中的价值，同时为未来改进（如 GPU 加速）奠定了基础。

欢迎讨论线程池实现细节或软渲染器的进一步优化！