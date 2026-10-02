#include "arenafight/common/config.hpp"
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <iostream>

namespace arenafight {

nlohmann::json Config::toJson() const {
    return {
        {"ollama_url", ollama_url},
        {"openrouter_api_key", openrouter_api_key.empty() ? "" : "[REDACTED]"},
        {"openrouter_base_url", openrouter_base_url},
        {"openrouter_models", openrouter_models},
        {"claude_command", claude_command},
        {"max_iterations", max_iterations},
        {"max_tool_calls", max_tool_calls},
        {"max_retries", max_retries},
        {"context_limit", context_limit},
        {"parallel_models", parallel_models},
        {"require_confirmation_for_destructive_commands", require_confirmation_for_destructive_commands},
        {"default_provider", default_provider},
        {"workspace_dir", workspace_dir},
        {"test_mock_enabled", test_mock_enabled}
    };
}

Config Config::fromJson(const nlohmann::json& j) {
    Config cfg;
    cfg.ollama_url = j.value("ollama_url", cfg.ollama_url);
    if (j.contains("openrouter_api_key")) {
        std::string key = j["openrouter_api_key"].get<std::string>();
        if (key != "[REDACTED]") {
            cfg.openrouter_api_key = key;
        }
    }
    cfg.openrouter_base_url = j.value("openrouter_base_url", cfg.openrouter_base_url);
    if (j.contains("openrouter_models") && j["openrouter_models"].is_array()) {
        cfg.openrouter_models = j["openrouter_models"].get<std::vector<std::string>>();
    }
    cfg.claude_command = j.value("claude_command", cfg.claude_command);
    cfg.max_iterations = j.value("max_iterations", cfg.max_iterations);
    cfg.max_tool_calls = j.value("max_tool_calls", cfg.max_tool_calls);
    cfg.max_retries = j.value("max_retries", cfg.max_retries);
    cfg.context_limit = j.value("context_limit", cfg.context_limit);
    cfg.parallel_models = j.value("parallel_models", cfg.parallel_models);
    cfg.require_confirmation_for_destructive_commands = j.value("require_confirmation_for_destructive_commands", cfg.require_confirmation_for_destructive_commands);
    cfg.default_provider = j.value("default_provider", cfg.default_provider);
    cfg.workspace_dir = j.value("workspace_dir", cfg.workspace_dir);
    cfg.test_mock_enabled = j.value("test_mock_enabled", cfg.test_mock_enabled);
    return cfg;
}

Config Config::loadFromFile(const std::string& path) {
    Config cfg;
    std::ifstream file(path);
    if (file.is_open()) {
        try {
            nlohmann::json j = nlohmann::json::parse(file);
            cfg = fromJson(j);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to parse config file " << path << ": " << e.what() << std::endl;
        }
    }
    cfg.applyEnvOverrides();
    return cfg;
}

bool Config::saveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    nlohmann::json j = toJson();
    if (!openrouter_api_key.empty()) {
        j["openrouter_api_key"] = openrouter_api_key;
    }
    file << j.dump(4);
    return true;
}

void Config::applyEnvOverrides() {
    if (const char* key = std::getenv("OPENROUTER_API_KEY")) {
        if (key[0] != '\0') {
            openrouter_api_key = key;
        }
    }
    if (const char* models = std::getenv("OPENROUTER_MODELS")) {
        if (models[0] != '\0') {
            openrouter_models.clear();
            std::stringstream ss(models);
            std::string item;
            while (std::getline(ss, item, ',')) {
                // Trim whitespace
                size_t first = item.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    size_t last = item.find_last_not_of(" \t");
                    openrouter_models.push_back(item.substr(first, (last - first + 1)));
                }
            }
        }
    }
    if (const char* url = std::getenv("OLLAMA_URL")) {
        if (url[0] != '\0') {
            ollama_url = url;
        }
    }
    if (const char* cmd = std::getenv("CLAUDE_COMMAND")) {
        if (cmd[0] != '\0') {
            claude_command = cmd;
        }
    }
}

} // namespace arenafight
