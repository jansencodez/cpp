# Production Setup

Learn how to deploy your C++ server in production environments with proper configuration, monitoring, and deployment strategies.

## Why Production Setup Matters

Production deployment requires:

- **Reliability**: 99.9%+ uptime
- **Scalability**: Handle production load
- **Security**: Protect against attacks
- **Monitoring**: Track performance and errors
- **Maintenance**: Easy updates and rollbacks

## Production Environment Configuration

### 1. Environment Configuration

```cpp
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>

class EnvironmentConfig {
private:
    std::map<std::string, std::string> config_;
    std::string configFile_;

public:
    EnvironmentConfig(const std::string& configFile = "config.env")
        : configFile_(configFile) {
        loadFromFile();
        loadFromEnvironment();
    }

    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        auto it = config_.find(key);
        return it != config_.end() ? it->second : defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = config_.find(key);
        if (it == config_.end()) return defaultValue;

        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }

    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = config_.find(key);
        if (it == config_.end()) return defaultValue;

        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return value == "true" || value == "1" || value == "yes";
    }

    void set(const std::string& key, const std::string& value) {
        config_[key] = value;
    }

    void saveToFile() const {
        std::ofstream file(configFile_);
        if (file.is_open()) {
            for (const auto& [key, value] : config_) {
                file << key << "=" << value << std::endl;
            }
        }
    }

private:
    void loadFromFile() {
        std::ifstream file(configFile_);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // Remove quotes if present
                if (!value.empty() && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }

                config_[key] = value;
            }
        }
    }

    void loadFromEnvironment() {
        // Load from system environment variables
        std::vector<std::string> envVars = {
            "SERVER_PORT", "SERVER_HOST", "DB_HOST", "DB_PORT", "DB_NAME",
            "DB_USER", "DB_PASSWORD", "LOG_LEVEL", "ENVIRONMENT", "DEBUG_MODE"
        };

        for (const auto& var : envVars) {
            const char* envValue = std::getenv(var.c_str());
            if (envValue) {
                config_[var] = envValue;
            }
        }
    }
};

// Configuration file example (config.env)
/*
SERVER_PORT=8080
SERVER_HOST=0.0.0.0
DB_HOST=localhost
DB_PORT=5432
DB_NAME=myapp
DB_USER=myuser
DB_PASSWORD=mypassword
LOG_LEVEL=INFO
ENVIRONMENT=production
DEBUG_MODE=false
MAX_CONNECTIONS=1000
THREAD_POOL_SIZE=8
SESSION_TIMEOUT=3600
*/
```

### 2. Production Server Configuration

```cpp
#include <string>
#include <memory>

class ProductionServer {
private:
    std::unique_ptr<EnvironmentConfig> config_;
    std::unique_ptr<HttpServer> httpServer_;
    std::unique_ptr<Logger> logger_;

    // Production settings
    int maxConnections_;
    int threadPoolSize_;
    int sessionTimeout_;
    bool enableSSL_;
    std::string sslCertPath_;
    std::string sslKeyPath_;

public:
    ProductionServer() {
        config_ = std::make_unique<EnvironmentConfig>();
        initializeProductionSettings();
        setupLogging();
        setupServer();
    }

    void start() {
        try {
            logger_->info("Starting production server...");
            logger_->info("Configuration loaded", {
                {"port", config_->get("SERVER_PORT")},
                {"host", config_->get("SERVER_HOST")},
                {"environment", config_->get("ENVIRONMENT")},
                {"max_connections", std::to_string(maxConnections_)},
                {"thread_pool_size", std::to_string(threadPoolSize_)}
            });

            httpServer_->start();

        } catch (const std::exception& e) {
            logger_->fatal("Failed to start server: " + std::string(e.what()));
            throw;
        }
    }

    void stop() {
        logger_->info("Stopping production server...");
        httpServer_->stop();
        logger_->info("Server stopped");
    }

    void reload() {
        logger_->info("Reloading server configuration...");

        // Reload configuration
        config_ = std::make_unique<EnvironmentConfig>();
        initializeProductionSettings();

        // Restart server with new settings
        httpServer_->stop();
        setupServer();
        httpServer_->start();

        logger_->info("Server reloaded successfully");
    }

private:
    void initializeProductionSettings() {
        maxConnections_ = config_->getInt("MAX_CONNECTIONS", 1000);
        threadPoolSize_ = config_->getInt("THREAD_POOL_SIZE", 8);
        sessionTimeout_ = config_->getInt("SESSION_TIMEOUT", 3600);
        enableSSL_ = config_->getBool("ENABLE_SSL", false);
        sslCertPath_ = config_->get("SSL_CERT_PATH", "");
        sslKeyPath_ = config_->get("SSL_KEY_PATH", "");

        // Validate critical settings
        if (config_->get("ENVIRONMENT") == "production") {
            if (config_->getBool("DEBUG_MODE", false)) {
                throw std::runtime_error("Debug mode cannot be enabled in production");
            }

            if (config_->get("LOG_LEVEL") == "TRACE" || config_->get("LOG_LEVEL") == "DEBUG") {
                throw std::runtime_error("Trace/Debug logging cannot be enabled in production");
            }
        }
    }

    void setupLogging() {
        std::string logLevel = config_->get("LOG_LEVEL", "INFO");
        std::string logFile = config_->get("LOG_FILE", "server.log");

        logger_ = std::make_unique<Logger>();
        logger_->setLogLevel(parseLogLevel(logLevel));
        logger_->setFileOutput(true);
        logger_->setConsoleOutput(config_->get("ENVIRONMENT") != "production");
    }

    void setupServer() {
        int port = config_->getInt("SERVER_PORT", 8080);
        std::string host = config_->get("SERVER_HOST", "0.0.0.0");

        httpServer_ = std::make_unique<HttpServer>(host, port);

        // Configure server settings
        httpServer_->setMaxConnections(maxConnections_);
        httpServer_->setThreadPoolSize(threadPoolSize_);
        httpServer_->setSessionTimeout(sessionTimeout_);

        if (enableSSL_) {
            httpServer_->enableSSL(sslCertPath_, sslKeyPath_);
        }

        // Set up production routes
        setupProductionRoutes();
    }

    void setupProductionRoutes() {
        // Health check endpoint
        httpServer_->addRoute("GET", "/health", [this](const HttpRequest& req) -> HttpResponse {
            return generateHealthResponse();
        });

        // Metrics endpoint
        httpServer_->addRoute("GET", "/metrics", [this](const HttpRequest& req) -> HttpResponse {
            return generateMetricsResponse();
        });

        // Status endpoint
        httpServer_->addRoute("GET", "/status", [this](const HttpRequest& req) -> HttpResponse {
            return generateStatusResponse();
        });
    }

    LogLevel parseLogLevel(const std::string& level) {
        if (level == "TRACE") return LogLevel::TRACE;
        if (level == "DEBUG") return LogLevel::DEBUG;
        if (level == "INFO") return LogLevel::INFO;
        if (level == "WARN") return LogLevel::WARN;
        if (level == "ERROR") return LogLevel::ERROR;
        if (level == "FATAL") return LogLevel::FATAL;
        return LogLevel::INFO;
    }

    HttpResponse generateHealthResponse() {
        // Check critical services
        bool dbHealthy = checkDatabaseHealth();
        bool cacheHealthy = checkCacheHealth();

        std::string status = (dbHealthy && cacheHealthy) ? "healthy" : "unhealthy";
        int statusCode = (dbHealthy && cacheHealthy) ? 200 : 503;

        std::ostringstream response;
        response << "{";
        response << "\"status\": \"" << status << "\",";
        response << "\"timestamp\": \"" << getCurrentTimestamp() << "\",";
        response << "\"services\": {";
        response << "\"database\": " << (dbHealthy ? "true" : "false") << ",";
        response << "\"cache\": " << (cacheHealthy ? "true" : "false");
        response << "}";
        response << "}";

        return HttpResponse(statusCode, response.str(), "application/json");
    }

    HttpResponse generateMetricsResponse() {
        // Return server metrics
        std::ostringstream response;
        response << "{";
        response << "\"uptime\": " << getUptimeSeconds() << ",";
        response << "\"requests_total\": " << getTotalRequests() << ",";
        response << "\"requests_per_second\": " << getRequestsPerSecond() << ",";
        response << "\"active_connections\": " << getActiveConnections() << ",";
        response << "\"memory_usage\": " << getMemoryUsageMB() << ",";
        response << "\"cpu_usage\": " << getCPUUsagePercent();
        response << "}";

        return HttpResponse(200, response.str(), "application/json");
    }

    HttpResponse generateStatusResponse() {
        // Return detailed server status
        std::ostringstream response;
        response << "{";
        response << "\"server\": {";
        response << "\"version\": \"1.0.0\",";
        response << "\"environment\": \"" << config_->get("ENVIRONMENT") << "\",";
        response << "\"uptime\": " << getUptimeSeconds();
        response << "},";
        response << "\"configuration\": {";
        response << "\"port\": " << config_->get("SERVER_PORT") << ",";
        response << "\"max_connections\": " << maxConnections_ << ",";
        response << "\"thread_pool_size\": " << threadPoolSize_;
        response << "}";
        response << "}";

        return HttpResponse(200, response.str(), "application/json");
    }

    // Health check methods
    bool checkDatabaseHealth() {
        // Implement database health check
        return true; // Simplified
    }

    bool checkCacheHealth() {
        // Implement cache health check
        return true; // Simplified
    }

    // Utility methods
    std::string getCurrentTimestamp() const;
    long getUptimeSeconds() const;
    long getTotalRequests() const;
    double getRequestsPerSecond() const;
    int getActiveConnections() const;
    double getMemoryUsageMB() const;
    double getCPUUsagePercent() const;
};
```

## Process Management

### 1. Daemon Process

```cpp
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fstream>

class DaemonProcess {
private:
    std::string pidFile_;
    std::string logFile_;
    std::string workingDir_;
    std::unique_ptr<ProductionServer> server_;

public:
    DaemonProcess(const std::string& pidFile = "/var/run/server.pid",
                  const std::string& logFile = "/var/log/server.log",
                  const std::string& workingDir = "/var/lib/server")
        : pidFile_(pidFile), logFile_(logFile), workingDir_(workingDir) {}

    void daemonize() {
        // First fork
        pid_t pid = fork();
        if (pid < 0) {
            throw std::runtime_error("Failed to fork first time");
        }

        if (pid > 0) {
            // Parent process, exit
            exit(0);
        }

        // Create new session
        if (setsid() < 0) {
            throw std::runtime_error("Failed to create new session");
        }

        // Second fork
        pid = fork();
        if (pid < 0) {
            throw std::runtime_error("Failed to fork second time");
        }

        if (pid > 0) {
            // Parent process, exit
            exit(0);
        }

        // Set file permissions
        umask(0);

        // Change working directory
        if (chdir(workingDir_.c_str()) < 0) {
            throw std::runtime_error("Failed to change working directory");
        }

        // Close standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        // Redirect to log file
        int logFd = open(logFile_.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (logFd >= 0) {
            dup2(logFd, STDOUT_FILENO);
            dup2(logFd, STDERR_FILENO);
            close(logFd);
        }

        // Write PID file
        writePidFile();

        // Set up signal handlers
        setupSignalHandlers();
    }

    void run() {
        try {
            server_ = std::make_unique<ProductionServer>();
            server_->start();

            // Keep main thread alive
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

        } catch (const std::exception& e) {
            std::cerr << "Server error: " << e.what() << std::endl;
            cleanup();
            exit(1);
        }
    }

private:
    void writePidFile() {
        std::ofstream pidStream(pidFile_);
        if (pidStream.is_open()) {
            pidStream << getpid() << std::endl;
        }
    }

    void setupSignalHandlers() {
        signal(SIGTERM, signalHandler);
        signal(SIGINT, signalHandler);
        signal(SIGHUP, signalHandler);
    }

    static void signalHandler(int signal) {
        switch (signal) {
            case SIGTERM:
            case SIGINT:
                std::cout << "Received termination signal, shutting down..." << std::endl;
                cleanup();
                exit(0);
                break;

            case SIGHUP:
                std::cout << "Received reload signal, reloading configuration..." << std::endl;
                if (instance_) {
                    instance_->server_->reload();
                }
                break;
        }
    }

    void cleanup() {
        if (server_) {
            server_->stop();
        }

        // Remove PID file
        std::remove(pidFile_.c_str());
    }

    static DaemonProcess* instance_;
};

// Static instance for signal handling
DaemonProcess* DaemonProcess::instance_ = nullptr;
```

### 2. Systemd Service

```ini
# /etc/systemd/system/cpp-server.service
[Unit]
Description=C++ HTTP Server
After=network.target
Wants=network.target

[Service]
Type=forking
User=server
Group=server
WorkingDirectory=/var/lib/server
ExecStart=/usr/local/bin/server --daemon
ExecReload=/bin/kill -HUP $MAINPID
ExecStop=/bin/kill -TERM $MAINPID
PIDFile=/var/run/server.pid
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/server /var/log/server

# Resource limits
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
```

## SSL/TLS Configuration

### 1. SSL Context Setup

```cpp
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>

class SSLContext {
private:
    SSL_CTX* ctx_;
    std::string certPath_;
    std::string keyPath_;

public:
    SSLContext() : ctx_(nullptr) {
        initializeOpenSSL();
    }

    ~SSLContext() {
        if (ctx_) {
            SSL_CTX_free(ctx_);
        }
        cleanupOpenSSL();
    }

    bool initialize(const std::string& certPath, const std::string& keyPath) {
        certPath_ = certPath;
        keyPath_ = keyPath;

        // Create SSL context
        ctx_ = SSL_CTX_new(TLS_server_method());
        if (!ctx_) {
            std::cerr << "Failed to create SSL context" << std::endl;
            return false;
        }

        // Set SSL options
        SSL_CTX_set_options(ctx_, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
        SSL_CTX_set_options(ctx_, SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

        // Set cipher preferences
        if (!SSL_CTX_set_cipher_list(ctx_, "HIGH:!aNULL:!MD5:!RC4")) {
            std::cerr << "Failed to set cipher list" << std::endl;
            return false;
        }

        // Load certificate and private key
        if (!SSL_CTX_use_certificate_file(ctx_, certPath_.c_str(), SSL_FILETYPE_PEM)) {
            std::cerr << "Failed to load certificate file" << std::endl;
            return false;
        }

        if (!SSL_CTX_use_PrivateKey_file(ctx_, keyPath_.c_str(), SSL_FILETYPE_PEM)) {
            std::cerr << "Failed to load private key file" << std::endl;
            return false;
        }

        // Verify private key
        if (!SSL_CTX_check_private_key(ctx_)) {
            std::cerr << "Private key verification failed" << std::endl;
            return false;
        }

        // Set session cache
        SSL_CTX_set_session_cache_mode(ctx_, SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(ctx_, 1024);

        return true;
    }

    SSL* createSSL() const {
        if (!ctx_) return nullptr;
        return SSL_new(ctx_);
    }

    SSL_CTX* getContext() const { return ctx_; }

private:
    void initializeOpenSSL() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }

    void cleanupOpenSSL() {
        EVP_cleanup();
        ERR_free_strings();
    }
};
```

### 2. SSL Server Integration

```cpp
class SSLHttpServer : public HttpServer {
private:
    std::unique_ptr<SSLContext> sslContext_;
    bool sslEnabled_;

public:
    SSLHttpServer(const std::string& host, int port)
        : HttpServer(host, port), sslEnabled_(false) {}

    bool enableSSL(const std::string& certPath, const std::string& keyPath) {
        sslContext_ = std::make_unique<SSLContext>();
        if (sslContext_->initialize(certPath, keyPath)) {
            sslEnabled_ = true;
            return true;
        }
        return false;
    }

protected:
    void handleClient(int clientSocket) override {
        if (sslEnabled_) {
            handleSSLClient(clientSocket);
        } else {
            HttpServer::handleClient(clientSocket);
        }
    }

private:
    void handleSSLClient(int clientSocket) {
        SSL* ssl = sslContext_->createSSL();
        if (!ssl) {
            close(clientSocket);
            return;
        }

        SSL_set_fd(ssl, clientSocket);

        if (SSL_accept(ssl) <= 0) {
            SSL_free(ssl);
            close(clientSocket);
            return;
        }

        // Handle SSL connection
        handleSSLConnection(ssl);

        SSL_free(ssl);
        close(clientSocket);
    }

    void handleSSLConnection(SSL* ssl) {
        // Implement SSL connection handling
        // Similar to regular HTTP handling but with SSL_read/SSL_write
    }
};
```

## Deployment Scripts

### 1. Build Script

```bash
#!/bin/bash
# build.sh

set -e

echo "Building C++ Server..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DENABLE_SSL=ON \
      -DENABLE_TESTS=OFF \
      ..

# Build
make -j$(nproc)

# Install
sudo make install

echo "Build completed successfully!"
```

### 2. Deployment Script

```bash
#!/bin/bash
# deploy.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Deploying C++ Server..."

# Build the project
"$SCRIPT_DIR/build.sh"

# Create necessary directories
sudo mkdir -p /var/lib/server
sudo mkdir -p /var/log/server
sudo mkdir -p /etc/server

# Create server user
if ! id "server" &>/dev/null; then
    sudo useradd -r -s /bin/false -d /var/lib/server server
fi

# Copy configuration files
sudo cp "$PROJECT_DIR/config.env" /etc/server/
sudo chown server:server /etc/server/config.env
sudo chmod 600 /etc/server/config.env

# Copy systemd service
sudo cp "$PROJECT_DIR/deploy/cpp-server.service" /etc/systemd/system/
sudo systemctl daemon-reload

# Enable and start service
sudo systemctl enable cpp-server
sudo systemctl start cpp-server

# Check status
sudo systemctl status cpp-server

echo "Deployment completed successfully!"
```

### 3. Rollback Script

```bash
#!/bin/bash
# rollback.sh

set -e

echo "Rolling back C++ Server..."

# Stop current service
sudo systemctl stop cpp-server

# Restore previous version
sudo cp /usr/local/bin/server.backup /usr/local/bin/server

# Restore previous configuration
sudo cp /etc/server/config.env.backup /etc/server/config.env

# Start service
sudo systemctl start cpp-server

# Check status
sudo systemctl status cpp-server

echo "Rollback completed successfully!"
```

## Monitoring and Health Checks

### 1. Health Check Script

```bash
#!/bin/bash
# health-check.sh

HEALTH_URL="http://localhost:8080/health"
MAX_RETRIES=3
RETRY_DELAY=5

check_health() {
    local response=$(curl -s -w "%{http_code}" "$HEALTH_URL" -o /dev/null)
    echo $response
}

for i in $(seq 1 $MAX_RETRIES); do
    echo "Health check attempt $i/$MAX_RETRIES..."

    status_code=$(check_health)

    if [ "$status_code" -eq 200 ]; then
        echo "Server is healthy (HTTP $status_code)"
        exit 0
    else
        echo "Server health check failed (HTTP $status_code)"

        if [ $i -lt $MAX_RETRIES ]; then
            echo "Retrying in $RETRY_DELAY seconds..."
            sleep $RETRY_DELAY
        fi
    fi
done

echo "Server health check failed after $MAX_RETRIES attempts"
exit 1
```

### 2. Log Rotation

```bash
# /etc/logrotate.d/cpp-server
/var/log/server.log {
    daily
    missingok
    rotate 52
    compress
    delaycompress
    notifempty
    create 644 server server
    postrotate
        systemctl reload cpp-server
    endscript
}
```

## Security Considerations

### 1. Firewall Configuration

```bash
#!/bin/bash
# setup-firewall.sh

# Allow SSH
sudo ufw allow ssh

# Allow HTTP/HTTPS
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp

# Allow custom server port (if different)
sudo ufw allow 8080/tcp

# Enable firewall
sudo ufw --force enable

echo "Firewall configured successfully!"
```

### 2. Security Headers

```cpp
void addSecurityHeaders(HttpResponse& response) {
    response.addHeader("X-Content-Type-Options", "nosniff");
    response.addHeader("X-Frame-Options", "DENY");
    response.addHeader("X-XSS-Protection", "1; mode=block");
    response.addHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
    response.addHeader("Content-Security-Policy", "default-src 'self'");
    response.addHeader("Referrer-Policy", "strict-origin-when-cross-origin");
}
```

## Best Practices

### 1. Configuration Management

- Use environment variables for sensitive data
- Validate configuration on startup
- Support configuration reloading
- Use separate configs for different environments

### 2. Process Management

- Run as non-root user
- Use systemd for service management
- Implement proper signal handling
- Support graceful shutdown

### 3. Security

- Enable SSL/TLS in production
- Use strong cipher suites
- Implement security headers
- Configure firewall rules

### 4. Monitoring

- Implement health check endpoints
- Use structured logging
- Monitor resource usage
- Set up alerting

### 5. Deployment

- Use automated deployment scripts
- Implement rollback procedures
- Test in staging environment
- Use blue-green deployment

## Next Steps

Now that you understand production setup, let's move on to monitoring and observability.

<div class="next-lesson">
    <a href="/course/deployment/monitoring" class="btn btn-primary">Next: Monitoring & Observability →</a>
</div>
