#include "arenafight/router/model_router.hpp"
#include "arenafight/common/logger.hpp"
#include <algorithm>
#include <iostream>

namespace arenafight {

ModelRouter::ModelRouter(const Config& config)
    : config_(config) {}

void ModelRouter::registerProvider(std::shared_ptr<ModelProvider> provider) {
    if (!provider) return;
    // Don't add duplicate providers by name
    for (const auto& p : providers_) {
        if (p->name() == provider->name()) return;
    }
    providers_.push_back(provider);
}

std::shared_ptr<ModelProvider> ModelRouter::getProviderByName(const std::string& name) const {
    for (const auto& p : providers_) {
        if (p->name() == name) return p;
    }
    return nullptr;
}

void ModelRouter::refreshModels() {
    rebuildCatalog();
}

void ModelRouter::rebuildCatalog() {
    catalog_.clear();
    for (auto& p : providers_) {
        if (!p->available()) {
            Logger::instance().debug("Provider " + p->name() + " is currently offline/unavailable.");
            continue;
        }

        auto models = p->discoverModels();
        for (const auto& m : models) {
            catalog_.push_back({p, m});
        }
    }
}

void ModelRouter::markProviderUnavailable(const std::string& providerName) {
    catalog_.erase(
        std::remove_if(catalog_.begin(), catalog_.end(),
            [&](const auto& entry) {
                return entry.first->name() == providerName;
            }),
        catalog_.end()
    );
}

std::vector<ModelInfo> ModelRouter::getAllAvailableModels() const {
    std::vector<ModelInfo> list;
    for (const auto& [p, m] : catalog_) {
        list.push_back(m);
    }
    return list;
}

ModelSelection ModelRouter::selectModel(
    TaskType taskType,
    const std::string& previousFailedModel,
    const std::string& coderModelUsed,
    const std::string& excludedProvider
) {
    if (catalog_.empty()) {
        rebuildCatalog();
    }

    ModelSelection selection;
    if (catalog_.empty()) {
        selection.reason = "No models available from any provider";
        return selection;
    }

    // Filter candidate list
    std::vector<std::pair<std::shared_ptr<ModelProvider>, ModelInfo>> candidates;
    for (const auto& entry : catalog_) {
        // Exclude failed provider if requested
        if (!excludedProvider.empty() && entry.first->name() == excludedProvider) {
            continue;
        }
        // Exclude if provider marked itself unavailable
        if (!entry.first->available()) {
            continue;
        }
        // Skip previous failing model if alternatives exist
        if (!previousFailedModel.empty() && entry.second.id == previousFailedModel && catalog_.size() > 1) {
            continue;
        }
        candidates.push_back(entry);
    }

    if (candidates.empty()) {
        // Fallback to remaining available entries that aren't excluded provider
        for (const auto& entry : catalog_) {
            if (excludedProvider.empty() || entry.first->name() != excludedProvider) {
                candidates.push_back(entry);
            }
        }
    }

    if (candidates.empty()) {
        selection.reason = "No alternative providers available";
        return selection;
    }

    // If explicit default provider configured
    if (config_.default_provider != "auto" && config_.default_provider != excludedProvider) {
        for (const auto& entry : candidates) {
            if (entry.first->name() == config_.default_provider) {
                selection.provider = entry.first;
                selection.model = entry.second;
                selection.reason = "Selected by explicit default provider config (" + config_.default_provider + ")";
                return selection;
            }
        }
    }

    // Dynamic routing by TaskType
    switch (taskType) {
        case TaskType::PLANNING: {
            for (const auto& entry : candidates) {
                const auto& id = entry.second.id;
                if (id.find("r1") != std::string::npos ||
                    id.find("3.7-sonnet") != std::string::npos ||
                    id.find("3.5-sonnet") != std::string::npos ||
                    id.find("reasoner") != std::string::npos) {
                    selection.provider = entry.first;
                    selection.model = entry.second;
                    selection.reason = "Strong reasoning model selected for Planning";
                    return selection;
                }
            }
            break;
        }
        case TaskType::CODING: {
            for (const auto& entry : candidates) {
                const auto& id = entry.second.id;
                if (entry.first->name() == "ClaudeCode" ||
                    id.find("coder") != std::string::npos ||
                    id.find("3.7-sonnet") != std::string::npos ||
                    id.find("mock-coder") != std::string::npos) {
                    selection.provider = entry.first;
                    selection.model = entry.second;
                    selection.reason = "Specialized coding backend selected for Coding";
                    return selection;
                }
            }
            break;
        }
        case TaskType::DEBUGGING: {
            for (const auto& entry : candidates) {
                const auto& id = entry.second.id;
                if (id.find("r1") != std::string::npos ||
                    id.find("sonnet") != std::string::npos ||
                    id.find("coder") != std::string::npos) {
                    selection.provider = entry.first;
                    selection.model = entry.second;
                    selection.reason = "Reasoning/debugging model selected for Debugging";
                    return selection;
                }
            }
            break;
        }
        case TaskType::VERIFICATION: {
            if (!coderModelUsed.empty() && candidates.size() > 1) {
                for (const auto& entry : candidates) {
                    if (entry.second.id != coderModelUsed) {
                        selection.provider = entry.first;
                        selection.model = entry.second;
                        selection.reason = "Independent model selected for Verification (different from coder: " + coderModelUsed + ")";
                        return selection;
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    // Default fallback: pick highest capability score
    auto bestIt = std::max_element(
        candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) {
            return a.second.capabilityScore < b.second.capabilityScore;
        }
    );

    if (bestIt != candidates.end()) {
        selection.provider = bestIt->first;
        selection.model = bestIt->second;
        selection.reason = "Highest capability available model selected (" + bestIt->first->name() + ": " + bestIt->second.name + ")";
    } else {
        selection.provider = candidates[0].first;
        selection.model = candidates[0].second;
        selection.reason = "Fallback model selection";
    }

    return selection;
}

} // namespace arenafight
