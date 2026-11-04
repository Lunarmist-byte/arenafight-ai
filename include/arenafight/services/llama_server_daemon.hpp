#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

struct LocalGgufModel {
    std::string id;              // e.g. "deepseek-r1:8b"
    std::string name;            // display name
    std::string ggufPath;        // absolute path to the .gguf file or blob
    uint64_t sizeBytes = 0;
    std::string parameterSize;   // e.g. "8.2B"
    std::string quantization;    // e.g. "Q4_K_M"
    std::string family;          // e.g. "qwen3"
    int contextWindow = 8192;
    bool isOllamaImported = false;
};

class LlamaServerDaemon {
public:
    // Discover all local GGUF models: Ollama manifests & blobs, plus local *.gguf
    static std::vector<LocalGgufModel> discoverModels();

    // Resolve model name/id (e.g. "deepseek-r1:8b" or "qwen2.5-coder:7b") to absolute GGUF path
    static std::string resolveModelGgufPath(const std::string& modelId);

    // Locate llama-server executable on Windows or Linux
    static std::string findLlamaServerExecutable();

    // Check if llama-server is responding at the specified base URL
    static bool isServerResponding(const std::string& baseUrl = "http://127.0.0.1:8080");

    // Get the model ID currently loaded in llama-server
    static std::string getLoadedModel(const std::string& baseUrl = "http://127.0.0.1:8080");

    // Ensure llama-server is running with the specified model loaded
    static bool ensureServerRunning(
        const std::string& modelIdOrGguf,
        const std::string& baseUrl = "http://127.0.0.1:8080",
        int gpuLayers = 99,
        int contextSize = 4096,
        int timeoutSeconds = 25
    );

    // Stop currently running llama-server background process if spawned
    static bool stopServer();

    // Autonomously pull an Ollama model if missing (downloads GGUF blob into Ollama store)
    static bool pullOllamaModel(const std::string& modelName, int timeoutSeconds = 180);

    // Helper to find the Ollama models directory root
    static std::filesystem::path getOllamaModelsDir();
};

} // namespace arenafight

