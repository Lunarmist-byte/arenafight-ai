#include "arenafight/executor/executor.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace arenafight {

Executor::Executor(
    ModelRouter& router,
    ToolRegistry& tools,
    ContextManager& contextMgr,
    const Config& config
)
    : router_(router),
      tools_(tools),
      contextMgr_(contextMgr),
      config_(config) {}

std::map<std::string, std::string> Executor::inspectRelevantFiles(
    const Task& task,
    const std::string& workingDir
) {
    std::map<std::string, std::string> files;
    // Scan task description for mentioned files
    static const std::vector<std::string> commonFiles = {
        "CMakeLists.txt", "README.md", "readme.md", "config.json",
        "package.json", "Cargo.toml"
    };

    for (const auto& f : commonFiles) {
        std::string fullPath = workingDir + "/" + f;
        if (std::filesystem::exists(fullPath) && !std::filesystem::is_directory(fullPath)) {
            if (task.description.find(f) != std::string::npos || files.size() < 2) {
                std::ifstream stream(fullPath);
                if (stream.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(stream)),
                                         std::istreambuf_iterator<char>());
                    files[f] = content;
                }
            }
        }
    }
    return files;
}

TaskExecutionResult Executor::executeTask(
    Task& task,
    Session& session,
    MemoryManager& memory,
    const std::string& workingDir
) {
    TaskExecutionResult res;
    task.status = TaskStatus::RUNNING;

    // Model selection with dynamic failure avoidance
    std::string previousFailedModel = (task.attempts > 0) ? task.assignedModel : "";
    ModelSelection selection = router_.selectModel(task.type, previousFailedModel);

    if (!selection.provider) {
        res.success = false;
        res.summary = "No provider available to execute task: " + task.id;
        task.status = TaskStatus::FAILED;
        task.failureReason = res.summary;
        return res;
    }

    task.assignedModel = selection.model.id;
    res.assignedModel = selection.model.id;

    Logger::instance().info("Executing [" + task.id + "] using " +
                           selection.provider->name() + " (" + selection.model.name + ")...");

    auto toolDefs = tools_.getDefinitions();
    std::vector<ChatMessage> conversation;
    int toolCallCount = 0;
    int consecutiveToolFailures = 0;

    while (toolCallCount < config_.max_tool_calls) {
        auto relevantFiles = inspectRelevantFiles(task, workingDir);
        AgentRequest req = contextMgr_.buildContext(
            session, memory, task, selection.model.id, toolDefs, relevantFiles
        );

        // Add conversation turns for tool calling loop
        for (const auto& msg : conversation) {
            req.messages.push_back(msg);
        }

        AgentResponse response = selection.provider->generate(req);
        res.tokensUsed += response.promptTokens + response.completionTokens;

        if (!response.success) {
            Logger::instance().warn("Provider generation error: " + response.errorMessage);

            // Mark the failed provider unavailable to prevent looping
            std::string failedProvider = selection.provider->name();
            router_.markProviderUnavailable(failedProvider);

            // Attempt fallback to another provider if available
            ModelSelection fallback = router_.selectModel(task.type, selection.model.id, "", failedProvider);
            if (fallback.provider && fallback.provider->name() != failedProvider) {
                Logger::instance().recovery("Falling back to provider " + fallback.provider->name() + " (" + fallback.model.name + ")");
                selection = fallback;
                task.assignedModel = fallback.model.id;
                continue;
            } else {
                res.success = false;
                res.summary = "Model generation failed: " + response.errorMessage;
                return res;
            }
        }

        // If no tool calls, model considers task finished
        if (response.toolCalls.empty()) {
            res.success = true;
            res.summary = response.content.empty() ? "Task executed successfully." : response.content;
            res.toolCallsCount = toolCallCount;
            return res;
        }

        // Add assistant message with tool calls to conversation
        ChatMessage assistantMsg;
        assistantMsg.role = "assistant";
        assistantMsg.content = response.content;
        assistantMsg.toolCalls = response.toolCalls;
        conversation.push_back(assistantMsg);

        // Execute each tool call
        for (const auto& tc : response.toolCalls) {
            toolCallCount++;
            res.toolCallsCount++;

            Logger::instance().info("Tool Call: " + tc.name + " (" + tc.arguments.dump() + ")");
            ToolResult toolRes = tools_.execute(tc.name, tc.arguments, workingDir);

            // Record execution
            ExecutionRecord record;
            record.taskId = task.id;
            record.iteration = session.iteration;
            record.model = selection.model.id;
            record.provider = selection.provider->name();
            record.tool = tc.name;
            record.input = tc.arguments;
            record.result = toolRes;
            record.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            session.executions.push_back(record);
            Logger::instance().logExecution(record);

            // Update memory based on tool action
            if (tc.name == "write_file" || tc.name == "edit_file") {
                std::string path = tc.arguments.value("path", "");
                memory.addFileModification(path, "Updated via " + tc.name);
            }

            if (!toolRes.success) {
                consecutiveToolFailures++;
                Logger::instance().warn("Tool error: " + toolRes.error);
                memory.addError(tc.name, toolRes.error);

                if (consecutiveToolFailures >= config_.max_retries) {
                    Logger::instance().error("Tool reached maximum consecutive retries (" +
                                            std::to_string(config_.max_retries) + ")");
                    res.success = false;
                    res.summary = "Tool repeated failures: " + toolRes.error;
                    return res;
                }
            } else {
                consecutiveToolFailures = 0;
                if (!toolRes.output.empty() && tc.name != "read_file") {
                    std::string outShort = toolRes.output;
                    if (outShort.size() > 150) outShort = outShort.substr(0, 150) + "...";
                    memory.addDiscovery(tc.name + " succeeded: " + outShort);
                }
            }

            // Append tool result message
            ChatMessage toolMsg;
            toolMsg.role = "tool";
            toolMsg.name = tc.name;
            toolMsg.toolCallId = tc.id;
            toolMsg.content = toolRes.success ? toolRes.output : ("ERROR: " + toolRes.error);
            conversation.push_back(toolMsg);
        }
    }

    res.success = (consecutiveToolFailures == 0);
    res.summary = "Task completed after reaching maximum tool calls limit";
    res.toolCallsCount = toolCallCount;
    return res;
}

} // namespace arenafight
