# Error Handling & Logging

Learn how to implement robust error handling and comprehensive logging systems for your C++ server.

## Why Error Handling Matters

Proper error handling is crucial for:

- **Debugging**: Quickly identify and fix issues
- **Monitoring**: Track server health and performance
- **User Experience**: Provide meaningful error messages
- **Security**: Prevent information leakage
- **Maintenance**: Reduce downtime and support costs

## Error Handling Strategies

### 1. Exception-Based Error Handling

```cpp
#include <stdexcept>
#include <string>
#include <memory>

// Custom exception classes
class ServerException : public std::runtime_error {
public:
    ServerException(const std::string& message) : std::runtime_error(message) {}
};

class DatabaseException : public ServerException {
private:
    std::string sqlError_;
    std::string query_;

public:
    DatabaseException(const std::string& message, const std::string& sqlError = "",
                     const std::string& query = "")
        : ServerException(message), sqlError_(sqlError), query_(query) {}

    const std::string& getSqlError() const { return sqlError_; }
    const std::string& getQuery() const { return query_; }
};

class NetworkException : public ServerException {
private:
    int errorCode_;

public:
    NetworkException(const std::string& message, int errorCode)
        : ServerException(message), errorCode_(errorCode) {}

    int getErrorCode() const { return errorCode_; }
};

class ValidationException : public ServerException {
private:
    std::string field_;
    std::string value_;

public:
    ValidationException(const std::string& message, const std::string& field = "",
                       const std::string& value = "")
        : ServerException(message), field_(field), value_(value) {}

    const std::string& getField() const { return field_; }
    const std::string& getValue() const { return value_; }
};

// Error handling wrapper
template<typename Func>
auto safeExecute(Func func, const std::string& operation) -> decltype(func()) {
    try {
        return func();
    } catch (const DatabaseException& e) {
        // Log database errors with context
        Logger::error("Database error in " + operation + ": " + e.what(), {
            {"sql_error", e.getSqlError()},
            {"query", e.getQuery()}
        });
        throw;
    } catch (const NetworkException& e) {
        // Log network errors with error codes
        Logger::error("Network error in " + operation + ": " + e.what(), {
            {"error_code", std::to_string(e.getErrorCode())}
        });
        throw;
    } catch (const ValidationException& e) {
        // Log validation errors with field information
        Logger::error("Validation error in " + operation + ": " + e.what(), {
            {"field", e.getField()},
            {"value", e.getValue()}
        });
        throw;
    } catch (const std::exception& e) {
        // Log general errors
        Logger::error("Unexpected error in " + operation + ": " + e.what());
        throw;
    }
}
```

### 2. Result Pattern (Alternative to Exceptions)

```cpp
#include <variant>
#include <string>
#include <optional>

template<typename T>
class Result {
private:
    std::variant<T, std::string> data_;

public:
    Result(const T& value) : data_(value) {}
    Result(const std::string& error) : data_(error) {}

    bool isSuccess() const { return std::holds_alternative<T>(data_); }
    bool isError() const { return std::holds_alternative<std::string>(data_); }

    T getValue() const {
        if (isSuccess()) {
            return std::get<T>(data_);
        }
        throw std::runtime_error("Cannot get value from error result");
    }

    std::string getError() const {
        if (isError()) {
            return std::get<std::string>(data_);
        }
        throw std::runtime_error("Cannot get error from success result");
    }

    T getValueOr(const T& defaultValue) const {
        return isSuccess() ? getValue() : defaultValue;
    }

    std::optional<T> getValueOpt() const {
        return isSuccess() ? std::optional<T>(getValue()) : std::nullopt;
    }
};

// Usage example
Result<std::string> validateUser(const std::string& username, const std::string& email) {
    if (username.empty()) {
        return Result<std::string>("Username cannot be empty");
    }

    if (email.empty() || email.find('@') == std::string::npos) {
        return Result<std::string>("Invalid email format");
    }

    return Result<std::string>("User validation successful");
}

// Chaining results
template<typename T, typename Func>
auto andThen(const Result<T>& result, Func func) -> decltype(func(result.getValue())) {
    if (result.isSuccess()) {
        return func(result.getValue());
    }
    return Result<decltype(func(result.getValue()))>(result.getError());
}
```

## Logging System

### 1. Logger Interface

```cpp
#include <string>
#include <map>
#include <chrono>
#include <mutex>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

class Logger {
private:
    static Logger* instance_;
    static std::mutex instanceMutex_;

    std::ofstream fileStream_;
    std::mutex logMutex_;
    LogLevel currentLevel_;
    std::string logFormat_;
    bool consoleOutput_;
    bool fileOutput_;

    Logger() : currentLevel_(LogLevel::INFO), consoleOutput_(true), fileOutput_(true) {
        initializeLogFile();
    }

public:
    static Logger& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        if (!instance_) {
            instance_ = new Logger();
        }
        return *instance_;
    }

    void setLogLevel(LogLevel level) { currentLevel_ = level; }
    void setConsoleOutput(bool enabled) { consoleOutput_ = enabled; }
    void setFileOutput(bool enabled) { fileOutput_ = enabled; }

    template<typename... Args>
    void trace(const std::string& message, Args... args) {
        log(LogLevel::TRACE, message, args...);
    }

    template<typename... Args>
    void debug(const std::string& message, Args... args) {
        log(LogLevel::DEBUG, message, args...);
    }

    template<typename... Args>
    void info(const std::string& message, Args... args) {
        log(LogLevel::INFO, message, args...);
    }

    template<typename... Args>
    void warn(const std::string& message, Args... args) {
        log(LogLevel::WARN, message, args...);
    }

    template<typename... Args>
    void error(const std::string& message, Args... args) {
        log(LogLevel::ERROR, message, args...);
    }

    template<typename... Args>
    void fatal(const std::string& message, Args... args) {
        log(LogLevel::FATAL, message, args...);
    }

    // Structured logging with key-value pairs
    template<typename... Args>
    void log(LogLevel level, const std::string& message, Args... args) {
        if (level < currentLevel_) return;

        std::lock_guard<std::mutex> lock(logMutex_);

        std::string formattedMessage = formatMessage(level, message, args...);

        if (consoleOutput_) {
            std::cout << formattedMessage << std::endl;
        }

        if (fileOutput_ && fileStream_.is_open()) {
            fileStream_ << formattedMessage << std::endl;
            fileStream_.flush();
        }
    }

private:
    void initializeLogFile() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream filename;
        filename << "server_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".log";

        fileStream_.open(filename.str(), std::ios::app);
    }

    std::string formatMessage(LogLevel level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        oss << " [" << getLevelString(level) << "] ";
        oss << message;

        return oss.str();
    }

    template<typename T, typename... Args>
    std::string formatMessage(LogLevel level, const std::string& message,
                             const std::map<std::string, T>& context, Args... args) {
        std::string baseMessage = formatMessage(level, message);

        if (!context.empty()) {
            baseMessage += " | Context: ";
            bool first = true;
            for (const auto& [key, value] : context) {
                if (!first) baseMessage += ", ";
                baseMessage += key + "=" + std::to_string(value);
                first = false;
            }
        }

        return baseMessage;
    }

    std::string getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FAL:   return "FATAL";
            default: return "UNKNOWN";
        }
    }
};

// Static member initialization
Logger* Logger::instance_ = nullptr;
std::mutex Logger::instanceMutex_;
```

### 2. Structured Logging

```cpp
#include <map>
#include <string>
#include <variant>

class StructuredLogger {
private:
    struct LogEntry {
        std::string timestamp;
        LogLevel level;
        std::string message;
        std::map<std::string, std::variant<std::string, int, double, bool>> fields;
        std::string source;
        std::string function;
        int line;
    };

    std::vector<LogEntry> logBuffer_;
    std::mutex bufferMutex_;
    size_t maxBufferSize_;

public:
    StructuredLogger(size_t maxBuffer = 1000) : maxBufferSize_(maxBuffer) {}

    template<typename... Args>
    void log(LogLevel level, const std::string& message,
             const std::map<std::string, std::variant<std::string, int, double, bool>>& fields = {},
             const std::string& source = "", const std::string& function = "", int line = 0) {

        std::lock_guard<std::mutex> lock(bufferMutex_);

        LogEntry entry;
        entry.timestamp = getCurrentTimestamp();
        entry.level = level;
        entry.message = message;
        entry.fields = fields;
        entry.source = source;
        entry.function = function;
        entry.line = line;

        logBuffer_.push_back(entry);

        // Maintain buffer size
        if (logBuffer_.size() > maxBufferSize_) {
            logBuffer_.erase(logBuffer_.begin());
        }

        // Also log to standard logger
        Logger::getInstance().log(level, message);
    }

    // Convenience methods
    template<typename... Args>
    void info(const std::string& message, Args... args) {
        log(LogLevel::INFO, message, args...);
    }

    template<typename... Args>
    void error(const std::string& message, Args... args) {
        log(LogLevel::ERROR, message, args...);
    }

    // Export logs in different formats
    std::string exportAsJson() const;
    std::string exportAsCsv() const;

    // Search and filter logs
    std::vector<LogEntry> searchByLevel(LogLevel level) const;
    std::vector<LogEntry> searchBySource(const std::string& source) const;
    std::vector<LogEntry> searchByTimeRange(const std::string& start, const std::string& end) const;

private:
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};
```

## Error Recovery Strategies

### 1. Circuit Breaker Pattern

```cpp
#include <chrono>
#include <atomic>
#include <functional>

enum class CircuitState {
    CLOSED,     // Normal operation
    OPEN,       // Failing, reject requests
    HALF_OPEN   // Testing if service recovered
};

template<typename T>
class CircuitBreaker {
private:
    std::atomic<CircuitState> state_;
    std::atomic<int> failureCount_;
    std::atomic<int> successCount_;

    int failureThreshold_;
    int successThreshold_;
    std::chrono::milliseconds timeout_;
    std::chrono::system_clock::time_point lastFailureTime_;

public:
    CircuitBreaker(int failureThreshold = 5, int successThreshold = 3,
                   std::chrono::milliseconds timeout = std::chrono::milliseconds(60000))
        : state_(CircuitState::CLOSED), failureCount_(0), successCount_(0),
          failureThreshold_(failureThreshold), successThreshold_(successThreshold),
          timeout_(timeout) {}

    template<typename Func>
    T execute(Func func, const T& fallbackValue = T{}) {
        if (state_.load() == CircuitState::OPEN) {
            if (shouldAttemptReset()) {
                state_.store(CircuitState::HALF_OPEN);
            } else {
                return fallbackValue;
            }
        }

        try {
            T result = func();

            if (state_.load() == CircuitState::HALF_OPEN) {
                successCount_.fetch_add(1);
                if (successCount_.load() >= successThreshold_) {
                    state_.store(CircuitState::CLOSED);
                    failureCount_.store(0);
                    successCount_.store(0);
                }
            }

            return result;

        } catch (...) {
            failureCount_.fetch_add(1);
            lastFailureTime_ = std::chrono::system_clock::now();

            if (failureCount_.load() >= failureThreshold_) {
                state_.store(CircuitState::OPEN);
            }

            throw;
        }
    }

    CircuitState getState() const { return state_.load(); }
    int getFailureCount() const { return failureCount_.load(); }
    int getSuccessCount() const { return successCount_.load(); }

private:
    bool shouldAttemptReset() const {
        auto now = std::chrono::system_clock::now();
        return (now - lastFailureTime_) > timeout_;
    }
};
```

### 2. Retry Mechanism

```cpp
#include <chrono>
#include <functional>
#include <random>

template<typename T>
class RetryMechanism {
private:
    int maxAttempts_;
    std::chrono::milliseconds baseDelay_;
    double backoffMultiplier_;
    std::chrono::milliseconds maxDelay_;

public:
    RetryMechanism(int maxAttempts = 3, std::chrono::milliseconds baseDelay = std::chrono::milliseconds(100),
                   double backoffMultiplier = 2.0, std::chrono::milliseconds maxDelay = std::chrono::milliseconds(5000))
        : maxAttempts_(maxAttempts), baseDelay_(baseDelay), backoffMultiplier_(backoffMultiplier), maxDelay_(maxDelay) {}

    template<typename Func>
    T execute(Func func) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        std::exception_ptr lastException;

        for (int attempt = 1; attempt <= maxAttempts_; ++attempt) {
            try {
                return func();

            } catch (...) {
                lastException = std::current_exception();

                if (attempt == maxAttempts_) {
                    break;
                }

                // Calculate delay with exponential backoff and jitter
                auto delay = std::min(
                    baseDelay_ * std::pow(backoffMultiplier_, attempt - 1),
                    maxDelay_
                );

                // Add jitter to prevent thundering herd
                auto jitter = delay * 0.1 * dis(gen);
                delay += std::chrono::milliseconds(static_cast<long>(jitter.count()));

                std::this_thread::sleep_for(delay);
            }
        }

        std::rethrow_exception(lastException);
    }
};
```

## HTTP Error Handling

### 1. HTTP Error Responses

```cpp
#include <map>
#include <string>

class HTTPErrorHandler {
private:
    static const std::map<int, std::string> errorMessages;

public:
    static std::string generateErrorResponse(int statusCode, const std::string& customMessage = "") {
        std::ostringstream response;

        response << "HTTP/1.1 " << statusCode << " " << getStatusText(statusCode) << "\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Connection: close\r\n\r\n";

        std::string message = customMessage.empty() ? getStatusText(statusCode) : customMessage;

        response << "{";
        response << "\"error\": {";
        response << "\"status\": " << statusCode << ",";
        response << "\"message\": \"" << message << "\",";
        response << "\"timestamp\": \"" << getCurrentTimestamp() << "\"";
        response << "}";
        response << "}";

        return response.str();
    }

    static std::string generateDetailedErrorResponse(int statusCode, const std::string& message,
                                                   const std::map<std::string, std::string>& details = {}) {
        std::ostringstream response;

        response << "HTTP/1.1 " << statusCode << " " << getStatusText(statusCode) << "\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Connection: close\r\n\r\n";

        response << "{";
        response << "\"error\": {";
        response << "\"status\": " << statusCode << ",";
        response << "\"message\": \"" << message << "\",";
        response << "\"timestamp\": \"" << getCurrentTimestamp() << "\"";

        if (!details.empty()) {
            response << ",\"details\": {";
            bool first = true;
            for (const auto& [key, value] : details) {
                if (!first) response << ",";
                response << "\"" << key << "\": \"" << value << "\"";
                first = false;
            }
            response << "}";
        }

        response << "}";
        response << "}";

        return response.str();
    }

private:
    static std::string getStatusText(int statusCode) {
        static const std::map<int, std::string> statusTexts = {
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {405, "Method Not Allowed"},
            {500, "Internal Server Error"},
            {502, "Bad Gateway"},
            {503, "Service Unavailable"}
        };

        auto it = statusTexts.find(statusCode);
        return it != statusTexts.end() ? it->second : "Unknown Error";
    }

    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};
```

### 2. Middleware Error Handling

```cpp
#include <functional>
#include <string>

class ErrorHandlingMiddleware {
public:
    using RequestHandler = std::function<std::string(const std::map<std::string, std::string>&,
                                                   const std::map<std::string, std::string>&)>;

    static RequestHandler wrapWithErrorHandling(RequestHandler handler,
                                              const std::string& operationName) {
        return [handler, operationName](const std::map<std::string, std::string>& headers,
                                       const std::map<std::string, std::string>& params) -> std::string {
            try {
                return handler(headers, params);

            } catch (const ValidationException& e) {
                Logger::getInstance().warn("Validation error in " + operationName + ": " + e.what());
                return HTTPErrorHandler::generateDetailedErrorResponse(400, "Validation Error", {
                    {"field", e.getField()},
                    {"value", e.getValue()}
                });

            } catch (const DatabaseException& e) {
                Logger::getInstance().error("Database error in " + operationName + ": " + e.what());
                return HTTPErrorHandler::generateErrorResponse(500, "Database Error");

            } catch (const NetworkException& e) {
                Logger::getInstance().error("Network error in " + operationName + ": " + e.what());
                return HTTPErrorHandler::generateErrorResponse(502, "Service Unavailable");

            } catch (const std::exception& e) {
                Logger::getInstance().error("Unexpected error in " + operationName + ": " + e.what());
                return HTTPErrorHandler::generateErrorResponse(500, "Internal Server Error");

            } catch (...) {
                Logger::getInstance().fatal("Unknown error in " + operationName);
                return HTTPErrorHandler::generateErrorResponse(500, "Internal Server Error");
            }
        };
    }
};
```

## Performance Monitoring

### 1. Request Timing

```cpp
#include <chrono>
#include <map>

class RequestTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::string operationName_;

public:
    RequestTimer(const std::string& operation)
        : startTime_(std::chrono::high_resolution_clock::now()), operationName_(operation) {}

    ~RequestTimer() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);

        Logger::getInstance().info("Operation completed", {
            {"operation", operationName_},
            {"duration_us", std::to_string(duration.count())}
        });
    }

    template<typename T>
    T measure(T result) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);

        Logger::getInstance().info("Operation completed", {
            {"operation", operationName_},
            {"duration_us", std::to_string(duration.count())}
        });

        return result;
    }
};

// Usage with RAII
#define TIME_OPERATION(name) RequestTimer timer(name)
```

### 2. Metrics Collection

```cpp
#include <atomic>
#include <chrono>
#include <map>

class MetricsCollector {
private:
    std::atomic<long> totalRequests_;
    std::atomic<long> successfulRequests_;
    std::atomic<long> failedRequests_;
    std::atomic<long> totalResponseTime_;

    std::map<std::string, std::atomic<long>> endpointRequests_;
    std::map<std::string, std::atomic<long>> endpointErrors_;

public:
    void recordRequest(const std::string& endpoint, bool success, long responseTime) {
        totalRequests_.fetch_add(1);
        totalResponseTime_.fetch_add(responseTime);

        if (success) {
            successfulRequests_.fetch_add(1);
        } else {
            failedRequests_.fetch_add(1);
        }

        endpointRequests_[endpoint].fetch_add(1);
        if (!success) {
            endpointErrors_[endpoint].fetch_add(1);
        }
    }

    std::map<std::string, double> getMetrics() const {
        std::map<std::string, double> metrics;

        long total = totalRequests_.load();
        if (total > 0) {
            metrics["total_requests"] = static_cast<double>(total);
            metrics["success_rate"] = static_cast<double>(successfulRequests_.load()) / total;
            metrics["error_rate"] = static_cast<double>(failedRequests_.load()) / total;
            metrics["avg_response_time"] = static_cast<double>(totalResponseTime_.load()) / total;
        }

        return metrics;
    }

    void reset() {
        totalRequests_.store(0);
        successfulRequests_.store(0);
        failedRequests_.store(0);
        totalResponseTime_.store(0);

        for (auto& [endpoint, count] : endpointRequests_) {
            count.store(0);
        }

        for (auto& [endpoint, count] : endpointErrors_) {
            count.store(0);
        }
    }
};
```

## Testing Error Handling

```cpp
#include <cassert>
#include <iostream>

void testErrorHandling() {
    try {
        // Test circuit breaker
        CircuitBreaker<std::string> breaker(3, 2);

        int callCount = 0;
        auto failingFunction = [&callCount]() -> std::string {
            callCount++;
            throw std::runtime_error("Simulated failure");
        };

        // First few calls should fail
        for (int i = 0; i < 3; i++) {
            try {
                breaker.execute(failingFunction);
                assert(false); // Should not reach here
            } catch (...) {
                // Expected
            }
        }

        // Circuit should be open now
        assert(breaker.getState() == CircuitState::OPEN);

        // Calls should return fallback value
        std::string result = breaker.execute(failingFunction, "fallback");
        assert(result == "fallback");

        std::cout << "✓ Circuit breaker test passed" << std::endl;

        // Test retry mechanism
        RetryMechanism<std::string> retry(3);

        int retryCount = 0;
        auto eventuallySucceedingFunction = [&retryCount]() -> std::string {
            retryCount++;
            if (retryCount < 3) {
                throw std::runtime_error("Temporary failure");
            }
            return "success";
        };

        std::string retryResult = retry.execute(eventuallySucceedingFunction);
        assert(retryResult == "success");
        assert(retryCount == 3);

        std::cout << "✓ Retry mechanism test passed" << std::endl;

        // Test error handling middleware
        auto testHandler = [](const std::map<std::string, std::string>& headers,
                              const std::map<std::string, std::string>& params) -> std::string {
            throw ValidationException("Invalid input", "email", "not-an-email");
        };

        auto wrappedHandler = ErrorHandlingMiddleware::wrapWithErrorHandling(testHandler, "test");
        std::string errorResponse = wrappedHandler({}, {});

        assert(errorResponse.find("400") != std::string::npos);
        assert(errorResponse.find("Validation Error") != std::string::npos);

        std::cout << "✓ Error handling middleware test passed" << std::endl;

        std::cout << "All error handling tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error handling test failed: " << e.what() << std::endl;
    }
}
```

## Best Practices

### 1. Log Everything Important

- Request/response details
- Performance metrics
- Error conditions
- Security events
- Business operations

### 2. Use Appropriate Log Levels

- **TRACE**: Detailed debugging information
- **DEBUG**: General debugging information
- **INFO**: General information about application flow
- **WARN**: Warning messages for potentially harmful situations
- **ERROR**: Error events that might still allow the application to continue
- **FATAL**: Severe error events that will prevent the application from running

### 3. Include Context in Logs

- User ID or session information
- Request ID for tracing
- Timestamp and duration
- Relevant parameters and values
- Stack traces for errors

### 4. Implement Structured Logging

- Use consistent log formats
- Include machine-readable fields
- Enable easy searching and filtering
- Support multiple output formats

### 5. Monitor and Alert

- Set up log aggregation
- Implement automated monitoring
- Create alerts for critical errors
- Track error rates and trends

## Next Steps

Now that you understand error handling and logging, let's move on to performance optimization.

<div class="next-lesson">
    <a href="/course/advanced-features/performance" class="btn btn-primary">Next: Performance Optimization →</a>
</div>
