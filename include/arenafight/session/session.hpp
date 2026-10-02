#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "arenafight/common/types.hpp"

namespace arenafight {

struct SessionInfo {
    std::string id;
    std::string originalTask;
    std::string currentState;
    int iteration = 0;
    bool completed = false;
    std::string startTime;
    std::string updatedTime;
};

struct Session {
    std::string id;
    std::string originalTask;
    std::string objective;
    std::vector<Task> tasks;
    std::vector<ExecutionRecord> executions;
    std::vector<MemoryItem> memories;
    std::vector<std::string> discoveries;
    std::vector<std::string> decisions;
    std::vector<std::string> completedTasks;
    std::vector<std::string> failedTasks;
    std::string currentState = "INITIALIZING";
    int iteration = 0;
    bool completed = false;
    std::string startTime;
    std::string updatedTime;

    // Serialization
    nlohmann::json toJson() const;
    static Session fromJson(const nlohmann::json& j);

    // Disk persistence
    bool saveToDisk(const std::string& sessionsDir = "sessions") const;
    static Session loadFromDisk(const std::string& sessionId, const std::string& sessionsDir = "sessions");
    static std::vector<SessionInfo> listSessions(const std::string& sessionsDir = "sessions");

    // Task graph helpers
    Task* getTask(const std::string& taskId);
    const Task* getTask(const std::string& taskId) const;
    std::vector<Task*> getReadyTasks();
    bool allTasksCompleted() const;
    void markTaskCompleted(const std::string& taskId, const std::string& result);
    void markTaskFailed(const std::string& taskId, const std::string& reason);
};

} // namespace arenafight
