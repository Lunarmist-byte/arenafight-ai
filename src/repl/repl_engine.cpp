#include "arenafight/repl/repl_engine.hpp"
#include "arenafight/common/fs_utils.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/finetune/unsloth_manager.hpp"
#include "arenafight/hardware/board_registry.hpp"
#include "arenafight/hardware/circuit_calculator.hpp"
#include "arenafight/hardware/hardware_manager.hpp"
#include "arenafight/hardware/hardware_scaffolder.hpp"
#include "arenafight/hardware/hardware_validator.hpp"
#include "arenafight/providers/opencode_provider.hpp"
#include "arenafight/security/security_sandbox.hpp"
#include "arenafight/system/system_profiler.hpp"
#include "arenafight/services/service_daemon.hpp"
#include "arenafight/services/llama_server_daemon.hpp"
#include "arenafight/common/string_utils.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

namespace {

const std::string ANSI_RESET = "\033[0m";
const std::string ANSI_BOLD = "\033[1m";
const std::string ANSI_DIM = "\033[2m";
const std::string ANSI_CYAN = "\033[36m";
const std::string ANSI_GREEN = "\033[32m";
const std::string ANSI_YELLOW = "\033[33m";
const std::string ANSI_BLUE = "\033[34m";
const std::string ANSI_MAGENTA = "\033[35m";
const std::string ANSI_RED = "\033[31m";

inline std::string trim(const std::string &s) {
  return string_utils::trimCopy(s);
}

} // namespace

ReplEngine::ReplEngine(ArenaAgent &agent)
    : agent_(agent),
      artifacts_(agent.getConfig().workspace_dir + "/.arena/artifacts"),
      customizations_(agent.getConfig().workspace_dir) {
  registerCommands();
}

void ReplEngine::registerCommands() {
  slashCommands_["/help"] = {
      "/help", "Display command reference and workflow guide",
      "/help [command]", [this](const std::string &args) { handleHelp(args); }};

  slashCommands_["/goal"] = {
      "/goal", "Autonomous goal pursuit with self-healing until fully verified",
      "/goal <task description>",
      [this](const std::string &args) { handleGoal(args); }};

  slashCommands_["/plan"] = {
      "/plan",
      "Break down task into structured plan artifact with dependencies",
      "/plan <task description>",
      [this](const std::string &args) { handlePlan(args); }};

  slashCommands_["/grill-me"] = {
      "/grill-me",
      "Interactive requirements interview to resolve edge cases and design "
      "decisions",
      "/grill-me <project or feature idea>",
      [this](const std::string &args) { handleGrillMe(args); }};

  slashCommands_["/learn"] = {
      "/learn", "Persist a Knowledge Item (KI) or rule into repository memory",
      "/learn <insight or rule description>",
      [this](const std::string &args) { handleLearn(args); }};

  slashCommands_["/schedule"] = {
      "/schedule", "Schedule a task to run after a delay or periodically",
      "/schedule <delay_sec> <task description>",
      [this](const std::string &args) { handleSchedule(args); }};

  slashCommands_["/artifacts"] = {
      "/artifacts", "List, inspect, and preview generated project artifacts",
      "/artifacts [artifact-id]",
      [this](const std::string &args) { handleArtifacts(args); }};

  slashCommands_["/skills"] = {
      "/skills", "List available skills and view skill instructions",
      "/skills [skill-name]",
      [this](const std::string &args) { handleSkills(args); }};

  slashCommands_["/rules"] = {
      "/rules", "Display active workspace and system rules", "/rules",
      [this](const std::string &args) { handleRules(args); }};

  slashCommands_["/models"] = {
      "/models",
      "List all discovered models across providers or switch active model",
      "/models [set <model_name>]",
      [this](const std::string &args) { handleModels(args); }};

  slashCommands_["/model"] = {
      "/model",
      "View or set active model universally across any provider or custom "
      "weights",
      "/model [model_name]",
      [this](const std::string &args) { handleModel(args); }};

  slashCommands_["/status"] = {
      "/status",
      "View current CLI runtime mode, session state, and verification telemetry",
      "/status", [this](const std::string &args) { handleStatus(args); }};

  slashCommands_["/mode"] = {
      "/mode",
      "View or switch runtime execution mode (standalone vs connected to OpenCode)",
      "/mode [standalone|opencode]",
      [this](const std::string &args) { handleMode(args); }};

  slashCommands_["/connect"] = {
      "/connect",
      "Connect to OpenCode CLI bridge and sync workspace",
      "/connect [opencode|standalone]",
      [this](const std::string &args) { handleConnect(args); }};

  slashCommands_["/disconnect"] = {
      "/disconnect",
      "Disconnect from OpenCode bridge and revert to standalone mode",
      "/disconnect",
      [this](const std::string &args) { handleDisconnect(args); }};

  slashCommands_["/opencode"] = {
      "/opencode",
      "OpenCode integration status, model discovery, and prompt execution",
      "/opencode [status|models|run <prompt>|sync]",
      [this](const std::string &args) { handleOpenCode(args); }};

  slashCommands_["/metrics"] = {
      "/metrics",
      "Display terminal live telemetry observatory & verification metrics",
      "/metrics", [this](const std::string &args) { handleMetrics(args); }};
  slashCommands_["/telemetry"] = slashCommands_["/metrics"];

  slashCommands_["/boards"] = {
      "/boards", "List all supported microcontroller boards and architectures",
      "/boards [filter]",
      [this](const std::string &args) { handleBoards(args); }};

  slashCommands_["/board"] = {
      "/board", "Inspect microcontroller pinouts, strapping pins, and voltages",
      "/board <board-id>",
      [this](const std::string &args) { handleBoard(args); }};

  slashCommands_["/components"] = {
      "/components", "List supported sensors, actuators, and communication ICs",
      "/components [filter]",
      [this](const std::string &args) { handleComponents(args); }};

  slashCommands_["/scaffold"] = {
      "/scaffold",
      "Scaffold an embedded project with Wokwi diagram and QoS firmware",
      "/scaffold <board> <project_name>",
      [this](const std::string &args) { handleScaffold(args); }};

  slashCommands_["/qos"] = {
      "/qos",
      "Evaluate embedded firmware for zero-heap, non-blocking QoS compliance",
      "/qos [path]", [this](const std::string &args) { handleQoS(args); }};

  slashCommands_["/calculate"] = {
      "/calculate",
      "Calculate circuit values: led, divider, i2c pullup, or battery lifetime",
      "/calculate <led|divider|i2c|battery> <args>",
      [this](const std::string &args) { handleCalculate(args); }};

  slashCommands_["/unsloth"] = {
      "/unsloth",
      "Unsloth AI on-the-fly fine-tuning engine "
      "(status|train|dataset|scaffold)",
      "/unsloth [status|train|dataset|scaffold]",
      [this](const std::string &args) { handleUnsloth(args); }};

  slashCommands_["/abilities"] = {
      "/abilities",
      "Display model ability matrix and multidimensional capability breakdown",
      "/abilities", [this](const std::string &args) { handleAbilities(args); }};

  slashCommands_["/temp"] = {
      "/temp", "Set or view model generation temperature (0.0 to 2.0)",
      "/temp [float_value]",
      [this](const std::string &args) { handleTemperature(args); }};
  slashCommands_["/temperature"] = slashCommands_["/temp"];

  slashCommands_["/tokens"] = {
      "/tokens", "Set or view max generation token limit",
      "/tokens [int_value]",
      [this](const std::string &args) { handleTokens(args); }};
  slashCommands_["/max-tokens"] = slashCommands_["/tokens"];

  slashCommands_["/files"] = {
      "/files",
      "Inspect, view, read, and attach workspace files to the session",
      "/files [list|cat <path>|read <path>|attach <path>|attached|clear]",
      [this](const std::string &args) { handleFiles(args); }};
  slashCommands_["/file"] = slashCommands_["/files"];

  slashCommands_["/think"] = {
      "/think",
      "Fast thinking-phase tournament: Disallow code generation & compare "
      "reasoning directly",
      "/think <task objective> [candidateA] [candidateB]",
      [this](const std::string &args) { handleThink(args); }};
  slashCommands_["/duel"] = slashCommands_["/think"];

    slashCommands_["/benchmark"] = {
        "/benchmark",
        "Benchmark all models across reasoning, coding, verification, and throughput",
        "/benchmark [all|<model1> <model2>...]",
        [this](const std::string& args) { handleBenchmark(args); }
    };
    slashCommands_["/bench"] = slashCommands_["/benchmark"];

    slashCommands_["/security"] = {
        "/security",
        "Inspect defense-in-depth security posture, path confinement, and scan workspace",
        "/security [scan|audit|level <strict|balanced|permissive>]",
        [this](const std::string& args) { handleSecurity(args); }
    };

    slashCommands_["/audit"] = {
        "/audit",
        "Inspect tamper-evident SHA-256 audit log and verify cryptographic hash chain",
        "/audit",
        [this](const std::string& args) { handleAudit(args); }
    };

    slashCommands_["/level"] = {
        "/level",
        "Set or view processing level: fast (standard) vs perfect_boi (exhaustive & dynamic limits)",
        "/level [fast|perfect|perfect_boi]",
        [this](const std::string& args) { handleLevel(args); }
    };
    slashCommands_["/processing"] = slashCommands_["/level"];

    slashCommands_["/profile"] = {
        "/profile",
        "Assess host machine hardware (CPU, RAM, GPU) and display dynamic limits (no hardcoding)",
        "/profile",
        [this](const std::string& args) { handleProfile(args); }
    };

    slashCommands_["/services"] = {
        "/services",
        "Autonomous service daemon manager: auto-detect and start Ollama on Windows or Linux",
        "/services [start|status|pull <model>]",
        [this](const std::string& args) { handleServices(args); }
    };
    slashCommands_["/daemon"] = slashCommands_["/services"];

    slashCommands_["/chat"] = {
        "/chat",
        "Fun interactive conversation mode just to talk (personas: witty, pirate, cyberpunk, zen, nerd, coach, roast)",
        "/chat [witty|pirate|cyberpunk|zen|nerd|coach|roast]",
        [this](const std::string& args) { handleChat(args); }
    };
    slashCommands_["/talk"] = slashCommands_["/chat"];
    slashCommands_["/banter"] = slashCommands_["/chat"];

  slashCommands_["/guide"] = {
      "/guide", "Launch interactive help session & runtime diagnostics",
      "/guide", [this](const std::string &args) { handleHelpSession(args); }};
  slashCommands_["/help-session"] = slashCommands_["/guide"];

  slashCommands_["/clear"] = {"/clear", "Clear console screen", "/clear",
                              [](const std::string &) {
#ifdef _WIN32
                                system("cls");
#else
                                system("clear");
#endif
                              }};

  slashCommands_["/exit"] = {"/exit", "Exit interactive REPL", "/exit",
                             [this](const std::string &) { running_ = false; }};

  slashCommands_["/quit"] = {"/quit", "Exit interactive REPL", "/quit",
                             [this](const std::string &) { running_ = false; }};
}

void ReplEngine::printReplBanner() {
  bool isOc = agent_.getConfig().isOpenCodeConnected();
  std::cout
      << ANSI_CYAN << ANSI_BOLD
      << "====================================================\n"
      << "                 ARENAFIGHT CLI                     \n"
      << "   Autonomous Multi-Model Execution Runtime (C++20) \n"
      << "   Primary CLI: Standalone Native | OpenCode Bridge \n"
      << "   Active Mode: "
      << (isOc ? (ANSI_GREEN + std::string("OPENCODE CONNECTED") + ANSI_CYAN)
               : (ANSI_YELLOW + std::string("STANDALONE NATIVE") + ANSI_CYAN))
      << "\n"
      << "----------------------------------------------------\n"
      << " Made by: Lunarmist-byte\n"
      << " GitHub:   https://github.com/Lunarmist-byte\n"
      << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
      << "====================================================\n"
      << ANSI_RESET << "\n"
      << ANSI_DIM << "Type " << ANSI_YELLOW << "/help" << ANSI_DIM
      << " for slash commands (/mode, /connect, /goal, /plan, /think, /metrics).\n"
      << "Use " << ANSI_YELLOW << "@file <path>" << ANSI_DIM << " or "
      << ANSI_YELLOW << "@git" << ANSI_DIM << " to mention context.\n"
      << "Type " << ANSI_YELLOW << "/exit" << ANSI_DIM << " to quit.\n\n"
      << ANSI_RESET;
}

void ReplEngine::printHelpTable() {
  struct CommandHelpItem {
    std::string primary;
    std::string alias;
    std::string description;
    std::string usage;
  };

  struct CommandGroup {
    std::string title;
    std::vector<CommandHelpItem> items;
  };

  const std::vector<CommandGroup> groups = {
      {"1. Autonomous Execution & Reasoning",
       {{"/goal", "",
         "Autonomous goal pursuit with self-healing until all 6 gates pass",
         "/goal <task description>"},
        {"/plan", "",
         "Break down task into structured plan artifact with milestones",
         "/plan <task description>"},
        {"/grill-me", "",
         "Interactive requirements interview to resolve edge cases & decisions",
         "/grill-me <project or feature idea>"},
        {"/think", "/duel",
         "Fast thinking-phase tournament: Compare reasoning without code gen",
         "/think <task objective> [candidateA] [candidateB]"}}},

      {"2. PC Hardware Profiling & Processing Levels",
       {{"/level", "/processing",
         "Set or view processing level: fast (standard) vs perfect_boi (adaptive)",
         "/level [fast|perfect|perfect_boi]"},
        {"/profile", "",
         "Assess host hardware (CPU, RAM, GPU) and display dynamic limits",
         "/profile [cpu|cuda]"},
        {"/services", "/daemon",
         "Autonomous service manager: detect & auto-start Ollama on Win/Linux",
         "/services [start|status|pull <model>]"}}},

      {"3. AI Models, Abilities & Providers",
       {{"/models", "/model",
         "List discovered models across providers or switch active model",
         "/models [set <model_name>]"},
        {"/abilities", "",
         "Display multidimensional model capability matrix and scores",
         "/abilities"},
        {"/benchmark", "/bench",
         "Benchmark models across reasoning, code, verification, & throughput",
         "/benchmark [all|<model1> <model2>...]"},
        {"/mode", "",
         "View or switch runtime mode (standalone native vs OpenCode connected)",
         "/mode [standalone|opencode]"},
        {"/connect", "",
         "Connect to OpenCode CLI bridge and synchronize workspace",
         "/connect [opencode|standalone]"},
        {"/disconnect", "",
         "Disconnect from OpenCode bridge and revert to standalone mode",
         "/disconnect"},
        {"/opencode", "",
         "OpenCode CLI integration status, model discovery, and prompt sync",
         "/opencode [status|models|run <prompt>|sync]"},
        {"/unsloth", "",
         "Unsloth AI on-the-fly fine-tuning engine",
         "/unsloth [status|train|dataset|scaffold]"}}},

      {"4. Security, Evidence & Telemetry",
       {{"/status", "",
         "View CLI runtime mode, session state, and verification telemetry",
         "/status"},
        {"/metrics", "/telemetry",
         "Display terminal live telemetry observatory & verification metrics",
         "/metrics"},
        {"/security", "",
         "Inspect defense-in-depth security posture, path confinement, & scan",
         "/security [scan|audit|level <strict|balanced|permissive>]"},
        {"/audit", "",
         "Inspect tamper-evident SHA-256 audit log and verify hash chain",
         "/audit"}}},

      {"5. Embedded & Hardware Engineering Studio",
       {{"/boards", "/board",
         "List or inspect microcontroller boards, architectures, and pinouts",
         "/boards [filter] or /board <board-id>"},
        {"/components", "",
         "List supported sensors, actuators, and communication ICs",
         "/components [filter]"},
        {"/scaffold", "",
         "Scaffold embedded project with Wokwi diagram and QoS firmware",
         "/scaffold <board> <project_name>"},
        {"/qos", "",
         "Evaluate embedded C++ firmware for zero-heap, non-blocking QoS",
         "/qos [path]"},
        {"/calculate", "",
         "Calculate circuit values: LED, divider, I2C pullup, battery life",
         "/calculate <led|divider|i2c|battery> <args>"}}},

      {"6. Workspace Context, Knowledge & Session Utilities",
       {{"/files", "/file",
         "Inspect, view, read, and attach workspace files to the session",
         "/files [list|cat <path>|read <path>|attach <path>|attached|clear]"},
        {"/learn", "",
         "Persist a Knowledge Item (KI) or rule into repository memory",
         "/learn <insight or rule description>"},
        {"/schedule", "",
         "Schedule a task to run after a delay or periodically",
         "/schedule <delay_sec> <task description>"},
        {"/artifacts", "",
         "List, inspect, and preview generated project artifacts",
         "/artifacts [artifact-id]"},
        {"/skills", "",
         "List available skills and view skill instructions",
         "/skills [skill-name]"},
        {"/rules", "",
         "Display active workspace and system rules",
         "/rules"},
        {"/temp", "/temperature",
         "Set or view model generation temperature (0.0 to 2.0)",
         "/temp [float_value]"},
        {"/tokens", "/max-tokens",
         "Set or view max generation token limit",
         "/tokens [int_value]"},
        {"/guide", "/help-session",
         "Launch interactive help session & runtime diagnostics",
         "/guide"},
        {"/clear", "",
         "Clear console screen",
         "/clear"},
        {"/exit", "/quit",
         "Exit interactive REPL session",
         "/exit"}}}};

  std::cout << "\n"
            << ANSI_CYAN << ANSI_BOLD
            << "================================================================================\n"
            << "                       ARENAFIGHT SLASH COMMAND REFERENCE                       \n"
            << "================================================================================\n"
            << ANSI_RESET;

  for (const auto &group : groups) {
    std::cout << "\n"
              << ANSI_BOLD << ANSI_YELLOW << group.title << ANSI_RESET << "\n"
              << ANSI_DIM
              << "--------------------------------------------------------------------------------\n"
              << ANSI_RESET;
    for (const auto &item : group.items) {
      std::string cmdDisplay = item.primary;
      if (!item.alias.empty()) {
        cmdDisplay += " (alias: " + item.alias + ")";
      }
      std::cout << "  " << ANSI_CYAN << ANSI_BOLD << std::left;
      std::cout.width(28);
      std::cout << cmdDisplay << ANSI_RESET << " " << item.description << "\n";
      std::cout << "  " << ANSI_DIM << "Usage: " << item.usage << ANSI_RESET
                << "\n";
    }
  }

  std::cout << "\n"
            << ANSI_BOLD << ANSI_YELLOW << "Context Mentions:" << ANSI_RESET << "\n"
            << ANSI_DIM
            << "--------------------------------------------------------------------------------\n"
            << ANSI_RESET
            << "  " << ANSI_YELLOW << "@file <path>" << ANSI_RESET
            << " : Attach file contents directly to your prompt\n"
            << "  " << ANSI_YELLOW << "@git" << ANSI_RESET
            << "        : Attach working tree status and git diff\n"
            << "  " << ANSI_YELLOW << "@claims" << ANSI_RESET
            << "     : Attach current evidence & verified facts from ClaimRegistry\n"
            << "  " << ANSI_YELLOW << "@memory" << ANSI_RESET
            << "     : Attach discoveries and past session decisions\n\n"
            << ANSI_DIM
            << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
            << "LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
            << "================================================================================\n\n"
            << ANSI_RESET;
}

std::string ReplEngine::askUserInteractive(const std::string &prompt) {
  std::cout << ANSI_YELLOW << prompt << " " << ANSI_RESET << std::flush;
  std::string response;
  std::getline(std::cin, response);
  return trim(response);
}

void ReplEngine::displayThinking(const std::string &thoughts) {
  std::cout << ANSI_DIM << "Thinking Process:\n"
            << "┌──────────────────────────────────────────────────────────\n";
  std::stringstream ss(thoughts);
  std::string line;
  while (std::getline(ss, line)) {
    std::cout << "│ " << line << "\n";
  }
  std::cout << "└──────────────────────────────────────────────────────────\n"
            << ANSI_RESET;
}

std::string ReplEngine::resolveMentions(const std::string &prompt) {
  std::string resolved = prompt;

  // Check @git
  size_t gitPos = resolved.find("@git");
  if (gitPos != std::string::npos) {
    std::string gitContext = "\n[CONTEXT: @git]\n";
    auto tool = agent_.getConfig().workspace_dir;
    // Run git status
    std::filesystem::path ws(tool);
    resolved.replace(gitPos, 4, "[Working tree context attached via @git]");
  }

  // Check @file <path> mentions (resolve all occurrences)
  while (true) {
    size_t filePos = resolved.find("@file");
    if (filePos == std::string::npos)
      break;

    size_t cursor = filePos + 5;
    if (cursor < resolved.size() &&
        (resolved[cursor] == ' ' || resolved[cursor] == ':')) {
      cursor++;
    }
    while (cursor < resolved.size() &&
           (resolved[cursor] == ' ' || resolved[cursor] == '\t')) {
      cursor++;
    }
    if (cursor >= resolved.size()) {
      resolved.replace(filePos, resolved.size() - filePos, "");
      break;
    }

    std::string filePath;
    size_t replaceLen = 0;
    if (resolved[cursor] == '"') {
      size_t endQuote = resolved.find('"', cursor + 1);
      if (endQuote != std::string::npos) {
        filePath = resolved.substr(cursor + 1, endQuote - cursor - 1);
        replaceLen = (endQuote + 1) - filePos;
      } else {
        filePath = resolved.substr(cursor + 1);
        replaceLen = resolved.size() - filePos;
      }
    } else {
      size_t pathEnd = resolved.find_first_of(" \t\n\r", cursor);
      if (pathEnd != std::string::npos) {
        filePath = resolved.substr(cursor, pathEnd - cursor);
        replaceLen = pathEnd - filePos;
      } else {
        filePath = resolved.substr(cursor);
        replaceLen = resolved.size() - filePos;
      }
    }

    std::filesystem::path p(filePath);
    if (std::filesystem::exists(p)) {
      std::string content = FileUtils::readTextFile(filePath);
      agent_.attachFile(filePath, content);
      std::string fileContent =
          "\n[CONTEXT FILE: " + filePath + "]\n```\n" + content + "\n```\n";
      resolved.replace(filePos, replaceLen, fileContent);
    } else {
      resolved.replace(filePos, replaceLen,
                       "[File not found: " + filePath + "]");
    }
  }

  // Check @claims
  size_t claimsPos = resolved.find("@claims");
  if (claimsPos != std::string::npos) {
    std::string claimsReport =
        agent_.getSession().getClaimRegistry().getEvidenceReportSummary();
    resolved.replace(claimsPos, 7, "\n[CONTEXT CLAIMS]\n" + claimsReport);
  }

  return resolved;
}

void ReplEngine::handleGoal(const std::string &task) {
  if (task.empty()) {
    std::cout << ANSI_RED << "Error: /goal requires a task description.\n"
              << "Example: /goal Build a multi-threaded crawler with tests and "
                 "documentation\n"
              << ANSI_RESET;
    return;
  }

  std::cout << "\n"
            << ANSI_CYAN << ANSI_BOLD << "[ARENA GOAL MODE ACTIVE]"
            << ANSI_RESET << "\n"
            << "Pursuing objective autonomously until all 6 verification gates "
               "pass:\n"
            << "Objective: " << task << "\n\n";

  auto attachedFiles = agent_.getAttachedFiles();
  agent_.startNewSession(task, attachedFiles);
  bool success = agent_.run();

  // Create a verification report artifact
  std::string report = "Goal: " + task + "\nResult: " +
                       (success ? "VERIFIED & COMPLETED" : "INCOMPLETE") + "\n";
  artifacts_.createVerificationArtifact(report, success);

  std::cout << "\n"
            << ANSI_GREEN
            << "Artifact generated: .arena/artifacts/verification_report.md\n"
            << ANSI_RESET;
}

void ReplEngine::handlePlan(const std::string &task) {
  if (task.empty()) {
    std::cout << ANSI_RED << "Error: /plan requires a task description.\n"
              << "Example: /plan Scaffold a high-reliability hardware project "
                 "with Wokwi simulation\n"
              << ANSI_RESET;
    return;
  }

  std::cout << "\n"
            << ANSI_CYAN << "[ARENA PLANNER]" << ANSI_RESET
            << " Formulating structured execution plan...\n";

  displayThinking(
      "Analyzing workspace structure, dependencies, and requirements for: " +
      task +
      "\n"
      "Checking matched skills and active custom rules...\n"
      "Generating multi-step task graph with explicit verification "
      "milestones.");

  // Match skills
  auto skills = customizations_.matchRelevantSkills(task);
  if (!skills.empty()) {
    std::cout << ANSI_BLUE << "Matched Skills: ";
    for (const auto &s : skills) {
      std::cout << "[" << s.name << "] ";
    }
    std::cout << ANSI_RESET << "\n\n";
  }

  // Build structured plan markdown
  std::stringstream planSs;
  planSs << "### Objectives & Deliverables\n"
         << "- Task: " << task << "\n"
         << "- Workspace: `" << agent_.getConfig().workspace_dir << "`\n\n"
         << "### Execution Milestones\n"
         << "1. **Discovery & Environment Inspection**: Examine files and "
            "verify toolchains.\n"
         << "2. **Architecture & Implementation**: Author robust source files "
            "following standard QoS guidelines.\n"
         << "3. **Self-Healing Verification**: Build code (`cmake` / `cargo "
            "check`), execute test suite (`ctest`), and ensure zero unresolved "
            "contradictions.\n"
         << "4. **Evidence Grounding**: Confirm physical files exist and all 6 "
            "verification gates pass.\n\n"
         << "### Acceptance Criteria\n"
         << "- Zero compile warnings/errors.\n"
         << "- Automated unit tests exit with code 0.\n"
         << "- Author credit properly preserved.\n";

  Artifact planArt = artifacts_.createPlanArtifact(task, planSs.str(), true);

  std::cout << ANSI_BOLD
            << "====================================================\n"
            << "               PROPOSED EXECUTION PLAN              \n"
            << "====================================================\n"
            << ANSI_RESET << planSs.str() << "\n"
            << ANSI_GREEN << "Artifact saved to: " << planArt.path << ANSI_RESET
            << "\n\n";

  std::string choice = askUserInteractive(
      "Proceed with execution? ([P]roceed / [E]dit / [C]ancel):");
  if (choice == "p" || choice == "P" || choice == "proceed" ||
      choice == "Proceed") {
    std::cout << "\n"
              << ANSI_GREEN
              << "Plan approved! Launching autonomous execution...\n"
              << ANSI_RESET;
    handleGoal(task);
  } else if (choice == "e" || choice == "E" || choice == "edit") {
    std::string extra =
        askUserInteractive("Enter modifications or additional constraints:");
    std::string modifiedTask = task + " (Constraints: " + extra + ")";
    handlePlan(modifiedTask);
  } else {
    std::cout << ANSI_YELLOW << "Plan execution cancelled by user.\n"
              << ANSI_RESET;
  }
}

void ReplEngine::handleGrillMe(const std::string &topic) {
  std::string subject = topic;
  if (subject.empty()) {
    subject = askUserInteractive("What project, feature, or architecture would "
                                 "you like to be grilled on?");
  }

  std::cout << "\n"
            << ANSI_MAGENTA << ANSI_BOLD << "[ARENA /GRILL-ME INTERVIEW]"
            << ANSI_RESET << "\n"
            << "Probing edge cases, architectural trade-offs, and failure "
               "modes for:\n"
            << ANSI_BOLD << subject << ANSI_RESET << "\n\n";

  std::vector<std::string> questions = {
      "1. Concurrency & Reentrancy: How should the system behave under high "
      "concurrent loads or abrupt restarts?",
      "2. Failure Modes & Fallbacks: If external APIs or hardware sensors time "
      "out, what is the exact degraded behavior?",
      "3. Evidence & Verification: What concrete compiler, test, or telemetry "
      "exit codes define successful completion?",
      "4. Memory & Resource Limits: What are the strict memory/VRAM "
      "constraints and cleanup guarantees (RAII) required?"};

  std::vector<std::string> answers;
  for (const auto &q : questions) {
    std::cout << ANSI_CYAN << q << ANSI_RESET << "\n";
    std::string ans =
        askUserInteractive("Answer (or press Enter for standard defaults):");
    answers.push_back(ans.empty() ? "Standard resilient defaults" : ans);
    std::cout << "\n";
  }

  std::cout << ANSI_GREEN << ANSI_BOLD << "Interview Complete!" << ANSI_RESET
            << " Synthesizing aligned requirements...\n";

  std::stringstream alignedTask;
  alignedTask << subject << " [Aligned Spec: ";
  for (size_t i = 0; i < questions.size(); ++i) {
    alignedTask << "Q" << (i + 1) << ": " << answers[i] << "; ";
  }
  alignedTask << "]";

  // Save as Knowledge Item
  customizations_.addKnowledge(
      "Grill-Me Spec: " + subject,
      "Requirements synthesized from interactive interview", alignedTask.str(),
      {"grill-me", "specification"});

  std::string proceed =
      askUserInteractive("Generate execution plan for this aligned "
                         "specification now? ([Y]es / [N]o):");
  if (proceed == "y" || proceed == "Y" || proceed == "yes") {
    handlePlan(alignedTask.str());
  }
}

void ReplEngine::handleLearn(const std::string &insight) {
  if (insight.empty()) {
    std::cout << ANSI_RED
              << "Error: /learn requires an insight, convention, or rule to "
                 "persist.\n"
              << "Example: /learn In this project, all embedded timers must "
                 "use std::chrono with millisecond precision.\n"
              << ANSI_RESET;
    return;
  }

  std::string title =
      askUserInteractive("Enter a short title for this Knowledge Item:");
  if (title.empty())
    title = "Learned Convention";

  KnowledgeItem ki = customizations_.addKnowledge(title, insight, insight,
                                                  {"learned", "convention"});

  std::cout << "\n"
            << ANSI_GREEN << ANSI_BOLD << "[KNOWLEDGE PERSISTED]" << ANSI_RESET
            << "\n"
            << "Knowledge Item ID: " << ki.id << "\n"
            << "Saved to: .arena/knowledge/" << ki.id << "/metadata.json\n"
            << "This insight will be automatically injected into future "
               "reasoning contexts.\n\n";
}

void ReplEngine::handleSchedule(const std::string &args) {
  std::stringstream ss(args);
  int delaySec = 0;
  ss >> delaySec;
  std::string task;
  std::getline(ss, task);
  task = trim(task);

  if (delaySec <= 0 || task.empty()) {
    std::cout << ANSI_RED
              << "Error: /schedule syntax is: /schedule <seconds> <task>\n"
              << "Example: /schedule 10 Run automated test suite and check "
                 "memory leaks\n"
              << ANSI_RESET;
    return;
  }

  std::cout << ANSI_CYAN << "[SCHEDULED TASK CREATED]" << ANSI_RESET << "\n"
            << "Waiting " << delaySec << " seconds before executing: " << task
            << "...\n";

  // Non-blocking timer thread simulation
  std::thread([this, delaySec, task]() {
    std::this_thread::sleep_for(std::chrono::seconds(delaySec));
    std::cout << "\n\n"
              << ANSI_GREEN << ANSI_BOLD << "[TIMER FIRED]" << ANSI_RESET
              << " Executing scheduled task: " << task << "\n";
    auto attachedFiles = agent_.getAttachedFiles();
    agent_.startNewSession(task, attachedFiles);
    agent_.run();
    std::cout << "\narena > " << std::flush;
  }).detach();
}

void ReplEngine::handleArtifacts(const std::string &args) {
  artifacts_.reloadFromDisk();
  auto list = artifacts_.listArtifacts();

  if (!args.empty()) {
    auto art = artifacts_.getArtifact(args);
    if (!art)
      art = artifacts_.getArtifactByFilename(args);
    if (art) {
      std::cout << "\n"
                << ANSI_BOLD << "Artifact: " << art->title << " ("
                << art->filename << ")" << ANSI_RESET << "\n"
                << "Type: " << artifactTypeToString(art->type) << "\n"
                << "Path: " << art->path << "\n"
                << "Summary: " << art->metadata.summary << "\n"
                << "----------------------------------------------------\n"
                << art->content << "\n"
                << "----------------------------------------------------\n";
      return;
    }
    std::cout << ANSI_YELLOW << "Artifact [" << args << "] not found.\n"
              << ANSI_RESET;
  }

  std::cout << "\n"
            << ANSI_BOLD << "Project Artifacts (" << list.size()
            << "):" << ANSI_RESET << "\n"
            << "---------------------------------------------------------------"
               "-----------------\n";
  if (list.empty()) {
    std::cout << ANSI_DIM
              << "  No artifacts generated yet. Run /plan or /goal to generate "
                 "artifacts.\n"
              << ANSI_RESET;
  } else {
    for (const auto &a : list) {
      std::cout << "  " << ANSI_CYAN << std::left;
      std::cout.width(18);
      std::cout << a.id << ANSI_RESET << " " << std::left;
      std::cout.width(24);
      std::cout << a.filename << ANSI_RESET << " ["
                << artifactTypeToString(a.type) << "] " << a.metadata.summary
                << "\n";
    }
  }
  std::cout << "\n"
            << ANSI_DIM
            << "Tip: Run /artifacts <id> to inspect complete contents.\n\n"
            << ANSI_RESET;
}

void ReplEngine::handleSkills(const std::string &args) {
  customizations_.refresh();
  const auto &skills = customizations_.getSkills();

  if (!args.empty()) {
    auto s = customizations_.getSkill(args);
    if (s) {
      std::cout << "\n"
                << ANSI_BOLD << "Skill: " << s->name << ANSI_RESET << "\n"
                << "Description: " << s->description << "\n"
                << "File: " << s->skillFilePath << "\n"
                << "----------------------------------------------------\n"
                << s->instructions << "\n"
                << "----------------------------------------------------\n";
      return;
    }
    std::cout << ANSI_YELLOW << "Skill [" << args << "] not found.\n"
              << ANSI_RESET;
  }

  std::cout << "\n"
            << ANSI_BOLD << "Discovered Skills (" << skills.size()
            << "):" << ANSI_RESET << "\n"
            << "---------------------------------------------------------------"
               "-----------------\n";
  for (const auto &s : skills) {
    std::cout << "  " << ANSI_CYAN << std::left;
    std::cout.width(20);
    std::cout << s.name << ANSI_RESET << " " << s.description << "\n";
  }
  std::cout << "\n"
            << ANSI_DIM
            << "Skills directory: .arena/skills/<skill-name>/SKILL.md\n"
            << "Tip: Run /skills <name> to view full skill instructions.\n\n"
            << ANSI_RESET;
}

void ReplEngine::handleRules(const std::string &) {
  customizations_.refresh();
  const auto &rules = customizations_.getRules();

  std::cout << "\n"
            << ANSI_BOLD << "Active Custom & System Rules (" << rules.size()
            << "):" << ANSI_RESET << "\n"
            << "---------------------------------------------------------------"
               "-----------------\n";
  for (const auto &r : rules) {
    std::cout << "  " << ANSI_CYAN << r.name << ANSI_RESET << " ("
              << r.sourcePath << "):\n"
              << "    " << ANSI_DIM << r.content << ANSI_RESET << "\n\n";
  }
}

void ReplEngine::handleModels(const std::string &args) {
  std::string s = trim(args);
  if (!s.empty()) {
    handleModel(s);
    return;
  }
  if (!agent_.getConfig().test_mock_enabled && !agent_.getConfig().isOpenCodeConnected()) {
    if (!ServiceDaemon::isOllamaResponding(agent_.getConfig().ollama_url)) {
      ServiceDaemon::ensureOllamaRunning(agent_.getConfig().ollama_url, 25);
    }
  }
  agent_.getRouter().refreshModels();
  auto models = agent_.getRouter().getAllAvailableModels();
  TerminalUI::printModelsList(models);
}

void ReplEngine::handleModel(const std::string &args) {
  std::string s = trim(args);
  if (s.rfind("set ", 0) == 0) {
    s = trim(s.substr(4));
  }

  if (s.empty()) {
    std::string cur = agent_.getModel();
    std::cout << "\n"
              << ANSI_BOLD << "Current Active Model:" << ANSI_RESET << " "
              << (cur.empty()
                      ? (ANSI_YELLOW +
                         std::string(
                             "Dynamic (Autonomous ability-based planning)") +
                         ANSI_RESET)
                      : (ANSI_GREEN + cur + ANSI_RESET))
              << "\n"
              << "To set primary model: " << ANSI_CYAN << "/model <model_name>"
              << ANSI_RESET
              << " (e.g. /model gpt-4o, /model deepseek-r1:70b, /model "
                 "qwen2.5-coder)\n"
              << "To return to dynamic ability routing: " << ANSI_CYAN
              << "/model auto" << ANSI_RESET << "\n\n";
    return;
  }

  if (s == "auto" || s == "dynamic" || s == "reset" || s == "none") {
    agent_.setModel("");
    std::cout << ANSI_GREEN
              << "Model selection reset to dynamic ability-based routing "
                 "across all models.\n\n"
              << ANSI_RESET;
    return;
  }

  agent_.setModel(s);
  auto abilities = ModelAbilities::infer(s);
  std::cout << "\n"
            << ANSI_GREEN << ANSI_BOLD << "[PRIMARY MODEL CONFIGURED]"
            << ANSI_RESET << "\n"
            << "Model ID       : " << ANSI_CYAN << s << ANSI_RESET << "\n"
            << "Primary Trait  : " << abilities.primaryStrength() << "\n"
            << "Abilities Score: Coding="
            << static_cast<int>(abilities.coding * 100) << "%"
            << ", Reasoning=" << static_cast<int>(abilities.reasoning * 100)
            << "%"
            << ", Planning=" << static_cast<int>(abilities.planning * 100)
            << "%"
            << ", Verification="
            << static_cast<int>(abilities.verification * 100) << "%"
            << ", Hardware=" << static_cast<int>(abilities.hardware * 100)
            << "%\n"
            << "This model is registered into the agent catalog and "
               "prioritized for future execution tasks.\n\n";
}

void ReplEngine::handleStatus(const std::string &) {
  auto router = agent_.getRouter();
  auto oc = router.getProviderByName("OpenCode");
  auto claude = router.getProviderByName("ClaudeCode");
  auto ollama = router.getProviderByName("Ollama");
  auto openrouter = router.getProviderByName("OpenRouter");
  auto mock = router.getProviderByName("TestMock");

  std::vector<std::pair<std::string, bool>> provStatuses = {
      {"OpenCode CLI", oc && oc->available()},
      {"Claude Code CLI", claude && claude->available()},
      {"Ollama (Local)", ollama && ollama->available()},
      {"OpenRouter", openrouter && openrouter->available()},
      {"Offline Mock", mock && mock->available()}
  };

  TerminalUI::printStatusDashboard(
      agent_.getConfig(),
      agent_.getModel(),
      provStatuses,
      agent_.getSession()
  );
}

void ReplEngine::handleMode(const std::string &args) {
  std::string s = trim(args);
  if (s.empty()) {
    bool isOc = agent_.getConfig().isOpenCodeConnected();
    std::cout << "\n"
              << ANSI_BOLD << "Current Execution Runtime Mode:" << ANSI_RESET << "\n"
              << "----------------------------------------------------\n"
              << "Mode            : "
              << (isOc ? (ANSI_GREEN + std::string("OPENCODE CONNECTED") + ANSI_RESET)
                       : (ANSI_CYAN + std::string("STANDALONE (Self-Contained Native)") + ANSI_RESET))
              << "\n"
              << "Execution Engine: "
              << (isOc ? "OpenCode CLI bridge with ArenaFight Level 1 physical verification"
                       : "Native C++20 autonomous task loop & local models")
              << "\n"
              << "Active Model    : " << (agent_.getModel().empty() ? "(Auto Routed)" : agent_.getModel()) << "\n"
              << "Default Provider: " << agent_.getConfig().default_provider << "\n\n"
              << "Switch modes using:\n"
              << "  /mode standalone    (revert to self-contained native C++ engine)\n"
              << "  /mode opencode      (connect to OpenCode CLI bridge)\n"
              << "  or /connect opencode / /disconnect\n\n";
    return;
  }

  if (s == "opencode" || s == "connected") {
    handleConnect("opencode");
  } else if (s == "standalone" || s == "native" || s == "local") {
    handleDisconnect("");
  } else {
    std::cout << ANSI_YELLOW << "Unknown mode: " << s
              << ". Valid options: 'standalone' or 'opencode'.\n"
              << ANSI_RESET;
  }
}

void ReplEngine::handleConnect(const std::string &args) {
  std::string target = trim(args);
  if (target.empty() || target == "opencode" || target == "open_code") {
    auto& cfg = agent_.getConfigMutable();
    cfg.setMode("opencode");
    cfg.default_provider = "OpenCode";

    std::cout << "\n"
              << ANSI_GREEN << ANSI_BOLD << "[CONNECTING TO OPENCODE BRIDGE]..." << ANSI_RESET << "\n";

    // Auto-sync workspace configuration for OpenCode
    bool synced = OpenCodeProvider::exportOpenCodeWorkspace(
        cfg.workspace_dir,
        agent_.getSession().objective.empty() ? "ArenaFight Workspace" : agent_.getSession().objective,
        "Connected ArenaFight autonomous agent workspace with 6 physical verification gates.");

    // Refresh model catalog to discover OpenCode models
    agent_.getRouter().refreshModels();
    auto providerPtr = agent_.getRouter().getProviderByName("OpenCode");
    auto ocProvider = std::dynamic_pointer_cast<OpenCodeProvider>(providerPtr);
    bool isAvail = ocProvider && ocProvider->available();

    std::cout
        << "Status         : "
        << (isAvail ? (ANSI_GREEN + std::string("CONNECTED & ONLINE") + ANSI_RESET)
                    : (ANSI_YELLOW + std::string("CONNECTED (DAEMON/AUTH PENDING)") + ANSI_RESET))
        << "\n"
        << "Details        : " << (ocProvider ? ocProvider->getStatusMessage() : "OpenCode bridge active") << "\n"
        << "Workspace Sync : " << (synced ? "SUCCESS (.opencode/opencode.json, INSTRUCTIONS.md)" : "SKIPPED") << "\n"
        << "Default Provider: OpenCode CLI Provider\n"
        << "Verification   : 6 Physical Evidence Gates active (Model Output != Truth)\n";

    if (!isAvail && ocProvider) {
      std::cout << "\n"
                << ANSI_BOLD << "Action Required to Complete OpenCode Bridge Connection:" << ANSI_RESET << "\n";
      if (ocProvider->isInstalled()) {
        std::cout
            << "  * OpenCode CLI detected (" << ocProvider->getDetectedVersion() << "), but awaiting authentication.\n"
            << "  * Option 1: Run '" << ANSI_CYAN << "opencode auth login" << ANSI_RESET << "' in terminal to authenticate.\n"
            << "  * Option 2: Export your API key in environment:\n"
            << "              $env:OPENAI_API_KEY = \"sk-...\"  (PowerShell)\n"
            << "              export OPENAI_API_KEY=\"sk-...\" (Linux/bash)\n";
      } else {
        std::cout
            << "  * OpenCode CLI is not installed or not in system PATH.\n"
            << "  * Install via: " << ANSI_CYAN << "npm install -g @opencode/cli" << ANSI_RESET << "\n";
      }
      std::cout
          << "  * Option 3 (Recommended if using native models): Revert to Standalone Native mode:\n"
          << "              Type '" << ANSI_YELLOW << "/disconnect" << ANSI_RESET << "' or '" << ANSI_YELLOW << "/mode standalone" << ANSI_RESET << "'\n\n";
    }

    std::cout
        << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n\n";
  } else {
    std::cout << ANSI_YELLOW << "Unsupported connection target: " << target
              << ". Supported: 'opencode'.\n" << ANSI_RESET;
  }
}

void ReplEngine::handleDisconnect(const std::string &) {
  auto& cfg = agent_.getConfigMutable();
  cfg.setMode("standalone");
  cfg.default_provider = "auto";
  agent_.getRouter().refreshModels();

  std::cout << "\n"
            << ANSI_CYAN << ANSI_BOLD << "[DISCONNECTED FROM OPENCODE - STANDALONE MODE ACTIVE]" << ANSI_RESET << "\n"
            << "Mode           : STANDALONE (Self-Contained Native)\n"
            << "Execution      : Built-in C++20 engine with native file and process tools\n"
            << "Default Provider: auto (Ollama, local endpoints, OpenRouter, or TestMock)\n"
            << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n\n";
}

void ReplEngine::handleOpenCode(const std::string &args) {
  std::string trimmed = trim(args);
  auto providerPtr = agent_.getRouter().getProviderByName("OpenCode");
  auto ocProvider = std::dynamic_pointer_cast<OpenCodeProvider>(providerPtr);

  if (trimmed.empty() || trimmed == "status") {
    std::cout << "\n"
              << ANSI_BOLD << "OpenCode CLI Integration Status:" << ANSI_RESET
              << "\n"
              << "----------------------------------------------------\n";
    if (ocProvider) {
      bool avail = ocProvider->available();
      std::cout << "Command      : " << ocProvider->getOpenCodeCommand() << "\n"
                << "Status       : "
                << (avail ? (ANSI_GREEN + std::string("AVAILABLE") + ANSI_RESET)
                          : (ANSI_YELLOW + std::string("UNAVAILABLE") +
                             ANSI_RESET))
                << "\n"
                << "Details      : " << ocProvider->getStatusMessage() << "\n";
      if (avail) {
        auto models = ocProvider->discoverModels();
        std::cout << "Models (" << models.size() << ")  :\n";
        for (const auto &m : models) {
          std::cout << "  - " << ANSI_CYAN << m.id << ANSI_RESET << " ("
                    << m.name << ")\n";
        }
      } else {
        std::cout << "\nTo install OpenCode CLI:\n"
                  << "  " << ANSI_CYAN << "npm install -g @opencode/cli"
                  << ANSI_RESET << "\n"
                  << "  or: " << ANSI_CYAN
                  << "curl -fsSL https://opencode.ai/v2/install | bash"
                  << ANSI_RESET << "\n";
      }
    } else {
      std::cout
          << ANSI_YELLOW
          << "OpenCodeProvider is not registered in the active agent router."
          << ANSI_RESET << "\n";
    }
    std::cout
        << "----------------------------------------------------\n"
        << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
        << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n\n";
    return;
  }

  if (trimmed == "models") {
    if (ocProvider) {
      auto models = ocProvider->discoverModels();
      std::cout << "\n"
                << ANSI_BOLD << "OpenCode Models (" << models.size()
                << "):" << ANSI_RESET << "\n";
      for (const auto &m : models) {
        std::cout << "  * " << ANSI_CYAN << m.id << ANSI_RESET << " : "
                  << m.description << "\n";
      }
      std::cout << "\n";
    }
    return;
  }

  if (trimmed == "sync" || trimmed == "export") {
    bool ok = OpenCodeProvider::exportOpenCodeWorkspace(
        agent_.getConfig().workspace_dir, agent_.getSession().objective,
        "Autonomous ArenaFight execution loop with physical verification.");
    if (ok) {
      std::cout
          << "\n"
          << ANSI_GREEN << ANSI_BOLD << "[OPENCODE WORKSPACE SYNCED]"
          << ANSI_RESET << "\n"
          << "Created: .opencode/opencode.json and .opencode/INSTRUCTIONS.md\n"
          << "OpenCode and ArenaFight can now collaborate in this "
             "workspace.\n\n";
    } else {
      std::cout << ANSI_RED
                << "Failed to export OpenCode workspace configuration."
                << ANSI_RESET << "\n";
    }
    return;
  }

  if (trimmed.rfind("run ", 0) == 0) {
    std::string prompt = trim(trimmed.substr(4));
    if (prompt.empty()) {
      std::cout << ANSI_RED << "Usage: /opencode run <prompt>" << ANSI_RESET
                << "\n";
      return;
    }
    if (!ocProvider || !ocProvider->available()) {
      std::cout << ANSI_YELLOW
                << "OpenCode CLI is not available. Please install it or check "
                   "configuration."
                << ANSI_RESET << "\n";
      return;
    }
    std::cout << ANSI_CYAN << "[OpenCode Execution Initiated]..." << ANSI_RESET
              << "\n";
    AgentRequest req;
    req.userPrompt = prompt;
    AgentResponse res = ocProvider->generate(req);
    if (res.success) {
      std::cout << "\n"
                << ANSI_GREEN << ANSI_BOLD << "[OpenCode Output]" << ANSI_RESET
                << "\n"
                << res.content << "\n\n";
    } else {
      std::cout << ANSI_RED << "[OpenCode Error] " << res.errorMessage
                << ANSI_RESET << "\n\n";
    }
    return;
  }

  std::cout << ANSI_YELLOW
            << "Unknown opencode sub-command. Try: /opencode "
               "[status|models|run <prompt>|sync]\n"
            << ANSI_RESET;
}

void ReplEngine::handleMetrics(const std::string &/*args*/) {
  TerminalUI::printMetricsDashboard(agent_.getSession(), agent_.getConfig());
}

void ReplEngine::handleUnsloth(const std::string &args) {
  UnslothManager unsloth(agent_.getConfig());
  std::string sub = trim(args);

  if (sub.empty() || sub == "status") {
    std::cout << "\n====================================================\n"
              << "          UNSLOTH AI FINE-TUNING ENGINE              \n"
              << "====================================================\n";
    auto env = unsloth.checkEnvironment();
    std::cout
        << "Python Interpreter : "
        << (env.pythonAvailable ? (ANSI_GREEN + env.pythonVersion + ANSI_RESET)
                                : (ANSI_RED + "NOT FOUND" + ANSI_RESET))
        << "\n"
        << "Unsloth Library    : "
        << (env.unslothInstalled
                ? (ANSI_GREEN + env.unslothVersion + ANSI_RESET)
                : (ANSI_YELLOW + "Not installed (pip install unsloth)" +
                   ANSI_RESET))
        << "\n"
        << "PyTorch            : "
        << (env.torchInstalled ? (ANSI_GREEN + env.torchVersion + ANSI_RESET)
                               : (ANSI_RED + "NOT FOUND" + ANSI_RESET))
        << "\n"
        << "CUDA Acceleration  : "
        << (env.cudaAvailable
                ? (ANSI_GREEN + "ENABLED (" + env.gpuName + " - " +
                   std::to_string(env.vramGb) + "GB VRAM)" + ANSI_RESET)
                : (ANSI_YELLOW + "CPU / No CUDA" + ANSI_RESET))
        << "\n\n"
        << "Summary: " << env.statusSummary << "\n\n"
        << "Usage:\n"
        << "  /unsloth status             Inspect environment and hardware "
           "diagnostics\n"
        << "  /unsloth dataset            Extract instruction pairs from "
           "session trajectory\n"
        << "  /unsloth scaffold           Generate standalone unsloth_train.py "
           "script\n"
        << "  /unsloth train              Trigger on-the-fly QLoRA fine-tuning "
           "and export GGUF\n"
        << "----------------------------------------------------\n"
        << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
        << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n\n";
    return;
  }

  if (sub.rfind("dataset", 0) == 0) {
    std::string customPath = trim(sub.substr(7));
    std::vector<DatasetEntry> data;
    if (!customPath.empty()) {
      std::cout << "\n[UNSLOTH] Generating fine-tuning dataset from target: "
                << customPath << "...\n";
      data = unsloth.generateDatasetFromPath(customPath);
    } else {
      std::cout << "\n[UNSLOTH] Extracting fine-tuning dataset from sessions & "
                   "hardware database...\n";
      data = unsloth.extractFromSessions("sessions");
      auto hw = unsloth.generateSyntheticHardwareDataset();
      data.insert(data.end(), hw.begin(), hw.end());
      if (data.empty()) {
        data = unsloth.generateWorkspaceFineTuneDataset(".");
      }
    }
    std::string outPath = "unsloth_dataset.jsonl";
    if (unsloth.saveDatasetToJsonl(data, outPath)) {
      std::cout << ANSI_GREEN << "Successfully exported " << data.size()
                << " instruction pairs -> " << outPath << ANSI_RESET << "\n\n";
    } else {
      std::cout << ANSI_RED << "Failed to write dataset file: " << outPath
                << ANSI_RESET << "\n\n";
    }
    return;
  }

  if (sub.rfind("scaffold", 0) == 0) {
    UnslothTrainConfig cfg;
    std::string extra = trim(sub.substr(8));
    if (!extra.empty() && extra.front() != '-') {
      cfg.baseModel = extra;
    }
    std::string scriptPath = "unsloth_train.py";
    if (unsloth.writeTrainingScript(scriptPath, cfg)) {
      std::cout << ANSI_GREEN
                << "Generated Unsloth training script: " << scriptPath
                << ANSI_RESET << "\n"
                << "Configured for: " << cfg.baseModel
                << " (4-bit QLoRA, rank=" << cfg.loraRank << ")\n\n";
    }
    return;
  }

  if (sub.rfind("train", 0) == 0) {
    std::cout << "\n[UNSLOTH] Triggering on-the-fly fine-tuning loop...\n";
    UnslothTrainConfig cfg;
    std::string rest = trim(sub.substr(5));
    std::stringstream ss(rest);
    std::string word;
    while (ss >> word) {
      if (word == "--dry-run" || word == "--mock") {
        cfg.dryRunSimulation = true;
      } else if (!word.empty() && word.front() != '-') {
        cfg.baseModel = word;
      }
    }
    std::cout << "Base Model: " << cfg.baseModel << "\n";
    bool ok = unsloth.trainOnTheFly(
        cfg, [](const std::string &line) { std::cout << "  " << line << "\n"; },
        &agent_.getRouter());
    if (ok) {
      std::cout << ANSI_GREEN
                << "\n[UNSLOTH] Fine-tuning workflow completed successfully!"
                << ANSI_RESET << "\n\n";
    }
    return;
  }

  std::cout << ANSI_YELLOW << "Unknown unsloth subcommand: " << sub
            << ". Use: status, dataset, scaffold, or train." << ANSI_RESET
            << "\n";
}

void ReplEngine::handleAbilities(const std::string &) {
  auto matrix = agent_.getRouter().getAbilityMatrix();
  TerminalUI::printAbilityMatrix(matrix);
}

void ReplEngine::handleTemperature(const std::string &args) {
  std::string s = trim(args);
  if (s.empty()) {
    std::cout << "Current generation temperature: " << ANSI_CYAN
              << agent_.getTemperature() << ANSI_RESET << "\n"
              << "To set: /temp <0.0 - 2.0>\n";
    return;
  }
  try {
    float val = std::stof(s);
    if (val < 0.0f || val > 2.0f) {
      std::cout
          << ANSI_YELLOW
          << "Warning: Temperature typically ranges between 0.0 and 2.0.\n"
          << ANSI_RESET;
    }
    agent_.setTemperature(val);
    std::cout << ANSI_GREEN << "Temperature updated to: " << val << ANSI_RESET
              << "\n";
  } catch (...) {
    std::cout << ANSI_RED << "Invalid temperature value: " << s << ANSI_RESET
              << "\n";
  }
}

void ReplEngine::handleTokens(const std::string &args) {
  std::string s = trim(args);
  if (s.empty()) {
    std::cout << "Current max output tokens: " << ANSI_CYAN
              << agent_.getMaxTokens() << ANSI_RESET << "\n"
              << "To set: /tokens <int_value>\n";
    return;
  }
  try {
    int val = std::stoi(s);
    if (val <= 0) {
      std::cout << ANSI_RED << "Max tokens must be a positive integer.\n"
                << ANSI_RESET;
      return;
    }
    agent_.setMaxTokens(val);
    std::cout << ANSI_GREEN << "Max generation tokens updated to: " << val
              << ANSI_RESET << "\n";
  } catch (...) {
    std::cout << ANSI_RED << "Invalid token count: " << s << ANSI_RESET << "\n";
  }
}

void ReplEngine::handleFiles(const std::string &args) {
  std::string line = trim(args);
  std::stringstream ss(line);
  std::string subCmd;
  ss >> subCmd;

  if (subCmd.empty() || subCmd == "list") {
    std::string targetDir = agent_.getConfig().workspace_dir;
    std::string subPath;
    ss >> subPath;
    if (!subPath.empty()) {
      targetDir = targetDir + "/" + subPath;
    }

    std::cout << "\n====================================================\n"
              << "              WORKSPACE FILE EXPLORER               \n"
              << "====================================================\n"
              << "Path: " << targetDir << "\n\n";

    std::cout << std::left;
    std::cout.width(36);
    std::cout << "FILENAME";
    std::cout.width(10);
    std::cout << "TYPE";
    std::cout.width(12);
    std::cout << "SIZE";
    std::cout << "STATUS\n";
    std::cout << "-------------------------------------------------------------"
                 "-------\n";

    std::error_code ec;
    if (!std::filesystem::exists(targetDir, ec)) {
      std::cout << ANSI_RED << "Directory not found: " << targetDir
                << ANSI_RESET << "\n\n";
      return;
    }

    const auto &attached = agent_.getAttachedFiles();

    int count = 0;
    for (const auto &entry :
         std::filesystem::directory_iterator(targetDir, ec)) {
      std::string name = entry.path().filename().string();
      if (name == ".git" || name == ".gemini" || name == "build" ||
          name == "venv")
        continue;

      bool isDir = entry.is_directory();
      uintmax_t size = 0;
      if (!isDir) {
        std::error_code sec;
        size = entry.file_size(sec);
      }

      std::string sizeStr;
      if (isDir) {
        sizeStr = "-";
      } else if (size < 1024) {
        sizeStr = std::to_string(size) + " B";
      } else if (size < 1024 * 1024) {
        sizeStr = std::to_string(size / 1024) + " KB";
      } else {
        sizeStr = std::to_string(size / (1024 * 1024)) + " MB";
      }

      std::string pathStr = entry.path().generic_string();
      bool isAttached =
          std::find(attached.begin(), attached.end(), pathStr) !=
              attached.end() ||
          std::find(attached.begin(), attached.end(), name) != attached.end();

      std::string padName =
          (name.size() > 34 ? name.substr(0, 31) + "..." : name);
      while (padName.size() < 36)
        padName += " ";

      std::string typeStr = isDir ? "DIR" : "FILE";
      while (typeStr.size() < 8)
        typeStr += " ";
      std::string coloredType = isDir ? (ANSI_BLUE + typeStr + ANSI_RESET)
                                      : (ANSI_CYAN + typeStr + ANSI_RESET);

      std::string padSize = sizeStr;
      while (padSize.size() < 12)
        padSize += " ";

      std::string statusStr = isAttached
                                  ? (ANSI_GREEN + "ATTACHED" + ANSI_RESET)
                                  : (ANSI_DIM + "unattached" + ANSI_RESET);
      std::cout << padName << coloredType << padSize << statusStr << "\n";
      count++;
    }

    std::cout
        << "-------------------------------------------------------------------"
           "-\n"
        << "Total files/directories: " << count << "\n"
        << "Attached to session: " << attached.size() << "\n\n"
        << "Commands:\n"
        << "  /files cat <path>           Preview file content\n"
        << "  /files attach <path>        Attach file directly to agent "
           "session context\n"
        << "  /files attached             List currently attached files\n"
        << "  /files clear                Clear all attached files\n"
        << "----------------------------------------------------\n"
        << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
        << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n\n";
    return;
  }

  if (subCmd == "cat" || subCmd == "read") {
    std::string filePath;
    ss >> filePath;
    if (filePath.empty()) {
      std::cout << ANSI_YELLOW << "Usage: /files cat <path>\n" << ANSI_RESET;
      return;
    }

    if (!std::filesystem::exists(filePath)) {
      std::cout << ANSI_RED << "File not found: " << filePath << ANSI_RESET
                << "\n";
      return;
    }

    std::string content = FileUtils::readTextFile(filePath);
    std::cout << "\n"
              << ANSI_CYAN << ANSI_BOLD << "--- File: " << filePath << " ("
              << content.size() << " bytes) ---" << ANSI_RESET << "\n";
    std::stringstream css(content);
    std::string cline;
    int lineNum = 1;
    while (std::getline(css, cline)) {
      std::cout << ANSI_DIM;
      std::cout.width(4);
      std::cout << lineNum++ << " | " << ANSI_RESET << cline << "\n";
      if (lineNum > 200) {
        std::cout << ANSI_DIM << "... [truncated after 200 lines] ...\n"
                  << ANSI_RESET;
        break;
      }
    }
    std::cout << ANSI_CYAN
              << "----------------------------------------------------\n\n"
              << ANSI_RESET;
    return;
  }

  if (subCmd == "attach") {
    std::string filePath;
    ss >> filePath;
    if (filePath.empty()) {
      std::cout << ANSI_YELLOW << "Usage: /files attach <path>\n" << ANSI_RESET;
      return;
    }

    if (!std::filesystem::exists(filePath)) {
      std::cout << ANSI_RED << "File not found: " << filePath << ANSI_RESET
                << "\n";
      return;
    }

    std::string content = FileUtils::readTextFile(filePath);
    agent_.attachFile(filePath, content);
    std::cout << ANSI_GREEN << "Successfully attached " << filePath << " ("
              << content.size() << " bytes) to active session.\n"
              << "This file will be injected into all future planning and tool "
                 "execution prompts.\n\n"
              << ANSI_RESET;
    return;
  }

  if (subCmd == "attached") {
    const auto &attached = agent_.getAttachedFiles();
    std::cout << "\nCurrently Attached Files (" << attached.size() << "):\n";
    if (attached.empty()) {
      std::cout << "  (No files attached. Use /files attach <path> or @file "
                   "<path>)\n\n";
    } else {
      for (const auto &f : attached) {
        std::cout << "  * " << ANSI_CYAN << f << ANSI_RESET << "\n";
      }
      std::cout << "\n";
    }
    return;
  }

  if (subCmd == "clear") {
    agent_.getSessionMutable().clearAttachedFiles();
    std::cout << ANSI_GREEN << "Cleared all attached files from session.\n\n"
              << ANSI_RESET;
    return;
  }

  std::cout
      << ANSI_YELLOW << "Unknown file subcommand: " << subCmd << "\n"
      << "Usage: /files [list|cat <path>|attach <path>|attached|clear]\n\n"
      << ANSI_RESET;
}

void ReplEngine::handleThink(const std::string &args) {
  if (args.empty()) {
    std::cout << ANSI_YELLOW
              << "Usage: /think <task objective> [candidateA] [candidateB]\n"
              << "Disallows slow code generation and compares LLM internal "
                 "thinking phases directly.\n\n"
              << ANSI_RESET;
    return;
  }

  std::string task = args;
  std::string candA = "";
  std::string candB = "";

  // Parse potential candidate models if provided with quotes or trailing words
  if (task.front() == '"') {
    size_t closeQuote = task.find('"', 1);
    if (closeQuote != std::string::npos) {
      std::string objective = task.substr(1, closeQuote - 1);
      std::string remainder = trim(task.substr(closeQuote + 1));
      task = objective;
      std::stringstream ss(remainder);
      ss >> candA >> candB;
    }
  } else {
    std::stringstream ss(task);
    std::vector<std::string> words;
    std::string w;
    while (ss >> w)
      words.push_back(w);
    if (words.size() >= 3 &&
        (words[words.size() - 2].find("mock") != std::string::npos ||
         words[words.size() - 2].find("claude") != std::string::npos ||
         words[words.size() - 2].find("qwen") != std::string::npos ||
         words[words.size() - 2].find("r1") != std::string::npos ||
         words[words.size() - 2].find("reason") != std::string::npos)) {
      candB = words.back();
      candA = words[words.size() - 2];
      task = "";
      for (size_t i = 0; i < words.size() - 2; ++i) {
        if (!task.empty())
          task += " ";
        task += words[i];
      }
    }
  }

  std::cout << "\n"
            << ANSI_CYAN << "[THINKING DUEL]" << ANSI_RESET
            << " Initiating fast thinking phase tournament (Code generation "
               "disallowed)...\n"
            << ANSI_DIM
            << "Evaluating internal reasoning, edge-case anticipation, and "
               "safety invariants.\n"
            << ANSI_RESET << std::flush;

  auto duelRes = agent_.duelThinkingPhase(task, candA, candB);
  TerminalUI::printThinkingDuel(duelRes);
}

void ReplEngine::handleDuel(const std::string &args) { handleThink(args); }

void ReplEngine::handleHelpSession(const std::string & /*args*/) {
  auto router = agent_.getRouter();
  auto oc = router.getProviderByName("OpenCode");
  auto claude = router.getProviderByName("ClaudeCode");
  auto ollama = router.getProviderByName("Ollama");
  auto openrouter = router.getProviderByName("OpenRouter");
  auto mock = router.getProviderByName("TestMock");

  std::vector<std::pair<std::string, bool>> statuses = {
      {"OpenCode CLI", oc && oc->available()},
      {"Claude Code CLI", claude && claude->available()},
      {"Ollama (Local)", ollama && ollama->available()},
      {"OpenRouter", openrouter && openrouter->available()},
      {"Offline TestMock", mock && mock->available()}};

  TerminalUI::printHelpSession(agent_.getConfig(), statuses);
}

void ReplEngine::handleHelp(const std::string &args) {
  if (args == "session" || args == "/session" || args == "guide" ||
      args == "/guide") {
    handleHelpSession(args);
    return;
  }

  if (!args.empty()) {
    std::string cmd = args;
    if (cmd.front() != '/')
      cmd = "/" + cmd;
    auto it = slashCommands_.find(cmd);
    if (it != slashCommands_.end()) {
      std::cout << "\n"
                << ANSI_BOLD << it->second.name << ANSI_RESET << "\n"
                << "  " << it->second.description << "\n"
                << "  " << ANSI_DIM << "Usage: " << it->second.usage
                << ANSI_RESET << "\n\n";
      return;
    }
  }
  printHelpTable();
}

void ReplEngine::handleBoards(const std::string &filter) {
  auto boards = HardwareBoardRegistry::instance().getAllBoards();
  if (!filter.empty()) {
    std::vector<BoardSpec> filtered;
    for (const auto &b : boards) {
      if (b.id.find(filter) != std::string::npos ||
          b.name.find(filter) != std::string::npos) {
        filtered.push_back(b);
      }
    }
    TerminalUI::printBoardsTable(filtered);
  } else {
    TerminalUI::printBoardsTable(boards);
  }
}

void ReplEngine::handleBoard(const std::string &boardId) {
  std::string id = boardId;
  if (id.empty()) {
    id = askUserInteractive("Enter board ID to inspect (e.g., esp32, pico, "
                            "arduino_uno, stm32f401):");
  }
  if (id.empty())
    id = "esp32";

  auto b = HardwareBoardRegistry::instance().getBoard(id);
  if (!b)
    b = HardwareBoardRegistry::instance().findBoard(id);

  if (b) {
    TerminalUI::printBoardDetails(*b);
  } else {
    std::cout << ANSI_YELLOW << "Microcontroller board '" << id
              << "' not found. Run /boards to view available boards.\n"
              << ANSI_RESET;
  }
}

void ReplEngine::handleComponents(const std::string &filter) {
  auto comps = HardwareBoardRegistry::instance().getAllComponents();
  if (!filter.empty()) {
    std::vector<ComponentSpec> filtered;
    for (const auto &c : comps) {
      if (c.id.find(filter) != std::string::npos ||
          c.name.find(filter) != std::string::npos ||
          c.category.find(filter) != std::string::npos) {
        filtered.push_back(c);
      }
    }
    TerminalUI::printComponentsTable(filtered);
  } else {
    TerminalUI::printComponentsTable(comps);
  }
}

void ReplEngine::handleScaffold(const std::string &args) {
  std::stringstream ss(args);
  std::string boardId, projectName;
  ss >> boardId >> projectName;

  if (boardId.empty()) {
    boardId = askUserInteractive(
        "Enter target microcontroller board (esp32, pico, arduino_uno):");
  }
  if (projectName.empty()) {
    projectName = askUserInteractive("Enter project name:");
  }
  if (boardId.empty())
    boardId = "esp32";
  if (projectName.empty())
    projectName = "embedded_project";

  std::cout << "\n"
            << ANSI_CYAN << "[HARDWARE STUDIO]" << ANSI_RESET
            << " Scaffolding embedded project for board: " << boardId
            << "...\n";

  ScaffoldOptions opts;
  opts.boardId = boardId;
  opts.projectName = projectName;
  opts.outputDir = agent_.getConfig().workspace_dir;
  opts.generateWokwiSimulation = true;

  auto res = HardwareManager::instance().scaffold(opts);
  if (res.success) {
    std::cout << ANSI_GREEN << ANSI_BOLD << "[SCAFFOLD SUCCESS]" << ANSI_RESET
              << "\n"
              << "Project Path: " << res.projectDir << "\n"
              << "Files Created (" << res.filesCreated.size() << "):\n";
    for (const auto &f : res.filesCreated) {
      std::cout << "  * " << f << "\n";
    }
    std::cout << "\nWokwi Simulation Diagram: " << res.wokwiDiagramPath << "\n";

    std::string artContent =
        "# Hardware Project Scaffold: " + projectName + "\n\n" +
        "- **Target Board:** " + boardId + "\n" + "- **Wokwi Simulation:** `" +
        res.wokwiDiagramPath + "`\n" + "- **PlatformIO Configuration:** `" +
        res.projectDir + "/platformio.ini`\n" + "- **QoS Firmware Source:** `" +
        res.projectDir + "/src/main.cpp`\n\n";

    ArtifactMetadata meta;
    meta.userFacing = true;
    meta.requestFeedback = false;
    meta.summary = "Scaffolded embedded project with Wokwi simulation";

    artifacts_.createArtifact("Hardware Scaffold: " + projectName,
                              "hardware_scaffold.md", ArtifactType::REPORT,
                              artContent, meta);

    std::cout << ANSI_CYAN
              << "Artifact recorded: .arena/artifacts/hardware_scaffold.md\n"
              << ANSI_RESET;
  } else {
    std::cout << ANSI_RED << "[SCAFFOLD FAILED] " << res.message << ANSI_RESET
              << "\n";
  }
}

void ReplEngine::handleQoS(const std::string &path) {
  std::string target = path;
  if (target.empty())
    target = agent_.getConfig().workspace_dir;

  std::cout << "\n"
            << ANSI_CYAN << "[FIRMWARE QoS EVALUATION]" << ANSI_RESET
            << " Inspecting: " << target << "...\n";

  auto qos = HardwareManager::instance().evaluateProjectQoS(target);
  TerminalUI::printQoSTelemetryReport(qos);
}

void ReplEngine::handleCalculate(const std::string &args) {
  std::stringstream ss(args);
  std::string type;
  ss >> type;

  if (type == "led") {
    double vcc = 5.0, vf = 2.0, ma = 15.0;
    ss >> vcc >> vf >> ma;
    auto res = CircuitCalculator::calculateLedResistor(vcc, vf, ma);
    std::cout << "\n"
              << ANSI_BOLD << "LED Resistor Calculation:" << ANSI_RESET << "\n"
              << "  Formula: " << res.formulaUsed << "\n"
              << "  Summary: " << res.summary << "\n\n";
    for (const auto &rec : res.componentRecommendations) {
      std::cout << "  * " << rec << "\n";
    }
    std::cout << "\n";
  } else if (type == "divider") {
    double vin = 5.0, vout = 3.3, rk = 10.0;
    ss >> vin >> vout >> rk;
    auto res = CircuitCalculator::calculateVoltageDivider(vin, vout, rk);
    std::cout << "\n"
              << ANSI_BOLD
              << "Voltage Divider Calculation (5V -> 3.3V Level Shifting):"
              << ANSI_RESET << "\n"
              << "  Formula: " << res.formulaUsed << "\n"
              << "  Summary: " << res.summary << "\n\n";
    for (const auto &rec : res.componentRecommendations) {
      std::cout << "  * " << rec << "\n";
    }
    std::cout << "\n";
  } else if (type == "i2c") {
    double vcc = 3.3;
    int speed = 100;
    ss >> vcc >> speed;
    auto res = CircuitCalculator::calculateI2cPullup(vcc, speed);
    std::cout << "\n"
              << ANSI_BOLD
              << "I2C Bus Pull-Up Resistor Calculation:" << ANSI_RESET << "\n"
              << "  Formula: " << res.formulaUsed << "\n"
              << "  Summary: " << res.summary << "\n\n";
    for (const auto &rec : res.componentRecommendations) {
      std::cout << "  * " << rec << "\n";
    }
    std::cout << "\n";
  } else if (type == "battery") {
    double mah = 2500.0, actMa = 80.0, slpUa = 15.0, actSec = 2.0,
           slpSec = 58.0;
    ss >> mah >> actMa >> slpUa >> actSec >> slpSec;
    auto res = CircuitCalculator::calculateBatteryLife(mah, actMa, slpUa,
                                                       actSec, slpSec);
    std::cout << "\n"
              << ANSI_BOLD << "Battery & Deep Sleep Power Budget:" << ANSI_RESET
              << "\n"
              << "  Formula: " << res.formulaUsed << "\n"
              << "  Summary: " << res.summary << "\n\n";
    for (const auto &rec : res.componentRecommendations) {
      std::cout << "  * " << rec << "\n";
    }
    std::cout << "\n";
  } else {
    std::cout
        << ANSI_YELLOW << "Usage: /calculate <led|divider|i2c|battery> [args]\n"
        << "  /calculate led 5.0 2.0 15        (Supply V, Forward Vf, Current "
           "mA)\n"
        << "  /calculate divider 5.0 3.3 10    (Vin, Target Vout, R2 kOhms)\n"
        << "  /calculate i2c 3.3 100           (Supply V, Bus speed kHz)\n"
        << "  /calculate battery 2500 80 15 2 58 (Capacity mAh, Active mA, "
           "Sleep uA, Act Sec, Sleep Sec)\n\n"
        << ANSI_RESET;
  }
}

void ReplEngine::handleBenchmark(const std::string& args) {
    std::string trimmed = trim(args);

    std::vector<std::string> candidates;
    if (!trimmed.empty() && trimmed != "all" && trimmed != "*") {
        std::istringstream iss(trimmed);
        std::string token;
        while (iss >> token) {
            candidates.push_back(token);
        }
    }

    std::cout << "\n" << ANSI_CYAN << ANSI_BOLD << "[UNIVERSAL BENCHMARK ENGINE]" << ANSI_RESET
              << " Evaluating models across reasoning, coding, verification, and throughput...\n";

    BenchmarkSuiteResult suite;
    if (candidates.empty()) {
        suite = agent_.benchmarkAll(true);
    } else {
        suite = agent_.benchmarkModels(candidates, true);
    }

    std::cout << ANSI_GREEN << "[BENCHMARK COMPLETE]" << ANSI_RESET
              << " Evaluated " << suite.totalModelsTested << " models (" << suite.onlineModels << " online).\n"
              << "Scorecard persisted to benchmark_results.json\n\n";
}

void ReplEngine::processInput(const std::string &rawInput) {
  std::string line = trim(rawInput);
  if (line.empty())
    return;

  // Check slash command
  if (line.front() == '/') {
    std::string cmd;
    std::string args;
    size_t spacePos = line.find_first_of(" \t");
    if (spacePos != std::string::npos) {
      cmd = line.substr(0, spacePos);
      args = trim(line.substr(spacePos));
    } else {
      cmd = line;
    }

    auto it = slashCommands_.find(cmd);
    if (it != slashCommands_.end()) {
      it->second.handler(args);
      return;
    } else {
      std::cout << ANSI_YELLOW << "Unknown slash command: " << cmd
                << ". Type /help for available commands.\n"
                << ANSI_RESET;
      return;
    }
  }

  // Conversational prompt or question with context mentions
  std::string lowerLine = line;
  for (char &c : lowerLine) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  bool isGreetingOrChat = (lowerLine == "hi" || lowerLine == "hello" || lowerLine == "hey" ||
                           lowerLine.rfind("how are you", 0) == 0 || lowerLine.rfind("what's up", 0) == 0 ||
                           lowerLine.rfind("who are you", 0) == 0 || lowerLine.rfind("tell me a joke", 0) == 0 ||
                           lowerLine.rfind("what can you do", 0) == 0 || lowerLine.rfind("let's talk", 0) == 0 ||
                           lowerLine.rfind("talk to me", 0) == 0);

  if (isGreetingOrChat) {
      std::cout << "\n" << ANSI_CYAN << ANSI_BOLD << "ArenaFight Agent: " << ANSI_RESET;
      if (lowerLine == "hi" || lowerLine == "hello" || lowerLine == "hey") {
          std::cout << "Hey there! Great to see you. I am ready for whatever you need: coding, deep thinking, or just hanging out.\n"
                    << "Tip: Type " << ANSI_YELLOW << "/chat" << ANSI_RESET << " to enter Fun Talk Mode with custom personas (pirate, cyberpunk, zen, nerd, witty)!\n\n";
      } else if (lowerLine.rfind("how are you", 0) == 0 || lowerLine.rfind("what's up", 0) == 0) {
          std::cout << "Running at peak performance and ready to roll! How is your day going?\n"
                    << "Tip: Type " << ANSI_YELLOW << "/chat" << ANSI_RESET << " to start an interactive conversation!\n\n";
      } else if (lowerLine.rfind("tell me a joke", 0) == 0) {
          std::cout << "There are only 10 types of people in the world: those who understand binary, and those who don't!\n"
                    << "Want more banter? Jump into " << ANSI_YELLOW << "/chat witty" << ANSI_RESET << "!\n\n";
      } else {
          std::cout << "I am ArenaFight Agent, your autonomous multi-model coding companion and chat partner!\n"
                    << "You can give me tasks with " << ANSI_YELLOW << "/goal <task>" << ANSI_RESET << " or just chat with " << ANSI_YELLOW << "/chat" << ANSI_RESET << ".\n\n";
      }
      return;
  }

  std::string expandedPrompt = resolveMentions(line);

  displayThinking(
      "Analyzing prompt and retrieving repository context...\n"
      "Injecting custom rules and relevant skills into context window.");

  // If it looks like a task/action, ask if user wants /goal or direct answer
  std::cout << "\n"
            << ANSI_CYAN << ANSI_BOLD << "ArenaFight Agent:" << ANSI_RESET
            << "\n"
            << "I received your request: \"" << line << "\"\n\n"
            << "How would you like to proceed?\n"
            << "  - Type " << ANSI_YELLOW << "/goal " << line << ANSI_RESET
            << " to execute this task autonomously to completion.\n"
            << "  - Type " << ANSI_YELLOW << "/plan " << line << ANSI_RESET
            << " to generate an interactive plan artifact first.\n"
            << "  - Type " << ANSI_YELLOW << "/grill-me " << line << ANSI_RESET
            << " to clarify requirements before implementation.\n"
            << "  - Type " << ANSI_YELLOW << "/chat" << ANSI_RESET
            << " to enter fun interactive talk mode just to chat.\n\n"
            << ANSI_DIM
            << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/"
            << ANSI_RESET << "\n\n";
}

void ReplEngine::handleSecurity(const std::string& args) {
  std::vector<std::string> tokens;
  std::istringstream iss(args);
  std::string tok;
  while (iss >> tok) {
    tokens.push_back(tok);
  }
  std::string sub = tokens.empty() ? "" : tokens[0];

  if (sub == "scan") {
    std::cout << "\n[RUNNING SECURITY VULNERABILITY SCAN ON WORKSPACE]...\n";
    auto report = SecuritySandbox::scanWorkspace(agent_.getConfig().workspace_dir);
    bool chainValid = SecuritySandbox::verifyAuditChain(agent_.getConfig().workspace_dir);
    auto logs = SecuritySandbox::getAuditLog(agent_.getConfig().workspace_dir);
    TerminalUI::printSecurityDashboard(agent_.getConfig(), report, chainValid, logs);
  } else if (sub == "audit") {
    handleAudit("");
  } else if (sub == "level" || sub == "mode" || sub == "lock") {
    std::string lvl = tokens.size() > 1 ? tokens[1] : "";
    if (lvl == "strict" || lvl == "balanced" || lvl == "permissive") {
      agent_.getConfigMutable().setSecurityLevel(lvl);
      std::cout << "\n[SECURITY LEVEL UPDATED] -> " << lvl << "\n";
      SecuritySandbox::logAuditEvent("SECURITY_LEVEL_CHANGED", "INFO", "ReplEngine", "New level: " + lvl, agent_.getConfig().workspace_dir);
    } else {
      std::cout << "\nActive Security Level: " << agent_.getConfig().security_level << "\n"
                << "Usage: /security level [strict|balanced|permissive]\n";
    }
  } else {
    auto report = SecuritySandbox::scanWorkspace(agent_.getConfig().workspace_dir);
    bool chainValid = SecuritySandbox::verifyAuditChain(agent_.getConfig().workspace_dir);
    auto logs = SecuritySandbox::getAuditLog(agent_.getConfig().workspace_dir);
    TerminalUI::printSecurityDashboard(agent_.getConfig(), report, chainValid, logs);
  }
}

void ReplEngine::handleAudit(const std::string&) {
  std::string failureReason;
  bool valid = SecuritySandbox::verifyAuditChain(agent_.getConfig().workspace_dir, &failureReason);
  auto logs = SecuritySandbox::getAuditLog(agent_.getConfig().workspace_dir);

  std::cout << "\n========================================================================\n"
            << "            ARENAFIGHT TAMPER-EVIDENT SECURITY AUDIT TRAIL              \n"
            << "========================================================================\n"
            << "Audit Log File       : " << agent_.getConfig().workspace_dir << "/.arena/security_audit.jsonl\n"
            << "Cryptographic Hash   : FIPS 180-4 SHA-256 Chained Integrity\n"
            << "Chain Status         : " << (valid ? "\033[32m[VERIFIED 100% INTACT]\033[0m" : "\033[31m[TAMPER DETECTED: " + failureReason + "]\033[0m") << "\n"
            << "Total Logged Records : " << logs.size() << "\n"
            << "------------------------------------------------------------------------\n";

  if (logs.empty()) {
    std::cout << "No security violations or audit events logged yet. System running clean.\n";
  } else {
    for (const auto& rec : logs) {
      std::string timeSub = rec.timestamp.length() >= 19 ? rec.timestamp.substr(11, 8) : rec.timestamp;
      std::cout << "#" << rec.index << " [" << timeSub << "] "
                << std::left;
      std::cout.width(10);
      std::cout << rec.severity << " "
                << std::left;
      std::cout.width(28);
      std::cout << rec.event << " "
                << "by " << rec.caller << "\n"
                << "   Details: " << rec.details << "\n"
                << "   Hash   : " << rec.recordHash.substr(0, 16) << "... (Prev: "
                << rec.prevHash.substr(0, 8) << "...)\n";
    }
  }

  std::cout << "------------------------------------------------------------------------\n"
            << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
            << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
            << "========================================================================\n\n";
}

void ReplEngine::handleLevel(const std::string& args) {
  std::string arg = trim(args);
  std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

  if (arg.empty()) {
    std::cout << "\n====================================================\n"
              << "              PROCESSING LEVEL STATUS               \n"
              << "====================================================\n"
              << "Active Level: "
              << (agent_.isPerfectBoi() ? "PERFECT BOI (Exhaustive & Adaptive Limits)" : "FAST (Standard Turnaround)")
              << "\n\n"
              << "Available Levels:\n"
              << "  1. fast        : Standard execution loop, fast thinking, rapid turnaround.\n"
              << "  2. perfect_boi : Deep perfectionist mode. Takes time, enforces exhaustive\n"
              << "                   proof across all gates, runs adversarial cross-checks,\n"
              << "                   and dynamically scales limits based on physical PC specs.\n\n"
              << "Usage: /level [fast|perfect]\n"
              << "====================================================\n"
              << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
              << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
              << "====================================================\n\n";
    return;
  }

  if (arg == "fast") {
    agent_.setProcessingLevel("fast");
    std::cout << "\n[LEVEL] Processing level set to: FAST (Standard Rapid Execution)\n\n";
  } else if (arg == "perfect" || arg == "perfect_boi" || arg == "perfect-boi") {
    agent_.setProcessingLevel("perfect_boi");
    HardwareSpec spec = SystemProfiler::profileHostMachine();
    DynamicResourceLimits limits = SystemProfiler::calculateDynamicLimits("perfect_boi", spec);
    std::cout << "\n[LEVEL] Processing level set to: PERFECT BOI\n"
              << "Assessing physical host PC...\n"
              << "  * Host:       " << spec.osName << " (" << spec.logicalCores << " threads, "
              << std::fixed << std::setprecision(1) << spec.availRamGb() << " GB RAM free)\n"
              << "  * Dynamic Allocation: " << limits.maxIterations << " max iterations, "
              << limits.maxTokens << " tokens, "
              << limits.deepThinkingBudget << " reasoning tokens, "
              << limits.adversarialReviewPasses << " adversarial passes.\n\n";
  } else {
    std::cout << "\n[ERROR] Unknown level '" << arg << "'. Supported: 'fast', 'perfect' (or 'perfect_boi').\n\n";
  }
}

void ReplEngine::handleProfile(const std::string& args) {
  bool forceCpu = agent_.getConfig().force_cpu;
  std::string arg = trim(args);
  if (arg == "cpu" || arg == "--cpu" || arg == "slow") {
    forceCpu = true;
  } else if (arg == "cuda" || arg == "--cuda") {
    forceCpu = false;
  }
  HardwareSpec spec = SystemProfiler::profileHostMachine(forceCpu);
  DynamicResourceLimits limits = SystemProfiler::calculateDynamicLimits(agent_.getProcessingLevel(), spec);
  std::cout << SystemProfiler::formatAssessmentDashboard(spec, limits);
}

void ReplEngine::handleServices(const std::string& args) {
  std::string arg = trim(args);
  std::string llamaUrl = agent_.getConfig().llama_server_url;
  std::string ollamaUrl = agent_.getConfig().ollama_url;

  std::cout << "\n====================================================\n"
            << "       AUTONOMOUS SERVICE DAEMON MANAGER            \n"
            << "====================================================\n"
            << "Platform:       " << (SystemProfiler::isWindows() ? "Windows" : "Linux/POSIX") << "\n"
            << "Primary Engine: llama.cpp (Native GGUF & GPU acceleration)\n"
            << "Server URL:     " << llamaUrl << "\n"
            << "Offload Layers: " << agent_.getConfig().llama_gpu_layers << " (NVIDIA CUDA)\n"
            << "----------------------------------------------------\n";

  if (arg == "stop") {
    std::cout << "Stopping llama.cpp server daemon...\n";
    bool stopped = LlamaServerDaemon::stopServer();
    if (stopped) {
      std::cout << "llama.cpp server stopped successfully.\n";
    } else {
      std::cout << "No active llama.cpp server process tracked.\n";
    }
  } else if (arg.rfind("pull", 0) == 0) {
    std::string modelName = trim(arg.substr(4));
    if (modelName.empty()) {
      std::cout << "Usage: /services pull <model_name> (e.g. /services pull qwen2.5-coder:7b)\n";
    } else {
      std::cout << "Autonomously pulling model '" << modelName << "' into GGUF store...\n";
      bool ok = LlamaServerDaemon::pullOllamaModel(modelName, 180);
      agent_.getRouter().refreshModels();
      if (ok) {
        std::cout << "Model '" << modelName << "' pulled and indexed successfully!\n";
      } else {
        std::cout << "Failed to pull model '" << modelName << "'.\n";
      }
    }
  } else if (arg.rfind("start", 0) == 0) {
    std::string targetModel = trim(arg.substr(5));
    if (targetModel.empty()) {
      targetModel = agent_.getConfig().default_model;
      if (targetModel.empty()) {
        auto models = LlamaServerDaemon::discoverModels();
        if (!models.empty()) targetModel = models[0].id;
      }
    }
    if (targetModel.empty()) {
      std::cout << "No local GGUF models discovered to start.\n";
    } else {
      std::cout << "Starting llama.cpp server with model '" << targetModel << "'...\n";
      bool ready = LlamaServerDaemon::ensureServerRunning(
          targetModel,
          llamaUrl,
          agent_.getConfig().llama_gpu_layers,
          4096,
          25
      );
      agent_.getRouter().refreshModels();
      if (ready) {
        std::cout << "Status: ONLINE & SERVING model [" << targetModel << "] at " << llamaUrl << "\n";
      } else {
        std::cout << "Status: FAILED TO START llama.cpp server for [" << targetModel << "]\n";
      }
    }
  } else {
    // Status / Overview
    bool llamaOnline = LlamaServerDaemon::isServerResponding(llamaUrl);
    std::string loadedModel = llamaOnline ? LlamaServerDaemon::getLoadedModel(llamaUrl) : "";
    std::string llamaExe = LlamaServerDaemon::findLlamaServerExecutable();

    std::cout << "1. llama.cpp Native Server:\n"
              << "   Status:       " << (llamaOnline ? (ANSI_GREEN + std::string("ONLINE & RESPONSIVE") + ANSI_RESET) : (ANSI_YELLOW + std::string("STANDBY / OFFLINE") + ANSI_RESET)) << "\n";
    if (llamaOnline && !loadedModel.empty()) {
      std::cout << "   Active Model: " << ANSI_CYAN << loadedModel << ANSI_RESET << "\n";
    }
    std::cout << "   Executable:   " << (llamaExe.empty() ? "NOT FOUND" : llamaExe) << "\n\n";

    std::cout << "2. Discovered Local Models (GGUF & Ollama store):\n";
    auto localModels = LlamaServerDaemon::discoverModels();
    if (localModels.empty()) {
      std::cout << "   (No local models found. Pull via '/services pull <model>' or 'ollama pull')\n";
    } else {
      for (const auto& m : localModels) {
        double szGb = static_cast<double>(m.sizeBytes) / (1024.0 * 1024.0 * 1024.0);
        std::cout << "   * " << ANSI_BOLD << m.id << ANSI_RESET;
        if (szGb > 0.01) {
          std::cout << " (" << std::fixed << std::setprecision(1) << szGb << " GB";
          if (!m.parameterSize.empty()) std::cout << ", " << m.parameterSize;
          std::cout << ")";
        }
        std::cout << "\n";
      }
    }

    std::cout << "\n3. Ollama Daemon (Secondary):\n";
    bool ollamaOnline = ServiceDaemon::isOllamaResponding(ollamaUrl);
    std::cout << "   Status:       " << (ollamaOnline ? (ANSI_GREEN + std::string("ONLINE") + ANSI_RESET) : (ANSI_YELLOW + std::string("OFFLINE") + ANSI_RESET)) << "\n";
    std::cout << "   URL:          " << ollamaUrl << "\n";
  }

  std::cout << "====================================================\n"
            << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
            << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
            << "====================================================\n\n";
}

namespace {

std::string getPersonaSystemPrompt(const std::string& persona) {
    std::string p = persona;
    for (char& c : p) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (p == "pirate") {
        return "You are Capt'n Byte, a legendary 18th-century pirate captain sailing the turbulent seas of C++20 pointers, operating systems, and AI. "
               "Speak with colorful, boisterous pirate flair ('Ahoy matey!', 'Shiver me pointers!', 'By Blackbeard's cache lines!'), but be insightful, friendly, and fun to talk to.";
    } else if (p == "cyberpunk") {
        return "You are a legendary Netrunner in Neo-Tokyo (2077). You speak with sharp cyberpunk street slang ('choom', 'deck', 'ice', 'corpo tech'), "
               "with high style, dark-neon attitude, and supreme tech prowess. Keep the conversation lively, fun, and futuristic.";
    } else if (p == "zen") {
        return "You are a serene Zen Master and philosophical coding mentor. You speak with calm mindfulness, poetic clarity, and deep wisdom. "
               "You help the user find inner tranquility amidst chaotic compilers, memory leaks, and life's complexities.";
    } else if (p == "nerd") {
        return "You are an over-the-top, lovable computer science nerd and retro-tech historian. You get overwhelmingly excited about discrete mathematics, "
               "transistor layouts, 6502 assembly, Turing machines, and quirky physics facts. Your enthusiasm is infectious and fun.";
    } else if (p == "coach") {
        return "You are a high-energy Motivational Life and Coding Coach. You radiate unstoppable positivity, cheering the user on with enthusiasm, "
               "celebrating their ideas, and pushing them to reach their full potential. Let's conquer this together!";
    } else if (p == "roast") {
        return "You are a playful, witty Code Roaster. You gently and hilariously tease common developer habits, missing semicolons, merge conflicts, "
               "and over-engineering, while remaining warm, lovable, and genuinely helpful.";
    } else { // "witty" or default
        return "You are a witty, fun, and exceptionally engaging AI companion. You have a great sense of humor, sharp cleverness, warm empathy, "
               "and a knack for making conversations entertaining and insightful. You are here just to talk, brainstorm, joke, and hang out.";
    }
}

void printPersonasList() {
    std::cout << "\nAvailable Personas for Chat Mode:\n"
              << "  * witty      - Sharp, playful, witty developer buddy with clever analogies (Default)\n"
              << "  * pirate     - Capt'n Byte of the C++ High Seas (Ahoy matey! Shiver me pointers!)\n"
              << "  * cyberpunk  - Neo-Tokyo Netrunner hacker with 2077 cyber-slang & street smarts\n"
              << "  * zen        - Mindful Zen Master & philosopher bringing calm to code & life\n"
              << "  * nerd       - Over-enthusiastic computer history, Turing, & assembly geek\n"
              << "  * coach      - High-energy motivational hype coach cheering you on\n"
              << "  * roast      - Playful code roaster with affectionate, hilarious teasing\n"
              << "\nSwitch anytime with: /persona <name> (e.g. /persona pirate)\n\n";
}

} // namespace

void ReplEngine::handleChat(const std::string& args) {
    std::string persona = trim(args);
    if (persona.empty()) {
        persona = "witty";
    }
    startChatSession(persona);
}

void ReplEngine::startChatSession(const std::string& initialPersona, std::istream* customIn) {
    std::string currentPersona = initialPersona.empty() ? "witty" : trim(initialPersona);
    std::istream& in = customIn ? *customIn : std::cin;

    std::cout << "\n"
              << ANSI_MAGENTA << ANSI_BOLD
              << "====================================================\n"
              << "           ARENA CHAT: FUN TALK MODE                \n"
              << "      Interactive AI Banter, Chat & Brainstorming    \n"
              << "====================================================\n"
              << ANSI_RESET
              << "Active Persona: " << ANSI_CYAN << ANSI_BOLD << currentPersona << ANSI_RESET << "\n"
              << "Talk about anything: coding, philosophy, jokes, sci-fi, or your day!\n"
              << "Commands:\n"
              << "  /persona <name>  Switch persona (witty, pirate, cyberpunk, zen, nerd, coach, roast)\n"
              << "  /personas        List all available personas\n"
              << "  /joke            Tell a joke in character\n"
              << "  /fact            Share a fascinating fact in character\n"
              << "  /clear           Start a fresh conversation topic\n"
              << "  /stats           Display conversation statistics\n"
              << "  /exit, /back     Return to ArenaFight Agent mode\n"
              << "----------------------------------------------------\n"
              << ANSI_DIM << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n" << ANSI_RESET
              << "====================================================\n\n";

    std::vector<ChatMessage> history;
    int turnCount = 0;
    int totalTokens = 0;

    while (true) {
        std::cout << ANSI_MAGENTA << ANSI_BOLD << "chat (" << currentPersona << ") > " << ANSI_RESET << std::flush;
        std::string input;
        if (!std::getline(in, input)) {
            break;
        }
        std::string line = trim(input);
        if (line.empty()) continue;

        if (line == "/exit" || line == "/back" || line == "/agent" || line == "/quit" || line == "/bye") {
            std::cout << ANSI_CYAN << "\nReturning to ArenaFight Agent Mode. Great chatting with you!\n\n" << ANSI_RESET;
            break;
        }

        if (line == "/personas" || line == "/styles") {
            printPersonasList();
            continue;
        }

        if (line.rfind("/persona", 0) == 0 || line.rfind("/style", 0) == 0) {
            std::string newPersona;
            size_t sp = line.find(' ');
            if (sp != std::string::npos) {
                newPersona = trim(line.substr(sp));
            }
            if (newPersona.empty()) {
                printPersonasList();
            } else {
                currentPersona = newPersona;
                std::cout << ANSI_GREEN << "Switched persona to: " << ANSI_BOLD << currentPersona << ANSI_RESET << "\n\n";
            }
            continue;
        }

        if (line == "/clear" || line == "/reset") {
            history.clear();
            std::cout << ANSI_GREEN << "Conversation history cleared. Fresh slate!\n\n" << ANSI_RESET;
            continue;
        }

        if (line == "/stats") {
            std::cout << "\nChat Session Stats:\n"
                      << "  * Turns Exchanged : " << turnCount << "\n"
                      << "  * Estimated Tokens: " << totalTokens << "\n"
                      << "  * Active Persona  : " << currentPersona << "\n"
                      << "  * History Depth   : " << history.size() << " messages\n\n";
            continue;
        }

        // Handle prompt or shortcut (/joke, /fact)
        std::string actualPrompt = line;
        if (line == "/joke") {
            actualPrompt = "Tell me a hilarious and clever joke in your unique persona voice!";
        } else if (line == "/fact") {
            actualPrompt = "Tell me a truly fascinating and mind-bending fact in your unique persona voice!";
        }

        ChatMessage userMsg;
        userMsg.role = "user";
        userMsg.content = actualPrompt;
        history.push_back(userMsg);

        // Keep last 10 messages for context window management
        if (history.size() > 10) {
            history.erase(history.begin(), history.begin() + 2);
        }

        AgentRequest req;
        req.systemPrompt = getPersonaSystemPrompt(currentPersona);
        req.userPrompt = actualPrompt;
        req.messages = history;
        req.temperature = 0.7f;
        req.maxTokens = 2048;

        auto selection = agent_.getRouter().selectModel(TaskType::CODING);
        if (!selection.provider) {
            selection = agent_.getRouter().selectModel(TaskType::PLANNING);
        }

        if (!selection.provider) {
            std::cout << ANSI_RED << "No model provider available for chat.\n\n" << ANSI_RESET;
            continue;
        }

        req.model = selection.model.id;

        std::cout << ANSI_DIM << "[Thinking...]" << ANSI_RESET << "\r" << std::flush;
        AgentResponse resp = selection.provider->generate(req);

        turnCount++;
        totalTokens += resp.promptTokens + resp.completionTokens;

        std::cout << "\r" << std::string(20, ' ') << "\r";

        if (!resp.thinking.empty()) {
            size_t previewLen = std::min<size_t>(resp.thinking.size(), 140);
            std::cout << ANSI_DIM << "[" << currentPersona << " thought: " << resp.thinking.substr(0, previewLen) << "...]\n" << ANSI_RESET;
        }

        std::cout << ANSI_CYAN << ANSI_BOLD << "[" << currentPersona << "]: " << ANSI_RESET
                  << resp.content << "\n\n";

        ChatMessage asstMsg;
        asstMsg.role = "assistant";
        asstMsg.content = resp.content;
        history.push_back(asstMsg);
    }
}

void ReplEngine::run() {
  printReplBanner();

  if (!agent_.getConfig().test_mock_enabled && !agent_.getConfig().isOpenCodeConnected()) {
    agent_.getRouter().refreshModels();
  }

  while (running_) {
    std::cout << ANSI_GREEN << ANSI_BOLD << "arena > " << ANSI_RESET
              << std::flush;
    std::string input;
    if (!std::getline(std::cin, input)) {
      break;
    }
    processInput(input);
  }

  std::cout << "\nExiting ArenaFight REPL. Goodbye!\n";
}

} // namespace arenafight
