# 🎯 C++ HTTP Server Practical Exercises

## 🚀 **Hands-On Learning Challenges**

These exercises will help you understand the code better and build new skills. Complete them in order - each builds on the previous one.

---

## **Exercise 1: Understanding the Code Flow** ⭐

### **Objective**: Trace through the code execution

**What to do:**

1. **Set a breakpoint** in your debugger at `main()` function
2. **Step through** the code line by line
3. **Observe** what happens when a request comes in

**Questions to answer:**

- What happens when `server.start()` is called?
- How does the server accept a new connection?
- What's the flow from receiving a request to sending a response?

**Expected outcome**: You'll understand the complete request lifecycle

---

## **Exercise 2: Add Request Logging** ⭐⭐

### **Objective**: Implement basic logging

**What to do:**

1. **Add logging** to the `handleClient` method
2. **Log** the method, path, and response status
3. **Format** logs with timestamps

**Code template:**

```cpp
// In HttpServer.cpp, modify handleClient method
void HttpServer::handleClient(int clientSocket) {
    // ... existing code ...

    // Add logging here
    std::cout << "[" << getCurrentTime() << "] "
              << method << " " << path
              << " -> " << statusCode << std::endl;

    // ... rest of the method ...
}
```

**Expected outcome**: You'll see all requests in the console with timestamps

---

## **Exercise 3: Implement POST Endpoint** ⭐⭐

### **Objective**: Add user creation capability

**What to do:**

1. **Create** a new handler function for POST requests
2. **Parse** the request body (start with simple text)
3. **Add** the route to your server
4. **Test** with curl

**Code template:**

```cpp
// In main.cpp, add this handler:
std::string handleCreateUser(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)headers;

    // Simple text parsing for now
    if (body.find("name=") != std::string::npos) {
        // Extract name from body
        size_t pos = body.find("name=") + 5;
        size_t end = body.find("&", pos);
        std::string name = body.substr(pos, end - pos);

        // Add to users (in-memory for now)
        // Return success response
        return "{\"success\": true, \"message\": \"User created: " + name + "\"}";
    }

    return "{\"error\": \"Invalid request body\"}";
}

// Register the route:
server.addRoute("POST", "/api/users", handleCreateUser);
```

**Test with:**

```bash
curl -X POST -d "name=Alice&email=alice@example.com" http://localhost:8080/api/users
```

**Expected outcome**: You'll be able to create new users via POST requests

---

## **Exercise 4: Add JSON Request Parsing** ⭐⭐⭐

### **Objective**: Handle JSON request bodies properly

**What to do:**

1. **Install** nlohmann/json library
2. **Modify** the POST handler to parse JSON
3. **Validate** the JSON structure
4. **Return** proper JSON responses

**Code template:**

```cpp
#include <nlohmann/json.hpp>

std::string handleCreateUser(const std::string& body, const std::map<std::string, std::string>& headers) {
    try {
        auto json = nlohmann::json::parse(body);

        // Validate required fields
        if (!json.contains("name") || !json.contains("email")) {
            return "{\"error\": \"Missing required fields: name and email\"}";
        }

        std::string name = json["name"];
        std::string email = json["email"];

        // Create user object
        nlohmann::json newUser = {
            {"id", std::to_string(users_.size() + 1)},
            {"name", name},
            {"email", email},
            {"age", json.value("age", 0)}
        };

        // Add to users (in-memory for now)
        users_.push_back({{"id", newUser["id"]}, {"name", name}, {"email", email}});

        return newUser.dump(2);

    } catch (const std::exception& e) {
        return "{\"error\": \"Invalid JSON: " + std::string(e.what()) + "\"}";
    }
}
```

**Test with:**

```bash
curl -X POST -H "Content-Type: application/json" \
     -d '{"name":"Alice","email":"alice@example.com","age":28}' \
     http://localhost:8080/api/users
```

**Expected outcome**: You'll handle structured JSON data and return proper responses

---

## **Exercise 5: Implement User Updates** ⭐⭐⭐

### **Objective**: Add PUT endpoint for updating users

**What to do:**

1. **Create** a PUT handler for `/api/users/{id}`
2. **Parse** the user ID from the URL path
3. **Update** existing user data
4. **Handle** cases where user doesn't exist

**Code template:**

```cpp
std::string handleUpdateUser(const std::string& body, const std::map<std::string, std::string>& headers, const std::string& path) {
    try {
        // Extract user ID from path (simplified)
        std::string userId = path.substr(path.find_last_of('/') + 1);

        // Find user
        auto userIt = std::find_if(users_.begin(), users_.end(),
            [&userId](const std::map<std::string, std::string>& user) {
                return user.at("id") == userId;
            });

        if (userIt == users_.end()) {
            return "{\"error\": \"User not found\"}";
        }

        // Parse JSON body
        auto json = nlohmann::json::parse(body);

        // Update user fields
        if (json.contains("name")) (*userIt)["name"] = json["name"];
        if (json.contains("email")) (*userIt)["email"] = json["email"];
        if (json.contains("age")) (*userIt)["age"] = json["age"].get<std::string>();

        return "{\"success\": true, \"message\": \"User updated\"}";

    } catch (const std::exception& e) {
        return "{\"error\": \"Update failed: " + std::string(e.what()) + "\"}";
    }
}

// Register the route (you'll need to modify the routing system for dynamic paths)
server.addRoute("PUT", "/api/users/1", [](const std::string& body, const std::map<std::string, std::string>& headers) {
    return handleUpdateUser(body, headers, "/api/users/1");
});
```

**Test with:**

```bash
curl -X PUT -H "Content-Type: application/json" \
     -d '{"name":"Alice Updated","age":29}' \
     http://localhost:8080/api/users/1
```

**Expected outcome**: You'll be able to update existing user information

---

## **Exercise 6: Add Error Handling & Validation** ⭐⭐⭐⭐

### **Objective**: Implement robust error handling

**What to do:**

1. **Add** input validation for all endpoints
2. **Implement** proper HTTP status codes
3. **Create** consistent error response format
4. **Handle** edge cases gracefully

**Code template:**

```cpp
// Create a validation helper
struct ValidationResult {
    bool isValid;
    std::string errorMessage;
    int statusCode;
};

ValidationResult validateUserData(const nlohmann::json& json) {
    if (!json.contains("name")) {
        return {false, "Name is required", 400};
    }

    if (!json.contains("email")) {
        return {false, "Email is required", 400};
    }

    std::string email = json["email"];
    if (email.find('@') == std::string::npos) {
        return {false, "Invalid email format", 400};
    }

    if (json.contains("age")) {
        int age = json["age"];
        if (age < 0 || age > 150) {
            return {false, "Age must be between 0 and 150", 400};
        }
    }

    return {true, "", 200};
}

// Use in your handlers
std::string handleCreateUser(const std::string& body, const std::map<std::string, std::string>& headers) {
    try {
        auto json = nlohmann::json::parse(body);

        auto validation = validateUserData(json);
        if (!validation.isValid) {
            return createErrorResponse(validation.errorMessage, validation.statusCode);
        }

        // ... rest of the handler ...

    } catch (const std::exception& e) {
        return createErrorResponse("Invalid JSON format", 400);
    }
}
```

**Expected outcome**: Your API will be more robust and user-friendly

---

## **Exercise 7: Implement Database Storage** ⭐⭐⭐⭐⭐

### **Objective**: Add persistent storage with SQLite

**What to do:**

1. **Install** SQLite development libraries
2. **Create** database schema for users
3. **Modify** handlers to use database instead of in-memory storage
4. **Handle** database errors gracefully

**Code template:**

```cpp
#include <sqlite3.h>

class HttpServer {
private:
    sqlite3* db_;

    bool initializeDatabase() {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                email TEXT UNIQUE NOT NULL,
                age INTEGER
            );
        )";

        char* errMsg = 0;
        int rc = sqlite3_exec(db_, sql, 0, 0, &errMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

public:
    HttpServer(int port = 8080) : port_(port), serverSocket_(-1), running_(false), db_(nullptr) {
        // Initialize database
        int rc = sqlite3_open("users.db", &db_);
        if (rc) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
            return;
        }

        if (!initializeDatabase()) {
            std::cerr << "Failed to initialize database" << std::endl;
            return;
        }

        // ... rest of constructor ...
    }

    ~HttpServer() {
        if (db_) {
            sqlite3_close(db_);
        }
        stop();
    }
};
```

**Expected outcome**: Your data will persist between server restarts

---

## **Exercise 8: Add Authentication** ⭐⭐⭐⭐⭐

### **Objective**: Implement basic authentication system

**What to do:**

1. **Create** user login/registration endpoints
2. **Implement** JWT token generation and validation
3. **Add** middleware for protected routes
4. **Store** hashed passwords securely

**This is advanced - tackle it after mastering the basics!**

---

## 🎯 **How to Use These Exercises**

### **1. Complete in Order**

- Each exercise builds on the previous one
- Don't skip ahead until you understand the current exercise

### **2. Test Everything**

- Use curl to test your endpoints
- Check the server logs
- Verify responses are correct

### **3. Break Things**

- Intentionally send malformed requests
- See how your error handling works
- Learn from failures

### **4. Document Your Progress**

- Keep notes on what you learned
- Write down questions that arise
- Track your understanding

---

## 🚨 **Common Issues & Solutions**

### **Server won't start**

- Check if port 8080 is already in use
- Look for compilation errors
- Verify all dependencies are installed

### **Requests hang**

- Check if server is actually running
- Look for infinite loops in your code
- Verify socket operations are working

### **JSON parsing fails**

- Check request Content-Type header
- Verify JSON syntax is valid
- Handle parsing exceptions properly

---

## 🎓 **Success Metrics**

**You're ready for the next level when you can:**

- ✅ Explain every line of the original code
- ✅ Add new endpoints without breaking existing ones
- ✅ Handle errors gracefully
- ✅ Parse and validate different types of input
- ✅ Store data persistently
- ✅ Debug issues independently

---

**Remember**: The goal isn't to complete all exercises quickly, but to **understand** what you're building. Take your time, experiment, and don't be afraid to make mistakes!

Happy coding! 🚀
