#include "arenafight/providers/provider_factory.hpp"
#include "arenafight/providers/llama_cpp_provider.hpp"
#include "arenafight/providers/ollama_provider.hpp"
#include "arenafight/providers/openrouter_provider.hpp"
#include "arenafight/providers/claude_code_provider.hpp"
#include "arenafight/providers/opencode_provider.hpp"
#include "arenafight/providers/test_mock_provider.hpp"
#include <cstdlib>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

std::vector<std::shared_ptr<ModelProvider>> ProviderFactory::createConfiguredProviders(const Config& config) {
    std::vector<std::shared_ptr<ModelProvider>> providers;

    // 1. llama.cpp Provider (Primary native local provider, loads Ollama models directly via GGUF)
    int gpuLayers = config.force_cpu ? 0 : config.llama_gpu_layers;
    providers.push_back(std::make_shared<LlamaCppProvider>(
        config.llama_server_url,
        gpuLayers
    ));

    // 2. Ollama Provider (Local daemon fallback)
    providers.push_back(std::make_shared<OllamaProvider>(config.ollama_url));

    // 3. OpenRouter Provider (Cloud multi-model)
    providers.push_back(std::make_shared<OpenRouterProvider>(
        config.openrouter_api_key,
        config.openrouter_base_url,
        config.openrouter_models
    ));

    // 4. Claude Code Provider (CLI)
    if (!config.anthropic_api_key.empty()) {
#ifdef _WIN32
        _putenv_s("ANTHROPIC_API_KEY", config.anthropic_api_key.c_str());
#else
        setenv("ANTHROPIC_API_KEY", config.anthropic_api_key.c_str(), 1);
#endif
    }
    providers.push_back(std::make_shared<ClaudeCodeProvider>(config.claude_command));

    // 5. OpenCode Provider (anomalyco/opencode CLI)
    providers.push_back(std::make_shared<OpenCodeProvider>(
        config.opencode_command,
        config.opencode_model
    ));

    // 6. Test Mock Provider (Deterministic simulation for testing)
    if (config.test_mock_enabled) {
        providers.push_back(std::make_shared<TestMockProvider>());
    }

    return providers;
}

} // namespace arenafight

