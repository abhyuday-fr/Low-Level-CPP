# Thread Pool in C++

This directory contains a simple yet robust implementation of a **Thread Pool** in C++.

## What is a Thread Pool?

A thread pool is a software design pattern for achieving concurrency. It maintains multiple threads waiting for tasks to be allocated for concurrent execution by the supervising program. 

By maintaining a pool of threads, the model increases performance and avoids latency in execution due to the frequent creation and destruction of threads for short-lived tasks.

## Why do we need Thread Pools?

1.  **Reduced Overhead**: Creating and destroying threads are expensive operations. Reusing existing threads significantly reduces this overhead.
2.  **Resource Management**: Thread pools help in limiting the number of concurrent threads, preventing the system from being overwhelmed by too many threads (which can lead to excessive context switching and memory exhaustion).
3.  **Improved Responsiveness**: Since threads are already running and waiting for tasks, the latency between submitting a task and its execution is minimized.
4.  **Graceful Degradation**: Under heavy load, tasks are simply queued until a thread becomes available, rather than failing to create new threads.

## Implementation Details

The implementation in `main.cpp` follows a standard producer-consumer pattern using modern C++ features:

### Core Components

*   **Worker Threads**: A `std::vector<std::thread>` that stores the worker threads. These threads run in an infinite loop, waiting for tasks to appear in the queue.
*   **Task Queue**: A `std::queue<std::function<void()>>` that holds the tasks to be executed. Each task is a callable object (typically a lambda).
*   **Synchronization**:
    *   `std::mutex`: Ensures thread-safe access to the task queue and the `stop_` flag.
    *   `std::condition_variable`: Used to signal worker threads when a new task is available or when the pool is shutting down.
*   **Graceful Shutdown**: The destructor sets a `stop_` flag and notifies all threads. The worker threads are designed to finish all remaining tasks in the queue before exiting and being joined.

### Key Features & Improvements

1.  **Safety**: Added a check in `enqueue()` to prevent adding tasks after the pool has started shutting down.
2.  **Efficiency**: Used `condition_.notify_one()` in the enqueue process to wake up only one thread per task, reducing unnecessary context switches.
3.  **Output Protection**: Implemented `coutMutex` in the `main` function example to prevent interleaved console output from multiple threads.
4.  **RAII**: Utilized `std::unique_lock` and `std::lock_guard` for automatic and safe mutex management.

## How to Run

To compile and run the example:

```bash
g++ -std=c++11 -pthread main.cpp -o thread_pool
./thread_pool
```

Note: C++11 or higher is required for `<thread>` and other concurrency features.
