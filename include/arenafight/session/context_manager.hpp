#pragma once

#include <string>
#include <vector>
#include <map>
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/session/memory.hpp"
#include "arenafight/common/config.hpp"

namespace arenafight {

class ContextManager {
public:
    ContextManager(const Config& config);

    // Build the smallest useful context for a specific task
    AgentRequest buildContext(
        const Session& session,
        const MemoryManager& memory,
        const Task& currentTask,
        const std::string& modelName,
        const std::vector<ToolDefinition>& availableTools,
        const std::map<std::string, std::string>& relevantFileContents = {}
    );

    // Build context for planning phase
    AgentRequest buildPlanningContext(
        const Session& session,
        const MemoryManager& memory,
        const std::string& modelName,
        const std::string& workspaceOverview
    );

    // Build context for verification phase
    AgentRequest buildVerificationContext(
        const Session& session,
        const MemoryManager& memory,
        const std::string& modelName,
        const std::string& verificationEvidence
    );

    // Compact older context if exceeding limits
    std::string compactContext(const std::string& rawContext, size_t maxChars);

    // Rough token estimator (~4 chars per token)
    static size_t estimateTokens(const std::string& text) {
        return (text.size() + 3) / 4;
    }

private:
    Config config_;

    std::string buildSystemPrompt(const Session& session) const;
    std::string buildPlanSummary(const Session& session) const;
};

} // namespace arenafight
