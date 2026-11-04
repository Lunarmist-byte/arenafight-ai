#include "arenafight/common/config.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace arenafight {

nlohmann::json Config::toJson() const {
  return {
      {"ollama_url", ollama_url},
      {"llama_server_url", llama_server_url},
      {"llama_gpu_layers", llama_gpu_layers},
      {"llama_threads", llama_threads},
      {"openrouter_api_key", openrouter_api_key.empty() ? "" : "[REDACTED]"},
      {"openrouter_base_url", openrouter_base_url},
      {"openrouter_models", openrouter_models},
      {"claude_command", claude_command},
      {"anthropic_api_key", anthropic_api_key.empty() ? "" : "[REDACTED]"},
      {"opencode_command", opencode_command},
      {"opencode_model", opencode_model},
      {"max_iterations", max_iterations},
      {"max_tool_calls", max_tool_calls},
      {"max_retries", max_retries},
      {"context_limit", context_limit},
      {"default_temperature", default_temperature},
      {"default_max_tokens", default_max_tokens},
      {"default_model", default_model},
      {"openai_base_url", openai_base_url},
      {"openai_api_key", openai_api_key.empty() ? "" : "[REDACTED]"},
      {"parallel_models", parallel_models},
      {"require_confirmation_for_destructive_commands",
       require_confirmation_for_destructive_commands},
      {"default_provider", default_provider},
      {"workspace_dir", workspace_dir},
      {"test_mock_enabled", test_mock_enabled},
      {"auto_mock_fallback", auto_mock_fallback},
      {"mode", mode},
      {"processing_level", processing_level},
      {"security_level", security_level},
      {"sandbox_enabled", sandbox_enabled},
      {"secret_masking_enabled", secret_masking_enabled},
      {"path_traversal_protection", path_traversal_protection},
      {"prompt_injection_guard", prompt_injection_guard},
      {"use_cuda", use_cuda},
      {"force_cpu", force_cpu}};
}

Config Config::fromJson(const nlohmann::json &j) {
  Config cfg;
  cfg.ollama_url = j.value("ollama_url", cfg.ollama_url);
  cfg.llama_server_url = j.value("llama_server_url", cfg.llama_server_url);
  cfg.llama_gpu_layers = j.value("llama_gpu_layers", cfg.llama_gpu_layers);
  cfg.llama_threads = j.value("llama_threads", cfg.llama_threads);
  if (j.contains("openrouter_api_key")) {
    std::string key = j["openrouter_api_key"].get<std::string>();
    if (key != "[REDACTED]") {
      cfg.openrouter_api_key = key;
    }
  }
  cfg.openrouter_base_url =
      j.value("openrouter_base_url", cfg.openrouter_base_url);
  if (j.contains("openrouter_models") && j["openrouter_models"].is_array()) {
    cfg.openrouter_models =
        j["openrouter_models"].get<std::vector<std::string>>();
  }
  cfg.claude_command = j.value("claude_command", cfg.claude_command);
  if (j.contains("anthropic_api_key")) {
    std::string aKey = j["anthropic_api_key"].get<std::string>();
    if (aKey != "[REDACTED]") {
      cfg.anthropic_api_key = aKey;
    }
  }
  cfg.opencode_command = j.value("opencode_command", cfg.opencode_command);
  cfg.opencode_model = j.value("opencode_model", cfg.opencode_model);
  cfg.max_iterations = j.value("max_iterations", cfg.max_iterations);
  cfg.max_tool_calls = j.value("max_tool_calls", cfg.max_tool_calls);
  cfg.max_retries = j.value("max_retries", cfg.max_retries);
  cfg.context_limit = j.value("context_limit", cfg.context_limit);
  cfg.default_temperature =
      j.value("default_temperature", cfg.default_temperature);
  if (j.contains("temperature")) {
    cfg.default_temperature = j["temperature"].get<float>();
  }
  cfg.default_max_tokens =
      j.value("default_max_tokens", cfg.default_max_tokens);
  if (j.contains("max_tokens")) {
    cfg.default_max_tokens = j["max_tokens"].get<int>();
  }
  cfg.default_model = j.value("default_model", cfg.default_model);
  if (j.contains("model")) {
    cfg.default_model = j["model"].get<std::string>();
  }
  cfg.openai_base_url = j.value("openai_base_url", cfg.openai_base_url);
  if (j.contains("openai_api_key")) {
    std::string oKey = j["openai_api_key"].get<std::string>();
    if (oKey != "[REDACTED]") {
      cfg.openai_api_key = oKey;
    }
  }
  cfg.parallel_models = j.value("parallel_models", cfg.parallel_models);
  cfg.require_confirmation_for_destructive_commands =
      j.value("require_confirmation_for_destructive_commands",
              cfg.require_confirmation_for_destructive_commands);
  cfg.default_provider = j.value("default_provider", cfg.default_provider);
  cfg.workspace_dir = j.value("workspace_dir", cfg.workspace_dir);
  cfg.test_mock_enabled = j.value("test_mock_enabled", cfg.test_mock_enabled);
  cfg.auto_mock_fallback =
      j.value("auto_mock_fallback", cfg.auto_mock_fallback);
  if (j.contains("mode")) {
    cfg.setMode(j["mode"].get<std::string>());
  } else if (j.contains("runtime_mode")) {
    cfg.setMode(j["runtime_mode"].get<std::string>());
  }
  if (j.contains("processing_level")) {
    cfg.setProcessingLevel(j["processing_level"].get<std::string>());
  } else if (j.contains("level")) {
    cfg.setProcessingLevel(j["level"].get<std::string>());
  }
  if (j.contains("security_level")) {
    cfg.setSecurityLevel(j["security_level"].get<std::string>());
  }
  cfg.sandbox_enabled = j.value("sandbox_enabled", cfg.sandbox_enabled);
  cfg.secret_masking_enabled =
      j.value("secret_masking_enabled", cfg.secret_masking_enabled);
  cfg.path_traversal_protection =
      j.value("path_traversal_protection", cfg.path_traversal_protection);
  cfg.prompt_injection_guard =
      j.value("prompt_injection_guard", cfg.prompt_injection_guard);
  cfg.use_cuda = j.value("use_cuda", cfg.use_cuda);
  cfg.force_cpu = j.value("force_cpu", cfg.force_cpu);
  return cfg;
}

Config Config::loadFromFile(const std::string &path) {
  Config cfg;
  std::ifstream file(path);
  if (file.is_open()) {
    try {
      nlohmann::json j = nlohmann::json::parse(file);
      cfg = fromJson(j);
    } catch (const std::exception &e) {
      std::cerr << "Warning: Failed to parse config file " << path << ": "
                << e.what() << std::endl;
    }
  }
  cfg.applyEnvOverrides();
  return cfg;
}

bool Config::saveToFile(const std::string &path) const {
  std::ofstream file(path);
  if (!file.is_open())
    return false;
  nlohmann::json j = toJson();
  if (!openrouter_api_key.empty()) {
    j["openrouter_api_key"] = openrouter_api_key;
  }
  if (!openai_api_key.empty()) {
    j["openai_api_key"] = openai_api_key;
  }
  if (!anthropic_api_key.empty()) {
    j["anthropic_api_key"] = anthropic_api_key;
  }
  file << j.dump(4);
  return true;
}

void Config::applyEnvOverrides() {
  if (const char *key = std::getenv("OPENROUTER_API_KEY")) {
    if (key[0] != '\0') {
      openrouter_api_key = key;
    }
  }
  if (const char *models = std::getenv("OPENROUTER_MODELS")) {
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
  if (const char *url = std::getenv("OLLAMA_URL")) {
    if (url[0] != '\0') {
      ollama_url = url;
    }
  }
  if (const char *cmd = std::getenv("CLAUDE_COMMAND")) {
    if (cmd[0] != '\0') {
      claude_command = cmd;
    }
  }
  if (const char *aKey = std::getenv("ANTHROPIC_API_KEY")) {
    if (aKey[0] != '\0') {
      anthropic_api_key = aKey;
    }
  }
  if (const char *ocCmd = std::getenv("OPENCODE_COMMAND")) {
    if (ocCmd[0] != '\0') {
      opencode_command = ocCmd;
    }
  }
  if (const char *ocModel = std::getenv("OPENCODE_MODEL")) {
    if (ocModel[0] != '\0') {
      opencode_model = ocModel;
    }
  }
  if (const char *tempEnv = std::getenv("ARENA_TEMPERATURE")) {
    if (tempEnv[0] != '\0') {
      try {
        default_temperature = std::stof(tempEnv);
      } catch (...) {
      }
    }
  }
  if (const char *tokEnv = std::getenv("ARENA_MAX_TOKENS")) {
    if (tokEnv[0] != '\0') {
      try {
        default_max_tokens = std::stoi(tokEnv);
      } catch (...) {
      }
    }
  }
  if (const char *mEnv = std::getenv("ARENA_MODEL")) {
    if (mEnv[0] != '\0') {
      default_model = mEnv;
    }
  } else if (const char *dmEnv = std::getenv("DEFAULT_MODEL")) {
    if (dmEnv[0] != '\0') {
      default_model = dmEnv;
    }
  }
  if (const char *oUrl = std::getenv("OPENAI_BASE_URL")) {
    if (oUrl[0] != '\0') {
      openai_base_url = oUrl;
    }
  } else if (const char *oBase = std::getenv("OPENAI_API_BASE")) {
    if (oBase[0] != '\0') {
      openai_base_url = oBase;
    }
  }
  if (const char *oKey = std::getenv("OPENAI_API_KEY")) {
    if (oKey[0] != '\0') {
      openai_api_key = oKey;
    }
  }
  if (const char *fb = std::getenv("ARENA_AUTO_MOCK_FALLBACK")) {
    if (std::string(fb) == "0" || std::string(fb) == "false") {
      auto_mock_fallback = false;
    } else if (std::string(fb) == "1" || std::string(fb) == "true") {
      auto_mock_fallback = true;
    }
  }
  if (const char *modeEnv = std::getenv("ARENA_MODE")) {
    if (modeEnv[0] != '\0') {
      setMode(modeEnv);
    }
  }
  if (const char *secEnv = std::getenv("ARENA_SECURITY_LEVEL")) {
    if (secEnv[0] != '\0') {
      setSecurityLevel(secEnv);
    }
  }
  if (const char *lvlEnv = std::getenv("ARENA_PROCESSING_LEVEL")) {
    if (lvlEnv[0] != '\0') {
      setProcessingLevel(lvlEnv);
    }
  } else if (const char *lvlEnv2 = std::getenv("ARENA_LEVEL")) {
    if (lvlEnv2[0] != '\0') {
      setProcessingLevel(lvlEnv2);
    }
  }
  if (const char *cudaEnv = std::getenv("ARENA_CUDA")) {
    if (std::string(cudaEnv) == "0" || std::string(cudaEnv) == "false") {
      setCuda(false);
    } else if (std::string(cudaEnv) == "1" || std::string(cudaEnv) == "true") {
      setCuda(true);
    }
  }
  if (const char *cpuEnv = std::getenv("ARENA_CPU")) {
    if (std::string(cpuEnv) == "1" || std::string(cpuEnv) == "true") {
      setForceCpu(true);
    }
  }
  if (const char *llamaUrl = std::getenv("LLAMA_SERVER_URL")) {
    if (llamaUrl[0] != '\0') {
      llama_server_url = llamaUrl;
    }
  }
  if (const char *layersEnv = std::getenv("LLAMA_GPU_LAYERS")) {
    if (layersEnv[0] != '\0') {
      try {
        llama_gpu_layers = std::stoi(layersEnv);
      } catch (...) {
      }
    }
  }
}

} // namespace arenafight
