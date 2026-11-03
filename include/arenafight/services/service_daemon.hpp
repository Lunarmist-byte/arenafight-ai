#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace arenafight {

// Autonomous Service Daemon for detecting and launching local AI services (Ollama, etc.)
// on Windows and Linux without prompting the user.
class ServiceDaemon {
public:
    // Check if Ollama is actively responding at the specified base URL
    static bool isOllamaResponding(const std::string& baseUrl = "http://localhost:11434");

    // Automatically locate the Ollama binary on Windows or Linux
    static std::string findOllamaExecutable();

    // Launch Ollama as an autonomous detached background process
    static bool launchOllamaBackground(const std::string& exePath = "");

    // Ensure Ollama is running: checks status, discovers binary, starts daemon, and waits for health
    static bool ensureOllamaRunning(
        const std::string& baseUrl = "http://localhost:11434",
        int timeoutSeconds = 12
    );

    // Discover models currently installed and available in Ollama
    static std::vector<std::string> getInstalledOllamaModels(
        const std::string& baseUrl = "http://localhost:11434"
    );

    // Autonomously pull an Ollama model if missing, without asking the user
    static bool ensureModelAvailable(
        const std::string& baseUrl,
        const std::string& modelName,
        int timeoutSeconds = 120
    );

    // Cross-platform autonomous starter for all local dependencies
    static bool autoStartLocalBackends(const std::string& ollamaUrl);
};

} // namespace arenafight

