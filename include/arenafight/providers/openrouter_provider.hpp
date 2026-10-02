#pragma once

#include "arenafight/providers/provider.hpp"

namespace arenafight {

class OpenRouterProvider : public ModelProvider {
public:
    OpenRouterProvider(
        std::string apiKey = "",
        std::string baseUrl = "https://openrouter.ai/api/v1",
        std::vector<std::string> configuredModels = {}
    );

    std::string name() const override { return "OpenRouter"; }

    bool available() override;

    std::vector<ModelInfo> discoverModels() override;

    AgentResponse generate(const AgentRequest& request) override;

    void setApiKey(const std::string& key) { apiKey_ = key; }
    void setConfiguredModels(const std::vector<std::string>& models) { configuredModels_ = models; }

private:
    std::string apiKey_;
    std::string baseUrl_;
    std::vector<std::string> configuredModels_;
};

} // namespace arenafight
