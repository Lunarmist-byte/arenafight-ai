#include "arenafight/tools/search_tools.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iostream>

namespace arenafight {

namespace {

std::string resolvePath(const std::string& path, const std::string& workingDir) {
    std::filesystem::path p(path);
    if (p.is_absolute()) return p.string();
    return (std::filesystem::path(workingDir) / p).lexically_normal().string();
}

bool shouldSkipDir(const std::string& name) {
    return name == ".git" || name == ".svn" || name == "node_modules" ||
           name == ".gemini" || name == "build" || name == "venv" || name == "__pycache__";
}

bool isBinaryFile(const std::filesystem::path& p) {
    static const std::vector<std::string> binExts = {
        ".exe", ".dll", ".so", ".a", ".lib", ".o", ".obj",
        ".png", ".jpg", ".jpeg", ".gif", ".webp", ".ico",
        ".pdf", ".zip", ".tar", ".gz", ".7z", ".bin"
    };
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (const auto& be : binExts) {
        if (ext == be) return true;
    }
    return false;
}

void listDirRecursive(
    const std::filesystem::path& current,
    int currentDepth,
    int maxDepth,
    std::stringstream& ss,
    int& totalCount
) {
    if (currentDepth > maxDepth || totalCount >= 500) return;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(current)) {
            std::string filename = entry.path().filename().string();
            if (shouldSkipDir(filename)) continue;

            std::string indent(currentDepth * 2, ' ');
            if (entry.is_directory()) {
                ss << indent << "[DIR]  " << filename << "/\n";
                totalCount++;
                listDirRecursive(entry.path(), currentDepth + 1, maxDepth, ss, totalCount);
            } else {
                uintmax_t size = 0;
                try { size = entry.file_size(); } catch (...) {}
                ss << indent << "       " << filename << " (" << size << " bytes)\n";
                totalCount++;
            }
            if (totalCount >= 500) {
                ss << "... [output capped at 500 items] ...\n";
                break;
            }
        }
    } catch (...) {}
}

} // namespace

// --- ListDirectoryTool ---
nlohmann::json ListDirectoryTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Directory path (default: current directory)"}}},
            {"max_depth", {{"type", "integer"}, {"description", "Maximum depth to recurse (default: 2)"}}}
        }}
    };
}

ToolResult ListDirectoryTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string relPath = arguments.value("path", ".");
    int maxDepth = arguments.value("max_depth", 2);

    std::string fullPath = resolvePath(relPath, workingDir);
    if (!std::filesystem::exists(fullPath)) {
        res.success = false;
        res.error = "Directory does not exist: " + relPath;
        return res;
    }

    if (!std::filesystem::is_directory(fullPath)) {
        res.success = false;
        res.error = "Path is not a directory: " + relPath;
        return res;
    }

    std::stringstream ss;
    ss << "Directory contents for: " << relPath << " (max depth: " << maxDepth << ")\n";
    int totalCount = 0;
    listDirRecursive(fullPath, 0, maxDepth, ss, totalCount);

    res.success = true;
    res.output = ss.str();
    return res;
}

// --- SearchFilesTool ---
nlohmann::json SearchFilesTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"pattern", {{"type", "string"}, {"description", "Text or regular expression to search for"}}},
            {"path", {{"type", "string"}, {"description", "Directory to search in (default: '.')"}}},
            {"regex", {{"type", "boolean"}, {"description", "Treat pattern as regular expression (default: false)"}}},
            {"max_results", {{"type", "integer"}, {"description", "Maximum number of matching lines to return (default: 50)"}}}
        }},
        {"required", {"pattern"}}
    };
}

ToolResult SearchFilesTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string pattern = arguments.value("pattern", "");
    std::string relPath = arguments.value("path", ".");
    bool isRegex = arguments.value("regex", false);
    int maxResults = arguments.value("max_results", 50);

    if (pattern.empty()) {
        res.success = false;
        res.error = "Missing 'pattern' argument";
        return res;
    }

    std::string fullPath = resolvePath(relPath, workingDir);
    if (!std::filesystem::exists(fullPath)) {
        res.success = false;
        res.error = "Path does not exist: " + relPath;
        return res;
    }

    std::regex reg;
    if (isRegex) {
        try {
            reg = std::regex(pattern, std::regex::icase);
        } catch (const std::exception& e) {
            res.success = false;
            res.error = "Invalid regular expression: " + std::string(e.what());
            return res;
        }
    }

    std::stringstream ss;
    int matchesFound = 0;

    auto searchFile = [&](const std::filesystem::path& filePath) {
        if (isBinaryFile(filePath)) return;
        std::ifstream file(filePath);
        if (!file.is_open()) return;

        std::string relToFile = std::filesystem::relative(filePath, workingDir).string();
        std::string line;
        int lineNum = 1;

        while (std::getline(file, line) && matchesFound < maxResults) {
            bool matched = false;
            if (isRegex) {
                matched = std::regex_search(line, reg);
            } else {
                matched = (line.find(pattern) != std::string::npos);
            }

            if (matched) {
                matchesFound++;
                ss << relToFile << ":" << lineNum << ": " << line << "\n";
            }
            lineNum++;
        }
    };

    if (std::filesystem::is_regular_file(fullPath)) {
        searchFile(fullPath);
    } else {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(fullPath)) {
                if (entry.is_directory()) {
                    if (shouldSkipDir(entry.path().filename().string())) {
                        continue;
                    }
                } else if (entry.is_regular_file()) {
                    searchFile(entry.path());
                }
                if (matchesFound >= maxResults) break;
            }
        } catch (...) {}
    }

    res.success = true;
    if (matchesFound == 0) {
        res.output = "No matches found for pattern: " + pattern;
    } else {
        res.output = ss.str();
        if (matchesFound >= maxResults) {
            res.output += "... [capped at " + std::to_string(maxResults) + " matches] ...\n";
        }
    }
    return res;
}

} // namespace arenafight
