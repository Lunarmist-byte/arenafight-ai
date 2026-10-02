#pragma once

#include "arenafight/providers/provider.hpp"

namespace arenafight {

class ClaudeCodeProvider : public ModelProvider {
public:
    explicit ClaudeCodeProvider(std::string claudeCommand = "claude");

    std::string name() const override { return "ClaudeCode"; }

    bool available() override;

    std::vector<ModelInfo> discoverModels() override;

    AgentResponse generate(const AgentRequest& request) override;

    void setClaudeCommand(const std::string& cmd) { claudeCommand_ = cmd; }
    const std::string& getClaudeCommand() const { return claudeCommand_; }

    std::string getDetectedVersion() const { return detectedVersion_; }
    bool isInstalled() const { return isInstalled_; }
    std::string getStatusMessage() const { return statusMessage_; }

private:
    std::string claudeCommand_;
    std::string detectedVersion_;
    bool checkedAvailability_ = false;
    bool isInstalled_ = false;
    bool isAvailable_ = false;
    std::string statusMessage_;

    void checkVersion();
};

} // namespace arenafight
