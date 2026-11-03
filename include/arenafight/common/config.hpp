#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace arenafight {

struct Config {
  std::string ollama_url = "http://localhost:11434";
  std::string openrouter_api_key = "";
  std::string openrouter_base_url = "https://openrouter.ai/api/v1";
  std::vector<std::string> openrouter_models;
  std::string claude_command = "claude";
  std::string anthropic_api_key = "";
  std::string opencode_command = "opencode";
  std::string opencode_model = "opencode-default";
  int max_iterations = 30;
  int max_tool_calls = 100;
  int max_retries = 3;
  int context_limit = 32000;
  float default_temperature = 0.2f;
  int default_max_tokens = 4096;
  std::string default_model = "";
  std::string openai_base_url = "";
  std::string openai_api_key = "";
  bool parallel_models = false;
  bool require_confirmation_for_destructive_commands = true;
  std::string default_provider = "auto";
  std::string workspace_dir = ".";
  bool test_mock_enabled = false;
  bool auto_mock_fallback = true;
  std::string mode = "standalone";         // "standalone" or "opencode"
  std::string processing_level = "fast";   // "fast" or "perfect_boi"
  std::string security_level = "balanced"; // "strict", "balanced", "permissive"
  bool sandbox_enabled = true;
  bool secret_masking_enabled = true;
  bool path_traversal_protection = true;
  bool prompt_injection_guard = true;
  bool use_cuda = true;   // Prefer NVIDIA CUDA acceleration when available
  bool force_cpu = false; // Force CPU execution (slow variant mode)

  // Hardware acceleration helpers
  bool isCudaPreferred() const { return use_cuda && !force_cpu; }
  void setForceCpu(bool force) {
    force_cpu = force;
    if (force)
      use_cuda = false;
  }
  void setCuda(bool enable) {
    use_cuda = enable;
    if (enable)
      force_cpu = false;
  }

  // Processing level helpers
  bool isFast() const { return processing_level == "fast"; }
  bool isPerfectBoi() const {
    return processing_level == "perfect" || processing_level == "perfect_boi" ||
           processing_level == "perfect-boi";
  }
  void setProcessingLevel(const std::string &lvl) {
    if (lvl == "perfect" || lvl == "perfect_boi" || lvl == "perfect-boi") {
      processing_level = "perfect_boi";
    } else {
      processing_level = "fast";
    }
  }

  // Security helpers
  bool isStrictSecurity() const { return security_level == "strict"; }
  void setSecurityLevel(const std::string &lvl) {
    if (lvl == "strict" || lvl == "permissive" || lvl == "balanced") {
      security_level = lvl;
    } else {
      security_level = "balanced";
    }
  }

  // Mode helpers
  bool isStandalone() const { return mode != "opencode"; }
  bool isOpenCodeConnected() const { return mode == "opencode"; }
  void setMode(const std::string &newMode) {
    if (newMode == "opencode" || newMode == "connected" ||
        newMode == "open_code") {
      mode = "opencode";
    } else {
      mode = "standalone";
    }
  }

  // Serialization
  nlohmann::json toJson() const;
  static Config fromJson(const nlohmann::json &j);

  // Load / Save
  static Config loadFromFile(const std::string &path);
  bool saveToFile(const std::string &path) const;

  // Environment overrides
  void applyEnvOverrides();
};

} // namespace arenafight
