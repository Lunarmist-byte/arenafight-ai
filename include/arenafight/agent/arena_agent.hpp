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

namespace arenafight {

class ArenaAgent {
public:
    ArenaAgent(Config config = Config());

    // Initialize all providers and tool registry
    void initialize();

    // Start a new session with a user task
    bool startNewSession(const std::string& userTask);

    // Resume an existing session by ID
    bool resumeSession(const std::string& sessionId);

    // Run the autonomous execution loop until completion, failure, or max iterations
    bool run();

    const Session& getSession() const { return session_; }
    Session& getSessionMutable() { return session_; }
    const Config& getConfig() const { return config_; }
    ModelRouter& getRouter() { return router_; }

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

    int64_t startTimeEpoch_ = 0;
    int totalTokensUsed_ = 0;
    int totalToolCalls_ = 0;

    std::string inspectWorkspaceSummary();
};

} // namespace arenafight
