#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include "LessonLoader.h"

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

class HttpServer {
public:
    using RouteHandler = std::function<std::string(const std::string&, const std::map<std::string, std::string>&)>;
    
    HttpServer(int port = 8080);
    ~HttpServer();
    
    // Start the server
    void start();
    
    // Stop the server
    void stop();
    
    // Add routes
    void addRoute(const std::string& method, const std::string& path, RouteHandler handler);
    
    // Check if server is running
    bool isRunning() const { return running_; }
    
    // Utility functions
    std::string getCurrentTime();
    
    // Static file serving
    std::string serveStaticFile(const std::string& path);
    std::string generateHTML(const std::string& title, const std::string& content);
    
    // Course-specific methods
    std::string generateCoursePage(const std::string& module, const std::string& lesson);
    std::string generateInteractiveCodeEditor(const std::string& language, const std::string& defaultCode);
    std::string generateProgressTracker(const std::map<std::string, bool>& completedLessons);
    
private:
    // Course initialization
    void initializeCourseContent();

private:
    // Server socket operations
    bool initializeSocket();
    void cleanupSocket();
    void acceptConnections();
    void handleClient(int clientSocket);
    
    // HTTP parsing
    std::string parseHttpRequest(const std::string& request, std::string& method, std::string& path, std::map<std::string, std::string>& headers);
    std::string createHttpResponse(int statusCode, const std::string& contentType, const std::string& body);
    
    // Helper methods
    std::string urlDecode(const std::string& str);
    std::string getMimeType(const std::string& path);
    std::string readFileContent(const std::string& filePath);
    bool fileExists(const std::string& filePath);

private:
    int port_;
    int serverSocket_;
    std::atomic<bool> running_;
    std::thread acceptThread_;
    
    // Route handlers
    std::map<std::string, std::map<std::string, RouteHandler>> routes_;
    
    // Sample data
    std::vector<std::map<std::string, std::string>> users_;
    
    // Course content structure
    std::map<std::string, std::vector<std::string>> courseModules_;
    std::map<std::string, std::map<std::string, std::string>> lessonContent_;
    
    // Lesson loader
    LessonLoader lessonLoader_;
};
