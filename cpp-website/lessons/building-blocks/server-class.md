# Server Class Architecture

Now that you understand the fundamentals, let's design a robust, maintainable server architecture using modern C++ design patterns and best practices.

## Architecture Principles

### 1. Single Responsibility Principle

Each class should have one reason to change:

```cpp
// BAD: Server does everything
class HttpServer {
    void handleRequest();
    void parseRequest();
    void generateResponse();
    void manageDatabase();
    void handleAuthentication();
    void logEvents();
    // ... 20 more methods
};

// GOOD: Separate concerns
class HttpServer {
    void start();
    void stop();
    void addRoute();
};

class RequestParser {
    HttpRequest parse(const std::string& rawRequest);
};

class ResponseGenerator {
    std::string generate(const HttpResponse& response);
};

class DatabaseManager {
    void connect();
    void query();
    void disconnect();
};
```

### 2. Dependency Injection

Make dependencies explicit and testable:

```cpp
class HttpServer {
private:
    std::unique_ptr<RequestParser> parser_;
    std::unique_ptr<ResponseGenerator> generator_;
    std::unique_ptr<RouteManager> routes_;
    std::unique_ptr<ThreadPool> threadPool_;

public:
    HttpServer(
        std::unique_ptr<RequestParser> parser,
        std::unique_ptr<ResponseGenerator> generator,
        std::unique_ptr<RouteManager> routes,
        std::unique_ptr<ThreadPool> threadPool
    ) : parser_(std::move(parser)),
        generator_(std::move(generator)),
        routes_(std::move(routes)),
        threadPool_(std::move(threadPool)) {}
};
```

## Core Server Class Design

Here's a well-structured server class:

```cpp
#include <memory>
#include <functional>
#include <map>
#include <string>

// Forward declarations
class RequestParser;
class ResponseGenerator;
class RouteManager;
class ThreadPool;
class Logger;

class HttpServer {
public:
    // Configuration struct
    struct Config {
        int port = 8080;
        size_t maxConnections = 1000;
        size_t threadPoolSize = 4;
        std::string staticFilesPath = "public";
        bool enableCors = true;
        std::string corsOrigin = "*";
        int requestTimeoutMs = 30000;
        int keepAliveTimeoutMs = 5000;
    };

    // Constructor with configuration
    explicit HttpServer(const Config& config);

    // Destructor
    ~HttpServer();

    // Non-copyable, movable
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) noexcept;
    HttpServer& operator=(HttpServer&&) noexcept;

    // Server control
    bool start();
    void stop();
    bool isRunning() const;

    // Route management
    void addRoute(const std::string& method, const std::string& path,
                  std::function<HttpResponse(const HttpRequest&)> handler);

    // Middleware support
    void addMiddleware(std::function<void(HttpRequest&, HttpResponse&)> middleware);

    // Configuration
    const Config& getConfig() const { return config_; }
    void updateConfig(const Config& newConfig);

    // Statistics
    struct Stats {
        size_t totalRequests = 0;
        size_t activeConnections = 0;
        size_t requestsPerSecond = 0;
        std::chrono::steady_clock::time_point startTime;
    };

    Stats getStats() const;

private:
    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> pImpl_;

    // Configuration
    Config config_;

    // Core components
    std::unique_ptr<RequestParser> parser_;
    std::unique_ptr<ResponseGenerator> generator_;
    std::unique_ptr<RouteManager> routes_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<Logger> logger_;

    // State
    std::atomic<bool> running_{false};
    std::atomic<size_t> activeConnections_{0};

    // Private methods
    bool initializeSocket();
    void cleanupSocket();
    void acceptConnections();
    void handleClient(int clientSocket);
    void updateStats();
};
```

## PIMPL Idiom Implementation

The PIMPL (Pointer to Implementation) idiom hides implementation details:

```cpp
// HttpServer.cpp
#include "HttpServer.h"

class HttpServer::Impl {
public:
    Impl(const Config& config) : config_(config) {}

    bool initializeSocket() {
        // Socket initialization logic
        return true;
    }

    void cleanupSocket() {
        // Cleanup logic
    }

    void acceptConnections() {
        // Connection acceptance logic
    }

    void handleClient(int clientSocket) {
        // Client handling logic
    }

private:
    Config config_;
    int serverSocket_ = -1;
    // ... other implementation details
};

HttpServer::HttpServer(const Config& config)
    : config_(config), pImpl_(std::make_unique<Impl>(config)) {

    // Initialize components
    parser_ = std::make_unique<RequestParser>();
    generator_ = std::make_unique<ResponseGenerator>();
    routes_ = std::make_unique<RouteManager>();
    threadPool_ = std::make_unique<ThreadPool>(config.threadPoolSize);
    logger_ = std::make_unique<Logger>();
}

HttpServer::~HttpServer() = default;

bool HttpServer::start() {
    if (running_) {
        return false;
    }

    if (!pImpl_->initializeSocket()) {
        logger_->error("Failed to initialize socket");
        return false;
    }

    running_ = true;

    // Start accepting connections in separate thread
    std::thread([this]() {
        pImpl_->acceptConnections();
    }).detach();

    logger_->info("Server started on port " + std::to_string(config_.port));
    return true;
}
```

## Route Management System

A flexible routing system:

```cpp
class RouteManager {
public:
    using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

    struct Route {
        std::string method;
        std::string path;
        RouteHandler handler;
        std::vector<std::string> middleware;
        std::map<std::string, std::string> constraints;
    };

    void addRoute(const std::string& method, const std::string& path,
                  RouteHandler handler, const std::vector<std::string>& middleware = {}) {
        routes_.push_back({method, path, handler, middleware, {}});
    }

    HttpResponse handleRequest(const HttpRequest& request) {
        for (const auto& route : routes_) {
            if (matchesRoute(request, route)) {
                return executeRoute(request, route);
            }
        }

        return HttpResponse{404, "Not Found", "Route not found"};
    }

private:
    std::vector<Route> routes_;

    bool matchesRoute(const HttpRequest& request, const Route& route) {
        if (request.method != route.method) {
            return false;
        }

        return matchPath(request.path, route.path);
    }

    bool matchPath(const std::string& requestPath, const std::string& routePath) {
        // Simple path matching (can be enhanced with regex)
        if (routePath.find(":") != std::string::npos) {
            return matchParameterizedPath(requestPath, routePath);
        }

        return requestPath == routePath;
    }

    bool matchParameterizedPath(const std::string& requestPath, const std::string& routePath) {
        // Handle /users/:id patterns
        std::vector<std::string> requestParts = splitPath(requestPath);
        std::vector<std::string> routeParts = splitPath(routePath);

        if (requestParts.size() != routeParts.size()) {
            return false;
        }

        for (size_t i = 0; i < routeParts.size(); ++i) {
            if (routeParts[i].starts_with(":")) {
                continue; // Parameter matches anything
            }
            if (requestParts[i] != routeParts[i]) {
                return false;
            }
        }

        return true;
    }

    std::vector<std::string> splitPath(const std::string& path) {
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string part;

        while (std::getline(ss, part, '/')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }

        return parts;
    }

    HttpResponse executeRoute(const HttpRequest& request, const Route& route) {
        // Execute middleware
        for (const auto& middlewareName : route.middleware) {
            auto middleware = getMiddleware(middlewareName);
            if (middleware) {
                middleware(request);
            }
        }

        // Execute handler
        return route.handler(request);
    }

    std::function<void(HttpRequest&)> getMiddleware(const std::string& name) {
        // Return middleware function
        return {};
    }
};
```

## Configuration Management

Flexible configuration system:

```cpp
class ConfigManager {
public:
    // Load from file
    bool loadFromFile(const std::string& filename);

    // Load from environment variables
    void loadFromEnvironment();

    // Load from command line arguments
    void loadFromCommandLine(int argc, char* argv[]);

    // Get configuration value
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const;

    // Set configuration value
    template<typename T>
    void set(const std::string& key, const T& value);

    // Validate configuration
    bool validate() const;

    // Get configuration as JSON
    std::string toJson() const;

private:
    std::map<std::string, std::string> config_;

    // Helper methods
    std::string getEnvVar(const std::string& name) const;
    bool parseConfigLine(const std::string& line);
};
```

## Error Handling Strategy

Comprehensive error handling:

```cpp
class ServerException : public std::runtime_error {
public:
    enum class ErrorType {
        CONFIGURATION_ERROR,
        SOCKET_ERROR,
        ROUTING_ERROR,
        INTERNAL_ERROR
    };

    ServerException(ErrorType type, const std::string& message)
        : std::runtime_error(message), type_(type) {}

    ErrorType getType() const { return type_; }

private:
    ErrorType type_;
};

class ErrorHandler {
public:
    static HttpResponse handleError(const ServerException& e) {
        switch (e.getType()) {
            case ServerException::ErrorType::CONFIGURATION_ERROR:
                return createErrorResponse(500, "Configuration Error", e.what());
            case ServerException::ErrorType::SOCKET_ERROR:
                return createErrorResponse(503, "Service Unavailable", e.what());
            case ServerException::ErrorType::ROUTING_ERROR:
                return createErrorResponse(404, "Not Found", e.what());
            case ServerException::ErrorType::INTERNAL_ERROR:
                return createErrorResponse(500, "Internal Server Error", e.what());
            default:
                return createErrorResponse(500, "Unknown Error", e.what());
        }
    }

private:
    static HttpResponse createErrorResponse(int status, const std::string& title,
                                         const std::string& message) {
        std::ostringstream body;
        body << "<html><head><title>" << title << "</title></head>"
             << "<body><h1>" << title << "</h1>"
             << "<p>" << message << "</p>"
             << "<hr><p>C++ HTTP Server</p></body></html>";

        return HttpResponse{status, title, body.str()};
    }
};
```

## Testing the Server

Unit testing with dependency injection:

```cpp
#include <gtest/gtest.h>

class HttpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock components
        auto mockParser = std::make_unique<MockRequestParser>();
        auto mockGenerator = std::make_unique<MockResponseGenerator>();
        auto mockRoutes = std::make_unique<MockRouteManager>();
        auto mockThreadPool = std::make_unique<MockThreadPool>();

        // Create server with mocks
        server_ = std::make_unique<HttpServer>(
            HttpServer::Config{8080, 100, 2},
            std::move(mockParser),
            std::move(mockGenerator),
            std::move(mockRoutes),
            std::move(mockThreadPool)
        );
    }

    std::unique_ptr<HttpServer> server_;
};

TEST_F(HttpServerTest, StartServer) {
    EXPECT_TRUE(server_->start());
    EXPECT_TRUE(server_->isRunning());
}

TEST_F(HttpServerTest, StopServer) {
    server_->start();
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}
```

## Interactive Exercise

Design a server configuration class:

```cpp
class ServerConfig {
public:
    // TODO: Implement configuration management
    // 1. Add configuration options (port, threads, timeouts, etc.)
    // 2. Implement validation methods
    // 3. Add methods to load from different sources
    // 4. Implement default values

private:
    // TODO: Add private members
};
```

## Next Steps

Now that you understand server architecture, let's implement route handling.

<div class="next-lesson">
    <a href="/course/building-blocks/route-handling" class="btn btn-primary">Next: Route Handling and Middleware →</a>
</div>
