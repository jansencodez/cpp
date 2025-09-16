#include "HttpServer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <filesystem>


HttpServer::HttpServer(int port) : port_(port), serverSocket_(-1), running_(false) {
    // Initialize sample user data
    users_ = {
        {{"id", "1"}, {"name", "John Doe"}, {"email", "john@example.com"}, {"age", "30"}},
        {{"id", "2"}, {"name", "Jane Smith"}, {"email", "jane@example.com"}, {"age", "25"}},
        {{"id", "3"}, {"name", "Bob Johnson"}, {"email", "bob@example.com"}, {"age", "35"}}
    };
    
    // Initialize course structure
    initializeCourseContent();
}

void HttpServer::initializeCourseContent() {
    // Load lessons from markdown files
    if (lessonLoader_.loadAllLessons()) {
        std::cout << "Successfully loaded lessons from markdown files" << std::endl;
        
        // Get modules from lesson loader
        auto modules = lessonLoader_.getAllModules();
        for (const auto& [moduleName, module] : modules) {
            courseModules_[moduleName] = module.lessons;
        }
    } else {
        std::cout << "Failed to load lessons, using fallback content" << std::endl;
        
        // Fallback course modules
        courseModules_ = {
            {"fundamentals", {"introduction", "sockets", "http-basics", "threading"}},
            {"building-blocks", {"server-class", "route-handling", "request-parsing", "response-generation"}},
            {"advanced-features", {"database-integration", "authentication", "error-handling", "performance"}},
            {"deployment", {"production-setup", "monitoring", "scaling", "security"}}
        };
    }
    

}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::initializeSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
#endif

    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return false;
    }

    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return false;
    }

    return true;
}

void HttpServer::cleanupSocket() {
    if (serverSocket_ != -1) {
#ifdef _WIN32
        closesocket(serverSocket_);
        WSACleanup();
#else
        close(serverSocket_);
#endif
        serverSocket_ = -1;
    }
}

void HttpServer::start() {
    if (running_) {
        std::cout << "Server is already running!" << std::endl;
        return;
    }

    if (!initializeSocket()) {
        std::cerr << "Failed to initialize socket" << std::endl;
        return;
    }

    std::cout << "Starting C++ HTTP Server on port " << port_ << std::endl;
    std::cout << "Server will be available at: http://localhost:" << port_ << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;

    running_ = true;
    acceptThread_ = std::thread(&HttpServer::acceptConnections, this);
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        cleanupSocket();
        if (acceptThread_.joinable()) {
            acceptThread_.join();
        }
        std::cout << "Server stopped" << std::endl;
    }
}

void HttpServer::acceptConnections() {
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        // Handle client in a new thread
        std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[4096];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        
        std::string method, path;
        std::map<std::string, std::string> headers;
        std::string body = parseHttpRequest(buffer, method, path, headers);
        
        // Find and execute route handler
        std::string responseBody;
        std::string contentType = "text/plain";
        int statusCode = 200;
        
        // Handle course routes dynamically
        if (path.find("/course/") == 0) {
            std::string coursePath = path.substr(8); // Remove "/course/"
            size_t slashPos = coursePath.find('/');
            if (slashPos != std::string::npos) {
                std::string module = coursePath.substr(0, slashPos);
                std::string lesson = coursePath.substr(slashPos + 1);
                responseBody = generateCoursePage(module, lesson);
                contentType = "text/html";
            } else {
                responseBody = "Invalid course path";
                statusCode = 400;
            }
        }
        // Handle static files
        else if (path.find("/css/") == 0 || path.find("/js/") == 0) {
            responseBody = serveStaticFile(path);
            contentType = getMimeType(path);
            if (responseBody == "File not found") {
                statusCode = 404;
            }
        }
        // Handle regular routes
        else {
            auto methodIt = routes_.find(method);
            if (methodIt != routes_.end()) {
                auto pathIt = methodIt->second.find(path);
                if (pathIt != methodIt->second.end()) {
                    responseBody = pathIt->second(body, headers);
                    // Determine content type based on path
                    if (path == "/") {
                        contentType = "text/html";
                        responseBody = generateHTML("C++ Server Development Course", responseBody);
                    }
                    else if (path == "/health" || path.find("/api/") == 0) contentType = "application/json";
                } else {
                    responseBody = "Not Found";
                    statusCode = 404;
                }
            } else {
                responseBody = "Method Not Allowed";
                statusCode = 405;
            }
        }
        
        // Create proper HTTP response
        std::string response = createHttpResponse(statusCode, contentType, responseBody);
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    
#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

std::string HttpServer::parseHttpRequest(const std::string& request, std::string& method, std::string& path, std::map<std::string, std::string>& headers) {
    std::istringstream stream(request);
    std::string line;
    
    // Parse first line (method, path, version)
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path;
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        if (line.empty() || line == "\r") break;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Remove \r from value
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            
            headers[key] = value;
        }
    }
    
    // Parse body (if any)
    std::string body;
    while (std::getline(stream, line)) {
        body += line + "\n";
    }
    
    return body;
}

std::string HttpServer::createHttpResponse(int statusCode, const std::string& contentType, const std::string& body) {
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        default: statusText = "Internal Server Error"; break;
    }
    
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    return response.str();
}

void HttpServer::addRoute(const std::string& method, const std::string& path, RouteHandler handler) {
    routes_[method][path] = handler;
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            iss >> std::hex >> value;
            result += static_cast<char>(value);
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string HttpServer::getMimeType(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".json") != std::string::npos) return "application/json";
    return "text/plain";
}

std::string HttpServer::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string HttpServer::serveStaticFile(const std::string& path) {
    std::string filePath = "../static" + path;
    if (fileExists(filePath)) {
        return readFileContent(filePath);
    }
    return "File not found";
}

std::string HttpServer::generateHTML(const std::string& title, const std::string& content) {
    std::ostringstream html;
    html << "<!DOCTYPE html>"
         << "<html lang=\"en\">"
         << "<head>"
         << "<meta charset=\"UTF-8\">"
         << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
         << "<title>" << title << "</title>"
         << "<link rel=\"stylesheet\" href=\"/css/style.css\">"
         << "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/themes/prism.min.css\">"
         << "</head>"
         << "<body>"
         << "<nav class=\"navbar\">"
         << "<div class=\"nav-container\">"
         << "<h1 class=\"nav-title\">🚀 C++ Server Development</h1>"
         << "<ul class=\"nav-menu\">"
         << "<li><a href=\"/\">Home</a></li>"
         << "<li><a href=\"/course/fundamentals/introduction\">Course</a></li>"
         << "<li><a href=\"/api/users\">API</a></li>"
         << "<li><a href=\"/health\">Health</a></li>"
         << "</ul>"
         << "</div>"
         << "</nav>"
         << "<main class=\"main-content\">"
         << content
         << "</main>"
         << "<footer class=\"footer\">"
         << "<p>&copy; 2024 C++ Server Development Course. Built with C++!</p>"
         << "</footer>"
         << "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/components/prism-core.min.js\"></script>"
         << "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/plugins/autoloader/prism-autoloader.min.js\"></script>"
         << "<script src=\"/js/app.js\"></script>"
         << "</body>"
         << "</html>";
    
    return html.str();
}

std::string HttpServer::generateCoursePage(const std::string& module, const std::string& lesson) {
    // Try to get content from lesson loader first
    std::string content = lessonLoader_.getLessonContent(module, lesson);
    
    if (content.find("Lesson Not Found") != std::string::npos || content.find("Module Not Found") != std::string::npos) {
        // Fallback to hardcoded content if lesson loader fails
        auto moduleIt = courseModules_.find(module);
        if (moduleIt == courseModules_.end()) {
            return generateHTML("Module Not Found", "<h2>Module Not Found</h2><p>The requested module does not exist.</p>");
        }
        
        auto lessonIt = moduleIt->second.begin();
        auto lessonItEnd = moduleIt->second.end();
        lessonIt = std::find(lessonIt, lessonItEnd, lesson);
        
        if (lessonIt == lessonItEnd) {
            return generateHTML("Lesson Not Found", "<h2>Lesson Not Found</h2><p>The requested lesson does not exist.</p>");
        }
        
        content = "<h2>" + lesson + "</h2><p>Lesson content is not available.</p>";
    }
    
    // Generate navigation
    std::string navigation = lessonLoader_.generateLessonNavigation(module, lesson);
    if (navigation.empty()) {
        // Fallback navigation
        auto moduleIt = courseModules_.find(module);
        if (moduleIt != courseModules_.end()) {
            std::ostringstream nav;
            nav << "<div class=\"course-navigation\">";
            nav << "<h2>Module: " << module << "</h2>";
            nav << "<ul class=\"lesson-list\">";
            
            for (const auto& lessonName : moduleIt->second) {
                std::string activeClass = (lessonName == lesson) ? " class=\"active\"" : "";
                nav << "<li" << activeClass << "><a href=\"/course/" << module << "/" << lessonName << "\">" << lessonName << "</a></li>";
            }
            
            nav << "</ul></div>";
            navigation = nav.str();
        }
    }
    
    // Combine navigation and content
    std::ostringstream fullContent;
    fullContent << navigation << "<div class=\"lesson-content\">" << content << "</div>";
    
    return generateHTML("C++ Server Development - " + module + " - " + lesson, fullContent.str());
}

std::string HttpServer::generateInteractiveCodeEditor(const std::string& language, const std::string& defaultCode) {
    std::ostringstream editor;
    editor << "<div class=\"code-editor\">"
           << "<h3>Interactive Code Editor</h3>"
           << "<textarea id=\"code-input\" class=\"code-input\" rows=\"15\">" << defaultCode << "</textarea>"
           << "<div class=\"editor-controls\">"
           << "<button onclick=\"runCode()\" class=\"btn btn-primary\">Run Code</button>"
           << "<button onclick=\"resetCode()\" class=\"btn btn-secondary\">Reset</button>"
           << "</div>"
           << "<div id=\"output\" class=\"code-output\"></div>"
           << "</div>";
    
    return editor.str();
}

std::string HttpServer::generateProgressTracker(const std::map<std::string, bool>& completedLessons) {
    std::ostringstream tracker;
    tracker << "<div class=\"progress-tracker\">"
            << "<h3>Your Progress</h3>"
            << "<div class=\"progress-bar\">";
    
    int totalLessons = 0;
    int completedCount = 0;
    
    for (const auto& module : courseModules_) {
        totalLessons += module.second.size();
        for (const auto& lesson : module.second) {
            if (completedLessons.count(lesson) && completedLessons.at(lesson)) {
                completedCount++;
            }
        }
    }
    
    int percentage = totalLessons > 0 ? (completedCount * 100) / totalLessons : 0;
    
    tracker << "<div class=\"progress-fill\" style=\"width: " << percentage << "%;\"></div>"
            << "</div>"
            << "<p>" << completedCount << " of " << totalLessons << " lessons completed (" << percentage << "%)</p>"
            << "</div>";
    
    return tracker.str();
}

std::string HttpServer::readFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    return "File not found";
}

bool HttpServer::fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}
