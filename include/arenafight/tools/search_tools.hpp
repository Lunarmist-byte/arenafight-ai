#pragma once

#include "arenafight/tools/tool.hpp"

namespace arenafight {

class ListDirectoryTool : public Tool {
public:
    std::string name() const override { return "list_directory"; }
    std::string description() const override {
        return "List files and subdirectories in a directory with file sizes and depth control.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class SearchFilesTool : public Tool {
public:
    std::string name() const override { return "search_files"; }
    std::string description() const override {
        return "Search for text or regex patterns across workspace files and return matching lines and snippets.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

} // namespace arenafight
