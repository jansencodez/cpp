# Authentication & Security

Learn how to implement secure authentication systems and protect your C++ server from common security vulnerabilities.

## Why Authentication Matters

Modern web applications need secure authentication to:

- Protect user data and privacy
- Control access to resources
- Prevent unauthorized actions
- Comply with security regulations
- Build user trust

## Authentication Methods

### 1. Session-Based Authentication

```cpp
#include <string>
#include <map>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

class SessionManager {
private:
    struct Session {
        std::string userId;
        std::string token;
        std::chrono::system_clock::time_point expiresAt;
        std::map<std::string, std::string> data;
    };

    std::map<std::string, Session> sessions_;
    std::mutex sessionsMutex_;
    std::chrono::minutes sessionTimeout_;

public:
    SessionManager(std::chrono::minutes timeout = std::chrono::minutes(30))
        : sessionTimeout_(timeout) {}

    std::string createSession(const std::string& userId) {
        std::lock_guard<std::mutex> lock(sessionsMutex_);

        // Generate secure random token
        std::string token = generateSecureToken();

        // Create session
        Session session;
        session.userId = userId;
        session.token = token;
        session.expiresAt = std::chrono::system_clock::now() + sessionTimeout_;

        sessions_[token] = session;

        return token;
    }

    bool validateSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessionsMutex_);

        auto it = sessions_.find(token);
        if (it == sessions_.end()) {
            return false;
        }

        // Check if session expired
        if (std::chrono::system_clock::now() > it->second.expiresAt) {
            sessions_.erase(it);
            return false;
        }

        // Extend session
        it->second.expiresAt = std::chrono::system_clock::now() + sessionTimeout_;
        return true;
    }

    std::string getUserId(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessionsMutex_);

        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            return it->second.userId;
        }
        return "";
    }

    void invalidateSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions_.erase(token);
    }

    void cleanupExpiredSessions() {
        std::lock_guard<std::mutex> lock(sessionsMutex_);

        auto now = std::chrono::system_clock::now();
        auto it = sessions_.begin();

        while (it != sessions_.end()) {
            if (now > it->second.expiresAt) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    std::string generateSecureToken() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        std::vector<unsigned char> randomBytes(32);
        for (auto& byte : randomBytes) {
            byte = dis(gen);
        }

        // Convert to hex string
        std::ostringstream oss;
        for (unsigned char byte : randomBytes) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }

        return oss.str();
    }
};
```

### 2. JWT (JSON Web Token) Authentication

```cpp
#include <string>
#include <map>
#include <chrono>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <base64.h>

class JWTManager {
private:
    std::string secretKey_;
    std::chrono::minutes tokenExpiry_;

public:
    JWTManager(const std::string& secretKey, std::chrono::minutes expiry = std::chrono::minutes(60))
        : secretKey_(secretKey), tokenExpiry_(expiry) {}

    std::string createToken(const std::map<std::string, std::string>& claims) {
        // Create header
        std::map<std::string, std::string> header = {
            {"alg", "HS256"},
            {"typ", "JWT"}
        };

        // Create payload
        std::map<std::string, std::string> payload = claims;
        auto now = std::chrono::system_clock::now();
        auto expiry = now + tokenExpiry_;

        auto nowTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto expiryTime = std::chrono::duration_cast<std::chrono::seconds>(expiry.time_since_epoch()).count();

        payload["iat"] = std::to_string(nowTime);
        payload["exp"] = std::to_string(expiryTime);

        // Encode header and payload
        std::string headerJson = mapToJson(header);
        std::string payloadJson = mapToJson(payload);

        std::string headerB64 = base64Encode(headerJson);
        std::string payloadB64 = base64Encode(payloadJson);

        // Create signature
        std::string data = headerB64 + "." + payloadB64;
        std::string signature = createHMAC(data, secretKey_);
        std::string signatureB64 = base64Encode(signature);

        return data + "." + signatureB64;
    }

    bool validateToken(const std::string& token) {
        try {
            size_t firstDot = token.find('.');
            size_t secondDot = token.find('.', firstDot + 1);

            if (firstDot == std::string::npos || secondDot == std::string::npos) {
                return false;
            }

            std::string headerB64 = token.substr(0, firstDot);
            std::string payloadB64 = token.substr(firstDot + 1, secondDot - firstDot - 1);
            std::string signatureB64 = token.substr(secondDot + 1);

            // Verify signature
            std::string data = headerB64 + "." + payloadB64;
            std::string expectedSignature = createHMAC(data, secretKey_);
            std::string expectedSignatureB64 = base64Encode(expectedSignature);

            if (signatureB64 != expectedSignatureB64) {
                return false;
            }

            // Check expiration
            std::string payloadJson = base64Decode(payloadB64);
            auto payload = jsonToMap(payloadJson);

            auto expIt = payload.find("exp");
            if (expIt == payload.end()) {
                return false;
            }

            auto expiryTime = std::stoll(expIt->second);
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            if (now > expiryTime) {
                return false;
            }

            return true;

        } catch (...) {
            return false;
        }
    }

    std::map<std::string, std::string> getClaims(const std::string& token) {
        if (!validateToken(token)) {
            return {};
        }

        size_t firstDot = token.find('.');
        size_t secondDot = token.find('.', firstDot + 1);

        std::string payloadB64 = token.substr(firstDot + 1, secondDot - firstDot - 1);
        std::string payloadJson = base64Decode(payloadB64);

        return jsonToMap(payloadJson);
    }

private:
    std::string createHMAC(const std::string& data, const std::string& key) {
        unsigned char hash[32];
        unsigned int hashLen;

        HMAC(EVP_sha256(), key.c_str(), key.length(),
              reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
              hash, &hashLen);

        return std::string(reinterpret_cast<char*>(hash), hashLen);
    }

    std::string mapToJson(const std::map<std::string, std::string>& map) {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& [key, value] : map) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }
        oss << "}";
        return oss.str();
    }

    std::map<std::string, std::string> jsonToMap(const std::string& json) {
        // Simplified JSON parsing - in production use a proper JSON library
        std::map<std::string, std::string> result;
        // Implementation details...
        return result;
    }

    std::string base64Encode(const std::string& data) {
        // Use a proper base64 library in production
        return data; // Simplified for example
    }

    std::string base64Decode(const std::string& data) {
        // Use a proper base64 library in production
        return data; // Simplified for example
    }
};
```

## Password Security

### 1. Secure Password Hashing

```cpp
#include <string>
#include <random>
#include <openssl/evp.h>
#include <openssl/rand.h>

class PasswordManager {
private:
    static constexpr size_t SALT_LENGTH = 32;
    static constexpr size_t HASH_LENGTH = 64;
    static constexpr int ITERATIONS = 100000;

public:
    struct HashedPassword {
        std::string hash;
        std::string salt;
        int iterations;
    };

    static HashedPassword hashPassword(const std::string& password) {
        HashedPassword result;

        // Generate random salt
        result.salt.resize(SALT_LENGTH);
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&result.salt[0]), SALT_LENGTH) != 1) {
            throw std::runtime_error("Failed to generate salt");
        }

        result.iterations = ITERATIONS;

        // Hash password using PBKDF2
        result.hash.resize(HASH_LENGTH);
        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                               reinterpret_cast<const unsigned char*>(result.salt.c_str()),
                               result.salt.length(), result.iterations,
                               EVP_sha256(), HASH_LENGTH,
                               reinterpret_cast<unsigned char*>(&result.hash[0])) != 1) {
            throw std::runtime_error("Failed to hash password");
        }

        return result;
    }

    static bool verifyPassword(const std::string& password, const HashedPassword& hashed) {
        std::string computedHash(HASH_LENGTH, 0);

        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                               reinterpret_cast<const unsigned char*>(hashed.salt.c_str()),
                               hashed.salt.length(), hashed.iterations,
                               EVP_sha256(), HASH_LENGTH,
                               reinterpret_cast<unsigned char*>(&computedHash[0])) != 1) {
            return false;
        }

        return computedHash == hashed.hash;
    }

    static std::string generateSecurePassword(size_t length = 16) {
        const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, charset.length() - 1);

        std::string password;
        password.reserve(length);

        for (size_t i = 0; i < length; ++i) {
            password += charset[dis(gen)];
        }

        return password;
    }
};
```

## Role-Based Access Control (RBAC)

```cpp
#include <string>
#include <vector>
#include <map>
#include <set>

class RBACManager {
private:
    struct Role {
        std::string name;
        std::set<std::string> permissions;
        std::set<std::string> inheritedRoles;
    };

    struct User {
        std::string id;
        std::set<std::string> roles;
    };

    std::map<std::string, Role> roles_;
    std::map<std::string, User> users_;

public:
    void addRole(const std::string& roleName, const std::vector<std::string>& permissions = {}) {
        Role role;
        role.name = roleName;
        role.permissions.insert(permissions.begin(), permissions.end());
        roles_[roleName] = role;
    }

    void addPermissionToRole(const std::string& roleName, const std::string& permission) {
        auto it = roles_.find(roleName);
        if (it != roles_.end()) {
            it->second.permissions.insert(permission);
        }
    }

    void inheritRole(const std::string& roleName, const std::string& inheritedRole) {
        auto it = roles_.find(roleName);
        if (it != roles_.end()) {
            it->second.inheritedRoles.insert(inheritedRole);
        }
    }

    void assignRoleToUser(const std::string& userId, const std::string& roleName) {
        auto it = users_.find(userId);
        if (it != users_.end()) {
            it->second.roles.insert(roleName);
        } else {
            User user;
            user.id = userId;
            user.roles.insert(roleName);
            users_[userId] = user;
        }
    }

    bool hasPermission(const std::string& userId, const std::string& permission) {
        auto userIt = users_.find(userId);
        if (userIt == users_.end()) {
            return false;
        }

        std::set<std::string> allPermissions;

        // Collect all permissions from user's roles
        for (const auto& roleName : userIt->second.roles) {
            collectRolePermissions(roleName, allPermissions);
        }

        return allPermissions.find(permission) != allPermissions.end();
    }

    std::vector<std::string> getUserPermissions(const std::string& userId) {
        auto userIt = users_.find(userId);
        if (userIt == users_.end()) {
            return {};
        }

        std::set<std::string> allPermissions;

        for (const auto& roleName : userIt->second.roles) {
            collectRolePermissions(roleName, allPermissions);
        }

        return std::vector<std::string>(allPermissions.begin(), allPermissions.end());
    }

private:
    void collectRolePermissions(const std::string& roleName, std::set<std::string>& permissions) {
        auto roleIt = roles_.find(roleName);
        if (roleIt == roles_.end()) {
            return;
        }

        // Add direct permissions
        permissions.insert(roleIt->second.permissions.begin(), roleIt->second.permissions.end());

        // Add inherited permissions
        for (const auto& inheritedRole : roleIt->second.inheritedRoles) {
            collectRolePermissions(inheritedRole, permissions);
        }
    }
};
```

## Security Middleware

```cpp
#include <string>
#include <map>
#include <functional>

class SecurityMiddleware {
private:
    std::shared_ptr<SessionManager> sessionManager_;
    std::shared_ptr<RBACManager> rbacManager_;

public:
    SecurityMiddleware(std::shared_ptr<SessionManager> sessionManager,
                      std::shared_ptr<RBACManager> rbacManager)
        : sessionManager_(sessionManager), rbacManager_(rbacManager) {}

    std::function<std::string(const std::map<std::string, std::string>&,
                              const std::map<std::string, std::string>&)>
    requireAuth(const std::string& requiredPermission = "") {
        return [this, requiredPermission](const std::map<std::string, std::string>& headers,
                                        const std::map<std::string, std::string>& params) -> std::string {
            // Extract session token from headers
            auto authIt = headers.find("Authorization");
            if (authIt == headers.end()) {
                return "{\"error\": \"No authorization header\"}";
            }

            std::string authHeader = authIt->second;
            if (authHeader.substr(0, 7) != "Bearer ") {
                return "{\"error\": \"Invalid authorization format\"}";
            }

            std::string token = authHeader.substr(7);

            // Validate session
            if (!sessionManager_->validateSession(token)) {
                return "{\"error\": \"Invalid or expired session\"}";
            }

            // Check permission if required
            if (!requiredPermission.empty()) {
                std::string userId = sessionManager_->getUserId(token);
                if (!rbacManager_->hasPermission(userId, requiredPermission)) {
                    return "{\"error\": \"Insufficient permissions\"}";
                }
            }

            return ""; // Authentication successful
        };
    }

    std::function<std::string(const std::map<std::string, std::string>&,
                              const std::map<std::string, std::string>&)>
    requireRole(const std::string& requiredRole) {
        return [this, requiredRole](const std::map<std::string, std::string>& headers,
                                   const std::map<std::string, std::string>& params) -> std::string {
            // Extract session token
            auto authIt = headers.find("Authorization");
            if (authIt == headers.end()) {
                return "{\"error\": \"No authorization header\"}";
            }

            std::string token = authIt->second.substr(7);

            if (!sessionManager_->validateSession(token)) {
                return "{\"error\": \"Invalid or expired session\"}";
            }

            // Check role
            std::string userId = sessionManager_->getUserId(token);
            auto permissions = rbacManager_->getUserPermissions(userId);

            // This is a simplified role check - in practice you'd have a more sophisticated role system
            bool hasRole = false;
            for (const auto& permission : permissions) {
                if (permission.find(requiredRole) != std::string::npos) {
                    hasRole = true;
                    break;
                }
            }

            if (!hasRole) {
                return "{\"error\": \"Insufficient role\"}";
            }

            return ""; // Authorization successful
        };
    }
};
```

## Input Validation and Sanitization

```cpp
#include <string>
#include <regex>
#include <vector>

class InputValidator {
public:
    static bool isValidEmail(const std::string& email) {
        std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        return std::regex_match(email, emailRegex);
    }

    static bool isValidUsername(const std::string& username) {
        // Username: 3-20 characters, alphanumeric and underscore only
        std::regex usernameRegex(R"([a-zA-Z0-9_]{3,20})");
        return std::regex_match(username, usernameRegex);
    }

    static bool isValidPassword(const std::string& password) {
        // Password: at least 8 characters, contains uppercase, lowercase, number, and special char
        if (password.length() < 8) return false;

        bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;

        for (char c : password) {
            if (std::isupper(c)) hasUpper = true;
            else if (std::islower(c)) hasLower = true;
            else if (std::isdigit(c)) hasDigit = true;
            else hasSpecial = true;
        }

        return hasUpper && hasLower && hasDigit && hasSpecial;
    }

    static std::string sanitizeHtml(const std::string& input) {
        std::string sanitized = input;

        // Replace potentially dangerous characters
        std::map<std::string, std::string> replacements = {
            {"<", "&lt;"},
            {">", "&gt;"},
            {"\"", "&quot;"},
            {"'", "&#39;"},
            {"&", "&amp;"}
        };

        for (const auto& [from, to] : replacements) {
            size_t pos = 0;
            while ((pos = sanitized.find(from, pos)) != std::string::npos) {
                sanitized.replace(pos, from.length(), to);
                pos += to.length();
            }
        }

        return sanitized;
    }

    static std::string sanitizeSql(const std::string& input) {
        // This is a basic example - in production, always use prepared statements
        std::string sanitized = input;

        // Remove SQL injection patterns
        std::vector<std::string> dangerousPatterns = {
            "DROP", "DELETE", "UPDATE", "INSERT", "SELECT", "UNION", "EXEC", "EXECUTE"
        };

        for (const auto& pattern : dangerousPatterns) {
            size_t pos = 0;
            while ((pos = sanitized.find(pattern, pos)) != std::string::npos) {
                sanitized.replace(pos, pattern.length(), "");
            }
        }

        return sanitized;
    }
};
```

## Rate Limiting

```cpp
#include <chrono>
#include <map>
#include <mutex>

class RateLimiter {
private:
    struct RequestCount {
        int count;
        std::chrono::system_clock::time_point windowStart;
    };

    std::map<std::string, RequestCount> requestCounts_;
    std::mutex rateLimitMutex_;
    int maxRequests_;
    std::chrono::seconds windowSize_;

public:
    RateLimiter(int maxRequests = 100, std::chrono::seconds window = std::chrono::seconds(60))
        : maxRequests_(maxRequests), windowSize_(window) {}

    bool allowRequest(const std::string& identifier) {
        std::lock_guard<std::mutex> lock(rateLimitMutex_);

        auto now = std::chrono::system_clock::now();
        auto it = requestCounts_.find(identifier);

        if (it == requestCounts_.end()) {
            // First request from this identifier
            requestCounts_[identifier] = {1, now};
            return true;
        }

        // Check if window has expired
        if (now - it->second.windowStart > windowSize_) {
            it->second.count = 1;
            it->second.windowStart = now;
            return true;
        }

        // Check if limit exceeded
        if (it->second.count >= maxRequests_) {
            return false;
        }

        // Increment count
        it->second.count++;
        return true;
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(rateLimitMutex_);

        auto now = std::chrono::system_clock::now();
        auto it = requestCounts_.begin();

        while (it != requestCounts_.end()) {
            if (now - it->second.windowStart > windowSize_) {
                it = requestCounts_.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

## Testing Security Features

```cpp
#include <cassert>
#include <iostream>

void testSecurityFeatures() {
    try {
        // Test password hashing
        std::string password = "MySecurePassword123!";
        auto hashed = PasswordManager::hashPassword(password);

        assert(PasswordManager::verifyPassword(password, hashed));
        assert(!PasswordManager::verifyPassword("WrongPassword", hashed));
        std::cout << "✓ Password hashing passed" << std::endl;

        // Test session management
        SessionManager sessionManager;
        std::string token = sessionManager.createSession("user123");

        assert(sessionManager.validateSession(token));
        assert(sessionManager.getUserId(token) == "user123");
        std::cout << "✓ Session management passed" << std::endl;

        // Test RBAC
        RBACManager rbac;
        rbac.addRole("admin", {"read", "write", "delete"});
        rbac.addRole("user", {"read"});
        rbac.assignRoleToUser("user123", "admin");

        assert(rbac.hasPermission("user123", "read"));
        assert(rbac.hasPermission("user123", "delete"));
        assert(!rbac.hasPermission("user123", "nonexistent"));
        std::cout << "✓ RBAC passed" << std::endl;

        // Test input validation
        assert(InputValidator::isValidEmail("test@example.com"));
        assert(!InputValidator::isValidEmail("invalid-email"));
        assert(InputValidator::isValidUsername("valid_user_123"));
        assert(!InputValidator::isValidUsername("invalid user"));
        std::cout << "✓ Input validation passed" << std::endl;

        std::cout << "All security tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Security test failed: " << e.what() << std::endl;
    }
}
```

## Security Best Practices

### 1. Always Use HTTPS

- Encrypt all data in transit
- Use strong TLS configurations
- Implement HSTS headers

### 2. Implement Proper Session Management

- Use secure, random session tokens
- Implement session expiration
- Secure session storage

### 3. Validate and Sanitize All Input

- Never trust user input
- Use whitelist validation
- Implement proper escaping

### 4. Use Prepared Statements

- Prevent SQL injection attacks
- Use parameterized queries
- Validate input types

### 5. Implement Rate Limiting

- Prevent brute force attacks
- Protect against DDoS
- Monitor suspicious activity

### 6. Regular Security Audits

- Review code for vulnerabilities
- Test authentication systems
- Update dependencies regularly

## Next Steps

Now that you understand authentication and security, let's move on to error handling and logging.

<div class="next-lesson">
    <a href="/course/advanced-features/error-handling" class="btn btn-primary">Next: Error Handling & Logging →</a>
</div>
