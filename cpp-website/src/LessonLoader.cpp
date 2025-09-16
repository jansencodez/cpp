#include "LessonLoader.h"
#include <filesystem>
#include <algorithm>
#include <regex>

LessonLoader::LessonLoader() {
    // Try multiple possible locations for lessons directory
    std::vector<std::filesystem::path> possiblePaths = {
        std::filesystem::current_path() / "lessons",                    // build/lessons
        std::filesystem::current_path().parent_path() / "lessons",      // lessons/
        std::filesystem::current_path().parent_path().parent_path() / "lessons", // ../lessons
        std::filesystem::path("lessons")                               // relative to current dir
    };
    
    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            lessonsDirectory_ = path.string();
            std::cout << "Found lessons directory: " << lessonsDirectory_ << std::endl;
            return;
        }
    }
    
    // Fallback to parent directory
    lessonsDirectory_ = (std::filesystem::current_path().parent_path() / "lessons").string();
    std::cout << "Using fallback lessons directory: " << lessonsDirectory_ << std::endl;
}

LessonLoader::~LessonLoader() {
}

bool LessonLoader::loadAllLessons() {
    try {
        std::filesystem::path lessonsPath(lessonsDirectory_);
        if (!std::filesystem::exists(lessonsPath)) {
            std::cerr << "Lessons directory not found: " << lessonsDirectory_ << std::endl;
            return false;
        }
        
        // Iterate through module directories
        for (const auto& moduleEntry : std::filesystem::directory_iterator(lessonsPath)) {
            if (moduleEntry.is_directory()) {
                std::string moduleName = moduleEntry.path().filename().string();
                Module module;
                module.name = moduleName;
                
                // Load lessons in this module
                for (const auto& lessonEntry : std::filesystem::directory_iterator(moduleEntry.path())) {
                    if (lessonEntry.path().extension() == ".md") {
                        std::string lessonName = lessonEntry.path().stem().string();
                        module.lessons.push_back(lessonName);
                        
                        // Load the lesson content
                        Lesson lesson;
                        if (parseMarkdownFile(lessonEntry.path().string(), lesson)) {
                            lesson.htmlContent = markdownToHtml(lesson.content);
                            module.lessonData[lessonName] = lesson;
                        }
                    }
                }
                
                // Sort lessons in logical order
                sortLessonsInModule(module);
                
                modules_[moduleName] = module;
            }
        }
        
        // Sort modules in logical order (Fundamentals first)
        sortModulesInOrder();
        
        std::cout << "Loaded " << modules_.size() << " modules with lessons" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading lessons: " << e.what() << std::endl;
        return false;
    }
}

bool LessonLoader::loadLesson(const std::string& module, const std::string& lessonName) {
    auto moduleIt = modules_.find(module);
    if (moduleIt == modules_.end()) {
        return false;
    }
    
    auto lessonIt = moduleIt->second.lessonData.find(lessonName);
    if (lessonIt == moduleIt->second.lessonData.end()) {
        return false;
    }
    
    return true;
}

std::string LessonLoader::getLessonContent(const std::string& module, const std::string& lessonName) {
    auto moduleIt = modules_.find(module);
    if (moduleIt == modules_.end()) {
        return "<h2>Module Not Found</h2><p>This module is not available.</p>";
    }
    
    auto lessonIt = moduleIt->second.lessonData.find(lessonName);
    if (lessonIt == moduleIt->second.lessonData.end()) {
        return "<h2>Lesson Not Found</h2><p>This lesson is not available.</p>";
    }
    
    return lessonIt->second.htmlContent;
}

LessonLoader::Module LessonLoader::getModule(const std::string& moduleName) {
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        return it->second;
    }
    return Module{};
}

std::map<std::string, LessonLoader::Module> LessonLoader::getAllModules() {
    return modules_;
}

std::string LessonLoader::markdownToHtml(const std::string& markdown) {
    std::string html = markdown;
    
    // Process different markdown elements
    html = processHeaders(html);
    html = processCodeBlocks(html);
    html = processBoldText(html);
    html = processLists(html);
    html = processLinks(html);
    html = processTables(html);
    
    return html;
}

bool LessonLoader::parseMarkdownFile(const std::string& filePath, Lesson& lesson) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    lesson.content = buffer.str();
    
    lesson.title = extractTitle(lesson.content);
    lesson.tags = extractTags(lesson.content);
    
    return true;
}

std::string LessonLoader::extractTitle(const std::string& markdown) {
    std::regex titleRegex(R"(^#\s+(.+)$)", std::regex::multiline);
    std::smatch match;
    if (std::regex_search(markdown, match, titleRegex)) {
        return match[1].str();
    }
    return "Untitled Lesson";
}

std::vector<std::string> LessonLoader::extractTags(const std::string& markdown) {
    std::vector<std::string> tags;
    std::regex tagRegex(R"(#(\w+))");
    
    auto words_begin = std::sregex_iterator(markdown.begin(), markdown.end(), tagRegex);
    auto words_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        tags.push_back(match[1].str());
    }
    
    return tags;
}

std::string LessonLoader::processCodeBlocks(const std::string& markdown) {
    std::string html = markdown;
    
    // Manual parsing for code blocks (more reliable than regex)
    size_t pos = 0;
    while ((pos = html.find("```", pos)) != std::string::npos) {
        size_t start = pos;
        size_t end = html.find("```", start + 3);
        
        if (end == std::string::npos) break;
        
        // Find the language specifier
        size_t langStart = start + 3;
        size_t newlinePos = html.find('\n', langStart);
        
        std::string language = "";
        std::string codeContent;
        
        if (newlinePos < end) {
            language = html.substr(langStart, newlinePos - langStart);
            // Trim whitespace
            language.erase(0, language.find_first_not_of(" \t"));
            language.erase(language.find_last_not_of(" \t") + 1);
            
            codeContent = html.substr(newlinePos + 1, end - newlinePos - 1);
        } else {
            codeContent = html.substr(langStart, end - langStart);
        }
        
        // Trim leading/trailing whitespace from code content
        codeContent.erase(0, codeContent.find_first_not_of(" \t\r\n"));
        codeContent.erase(codeContent.find_last_not_of(" \t\r\n") + 1);
        
        // Create HTML for code block
        std::string codeHtml;
        if (language == "cpp" || language == "c++") {
            codeHtml = "<div class=\"code-example\"><pre><code class=\"language-cpp\">" + codeContent + "</code></pre></div>";
        } else {
            codeHtml = "<div class=\"code-example\"><pre><code>" + codeContent + "</code></pre></div>";
        }
        
        // Replace the markdown code block with HTML
        html.replace(start, end - start + 3, codeHtml);
        
        // Update position for next iteration
        pos = start + codeHtml.length();
    }
    
    // Process inline code (but not inside code blocks)
    pos = 0;
    while ((pos = html.find("`", pos)) != std::string::npos) {
        // Skip if we're inside a code block
        if (pos > 0 && html.substr(pos - 1, 6) == "</code>") {
            pos += 1;
            continue;
        }
        
        size_t end = html.find("`", pos + 1);
        if (end == std::string::npos) break;
        
        std::string code = html.substr(pos + 1, end - pos - 1);
        std::string codeHtml = "<code>" + code + "</code>";
        
        html.replace(pos, end - pos + 1, codeHtml);
        pos += codeHtml.length();
    }
    
    return html;
}

std::string LessonLoader::processBoldText(const std::string& markdown) {
    std::string html = markdown;
    
    // Process **text** bold formatting
    size_t pos = 0;
    while ((pos = html.find("**", pos)) != std::string::npos) {
        size_t start = pos;
        size_t end = html.find("**", start + 2);
        
        if (end == std::string::npos) break;
        
        std::string boldText = html.substr(start + 2, end - start - 2);
        html.replace(start, end - start + 2, "<strong>" + boldText + "</strong>");
        
        // Move position past the replaced text
        pos = start + 8 + boldText.length(); // Length of <strong></strong>
    }
    
    return html;
}

std::string LessonLoader::processHeaders(const std::string& markdown) {
    std::string html = markdown;
    
    // Process # headers
    std::regex h1Regex(R"(^#\s+(.+)$)", std::regex::multiline);
    html = std::regex_replace(html, h1Regex, "<h1>$1</h1>");
    
    // Process ## headers
    std::regex h2Regex(R"(^##\s+(.+)$)", std::regex::multiline);
    html = std::regex_replace(html, h2Regex, "<h2>$1</h2>");
    
    // Process ### headers
    std::regex h3Regex(R"(^###\s+(.+)$)", std::regex::multiline);
    html = std::regex_replace(html, h3Regex, "<h3>$1</h3>");
    
    // Process #### headers
    std::regex h4Regex(R"(^####\s+(.+)$)", std::regex::multiline);
    html = std::regex_replace(html, h4Regex, "<h4>$1</h4>");
    
    return html;
}

std::string LessonLoader::processLists(const std::string& markdown) {
    std::string html = markdown;
    
    // Process unordered lists
    std::regex ulRegex(R"(^-\s+(.+)$)", std::regex::multiline);
    html = std::regex_replace(html, ulRegex, "<li>$1</li>");
    
    // Wrap consecutive <li> elements in <ul>
    std::regex ulWrapRegex(R"((<li>.*?</li>\s*)+)");
    html = std::regex_replace(html, ulWrapRegex, "<ul>$&</ul>");
    
    // Process ordered lists
    std::regex olRegex(R"(^\d+\.\s+(.+)$)", std::regex::multiline);
    html = std::regex_replace(html, olRegex, "<li>$1</li>");
    
    return html;
}

std::string LessonLoader::processLinks(const std::string& markdown) {
    std::string html = markdown;
    
    // Process [text](url) links
    std::regex linkRegex(R"(\[([^\]]+)\]\(([^)]+)\))");
    html = std::regex_replace(html, linkRegex, "<a href=\"$2\">$1</a>");
    
    return html;
}

std::string LessonLoader::processTables(const std::string& markdown) {
    std::string html = markdown;
    
    // Find table blocks (lines starting and ending with |)
    std::istringstream stream(markdown);
    std::string line;
    std::vector<std::string> lines;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    std::ostringstream result;
    bool inTable = false;
    std::vector<std::string> tableLines;
    
    for (size_t i = 0; i < lines.size(); i++) {
        const std::string& currentLine = lines[i];
        
        // Check if this line is a table row (starts and ends with |)
        bool isTableRow = !currentLine.empty() && 
                         currentLine.front() == '|' && 
                         currentLine.back() == '|';
        
        if (isTableRow) {
            if (!inTable) {
                // Start of table
                inTable = true;
                result << "<div class=\"table-container\"><table class=\"course-table\">";
            }
            
            // Process table row
            std::string processedRow = processTableRow(currentLine);
            result << processedRow;
            
            // Check if next line is separator (contains only |, -, and spaces)
            if (i + 1 < lines.size()) {
                const std::string& nextLine = lines[i + 1];
                bool isSeparator = std::all_of(nextLine.begin(), nextLine.end(), 
                    [](char c) { return c == '|' || c == '-' || c == ' '; });
                
                if (isSeparator) {
                    // Skip the separator line
                    i++;
                    continue;
                }
            }
        } else {
            if (inTable) {
                // End of table
                inTable = false;
                result << "</table></div>";
            }
            result << currentLine << "\n";
        }
    }
    
    // Close table if still open
    if (inTable) {
        result << "</table></div>";
    }
    
    return result.str();
}

std::string LessonLoader::processTableRow(const std::string& row) {
    std::ostringstream result;
    result << "<tr>";
    
    // Split by | and process each cell
    std::istringstream cellStream(row);
    std::string cell;
    bool first = true;
    
    while (std::getline(cellStream, cell, '|')) {
        if (first) {
            first = false;
            continue; // Skip first empty cell
        }
        
        // Trim whitespace
        cell.erase(0, cell.find_first_not_of(" \t"));
        cell.erase(cell.find_last_not_of(" \t") + 1);
        
        // Check if cell contains bold text (**text**)
        if (cell.find("**") != std::string::npos) {
            // Replace **text** with <strong>text</strong>
            size_t start = 0;
            while ((start = cell.find("**", start)) != std::string::npos) {
                size_t end = cell.find("**", start + 2);
                if (end != std::string::npos) {
                    std::string boldText = cell.substr(start + 2, end - start - 2);
                    cell.replace(start, end - start + 2, "<strong>" + boldText + "</strong>");
                    start += 8 + boldText.length(); // Length of <strong></strong>
                } else {
                    break;
                }
            }
        }
        
        result << "<td>" << cell << "</td>";
    }
    
    result << "</tr>";
    return result.str();
}

std::string LessonLoader::escapeHtml(const std::string& text) {
    std::string html = text;
    
    // Replace special characters
    std::map<std::string, std::string> replacements = {
        {"&", "&amp;"},
        {"<", "&lt;"},
        {">", "&gt;"},
        {"\"", "&quot;"},
        {"'", "&#39;"}
    };
    
    for (const auto& [from, to] : replacements) {
        size_t pos = 0;
        while ((pos = html.find(from, pos)) != std::string::npos) {
            html.replace(pos, from.length(), to);
            pos += to.length();
        }
    }
    
    return html;
}

std::string LessonLoader::generateLessonNavigation(const std::string& module, const std::string& currentLesson) {
    std::ostringstream nav;
    
    // Main course navigation header
    nav << "<div class=\"course-navigation\">";
    nav << "<div class=\"nav-header\">";
    nav << "<h2>🚀 C++ Server Development Course</h2>";
    nav << "<div class=\"breadcrumb\">";
    nav << "<a href=\"/\">Home</a> → ";
    nav << "<a href=\"/#course-overview\">Course</a> → ";
    nav << "<span class=\"current-module\">" << module << "</span>";
    nav << "</div>";
    nav << "</div>";
    
    // Module tabs for easy navigation between modules
    nav << "<div class=\"module-tabs\">";
    nav << "<ul>";
    
    // Generate all module tabs in correct order
    for (const auto& moduleName : moduleOrder_) {
        auto moduleIt = modules_.find(moduleName);
        if (moduleIt != modules_.end()) {
            std::string activeClass = (moduleName == module) ? " class=\"active\"" : "";
            std::string moduleTitle = getModuleTitle(moduleName);
            
            // Get the first lesson of this module (or a default if empty)
            std::string firstLesson = moduleIt->second.lessons.empty() ? "introduction" : moduleIt->second.lessons.front();
            
            nav << "<li" << activeClass << "><a href=\"/course/" << moduleName << "/" << firstLesson << "\">" << moduleTitle << "</a></li>";
        }
    }
    
    nav << "</ul></div>";
    
    // Module navigation with lessons
    nav << "<div class=\"module-navigation\">";
    nav << "<h3>Module: " << getModuleTitle(module) << "</h3>";
    nav << "<ul class=\"lesson-list\">";
    
    auto moduleIt = modules_.find(module);
    if (moduleIt != modules_.end()) {
        for (const auto& lessonName : moduleIt->second.lessons) {
            std::string activeClass = (lessonName == currentLesson) ? " class=\"active\"" : "";
            std::string lessonTitle = getLessonTitle(lessonName);
            nav << "<li" << activeClass << "><a href=\"/course/" << module << "/" << lessonName << "\">" << lessonTitle << "</a></li>";
        }
    }
    
    nav << "</ul></div>";
    
    // Previous/Next navigation
    nav << "<div class=\"lesson-navigation\">";
    auto prevNext = getPreviousNextLesson(module, currentLesson);
    if (!prevNext.first.empty()) {
        nav << "<a href=\"/course/" << module << "/" << prevNext.first << "\" class=\"nav-btn prev-btn\">← Previous</a>";
    }
    if (!prevNext.second.empty()) {
        nav << "<a href=\"/course/" << module << "/" << prevNext.second << "\" class=\"nav-btn next-btn\">Next →</a>";
    }
    nav << "</div>";
    
    nav << "</div>";
    return nav.str();
}

std::string LessonLoader::getLessonTitle(const std::string& lessonName) {
    // Convert lesson names to readable titles
    std::map<std::string, std::string> titleMap = {
        {"introduction", "Introduction"},
        {"sockets", "Socket Programming"},
        {"http-basics", "HTTP Protocol Basics"},
        {"threading", "Multi-threading & Concurrency"},
        {"server-class", "Server Class Architecture"},
        {"route-handling", "Route Handling & Middleware"},
        {"request-parsing", "Request Parsing & Validation"},
        {"response-generation", "Response Generation & Headers"},
        {"database-integration", "Database Integration"},
        {"authentication", "Authentication & Security"},
        {"error-handling", "Error Handling & Logging"},
        {"performance", "Performance Optimization"},
        {"production-setup", "Production Setup"},
        {"monitoring", "Monitoring & Observability"},
        {"scaling", "Scaling & Load Balancing"},
        {"security", "Security Best Practices"}
    };
    
    auto it = titleMap.find(lessonName);
    return (it != titleMap.end()) ? it->second : lessonName;
}

std::string LessonLoader::getModuleTitle(const std::string& moduleName) {
    // Convert module names to readable titles
    std::map<std::string, std::string> titleMap = {
        {"fundamentals", "Fundamentals"},
        {"building-blocks", "Building Blocks"},
        {"advanced-features", "Advanced Features"},
        {"deployment", "Deployment & Production"}
    };
    
    auto it = titleMap.find(moduleName);
    return (it != titleMap.end()) ? it->second : moduleName;
}

std::pair<std::string, std::string> LessonLoader::getPreviousNextLesson(const std::string& module, const std::string& currentLesson) {
    auto moduleIt = modules_.find(module);
    if (moduleIt == modules_.end()) {
        return {"", ""};
    }
    
    const auto& lessons = moduleIt->second.lessons;
    auto currentIt = std::find(lessons.begin(), lessons.end(), currentLesson);
    
    if (currentIt == lessons.end()) {
        return {"", ""};
    }
    
    std::string prev = "";
    std::string next = "";
    
    if (currentIt != lessons.begin()) {
        prev = *(currentIt - 1);
    }
    
    if (currentIt + 1 != lessons.end()) {
        next = *(currentIt + 1);
    }
    
    return {prev, next};
}

void LessonLoader::sortLessonsInModule(Module& module) {
    // Define the logical order for each module
    std::map<std::string, std::vector<std::string>> lessonOrder = {
        {"fundamentals", {
            "introduction",
            "sockets", 
            "http-basics",
            "threading"
        }},
        {"building-blocks", {
            "server-class",
            "route-handling",
            "request-parsing",
            "response-generation"
        }},
        {"advanced-features", {
            "database-integration",
            "authentication",
            "error-handling",
            "performance"
        }},
        {"deployment", {
            "production-setup",
            "monitoring",
            "scaling",
            "security"
        }}
    };
    
    auto orderIt = lessonOrder.find(module.name);
    if (orderIt != lessonOrder.end()) {
        const auto& desiredOrder = orderIt->second;
        
        // Create a new sorted lessons vector
        std::vector<std::string> sortedLessons;
        std::map<std::string, Lesson> sortedLessonData;
        
        // Add lessons in the desired order
        for (const auto& lessonName : desiredOrder) {
            auto lessonIt = std::find(module.lessons.begin(), module.lessons.end(), lessonName);
            if (lessonIt != module.lessons.end()) {
                sortedLessons.push_back(lessonName);
                sortedLessonData[lessonName] = module.lessonData[lessonName];
            }
        }
        
        // Add any remaining lessons that weren't in the order list
        for (const auto& lessonName : module.lessons) {
            if (std::find(sortedLessons.begin(), sortedLessons.end(), lessonName) == sortedLessons.end()) {
                sortedLessons.push_back(lessonName);
                sortedLessonData[lessonName] = module.lessonData[lessonName];
            }
        }
        
        // Update the module with sorted data
        module.lessons = sortedLessons;
        module.lessonData = sortedLessonData;
    }
}

void LessonLoader::sortModulesInOrder() {
    // Define the logical order for modules
    std::vector<std::string> desiredOrder = {
        "fundamentals",
        "building-blocks", 
        "advanced-features",
        "deployment"
    };
    
    // Clear and rebuild the module order vector
    moduleOrder_.clear();
    
    // Add modules in the desired order (only if they exist)
    for (const auto& moduleName : desiredOrder) {
        if (modules_.find(moduleName) != modules_.end()) {
            moduleOrder_.push_back(moduleName);
        }
    }
    
    // Add any remaining modules that weren't in the order list
    for (const auto& [moduleName, moduleData] : modules_) {
        if (std::find(moduleOrder_.begin(), moduleOrder_.end(), moduleName) == moduleOrder_.end()) {
            moduleOrder_.push_back(moduleName);
        }
    }
    
    // Debug output to verify order
    std::cout << "Modules in order: ";
    for (const auto& moduleName : moduleOrder_) {
        std::cout << moduleName << " ";
    }
    std::cout << std::endl;
}
