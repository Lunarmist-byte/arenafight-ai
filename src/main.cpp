#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "arenafight/agent/arena_agent.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/common/logger.hpp"

using namespace arenafight;

void printUsage() {
    TerminalUI::printBanner();
    std::cout << "Usage:\n"
              << "  arena                      Launch interactive task mode\n"
              << "  arena \"<task>\"              Execute a task autonomously\n"
              << "  arena resume <session-id>  Resume an existing session\n"
              << "  arena sessions             List past saved sessions\n"
              << "  arena models               Discover and list models across all backends\n"
              << "  arena config               Display current configuration\n"
              << "  arena test                 Run built-in automated self-tests\n\n"
              << "Options:\n"
              << "  --verbose                  Enable debug output\n"
              << "  --mock                     Enable built-in offline test mock provider\n\n";
    TerminalUI::printFooter();
}

int main(int argc, char* argv[]) {
    Config config = Config::loadFromFile("config.json");
    bool verbose = false;

    // Check flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
            Logger::instance().setVerbose(true);
        } else if (arg == "--mock") {
            config.test_mock_enabled = true;
            config.default_provider = "TestMock";
        }
    }

    if (argc > 1) {
        std::string cmd = argv[1];

        if (cmd == "--help" || cmd == "-h" || cmd == "help") {
            printUsage();
            return 0;
        }

        if (cmd == "sessions") {
            auto sessions = Session::listSessions("sessions");
            TerminalUI::printSessionsList(sessions);
            return 0;
        }

        if (cmd == "config") {
            TerminalUI::printConfigView(config.toJson());
            return 0;
        }

        if (cmd == "models") {
            ArenaAgent agent(config);
            auto models = agent.getRouter().getAllAvailableModels();
            TerminalUI::printModelsList(models);
            return 0;
        }

        if (cmd == "resume") {
            if (argc < 3) {
                std::cerr << "Error: Missing session ID.\nUsage: arena resume <session-id>\n";
                return 1;
            }
            std::string sessionId = argv[2];
            ArenaAgent agent(config);
            if (!agent.resumeSession(sessionId)) {
                return 1;
            }
            bool success = agent.run();
            return success ? 0 : 1;
        }

        if (cmd == "test") {
            std::cout << "Executing self-test suite (run 'ctest' or 'arena_test' for binary tests)...\n";
            std::string testWs = "sandbox_test";
            std::filesystem::create_directories(testWs);
            config.workspace_dir = testWs;
            config.test_mock_enabled = true;
            config.default_provider = "TestMock";
            ArenaAgent agent(config);
            agent.startNewSession("Test multi-step execution and verification");
            bool ok = agent.run();
            std::filesystem::remove_all(testWs);
            return ok ? 0 : 1;
        }

        // If command is not a keyword, treat argument(s) as task string
        if (cmd[0] != '-') {
            std::string task = cmd;
            for (int i = 2; i < argc; ++i) {
                std::string a = argv[i];
                if (a != "--verbose" && a != "-v" && a != "--mock") {
                    task += " " + a;
                }
            }

            ArenaAgent agent(config);
            agent.startNewSession(task);
            bool ok = agent.run();
            return ok ? 0 : 1;
        }
    }

    // Interactive Mode
    TerminalUI::printBanner();
    std::cout << "Entering interactive mode. Type your task below, or 'exit' to quit.\n\n"
              << "> " << std::flush;

    std::string taskInput;
    if (std::getline(std::cin, taskInput)) {
        // Trim whitespace
        while (!taskInput.empty() && (taskInput.back() == ' ' || taskInput.back() == '\r')) {
            taskInput.pop_back();
        }

        if (taskInput == "exit" || taskInput == "quit" || taskInput.empty()) {
            std::cout << "Exiting ArenaFight Agent.\n";
            return 0;
        }

        ArenaAgent agent(config);
        agent.startNewSession(taskInput);
        bool ok = agent.run();
        return ok ? 0 : 1;
    }

    return 0;
}
