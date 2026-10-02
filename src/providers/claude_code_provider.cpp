#include "arenafight/providers/claude_code_provider.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace arenafight {

ClaudeCodeProvider::ClaudeCodeProvider(std::string claudeCommand)
    : claudeCommand_(std::move(claudeCommand)) {}

void ClaudeCodeProvider::checkVersion() {
    if (checkedAvailability_) return;
    checkedAvailability_ = true;

    // Check if claude or claude.cmd reports version
    std::string verCmd = claudeCommand_ + " --version";
    ProcessResult res = ProcessExecutor::executeShell(verCmd, ".", 5000);
    if (res.exitCode != 0 && ProcessExecutor::isWindows() && claudeCommand_ == "claude") {
        claudeCommand_ = "claude.cmd";
        verCmd = claudeCommand_ + " --version";
        res = ProcessExecutor::executeShell(verCmd, ".", 5000);
    }

    if (res.exitCode == 0 && !res.stdOut.empty()) {
        detectedVersion_ = res.stdOut;
        while (!detectedVersion_.empty() && (detectedVersion_.back() == '\r' || detectedVersion_.back() == '\n' || detectedVersion_.back() == ' ')) {
            detectedVersion_.pop_back();
        }
        isInstalled_ = true;

        isAvailable_ = true;
        statusMessage_ = "Installed (" + detectedVersion_ + ")";
    } else {
        isInstalled_ = false;
        isAvailable_ = false;
        statusMessage_ = "Not installed or not in PATH";
    }
}

bool ClaudeCodeProvider::available() {
    checkVersion();
    return isAvailable_;
}

std::vector<ModelInfo> ClaudeCodeProvider::discoverModels() {
    std::vector<ModelInfo> models;
    checkVersion();
    if (!isAvailable_) return models;

    ModelInfo defaultModel;
    defaultModel.id = "claude-code-default";
    defaultModel.name = "Claude Code CLI (" + detectedVersion_ + ")";
    defaultModel.provider = "ClaudeCode";
    defaultModel.contextWindow = 200000;
    defaultModel.isLocal = false;
    defaultModel.capabilityScore = 3.0f;
    defaultModel.description = "Local Claude Code CLI non-interactive agent backend";
    models.push_back(defaultModel);

    ModelInfo sonnetModel;
    sonnetModel.id = "claude-3-7-sonnet";
    sonnetModel.name = "Claude 3.7 Sonnet (via Claude Code)";
    sonnetModel.provider = "ClaudeCode";
    sonnetModel.contextWindow = 200000;
    sonnetModel.isLocal = false;
    sonnetModel.capabilityScore = 3.2f;
    sonnetModel.description = "Anthropic Claude 3.7 Sonnet via Claude CLI";
    models.push_back(sonnetModel);

    return models;
}

AgentResponse ClaudeCodeProvider::generate(const AgentRequest& request) {
    AgentResponse response;
    checkVersion();
    if (!isAvailable_) {
        response.success = false;
        response.errorMessage = "Claude Code CLI is not available: " + statusMessage_;
        return response;
    }

    // Build instruction prompt
    std::stringstream promptBuilder;
    if (!request.systemPrompt.empty()) {
        promptBuilder << "System Instructions:\n" << request.systemPrompt << "\n\n";
    }

    if (!request.tools.empty()) {
        promptBuilder << "You have access to the following tools. To call a tool, respond with a JSON block:\n"
                      << "```json\n"
                      << "{\"tool\": \"tool_name\", \"arguments\": { ... }}\n"
                      << "```\n\n"
                      << "Tools available:\n";
        for (const auto& t : request.tools) {
            promptBuilder << "- " << t.name << ": " << t.description << "\n"
                          << "  Parameters: " << t.parameters.dump() << "\n";
        }
        promptBuilder << "\n";
    }

    for (const auto& msg : request.messages) {
        promptBuilder << "[" << msg.role << "]: " << msg.content << "\n";
    }

    if (!request.userPrompt.empty()) {
        promptBuilder << "[user]: " << request.userPrompt << "\n";
    }

    std::string prompt = promptBuilder.str();

    // Use temporary prompt file to prevent shell escaping issues, managed via RAII guard
    auto nowTicks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string tmpPromptFile = ".claude_prompt_" + std::to_string(nowTicks) + ".tmp";
    
    struct FileCleanupGuard {
        std::string path;
        ~FileCleanupGuard() {
            if (!path.empty()) {
                std::error_code ec;
                std::filesystem::remove(path, ec);
            }
        }
    } fileGuard{tmpPromptFile};

    {
        std::ofstream pf(tmpPromptFile, std::ios::trunc);
        if (pf.is_open()) {
            pf << prompt;
            pf.close();
        }
    }

    std::string runCmd;
    if (ProcessExecutor::isWindows()) {
        runCmd = "Get-Content -Raw " + tmpPromptFile + " | " + claudeCommand_ + " -p";
    } else {
        runCmd = "cat " + tmpPromptFile + " | " + claudeCommand_ + " -p";
    }

    ProcessResult procRes = ProcessExecutor::executeShell(runCmd, ".", 120000);

    if (procRes.timedOut) {
        response.success = false;
        response.errorMessage = "Claude Code execution timed out";
        return response;
    }

    std::string combinedOutput = procRes.stdOut + "\n" + procRes.stdErr;
    if (combinedOutput.find("Not logged in") != std::string::npos ||
        combinedOutput.find("/login") != std::string::npos) {
        isAvailable_ = false;
        response.success = false;
        response.errorMessage = "Claude Code is not authenticated. Run 'claude login' or use another configured backend.";
        return response;
    }

    if (procRes.exitCode != 0 && procRes.stdOut.empty()) {
        response.success = false;
        response.errorMessage = "Claude Code failed (exit " + std::to_string(procRes.exitCode) + "): " + procRes.stdErr;
        return response;
    }

    response.success = true;
    response.content = procRes.stdOut;
    response.toolCalls = extractToolCallsFromText(response.content);

    return response;
}

} // namespace arenafight
