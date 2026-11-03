#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "arenafight/evidence/evidence_types.hpp"

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
    HARDWARE,
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
        case TaskType::HARDWARE: return "HARDWARE";
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
    if (str == "HARDWARE") return TaskType::HARDWARE;
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
    std::string plannedModel;
    std::string abilityReason;
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
            {"plannedModel", plannedModel},
            {"abilityReason", abilityReason},
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
        t.plannedModel = j.value("plannedModel", "");
        t.abilityReason = j.value("abilityReason", "");
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
    KnowledgeType knowledgeType = KnowledgeType::FACT;
    EvidenceLevel evidenceLevel = EvidenceLevel::UNKNOWN;
    std::string evidenceId;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"category", category},
            {"content", content},
            {"source", source},
            {"timestamp", timestamp},
            {"knowledgeType", knowledgeTypeToString(knowledgeType)},
            {"evidenceLevel", evidenceLevelToString(evidenceLevel)},
            {"evidenceId", evidenceId}
        };
    }

    static MemoryItem fromJson(const nlohmann::json& j) {
        MemoryItem m;
        m.id = j.value("id", "");
        m.category = j.value("category", "fact");
        m.content = j.value("content", "");
        m.source = j.value("source", "");
        m.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        m.knowledgeType = knowledgeTypeFromString(j.value("knowledgeType", "FACT"));
        m.evidenceLevel = evidenceLevelFromString(j.value("evidenceLevel", "UNKNOWN"));
        m.evidenceId = j.value("evidenceId", "");
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

struct ModelAbilities {
    float coding = 0.5f;        // 0.0 to 1.0: C++, Python, algorithms, syntax precision
    float reasoning = 0.5f;     // 0.0 to 1.0: Multi-step logic, architectural planning, math
    float planning = 0.5f;      // 0.0 to 1.0: Task decomposition, dependency graphs, roadmaps
    float verification = 0.5f;  // 0.0 to 1.0: Bug detection, adversarial testing, edge cases
    float hardware = 0.5f;      // 0.0 to 1.0: Embedded C/C++, pinout registers, Wokwi, circuits
    float speed = 0.5f;         // 0.0 to 1.0: Token throughput, low latency
    float context = 0.5f;       // 0.0 to 1.0: Large context processing capability

    float scoreForTask(TaskType type) const {
        switch (type) {
            case TaskType::PLANNING:     return planning * 0.6f + reasoning * 0.4f;
            case TaskType::CODING:       return coding * 0.7f + reasoning * 0.3f;
            case TaskType::DEBUGGING:    return reasoning * 0.4f + coding * 0.4f + verification * 0.2f;
            case TaskType::RESEARCH:     return context * 0.5f + reasoning * 0.3f + speed * 0.2f;
            case TaskType::REVIEW:       return verification * 0.6f + coding * 0.4f;
            case TaskType::VERIFICATION: return verification * 0.7f + reasoning * 0.3f;
            case TaskType::HARDWARE:     return hardware * 0.6f + coding * 0.4f;
            default:                     return (coding + reasoning + speed) / 3.0f;
        }
    }

    std::string primaryStrength() const {
        float maxVal = coding;
        std::string best = "Coding";
        if (reasoning > maxVal) { maxVal = reasoning; best = "Reasoning"; }
        if (planning > maxVal) { maxVal = planning; best = "Planning"; }
        if (verification > maxVal) { maxVal = verification; best = "Verification"; }
        if (hardware > maxVal) { maxVal = hardware; best = "Embedded/Hardware"; }
        if (speed > maxVal) { maxVal = speed; best = "Speed/Latency"; }
        if (context > maxVal) { maxVal = context; best = "Large Context"; }
        return best;
    }

    nlohmann::json toJson() const {
        return {
            {"coding", coding},
            {"reasoning", reasoning},
            {"planning", planning},
            {"verification", verification},
            {"hardware", hardware},
            {"speed", speed},
            {"context", context},
            {"primaryStrength", primaryStrength()}
        };
    }

    static ModelAbilities fromJson(const nlohmann::json& j) {
        ModelAbilities a;
        a.coding = j.value("coding", 0.5f);
        a.reasoning = j.value("reasoning", 0.5f);
        a.planning = j.value("planning", 0.5f);
        a.verification = j.value("verification", 0.5f);
        a.hardware = j.value("hardware", 0.5f);
        a.speed = j.value("speed", 0.5f);
        a.context = j.value("context", 0.5f);
        return a;
    }

    static ModelAbilities infer(const std::string& id, const std::string& desc = "") {
        ModelAbilities a;
        // Strong universal baseline for any model
        a.coding = 0.82f;
        a.reasoning = 0.82f;
        a.planning = 0.80f;
        a.verification = 0.80f;
        a.hardware = 0.78f;
        a.speed = 0.82f;
        a.context = 0.80f;

        std::string s = id + " " + desc;
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        // Domain-specific roles
        if (s.find("coder") != std::string::npos || s.find("code") != std::string::npos || s.find("dev") != std::string::npos) {
            a.coding = 0.96f; a.hardware = std::max(a.hardware, 0.92f);
        }
        if (s.find("r1") != std::string::npos || s.find("reason") != std::string::npos ||
            s.find("o1") != std::string::npos || s.find("o3") != std::string::npos || s.find("think") != std::string::npos) {
            a.reasoning = 0.98f; a.planning = 0.95f; a.verification = 0.94f;
        }
        if (s.find("sonnet") != std::string::npos || s.find("claude") != std::string::npos || s.find("opus") != std::string::npos) {
            a.coding = 0.96f; a.reasoning = 0.97f; a.planning = 0.96f; a.verification = 0.95f; a.context = 0.98f;
        }
        if (s.find("verifier") != std::string::npos || s.find("audit") != std::string::npos || s.find("review") != std::string::npos) {
            a.verification = 0.98f; a.reasoning = 0.92f;
        }
        if (s.find("deepseek") != std::string::npos) {
            a.reasoning = std::max(a.reasoning, 0.96f); a.coding = std::max(a.coding, 0.95f);
        }
        if (s.find("qwen") != std::string::npos || s.find("qwq") != std::string::npos) {
            a.coding = std::max(a.coding, 0.94f); a.speed = std::max(a.speed, 0.90f);
            if (s.find("qwq") != std::string::npos) { a.reasoning = 0.98f; a.planning = 0.96f; }
        }
        if (s.find("gpt-4") != std::string::npos || s.find("openai") != std::string::npos || s.find("chatgpt") != std::string::npos) {
            a.reasoning = std::max(a.reasoning, 0.95f); a.planning = std::max(a.planning, 0.94f); a.coding = std::max(a.coding, 0.94f);
        }
        if (s.find("llama") != std::string::npos) {
            a.reasoning = std::max(a.reasoning, 0.90f); a.speed = std::max(a.speed, 0.90f);
        }
        if (s.find("mistral") != std::string::npos || s.find("mixtral") != std::string::npos || s.find("codestral") != std::string::npos) {
            a.coding = std::max(a.coding, 0.94f); a.speed = std::max(a.speed, 0.92f);
            if (s.find("codestral") != std::string::npos) { a.coding = 0.97f; a.hardware = 0.93f; }
        }
        if (s.find("gemini") != std::string::npos) {
            a.context = 0.99f; a.planning = 0.94f;
        }
        if (s.find("phi") != std::string::npos) {
            a.reasoning = std::max(a.reasoning, 0.91f); a.speed = std::max(a.speed, 0.93f);
        }
        if (s.find("gemma") != std::string::npos) {
            a.coding = std::max(a.coding, 0.91f); a.speed = std::max(a.speed, 0.92f);
        }

        // Parameter scale multipliers
        if (s.find("405b") != std::string::npos) {
            a.reasoning = 0.99f; a.planning = 0.98f; a.coding = std::max(a.coding, 0.97f); a.context = 0.98f;
        } else if (s.find("70b") != std::string::npos || s.find("72b") != std::string::npos) {
            a.reasoning = std::max(a.reasoning, 0.96f); a.planning = std::max(a.planning, 0.95f); a.coding = std::max(a.coding, 0.95f);
        } else if (s.find("32b") != std::string::npos || s.find("34b") != std::string::npos) {
            a.coding = std::max(a.coding, 0.96f); a.hardware = std::max(a.hardware, 0.94f);
        } else if (s.find("flash") != std::string::npos || s.find("mini") != std::string::npos || s.find("haiku") != std::string::npos || s.find("1b") != std::string::npos || s.find("3b") != std::string::npos) {
            a.speed = 0.98f;
        }

        // Fine-tuned / LoRA / Unsloth adapters
        if (s.find("unsloth") != std::string::npos || s.find("lora") != std::string::npos || s.find("finetun") != std::string::npos || s.find("dpo") != std::string::npos) {
            a.coding = std::max(a.coding, 0.96f); a.hardware = std::max(a.hardware, 0.95f); a.speed = std::max(a.speed, 0.95f);
        }

        // Hardware & embedded constraints
        if (s.find("embedded") != std::string::npos || s.find("hardware") != std::string::npos || s.find("esp32") != std::string::npos || s.find("firmware") != std::string::npos || s.find("pico") != std::string::npos) {
            a.hardware = 0.98f; a.coding = std::max(a.coding, 0.94f);
        }
        return a;
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
    ModelAbilities abilities;

    ModelInfo() = default;
    ModelInfo(std::string id_, std::string name_, std::string prov_, size_t ctx_, bool local_, std::string desc_, float cap_, ModelAbilities ab_ = {})
        : id(std::move(id_)), name(std::move(name_)), provider(std::move(prov_)),
          contextWindow(ctx_), isLocal(local_), description(std::move(desc_)),
          capabilityScore(cap_), abilities(std::move(ab_)) {}

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"provider", provider},
            {"contextWindow", contextWindow},
            {"isLocal", isLocal},
            {"description", description},
            {"capabilityScore", capabilityScore},
            {"abilities", abilities.toJson()}
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
    std::string thinking;
    std::vector<ToolCall> toolCalls;
    std::string finishReason;
    std::string errorMessage;
    int promptTokens = 0;
    int completionTokens = 0;
};

struct ThinkingEvaluation {
    std::string model;
    std::string thinking;
    float edgeCaseScore = 0.0f;       // 0.0 to 1.0: Anticipation of boundary conditions, nulls, race conditions
    float invariantScore = 0.0f;      // 0.0 to 1.0: RAII, memory bounds, hardware constraints, safety
    float efficiencyScore = 0.0f;     // 0.0 to 1.0: Time/space complexity, modularity, algorithmic elegance
    float codeDisciplineScore = 0.0f; // 0.0 to 1.0: Strictly reasoning/planning without premature output
    float compositeScore = 0.0f;      // Weighted overall score
    int thinkingTokens = 0;
    int latencyMs = 0;
    std::vector<std::string> strengths;
    std::vector<std::string> edgeCasesIdentified;

    nlohmann::json toJson() const {
        return {
            {"model", model},
            {"thinking", thinking},
            {"edgeCaseScore", edgeCaseScore},
            {"invariantScore", invariantScore},
            {"efficiencyScore", efficiencyScore},
            {"codeDisciplineScore", codeDisciplineScore},
            {"compositeScore", compositeScore},
            {"thinkingTokens", thinkingTokens},
            {"latencyMs", latencyMs},
            {"strengths", strengths},
            {"edgeCasesIdentified", edgeCasesIdentified}
        };
    }

    static ThinkingEvaluation fromJson(const nlohmann::json& j) {
        ThinkingEvaluation te;
        te.model = j.value("model", "");
        te.thinking = j.value("thinking", "");
        te.edgeCaseScore = j.value("edgeCaseScore", 0.0f);
        te.invariantScore = j.value("invariantScore", 0.0f);
        te.efficiencyScore = j.value("efficiencyScore", 0.0f);
        te.codeDisciplineScore = j.value("codeDisciplineScore", 0.0f);
        te.compositeScore = j.value("compositeScore", 0.0f);
        te.thinkingTokens = j.value("thinkingTokens", 0);
        te.latencyMs = j.value("latencyMs", 0);
        te.strengths = j.value("strengths", std::vector<std::string>{});
        te.edgeCasesIdentified = j.value("edgeCasesIdentified", std::vector<std::string>{});
        return te;
    }
};

struct ThinkingDuelResult {
    std::string taskObjective;
    std::string winningModel;
    std::string losingModel;
    std::string winningRationale;
    ThinkingEvaluation candidateA;
    ThinkingEvaluation candidateB;
    int tokensSaved = 0;              // Tokens avoided by disallowing full generation
    int estimatedMsSaved = 0;         // Latency saved by early stopping
    bool earlyTermination = true;     // Disallowed full code generation

    nlohmann::json toJson() const {
        return {
            {"taskObjective", taskObjective},
            {"winningModel", winningModel},
            {"losingModel", losingModel},
            {"winningRationale", winningRationale},
            {"candidateA", candidateA.toJson()},
            {"candidateB", candidateB.toJson()},
            {"tokensSaved", tokensSaved},
            {"estimatedMsSaved", estimatedMsSaved},
            {"earlyTermination", earlyTermination}
        };
    }

    static ThinkingDuelResult fromJson(const nlohmann::json& j) {
        ThinkingDuelResult res;
        res.taskObjective = j.value("taskObjective", "");
        res.winningModel = j.value("winningModel", "");
        res.losingModel = j.value("losingModel", "");
        res.winningRationale = j.value("winningRationale", "");
        if (j.contains("candidateA")) res.candidateA = ThinkingEvaluation::fromJson(j["candidateA"]);
        if (j.contains("candidateB")) res.candidateB = ThinkingEvaluation::fromJson(j["candidateB"]);
        res.tokensSaved = j.value("tokensSaved", 0);
        res.estimatedMsSaved = j.value("estimatedMsSaved", 0);
        res.earlyTermination = j.value("earlyTermination", true);
        return res;
    }
};

struct VerificationResult {
    bool complete = false;
    std::string summary;
    std::vector<std::string> passedChecks;
    std::vector<std::string> failedChecks;
    std::string evidence;
    double completionScore = 0.0;
    double confidence = 0.0;
    std::vector<GateCheckResult> gates;
    std::vector<std::string> remainingIssues;
    GroundedFinalAnswer groundedAnswer;

    nlohmann::json toJson() const {
        nlohmann::json gatesArr = nlohmann::json::array();
        for (const auto& g : gates) {
            gatesArr.push_back({
                {"gateName", g.gateName},
                {"passed", g.passed},
                {"detail", g.detail},
                {"evidenceId", g.evidenceId},
                {"hierarchy", static_cast<int>(g.hierarchy)}
            });
        }

        return {
            {"complete", complete},
            {"summary", summary},
            {"passedChecks", passedChecks},
            {"failedChecks", failedChecks},
            {"evidence", evidence},
            {"completionScore", completionScore},
            {"confidence", confidence},
            {"remainingIssues", remainingIssues},
            {"gates", gatesArr}
        };
    }

    static VerificationResult fromJson(const nlohmann::json& j) {
        VerificationResult vr;
        vr.complete = j.value("complete", false);
        vr.summary = j.value("summary", "");
        vr.passedChecks = j.value("passedChecks", std::vector<std::string>{});
        vr.failedChecks = j.value("failedChecks", std::vector<std::string>{});
        vr.evidence = j.value("evidence", "");
        vr.completionScore = j.value("completionScore", 0.0);
        vr.confidence = j.value("confidence", 0.0);
        vr.remainingIssues = j.value("remainingIssues", std::vector<std::string>{});

        if (j.contains("gates") && j["gates"].is_array()) {
            for (const auto& gj : j["gates"]) {
                GateCheckResult g;
                g.gateName = gj.value("gateName", "");
                g.passed = gj.value("passed", false);
                g.detail = gj.value("detail", "");
                g.evidenceId = gj.value("evidenceId", "");
                g.hierarchy = static_cast<EvidenceHierarchy>(gj.value("hierarchy", 6));
                vr.gates.push_back(g);
            }
        }
        return vr;
    }
};

} // namespace arenafight
