#pragma once

#include <string>
#include <memory>
#include <vector>
#include "arenafight/common/types.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/session/memory.hpp"
#include "arenafight/session/context_manager.hpp"
#include "arenafight/router/model_router.hpp"
#include "arenafight/planner/planner.hpp"
#include "arenafight/executor/executor.hpp"
#include "arenafight/verifier/verifier.hpp"
#include "arenafight/tools/tool.hpp"
#include "arenafight/benchmark/benchmark_engine.hpp"
#include "arenafight/system/system_profiler.hpp"
#include "arenafight/services/service_daemon.hpp"

namespace arenafight {

class ArenaAgent {
public:
    ArenaAgent(Config config = Config());

    // Initialize all providers and tool registry
    void initialize();

    // Start a new session with a user task and optional attached files
    bool startNewSession(const std::string& userTask, const std::vector<std::string>& attachedFiles = {});

    // Attach a file to the active session
    void attachFile(const std::string& filePath, const std::string& content = "");
    const std::vector<std::string>& getAttachedFiles() const { return session_.attachedFiles; }

    // Resume an existing session by ID
    bool resumeSession(const std::string& sessionId);

    // Run the autonomous execution loop until completion, failure, or max iterations
    bool run();

    const Session& getSession() const { return session_; }
    Session& getSessionMutable() { return session_; }
    const Config& getConfig() const { return config_; }
    Config& getConfigMutable() { return config_; }
    void setMode(const std::string& mode) { config_.setMode(mode); }
    std::string getMode() const { return config_.mode; }
    ModelRouter& getRouter() { return router_; }
    void setTemperature(float temp) { config_.default_temperature = temp; }
    void setMaxTokens(int tokens) { config_.default_max_tokens = tokens; }
    float getTemperature() const { return config_.default_temperature; }
    int getMaxTokens() const { return config_.default_max_tokens; }
    void setModel(const std::string& modelName) {
        std::string resolved = router_.resolveAlias(modelName);
        config_.default_model = resolved;
        router_.getOrRegisterDynamicModel(resolved);
    }
    const std::string& getModel() const { return config_.default_model; }
    void setModelAlias(const std::string& alias, const std::string& targetModel) {
        config_.setAlias(alias, targetModel);
        router_.setAlias(alias, targetModel);
        config_.saveToFile("config.json");
    }
    bool removeModelAlias(const std::string& alias) {
        bool ok = config_.removeAlias(alias);
        router_.removeAlias(alias);
        config_.saveToFile("config.json");
        return ok;
    }
    std::string resolveModelAlias(const std::string& aliasOrName) const {
        return router_.resolveAlias(aliasOrName);
    }
    std::map<std::string, std::string> getModelAliases() const {
        return router_.getAliases();
    }
    void setProcessingLevel(const std::string& lvl) { config_.setProcessingLevel(lvl); }
    std::string getProcessingLevel() const { return config_.processing_level; }
    bool isFast() const { return config_.isFast(); }
    bool isPerfectBoi() const { return config_.isPerfectBoi(); }
    const DynamicResourceLimits& getActiveLimits() const { return activeLimits_; }
    const HardwareSpec& getHostHardwareSpec() const { return hostSpec_; }

    ThinkingDuelResult duelThinkingPhase(
        const std::string& taskObjective,
        const std::string& candidateModelA = "",
        const std::string& candidateModelB = ""
    ) {
        return router_.duelThinkingPhase(taskObjective, candidateModelA, candidateModelB);
    }

    BenchmarkSuiteResult benchmarkAll(bool verbose = true);
    BenchmarkSuiteResult benchmarkModels(const std::vector<std::string>& modelNames, bool verbose = true);

private:
    Config config_;
    Session session_;
    MemoryManager memory_;
    ContextManager contextMgr_;
    ModelRouter router_;
    ToolRegistry tools_;

    std::unique_ptr<Planner> planner_;
    std::unique_ptr<Executor> executor_;
    std::unique_ptr<Verifier> verifier_;

    HardwareSpec hostSpec_;
    DynamicResourceLimits activeLimits_;

    int64_t startTimeEpoch_ = 0;
    int totalTokensUsed_ = 0;
    int totalToolCalls_ = 0;

    std::string inspectWorkspaceSummary();
};

} // namespace arenafight
