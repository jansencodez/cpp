# Performance Optimization

Learn how to optimize your C++ server for maximum performance, scalability, and efficiency.

## Why Performance Matters

Performance optimization is crucial for:

- **User Experience**: Faster response times
- **Scalability**: Handle more concurrent users
- **Resource Efficiency**: Reduce server costs
- **Competitive Advantage**: Better than alternatives
- **Business Success**: Higher user satisfaction

## Performance Measurement

### 1. Benchmarking Tools

```cpp
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

class Benchmark {
private:
    std::string name_;
    std::vector<double> measurements_;

public:
    Benchmark(const std::string& name) : name_(name) {}

    template<typename Func>
    void measure(Func func, int iterations = 1000) {
        measurements_.clear();
        measurements_.reserve(iterations);

        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            measurements_.push_back(duration.count());
        }

        printResults();
    }

    void printResults() const {
        if (measurements_.empty()) return;

        std::sort(measurements_.begin(), measurements_.end());

        double min = measurements_.front();
        double max = measurements_.back();
        double median = measurements_[measurements_.size() / 2];
        double mean = std::accumulate(measurements_.begin(), measurements_.end(), 0.0) / measurements_.size();

        std::cout << "\n=== " << name_ << " Benchmark ===" << std::endl;
        std::cout << "Iterations: " << measurements_.size() << std::endl;
        std::cout << "Min: " << min << " ns" << std::endl;
        std::cout << "Max: " << max << " ns" << std::endl;
        std::cout << "Median: " << median << " ns" << std::endl;
        std::cout << "Mean: " << mean << " ns" << std::endl;
        std::cout << "Std Dev: " << calculateStdDev(mean) << " ns" << std::endl;
    }

private:
    double calculateStdDev(double mean) const {
        double variance = 0.0;
        for (double value : measurements_) {
            variance += std::pow(value - mean, 2);
        }
        variance /= measurements_.size();
        return std::sqrt(variance);
    }
};

// Usage example
void benchmarkExample() {
    Benchmark bench("String Concatenation");

    // Test different string concatenation methods
    bench.measure([]() {
        std::string result;
        for (int i = 0; i < 100; ++i) {
            result += "test" + std::to_string(i);
        }
    });

    Benchmark bench2("String Builder");
    bench2.measure([]() {
        std::ostringstream oss;
        for (int i = 0; i < 100; ++i) {
            oss << "test" << i;
        }
        std::string result = oss.str();
    });
}
```

### 2. Profiling Integration

```cpp
#include <gperftools/profiler.h>
#include <string>

class Profiler {
public:
    static void start(const std::string& profileName) {
        std::string filename = profileName + ".prof";
        ProfilerStart(filename.c_str());
    }

    static void stop() {
        ProfilerStop();
    }

    static void flush() {
        ProfilerFlush();
    }
};

// RAII wrapper for profiling
class ScopedProfiler {
private:
    std::string name_;

public:
    ScopedProfiler(const std::string& name) : name_(name) {
        Profiler::start(name_);
    }

    ~ScopedProfiler() {
        Profiler::stop();
    }
};

#define PROFILE_SCOPE(name) ScopedProfiler profiler(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
```

## Memory Optimization

### 1. Memory Pooling

```cpp
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

template<typename T>
class MemoryPool {
private:
    struct Block {
        std::vector<T> data;
        std::vector<bool> used;
        size_t nextFree;

        Block(size_t size) : data(size), used(size, false), nextFree(0) {}

        T* allocate() {
            if (nextFree >= data.size()) {
                // Find next free slot
                for (size_t i = 0; i < used.size(); ++i) {
                    if (!used[i]) {
                        nextFree = i;
                        break;
                    }
                }
                if (nextFree >= data.size()) return nullptr;
            }

            used[nextFree] = true;
            T* result = &data[nextFree];
            nextFree++;
            return result;
        }

        void deallocate(T* ptr) {
            size_t index = ptr - &data[0];
            if (index < data.size()) {
                used[index] = false;
                if (index < nextFree) {
                    nextFree = index;
                }
            }
        }
    };

    std::vector<std::unique_ptr<Block>> blocks_;
    std::mutex poolMutex_;
    size_t blockSize_;
    size_t maxBlocks_;

public:
    MemoryPool(size_t blockSize = 1024, size_t maxBlocks = 10)
        : blockSize_(blockSize), maxBlocks_(maxBlocks) {}

    T* allocate() {
        std::lock_guard<std::mutex> lock(poolMutex_);

        // Try existing blocks
        for (auto& block : blocks_) {
            T* result = block->allocate();
            if (result) return result;
        }

        // Create new block if possible
        if (blocks_.size() < maxBlocks_) {
            blocks_.push_back(std::make_unique<Block>(blockSize_));
            return blocks_.back()->allocate();
        }

        return nullptr; // Pool exhausted
    }

    void deallocate(T* ptr) {
        std::lock_guard<std::mutex> lock(poolMutex_);

        for (auto& block : blocks_) {
            if (ptr >= &block->data[0] && ptr < &block->data[block->data.size() - 1] + 1) {
                block->deallocate(ptr);
                return;
            }
        }
    }

    size_t getBlockCount() const { return blocks_.size(); }
    size_t getTotalCapacity() const { return blocks_.size() * blockSize_; }
};

// Custom allocator using memory pool
template<typename T>
class PoolAllocator {
private:
    static MemoryPool<T> pool_;

public:
    using value_type = T;

    T* allocate(size_t n) {
        if (n == 1) {
            return pool_.allocate();
        }
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n) {
        if (n == 1) {
            pool_.deallocate(p);
        } else {
            ::operator delete(p);
        }
    }
};

template<typename T>
MemoryPool<T> PoolAllocator<T>::pool_;
```

### 2. Object Pooling

```cpp
#include <queue>
#include <functional>
#include <memory>

template<typename T>
class ObjectPool {
private:
    std::queue<std::unique_ptr<T>> pool_;
    std::mutex poolMutex_;
    std::function<std::unique_ptr<T>()> factory_;
    size_t maxSize_;

public:
    ObjectPool(std::function<std::unique_ptr<T>()> factory, size_t maxSize = 100)
        : factory_(factory), maxSize_(maxSize) {}

    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(poolMutex_);

        if (!pool_.empty()) {
            std::unique_ptr<T> obj = std::move(pool_.front());
            pool_.pop();
            return obj;
        }

        return factory_();
    }

    void release(std::unique_ptr<T> obj) {
        if (!obj) return;

        std::lock_guard<std::mutex> lock(poolMutex_);

        if (pool_.size() < maxSize_) {
            // Reset object state if needed
            if constexpr (requires { obj->reset(); }) {
                obj->reset();
            }
            pool_.push(std::move(obj));
        }
    }

    size_t getPoolSize() const {
        std::lock_guard<std::mutex> lock(poolMutex_);
        return pool_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(poolMutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }
};

// Example usage with HTTP request objects
class HTTPRequest {
private:
    std::string method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::string body_;

public:
    void reset() {
        method_.clear();
        path_.clear();
        headers_.clear();
        body_.clear();
    }

    void setMethod(const std::string& method) { method_ = method; }
    void setPath(const std::string& path) { path_ = path; }
    void addHeader(const std::string& key, const std::string& value) { headers_[key] = value; }
    void setBody(const std::string& body) { body_ = body; }

    const std::string& getMethod() const { return method_; }
    const std::string& getPath() const { return path_; }
    const std::map<std::string, std::string>& getHeaders() const { return headers_; }
    const std::string& getBody() const { return body_; }
};

// Create object pool for HTTP requests
auto createRequestPool() {
    return std::make_unique<ObjectPool<HTTPRequest>>(
        []() { return std::make_unique<HTTPRequest>(); },
        1000
    );
}
```

## String Optimization

### 1. String View Usage

```cpp
#include <string_view>
#include <string>
#include <vector>

class StringProcessor {
public:
    // Use string_view for read-only operations
    static std::vector<std::string_view> split(std::string_view str, char delimiter) {
        std::vector<std::string_view> result;
        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string_view::npos) {
            result.emplace_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }

        result.emplace_back(str.substr(start));
        return result;
    }

    // Efficient string concatenation
    static std::string concatenate(const std::vector<std::string_view>& parts) {
        size_t totalLength = 0;
        for (const auto& part : parts) {
            totalLength += part.length();
        }

        std::string result;
        result.reserve(totalLength);

        for (const auto& part : parts) {
            result.append(part);
        }

        return result;
    }

    // Fast string search
    static bool contains(std::string_view haystack, std::string_view needle) {
        return haystack.find(needle) != std::string_view::npos;
    }

    // Efficient string replacement
    static std::string replace(std::string_view str, std::string_view from, std::string_view to) {
        std::string result;
        size_t start = 0;
        size_t pos = str.find(from);

        while (pos != std::string_view::npos) {
            result.append(str.substr(start, pos - start));
            result.append(to);
            start = pos + from.length();
            pos = str.find(from, start);
        }

        result.append(str.substr(start));
        return result;
    }
};
```

### 2. String Builder Pattern

```cpp
#include <sstream>
#include <string>

class StringBuilder {
private:
    std::ostringstream stream_;

public:
    StringBuilder() {
        stream_.str().reserve(1024); // Pre-allocate buffer
    }

    template<typename T>
    StringBuilder& append(const T& value) {
        stream_ << value;
        return *this;
    }

    StringBuilder& appendLine(const std::string& line) {
        stream_ << line << '\n';
        return *this;
    }

    StringBuilder& appendFormat(const std::string& format, ...) {
        // Implementation with variadic arguments
        return *this;
    }

    std::string toString() const {
        return stream_.str();
    }

    void clear() {
        stream_.str("");
        stream_.clear();
    }

    size_t length() const {
        return stream_.str().length();
    }

    bool isEmpty() const {
        return stream_.str().empty();
    }
};

// Usage example
std::string buildResponse() {
    StringBuilder builder;
    builder.append("HTTP/1.1 200 OK\r\n")
           .append("Content-Type: application/json\r\n")
           .append("Content-Length: ")
           .append(contentLength)
           .append("\r\n\r\n")
           .append(jsonContent);

    return builder.toString();
}
```

## Algorithm Optimization

### 1. Efficient Data Structures

```cpp
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

class OptimizedDataProcessor {
private:
    // Use unordered containers for O(1) lookups
    std::unordered_map<std::string, int> userCounts_;
    std::unordered_set<std::string> activeUsers_;

public:
    void addUser(const std::string& userId) {
        userCounts_[userId]++;
        activeUsers_.insert(userId);
    }

    void removeUser(const std::string& userId) {
        auto it = userCounts_.find(userId);
        if (it != userCounts_.end()) {
            if (--it->second == 0) {
                userCounts_.erase(it);
            }
        }
        activeUsers_.erase(userId);
    }

    bool isUserActive(const std::string& userId) const {
        return activeUsers_.find(userId) != activeUsers_.end();
    }

    int getUserCount(const std::string& userId) const {
        auto it = userCounts_.find(userId);
        return it != userCounts_.end() ? it->second : 0;
    }

    // Efficient bulk operations
    void addUsers(const std::vector<std::string>& userIds) {
        userCounts_.reserve(userCounts_.size() + userIds.size());
        activeUsers_.reserve(activeUsers_.size() + userIds.size());

        for (const auto& userId : userIds) {
            addUser(userId);
        }
    }

    // Fast intersection
    std::vector<std::string> getCommonUsers(const std::vector<std::string>& otherUsers) const {
        std::vector<std::string> result;
        result.reserve(std::min(activeUsers_.size(), otherUsers.size()));

        for (const auto& userId : otherUsers) {
            if (isUserActive(userId)) {
                result.push_back(userId);
            }
        }

        return result;
    }
};
```

### 2. Cache Optimization

```cpp
#include <unordered_map>
#include <list>
#include <mutex>

template<typename K, typename V>
class LRUCache {
private:
    struct CacheEntry {
        K key;
        V value;
        CacheEntry(const K& k, const V& v) : key(k), value(v) {}
    };

    std::list<CacheEntry> cache_;
    std::unordered_map<K, typename std::list<CacheEntry>::iterator> cacheMap_;
    size_t capacity_;
    mutable std::mutex mutex_;

public:
    LRUCache(size_t capacity) : capacity_(capacity) {}

    V* get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cacheMap_.find(key);
        if (it == cacheMap_.end()) {
            return nullptr;
        }

        // Move to front (most recently used)
        cache_.splice(cache_.begin(), cache_, it->second);
        return &it->second->value;
    }

    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end()) {
            // Update existing entry
            it->second->value = value;
            cache_.splice(cache_.begin(), cache_, it->second);
        } else {
            // Add new entry
            if (cache_.size() >= capacity_) {
                // Remove least recently used
                cacheMap_.erase(cache_.back().key);
                cache_.pop_back();
            }

            cache_.emplace_front(key, value);
            cacheMap_[key] = cache_.begin();
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        cacheMap_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    bool contains(const K& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cacheMap_.find(key) != cacheMap_.end();
    }
};
```

## Network Optimization

### 1. Connection Pooling

```cpp
#include <queue>
#include <mutex>
#include <chrono>
#include <memory>

class ConnectionPool {
private:
    struct Connection {
        int socket;
        std::chrono::system_clock::time_point lastUsed;
        bool inUse;

        Connection(int sock) : socket(sock), inUse(false) {
            lastUsed = std::chrono::system_clock::now();
        }
    };

    std::queue<std::unique_ptr<Connection>> availableConnections_;
    std::vector<std::unique_ptr<Connection>> allConnections_;
    std::mutex poolMutex_;

    int serverPort_;
    std::string serverHost_;
    size_t maxConnections_;
    size_t minConnections_;

public:
    ConnectionPool(const std::string& host, int port, size_t minConn = 5, size_t maxConn = 20)
        : serverHost_(host), serverPort_(port), minConnections_(minConn), maxConnections_(maxConn) {
        initializePool();
    }

    std::unique_ptr<Connection> getConnection() {
        std::lock_guard<std::mutex> lock(poolMutex_);

        if (!availableConnections_.empty()) {
            auto conn = std::move(availableConnections_.front());
            availableConnections_.pop();
            conn->inUse = true;
            conn->lastUsed = std::chrono::system_clock::now();
            return conn;
        }

        if (allConnections_.size() < maxConnections_) {
            auto conn = createNewConnection();
            if (conn) {
                conn->inUse = true;
                return conn;
            }
        }

        return nullptr; // Pool exhausted
    }

    void returnConnection(std::unique_ptr<Connection> conn) {
        if (!conn) return;

        std::lock_guard<std::mutex> lock(poolMutex_);

        conn->inUse = false;
        conn->lastUsed = std::chrono::system_clock::now();

        // Check if connection is still valid
        if (isConnectionValid(conn.get())) {
            availableConnections_.push(std::move(conn));
        } else {
            // Remove invalid connection
            auto it = std::find_if(allConnections_.begin(), allConnections_.end(),
                                 [conn = conn.get()](const std::unique_ptr<Connection>& c) {
                                     return c.get() == conn;
                                 });
            if (it != allConnections_.end()) {
                allConnections_.erase(it);
            }
        }
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(poolMutex_);

        auto now = std::chrono::system_clock::now();
        auto timeout = std::chrono::minutes(5);

        // Remove stale connections
        allConnections_.erase(
            std::remove_if(allConnections_.begin(), allConnections_.end(),
                          [&](const std::unique_ptr<Connection>& conn) {
                              return !conn->inUse && (now - conn->lastUsed) > timeout;
                          }),
            allConnections_.end()
        );

        // Ensure minimum connections
        while (allConnections_.size() < minConnections_) {
            auto conn = createNewConnection();
            if (conn) {
                availableConnections_.push(std::move(conn));
            } else {
                break;
            }
        }
    }

private:
    void initializePool() {
        for (size_t i = 0; i < minConnections_; ++i) {
            auto conn = createNewConnection();
            if (conn) {
                availableConnections_.push(std::move(conn));
            }
        }
    }

    std::unique_ptr<Connection> createNewConnection() {
        // Implementation to create new socket connection
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return nullptr;

        // Connect to server...

        return std::make_unique<Connection>(sock);
    }

    bool isConnectionValid(Connection* conn) {
        // Check if socket is still valid
        int error = 0;
        socklen_t len = sizeof(error);
        return getsockopt(conn->socket, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0;
    }
};
```

### 2. HTTP Response Caching

```cpp
#include <string>
#include <chrono>
#include <unordered_map>

class HTTPCache {
private:
    struct CacheEntry {
        std::string response;
        std::string etag;
        std::chrono::system_clock::time_point expiresAt;
        std::chrono::system_clock::time_point lastModified;
        bool mustRevalidate;

        CacheEntry(const std::string& resp, const std::string& tag,
                   std::chrono::seconds maxAge, bool revalidate = false)
            : response(resp), etag(tag), mustRevalidate(revalidate) {
            lastModified = std::chrono::system_clock::now();
            expiresAt = lastModified + maxAge;
        }

        bool isExpired() const {
            return std::chrono::system_clock::now() > expiresAt;
        }

        bool needsRevalidation() const {
            return mustRevalidate || isExpired();
        }
    };

    std::unordered_map<std::string, CacheEntry> cache_;
    std::mutex cacheMutex_;
    size_t maxSize_;

public:
    HTTPCache(size_t maxSize = 1000) : maxSize_(maxSize) {}

    std::string get(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return "";
        }

        if (it->second.needsRevalidation()) {
            cache_.erase(it);
            return "";
        }

        return it->second.response;
    }

    void put(const std::string& key, const std::string& response,
             const std::string& etag, std::chrono::seconds maxAge, bool revalidate = false) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        // Remove old entry if exists
        cache_.erase(key);

        // Ensure cache size limit
        if (cache_.size() >= maxSize_) {
            // Remove oldest entries
            auto oldest = std::min_element(cache_.begin(), cache_.end(),
                                         [](const auto& a, const auto& b) {
                                             return a.second.lastModified < b.second.lastModified;
                                         });
            if (oldest != cache_.end()) {
                cache_.erase(oldest);
            }
        }

        cache_[key] = CacheEntry(response, etag, maxAge, revalidate);
    }

    bool contains(const std::string& key) const {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = cache_.find(key);
        return it != cache_.end() && !it->second.needsRevalidation();
    }

    void invalidate(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cache_.erase(key);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cache_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        return cache_.size();
    }
};
```

## Threading Optimization

### 1. Work Stealing Thread Pool

```cpp
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <deque>

class WorkStealingThreadPool {
private:
    struct ThreadLocalQueue {
        std::deque<std::function<void()>> tasks;
        mutable std::mutex mutex;

        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex);
            return tasks.empty();
        }

        void push(std::function<void()> task) {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push_front(std::move(task));
        }

        bool pop(std::function<void()>& task) {
            std::lock_guard<std::mutex> lock(mutex);
            if (tasks.empty()) return false;

            task = std::move(tasks.front());
            tasks.pop_front();
            return true;
        }

        bool steal(std::function<void()>& task) {
            std::lock_guard<std::mutex> lock(mutex);
            if (tasks.empty()) return false;

            task = std::move(tasks.back());
            tasks.pop_back();
            return true;
        }
    };

    std::vector<std::unique_ptr<ThreadLocalQueue>> localQueues_;
    std::queue<std::function<void()>> globalQueue_;
    std::mutex globalMutex_;
    std::condition_variable globalCV_;

    std::vector<std::thread> workers_;
    std::atomic<bool> stop_;

public:
    WorkStealingThreadPool(size_t threadCount = std::thread::hardware_concurrency())
        : stop_(false) {

        localQueues_.resize(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            localQueues_[i] = std::make_unique<ThreadLocalQueue>();
        }

        for (size_t i = 0; i < threadCount; ++i) {
            workers_.emplace_back(&WorkStealingThreadPool::workerThread, this, i);
        }
    }

    ~WorkStealingThreadPool() {
        stop_ = true;
        globalCV_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        std::function<void()> wrapper = [task]() { (*task)(); };

        // Try to add to global queue first
        {
            std::lock_guard<std::mutex> lock(globalMutex_);
            globalQueue_.push(std::move(wrapper));
        }
        globalCV_.notify_one();

        return result;
    }

private:
    void workerThread(size_t threadId) {
        ThreadLocalQueue* localQueue = localQueues_[threadId].get();

        while (!stop_) {
            std::function<void()> task;

            // Try local queue first
            if (localQueue->pop(task)) {
                task();
                continue;
            }

            // Try global queue
            {
                std::unique_lock<std::mutex> lock(globalMutex_);
                globalCV_.wait(lock, [this] { return !globalQueue_.empty() || stop_; });

                if (stop_) break;

                if (!globalQueue_.empty()) {
                    task = std::move(globalQueue_.front());
                    globalQueue_.pop();
                    lock.unlock();
                    task();
                    continue;
                }
            }

            // Try stealing from other queues
            bool found = false;
            for (size_t i = 0; i < localQueues_.size(); ++i) {
                if (i != threadId && localQueues_[i]->steal(task)) {
                    found = true;
                    break;
                }
            }

            if (found) {
                task();
            } else {
                // No work available, yield
                std::this_thread::yield();
            }
        }
    }
};
```

## Performance Testing

```cpp
#include <iostream>
#include <chrono>
#include <vector>

void performanceTest() {
    std::cout << "=== Performance Test Suite ===" << std::endl;

    // Test memory pool vs standard allocation
    Benchmark bench("Memory Pool vs Standard Allocation");

    bench.measure([]() {
        std::vector<int*> pointers;
        pointers.reserve(1000);

        for (int i = 0; i < 1000; ++i) {
            pointers.push_back(new int(i));
        }

        for (int* ptr : pointers) {
            delete ptr;
        }
    });

    // Test string operations
    Benchmark stringBench("String Operations");

    stringBench.measure([]() {
        std::string result;
        result.reserve(10000);

        for (int i = 0; i < 1000; ++i) {
            result += "test" + std::to_string(i) + " ";
        }
    });

    // Test cache performance
    Benchmark cacheBench("Cache Performance");

    LRUCache<std::string, int> cache(1000);

    cacheBench.measure([&cache]() {
        for (int i = 0; i < 1000; ++i) {
            std::string key = "key" + std::to_string(i);
            cache.put(key, i);
            cache.get(key);
        }
    });

    // Test thread pool
    Benchmark poolBench("Thread Pool Performance");

    WorkStealingThreadPool pool(4);

    poolBench.measure([&pool]() {
        std::vector<std::future<int>> futures;

        for (int i = 0; i < 1000; ++i) {
            futures.push_back(pool.submit([i]() {
                // Simulate some work
                int result = 0;
                for (int j = 0; j < 1000; ++j) {
                    result += i * j;
                }
                return result;
            }));
        }

        for (auto& future : futures) {
            future.get();
        }
    });

    std::cout << "\nAll performance tests completed!" << std::endl;
}
```

## Best Practices

### 1. Measure First

- Profile before optimizing
- Identify bottlenecks
- Set performance goals
- Measure after changes

### 2. Algorithm Selection

- Choose appropriate data structures
- Use efficient algorithms
- Consider time vs space complexity
- Profile different approaches

### 3. Memory Management

- Minimize allocations
- Use object pools
- Pre-allocate buffers
- Avoid unnecessary copies

### 4. Caching Strategy

- Cache frequently accessed data
- Use appropriate cache eviction
- Consider cache locality
- Monitor cache hit rates

### 5. Threading

- Use thread pools
- Minimize lock contention
- Use lock-free data structures
- Balance thread count

### 6. Network Optimization

- Connection pooling
- Response caching
- Compression
- Batch operations

## Next Steps

Now that you understand performance optimization, let's move on to deployment and production setup.

<div class="next-lesson">
    <a href="/course/deployment/production-setup" class="btn btn-primary">Next: Production Setup →</a>
</div>
