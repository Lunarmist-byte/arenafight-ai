#include "arenafight/ui/terminal_ui.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace arenafight {

namespace {

std::string formatTime(int64_t seconds) {
    int mins = static_cast<int>(seconds / 60);
    int secs = static_cast<int>(seconds % 60);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << mins << ":"
       << std::setfill('0') << std::setw(2) << secs;
    return ss.str();
}

std::string formatNumber(int num) {
    std::string s = std::to_string(num);
    int insertPosition = static_cast<int>(s.length()) - 3;
    while (insertPosition > 0) {
        s.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return s;
}

} // namespace

void TerminalUI::printBanner() {
    std::cout << "\n"
              << "====================================================\n"
              << "                 ARENAFIGHT AGENT                   \n"
              << "   Autonomous Multi-Model Execution Runtime (C++20) \n"
              << "----------------------------------------------------\n"
              << " Made by: Lunarmist-byte\n"
              << " GitHub:   https://github.com/Lunarmist-byte\n"
              << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
              << "====================================================\n\n";
}

void TerminalUI::printFooter() {
    std::cout << "\n----------------------------------------------------\n"
              << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
              << "====================================================\n";
}

void TerminalUI::printSessionHeader(const std::string& task, const std::string& sessionId) {
    std::cout << "Task:\n" << task << "\n\n"
              << "Session ID:\n" << sessionId << "\n\n";
}

void TerminalUI::printPlanStatus(const Session& session) {
    std::cout << "Execution Plan:\n";
    for (size_t i = 0; i < session.tasks.size(); ++i) {
        const auto& t = session.tasks[i];
        std::stringstream ss;
        ss << "[" << (i + 1) << "/" << session.tasks.size() << "] "
           << std::left << std::setw(34) << (t.description.size() > 34 ? t.description.substr(0, 31) + "..." : t.description);

        std::string statusTag;
        switch (t.status) {
            case TaskStatus::COMPLETED: statusTag = "\033[32mDONE\033[0m"; break;
            case TaskStatus::RUNNING:   statusTag = "\033[36mRUNNING\033[0m"; break;
            case TaskStatus::FAILED:    statusTag = "\033[31mFAILED\033[0m"; break;
            case TaskStatus::BLOCKED:   statusTag = "\033[33mBLOCKED\033[0m"; break;
            default:                    statusTag = "\033[90mPENDING\033[0m"; break;
        }

        std::cout << ss.str() << " " << statusTag << "\n";
    }
    std::cout << "\n";
}

void TerminalUI::printActiveExecution(
    const std::string& model,
    const std::string& tool,
    const std::string& detail
) {
    std::cout << "Model:\n" << model << "\n\n"
              << "Tool:\n" << tool << "\n\n";
    if (!detail.empty()) {
        std::cout << "Target:\n" << detail << "\n\n";
    }
}

void TerminalUI::printIterationFooter(
    int iteration,
    int tokens,
    int toolCalls,
    int64_t elapsedSec
) {
    std::cout << "----------------------------------------------------\n"
              << "Iteration:  " << iteration << "\n"
              << "Tokens:     " << formatNumber(tokens) << "\n"
              << "Tool calls: " << toolCalls << "\n"
              << "Elapsed:    " << formatTime(elapsedSec) << "\n"
              << "----------------------------------------------------\n\n";
}

void TerminalUI::printErrorBox(
    const std::string& command,
    int exitCode,
    const std::string& errorDetail
) {
    std::cout << "\n\033[31m[ERROR]\033[0m\n"
              << "Command:   " << command << "\n"
              << "Exit code: " << exitCode << "\n\n"
              << errorDetail << "\n\n";
}

void TerminalUI::printRecoveryNotice(const std::string& recoveryPlan) {
    std::cout << "\033[35m[RECOVERY]\033[0m\n"
              << recoveryPlan << "\n\n";
}

void TerminalUI::printFinalReport(
    bool completed,
    const std::string& objective,
    const VerificationResult& verif,
    const std::vector<std::string>& filesCreated,
    int64_t totalElapsedSec
) {
    std::cout << "\n====================================================\n";
    if (completed) {
        std::cout << "                 TASK COMPLETED                     \n";
    } else {
        std::cout << "             TASK PARTIALLY COMPLETED               \n";
    }
    std::cout << "====================================================\n\n"
              << "Objective:\n" << objective << "\n\n";

    if (!filesCreated.empty()) {
        std::cout << "Modified / Created Files:\n";
        for (const auto& f : filesCreated) {
            std::cout << "- " << f << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "Verification Evidence:\n";
    for (const auto& pc : verif.passedChecks) {
        std::cout << "[PASS] " << pc << "\n";
    }
    for (const auto& fc : verif.failedChecks) {
        std::cout << "[FAIL] " << fc << "\n";
    }

    std::cout << "\nSummary:\n" << verif.summary << "\n\n"
              << "Total Duration: " << formatTime(totalElapsedSec) << "\n";

    printFooter();
}

void TerminalUI::printSessionsList(const std::vector<SessionInfo>& sessions) {
    printBanner();
    std::cout << "Saved Agent Sessions:\n\n";
    if (sessions.empty()) {
        std::cout << "(No saved sessions found in sessions/)\n";
        return;
    }

    std::cout << std::left
              << std::setw(20) << "SESSION ID"
              << std::setw(12) << "STATUS"
              << std::setw(6)  << "ITER"
              << std::setw(22) << "LAST UPDATED"
              << "TASK"
              << "\n";
    std::cout << std::string(75, '-') << "\n";

    for (const auto& s : sessions) {
        std::string status = s.completed ? "COMPLETED" : s.currentState;
        std::string taskBrief = s.originalTask;
        if (taskBrief.size() > 30) taskBrief = taskBrief.substr(0, 27) + "...";

        std::cout << std::left
                  << std::setw(20) << s.id
                  << std::setw(12) << status
                  << std::setw(6)  << s.iteration
                  << std::setw(22) << s.updatedTime
                  << taskBrief
                  << "\n";
    }
    std::cout << "\nTo resume a session run:\n  arena resume <session-id>\n";
    printFooter();
}

void TerminalUI::printModelsList(const std::vector<ModelInfo>& models) {
    printBanner();
    std::cout << "Available Models Discovered:\n\n";
    if (models.empty()) {
        std::cout << "No models currently available.\n"
                  << "Ensure Ollama is running ('ollama serve') or set OPENROUTER_API_KEY.\n";
        return;
    }

    std::cout << std::left
              << std::setw(14) << "PROVIDER"
              << std::setw(36) << "MODEL ID"
              << std::setw(10) << "LOCAL"
              << std::setw(10) << "CONTEXT"
              << "SCORE"
              << "\n";
    std::cout << std::string(75, '-') << "\n";

    for (const auto& m : models) {
        std::cout << std::left
                  << std::setw(14) << m.provider
                  << std::setw(36) << (m.id.size() > 34 ? m.id.substr(0, 32) + ".." : m.id)
                  << std::setw(10) << (m.isLocal ? "YES" : "NO")
                  << std::setw(10) << m.contextWindow
                  << std::fixed << std::setprecision(1) << m.capabilityScore
                  << "\n";
    }
    printFooter();
}

void TerminalUI::printConfigView(const nlohmann::json& configJson) {
    printBanner();
    std::cout << "Current Configuration (config.json):\n\n"
              << configJson.dump(4) << "\n";
    printFooter();
}

} // namespace arenafight
