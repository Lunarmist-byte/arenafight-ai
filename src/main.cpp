#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "arenafight/agent/arena_agent.hpp"
#include "arenafight/repl/repl_engine.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/common/logger.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

using namespace arenafight;

void printUsage() {
    TerminalUI::printBanner();
    std::cout << "Usage:\n"
              << "  arena                          Launch Antigravity Interactive REPL\n"
              << "  arena \"<task>\"                  Execute a task autonomously (/goal)\n"
              << "  arena goal \"<task>\"             Autonomous goal pursuit with verification\n"
              << "  arena plan \"<task>\"             Generate structured plan artifact with milestones\n"
              << "  arena grill-me \"<topic>\"        Interactive requirements interview\n"
              << "  arena artifacts                List and inspect project artifacts\n"
              << "  arena skills                   List active skills\n"
              << "  arena rules                    Display active workspace and system rules\n"
              << "  arena resume <session-id>      Resume an existing session\n"
              << "  arena sessions                 List past saved sessions\n"
              << "  arena models                   Discover and list models across all backends\n"
              << "  arena config                   Display current configuration\n"
              << "  arena test                     Run built-in automated self-tests\n\n"
              << "Options:\n"
              << "  --verbose                      Enable debug output\n"
              << "  --mock                         Force built-in offline test mock provider\n"
              << "  --no-mock-fallback             Disable automatic fallback to offline mock\n\n";
    TerminalUI::printFooter();
}

int main(int argc, char* argv[]) {
    std::string configPath = "config.json";
    if (!std::filesystem::exists(configPath) && std::filesystem::exists("../config.json")) {
        configPath = "../config.json";
    }
    Config config = Config::loadFromFile(configPath);

    // Check flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            Logger::instance().setVerbose(true);
        } else if (arg == "--mock") {
            config.test_mock_enabled = true;
            config.default_provider = "TestMock";
        if (arg == "--help" || arg == "-h" || arg == "help") {
            printUsage();
            return 0;
        }
    }

    ArenaAgent agent(config);

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
            auto models = agent.getRouter().getAllAvailableModels();
            TerminalUI::printModelsList(models);
            return 0;
        }

        if (cmd == "artifacts") {
            ReplEngine repl(agent);
            std::string arg = (argc > 2) ? argv[2] : "";
            repl.handleArtifacts(arg);
            return 0;
        }

        if (cmd == "skills") {
            ReplEngine repl(agent);
            std::string arg = (argc > 2) ? argv[2] : "";
            repl.handleSkills(arg);
            return 0;
        }

        if (cmd == "rules") {
            ReplEngine repl(agent);
            repl.handleRules("");
            return 0;
        }

        if (cmd == "plan") {
            if (argc < 3) {
                std::cerr << "Error: Missing task for plan.\nUsage: arena plan \"<task>\"\n";
                return 1;
            }
            std::string task = argv[2];
            for (int i = 3; i < argc; ++i) task += " " + std::string(argv[i]);
            ReplEngine repl(agent);
            repl.handlePlan(task);
            return 0;
        }

        if (cmd == "goal") {
            if (argc < 3) {
                std::cerr << "Error: Missing task for goal.\nUsage: arena goal \"<task>\"\n";
                return 1;
            }
            std::string task = argv[2];
            for (int i = 3; i < argc; ++i) task += " " + std::string(argv[i]);
            ReplEngine repl(agent);
            repl.handleGoal(task);
            return 0;
        }

        if (cmd == "grill-me") {
            std::string topic = (argc > 2) ? argv[2] : "";
            for (int i = 3; i < argc; ++i) topic += " " + std::string(argv[i]);
            ReplEngine repl(agent);
            repl.handleGrillMe(topic);
            return 0;
        }

        if (cmd == "resume") {
            if (argc < 3) {
                std::cerr << "Error: Missing session ID.\nUsage: arena resume <session-id>\n";
                return 1;
            }
            std::string sessionId = argv[2];
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
            ArenaAgent testAgent(config);
            testAgent.startNewSession("Test multi-step execution and verification");
            bool ok = testAgent.run();
            std::error_code ec;
            std::filesystem::remove_all(testWs, ec);
            return ok ? 0 : 1;
        }

        // If command is not a keyword and doesn't start with '-', treat argument(s) as task string
        if (cmd[0] != '-') {
            std::string task = cmd;
            for (int i = 2; i < argc; ++i) {
                std::string a = argv[i];
                if (a != "--verbose" && a != "-v" && a != "--mock") {
                    task += " " + a;
                }
            }

            ReplEngine repl(agent);
            repl.handleGoal(task);
            return 0;
        }
    }

    // Launch Antigravity Interactive REPL
    ReplEngine repl(agent);
    repl.run();

    return 0;
}
