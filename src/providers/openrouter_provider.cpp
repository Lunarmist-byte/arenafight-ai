#include "arenafight/providers/openrouter_provider.hpp"
#include "arenafight/common/http_client.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>

namespace arenafight {

OpenRouterProvider::OpenRouterProvider(
    std::string apiKey,
    std::string baseUrl,
    std::vector<std::string> configuredModels
)
    : apiKey_(std::move(apiKey)),
      baseUrl_(std::move(baseUrl)),
      configuredModels_(std::move(configuredModels)) {}

bool OpenRouterProvider::available() {
    return !apiKey_.empty();
}

std::vector<ModelInfo> OpenRouterProvider::discoverModels() {
    std::vector<ModelInfo> models;
    if (apiKey_.empty()) {
        return models;
    }

    // First include any explicitly configured models from OPENROUTER_MODELS
    for (const auto& modelId : configuredModels_) {
        ModelInfo info;
        info.id = modelId;
        info.name = modelId;
        info.provider = "OpenRouter";
        info.contextWindow = 128000;
        info.isLocal = false;
        info.description = "Configured OpenRouter model: " + modelId;
        info.capabilityScore = 2.8f;
        models.push_back(info);
    }

    // Try discovering catalog from OpenRouter models API
    try {
        std::vector<std::string> headers = {
            "Authorization: Bearer " + apiKey_,
            "HTTP-Referer: https://github.com/Lunarmist-byte/arenafight-ai",
            "X-Title: ArenaFight Agent"
        };
        HttpResponse resp = HttpClient::instance().get(baseUrl_ + "/models", headers, 5);
        if (resp.success) {
            nlohmann::json j = nlohmann::json::parse(resp.body);
            if (j.contains("data") && j["data"].is_array()) {
                for (const auto& item : j["data"]) {
                    std::string id = item.value("id", "");
                    // Avoid duplicating configured models
                    bool alreadyAdded = false;
                    for (const auto& m : models) {
                        if (m.id == id) {
                            alreadyAdded = true;
                            break;
                        }
                    }
                    if (alreadyAdded) continue;

                    ModelInfo info;
                    info.id = id;
                    info.name = item.value("name", id);
                    info.provider = "OpenRouter";
                    info.contextWindow = item.value("context_length", static_cast<size_t>(128000));
                    info.isLocal = false;
                    info.description = item.value("description", "");
                    info.capabilityScore = 2.5f;

                    // Only take notable/well-known models or up to 30 models to keep catalog manageable
                    if (id.find("claude") != std::string::npos ||
                        id.find("gpt-4") != std::string::npos ||
                        id.find("gemini") != std::string::npos ||
                        id.find("deepseek") != std::string::npos ||
                        id.find("qwen") != std::string::npos) {
                        models.push_back(info);
                    }
                    if (models.size() >= 40) break;
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().debug("OpenRouter model catalog fetch failed: " + std::string(e.what()));
    }

    // If no models found yet but key is present, provide high-quality standard catalog
    if (models.empty()) {
        std::vector<std::pair<std::string, size_t>> defaults = {
            {"anthropic/claude-3.7-sonnet", 200000},
            {"anthropic/claude-3.5-sonnet", 200000},
            {"deepseek/deepseek-r1", 128000},
            {"deepseek/deepseek-chat", 128000},
            {"google/gemini-2.0-flash-001", 1000000},
            {"meta-llama/llama-3.3-70b-instruct", 128000},
            {"qwen/qwen-2.5-coder-32b-instruct", 128000}
        };
        for (const auto& [id, ctx] : defaults) {
            ModelInfo info;
            info.id = id;
            info.name = id;
            info.provider = "OpenRouter";
            info.contextWindow = ctx;
            info.isLocal = false;
            info.description = "OpenRouter standard model: " + id;
            info.capabilityScore = 2.7f;
            models.push_back(info);
        }
    }

    return models;
}

AgentResponse OpenRouterProvider::generate(const AgentRequest& request) {
    AgentResponse response;
    if (apiKey_.empty()) {
        response.success = false;
        response.errorMessage = "OpenRouter API key is not configured";
        return response;
    }

    try {
        nlohmann::json payload;
        payload["model"] = request.model.empty() ? "anthropic/claude-3.5-sonnet" : request.model;
        payload["temperature"] = request.temperature;
        payload["max_tokens"] = request.maxTokens;

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
        payload["messages"] = messagesArray;

        if (!request.tools.empty()) {
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
        }

        std::vector<std::string> headers = {
            "Content-Type: application/json",
            "Authorization: Bearer " + apiKey_,
            "HTTP-Referer: https://github.com/Lunarmist-byte/arenafight-ai",
            "X-Title: ArenaFight Agent"
        };

        HttpResponse httpResp = HttpClient::instance().post(
            baseUrl_ + "/chat/completions",
            payload.dump(),
            headers,
            120
        );

        if (!httpResp.success) {
            response.success = false;
            response.errorMessage = Logger::sanitize("OpenRouter HTTP error: " + httpResp.error);
            return response;
        }

        nlohmann::json respJson = nlohmann::json::parse(httpResp.body);
        if (respJson.contains("error")) {
            response.success = false;
            response.errorMessage = Logger::sanitize(respJson["error"].value("message", "Unknown error"));
            return response;
        }

        response.success = true;
        if (respJson.contains("usage")) {
            response.promptTokens = respJson["usage"].value("prompt_tokens", 0);
            response.completionTokens = respJson["usage"].value("completion_tokens", 0);
        }

        if (respJson.contains("choices") && respJson["choices"].is_array() && !respJson["choices"].empty()) {
            auto choice = respJson["choices"][0];
            response.finishReason = choice.value("finish_reason", "");
            if (choice.contains("message")) {
                auto msg = choice["message"];
                response.content = msg.value("content", "");

                // Check native tool calls
                if (msg.contains("tool_calls") && msg["tool_calls"].is_array()) {
                    for (const auto& tcJson : msg["tool_calls"]) {
                        ToolCall tc;
                        tc.id = tcJson.value("id", "call_" + std::to_string(response.toolCalls.size() + 1));
                        if (tcJson.contains("function")) {
                            tc.name = tcJson["function"].value("name", "");
                            std::string argsStr = tcJson["function"].value("arguments", "{}");
                            try {
                                tc.arguments = nlohmann::json::parse(argsStr);
                            } catch (...) {
                                tc.arguments = argsStr;
                            }
                        }
                        if (!tc.name.empty()) {
                            response.toolCalls.push_back(tc);
                        }
                    }
                }
            }
        }

        // Fallback: check text for tool call patterns
        if (response.toolCalls.empty() && !response.content.empty()) {
            auto extracted = extractToolCallsFromText(response.content);
            if (!extracted.empty()) {
                response.toolCalls = extracted;
            }
        }

    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = Logger::sanitize("OpenRouter exception: " + std::string(e.what()));
    }

    return response;
}

} // namespace arenafight
