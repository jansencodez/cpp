# Database Integration

Learn how to integrate databases with your C++ server for persistent data storage and management.

## Why Database Integration?

Modern web applications require persistent data storage for:

- User accounts and authentication
- Content management
- Analytics and logging
- Configuration settings
- Session management

## Database Options for C++

### 1. SQLite (Embedded)

```cpp
#include <sqlite3.h>
#include <string>
#include <vector>

class SQLiteManager {
private:
    sqlite3* db_;

public:
    SQLiteManager(const std::string& dbPath) {
        int rc = sqlite3_open(dbPath.c_str(), &db_);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to open database");
        }
    }

    ~SQLiteManager() {
        if (db_) sqlite3_close(db_);
    }

    bool executeQuery(const std::string& sql) {
        char* errMsg = 0;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error("SQL error: " + error);
        }
        return true;
    }

    std::vector<std::map<std::string, std::string>> selectQuery(const std::string& sql) {
        std::vector<std::map<std::string, std::string>> results;
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement");
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::map<std::string, std::string> row;
            int cols = sqlite3_column_count(stmt);

            for (int i = 0; i < cols; i++) {
                std::string colName = sqlite3_column_name(stmt, i);
                std::string colValue = (char*)sqlite3_column_text(stmt, i);
                row[colName] = colValue;
            }
            results.push_back(row);
        }

        sqlite3_finalize(stmt);
        return results;
    }
};
```

### 2. MySQL/PostgreSQL (Client-Server)

```cpp
#include <mysql/mysql.h>
#include <string>
#include <vector>

class MySQLManager {
private:
    MYSQL* mysql_;

public:
    MySQLManager() {
        mysql_ = mysql_init(nullptr);
        if (!mysql_) {
            throw std::runtime_error("Failed to initialize MySQL");
        }
    }

    ~MySQLManager() {
        if (mysql_) mysql_close(mysql_);
    }

    bool connect(const std::string& host, const std::string& user,
                 const std::string& password, const std::string& database) {
        if (!mysql_real_connect(mysql_, host.c_str(), user.c_str(),
                               password.c_str(), database.c_str(), 0, nullptr, 0)) {
            throw std::runtime_error("Failed to connect to MySQL");
        }
        return true;
    }

    bool executeQuery(const std::string& sql) {
        int rc = mysql_query(mysql_, sql.c_str());
        if (rc != 0) {
            throw std::runtime_error("MySQL error: " + std::string(mysql_error(mysql_)));
        }
        return true;
    }

    std::vector<std::map<std::string, std::string>> selectQuery(const std::string& sql) {
        std::vector<std::map<std::string, std::string>> results;

        if (mysql_query(mysql_, sql.c_str()) != 0) {
            throw std::runtime_error("MySQL error: " + std::string(mysql_error(mysql_)));
        }

        MYSQL_RES* result = mysql_store_result(mysql_);
        if (!result) {
            throw std::runtime_error("Failed to store result");
        }

        MYSQL_FIELD* fields = mysql_fetch_fields(result);
        int numFields = mysql_num_fields(result);

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            std::map<std::string, std::string> rowData;
            for (int i = 0; i < numFields; i++) {
                std::string fieldName = fields[i].name;
                std::string fieldValue = row[i] ? row[i] : "";
                rowData[fieldName] = fieldValue;
            }
            results.push_back(rowData);
        }

        mysql_free_result(result);
        return results;
    }
};
```

## Connection Pooling

For high-performance applications, implement connection pooling:

```cpp
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template<typename DBManager>
class ConnectionPool {
private:
    std::queue<std::unique_ptr<DBManager>> connections_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t maxConnections_;
    size_t currentConnections_;

public:
    ConnectionPool(size_t maxConnections = 10)
        : maxConnections_(maxConnections), currentConnections_(0) {}

    std::unique_ptr<DBManager> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait for available connection
        cv_.wait(lock, [this] {
            return !connections_.empty() || currentConnections_ < maxConnections_;
        });

        if (!connections_.empty()) {
            auto conn = std::move(connections_.front());
            connections_.pop();
            return conn;
        } else {
            // Create new connection
            currentConnections_++;
            return std::make_unique<DBManager>();
        }
    }

    void returnConnection(std::unique_ptr<DBManager> connection) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (connections_.size() < maxConnections_) {
            connections_.push(std::move(connection));
        } else {
            currentConnections_--;
        }
        cv_.notify_one();
    }
};
```

## Database Schema Design

### User Management

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE user_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    session_token VARCHAR(255) UNIQUE NOT NULL,
    expires_at TIMESTAMP NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

### Content Management

```sql
CREATE TABLE posts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title VARCHAR(200) NOT NULL,
    content TEXT NOT NULL,
    author_id INTEGER NOT NULL,
    status VARCHAR(20) DEFAULT 'draft',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (author_id) REFERENCES users(id)
);

CREATE TABLE categories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    parent_id INTEGER,
    FOREIGN KEY (parent_id) REFERENCES categories(id)
);
```

## Integration with HTTP Server

```cpp
class DatabaseHandler {
private:
    std::shared_ptr<ConnectionPool<SQLiteManager>> dbPool_;

public:
    DatabaseHandler(const std::string& dbPath) {
        dbPool_ = std::make_shared<ConnectionPool<SQLiteManager>>(5);

        // Initialize database schema
        auto conn = dbPool_->getConnection();
        initializeSchema(*conn);
        dbPool_->returnConnection(std::move(conn));
    }

    std::string handleUserRegistration(const std::map<std::string, std::string>& userData) {
        auto conn = dbPool_->getConnection();

        try {
            // Hash password
            std::string hashedPassword = hashPassword(userData.at("password"));

            // Insert user
            std::string sql = "INSERT INTO users (username, email, password_hash) VALUES (?, ?, ?)";
            // Execute prepared statement...

            dbPool_->returnConnection(std::move(conn));
            return "{\"status\": \"success\", \"message\": \"User registered\"}";

        } catch (const std::exception& e) {
            dbPool_->returnConnection(std::move(conn));
            return "{\"status\": \"error\", \"message\": \"" + std::string(e.what()) + "\"}";
        }
    }

    std::string handleUserLogin(const std::string& username, const std::string& password) {
        auto conn = dbPool_->getConnection();

        try {
            std::string sql = "SELECT id, password_hash FROM users WHERE username = ?";
            auto results = conn->selectQuery(sql);

            if (results.empty()) {
                dbPool_->returnConnection(std::move(conn));
                return "{\"status\": \"error\", \"message\": \"Invalid credentials\"}";
            }

            // Verify password and create session
            // ...

            dbPool_->returnConnection(std::move(conn));
            return "{\"status\": \"success\", \"token\": \"session_token_here\"}";

        } catch (const std::exception& e) {
            dbPool_->returnConnection(std::move(conn));
            return "{\"status\": \"error\", \"message\": \"" + std::string(e.what()) + "\"}";
        }
    }

private:
    void initializeSchema(SQLiteManager& db) {
        // Create tables if they don't exist
        db.executeQuery(R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username VARCHAR(50) UNIQUE NOT NULL,
                email VARCHAR(100) UNIQUE NOT NULL,
                password_hash VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )");

        // Add more table creation statements...
    }

    std::string hashPassword(const std::string& password) {
        // In production, use proper password hashing (bcrypt, Argon2, etc.)
        return "hashed_" + password; // Simplified for example
    }
};
```

## Error Handling and Transactions

```cpp
class TransactionManager {
private:
    sqlite3* db_;

public:
    TransactionManager(sqlite3* db) : db_(db) {}

    bool beginTransaction() {
        return executeQuery("BEGIN TRANSACTION");
    }

    bool commitTransaction() {
        return executeQuery("COMMIT");
    }

    bool rollbackTransaction() {
        return executeQuery("ROLLBACK");
    }

    template<typename Func>
    bool executeInTransaction(Func func) {
        try {
            if (!beginTransaction()) return false;

            bool result = func();

            if (result) {
                return commitTransaction();
            } else {
                rollbackTransaction();
                return false;
            }
        } catch (...) {
            rollbackTransaction();
            throw;
        }
    }

private:
    bool executeQuery(const std::string& sql) {
        char* errMsg = 0;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }
};
```

## Performance Optimization

### 1. Prepared Statements

```cpp
class PreparedStatement {
private:
    sqlite3_stmt* stmt_;

public:
    PreparedStatement(sqlite3* db, const std::string& sql) {
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement");
        }
    }

    ~PreparedStatement() {
        if (stmt_) sqlite3_finalize(stmt_);
    }

    void bind(int position, const std::string& value) {
        sqlite3_bind_text(stmt_, position, value.c_str(), -1, SQLITE_TRANSIENT);
    }

    void bind(int position, int value) {
        sqlite3_bind_int(stmt_, position, value);
    }

    bool step() {
        return sqlite3_step(stmt_) == SQLITE_ROW;
    }

    void reset() {
        sqlite3_reset(stmt_);
    }
};
```

### 2. Indexing

```sql
-- Create indexes for frequently queried columns
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_posts_author_id ON posts(author_id);
CREATE INDEX idx_posts_status ON posts(status);
CREATE INDEX idx_posts_created_at ON posts(created_at);
```

## Security Considerations

### 1. SQL Injection Prevention

- Always use prepared statements
- Never concatenate user input into SQL strings
- Validate and sanitize all input data

### 2. Connection Security

- Use SSL/TLS for database connections
- Implement proper authentication
- Limit database user permissions

### 3. Data Validation

```cpp
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

    static std::string sanitizeInput(const std::string& input) {
        // Remove potentially dangerous characters
        std::string sanitized = input;
        // Implementation details...
        return sanitized;
    }
};
```

## Testing Database Integration

```cpp
#include <cassert>
#include <iostream>

void testDatabaseIntegration() {
    try {
        // Test database connection
        SQLiteManager db(":memory:"); // In-memory database for testing

        // Test table creation
        db.executeQuery("CREATE TABLE test (id INTEGER, name TEXT)");
        std::cout << "✓ Table creation passed" << std::endl;

        // Test data insertion
        db.executeQuery("INSERT INTO test VALUES (1, 'Test User')");
        std::cout << "✓ Data insertion passed" << std::endl;

        // Test data retrieval
        auto results = db.selectQuery("SELECT * FROM test WHERE id = 1");
        assert(results.size() == 1);
        assert(results[0]["name"] == "Test User");
        std::cout << "✓ Data retrieval passed" << std::endl;

        std::cout << "All database tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Database test failed: " << e.what() << std::endl;
    }
}
```

## Next Steps

Now that you understand database integration, let's move on to authentication and security.

<div class="next-lesson">
    <a href="/course/advanced-features/authentication" class="btn btn-primary">Next: Authentication & Security →</a>
</div>
