#include "arenafight/session/session.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace arenafight {

namespace {

std::string getCurrentIsoTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

} // namespace

nlohmann::json Session::toJson() const {
    nlohmann::json tasksJson = nlohmann::json::array();
    for (const auto& t : tasks) tasksJson.push_back(t.toJson());

    nlohmann::json execJson = nlohmann::json::array();
    for (const auto& e : executions) execJson.push_back(e.toJson());

    nlohmann::json memJson = nlohmann::json::array();
    for (const auto& m : memories) memJson.push_back(m.toJson());

    return {
        {"id", id},
        {"originalTask", originalTask},
        {"objective", objective},
        {"tasks", tasksJson},
        {"executions", execJson},
        {"memories", memJson},
        {"discoveries", discoveries},
        {"decisions", decisions},
        {"completedTasks", completedTasks},
        {"failedTasks", failedTasks},
        {"currentState", currentState},
        {"iteration", iteration},
        {"completed", completed},
        {"startTime", startTime},
        {"updatedTime", updatedTime}
    };
}

Session Session::fromJson(const nlohmann::json& j) {
    Session s;
    s.id = j.value("id", "");
    s.originalTask = j.value("originalTask", "");
    s.objective = j.value("objective", "");

    if (j.contains("tasks") && j["tasks"].is_array()) {
        for (const auto& tj : j["tasks"]) {
            s.tasks.push_back(Task::fromJson(tj));
        }
    }
    if (j.contains("executions") && j["executions"].is_array()) {
        for (const auto& ej : j["executions"]) {
            s.executions.push_back(ExecutionRecord::fromJson(ej));
        }
    }
    if (j.contains("memories") && j["memories"].is_array()) {
        for (const auto& mj : j["memories"]) {
            s.memories.push_back(MemoryItem::fromJson(mj));
        }
    }
    s.discoveries = j.value("discoveries", std::vector<std::string>{});
    s.decisions = j.value("decisions", std::vector<std::string>{});
    s.completedTasks = j.value("completedTasks", std::vector<std::string>{});
    s.failedTasks = j.value("failedTasks", std::vector<std::string>{});
    s.currentState = j.value("currentState", "INITIALIZING");
    s.iteration = j.value("iteration", 0);
    s.completed = j.value("completed", false);
    s.startTime = j.value("startTime", "");
    s.updatedTime = j.value("updatedTime", "");
    return s;
}

bool Session::saveToDisk(const std::string& sessionsDir) const {
    try {
        std::string dir = sessionsDir + "/" + id;
        std::filesystem::create_directories(dir);

        // 1. session.json (atomic write via temp file)
        std::string sessTmp = dir + "/session.json.tmp";
        std::string sessFinal = dir + "/session.json";
        {
            std::ofstream f(sessTmp, std::ios::trunc);
            if (!f.is_open()) return false;
            f << toJson().dump(2);
            f.flush();
        }
        std::error_code ec;
        std::filesystem::rename(sessTmp, sessFinal, ec);
        if (ec) {
            std::filesystem::remove(sessFinal, ec);
            std::filesystem::rename(sessTmp, sessFinal, ec);
        }

        // 2. memory.json (atomic write via temp file)
        std::string memTmp = dir + "/memory.json.tmp";
        std::string memFinal = dir + "/memory.json";
        {
            std::ofstream f(memTmp, std::ios::trunc);
            if (f.is_open()) {
                nlohmann::json memArr = nlohmann::json::array();
                for (const auto& m : memories) memArr.push_back(m.toJson());
                nlohmann::json memDoc = {
                    {"sessionId", id},
                    {"discoveries", discoveries},
                    {"decisions", decisions},
                    {"memories", memArr}
                };
                f << memDoc.dump(2);
                f.flush();
                f.close();

                std::filesystem::rename(memTmp, memFinal, ec);
                if (ec) {
                    std::filesystem::remove(memFinal, ec);
                    std::filesystem::rename(memTmp, memFinal, ec);
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving session to disk: " << e.what() << std::endl;
        return false;
    }
}

Session Session::loadFromDisk(const std::string& sessionId, const std::string& sessionsDir) {
    std::string path = sessionsDir + "/" + sessionId + "/session.json";
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Session file not found: " + path);
    }
    nlohmann::json j = nlohmann::json::parse(f);
    return fromJson(j);
}

std::vector<SessionInfo> Session::listSessions(const std::string& sessionsDir) {
    std::vector<SessionInfo> list;
    if (!std::filesystem::exists(sessionsDir)) {
        return list;
    }

    for (const auto& entry : std::filesystem::directory_iterator(sessionsDir)) {
        if (entry.is_directory()) {
            std::string sessionFile = entry.path().string() + "/session.json";
            if (std::filesystem::exists(sessionFile)) {
                try {
                    std::ifstream f(sessionFile);
                    nlohmann::json j = nlohmann::json::parse(f);
                    SessionInfo info;
                    info.id = j.value("id", entry.path().filename().string());
                    info.originalTask = j.value("originalTask", "");
                    info.currentState = j.value("currentState", "UNKNOWN");
                    info.iteration = j.value("iteration", 0);
                    info.completed = j.value("completed", false);
                    info.startTime = j.value("startTime", "");
                    info.updatedTime = j.value("updatedTime", "");
                    list.push_back(info);
                } catch (...) {}
            }
        }
    }

    // Sort by updated time descending
    std::sort(list.begin(), list.end(), [](const SessionInfo& a, const SessionInfo& b) {
        return a.updatedTime > b.updatedTime;
    });

    return list;
}

Task* Session::getTask(const std::string& taskId) {
    for (auto& t : tasks) {
        if (t.id == taskId) return &t;
    }
    return nullptr;
}

const Task* Session::getTask(const std::string& taskId) const {
    for (const auto& t : tasks) {
        if (t.id == taskId) return &t;
    }
    return nullptr;
}

std::vector<Task*> Session::getReadyTasks() {
    std::vector<Task*> ready;
    for (auto& task : tasks) {
        if (task.status != TaskStatus::PENDING) continue;

        // Check if all dependencies are completed
        bool depsSatisfied = true;
        for (const auto& depId : task.dependencies) {
            bool depCompleted = false;
            for (const auto& compId : completedTasks) {
                if (compId == depId) {
                    depCompleted = true;
                    break;
                }
            }
            if (!depCompleted) {
                // Also check task list directly
                const Task* depTask = getTask(depId);
                if (depTask && depTask->status == TaskStatus::COMPLETED) {
                    depCompleted = true;
                }
            }
            if (!depCompleted) {
                depsSatisfied = false;
                break;
            }
        }

        if (depsSatisfied) {
            ready.push_back(&task);
        }
    }
    return ready;
}

bool Session::allTasksCompleted() const {
    if (tasks.empty()) return false;
    for (const auto& t : tasks) {
        if (t.status != TaskStatus::COMPLETED && t.status != TaskStatus::SKIPPED) {
            return false;
        }
    }
    return true;
}

void Session::markTaskCompleted(const std::string& taskId, const std::string& result) {
    Task* t = getTask(taskId);
    if (t) {
        t->status = TaskStatus::COMPLETED;
        t->result = result;
        t->completedAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    if (std::find(completedTasks.begin(), completedTasks.end(), taskId) == completedTasks.end()) {
        completedTasks.push_back(taskId);
    }
    // Remove from failed if it was there
    failedTasks.erase(std::remove(failedTasks.begin(), failedTasks.end(), taskId), failedTasks.end());
    updatedTime = getCurrentIsoTime();
}

void Session::markTaskFailed(const std::string& taskId, const std::string& reason) {
    Task* t = getTask(taskId);
    if (t) {
        t->status = TaskStatus::FAILED;
        t->failureReason = reason;
        t->attempts++;
    }
    if (std::find(failedTasks.begin(), failedTasks.end(), taskId) == failedTasks.end()) {
        failedTasks.push_back(taskId);
    }
    updatedTime = getCurrentIsoTime();
}

} // namespace arenafight
