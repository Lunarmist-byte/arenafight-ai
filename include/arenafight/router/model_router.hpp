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

    // Ability-based planning and selection: find which model is best suited based on multidimensional abilities
    ModelSelection selectModelForTask(
        const Task& task,
        const std::string& previousFailedModel = "",
        const std::string& coderModelUsed = "",
        const std::string& excludedProvider = ""
    );

    std::vector<std::pair<ModelInfo, ModelAbilities>> getAbilityMatrix() const;

    // Fast thinking-phase duel: Evaluate reasoning, strategy, edge cases, and safety invariants
    // while strictly disallowing full code generation to accelerate execution and save tokens.
    ThinkingDuelResult duelThinkingPhase(
        const std::string& taskObjective,
        const std::string& candidateModelA = "",
        const std::string& candidateModelB = ""
    );

    ThinkingEvaluation evaluateThinking(
        const std::string& modelId,
        const std::string& thinkingText,
        int tokens,
        int latencyMs
    );

    void markProviderUnavailable(const std::string& providerName);

    std::shared_ptr<ModelProvider> getProviderByName(const std::string& name) const;

    // Register a custom model explicitly (e.g. fine-tuned with Unsloth or configured by user)
    void registerCustomModel(const ModelInfo& model, std::shared_ptr<ModelProvider> provider = nullptr);

    // Dynamically retrieve or bind any arbitrary model string to an active provider
    ModelSelection getOrRegisterDynamicModel(const std::string& modelName);

private:
    Config config_;
    std::vector<std::shared_ptr<ModelProvider>> providers_;
    std::vector<std::pair<std::shared_ptr<ModelProvider>, ModelInfo>> catalog_;

    void rebuildCatalog();
};

} // namespace arenafight
