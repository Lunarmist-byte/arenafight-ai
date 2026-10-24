#include "arenafight/providers/ollama_provider.hpp"
#include "arenafight/common/http_client.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/common/process.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cctype>

namespace arenafight {

OllamaProvider::OllamaProvider(std::string baseUrl)
    : baseUrl_(std::move(baseUrl)) {}

bool OllamaProvider::supportsTools(const std::string& modelId) const {
    if (toolSupportingModels_.count(modelId) > 0) return true;
    std::string lower = modelId;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower.find("gemma") != std::string::npos) return false;
    if (lower.find("qwen") != std::string::npos ||
        lower.find("llama") != std::string::npos ||
        lower.find("mistral") != std::string::npos ||
        lower.find("coder") != std::string::npos) {
        return true;
    }
    return false;
}

bool OllamaProvider::tryAutoStartOllama() {
    ProcessResult chk = ProcessExecutor::executeShell("Get-Command ollama -ErrorAction SilentlyContinue", ".", 3000);
    if (chk.exitCode != 0 || chk.stdOut.empty()) {
        return false;
    }

    Logger::instance().info("Ollama is not responding. Attempting to start Ollama server in background...");

    if (ProcessExecutor::isWindows()) {
        ProcessExecutor::executeShell("Start-Process -FilePath 'ollama' -ArgumentList 'serve' -WindowStyle Hidden", ".", 3000);
    } else {
        ProcessExecutor::executeShell("nohup ollama serve > /dev/null 2>&1 &", ".", 3000);
    }

    for (int i = 0; i < 8; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        HttpResponse checkResp = HttpClient::instance().get(baseUrl_ + "/api/tags", {}, 1);
        if (checkResp.success) {
            Logger::instance().info("Ollama server successfully connected at " + baseUrl_);
            return true;
        }
    }
    return false;
}

bool OllamaProvider::available() {
    try {
        HttpResponse resp = HttpClient::instance().get(baseUrl_ + "/api/tags", {}, 2);
        if (resp.success) return true;

        if (baseUrl_.find("localhost") != std::string::npos) {
            std::string altUrl = baseUrl_;
            size_t pos = altUrl.find("localhost");
            altUrl.replace(pos, 9, "127.0.0.1");
            HttpResponse resp2 = HttpClient::instance().get(altUrl + "/api/tags", {}, 2);
            if (resp2.success) {
                baseUrl_ = altUrl;
                return true;
            }
        }
    } catch (...) {}

    if (tryAutoStartOllama()) {
        try {
            HttpResponse resp = HttpClient::instance().get(baseUrl_ + "/api/tags", {}, 2);
            return resp.success;
        } catch (...) {}
    }

    return false;
}

std::vector<ModelInfo> OllamaProvider::discoverModels() {
    std::vector<ModelInfo> models;
    try {
        HttpResponse resp = HttpClient::instance().get(baseUrl_ + "/api/tags", {}, 5);
        if (!resp.success && baseUrl_.find("localhost") != std::string::npos) {
            std::string altUrl = baseUrl_;
            size_t pos = altUrl.find("localhost");
            altUrl.replace(pos, 9, "127.0.0.1");
            resp = HttpClient::instance().get(altUrl + "/api/tags", {}, 5);
            if (resp.success) baseUrl_ = altUrl;
        }

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
                info.contextWindow = 8192;

                bool hasTools = false;
                bool hasThinking = false;
                if (item.contains("capabilities") && item["capabilities"].is_array()) {
                    for (const auto& cap : item["capabilities"]) {
                        std::string capStr = cap.get<std::string>();
                        if (capStr == "tools") hasTools = true;
                        if (capStr == "thinking") hasThinking = true;
                    }
                }
                if (hasTools) {
                    toolSupportingModels_.insert(info.id);
                }

                std::string detailsStr;
                if (item.contains("details") && item["details"].is_object()) {
                    auto details = item["details"];
                    std::string paramSize = details.value("parameter_size", "");
                    std::string quant = details.value("quantization_level", "");
                    std::string family = details.value("family", "");
                    detailsStr = family + " " + paramSize + " " + quant;

                    double sizeVal = 0.0;
                    try {
                        sizeVal = std::stod(paramSize);
                    } catch (...) {}

                    if (sizeVal >= 70.0) {
                        info.capabilityScore = 2.8f;
                        info.contextWindow = 16384;
                    } else if (sizeVal >= 30.0) {
                        info.capabilityScore = 2.4f;
                        info.contextWindow = 16384;
                    } else if (sizeVal >= 13.0) {
                        info.capabilityScore = 2.0f;
                    } else if (sizeVal >= 7.0) {
                        info.capabilityScore = 1.7f;
                    } else {
                        info.capabilityScore = 1.1f;
                    }

                    if (hasTools) info.capabilityScore += 0.5f;
                    if (hasThinking) info.capabilityScore += 0.3f;
                } else {
                    info.capabilityScore = 1.0f;
                }

                info.description = "Local Ollama model: " + info.name + " (" + detailsStr +
                                   (hasTools ? ", tools" : "") +
                                   (hasThinking ? ", thinking" : "") + ")";
                models.push_back(info);
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().debug("Ollama model discovery failed: " + std::string(e.what()));
    }
    return models;
}

AgentResponse OllamaProvider::generate(const AgentRequest& request) {
    // Hardware awareness: Sequential local model execution to protect GPU VRAM
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
            // Model does not natively support Ollama tools array: inject tool instructions as text
            std::stringstream toolPrompt;
            toolPrompt << "\n\nAvailable Tools:\n";
            for (const auto& t : request.tools) {
                toolPrompt << "- " << t.name << ": " << t.description << "\n";
            }
            toolPrompt << "\nTo call a tool, return JSON: {\"name\": \"<tool_name>\", \"arguments\": { ... }}\n";
            if (!messagesArray.empty()) {
                messagesArray.back()["content"] = messagesArray.back()["content"].get<std::string>() + toolPrompt.str();
                payload["messages"] = messagesArray;
            }
        }

        std::vector<std::string> headers = {"Content-Type: application/json"};
        HttpResponse httpResp = HttpClient::instance().post(
            baseUrl_ + "/api/chat",
            payload.dump(),
            headers,
            180
        );

        // Fallback retry if model rejected tools parameter
        if (!httpResp.success && httpResp.error.find("does not support tools") != std::string::npos && payload.contains("tools")) {
            Logger::instance().warn("Model " + request.model + " does not support native tools. Retrying without tools parameter...");
            payload.erase("tools");
            httpResp = HttpClient::instance().post(
                baseUrl_ + "/api/chat",
                payload.dump(),
                headers,
                180
            );
        }

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

        // Fallback: check if tool call is written in markdown code block or JSON text
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
