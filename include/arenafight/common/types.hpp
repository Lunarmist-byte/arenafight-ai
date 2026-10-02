#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace arenafight {

enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    BLOCKED,
    SKIPPED
};

inline std::string taskStatusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::PENDING: return "PENDING";
        case TaskStatus::RUNNING: return "RUNNING";
        case TaskStatus::COMPLETED: return "COMPLETED";
        case TaskStatus::FAILED: return "FAILED";
        case TaskStatus::BLOCKED: return "BLOCKED";
        case TaskStatus::SKIPPED: return "SKIPPED";
        default: return "UNKNOWN";
    }
}

inline TaskStatus taskStatusFromString(const std::string& str) {
    if (str == "RUNNING") return TaskStatus::RUNNING;
    if (str == "COMPLETED") return TaskStatus::COMPLETED;
    if (str == "FAILED") return TaskStatus::FAILED;
    if (str == "BLOCKED") return TaskStatus::BLOCKED;
    if (str == "SKIPPED") return TaskStatus::SKIPPED;
    return TaskStatus::PENDING;
}

enum class TaskType {
    PLANNING,
    CODING,
    DEBUGGING,
    RESEARCH,
    REVIEW,
    VERIFICATION,
    GENERAL
};

inline std::string taskTypeToString(TaskType type) {
    switch (type) {
        case TaskType::PLANNING: return "PLANNING";
        case TaskType::CODING: return "CODING";
        case TaskType::DEBUGGING: return "DEBUGGING";
        case TaskType::RESEARCH: return "RESEARCH";
        case TaskType::REVIEW: return "REVIEW";
        case TaskType::VERIFICATION: return "VERIFICATION";
        default: return "GENERAL";
    }
}

inline TaskType taskTypeFromString(const std::string& str) {
    if (str == "PLANNING") return TaskType::PLANNING;
    if (str == "CODING") return TaskType::CODING;
    if (str == "DEBUGGING") return TaskType::DEBUGGING;
    if (str == "RESEARCH") return TaskType::RESEARCH;
    if (str == "REVIEW") return TaskType::REVIEW;
    if (str == "VERIFICATION") return TaskType::VERIFICATION;
    return TaskType::GENERAL;
}

struct Task {
    std::string id;
    std::string description;
    TaskType type = TaskType::GENERAL;
    std::vector<std::string> dependencies;
    TaskStatus status = TaskStatus::PENDING;
    int attempts = 0;
    std::string result;
    std::string failureReason;
    std::string assignedModel;
    int64_t createdAtMs = 0;
    int64_t completedAtMs = 0;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"description", description},
            {"type", taskTypeToString(type)},
            {"dependencies", dependencies},
            {"status", taskStatusToString(status)},
            {"attempts", attempts},
            {"result", result},
            {"failureReason", failureReason},
            {"assignedModel", assignedModel},
            {"createdAtMs", createdAtMs},
            {"completedAtMs", completedAtMs}
        };
    }

    static Task fromJson(const nlohmann::json& j) {
        Task t;
        t.id = j.value("id", "");
        t.description = j.value("description", "");
        t.type = taskTypeFromString(j.value("type", "GENERAL"));
        t.dependencies = j.value("dependencies", std::vector<std::string>{});
        t.status = taskStatusFromString(j.value("status", "PENDING"));
        t.attempts = j.value("attempts", 0);
        t.result = j.value("result", "");
        t.failureReason = j.value("failureReason", "");
        t.assignedModel = j.value("assignedModel", "");
        t.createdAtMs = j.value("createdAtMs", static_cast<int64_t>(0));
        t.completedAtMs = j.value("completedAtMs", static_cast<int64_t>(0));
        return t;
    }
};

struct ToolResult {
    bool success = false;
    std::string output;
    std::string error;
    int exitCode = 0;
    int64_t durationMs = 0;

    nlohmann::json toJson() const {
        return {
            {"success", success},
            {"output", output},
            {"error", error},
            {"exitCode", exitCode},
            {"durationMs", durationMs}
        };
    }

    static ToolResult fromJson(const nlohmann::json& j) {
        ToolResult r;
        r.success = j.value("success", false);
        r.output = j.value("output", "");
        r.error = j.value("error", "");
        r.exitCode = j.value("exitCode", 0);
        r.durationMs = j.value("durationMs", static_cast<int64_t>(0));
        return r;
    }
};

struct ToolCall {
    std::string id;
    std::string name;
    nlohmann::json arguments;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"arguments", arguments}
        };
    }

    static ToolCall fromJson(const nlohmann::json& j) {
        ToolCall tc;
        tc.id = j.value("id", "");
        tc.name = j.value("name", "");
        if (j.contains("arguments")) {
            if (j["arguments"].is_string()) {
                try {
                    tc.arguments = nlohmann::json::parse(j["arguments"].get<std::string>());
                } catch (...) {
                    tc.arguments = j["arguments"];
                }
            } else {
                tc.arguments = j["arguments"];
            }
        }
        return tc;
    }
};

struct ToolDefinition {
    std::string name;
    std::string description;
    nlohmann::json parameters;

    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"description", description},
            {"parameters", parameters}
        };
    }
};

struct MemoryItem {
    std::string id;
    std::string category; // "fact", "decision", "discovery", "error", "file_change"
    std::string content;
    std::string source;
    int64_t timestamp = 0;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"category", category},
            {"content", content},
            {"source", source},
            {"timestamp", timestamp}
        };
    }

    static MemoryItem fromJson(const nlohmann::json& j) {
        MemoryItem m;
        m.id = j.value("id", "");
        m.category = j.value("category", "fact");
        m.content = j.value("content", "");
        m.source = j.value("source", "");
        m.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        return m;
    }
};

struct ExecutionRecord {
    std::string taskId;
    int iteration = 0;
    std::string model;
    std::string provider;
    std::string tool;
    nlohmann::json input;
    ToolResult result;
    int64_t timestamp = 0;

    nlohmann::json toJson() const {
        return {
            {"taskId", taskId},
            {"iteration", iteration},
            {"model", model},
            {"provider", provider},
            {"tool", tool},
            {"input", input},
            {"result", result.toJson()},
            {"timestamp", timestamp}
        };
    }

    static ExecutionRecord fromJson(const nlohmann::json& j) {
        ExecutionRecord er;
        er.taskId = j.value("taskId", "");
        er.iteration = j.value("iteration", 0);
        er.model = j.value("model", "");
        er.provider = j.value("provider", "");
        er.tool = j.value("tool", "");
        if (j.contains("input")) er.input = j["input"];
        if (j.contains("result")) er.result = ToolResult::fromJson(j["result"]);
        er.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        return er;
    }
};

struct ModelInfo {
    std::string id;
    std::string name;
    std::string provider;
    size_t contextWindow = 8192;
    bool isLocal = false;
    std::string description;
    float capabilityScore = 1.0f; // estimated capability

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"provider", provider},
            {"contextWindow", contextWindow},
            {"isLocal", isLocal},
            {"description", description},
            {"capabilityScore", capabilityScore}
        };
    }
};

struct ChatMessage {
    std::string role; // "system", "user", "assistant", "tool"
    std::string content;
    std::string name;
    std::string toolCallId;
    std::vector<ToolCall> toolCalls;

    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"role", role},
            {"content", content}
        };
        if (!name.empty()) j["name"] = name;
        if (!toolCallId.empty()) j["tool_call_id"] = toolCallId;
        if (!toolCalls.empty()) {
            nlohmann::json tcArray = nlohmann::json::array();
            for (const auto& tc : toolCalls) {
                tcArray.push_back({
                    {"id", tc.id},
                    {"type", "function"},
                    {"function", {
                        {"name", tc.name},
                        {"arguments", tc.arguments.dump()}
                    }}
                });
            }
            j["tool_calls"] = tcArray;
        }
        return j;
    }

    static ChatMessage fromJson(const nlohmann::json& j) {
        ChatMessage msg;
        msg.role = j.value("role", "user");
        msg.content = j.value("content", "");
        msg.name = j.value("name", "");
        msg.toolCallId = j.value("tool_call_id", "");
        if (j.contains("tool_calls") && j["tool_calls"].is_array()) {
            for (const auto& item : j["tool_calls"]) {
                ToolCall tc;
                tc.id = item.value("id", "");
                if (item.contains("function")) {
                    tc.name = item["function"].value("name", "");
                    std::string argsStr = item["function"].value("arguments", "{}");
                    try {
                        tc.arguments = nlohmann::json::parse(argsStr);
                    } catch (...) {
                        tc.arguments = argsStr;
                    }
                }
                msg.toolCalls.push_back(tc);
            }
        }
        return msg;
    }
};

struct AgentRequest {
    std::string model;
    std::string systemPrompt;
    std::string userPrompt;
    std::vector<ChatMessage> messages;
    std::vector<ToolDefinition> tools;
    float temperature = 0.2f;
    int maxTokens = 4096;
    bool stream = false;
};

struct AgentResponse {
    bool success = false;
    std::string content;
    std::vector<ToolCall> toolCalls;
    std::string finishReason;
    std::string errorMessage;
    int promptTokens = 0;
    int completionTokens = 0;
};

struct VerificationResult {
    bool complete = false;
    std::string summary;
    std::vector<std::string> passedChecks;
    std::vector<std::string> failedChecks;
    std::string evidence;

    nlohmann::json toJson() const {
        return {
            {"complete", complete},
            {"summary", summary},
            {"passedChecks", passedChecks},
            {"failedChecks", failedChecks},
            {"evidence", evidence}
        };
    }

    static VerificationResult fromJson(const nlohmann::json& j) {
        VerificationResult vr;
        vr.complete = j.value("complete", false);
        vr.summary = j.value("summary", "");
        vr.passedChecks = j.value("passedChecks", std::vector<std::string>{});
        vr.failedChecks = j.value("failedChecks", std::vector<std::string>{});
        vr.evidence = j.value("evidence", "");
        return vr;
    }
};

} // namespace arenafight
