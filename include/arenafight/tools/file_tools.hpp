#pragma once

#include "arenafight/tools/tool.hpp"

namespace arenafight {

class ReadFileTool : public Tool {
public:
    std::string name() const override { return "read_file"; }
    std::string description() const override {
        return "Read the contents of a file with line numbers. Supports optional start_line and end_line.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class WriteFileTool : public Tool {
public:
    std::string name() const override { return "write_file"; }
    std::string description() const override {
        return "Write or create a file with the given content. Creates parent directories automatically.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class EditFileTool : public Tool {
public:
    std::string name() const override { return "edit_file"; }
    std::string description() const override {
        return "Perform a targeted search-and-replace edit on an existing file with patch validation and automatic backup.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class CreateDirectoryTool : public Tool {
public:
    std::string name() const override { return "create_directory"; }
    std::string description() const override {
        return "Create a directory path recursively.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class DeleteFileTool : public Tool {
public:
    explicit DeleteFileTool(bool requireConfirmation = true)
        : requireConfirmation_(requireConfirmation) {}

    std::string name() const override { return "delete_file"; }
    std::string description() const override {
        return "Delete a file from the workspace.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;

private:
    bool requireConfirmation_;
};

} // namespace arenafight
