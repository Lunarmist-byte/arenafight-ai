#pragma once

#include "arenafight/providers/provider.hpp"
#include <mutex>
#include <unordered_set>
#include <unordered_map>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class LlamaCppProvider : public ModelProvider {
public:
    explicit LlamaCppProvider(
        std::string serverUrl = "http://127.0.0.1:8080",
        int gpuLayers = 99
    );

    std::string name() const override { return "llama.cpp"; }

    bool available() override;

    std::vector<ModelInfo> discoverModels() override;

    AgentResponse generate(const AgentRequest& request) override;

    void setServerUrl(const std::string& url) { serverUrl_ = url; }
    const std::string& getServerUrl() const { return serverUrl_; }
    bool supportsTools(const std::string& modelId) const;

private:
    std::string serverUrl_;
    int gpuLayers_;
    std::unordered_set<std::string> toolSupportingModels_;
    std::unordered_map<std::string, std::string> modelToGgufPath_;

    static std::mutex& getServerMutex() {
        static std::mutex mtx;
        return mtx;
    }
};

} // namespace arenafight

