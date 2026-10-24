#pragma once

#include "arenafight/providers/provider.hpp"
#include <mutex>
#include <unordered_set>

namespace arenafight {

class OllamaProvider : public ModelProvider {
public:
    explicit OllamaProvider(std::string baseUrl = "http://localhost:11434");

    std::string name() const override { return "Ollama"; }

    bool available() override;

    std::vector<ModelInfo> discoverModels() override;

    AgentResponse generate(const AgentRequest& request) override;

    void setBaseUrl(const std::string& url) { baseUrl_ = url; }
    const std::string& getBaseUrl() const { return baseUrl_; }
    bool supportsTools(const std::string& modelId) const;

private:
    std::string baseUrl_;
    std::unordered_set<std::string> toolSupportingModels_;

    bool tryAutoStartOllama();

    static std::mutex& getLocalGpuMutex() {
        static std::mutex mtx;
        return mtx;
    }
};

} // namespace arenafight
