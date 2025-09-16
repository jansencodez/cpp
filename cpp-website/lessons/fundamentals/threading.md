# Multi-threading and Concurrency

Multi-threading is essential for building high-performance servers that can handle multiple clients simultaneously. In this lesson, you'll learn how to implement thread-safe server architecture.

## Why Multi-threading?

A single-threaded server can only handle one client at a time. When a client connects, all other clients must wait. Multi-threading allows your server to:

- Handle multiple clients concurrently
- Improve responsiveness and throughput
- Better utilize multi-core processors
- Provide non-blocking I/O operations

## Thread Safety Concepts

### Race Conditions

When multiple threads access shared data simultaneously, unpredictable results can occur:

```cpp
#include <thread>
#include <iostream>

int counter = 0;  // Shared variable - DANGEROUS!

void increment() {
    for (int i = 0; i < 1000000; ++i) {
        counter++;  // Race condition!
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    std::cout << "Counter: " << counter << std::endl;
    // Expected: 2000000, Actual: unpredictable!
    return 0;
}
```

### Atomic Operations

Use `std::atomic` for simple operations:

```cpp
#include <atomic>
#include <thread>
#include <iostream>

std::atomic<int> counter{0};  // Thread-safe!

void increment() {
    for (int i = 0; i < 1000000; ++i) {
        counter++;  // Atomic operation
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    std::cout << "Counter: " << counter << std::endl;
    // Expected: 2000000, Actual: 2000000 ✓
    return 0;
}
```

## Thread Pool Implementation

Here's a complete thread pool implementation for your server:

```cpp
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {

        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks_.emplace([task]() { (*task)(); });
        }

        condition_.notify_one();
        return result;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }

        condition_.notify_all();

        for (std::thread& worker : workers_) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};
```

## Server Threading Architecture

Here's how to integrate threading into your HTTP server:

```cpp
class HttpServer {
private:
    ThreadPool threadPool_;
    std::atomic<bool> running_{false};

public:
    HttpServer(int port, size_t numThreads = 4)
        : port_(port), threadPool_(numThreads) {
    }

    void start() {
        running_ = true;

        // Start accepting connections in main thread
        while (running_) {
            int clientSocket = accept(serverSocket_, nullptr, nullptr);
            if (clientSocket >= 0) {
                // Handle client in thread pool
                threadPool_.enqueue([this, clientSocket]() {
                    handleClient(clientSocket);
                });
            }
        }
    }

    void stop() {
        running_ = false;
    }

private:
    void handleClient(int clientSocket) {
        // Handle HTTP request/response
        std::string request = receiveRequest(clientSocket);
        std::string response = processRequest(request);
        sendResponse(clientSocket, response);

        close(clientSocket);
    }
};
```

## Synchronization Primitives

### Mutex

Protect shared resources:

```cpp
#include <mutex>
#include <map>

class ThreadSafeCache {
private:
    std::map<std::string, std::string> cache_;
    mutable std::mutex mutex_;

public:
    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[key] = value;
    }

    std::string get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        return (it != cache_.end()) ? it->second : "";
    }

    bool exists(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.find(key) != cache_.end();
    }
};
```

### Condition Variables

Coordinate between threads:

```cpp
#include <condition_variable>
#include <queue>

template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;

public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        condition_.notify_one();
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });
        value = std::move(queue_.front());
        queue_.pop();
    }
};
```

## Best Practices

### 1. Avoid Global Variables

```cpp
// BAD: Global state
static int globalCounter = 0;

// GOOD: Encapsulated state
class Server {
private:
    std::atomic<int> connectionCount_{0};
};
```

### 2. Use RAII for Resources

```cpp
class ScopedLock {
public:
    ScopedLock(std::mutex& mutex) : mutex_(mutex) {
        mutex_.lock();
    }

    ~ScopedLock() {
        mutex_.unlock();
    }

private:
    std::mutex& mutex_;
};
```

### 3. Minimize Critical Sections

```cpp
// BAD: Long critical section
void processData() {
    std::lock_guard<std::mutex> lock(mutex_);
    // ... lots of work ...
    // ... more work ...
    // ... even more work ...
}

// GOOD: Minimize critical section
void processData() {
    std::string data;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data = sharedData_;
    }
    // ... work with local copy ...
}
```

## Interactive Exercise

Implement a thread-safe counter with multiple operations:

```cpp
class ThreadSafeCounter {
private:
    std::atomic<int> value_{0};
    std::mutex mutex_;

public:
    void increment() {
        // TODO: Implement atomic increment
    }

    void decrement() {
        // TODO: Implement atomic decrement
    }

    void add(int amount) {
        // TODO: Implement atomic addition
    }

    int get() const {
        // TODO: Return current value
        return 0;
    }

    void reset() {
        // TODO: Reset to zero
    }
};
```

## Performance Considerations

- **Thread Count**: Usually 2-4x number of CPU cores
- **Memory Overhead**: Each thread uses ~1MB stack space
- **Context Switching**: Minimize with proper synchronization
- **Lock Contention**: Use lock-free data structures when possible

## Next Steps

Now that you understand threading, let's move on to building the server architecture.

<div class="next-lesson">
    <a href="/course/building-blocks/server-class" class="btn btn-primary">Next: Server Class Architecture →</a>
</div>
