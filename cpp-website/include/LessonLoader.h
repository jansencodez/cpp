#pragma once

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

class LessonLoader {
public:
    struct Lesson {
        std::string title;
        std::string content;
        std::string htmlContent;
        std::vector<std::string> tags;
    };
    
    struct Module {
        std::string name;
        std::string description;
        std::vector<std::string> lessons;
        std::map<std::string, Lesson> lessonData;
    };
    
    LessonLoader();
    ~LessonLoader();
    
    // Load all lessons from the lessons directory
    bool loadAllLessons();
    
    // Load a specific lesson
    bool loadLesson(const std::string& module, const std::string& lessonName);
    
    // Get lesson content
    std::string getLessonContent(const std::string& module, const std::string& lessonName);
    
    // Get module information
    Module getModule(const std::string& moduleName);
    
    // Get all modules
    std::map<std::string, Module> getAllModules();
    
    // Convert markdown to HTML
    std::string markdownToHtml(const std::string& markdown);
    
    // Get lesson navigation
    std::string generateLessonNavigation(const std::string& module, const std::string& currentLesson);
    
private:
    // Helper methods for navigation
    std::string getLessonTitle(const std::string& lessonName);
    std::string getModuleTitle(const std::string& moduleName);
    std::pair<std::string, std::string> getPreviousNextLesson(const std::string& module, const std::string& currentLesson);
    std::map<std::string, Module> modules_;
    std::vector<std::string> moduleOrder_;
    std::string lessonsDirectory_;
    
    // Helper methods
    bool parseMarkdownFile(const std::string& filePath, Lesson& lesson);
    std::string extractTitle(const std::string& markdown);
    std::vector<std::string> extractTags(const std::string& markdown);
    std::string processCodeBlocks(const std::string& markdown);
    std::string processBoldText(const std::string& markdown);
    std::string processHeaders(const std::string& markdown);
    std::string processLists(const std::string& markdown);
    std::string processLinks(const std::string& markdown);
    std::string processTables(const std::string& markdown);
    std::string processTableRow(const std::string& row);
    std::string escapeHtml(const std::string& text);
    
    // Helper methods for lesson organization
    void sortLessonsInModule(Module& module);
    void sortModulesInOrder();
};
