#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/session/memory.hpp"
#include "arenafight/session/context_manager.hpp"
#include "arenafight/tools/tool.hpp"
#include "arenafight/router/model_router.hpp"

namespace arenafight {

struct TaskExecutionResult {
    bool success = false;
    std::string summary;
    int toolCallsCount = 0;
    int tokensUsed = 0;
    std::string assignedModel;
};

class Executor {
public:
    Executor(
        ModelRouter& router,
        ToolRegistry& tools,
        ContextManager& contextMgr,
        const Config& config
    );

    // Executes a single task from start to finish (including tool loop)
    TaskExecutionResult executeTask(
        Task& task,
        Session& session,
        MemoryManager& memory,
        const std::string& workingDir = "."
    );

private:
    ModelRouter& router_;
    ToolRegistry& tools_;
    ContextManager& contextMgr_;
    Config config_;

    std::map<std::string, std::string> inspectRelevantFiles(
        const Task& task,
        const std::string& workingDir
    );
};

} // namespace arenafight
