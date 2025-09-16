#include "HttpServer.h"
#include <iostream>
#include <csignal>
#include <sstream>

// Global server pointer for signal handling
HttpServer* g_server = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    if (g_server) {
        std::cout << "\nReceived signal " << signum << ". Shutting down gracefully..." << std::endl;
        g_server->stop();
    }
    exit(signum);
}

// Route handlers
std::string handleRoot(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)body; (void)headers; // Suppress unused parameter warnings
    
    std::ostringstream html;
    html << "<div class=\"hero-section\">"
         << "<h1>🚀 Learn C++ Server Development</h1>"
         << "<p class=\"hero-subtitle\">Build production-ready HTTP servers from scratch using only C++ and system libraries</p>"
         << "<div class=\"hero-buttons\">"
         << "<a href=\"/course/fundamentals/introduction\" class=\"btn btn-primary\">Start Learning</a>"
         << "<a href=\"/api/users\" class=\"btn btn-secondary\">View API</a>"
         << "</div>"
         << "</div>"
         << "<div class=\"features-grid\">"
         << "<div class=\"feature-card\">"
         << "<h3>🎯 Hands-On Learning</h3>"
         << "<p>Build a real HTTP server step by step with interactive examples</p>"
         << "</div>"
         << "<div class=\"feature-card\">"
         << "<h3>⚡ Modern C++</h3>"
         << "<p>Learn C++17/20 features like smart pointers, std::function, and threading</p>"
         << "</div>"
         << "<div class=\"feature-card\">"
         << "<h3>🌐 Network Programming</h3>"
         << "<p>Master socket programming, HTTP protocol, and server architecture</p>"
         << "</div>"
         << "<div class=\"feature-card\">"
         << "<h3>🚀 Production Ready</h3>"
         << "<p>Build scalable, maintainable servers suitable for real-world use</p>"
         << "</div>"
         << "</div>"
         << "<div class=\"course-overview\">"
         << "<h2>Course Modules</h2>"
         << "<div class=\"module-list\">"
         << "<div class=\"module-item\">"
         << "<h3>1. Fundamentals</h3>"
         << "<p>Socket programming, HTTP basics, threading concepts</p>"
         << "<a href=\"/course/fundamentals/introduction\" class=\"module-link\">Start Module →</a>"
         << "</div>"
         << "<div class=\"module-item\">"
         << "<h3>2. Building Blocks</h3>"
         << "<p>Server class design, route handling, request parsing</p>"
         << "<a href=\"/course/building-blocks/server-class\" class=\"module-link\">Start Module →</a>"
         << "</div>"
         << "<div class=\"module-item\">"
         << "<h3>3. Advanced Features</h3>"
         << "<p>Database integration, authentication, error handling</p>"
         << "<a href=\"/course/advanced-features/database-integration\" class=\"module-link\">Start Module →</a>"
         << "</div>"
         << "<div class=\"module-item\">"
         << "<h3>4. Deployment</h3>"
         << "<p>Production setup, monitoring, scaling, security</p>"
         << "<a href=\"/course/deployment/production-setup\" class=\"module-link\">Start Module →</a>"
         << "</div>"
         << "</div>"
         << "</div>";
    
    return html.str();
}

std::string handleHealth(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)body; (void)headers; // Suppress unused parameter warnings
    
    std::ostringstream json;
    json << "{"
         << "\"status\": \"healthy\","
         << "\"timestamp\": " << std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch()).count() << ","
         << "\"uptime\": \"running\","
         << "\"version\": \"1.0.0\","
         << "\"course\": \"C++ Server Development\","
         << "\"modules\": 4,"
         << "\"lessons\": 16"
         << "}";
    
    return json.str();
}

std::string handleUsers(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)body; (void)headers; // Suppress unused parameter warnings
    
    std::ostringstream json;
    json << "{"
         << "\"success\": true,"
         << "\"count\": 3,"
         << "\"users\": ["
         << "{\"id\": \"1\", \"name\": \"John Doe\", \"email\": \"john@example.com\", \"age\": 30},"
         << "{\"id\": \"2\", \"name\": \"Jane Smith\", \"email\": \"jane@example.com\", \"age\": 25},"
         << "{\"id\": \"3\", \"name\": \"Bob Johnson\", \"email\": \"bob@example.com\", \"age\": 35}"
         << "]"
         << "}";
    
    return json.str();
}

std::string handleUserById(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)body; (void)headers; // Suppress unused parameter warnings
    
    std::ostringstream json;
    json << "{"
         << "\"success\": true,"
         << "\"user\": {\"id\": \"1\", \"name\": \"John Doe\", \"email\": \"john@example.com\", \"age\": 30}"
         << "}";
    
    return json.str();
}

// Course route handlers
std::string handleCourseRoute(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)body; (void)headers; // Suppress unused parameter warnings
    
    // This will be handled by the main routing logic
    // The path will be parsed to extract module and lesson
    return "Course content will be generated dynamically";
}

std::string handleStaticFile(const std::string& body, const std::map<std::string, std::string>& headers) {
    (void)body; (void)headers; // Suppress unused parameter warnings
    
    // This will be handled by the main routing logic
    return "Static file content";
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Parse command line arguments
    int port = 8080;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
            if (port < 1 || port > 65535) {
                std::cerr << "Invalid port number. Must be between 1 and 65535." << std::endl;
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    try {
        // Create and configure server
        HttpServer server(port);
        g_server = &server;
        
        // Add basic routes
        server.addRoute("GET", "/", handleRoot);
        server.addRoute("GET", "/health", handleHealth);
        server.addRoute("GET", "/api/users", handleUsers);
        server.addRoute("GET", "/api/users/1", handleUserById);
        server.addRoute("GET", "/api/users/2", handleUserById);
        server.addRoute("GET", "/api/users/3", handleUserById);
        
        std::cout << "C++ HTTP Server v1.0.0 - Course Website" << std::endl;
        std::cout << "=======================================" << std::endl;
        
        // Start the server (this will block until server stops)
        server.start();
        
        // Keep main thread alive
        while (server.isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error starting server: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
