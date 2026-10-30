#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <filesystem>
#include "arenafight/agent/arena_agent.hpp"
#include "arenafight/repl/repl_engine.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/providers/opencode_provider.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

using namespace arenafight;

namespace {

void printUsage() {
    TerminalUI::printBanner();
    std::cout << "Usage:\n"
              << "  arena                          Launch Antigravity Interactive REPL\n"
              << "  arena \"<task>\"                  Execute a task autonomously (/goal)\n"
              << "  arena goal \"<task>\"             Autonomous goal pursuit with verification\n"
              << "  arena plan \"<task>\"             Generate structured plan artifact with milestones\n"
              << "  arena grill-me \"<topic>\"        Interactive requirements interview\n"
              << "  arena opencode [cmd]           OpenCode CLI integration (status|models|run|sync)\n"
              << "  arena ui [simple|studio]       Launch Simplistic Web UI dashboard in browser\n"
              << "  arena providers                Inspect all AI model backends status\n"
              << "  arena artifacts                List and inspect project artifacts\n"
              << "  arena skills                   List active skills\n"
              << "  arena rules                    Display active workspace and system rules\n"
              << "  arena boards                   List supported microcontroller boards\n"
              << "  arena board <id>               Inspect microcontroller pinout & strapping pins\n"
              << "  arena components               List supported sensors, actuators & ICs\n"
              << "  arena scaffold <board> <proj>  Scaffold hardware project with Wokwi simulation\n"
              << "  arena qos [path]               Evaluate embedded C++ firmware QoS\n"
              << "  arena calculate <type> <args>  Calculate circuit values (led, divider, i2c, battery)\n"
              << "  arena resume <session-id>      Resume an existing session\n"
              << "  arena sessions                 List past saved sessions\n"
              << "  arena models                   Discover and list models across all backends\n"
              << "  arena config                   Display current configuration\n"
              << "  arena test                     Run built-in automated self-tests\n\n"
              << "Options:\n"
              << "  --opencode                     Select OpenCode as primary provider\n"
              << "  --claude                       Select Claude Code CLI as primary provider\n"
              << "  --ollama                       Select Ollama as primary provider\n"
              << "  --openrouter                   Select OpenRouter as primary provider\n"
              << "  --mock                         Force built-in offline test mock provider\n"
              << "  --no-mock-fallback             Disable automatic fallback to offline mock\n"
              << "  --verbose, -v                  Enable debug output\n\n";
    TerminalUI::printFooter();
}

std::string joinArgs(const std::vector<std::string>& args, size_t startIdx = 0) {
    std::string s;
    for (size_t i = startIdx; i < args.size(); ++i) {
        if (!s.empty()) s += " ";
        s += args[i];
    }
    return s;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = "config.json";
    if (!std::filesystem::exists(configPath) && std::filesystem::exists("../config.json")) {
        configPath = "../config.json";
    }
    Config config = Config::loadFromFile(configPath);

    std::vector<std::string> rawArgs;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            Logger::instance().setVerbose(true);
        } else if (arg == "--mock") {
            config.test_mock_enabled = true;
            config.default_provider = "TestMock";
        } else if (arg == "--opencode") {
            config.default_provider = "OpenCode";
        } else if (arg == "--claude") {
            config.default_provider = "ClaudeCode";
        } else if (arg == "--ollama") {
            config.default_provider = "Ollama";
        } else if (arg == "--openrouter") {
            config.default_provider = "OpenRouter";
        } else if (arg == "--no-mock-fallback") {
            config.auto_mock_fallback = false;
        } else if (arg == "--help" || arg == "-h" || arg == "help") {
            printUsage();
            return 0;
        } else {
            rawArgs.push_back(arg);
        }
    }

    ArenaAgent agent(config);

    if (rawArgs.empty()) {
        // Launch Antigravity Interactive REPL
        ReplEngine repl(agent);
        repl.run();
        return 0;
    }

    const std::string& cmd = rawArgs[0];

    // Command Dispatch Table
    using Handler = std::function<int(const std::vector<std::string>&)>;
    std::map<std::string, Handler> dispatchTable;

    dispatchTable["sessions"] = [&](const auto&) {
        auto sessions = Session::listSessions("sessions");
        TerminalUI::printSessionsList(sessions);
        return 0;
    };

    dispatchTable["config"] = [&](const auto&) {
        TerminalUI::printConfigView(config.toJson());
        return 0;
    };

    dispatchTable["models"] = [&](const auto&) {
        auto models = agent.getRouter().getAllAvailableModels();
        TerminalUI::printModelsList(models);
        return 0;
    };

    dispatchTable["providers"] = [&](const auto&) {
        std::cout << "\n====================================================\n"
                  << "              AI MODEL PROVIDERS STATUS              \n"
                  << "====================================================\n";
        auto router = agent.getRouter();
        auto oc = router.getProviderByName("OpenCode");
        auto claude = router.getProviderByName("ClaudeCode");
        auto ollama = router.getProviderByName("Ollama");
        auto openrouter = router.getProviderByName("OpenRouter");
        auto mock = router.getProviderByName("TestMock");

        auto printStatus = [](const std::string& name, auto prov) {
            bool ok = prov && prov->available();
            std::cout << "  * " << std::left;
            std::cout.width(16);
            std::cout << name << ": " << (ok ? "\033[32mONLINE\033[0m" : "\033[33mOFFLINE\033[0m") << "\n";
        };

        printStatus("OpenCode CLI", oc);
        printStatus("Claude Code CLI", claude);
        printStatus("Ollama (Local)", ollama);
        printStatus("OpenRouter", openrouter);
        printStatus("Offline Mock", mock);
        std::cout << "----------------------------------------------------\n"
                  << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n\n";
        return 0;
    };

    dispatchTable["opencode"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string subArgs = joinArgs(args, 1);
        repl.handleOpenCode(subArgs);
        return 0;
    };

    dispatchTable["ui"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string mode = (args.size() > 1) ? args[1] : "simple";
        repl.handleUI(mode);
        return 0;
    };

    dispatchTable["web"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string mode = (args.size() > 1) ? args[1] : "simple";
        repl.handleUI(mode);
        return 0;
    };

    dispatchTable["artifacts"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string arg = (args.size() > 1) ? args[1] : "";
        repl.handleArtifacts(arg);
        return 0;
    };

    dispatchTable["skills"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string arg = (args.size() > 1) ? args[1] : "";
        repl.handleSkills(arg);
        return 0;
    };

    dispatchTable["rules"] = [&](const auto&) {
        ReplEngine repl(agent);
        repl.handleRules("");
        return 0;
    };

    dispatchTable["boards"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string filter = (args.size() > 1) ? args[1] : "";
        repl.handleBoards(filter);
        return 0;
    };

    dispatchTable["board"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string id = (args.size() > 1) ? args[1] : "";
        repl.handleBoard(id);
        return 0;
    };

    dispatchTable["components"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string filter = (args.size() > 1) ? args[1] : "";
        repl.handleComponents(filter);
        return 0;
    };

    dispatchTable["scaffold"] = [&](const auto& args) {
        ReplEngine repl(agent);
        repl.handleScaffold(joinArgs(args, 1));
        return 0;
    };

    dispatchTable["scaffold-hardware"] = dispatchTable["scaffold"];

    dispatchTable["qos"] = [&](const auto& args) {
        ReplEngine repl(agent);
        std::string path = (args.size() > 1) ? args[1] : "";
        repl.handleQoS(path);
        return 0;
    };

    dispatchTable["calculate"] = [&](const auto& args) {
        ReplEngine repl(agent);
        repl.handleCalculate(joinArgs(args, 1));
        return 0;
    };

    dispatchTable["plan"] = [&](const auto& args) {
        if (args.size() < 2) {
            std::cerr << "Error: Missing task for plan.\nUsage: arena plan \"<task>\"\n";
            return 1;
        }
        std::string task = joinArgs(args, 1);
        ReplEngine repl(agent);
        repl.handlePlan(task);
        return 0;
    };

    dispatchTable["goal"] = [&](const auto& args) {
        if (args.size() < 2) {
            std::cerr << "Error: Missing task for goal.\nUsage: arena goal \"<task>\"\n";
            return 1;
        }
        std::string task = joinArgs(args, 1);
        ReplEngine repl(agent);
        repl.handleGoal(task);
        return 0;
    };

    dispatchTable["grill-me"] = [&](const auto& args) {
        std::string topic = joinArgs(args, 1);
        ReplEngine repl(agent);
        repl.handleGrillMe(topic);
        return 0;
    };

    dispatchTable["resume"] = [&](const auto& args) {
        if (args.size() < 2) {
            std::cerr << "Error: Missing session ID.\nUsage: arena resume <session-id>\n";
            return 1;
        }
        std::string sessionId = args[1];
        if (!agent.resumeSession(sessionId)) {
            return 1;
        }
        bool success = agent.run();
        return success ? 0 : 1;
    };

    dispatchTable["test"] = [&](const auto&) {
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
    };

    // Check if command is registered in dispatch table
    auto it = dispatchTable.find(cmd);
    if (it != dispatchTable.end()) {
        return it->second(rawArgs);
    }

    // If not a subcommand keyword and doesn't start with '-', treat all args as a direct task string
    if (cmd[0] != '-') {
        std::string task = joinArgs(rawArgs, 0);
        ReplEngine repl(agent);
        repl.handleGoal(task);
        return 0;
    }

    std::cerr << "Unknown option: " << cmd << "\n";
    printUsage();
    return 1;
}
