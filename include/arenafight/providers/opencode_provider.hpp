#pragma once

#include "arenafight/providers/provider.hpp"
#include <string>
#include <vector>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

/**
 * @brief OpenCode Provider - Integrates OpenCode CLI (anomalyco/opencode)
 * 
 * Supports model-agnostic CLI execution, multi-model discovery, tool calling,
 * and workspace synchronization (.opencode config).
 */
class OpenCodeProvider : public ModelProvider {
public:
    explicit OpenCodeProvider(
        std::string opencodeCommand = "opencode",
        std::string defaultModel = "opencode-default"
    );

    std::string name() const override { return "OpenCode"; }

    bool available() override;

    std::vector<ModelInfo> discoverModels() override;

    AgentResponse generate(const AgentRequest& request) override;

    void setOpenCodeCommand(const std::string& cmd) {
        opencodeCommand_ = cmd;
        checkedAvailability_ = false;
    }
    const std::string& getOpenCodeCommand() const { return opencodeCommand_; }

    void setDefaultModel(const std::string& model) { defaultModel_ = model; }
    const std::string& getDefaultModel() const { return defaultModel_; }

    std::string getDetectedVersion() const { return detectedVersion_; }
    bool isInstalled() const { return isInstalled_; }
    std::string getStatusMessage() const { return statusMessage_; }

    // Mock mode for deterministic testing & offline simulation
    void setMockMode(bool enabled) {
        mockMode_ = enabled;
        checkedAvailability_ = false;
    }
    bool isMockMode() const { return mockMode_; }

    // OpenCode Workspace integration helpers
    static bool exportOpenCodeWorkspace(
        const std::string& workspaceDir,
        const std::string& taskObjective,
        const std::string& instructions
    );

    static std::string generateOpenCodeConfig(
        const std::string& projectName = "ArenaFight Agent"
    );

private:
    std::string opencodeCommand_;
    std::string defaultModel_;
    std::string detectedVersion_;
    bool checkedAvailability_ = false;
    bool isInstalled_ = false;
    bool isAvailable_ = false;
    std::string statusMessage_;
    bool mockMode_ = false;

    void checkVersion();
};

} // namespace arenafight

