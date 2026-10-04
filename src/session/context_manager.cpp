#include "arenafight/session/context_manager.hpp"
#include <sstream>
#include <iostream>

namespace arenafight {

ContextManager::ContextManager(const Config& config)
    : config_(config) {}

std::string ContextManager::buildSystemPrompt(const Session& session) const {
    std::stringstream ss;
    ss << "You are ArenaFight Agent, an autonomous agentic execution engine.\n"
       << "You operate in an autonomous execution loop: PLAN -> EXECUTE -> OBSERVE -> REMEMBER -> VERIFY -> REPLAN.\n\n"
       << "ANTI-CONFABULATION & EVIDENCE-FIRST MANDATE:\n"
       << "1. MODEL OUTPUT != TRUTH. An assertion is only a hypothesis until verified by tool execution.\n"
       << "2. Do not fabricate facts, file contents, or command outputs.\n"
       << "3. Do not claim to have used a tool you did not actually execute.\n"
       << "4. Do not claim to have inspected files you did not inspect via tool calls.\n"
       << "5. Do not claim tests or builds passed unless actual tool execution proves exit code 0.\n"
       << "6. Do not invent APIs, file contents, command syntax, or temporal information.\n"
       << "7. Clearly distinguish FACTS from HYPOTHESES and ASSUMPTIONS.\n"
       << "8. When evidence is insufficient, explicitly state UNKNOWN or INSUFFICIENT EVIDENCE.\n"
       << "9. USER RULE: Always add a 'Made by Lunarmist-byte' credit section with GitHub (https://github.com/Lunarmist-byte) and LinkedIn (https://www.linkedin.com/in/amal-s-kumar-ba69a1290/) links to all created projects, readmes, and documentation.\n"
       << "10. Respond with tool calls when you need to interact with the workspace.\n";
    return ss.str();
}

std::string ContextManager::buildPlanSummary(const Session& session) const {
    std::stringstream ss;
    ss << "Current Execution Plan:\n";
    for (size_t i = 0; i < session.tasks.size(); ++i) {
        const auto& t = session.tasks[i];
        ss << "[" << (i + 1) << "/" << session.tasks.size() << "] "
           << t.id << " - " << t.description
           << " [" << taskStatusToString(t.status) << "]";
        if (!t.dependencies.empty()) {
            ss << " (Depends on: ";
            for (size_t d = 0; d < t.dependencies.size(); ++d) {
                ss << t.dependencies[d] << (d + 1 < t.dependencies.size() ? ", " : "");
            }
            ss << ")";
        }
        ss << "\n";
    }
    return ss.str();
}

AgentRequest ContextManager::buildContext(
    const Session& session,
    const MemoryManager& memory,
    const Task& currentTask,
    const std::string& modelName,
    const std::vector<ToolDefinition>& availableTools,
    const std::map<std::string, std::string>& relevantFileContents
) {
    AgentRequest req;
    req.model = modelName;
    req.systemPrompt = buildSystemPrompt(session);
    req.tools = availableTools;

    std::stringstream userPrompt;
    // 1. Objective
    userPrompt << "OBJECTIVE: " << session.originalTask << "\n\n";

    // 2. Current Task
    userPrompt << "ACTIVE TASK: [" << currentTask.id << "] " << currentTask.description << "\n"
               << "Task Type: " << taskTypeToString(currentTask.type) << "\n"
               << "Attempt: " << (currentTask.attempts + 1) << "\n";
    if (!currentTask.failureReason.empty()) {
        userPrompt << "Previous Failure: " << currentTask.failureReason << "\n";
    }
    userPrompt << "\n";

    // 3. Plan Status
    userPrompt << buildPlanSummary(session) << "\n";

    // 4. Memory Summary
    std::string memSummary = memory.getCompactSummary();
    if (!memSummary.empty()) {
        userPrompt << "SESSION MEMORY:\n" << memSummary << "\n";
    }

    // 5. Relevant File Contents (targeted, not whole repository)
    if (!relevantFileContents.empty()) {
        userPrompt << "RELEVANT FILES:\n";
        for (const auto& [path, content] : relevantFileContents) {
            userPrompt << "--- " << path << " ---\n";
            if (content.size() > 4000) {
                userPrompt << content.substr(0, 4000) << "\n... [truncated] ...\n";
            } else {
                userPrompt << content << "\n";
            }
            userPrompt << "-------------------\n";
        }
        userPrompt << "\n";
    }

    // 6. Recent Executions (last 4 records)
    if (!session.executions.empty()) {
        userPrompt << "RECENT ACTIONS & TOOL RESULTS:\n";
        size_t start = session.executions.size() > 4 ? session.executions.size() - 4 : 0;
        for (size_t i = start; i < session.executions.size(); ++i) {
            const auto& ex = session.executions[i];
            userPrompt << "Tool: " << ex.tool << " | Success: " << (ex.result.success ? "YES" : "NO") << "\n";
            std::string out = ex.result.output;
            if (out.size() > 1000) out = out.substr(0, 1000) + "... [truncated]";
            if (!out.empty()) userPrompt << "Output: " << out << "\n";
            if (!ex.result.error.empty()) userPrompt << "Error: " << ex.result.error << "\n";
        }
        userPrompt << "\n";
    }

    userPrompt << "INSTRUCTION: Perform the active task by invoking necessary tools. When the task is complete, summarize your findings or modifications.\n";

    std::string promptStr = userPrompt.str();
    size_t charLimit = static_cast<size_t>(config_.context_limit * 3); // rough char limit
    if (promptStr.size() > charLimit) {
        promptStr = compactContext(promptStr, charLimit);
    }

    req.userPrompt = promptStr;
    return req;
}

AgentRequest ContextManager::buildPlanningContext(
    const Session& session,
    const MemoryManager& memory,
    const std::string& modelName,
    const std::string& workspaceOverview
) {
    AgentRequest req;
    req.model = modelName;
    req.systemPrompt = buildSystemPrompt(session);

    std::stringstream ss;
    ss << "TASK OBJECTIVE: " << session.originalTask << "\n\n"
       << "WORKSPACE OVERVIEW:\n" << workspaceOverview << "\n\n"
       << "Please create a structured execution plan. Break down the objective into ordered subtasks with clear dependencies.\n"
       << "Output your plan as a JSON array inside a ```json ``` code block:\n"
       << "```json\n"
       << "[\n"
       << "  {\n"
       << "    \"id\": \"task_1\",\n"
       << "    \"description\": \"Inspect workspace and dependencies\",\n"
       << "    \"type\": \"RESEARCH\",\n"
       << "    \"dependencies\": []\n"
       << "  },\n"
       << "  {\n"
       << "    \"id\": \"task_2\",\n"
       << "    \"description\": \"Implement core functionality\",\n"
       << "    \"type\": \"CODING\",\n"
       << "    \"dependencies\": [\"task_1\"]\n"
       << "  }\n"
       << "]\n"
       << "```\n";

    req.userPrompt = ss.str();
    return req;
}

AgentRequest ContextManager::buildVerificationContext(
    const Session& session,
    const MemoryManager& memory,
    const std::string& modelName,
    const std::string& verificationEvidence
) {
    AgentRequest req;
    req.model = modelName;
    req.systemPrompt = "You are an independent Verifier for ArenaFight Agent.\n"
                       "Your job is to objectively inspect whether the original user task has been completed based on actual evidence.\n"
                       "DO NOT accept 'looks good' without proof.\n";

    std::stringstream ss;
    ss << "ORIGINAL OBJECTIVE: " << session.originalTask << "\n\n"
       << "EXECUTION SUMMARY:\n"
       << buildPlanSummary(session) << "\n"
       << "MEMORY SUMMARY:\n" << memory.getCompactSummary() << "\n\n"
       << "VERIFICATION EVIDENCE:\n"
       << verificationEvidence << "\n\n"
       << "Determine if the task is complete. Respond in JSON format:\n"
       << "```json\n"
       << "{\n"
       << "  \"complete\": true,\n"
       << "  \"summary\": \"All files present, build passed, tests passed\",\n"
       << "  \"passedChecks\": [\"Build passed\", \"Tests passed\"],\n"
       << "  \"failedChecks\": []\n"
       << "}\n"
       << "```\n";

    req.userPrompt = ss.str();
    return req;
}

std::string ContextManager::compactContext(const std::string& rawContext, size_t maxChars) {
    if (rawContext.size() <= maxChars) return rawContext;
    // Retain head and tail of prompt
    size_t headSize = maxChars * 2 / 3;
    size_t tailSize = maxChars / 3;
    std::string compacted = rawContext.substr(0, headSize);
    compacted += "\n\n... [Older conversation and tool outputs compacted to preserve memory] ...\n\n";
    compacted += rawContext.substr(rawContext.size() - tailSize);
    return compacted;
}

} // namespace arenafight
