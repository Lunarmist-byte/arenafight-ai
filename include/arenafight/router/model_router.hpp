#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "arenafight/common/types.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/providers/provider.hpp"

namespace arenafight {

struct ModelSelection {
    std::shared_ptr<ModelProvider> provider;
    ModelInfo model;
    std::string reason;
};

class ModelRouter {
public:
    explicit ModelRouter(const Config& config);

    void registerProvider(std::shared_ptr<ModelProvider> provider);

    void refreshModels();

    std::vector<ModelInfo> getAllAvailableModels() const;

    // Route dynamically based on task type, capabilities, and failure history
    ModelSelection selectModel(
        TaskType taskType,
        const std::string& previousFailedModel = "",
        const std::string& coderModelUsed = "",
        const std::string& excludedProvider = ""
    );

    void markProviderUnavailable(const std::string& providerName);

    std::shared_ptr<ModelProvider> getProviderByName(const std::string& name) const;

private:
    Config config_;
    std::vector<std::shared_ptr<ModelProvider>> providers_;
    std::vector<std::pair<std::shared_ptr<ModelProvider>, ModelInfo>> catalog_;

    void rebuildCatalog();
};

} // namespace arenafight
