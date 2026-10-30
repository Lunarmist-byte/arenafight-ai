#include "arenafight/agent/arena_agent.hpp"
#include "arenafight/providers/provider_factory.hpp"
#include "arenafight/providers/opencode_provider.hpp"
#include "arenafight/providers/test_mock_provider.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace arenafight {

namespace {

std::string generateSessionId() {
    static const char hexChars[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::string s;
    for (int i = 0; i < 8; ++i) {
        s += hexChars[dis(gen)];
    }
    return s;
}

std::string getCurrentIsoTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

} // namespace

ArenaAgent::ArenaAgent(Config config)
    : config_(std::move(config)),
      contextMgr_(config_),
      router_(config_) {
    initialize();
}

void ArenaAgent::initialize() {
    // Register all configured providers via ProviderFactory
    auto providers = ProviderFactory::createConfiguredProviders(config_);
    for (auto& p : providers) {
        router_.registerProvider(p);
    }

    router_.refreshModels();

    // Tools
    tools_ = ToolRegistry::createStandardRegistry(config_);

    // Subsystems
    planner_ = std::make_unique<Planner>(router_, contextMgr_);
    executor_ = std::make_unique<Executor>(router_, tools_, contextMgr_, config_);
    verifier_ = std::make_unique<Verifier>(router_, contextMgr_, config_.workspace_dir);
}

std::string ArenaAgent::inspectWorkspaceSummary() {
    std::stringstream ss;
    ss << "Current Working Directory: " << config_.workspace_dir << "\n";
    try {
        int count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(config_.workspace_dir)) {
            std::string name = entry.path().filename().string();
            if (name == ".git" || name == ".gemini" || name == "build" || name == "venv") continue;
            ss << (entry.is_directory() ? "[DIR]  " : "[FILE] ") << name << "\n";
            if (++count > 25) {
                ss << "... [truncated] ...\n";
                break;
            }
        }
    } catch (...) {}
    return ss.str();
}

bool ArenaAgent::startNewSession(const std::string& userTask) {
    session_ = Session();
    session_.id = generateSessionId();
    session_.originalTask = userTask;
    session_.objective = userTask;
    session_.currentState = "INITIALIZING";
    session_.startTime = getCurrentIsoTime();
    session_.updatedTime = session_.startTime;
    session_.iteration = 0;
    session_.completed = false;

    Logger::instance().init(session_.id);
    Logger::instance().info("New session started: " + session_.id);
    Logger::instance().info("Objective: " + userTask);

    startTimeEpoch_ = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    // Inspect initial workspace and store in memory
    std::string wsOverview = inspectWorkspaceSummary();
    memory_.addFact("Workspace root: " + config_.workspace_dir);
    memory_.addDiscovery("Initial workspace files:\n" + wsOverview);

    session_.saveToDisk();
    return true;
}

bool ArenaAgent::resumeSession(const std::string& sessionId) {
    std::cout << "Loading session...\n";
    try {
        session_ = Session::loadFromDisk(sessionId);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load session " << sessionId << ": " << e.what() << "\n";
        return false;
    }

    std::cout << "Restoring memory...\n";
    memory_.loadFromItems(session_.memories);

    std::cout << "Restoring task graph...\n";
    std::cout << "Restoring current task...\n";
    std::cout << "Continuing execution...\n\n";

    Logger::instance().init(session_.id);
    Logger::instance().info("Resumed session: " + session_.id + " (Iteration " + std::to_string(session_.iteration) + ")");

    startTimeEpoch_ = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    return true;
}

bool ArenaAgent::run() {
    TerminalUI::printBanner();
    TerminalUI::printSessionHeader(session_.originalTask, session_.id);

    if (session_.completed) {
        std::cout << "Session [" << session_.id << "] is already marked COMPLETED.\n"
                  << "Objective: " << session_.originalTask << "\n\n";
        TerminalUI::printPlanStatus(session_);
        TerminalUI::printFooter();
        return true;
    }

    // Check if any providers are available before starting
    router_.refreshModels();
    auto availableModels = router_.getAllAvailableModels();
    if (availableModels.empty()) {
        if (config_.auto_mock_fallback) {
            Logger::instance().warn("No active AI model backends detected. Automatically falling back to Offline Mock Provider (TestMock).");
            std::cout << "\n\033[33m[NOTICE] No active AI model backends detected!\033[0m\n"
                      << "  1. OpenCode CLI    : Not installed or not in PATH (npm i -g @opencode/cli)\n"
                      << "  2. Claude Code CLI : Not authenticated or not found in PATH\n"
                      << "  3. OpenRouter      : No API key configured\n"
                      << "  4. Ollama          : Server offline or no models pulled\n\n"
                      << "\033[36m[AUTO-FALLBACK] Automatically switching to built-in Offline Mock Provider (TestMock) for execution.\033[0m\n\n";

            config_.test_mock_enabled = true;
            auto mock = std::make_shared<TestMockProvider>();
            router_.registerProvider(mock);
            router_.refreshModels();
            availableModels = router_.getAllAvailableModels();
        } else {
            Logger::instance().error("Execution halted: No active AI model backends found.");
            std::cout << "\n\033[31m[ERROR] No active AI model backends detected!\033[0m\n\n"
                      << "Arenafight Agent requires at least one working backend:\n"
                      << "  1. OpenCode CLI    : Install via 'npm install -g @opencode/cli'\n"
                      << "  2. Claude Code CLI : Run 'claude login' in terminal\n"
                      << "  3. OpenRouter      : Set OPENROUTER_API_KEY environment variable or edit config.json\n"
                      << "  4. Ollama          : Start Ollama ('ollama serve') with a pulled model\n"
                      << "  5. Offline Mock    : Run with '--mock' flag for built-in self-test mode\n\n";
            TerminalUI::printFooter();
            session_.currentState = "FAILED_NO_PROVIDERS";
            session_.saveToDisk();
            return false;
        }
    }

    if (availableModels.empty()) {
        Logger::instance().error("Execution halted: Failed to initialize any model provider.");
        TerminalUI::printFooter();
        session_.currentState = "FAILED_NO_PROVIDERS";
        session_.saveToDisk();
        return false;
    }

    // Initial Plan if needed
    if (session_.tasks.empty()) {
        std::cout << "Planner:\nCreating execution plan...\n\n";
        std::string wsOverview = inspectWorkspaceSummary();
        planner_->createInitialPlan(session_, memory_, wsOverview);
        session_.currentState = "PLANNING_COMPLETE";
        session_.saveToDisk();
    }

    // Main Autonomous Loop
    while (!session_.completed && session_.iteration < config_.max_iterations) {
        session_.iteration++;
        session_.currentState = "EXECUTING";

        int64_t nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        int64_t elapsedSec = nowSec - startTimeEpoch_;

        TerminalUI::printPlanStatus(session_);

        auto readyTasks = session_.getReadyTasks();

        if (readyTasks.empty()) {
            if (session_.allTasksCompleted()) {
                Logger::instance().info("All tasks in graph are marked complete. Proceeding to verification...");
            } else {
                // Dependency deadlock or blocked tasks
                bool hasUnresolvedFailures = false;
                for (const auto& t : session_.tasks) {
                    if (t.status == TaskStatus::FAILED) {
                        hasUnresolvedFailures = true;
                        break;
                    }
                }
                if (hasUnresolvedFailures && router_.getAllAvailableModels().empty()) {
                    if (config_.auto_mock_fallback && !config_.test_mock_enabled) {
                        Logger::instance().warn("Providers unavailable for blocked tasks. Automatically enabling Offline Mock Provider (TestMock).");
                        config_.test_mock_enabled = true;
                        auto mock = std::make_shared<TestMockProvider>();
                        router_.registerProvider(mock);
                        router_.refreshModels();
                    } else {
                        Logger::instance().error("Cannot resolve blocked tasks: No AI model provider available.");
                        break;
                    }
                }

                TerminalUI::printRecoveryNotice("Task dependencies blocked. Replanning remaining tasks...");
                planner_->replan(session_, memory_, "Dependency blockage");
                session_.saveToDisk();
                continue;
            }
        }

        // Execute ready tasks
        for (auto* taskPtr : readyTasks) {
            Task& task = *taskPtr;
            task.attempts++;

            TerminalUI::printActiveExecution(
                task.assignedModel.empty() ? "Auto-Selected" : task.assignedModel,
                "Starting Task",
                task.id + " - " + task.description
            );

            TaskExecutionResult execRes = executor_->executeTask(
                task, session_, memory_, config_.workspace_dir
            );

            totalTokensUsed_ += execRes.tokensUsed;
            totalToolCalls_ += execRes.toolCallsCount;

            if (execRes.success) {
                session_.markTaskCompleted(task.id, execRes.summary);
                Logger::instance().info("Task completed: " + task.id);
            } else {
                session_.markTaskFailed(task.id, execRes.summary);
                TerminalUI::printErrorBox(task.id, 1, execRes.summary);

                // Check if all providers became unavailable
                if (execRes.summary.find("No provider available") != std::string::npos ||
                    router_.getAllAvailableModels().empty()) {
                    if (config_.auto_mock_fallback && !config_.test_mock_enabled) {
                        Logger::instance().warn("Providers unavailable during execution. Automatically enabling Offline Mock Provider (TestMock).");
                        config_.test_mock_enabled = true;
                        auto mock = std::make_shared<TestMockProvider>();
                        router_.registerProvider(mock);
                        router_.refreshModels();
                    } else {
                        Logger::instance().error("Execution halted: AI model providers unavailable during execution.");
                        session_.currentState = "FAILED_NO_PROVIDERS";
                        session_.saveToDisk();
                        break;
                    }
                }

                // Prevent infinite debug task recursion
                bool isDebugTask = (task.type == TaskType::DEBUGGING || task.id.rfind("debug_", 0) == 0);

                if (!isDebugTask && task.attempts <= config_.max_retries) {
                    TerminalUI::printRecoveryNotice("Formulating recovery task for [" + task.id + "]...");
                    planner_->replan(session_, memory_, execRes.summary, task.id);
                } else if (isDebugTask) {
                    Logger::instance().error("Debugging task [" + task.id + "] failed; stopping debug recursion.");
                } else {
                    Logger::instance().error("Task reached maximum attempts (" +
                                            std::to_string(config_.max_retries) + "): " + task.id);
                }
            }

            // Sync session memories and save state
            session_.memories = memory_.getAll();
            session_.saveToDisk();
        }

        // Verify phase
        Logger::instance().info("Running independent verification...");
        VerificationResult verif = verifier_->verify(session_, memory_);

        nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        elapsedSec = nowSec - startTimeEpoch_;

        TerminalUI::printIterationFooter(
            session_.iteration,
            totalTokensUsed_,
            totalToolCalls_,
            elapsedSec
        );

        if (verif.complete) {
            session_.completed = true;
            session_.currentState = "COMPLETED";
            session_.saveToDisk();

            std::vector<std::string> modifiedFiles;
            for (const auto& m : memory_.getItemsByCategory("file_change")) {
                modifiedFiles.push_back(m.content);
            }

            TerminalUI::printFinalReport(
                true,
                session_.originalTask,
                verif,
                modifiedFiles,
                elapsedSec
            );
            return true;
        } else {
            if (session_.allTasksCompleted()) {
                // If tasks are all checked off but verification failed: replan corrective tasks
                TerminalUI::printRecoveryNotice("Verification gap detected: " + verif.summary + ". Re-planning corrective steps...");
                planner_->replan(session_, memory_, verif.summary);
                session_.saveToDisk();
            }
        }
    }

    // If max iterations reached without completion
    int64_t nowSec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    int64_t elapsedSec = nowSec - startTimeEpoch_;

    VerificationResult finalVerif = verifier_->verify(session_, memory_);
    session_.currentState = "INCOMPLETE";
    session_.saveToDisk();

    std::vector<std::string> modifiedFiles;
    for (const auto& m : memory_.getItemsByCategory("file_change")) {
        modifiedFiles.push_back(m.content);
    }

    TerminalUI::printFinalReport(
        false,
        session_.originalTask,
        finalVerif,
        modifiedFiles,
        elapsedSec
    );

    return false;
}

} // namespace arenafight
