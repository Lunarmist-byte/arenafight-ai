#include "arenafight/planner/planner.hpp"
#include "arenafight/common/logger.hpp"
#include <regex>
#include <iostream>

namespace arenafight {

Planner::Planner(ModelRouter& router, ContextManager& contextMgr)
    : router_(router), contextMgr_(contextMgr) {}

std::vector<Task> Planner::parsePlanFromResponse(const std::string& responseText) {
    std::vector<Task> tasks;
    if (responseText.empty()) return tasks;

    // Search for JSON array in text: ```json [ ... ] ``` or standalone [ ... ]
    static const std::regex jsonArrayRegex("(\\[\\s*\\{[\\s\\S]*?\\}\\s*\\])");
    std::smatch match;
    if (std::regex_search(responseText, match, jsonArrayRegex)) {
        try {
            nlohmann::json j = nlohmann::json::parse(match[1].str());
            if (j.is_array()) {
                for (size_t i = 0; i < j.size(); ++i) {
                    const auto& item = j[i];
                    Task t;
                    t.id = item.value("id", "task_" + std::to_string(i + 1));
                    t.description = item.value("description", "");
                    t.type = taskTypeFromString(item.value("type", "GENERAL"));
                    t.status = TaskStatus::PENDING;

                    if (item.contains("dependencies") && item["dependencies"].is_array()) {
                        for (const auto& dep : item["dependencies"]) {
                            if (dep.is_string()) t.dependencies.push_back(dep.get<std::string>());
                        }
                    }
                    if (!t.description.empty()) {
                        tasks.push_back(t);
                    }
                }
            }
        } catch (const std::exception& e) {
            Logger::instance().debug("Failed to parse plan JSON: " + std::string(e.what()));
        }
    }

    return tasks;
}

std::vector<Task> Planner::generateHeuristicPlan(const std::string& objective) {
    std::vector<Task> tasks;

    Task t1;
    t1.id = "task_1";
    t1.description = "Inspect workspace structure, files, and requirements";
    t1.type = TaskType::RESEARCH;
    t1.dependencies = {};
    t1.status = TaskStatus::PENDING;
    tasks.push_back(t1);

    Task t2;
    t2.id = "task_2";
    t2.description = "Implement core functionality according to objective: " + objective;
    t2.type = TaskType::CODING;
    t2.dependencies = {"task_1"};
    t2.status = TaskStatus::PENDING;
    tasks.push_back(t2);

    Task t3;
    t3.id = "task_3";
    t3.description = "Compile workspace, execute test suite, and validate behavior";
    t3.type = TaskType::DEBUGGING;
    t3.dependencies = {"task_2"};
    t3.status = TaskStatus::PENDING;
    tasks.push_back(t3);

    Task t4;
    t4.id = "task_4";
    t4.description = "Review implementation, ensure credits ('Made by Lunarmist-byte') and finalize documentation";
    t4.type = TaskType::REVIEW;
    t4.dependencies = {"task_3"};
    t4.status = TaskStatus::PENDING;
    tasks.push_back(t4);

    return tasks;
}

std::vector<Task> Planner::createInitialPlan(
    Session& session,
    MemoryManager& memory,
    const std::string& workspaceOverview
) {
    ModelSelection selection = router_.selectModel(TaskType::PLANNING);
    std::vector<Task> tasks;

    if (selection.provider) {
        Logger::instance().info("Planner generating task graph using " + selection.provider->name() + " (" + selection.model.name + ")...");
        AgentRequest req = contextMgr_.buildPlanningContext(
            session, memory, selection.model.id, workspaceOverview
        );

        AgentResponse resp = selection.provider->generate(req);
        if (resp.success) {
            tasks = parsePlanFromResponse(resp.content);
        }
    }

    if (tasks.empty()) {
        Logger::instance().info("Using structured autonomous task planner fallback...");
        tasks = generateHeuristicPlan(session.originalTask);
    }

    session.tasks = tasks;
    memory.addDecision("Initial execution plan created with " + std::to_string(tasks.size()) + " subtasks");
    return tasks;
}

std::vector<Task> Planner::replan(
    Session& session,
    MemoryManager& memory,
    const std::string& reason,
    const std::string& failedTaskId
) {
    Logger::instance().recovery("Triggering dynamic replan: " + reason);
    memory.addDecision("Replanning triggered", reason);

    if (!failedTaskId.empty()) {
        Task* failedTask = session.getTask(failedTaskId);
        if (failedTask) {
            std::string debugId = "debug_" + failedTaskId + "_" + std::to_string(failedTask->attempts);
            Task debugTask;
            debugTask.id = debugId;
            debugTask.description = "Debug and fix failure in [" + failedTaskId + "]: " + failedTask->failureReason;
            debugTask.type = TaskType::DEBUGGING;
            debugTask.status = TaskStatus::PENDING;
            debugTask.dependencies = {}; // Ready immediately

            // Update downstream tasks that depended on failedTask to depend on debugTask instead
            for (auto& t : session.tasks) {
                for (auto& dep : t.dependencies) {
                    if (dep == failedTaskId) {
                        dep = debugId;
                    }
                }
            }

            session.tasks.push_back(debugTask);
            Logger::instance().info("Created recovery debugging task: " + debugId);
        }
    } else {
        // If incomplete according to verifier, append a corrective task
        std::string fixId = "fix_verification_" + std::to_string(session.iteration);
        Task fixTask;
        fixTask.id = fixId;
        fixTask.description = "Address unverified requirements: " + reason;
        fixTask.type = TaskType::CODING;
        fixTask.status = TaskStatus::PENDING;
        fixTask.dependencies = session.completedTasks;
        session.tasks.push_back(fixTask);
        Logger::instance().info("Created corrective task: " + fixId);
    }

    return session.tasks;
}

} // namespace arenafight
