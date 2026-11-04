#include "arenafight/providers/llama_cpp_provider.hpp"
#include "arenafight/services/llama_server_daemon.hpp"
#include "arenafight/system/system_profiler.hpp"
#include "arenafight/common/http_client.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>
#include <sstream>
#include <cctype>
#include <nlohmann/json.hpp>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

LlamaCppProvider::LlamaCppProvider(std::string serverUrl, int gpuLayers)
    : serverUrl_(std::move(serverUrl)), gpuLayers_(gpuLayers) {}

bool LlamaCppProvider::supportsTools(const std::string& modelId) const {
    if (toolSupportingModels_.count(modelId) > 0) return true;
    std::string lower = modelId;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower.find("gemma") != std::string::npos) return false;
    if (lower.find("qwen") != std::string::npos ||
        lower.find("llama") != std::string::npos ||
        lower.find("mistral") != std::string::npos ||
        lower.find("deepseek") != std::string::npos ||
        lower.find("coder") != std::string::npos) {
        return true;
    }
    return false;
}

bool LlamaCppProvider::available() {
    if (LlamaServerDaemon::isServerResponding(serverUrl_)) {
        return true;
    }
    std::string exe = LlamaServerDaemon::findLlamaServerExecutable();
    if (exe.empty()) {
        return false;
    }
    auto models = LlamaServerDaemon::discoverModels();
    return !models.empty();
}

std::vector<ModelInfo> LlamaCppProvider::discoverModels() {
    std::vector<ModelInfo> models;
    auto localModels = LlamaServerDaemon::discoverModels();

    for (const auto& lm : localModels) {
        ModelInfo info;
        info.id = lm.id;
        info.name = lm.id;
        info.provider = "llama.cpp";
        info.isLocal = true;
        info.contextWindow = lm.contextWindow > 0 ? lm.contextWindow : 8192;
        info.description = "llama.cpp native GGUF (" + lm.parameterSize + ", " + lm.quantization + ")";
        info.abilities = ModelAbilities::infer(lm.id, info.description);
        info.capabilityScore = info.abilities.scoreForTask(TaskType::CODING);

        if (supportsTools(lm.id)) {
            toolSupportingModels_.insert(lm.id);
        }

        modelToGgufPath_[lm.id] = lm.ggufPath;
        models.push_back(info);
    }

    return models;
}

AgentResponse LlamaCppProvider::generate(const AgentRequest& request) {
    AgentResponse response;
    std::lock_guard<std::mutex> lock(getServerMutex());

    // 1. Ensure llama-server is active and serving the target model
    bool ready = LlamaServerDaemon::ensureServerRunning(request.model, serverUrl_, gpuLayers_, 4096, 25);
    if (!ready) {
        response.success = false;
        response.errorMessage = "Failed to launch or connect to llama.cpp server for model: " + request.model;
        return response;
    }

    // 2. Build OpenAI-compatible chat completion payload
    nlohmann::json payload;
    payload["model"] = request.model;
    payload["temperature"] = request.temperature;
    payload["max_tokens"] = request.maxTokens;
    payload["stream"] = false;

    nlohmann::json messagesArray = nlohmann::json::array();
    if (!request.systemPrompt.empty()) {
        messagesArray.push_back({
            {"role", "system"},
            {"content", request.systemPrompt}
        });
    }
    for (const auto& msg : request.messages) {
        messagesArray.push_back(msg.toJson());
    }
    if (messagesArray.empty() && !request.userPrompt.empty()) {
        messagesArray.push_back({
            {"role", "user"},
            {"content", request.userPrompt}
        });
    }

    bool modelHasTools = supportsTools(request.model);
    if (modelHasTools && !request.tools.empty()) {
        nlohmann::json toolsArray = nlohmann::json::array();
        for (const auto& t : request.tools) {
            toolsArray.push_back({
                {"type", "function"},
                {"function", {
                    {"name", t.name},
                    {"description", t.description},
                    {"parameters", t.parameters}
                }}
            });
        }
        payload["tools"] = toolsArray;
    } else if (!request.tools.empty()) {
        std::stringstream toolPrompt;
        toolPrompt << "\n\nAvailable Tools:\n";
        for (const auto& t : request.tools) {
            toolPrompt << "- " << t.name << ": " << t.description << "\n";
        }
        toolPrompt << "\nTo call a tool, return JSON: {\"name\": \"<tool_name>\", \"arguments\": { ... }}\n";
        if (!messagesArray.empty()) {
            messagesArray.back()["content"] = messagesArray.back()["content"].get<std::string>() + toolPrompt.str();
        }
    }
    payload["messages"] = messagesArray;

    // 3. Post to OpenAI chat completions endpoint
    std::string endpoint = serverUrl_ + "/v1/chat/completions";
    std::vector<std::string> headers = {"Content-Type: application/json"};
    HttpResponse httpResp = HttpClient::instance().post(
        endpoint,
        payload.dump(),
        headers,
        180
    );

    // Fallback if tools array was rejected by template
    if (!httpResp.success && payload.contains("tools")) {
        Logger::instance().warn("[LLAMA-CPP] Model rejected tools parameter. Retrying with prompt injection...");
        payload.erase("tools");
        httpResp = HttpClient::instance().post(
            endpoint,
            payload.dump(),
            headers,
            180
        );
    }

    if (!httpResp.success) {
        response.success = false;
        response.errorMessage = "llama.cpp request failed (" + std::to_string(httpResp.statusCode) + "): " + httpResp.error;
        return response;
    }

    // 4. Parse OpenAI response format
    nlohmann::json respJson = nlohmann::json::parse(httpResp.body, nullptr, false);
    if (respJson.is_discarded()) {
        response.success = false;
        response.errorMessage = "Failed to parse llama.cpp JSON response";
        return response;
    }

    response.success = true;
    if (respJson.contains("usage")) {
        response.promptTokens = respJson["usage"].value("prompt_tokens", 0);
        response.completionTokens = respJson["usage"].value("completion_tokens", 0);
    }

    if (respJson.contains("choices") && respJson["choices"].is_array() && !respJson["choices"].empty()) {
        auto choice = respJson["choices"][0];
        if (choice.contains("message")) {
            auto msg = choice["message"];
            response.content = msg.value("content", "");

            // Parse reasoning / thinking tokens (e.g. DeepSeek-R1 <think>...</think>)
            if (msg.contains("reasoning_content")) {
                response.thinking = msg.value("reasoning_content", "");
            } else if (!response.content.empty()) {
                size_t ts = response.content.find("<think>");
                size_t te = response.content.find("</think>");
                if (ts != std::string::npos && te != std::string::npos && te > ts) {
                    response.thinking = response.content.substr(ts + 7, te - (ts + 7));
                    response.content = response.content.substr(te + 8);
                    // Trim leading whitespace/newlines
                    size_t start = response.content.find_first_not_of(" \r\n\t");
                    if (start != std::string::npos) {
                        response.content = response.content.substr(start);
                    }
                }
            }

            // Parse native tool calls
            if (msg.contains("tool_calls") && msg["tool_calls"].is_array()) {
                for (const auto& tc : msg["tool_calls"]) {
                    if (tc.contains("function")) {
                        ToolCall call;
                        call.id = tc.value("id", "call_0");
                        call.name = tc["function"].value("name", "");
                        std::string argStr = tc["function"].value("arguments", "{}");
                        try {
                            call.arguments = nlohmann::json::parse(argStr);
                        } catch (...) {
                            call.arguments = nlohmann::json::object();
                        }
                        response.toolCalls.push_back(call);
                    }
                }
            }
        }
    }

    // If no native tool calls returned, check for Markdown JSON tool calls in text content
    if (response.toolCalls.empty() && !response.content.empty()) {
        response.toolCalls = extractToolCallsFromText(response.content);
    }

    return response;
}

} // namespace arenafight

