#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace arenafight {

struct Config {
    std::string ollama_url = "http://localhost:11434";
    std::string openrouter_api_key = "";
    std::string openrouter_base_url = "https://openrouter.ai/api/v1";
    std::vector<std::string> openrouter_models;
    std::string claude_command = "claude";
    int max_iterations = 30;
    int max_tool_calls = 100;
    int max_retries = 3;
    int context_limit = 32000;
    bool parallel_models = false; // RTX 4060 8GB VRAM safety: sequential execution by default
    bool require_confirmation_for_destructive_commands = true;
    std::string default_provider = "auto";
    std::string workspace_dir = ".";
    bool test_mock_enabled = false;

    // Serialization
    nlohmann::json toJson() const;
    static Config fromJson(const nlohmann::json& j);

    // Load / Save
    static Config loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;

    // Environment overrides
    void applyEnvOverrides();
};

} // namespace arenafight
