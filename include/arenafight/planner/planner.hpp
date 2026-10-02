#pragma once

#include <string>
#include <vector>
#include <memory>
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/session/memory.hpp"
#include "arenafight/session/context_manager.hpp"
#include "arenafight/router/model_router.hpp"

namespace arenafight {

class Planner {
public:
    Planner(ModelRouter& router, ContextManager& contextMgr);

    // Initial plan creation from user objective
    std::vector<Task> createInitialPlan(
        Session& session,
        MemoryManager& memory,
        const std::string& workspaceOverview
    );

    // Dynamic replanning when tasks fail or verification reveals gaps
    std::vector<Task> replan(
        Session& session,
        MemoryManager& memory,
        const std::string& reason,
        const std::string& failedTaskId = ""
    );

    // Helper: topological sort / ready task picker
    static std::vector<std::string> resolveDependencies(const std::vector<Task>& tasks);

private:
    ModelRouter& router_;
    ContextManager& contextMgr_;

    std::vector<Task> parsePlanFromResponse(const std::string& responseText);
    std::vector<Task> generateHeuristicPlan(const std::string& objective);
};

} // namespace arenafight
