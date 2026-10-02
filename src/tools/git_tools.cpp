#include "arenafight/tools/git_tools.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"

namespace arenafight {

// --- GitStatusTool ---
nlohmann::json GitStatusTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"cwd", {{"type", "string"}, {"description", "Optional working directory"}}}
        }}
    };
}

ToolResult GitStatusTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string cwd = arguments.value("cwd", workingDir);
    ProcessResult pr = ProcessExecutor::executeShell("git status -s -b", cwd, 10000);
    res.exitCode = pr.exitCode;
    res.durationMs = pr.durationMs;
    res.output = Logger::sanitize(pr.stdOut);
    res.error = Logger::sanitize(pr.stdErr);
    res.success = (pr.exitCode == 0);
    if (!res.success && res.error.empty()) {
        res.error = "git status failed";
    }
    return res;
}

// --- GitDiffTool ---
nlohmann::json GitDiffTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"file", {{"type", "string"}, {"description", "Optional specific file to diff"}}},
            {"cwd", {{"type", "string"}, {"description", "Optional working directory"}}}
        }}
    };
}

ToolResult GitDiffTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string file = arguments.value("file", "");
    std::string cwd = arguments.value("cwd", workingDir);

    std::string cmd = "git diff";
    if (!file.empty()) {
        cmd += " -- \"" + file + "\"";
    }

    ProcessResult pr = ProcessExecutor::executeShell(cmd, cwd, 15000);
    res.exitCode = pr.exitCode;
    res.durationMs = pr.durationMs;
    res.output = Logger::sanitize(pr.stdOut);
    res.error = Logger::sanitize(pr.stdErr);
    res.success = (pr.exitCode == 0);
    if (res.output.empty() && res.success) {
        res.output = "(No differences found)";
    }
    return res;
}

// --- GitLogTool ---
nlohmann::json GitLogTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"max_commits", {{"type", "integer"}, {"description", "Number of recent commits to display (default: 5)"}}},
            {"cwd", {{"type", "string"}, {"description", "Optional working directory"}}}
        }}
    };
}

ToolResult GitLogTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    int maxCommits = arguments.value("max_commits", 5);
    std::string cwd = arguments.value("cwd", workingDir);

    std::string cmd = "git log -n " + std::to_string(maxCommits) + " --oneline --decorate";
    ProcessResult pr = ProcessExecutor::executeShell(cmd, cwd, 10000);
    res.exitCode = pr.exitCode;
    res.durationMs = pr.durationMs;
    res.output = Logger::sanitize(pr.stdOut);
    res.error = Logger::sanitize(pr.stdErr);
    res.success = (pr.exitCode == 0);
    return res;
}

} // namespace arenafight
