# Route Handling and Middleware

Route handling is the heart of any web server. In this lesson, you'll learn how to implement flexible, powerful routing systems that can handle complex URL patterns and middleware chains.

## Route System Architecture

A modern routing system should support:

- **Pattern Matching**: Handle dynamic URLs with parameters
- **HTTP Methods**: Support GET, POST, PUT, DELETE, etc.
- **Middleware**: Execute code before/after route handlers
- **Route Groups**: Organize related routes
- **Constraints**: Validate route parameters
- **Performance**: Fast route matching

## Basic Route Implementation

Here's a simple but effective route system:

```cpp
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <regex>

class Route {
public:
    std::string method;
    std::string pattern;
    std::function<HttpResponse(const HttpRequest&)> handler;
    std::vector<std::string> middleware;

    Route(const std::string& m, const std::string& p,
          std::function<HttpResponse(const HttpRequest&)> h)
        : method(m), pattern(p), handler(h) {}
};

class Router {
private:
    std::vector<Route> routes_;

public:
    void addRoute(const std::string& method, const std::string& pattern,
                  std::function<HttpResponse(const HttpRequest&)> handler) {
        routes_.emplace_back(method, pattern, handler);
    }

    HttpResponse handleRequest(const HttpRequest& request) {
        for (const auto& route : routes_) {
            if (matchesRoute(request, route)) {
                return route.handler(request);
            }
        }

        return HttpResponse{404, "Not Found", "Route not found"};
    }

private:
    bool matchesRoute(const HttpRequest& request, const Route& route) {
        if (request.method != route.method) {
            return false;
        }

        return matchPattern(request.path, route.pattern);
    }

    bool matchPattern(const std::string& path, const std::string& pattern) {
        // Convert pattern to regex
        std::string regexPattern = pattern;

        // Replace :param with capture groups
        std::regex paramRegex(R"(:(\w+))");
        regexPattern = std::regex_replace(regexPattern, paramRegex, R"(([^/]+))");

        // Escape other regex special characters
        std::regex escapeRegex(R"([.*+?^${}()|[\]\\])");
        regexPattern = std::regex_replace(regexPattern, escapeRegex, R"(\$&)");

        // Add start/end anchors
        regexPattern = "^" + regexPattern + "$";

        std::regex routeRegex(regexPattern);
        return std::regex_match(path, routeRegex);
    }
};
```

## Advanced Pattern Matching

Support for complex URL patterns:

```cpp
class AdvancedRouter {
public:
    struct RouteMatch {
        bool matched;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query;
    };

    void addRoute(const std::string& method, const std::string& pattern,
                  std::function<HttpResponse(const HttpRequest&)> handler) {
        routes_.emplace_back(method, pattern, handler);
    }

    RouteMatch matchRoute(const HttpRequest& request) {
        for (const auto& route : routes_) {
            if (request.method == route.method) {
                auto match = matchPattern(request.path, route.pattern);
                if (match.matched) {
                    return match;
                }
            }
        }

        return {false, {}, {}};
    }

private:
    std::vector<Route> routes_;

    RouteMatch matchPattern(const std::string& path, const std::string& pattern) {
        RouteMatch match{false, {}, {}};

        // Parse pattern into segments
        auto patternSegments = splitPath(pattern);
        auto pathSegments = splitPath(path);

        if (patternSegments.size() != pathSegments.size()) {
            return match;
        }

        for (size_t i = 0; i < patternSegments.size(); ++i) {
            if (patternSegments[i].starts_with(":")) {
                // Parameter segment
                std::string paramName = patternSegments[i].substr(1);
                match.params[paramName] = pathSegments[i];
            } else if (patternSegments[i] == "*") {
                // Wildcard segment - matches anything
                continue;
            } else if (patternSegments[i] != pathSegments[i]) {
                // Static segment must match exactly
                return {false, {}, {}};
            }
        }

        match.matched = true;
        return match;
    }

    std::vector<std::string> splitPath(const std::string& path) {
        std::vector<std::string> segments;
        std::stringstream ss(path);
        std::string segment;

        while (std::getline(ss, segment, '/')) {
            if (!segment.empty()) {
                segments.push_back(segment);
            }
        }

        return segments;
    }
};
```

## Middleware System

Middleware allows you to execute code before and after route handlers:

```cpp
class Middleware {
public:
    virtual ~Middleware() = default;

    virtual void process(HttpRequest& request, HttpResponse& response,
                        std::function<void()> next) = 0;
};

class MiddlewareChain {
private:
    std::vector<std::shared_ptr<Middleware>> middleware_;

public:
    void add(std::shared_ptr<Middleware> middleware) {
        middleware_.push_back(middleware);
    }

    void execute(HttpRequest& request, HttpResponse& response,
                 std::function<HttpResponse(const HttpRequest&)> handler) {
        executeMiddleware(request, response, 0, handler);
    }

private:
    void executeMiddleware(HttpRequest& request, HttpResponse& response,
                          size_t index, std::function<HttpResponse(const HttpRequest&)> handler) {
        if (index >= middleware_.size()) {
            // Execute the actual route handler
            response = handler(request);
            return;
        }

        middleware_[index]->process(request, response, [this, &request, &response, index, handler]() {
            executeMiddleware(request, response, index + 1, handler);
        });
    }
};
```

## Common Middleware Examples

### 1. Logging Middleware

```cpp
class LoggingMiddleware : public Middleware {
public:
    void process(HttpRequest& request, HttpResponse& response,
                 std::function<void()> next) override {
        auto start = std::chrono::high_resolution_clock::now();

        // Execute next middleware/handler
        next();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << request.method << " " << request.path << " - "
                  << response.statusCode << " (" << duration.count() << "ms)" << std::endl;
    }
};
```

### 2. Authentication Middleware

```cpp
class AuthMiddleware : public Middleware {
private:
    std::string secretKey_;

public:
    AuthMiddleware(const std::string& secretKey) : secretKey_(secretKey) {}

    void process(HttpRequest& request, HttpResponse& response,
                 std::function<void()> next) override {
        auto authHeader = request.headers.find("Authorization");

        if (authHeader == request.headers.end()) {
            response = HttpResponse{401, "Unauthorized", "Authentication required"};
            return;
        }

        if (!validateToken(authHeader->second)) {
            response = HttpResponse{403, "Forbidden", "Invalid token"};
            return;
        }

        // Token is valid, continue to next middleware
        next();
    }

private:
    bool validateToken(const std::string& token) {
        // Implement JWT validation or other auth logic
        return token.starts_with("Bearer ") && token.length() > 7;
    }
};
```

### 3. CORS Middleware

```cpp
class CorsMiddleware : public Middleware {
private:
    std::string allowedOrigin_;
    std::vector<std::string> allowedMethods_;

public:
    CorsMiddleware(const std::string& origin = "*",
                   const std::vector<std::string>& methods = {"GET", "POST", "PUT", "DELETE"})
        : allowedOrigin_(origin), allowedMethods_(methods) {}

    void process(HttpRequest& request, HttpResponse& response,
                 std::function<void()> next) override {
        // Handle preflight request
        if (request.method == "OPTIONS") {
            response.headers["Access-Control-Allow-Origin"] = allowedOrigin_;
            response.headers["Access-Control-Allow-Methods"] = join(allowedMethods_, ", ");
            response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
            response.statusCode = 200;
            return;
        }

        // Execute next middleware/handler
        next();

        // Add CORS headers to response
        response.headers["Access-Control-Allow-Origin"] = allowedOrigin_;
        response.headers["Access-Control-Allow-Methods"] = join(allowedMethods_, ", ");
    }

private:
    std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
        std::ostringstream result;
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i > 0) result << delimiter;
            result << strings[i];
        }
        return result.str();
    }
};
```

## Route Groups and Organization

Organize related routes together:

```cpp
class RouteGroup {
private:
    std::string prefix_;
    std::vector<std::string> middleware_;
    Router& router_;

public:
    RouteGroup(Router& router, const std::string& prefix = "",
               const std::vector<std::string>& middleware = {})
        : prefix_(prefix), middleware_(middleware), router_(router) {}

    RouteGroup& middleware(const std::string& name) {
        middleware_.push_back(name);
        return *this;
    }

    RouteGroup& get(const std::string& path,
                    std::function<HttpResponse(const HttpRequest&)> handler) {
        router_.addRoute("GET", prefix_ + path, handler);
        return *this;
    }

    RouteGroup& post(const std::string& path,
                     std::function<HttpResponse(const HttpRequest&)> handler) {
        router_.addRoute("POST", prefix_ + path, handler);
        return *this;
    }

    RouteGroup& put(const std::string& path,
                    std::function<HttpResponse(const HttpRequest&)> handler) {
        router_.addRoute("PUT", prefix_ + path, handler);
        return *this;
    }

    RouteGroup& del(const std::string& path,
                    std::function<HttpResponse(const HttpRequest&)> handler) {
        router_.addRoute("DELETE", prefix_ + path, handler);
        return *this;
    }
};
```

## Using the Router

Here's how to use the complete routing system:

```cpp
int main() {
    Router router;
    MiddlewareChain middlewareChain;

    // Add middleware
    middlewareChain.add(std::make_shared<LoggingMiddleware>());
    middlewareChain.add(std::make_shared<CorsMiddleware>());

    // Add routes
    router.addRoute("GET", "/", [](const HttpRequest& req) {
        return HttpResponse{200, "OK", "<h1>Welcome to C++ Server!</h1>"};
    });

    router.addRoute("GET", "/users/:id", [](const HttpRequest& req) {
        // Extract user ID from params
        std::string userId = req.params.at("id");
        return HttpResponse{200, "OK", "User: " + userId};
    });

    router.addRoute("POST", "/users", [](const HttpRequest& req) {
        // Create new user
        return HttpResponse{201, "Created", "User created successfully"};
    });

    // API routes with middleware
    RouteGroup api(router, "/api");
    api.middleware("auth")
       .get("/users", [](const HttpRequest& req) {
           return HttpResponse{200, "OK", "User list"};
       })
       .post("/users", [](const HttpRequest& req) {
           return HttpResponse{201, "Created", "User created"};
       });

    // Start server
    HttpServer server(8080);
    server.setRouter(router);
    server.setMiddlewareChain(middlewareChain);
    server.start();

    return 0;
}
```

## Performance Optimization

### 1. Route Caching

Cache compiled regex patterns:

```cpp
class CachedRouter {
private:
    std::map<std::string, std::regex> compiledPatterns_;
    std::mutex patternsMutex_;

    std::regex getCompiledPattern(const std::string& pattern) {
        std::lock_guard<std::mutex> lock(patternsMutex_);

        auto it = compiledPatterns_.find(pattern);
        if (it != compiledPatterns_.end()) {
            return it->second;
        }

        std::regex compiled = compilePattern(pattern);
        compiledPatterns_[pattern] = compiled;
        return compiled;
    }
};
```

### 2. Trie-based Routing

For very large route sets, use a trie data structure:

```cpp
class TrieNode {
public:
    std::map<std::string, std::unique_ptr<TrieNode>> children;
    std::map<std::string, Route> routes;  // method -> route
    std::string paramName;  // for :param segments
    bool isParam = false;
};

class TrieRouter {
private:
    std::unique_ptr<TrieNode> root_;

public:
    void addRoute(const std::string& method, const std::string& path,
                  std::function<HttpResponse(const HttpRequest&)> handler) {
        auto segments = splitPath(path);
        addRouteToTrie(root_.get(), segments, 0, method, handler);
    }

private:
    void addRouteToTrie(TrieNode* node, const std::vector<std::string>& segments,
                        size_t index, const std::string& method,
                        std::function<HttpResponse(const HttpRequest&)> handler) {
        if (index >= segments.size()) {
            // End of path, add route
            node->routes[method] = Route{method, "", handler};
            return;
        }

        std::string segment = segments[index];

        if (segment.starts_with(":")) {
            // Parameter segment
            if (!node->isParam) {
                node->isParam = true;
                node->paramName = segment.substr(1);
                node->children["*"] = std::make_unique<TrieNode>();
            }
            addRouteToTrie(node->children["*"].get(), segments, index + 1, method, handler);
        } else {
            // Static segment
            if (node->children.find(segment) == node->children.end()) {
                node->children[segment] = std::make_unique<TrieNode>();
            }
            addRouteToTrie(node->children[segment].get(), segments, index + 1, method, handler);
        }
    }
};
```

## Interactive Exercise

Implement a route parameter extractor:

```cpp
class RouteParameterExtractor {
public:
    struct ExtractedParams {
        std::map<std::string, std::string> params;
        bool matched;
    };

    // TODO: Implement parameter extraction
    // 1. Parse route pattern (e.g., "/users/:id/posts/:postId")
    // 2. Extract parameters from actual path (e.g., "/users/123/posts/456")
    // 3. Return map of parameter names to values

    ExtractedParams extract(const std::string& pattern, const std::string& path) {
        // TODO: Implement this method
        return {std::map<std::string, std::string>{}, false};
    }

private:
    // TODO: Add helper methods
};
```

## Next Steps

Now that you understand route handling, let's implement request parsing and validation.

<div class="next-lesson">
    <a href="/course/building-blocks/request-parsing" class="btn btn-primary">Next: Request Parsing and Validation →</a>
</div>
