#include "arenafight/agent/arena_agent.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/mcp/mcp_server.hpp"
#include "arenafight/providers/opencode_provider.hpp"
#include "arenafight/repl/repl_engine.hpp"
#include "arenafight/services/service_daemon.hpp"
#include "arenafight/services/llama_server_daemon.hpp"
#include "arenafight/services/service_daemon.hpp"
#include "arenafight/services/llama_server_daemon.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

using namespace arenafight;

namespace {

void printUsage() {
  TerminalUI::printBanner();
  std::cout
      << "Usage:\n"
      << "  arena                          Launch Interactive "
         "REPL\n"
      << "  arena \"<task>\"                  Execute a task autonomously "
         "(/goal)\n"
      << "  arena chat [persona]           Launch fun interactive conversation mode (witty, pirate, zen, etc.)\n"
      << "  arena talk [persona]           Alias for arena chat\n"
      << "  arena goal \"<task>\"             Autonomous goal pursuit with "
         "verification\n"
      << "  arena plan \"<task>\"             Generate structured plan "
         "artifact with milestones\n"
      << "  arena mode [standalone|opencode] View or switch execution mode\n"
      << "  arena level [fast|perfect]     Set processing level: Fast (standard) or Perfect Boi (adaptive)\n"
      << "  arena profile                  Assess host machine hardware & dynamic limits (no hardcoding)\n"
      << "  arena services, daemon         Autonomous background service starter (Ollama, etc.)\n"
      << "  arena connect [opencode]       Connect to OpenCode CLI bridge\n"
      << "  arena disconnect               Disconnect from OpenCode and revert to standalone\n"
      << "  arena status                   Display CLI status dashboard & backend health\n"
      << "  arena security [cmd]           Security posture, path jail, DLP & vulnerability scan\n"
      << "  arena audit                    Inspect tamper-evident SHA-256 cryptographic audit trail\n"
      << "  arena benchmark [all|models]   Universal model benchmarking suite (/bench)\n"
      << "  arena grill-me \"<topic>\"        Interactive requirements "
         "interview\n"
      << "  arena opencode [cmd]           OpenCode CLI integration "
         "(status|models|run|sync)\n"
      << "  arena unsloth [cmd]            Unsloth AI on-the-fly fine-tuning "
         "(status|dataset|scaffold|train)\n"
      << "  arena files [cmd]              Workspace file operations "
         "(list|cat|read|attach|attached)\n"
      << "  arena think <task>             Fast thinking tournament (Code "
         "generation disallowed)\n"
      << "  arena duel <task>              Alias for think duel\n"
      << "  arena abilities                Display model ability matrix & "
         "capability scores\n"
      << "  arena metrics, telemetry       Display live verification & evidence telemetry observatory\n"
      << "  arena providers                Inspect all AI model backends "
         "status\n"
      << "  arena artifacts                List and inspect project artifacts\n"
      << "  arena skills                   List active skills\n"
      << "  arena rules                    Display active workspace and system "
         "rules\n"
      << "  arena boards                   List supported microcontroller "
         "boards\n"
      << "  arena board <id>               Inspect microcontroller pinout & "
         "strapping pins\n"
      << "  arena components               List supported sensors, actuators & "
         "ICs\n"
      << "  arena scaffold <board> <proj>  Scaffold hardware project with "
         "Wokwi simulation\n"
      << "  arena qos [path]               Evaluate embedded C++ firmware QoS\n"
      << "  arena calculate <type> <args>  Calculate circuit values (led, "
         "divider, i2c, battery)\n"
      << "  arena resume <session-id>      Resume an existing session\n"
      << "  arena sessions                 List past saved sessions\n"
      << "  arena models [set <name>]      Discover, list, or set models "
         "across all backends\n"
      << "  arena model <name>             Set active reasoning model for "
         "session execution\n"
      << "  arena config                   Display current configuration\n"
      << "  arena help-session, guide      Launch interactive guided help "
         "session\n"
      << "  arena mcp                      Launch Model Context Protocol (MCP) server for OpenCode\n"
      << "  arena verify                   Execute Level 1 physical evidence verification gates\n"
      << "  arena test                     Run built-in automated "
         "self-tests\n\n"
      << "Options:\n"
      << "  --mcp                          Run as stdio Model Context Protocol (MCP) server\n"
      << "  --cuda                         Enable dedicated NVIDIA CUDA GPU acceleration (default if present)\n"
      << "  --cpu, --no-cuda               Run in CPU-only / slow variant mode (for non-GPU or testing)\n"
      << "  --fast                         Fast processing level: standard turnaround & loops\n"
      << "  --perfect, --perfect-boi       Perfect Boi level: exhaustive verification & PC-assessed limits\n"
      << "  --level <fast|perfect>         Explicitly set processing level\n"
      << "  --standalone                   Force standalone native mode (default)\n"
      << "  --opencode, --connected        Force OpenCode connected bridge mode\n"
      << "  --mode <standalone|opencode>   Explicitly specify runtime execution mode\n"
      << "  --security <level>             Set security level (strict, balanced, permissive)\n"
      << "  --strict                       Enable strict security sandbox lockdown\n"
      << "  --model, -m <model_name>       Set primary model for task/session "
         "(any custom/cloud model)\n"
      << "  --base-url, --endpoint <url>   Set custom OpenAI-compatible API "
         "base URL (vLLM, LM Studio, etc.)\n"
      << "  --api-key <key>                Set API key for remote providers\n"
      << "  --file, -f <path>              Attach specific file context to "
         "prompt/session\n"
      << "  --files <p1,p2,...>            Attach multiple comma-separated "
         "files\n"
      << "  --temperature, -t <float>      Set model generation temperature "
         "(0.0 to 2.0)\n"
      << "  --max-tokens, --tokens <int>   Set max generation token limit\n"
      << "  --opencode                     Select OpenCode as primary "
         "provider\n"
      << "  --claude                       Select Claude Code CLI as primary "
         "provider\n"
      << "  --llama, --llamacpp            Select native llama.cpp server as primary provider\n"
      << "  --ollama                       Select Ollama as primary provider\n"
      << "  --openrouter                   Select OpenRouter as primary "
         "provider\n"
      << "  --mock                         Force built-in offline test mock "
         "provider\n"
      << "  --no-mock-fallback             Disable automatic fallback to "
         "offline mock\n"
      << "  --verbose, -v                  Enable debug output\n\n";
  TerminalUI::printFooter();
}

std::string joinArgs(const std::vector<std::string> &args,
                     size_t startIdx = 0) {
  std::string s;
  for (size_t i = startIdx; i < args.size(); ++i) {
    if (!s.empty())
      s += " ";
    s += args[i];
  }
  return s;
}

} // namespace

int main(int argc, char *argv[]) {
  std::string configPath = "config.json";
  if (!std::filesystem::exists(configPath) &&
      std::filesystem::exists("../config.json")) {
    configPath = "../config.json";
  }
  Config config = Config::loadFromFile(configPath);

  std::vector<std::string> rawArgs;
  std::vector<std::string> attachedCliFiles;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--verbose" || arg == "-v") {
      Logger::instance().setVerbose(true);
    } else if (arg == "--mock") {
      config.test_mock_enabled = true;
      config.default_provider = "TestMock";
    } else if (arg == "--security" && i + 1 < argc) {
      config.setSecurityLevel(argv[++i]);
    } else if (arg == "--strict") {
      config.setSecurityLevel("strict");
    } else if (arg == "--fast") {
      config.setProcessingLevel("fast");
    } else if (arg == "--perfect" || arg == "--perfect-boi" || arg == "--perfect_boi") {
      config.setProcessingLevel("perfect_boi");
    } else if (arg == "--level" && i + 1 < argc) {
      config.setProcessingLevel(argv[++i]);
    } else if (arg == "--cuda") {
      config.setCuda(true);
    } else if (arg == "--no-cuda" || arg == "--cpu") {
      config.setForceCpu(true);
    } else if (arg == "--standalone") {
      config.setMode("standalone");
      config.default_provider = "auto";
    } else if (arg == "--opencode" || arg == "--connected") {
      config.setMode("opencode");
      config.default_provider = "OpenCode";
    } else if (arg == "--mode" && i + 1 < argc) {
      config.setMode(argv[++i]);
      if (config.isOpenCodeConnected()) {
        config.default_provider = "OpenCode";
      }
    } else if (arg == "--claude") {
      config.default_provider = "ClaudeCode";
    } else if (arg == "--llama" || arg == "--llamacpp") {
      config.default_provider = "llama.cpp";
    } else if (arg == "--ollama") {
      config.default_provider = "Ollama";
    } else if (arg == "--openrouter") {
      config.default_provider = "OpenRouter";
    } else if (arg == "--no-mock-fallback") {
      config.auto_mock_fallback = false;
    } else if ((arg == "--file" || arg == "-f") && i + 1 < argc) {
      attachedCliFiles.push_back(argv[++i]);
    } else if (arg == "--files" && i + 1 < argc) {
      std::string fileList = argv[++i];
      std::stringstream fss(fileList);
      std::string item;
      while (std::getline(fss, item, ',')) {
        if (!item.empty())
          attachedCliFiles.push_back(item);
      }
    } else if ((arg == "--temperature" || arg == "-t") && i + 1 < argc) {
      try {
        config.default_temperature = std::stof(argv[++i]);
      } catch (...) {
      }
    } else if ((arg == "--max-tokens" || arg == "--tokens" ||
                arg == "--max_tokens") &&
               i + 1 < argc) {
      try {
        config.default_max_tokens = std::stoi(argv[++i]);
      } catch (...) {
      }
    } else if ((arg == "--model" || arg == "-m") && i + 1 < argc) {
      config.default_model = argv[++i];
    } else if ((arg == "--base-url" || arg == "--endpoint" || arg == "--url") &&
               i + 1 < argc) {
      config.openai_base_url = argv[++i];
      config.openrouter_base_url = config.openai_base_url;
    } else if ((arg == "--api-key" || arg == "--key") && i + 1 < argc) {
      config.openai_api_key = argv[++i];
      config.openrouter_api_key = config.openai_api_key;
    } else if (arg == "--help" || arg == "-h" || arg == "help") {
      if (i + 1 < argc && (std::string(argv[i + 1]) == "session" ||
                           std::string(argv[i + 1]) == "guide")) {
        ArenaAgent agent(config);
        ReplEngine repl(agent);
        repl.handleHelpSession("");
        return 0;
      }
      printUsage();
      return 0;
    } else if (arg == "--mcp") {
      rawArgs.push_back("mcp");
    } else {
      rawArgs.push_back(arg);
    }
  }

  ArenaAgent agent(config);
  for (const auto &f : attachedCliFiles) {
    agent.attachFile(f);
  }

  // Autonomously index models across llama.cpp GGUF and providers
  if (!config.test_mock_enabled && !config.isOpenCodeConnected()) {
    agent.getRouter().refreshModels();
  }

  // Autonomously index models across llama.cpp GGUF and providers
  if (!config.test_mock_enabled && !config.isOpenCodeConnected()) {
    agent.getRouter().refreshModels();
  }

  if (rawArgs.empty()) {
    // Launch Interactive REPL
    ReplEngine repl(agent);
    repl.run();
    return 0;
  }

  const std::string &cmd = rawArgs[0];

  // Command Dispatch Table
  using Handler = std::function<int(const std::vector<std::string> &)>;
  std::map<std::string, Handler> dispatchTable;

  dispatchTable["sessions"] = [&](const auto &) {
    auto sessions = Session::listSessions("sessions");
    TerminalUI::printSessionsList(sessions);
    return 0;
  };

  dispatchTable["config"] = [&](const auto &) {
    TerminalUI::printConfigView(config.toJson());
    return 0;
  };

  dispatchTable["mode"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string sub = joinArgs(args, 1);
    repl.handleMode(sub);
    return 0;
  };

  dispatchTable["status"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleStatus(joinArgs(args, 1));
    return 0;
  };

  dispatchTable["security"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string subArgs = joinArgs(args, 1);
    repl.handleSecurity(subArgs);
    return 0;
  };

  dispatchTable["audit"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string subArgs = joinArgs(args, 1);
    repl.handleAudit(subArgs);
    return 0;
  };

  dispatchTable["level"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleLevel(joinArgs(args, 1));
    return 0;
  };

  dispatchTable["profile"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleProfile(joinArgs(args, 1));
    return 0;
  };

  dispatchTable["services"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleServices(joinArgs(args, 1));
    return 0;
  };

  dispatchTable["daemon"] = dispatchTable["services"];

  dispatchTable["chat"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string persona = (args.size() > 1) ? args[1] : "witty";
    repl.startChatSession(persona);
    return 0;
  };

  dispatchTable["talk"] = dispatchTable["chat"];
  dispatchTable["banter"] = dispatchTable["chat"];

  dispatchTable["connect"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string target = (args.size() > 1) ? args[1] : "opencode";
    repl.handleConnect(target);
    return 0;
  };

  dispatchTable["disconnect"] = [&](const auto &) {
    ReplEngine repl(agent);
    repl.handleDisconnect("");
    return 0;
  };

  dispatchTable["models"] = [&](const auto &args) {
    ReplEngine repl(agent);
    if (args.size() > 1) {
      repl.handleModel(joinArgs(args, 1));
    } else {
      agent.getRouter().refreshModels();
      agent.getRouter().refreshModels();
      auto models = agent.getRouter().getAllAvailableModels();
      TerminalUI::printModelsList(models);
    }
    return 0;
  };

  dispatchTable["model"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleModel(joinArgs(args, 1));
    return 0;
  };

  dispatchTable["providers"] = [&](const auto &) {
    std::cout << "\n====================================================\n"
              << "              AI MODEL PROVIDERS STATUS              \n"
              << "====================================================\n";
    agent.getRouter().refreshModels();
    agent.getRouter().refreshModels();
    auto router = agent.getRouter();
    auto llamacpp = router.getProviderByName("llama.cpp");
    auto oc = router.getProviderByName("OpenCode");
    auto claude = router.getProviderByName("ClaudeCode");
    auto ollama = router.getProviderByName("Ollama");
    auto openrouter = router.getProviderByName("OpenRouter");
    auto mock = router.getProviderByName("TestMock");

    auto printStatus = [](const std::string &name, auto prov) {
      bool ok = prov && prov->available();
      std::cout << "  * " << std::left;
      std::cout.width(18);
      std::cout << name << ": "
                << (ok ? "\033[32mONLINE\033[0m" : "\033[33mOFFLINE\033[0m")
                << "\n";
    };

    printStatus("llama.cpp (Native)", llamacpp);
    printStatus("OpenCode CLI", oc);
    printStatus("Claude Code CLI", claude);
    printStatus("Ollama (Local)", ollama);
    printStatus("OpenRouter", openrouter);
    printStatus("Offline Mock", mock);
    std::cout
        << "----------------------------------------------------\n"
        << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
        << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n\n";
    return 0;
  };

  dispatchTable["opencode"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string subArgs = joinArgs(args, 1);
    repl.handleOpenCode(subArgs);
    return 0;
  };

  dispatchTable["unsloth"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string subArgs = joinArgs(args, 1);
    repl.handleUnsloth(subArgs);
    return 0;
  };

  dispatchTable["abilities"] = [&](const auto &) {
    auto matrix = agent.getRouter().getAbilityMatrix();
    TerminalUI::printAbilityMatrix(matrix);
    return 0;
  };

  dispatchTable["files"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string subArgs = joinArgs(args, 1);
    repl.handleFiles(subArgs);
    return 0;
  };
  dispatchTable["file"] = dispatchTable["files"];

  dispatchTable["think"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string subArgs = joinArgs(args, 1);
    repl.handleThink(subArgs);
    return 0;
  };
  dispatchTable["duel"] = dispatchTable["think"];

  dispatchTable["help-session"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string subArgs = joinArgs(args, 1);
    repl.handleHelpSession(subArgs);
    return 0;
  };
  dispatchTable["guide"] = dispatchTable["help-session"];

  dispatchTable["metrics"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleMetrics(joinArgs(args, 1));
    return 0;
  };
  dispatchTable["telemetry"] = dispatchTable["metrics"];

  dispatchTable["artifacts"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string arg = (args.size() > 1) ? args[1] : "";
    repl.handleArtifacts(arg);
    return 0;
  };

  dispatchTable["skills"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string arg = (args.size() > 1) ? args[1] : "";
    repl.handleSkills(arg);
    return 0;
  };

  dispatchTable["rules"] = [&](const auto &) {
    ReplEngine repl(agent);
    repl.handleRules("");
    return 0;
  };

  dispatchTable["boards"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string filter = (args.size() > 1) ? args[1] : "";
    repl.handleBoards(filter);
    return 0;
  };

  dispatchTable["board"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string id = (args.size() > 1) ? args[1] : "";
    repl.handleBoard(id);
    return 0;
  };

  dispatchTable["components"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string filter = (args.size() > 1) ? args[1] : "";
    repl.handleComponents(filter);
    return 0;
  };

  dispatchTable["scaffold"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleScaffold(joinArgs(args, 1));
    return 0;
  };

  dispatchTable["scaffold-hardware"] = dispatchTable["scaffold"];

  dispatchTable["qos"] = [&](const auto &args) {
    ReplEngine repl(agent);
    std::string path = (args.size() > 1) ? args[1] : "";
    repl.handleQoS(path);
    return 0;
  };

  dispatchTable["calculate"] = [&](const auto &args) {
    ReplEngine repl(agent);
    repl.handleCalculate(joinArgs(args, 1));
    return 0;
  };

  dispatchTable["plan"] = [&](const auto &args) {
    if (args.size() < 2) {
      std::cerr
          << "Error: Missing task for plan.\nUsage: arena plan \"<task>\"\n";
      return 1;
    }
    std::string task = joinArgs(args, 1);
    ReplEngine repl(agent);
    repl.handlePlan(task);
    return 0;
  };

  dispatchTable["goal"] = [&](const auto &args) {
    if (args.size() < 2) {
      std::cerr
          << "Error: Missing task for goal.\nUsage: arena goal \"<task>\"\n";
      return 1;
    }
    std::string task = joinArgs(args, 1);
    ReplEngine repl(agent);
    repl.handleGoal(task);
    return 0;
  };

  dispatchTable["grill-me"] = [&](const auto &args) {
    std::string topic = joinArgs(args, 1);
    ReplEngine repl(agent);
    repl.handleGrillMe(topic);
    return 0;
  };

  dispatchTable["resume"] = [&](const auto &args) {
    if (args.size() < 2) {
      std::cerr
          << "Error: Missing session ID.\nUsage: arena resume <session-id>\n";
      return 1;
    }
    std::string sessionId = args[1];
    if (!agent.resumeSession(sessionId)) {
      return 1;
    }
    bool success = agent.run();
    return success ? 0 : 1;
  };

  dispatchTable["mcp"] = [&](const auto &) {
    MCPServer mcp(agent);
    mcp.run();
    return 0;
  };
  dispatchTable["--mcp"] = dispatchTable["mcp"];

  dispatchTable["verify"] = [&](const auto &) {
    MCPServer mcp(agent);
    auto res = mcp.handleRequest({
      {"jsonrpc", "2.0"},
      {"id", 1},
      {"method", "tools/call"},
      {"params", {
        {"name", "arena_verify"},
        {"arguments", nlohmann::json::object()}
      }}
    });
    if (res.contains("result") && res["result"].contains("content")) {
      for (const auto& item : res["result"]["content"]) {
        std::cout << item.value("text", "") << "\n";
      }
    }
    return 0;
  };

  dispatchTable["test"] = [&](const auto &) {
    std::cout << "Executing self-test suite (run 'ctest' or 'arena_test' for "
                 "binary tests)...\n";
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

  // If not a subcommand keyword and doesn't start with '-', treat all args as a
  // direct task string
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
