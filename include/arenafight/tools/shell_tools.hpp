#pragma once

#include "arenafight/tools/tool.hpp"

namespace arenafight {

class RunCommandTool : public Tool {
public:
    explicit RunCommandTool(bool requireConfirmation = true)
        : requireConfirmation_(requireConfirmation) {}

    std::string name() const override { return "run_command"; }
    std::string description() const override {
        return "Execute a shell command (PowerShell on Windows, bash on Linux). Captures stdout, stderr, exit code, and execution time.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;

private:
    bool requireConfirmation_;
};

class RunProgramTool : public Tool {
public:
    std::string name() const override { return "run_program"; }
    std::string description() const override {
        return "Execute an executable program directly with arguments list.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class RunTestsTool : public Tool {
public:
    std::string name() const override { return "run_tests"; }
    std::string description() const override {
        return "Execute test suite (auto-detects ctest, cargo test, npm test, pytest, etc. if command not specified).";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

} // namespace arenafight
