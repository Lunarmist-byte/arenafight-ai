#include "arenafight/tools/file_tools.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace arenafight {

namespace {

std::string resolvePath(const std::string& path, const std::string& workingDir) {
    std::filesystem::path p(path);
    if (p.is_absolute()) return p.string();
    return (std::filesystem::path(workingDir) / p).lexically_normal().string();
}

} // namespace

// --- ReadFileTool ---
nlohmann::json ReadFileTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Path to the file to read"}}},
            {"start_line", {{"type", "integer"}, {"description", "Optional 1-based start line number"}}},
            {"end_line", {{"type", "integer"}, {"description", "Optional 1-based end line number"}}}
        }},
        {"required", {"path"}}
    };
}

ToolResult ReadFileTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string relPath = arguments.value("path", "");
    if (relPath.empty()) {
        res.success = false;
        res.error = "Missing 'path' argument";
        return res;
    }

    std::string fullPath = resolvePath(relPath, workingDir);
    if (!std::filesystem::exists(fullPath)) {
        res.success = false;
        res.error = "File does not exist: " + relPath;
        return res;
    }

    int startLine = arguments.value("start_line", 1);
    int endLine = arguments.value("end_line", -1);

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        res.success = false;
        res.error = "Failed to open file for reading: " + relPath;
        return res;
    }

    std::stringstream ss;
    std::string line;
    int currentLine = 1;
    int linesRead = 0;

    while (std::getline(file, line)) {
        if (currentLine >= startLine && (endLine == -1 || currentLine <= endLine)) {
            ss << currentLine << ": " << line << "\n";
            linesRead++;
        }
        currentLine++;
    }

    res.success = true;
    res.output = ss.str();
    if (linesRead == 0) {
        res.output = "(Empty file or specified line range is empty)";
    }
    return res;
}

// --- WriteFileTool ---
nlohmann::json WriteFileTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Target file path"}}},
            {"content", {{"type", "string"}, {"description", "Content to write into the file"}}},
            {"overwrite", {{"type", "boolean"}, {"description", "Whether to overwrite if file exists (default: true)"}}}
        }},
        {"required", {"path", "content"}}
    };
}

ToolResult WriteFileTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string relPath = arguments.value("path", "");
    std::string content = arguments.value("content", "");
    bool overwrite = arguments.value("overwrite", true);

    if (relPath.empty()) {
        res.success = false;
        res.error = "Missing 'path' argument";
        return res;
    }

    std::string fullPath = resolvePath(relPath, workingDir);
    std::filesystem::path p(fullPath);

    if (std::filesystem::exists(fullPath) && !overwrite) {
        res.success = false;
        res.error = "File already exists and overwrite is set to false: " + relPath;
        return res;
    }

    try {
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream file(fullPath, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) {
            res.success = false;
            res.error = "Failed to open file for writing: " + relPath;
            return res;
        }

        file.write(content.data(), content.size());
        file.close();

        res.success = true;
        res.output = "File successfully written (" + std::to_string(content.size()) + " bytes): " + relPath;
    } catch (const std::exception& e) {
        res.success = false;
        res.error = "Filesystem exception: " + std::string(e.what());
    }

    return res;
}

// --- EditFileTool ---
nlohmann::json EditFileTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Target file path to edit"}}},
            {"target_content", {{"type", "string"}, {"description", "Exact text in the file to be replaced"}}},
            {"replacement_content", {{"type", "string"}, {"description", "New replacement text"}}}
        }},
        {"required", {"path", "target_content", "replacement_content"}}
    };
}

ToolResult EditFileTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string relPath = arguments.value("path", "");
    std::string targetContent = arguments.value("target_content", "");
    std::string replacementContent = arguments.value("replacement_content", "");

    if (relPath.empty()) {
        res.success = false;
        res.error = "Missing 'path' argument";
        return res;
    }
    if (targetContent.empty()) {
        res.success = false;
        res.error = "Missing 'target_content' argument";
        return res;
    }

    std::string fullPath = resolvePath(relPath, workingDir);
    if (!std::filesystem::exists(fullPath)) {
        res.success = false;
        res.error = "File does not exist: " + relPath;
        return res;
    }

    // Read full file
    std::ifstream inFile(fullPath, std::ios::binary);
    if (!inFile.is_open()) {
        res.success = false;
        res.error = "Could not open file for editing: " + relPath;
        return res;
    }

    std::string fileContent((std::istreambuf_iterator<char>(inFile)),
                             std::istreambuf_iterator<char>());
    inFile.close();

    size_t pos = fileContent.find(targetContent);
    if (pos == std::string::npos) {
        res.success = false;
        res.error = "Target content not found in file. Patch validation failed for: " + relPath;
        return res;
    }

    // Check if target appears multiple times
    size_t nextPos = fileContent.find(targetContent, pos + targetContent.length());
    if (nextPos != std::string::npos) {
        res.success = false;
        res.error = "Target content found multiple times in file. Please provide more surrounding context to make target unique.";
        return res;
    }

    // Create backup
    try {
        std::filesystem::copy_file(fullPath, fullPath + ".bak", std::filesystem::copy_options::overwrite_existing);
    } catch (...) {}

    // Replace
    fileContent.replace(pos, targetContent.length(), replacementContent);

    std::ofstream outFile(fullPath, std::ios::trunc | std::ios::binary);
    if (!outFile.is_open()) {
        res.success = false;
        res.error = "Could not write patched content to file: " + relPath;
        return res;
    }
    outFile.write(fileContent.data(), fileContent.size());
    outFile.close();

    res.success = true;
    res.output = "Targeted edit applied successfully to: " + relPath + " (Backup created at " + relPath + ".bak)";
    return res;
}

// --- CreateDirectoryTool ---
nlohmann::json CreateDirectoryTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Directory path to create recursively"}}}
        }},
        {"required", {"path"}}
    };
}

ToolResult CreateDirectoryTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string relPath = arguments.value("path", "");
    if (relPath.empty()) {
        res.success = false;
        res.error = "Missing 'path' argument";
        return res;
    }

    std::string fullPath = resolvePath(relPath, workingDir);
    try {
        std::filesystem::create_directories(fullPath);
        res.success = true;
        res.output = "Directory created: " + relPath;
    } catch (const std::exception& e) {
        res.success = false;
        res.error = "Failed to create directory: " + std::string(e.what());
    }
    return res;
}

// --- DeleteFileTool ---
nlohmann::json DeleteFileTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Path of the file to delete"}}}
        }},
        {"required", {"path"}}
    };
}

ToolResult DeleteFileTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string relPath = arguments.value("path", "");
    if (relPath.empty()) {
        res.success = false;
        res.error = "Missing 'path' argument";
        return res;
    }

    std::string fullPath = resolvePath(relPath, workingDir);
    if (!std::filesystem::exists(fullPath)) {
        res.success = false;
        res.error = "File does not exist: " + relPath;
        return res;
    }

    try {
        if (std::filesystem::is_directory(fullPath)) {
            res.success = false;
            res.error = "Target is a directory, not a file. Use explicit shell command if directory deletion is necessary.";
            return res;
        }

        std::filesystem::remove(fullPath);
        res.success = true;
        res.output = "File deleted: " + relPath;
    } catch (const std::exception& e) {
        res.success = false;
        res.error = "Failed to delete file: " + std::string(e.what());
    }

    return res;
}

} // namespace arenafight
