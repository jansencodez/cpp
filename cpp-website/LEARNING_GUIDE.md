# 🚀 C++ HTTP Server Learning Guide

## 📋 **Project Overview**

This project demonstrates how to build a **production-ready HTTP server** using only **standard C++17** and **system libraries**. No external dependencies required!

**What you'll learn:**

- Socket programming fundamentals
- HTTP protocol implementation
- Multi-threaded server architecture
- Modern C++ design patterns
- Error handling and resource management

---

## 🏗️ **Project Architecture**

```
mobileapp/
├── CMakeLists.txt          # Build configuration
├── config.json             # Server settings
├── include/
│   └── HttpServer.h        # Server class interface
├── src/
│   ├── main.cpp            # Entry point & route setup
│   └── HttpServer.cpp      # Server implementation
└── build/                  # Compiled output
```

---

## 🔧 **Core Components Deep Dive**

### **1. HttpServer Class (`include/HttpServer.h`)**

#### **Class Design Principles**

```cpp
class HttpServer {
public:
    using RouteHandler = std::function<std::string(const std::string&, const std::map<std::string, std::string>&)>;
    // ...
};
```

**Key Concepts:**

- **`using` alias**: Creates a type alias for function signatures
- **`std::function`**: Modern C++ way to store callable objects
- **Function signature**: `(body, headers) -> response_body`

#### **Modern C++ Features Used**

- **`std::unique_ptr`**: Smart pointer for automatic memory management
- **`std::atomic<bool>`**: Thread-safe boolean for server state
- **`std::thread`**: Modern threading support
- **RAII**: Resource Acquisition Is Initialization

---

### **2. Socket Management (`src/HttpServer.cpp`)**

#### **Cross-Platform Socket Handling**

```cpp
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif
```

**Why This Matters:**

- **Portability**: Code works on Windows, macOS, and Linux
- **Conditional compilation**: `#ifdef` directives for platform-specific code
- **System calls**: Direct access to operating system networking APIs

#### **Socket Lifecycle**

1. **`socket()`** - Create socket descriptor
2. **`setsockopt()`** - Configure socket options (reuse address)
3. **`bind()`** - Bind to specific port
4. **`listen()`** - Start accepting connections
5. **`accept()`** - Accept incoming connections
6. **`close()`** - Clean up resources

---

### **3. HTTP Protocol Implementation**

#### **Request Parsing**

```cpp
std::string HttpServer::parseHttpRequest(const std::string& request,
                                       std::string& method,
                                       std::string& path,
                                       std::map<std::string, std::string>& headers)
```

**What This Does:**

- **Parses HTTP request line**: `GET /health HTTP/1.1`
- **Extracts headers**: `Content-Type: application/json`
- **Separates body**: Request payload (if any)

**HTTP Request Structure:**

```
GET /health HTTP/1.1          ← Request line
Host: localhost:8080          ← Headers
User-Agent: curl/8.7.1
Accept: */*

[Body content here]            ← Body (optional)
```

#### **Response Generation**

```cpp
std::string HttpServer::createHttpResponse(int statusCode,
                                         const std::string& contentType,
                                         const std::string& body)
```

**HTTP Response Structure:**

```
HTTP/1.1 200 OK               ← Status line
Content-Type: application/json ← Headers
Content-Length: 45
Access-Control-Allow-Origin: *
Connection: close

{"status": "healthy"}         ← Body
```

---

### **4. Multi-Threading Architecture**

#### **Thread Management**

```cpp
void HttpServer::acceptConnections() {
    while (running_) {
        int clientSocket = accept(serverSocket_, ...);
        // Handle client in new thread
        std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}
```

**Key Concepts:**

- **Main thread**: Accepts new connections
- **Worker threads**: Handle individual client requests
- **`detach()`**: Thread runs independently (fire-and-forget)
- **Thread safety**: `std::atomic<bool>` for safe shutdown

#### **Why Multi-Threading?**

- **Concurrency**: Handle multiple clients simultaneously
- **Responsiveness**: Server doesn't block on slow clients
- **Scalability**: Can handle hundreds of concurrent connections

---

### **5. Route Handling System**

#### **Route Registration**

```cpp
void HttpServer::addRoute(const std::string& method,
                         const std::string& path,
                         RouteHandler handler)
```

**Route Storage Structure:**

```cpp
std::map<std::string, std::map<std::string, RouteHandler>> routes_;
//           method           path        handler
//           GET              /health     function
```

#### **Route Matching**

```cpp
auto methodIt = routes_.find(method);        // Find HTTP method
if (methodIt != routes_.end()) {
    auto pathIt = methodIt->second.find(path); // Find path
    if (pathIt != methodIt->second.end()) {
        responseBody = pathIt->second(body, headers); // Execute handler
    }
}
```

---

## 🎯 **Learning Exercises**

### **Exercise 1: Add a New Endpoint**

Add a POST endpoint to create new users:

```cpp
// In main.cpp, add this handler:
std::string handleCreateUser(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)headers;
    // Parse JSON body and add user
    return "{\"success\": true, \"message\": \"User created\"}";
}

// Register the route:
server.addRoute("POST", "/api/users", handleCreateUser);
```

### **Exercise 2: Implement JSON Parsing**

Add proper JSON request body parsing:

```cpp
#include <nlohmann/json.hpp>

std::string handleCreateUser(const std::string& body, const std::map<std::string, std::string>& headers) {
    try {
        auto json = nlohmann::json::parse(body);
        std::string name = json["name"];
        // Process user creation...
    } catch (const std::exception& e) {
        return "{\"error\": \"Invalid JSON\"}";
    }
}
```

### **Exercise 3: Add Database Integration**

Integrate SQLite for persistent storage:

```cpp
#include <sqlite3.h>

class HttpServer {
private:
    sqlite3* db_;
    // ... existing members
};
```

---

## 🔍 **Code Analysis Questions**

### **Understanding the Design**

1. **Why use `std::function` instead of function pointers?**

   - **Answer**: `std::function` can store any callable object (functions, lambdas, member functions, etc.)

2. **Why `detach()` threads instead of `join()`?**

   - **Answer**: `detach()` allows the main thread to continue accepting connections while worker threads handle requests independently

3. **Why use `std::atomic<bool>` for the running flag?**

   - **Answer**: Multiple threads access this variable, so we need atomic operations to prevent race conditions

4. **Why separate request parsing from response generation?**
   - **Answer**: Separation of concerns - parsing and response creation are independent operations that can be tested separately

---

## 🚨 **Common Pitfalls & Solutions**

### **1. Socket Resource Management**

```cpp
// ❌ BAD: Manual resource management
int socket = socket(AF_INET, SOCK_STREAM, 0);
// ... use socket ...
close(socket); // What if exception occurs before this?

// ✅ GOOD: RAII with smart pointers
class SocketGuard {
    int socket_;
public:
    SocketGuard(int s) : socket_(s) {}
    ~SocketGuard() { close(socket_); }
};
```

### **2. Thread Safety**

```cpp
// ❌ BAD: Race condition
bool running = false; // Multiple threads access this

// ✅ GOOD: Atomic operations
std::atomic<bool> running_{false};
```

### **3. Error Handling**

```cpp
// ❌ BAD: Ignoring errors
int result = bind(socket, ...);
// What if bind fails?

// ✅ GOOD: Check and handle errors
if (bind(socket, ...) < 0) {
    std::cerr << "Bind failed: " << strerror(errno) << std::endl;
    return false;
}
```

---

## 📚 **Advanced Topics to Explore**

### **1. Async I/O with `std::async`**

```cpp
auto future = std::async(std::launch::async, [this, clientSocket]() {
    return handleClient(clientSocket);
});
```

### **2. Event-Driven Architecture**

```cpp
// Use epoll (Linux) or kqueue (macOS) for non-blocking I/O
int epoll_fd = epoll_create1(0);
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket, &event);
```

### **3. Memory Pools**

```cpp
// Pre-allocate buffers for better performance
class BufferPool {
    std::vector<std::vector<char>> buffers_;
public:
    std::vector<char>& getBuffer();
    void returnBuffer(std::vector<char>& buffer);
};
```

---

## 🧪 **Testing Your Understanding**

### **Quiz Questions**

1. **What happens if you don't call `close()` on a socket?**

   - **Answer**: Socket descriptor leak - the OS thinks the socket is still in use

2. **Why do we need `SO_REUSEADDR`?**

   - **Answer**: Allows the server to restart immediately without waiting for the OS to release the port

3. **What's the difference between `std::thread` and `std::async`?**

   - **Answer**: `std::thread` gives you direct control, `std::async` provides a higher-level interface with automatic resource management

4. **Why use `std::map` for routes instead of `std::vector`?**
   - **Answer**: `std::map` provides O(log n) lookup time, while `std::vector` would be O(n)

---

## 🎓 **Next Steps in Your Learning Journey**

### **Immediate (This Week)**

1. **Understand every line** of the current code
2. **Add a POST endpoint** for creating users
3. **Implement proper error handling** for malformed requests
4. **Add request logging** to see what's happening

### **Short Term (Next Month)**

1. **Database integration** with SQLite
2. **Authentication system** with JWT tokens
3. **Configuration file parsing** (JSON/YAML)
4. **Unit tests** using Google Test or Catch2

### **Long Term (Next 3 Months)**

1. **Load balancing** across multiple server instances
2. **SSL/TLS support** for HTTPS
3. **API documentation** with OpenAPI/Swagger
4. **Performance profiling** and optimization

---

## 🔗 **Additional Resources**

- **Beej's Guide to Network Programming**: https://beej.us/guide/bgnet/
- **C++ Reference**: https://en.cppreference.com/
- **CppCon Videos**: https://www.youtube.com/user/CppCon
- **Stack Overflow C++**: https://stackoverflow.com/questions/tagged/c%2b%2b

---

## 💡 **Pro Tips for Learning**

1. **Read the code line by line** - Don't skip anything
2. **Use a debugger** - Step through the code to see execution flow
3. **Modify and experiment** - Break things to understand how they work
4. **Build incrementally** - Add one feature at a time
5. **Document your learning** - Write down what you discover

---

**Remember**: This server is a **foundation** - it's designed to be simple but extensible. Every line of code teaches you something about systems programming, networking, and modern C++ design.

Happy coding! 🚀
