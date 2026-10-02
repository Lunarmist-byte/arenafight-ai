#include "arenafight/tools/shell_tools.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>
#include <filesystem>

namespace arenafight {

// --- RunCommandTool ---
nlohmann::json RunCommandTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"command", {{"type", "string"}, {"description", "The shell command line string to execute"}}},
            {"cwd", {{"type", "string"}, {"description", "Optional working directory for command execution"}}},
            {"timeout_ms", {{"type", "integer"}, {"description", "Timeout in milliseconds (default: 120000)"}}}
        }},
        {"required", {"command"}}
    };
}

ToolResult RunCommandTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string command = arguments.value("command", "");
    std::string cwd = arguments.value("cwd", workingDir);
    int timeoutMs = arguments.value("timeout_ms", 120000);

    if (command.empty()) {
        res.success = false;
        res.error = "Missing 'command' argument";
        return res;
    }

    // Safety check for destructive commands
    if (requireConfirmation_ && ProcessExecutor::isDestructive(command)) {
        std::cout << "\n[SECURITY WARNING] The agent requested to execute a potentially destructive command:\n"
                  << "  COMMAND: " << command << "\n"
                  << "Do you want to authorize this operation? (yes/no): " << std::flush;
        std::string reply;
        std::getline(std::cin, reply);
        if (reply != "yes" && reply != "y" && reply != "YES") {
            res.success = false;
            res.error = "Operation aborted by user confirmation check.";
            return res;
        }
    }

    ProcessResult pr = ProcessExecutor::executeShell(command, cwd, timeoutMs);
    res.exitCode = pr.exitCode;
    res.durationMs = pr.durationMs;
    res.output = Logger::sanitize(pr.stdOut);
    res.error = Logger::sanitize(pr.stdErr);
    res.success = (pr.exitCode == 0);

    if (pr.timedOut) {
        res.success = false;
        res.error = "Command timed out after " + std::to_string(timeoutMs) + "ms: " + res.error;
    }

    return res;
}

// --- RunProgramTool ---
nlohmann::json RunProgramTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"program", {{"type", "string"}, {"description", "Executable program name or path"}}},
            {"arguments", {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "Array of argument strings"}}},
            {"cwd", {{"type", "string"}, {"description", "Optional working directory"}}}
        }},
        {"required", {"program"}}
    };
}

ToolResult RunProgramTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string program = arguments.value("program", "");
    std::string cwd = arguments.value("cwd", workingDir);
    std::vector<std::string> args;

    if (arguments.contains("arguments") && arguments["arguments"].is_array()) {
        for (const auto& a : arguments["arguments"]) {
            if (a.is_string()) args.push_back(a.get<std::string>());
        }
    }

    ProcessResult pr = ProcessExecutor::executeProgram(program, args, cwd);
    res.exitCode = pr.exitCode;
    res.durationMs = pr.durationMs;
    res.output = Logger::sanitize(pr.stdOut);
    res.error = Logger::sanitize(pr.stdErr);
    res.success = (pr.exitCode == 0);
    return res;
}

// --- RunTestsTool ---
nlohmann::json RunTestsTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"test_command", {{"type", "string"}, {"description", "Optional custom test command"}}},
            {"cwd", {{"type", "string"}, {"description", "Optional working directory"}}}
        }}
    };
}

ToolResult RunTestsTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string testCmd = arguments.value("test_command", "");
    std::string cwd = arguments.value("cwd", workingDir);

    if (testCmd.empty()) {
        // Auto-detect test runner based on workspace files
        if (std::filesystem::exists(cwd + "/CMakeLists.txt")) {
            if (std::filesystem::exists(cwd + "/build")) {
                testCmd = "ctest --test-dir build --output-on-failure";
            } else {
                testCmd = "cmake -B build -S . && cmake --build build && ctest --test-dir build --output-on-failure";
            }
        } else if (std::filesystem::exists(cwd + "/Cargo.toml")) {
            testCmd = "cargo test";
        } else if (std::filesystem::exists(cwd + "/package.json")) {
            testCmd = "npm test";
        } else if (std::filesystem::exists(cwd + "/pytest.ini") || std::filesystem::exists(cwd + "/tests")) {
            testCmd = "pytest";
        } else {
            res.success = false;
            res.error = "Could not automatically determine test framework. Please specify 'test_command'.";
            return res;
        }
    }

    ProcessResult pr = ProcessExecutor::executeShell(testCmd, cwd, 180000);
    res.exitCode = pr.exitCode;
    res.durationMs = pr.durationMs;
    res.output = Logger::sanitize(pr.stdOut);
    res.error = Logger::sanitize(pr.stdErr);
    res.success = (pr.exitCode == 0);

    return res;
}

} // namespace arenafight
