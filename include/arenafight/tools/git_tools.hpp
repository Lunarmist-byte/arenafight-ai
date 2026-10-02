#pragma once

#include "arenafight/tools/tool.hpp"

namespace arenafight {

class GitStatusTool : public Tool {
public:
    std::string name() const override { return "git_status"; }
    std::string description() const override {
        return "Get git branch and working tree status (git status --porcelain).";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class GitDiffTool : public Tool {
public:
    std::string name() const override { return "git_diff"; }
    std::string description() const override {
        return "Get working directory or file git diff (git diff).";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

class GitLogTool : public Tool {
public:
    std::string name() const override { return "git_log"; }
    std::string description() const override {
        return "Get recent git commit history (git log -n <limit>).";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

} // namespace arenafight
