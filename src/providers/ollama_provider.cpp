#include "arenafight/providers/ollama_provider.hpp"
#include "arenafight/common/http_client.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>

namespace arenafight {

OllamaProvider::OllamaProvider(std::string baseUrl)
    : baseUrl_(std::move(baseUrl)) {}

bool OllamaProvider::available() {
    try {
        HttpResponse resp = HttpClient::instance().get(baseUrl_ + "/api/tags", {}, 3);
        return resp.success;
    } catch (...) {
        return false;
    }
}

std::vector<ModelInfo> OllamaProvider::discoverModels() {
    std::vector<ModelInfo> models;
    try {
        HttpResponse resp = HttpClient::instance().get(baseUrl_ + "/api/tags", {}, 5);
        if (!resp.success) {
            return models;
        }

        nlohmann::json j = nlohmann::json::parse(resp.body);
        if (j.contains("models") && j["models"].is_array()) {
            for (const auto& item : j["models"]) {
                ModelInfo info;
                info.id = item.value("name", "");
                info.name = info.id;
                info.provider = "Ollama";
                info.isLocal = true;
                info.contextWindow = 8192; // Default Ollama context window

                std::string detailsStr;
                if (item.contains("details") && item["details"].is_object()) {
                    auto details = item["details"];
                    std::string paramSize = details.value("parameter_size", "");
                    std::string quant = details.value("quantization_level", "");
                    std::string family = details.value("family", "");
                    detailsStr = family + " " + paramSize + " " + quant;

                    // Capability estimation based on parameter size
                    if (paramSize.find("70B") != std::string::npos || paramSize.find("72B") != std::string::npos) {
                        info.capabilityScore = 2.5f;
                        info.contextWindow = 16384;
                    } else if (paramSize.find("32B") != std::string::npos || paramSize.find("34B") != std::string::npos) {
                        info.capabilityScore = 2.0f;
                    } else if (paramSize.find("14B") != std::string::npos || paramSize.find("13B") != std::string::npos) {
                        info.capabilityScore = 1.6f;
                    } else if (paramSize.find("7B") != std::string::npos || paramSize.find("8B") != std::string::npos) {
                        info.capabilityScore = 1.3f;
                    } else {
                        info.capabilityScore = 1.0f;
                    }
                }
                info.description = "Local Ollama model: " + info.name + " (" + detailsStr + ")";
                models.push_back(info);
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().debug("Ollama model discovery failed: " + std::string(e.what()));
    }
    return models;
}

AgentResponse OllamaProvider::generate(const AgentRequest& request) {
    // Hardware awareness: Sequential local model execution to protect 8GB VRAM
    std::lock_guard<std::mutex> lock(getLocalGpuMutex());

    AgentResponse response;
    try {
        nlohmann::json payload;
        payload["model"] = request.model;
        payload["stream"] = false;
        payload["options"] = {
            {"temperature", request.temperature},
            {"num_predict", request.maxTokens}
        };

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

        std::vector<std::string> headers = {"Content-Type: application/json"};
        HttpResponse httpResp = HttpClient::instance().post(
            baseUrl_ + "/api/chat",
            payload.dump(),
            headers,
            180
        );

        if (!httpResp.success) {
            response.success = false;
            response.errorMessage = "Ollama request failed: " + httpResp.error;
            return response;
        }

        nlohmann::json respJson = nlohmann::json::parse(httpResp.body);
        response.success = true;
        response.promptTokens = respJson.value("prompt_eval_count", 0);
        response.completionTokens = respJson.value("eval_count", 0);

        if (respJson.contains("message")) {
            auto msg = respJson["message"];
            response.content = msg.value("content", "");

            // Native tool calls
            if (msg.contains("tool_calls") && msg["tool_calls"].is_array()) {
                for (const auto& tcJson : msg["tool_calls"]) {
                    ToolCall tc;
                    tc.id = tcJson.value("id", "call_" + std::to_string(response.toolCalls.size() + 1));
                    if (tcJson.contains("function")) {
                        tc.name = tcJson["function"].value("name", "");
                        tc.arguments = tcJson["function"].value("arguments", nlohmann::json::object());
                    }
                    if (!tc.name.empty()) {
                        response.toolCalls.push_back(tc);
                    }
                }
            }
        }

        // Fallback: check if tool call is written in markdown code block
        if (response.toolCalls.empty() && !response.content.empty()) {
            auto extracted = extractToolCallsFromText(response.content);
            if (!extracted.empty()) {
                response.toolCalls = extracted;
            }
        }

    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = "Ollama exception: " + std::string(e.what());
    }

    return response;
}

} // namespace arenafight
