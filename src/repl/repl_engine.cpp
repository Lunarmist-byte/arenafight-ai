#include "arenafight/repl/repl_engine.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/providers/opencode_provider.hpp"
#include "arenafight/hardware/hardware_manager.hpp"
#include "arenafight/hardware/board_registry.hpp"
#include "arenafight/hardware/circuit_calculator.hpp"
#include "arenafight/hardware/hardware_validator.hpp"
#include "arenafight/hardware/hardware_scaffolder.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <algorithm>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

namespace {

const std::string ANSI_RESET  = "\033[0m";
const std::string ANSI_BOLD   = "\033[1m";
const std::string ANSI_DIM    = "\033[2m";
const std::string ANSI_CYAN   = "\033[36m";
const std::string ANSI_GREEN  = "\033[32m";
const std::string ANSI_YELLOW = "\033[33m";
const std::string ANSI_BLUE   = "\033[34m";
const std::string ANSI_MAGENTA= "\033[35m";
const std::string ANSI_RED    = "\033[31m";

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

} // namespace

ReplEngine::ReplEngine(ArenaAgent& agent)
    : agent_(agent),
      artifacts_(agent.getConfig().workspace_dir + "/.arena/artifacts"),
      customizations_(agent.getConfig().workspace_dir) {
    registerCommands();
}

void ReplEngine::registerCommands() {
    slashCommands_["/help"] = {
        "/help",
        "Display command reference and Antigravity workflow guide",
        "/help [command]",
        [this](const std::string& args) { handleHelp(args); }
    };

    slashCommands_["/goal"] = {
        "/goal",
        "Autonomous goal pursuit with self-healing until fully verified",
        "/goal <task description>",
        [this](const std::string& args) { handleGoal(args); }
    };

    slashCommands_["/plan"] = {
        "/plan",
        "Break down task into structured plan artifact with dependencies",
        "/plan <task description>",
        [this](const std::string& args) { handlePlan(args); }
    };

    slashCommands_["/grill-me"] = {
        "/grill-me",
        "Interactive requirements interview to resolve edge cases and design decisions",
        "/grill-me <project or feature idea>",
        [this](const std::string& args) { handleGrillMe(args); }
    };

    slashCommands_["/learn"] = {
        "/learn",
        "Persist a Knowledge Item (KI) or rule into repository memory",
        "/learn <insight or rule description>",
        [this](const std::string& args) { handleLearn(args); }
    };

    slashCommands_["/schedule"] = {
        "/schedule",
        "Schedule a task to run after a delay or periodically",
        "/schedule <delay_sec> <task description>",
        [this](const std::string& args) { handleSchedule(args); }
    };

    slashCommands_["/artifacts"] = {
        "/artifacts",
        "List, inspect, and preview generated project artifacts",
        "/artifacts [artifact-id]",
        [this](const std::string& args) { handleArtifacts(args); }
    };

    slashCommands_["/skills"] = {
        "/skills",
        "List available skills and view skill instructions",
        "/skills [skill-name]",
        [this](const std::string& args) { handleSkills(args); }
    };

    slashCommands_["/rules"] = {
        "/rules",
        "Display active workspace and system rules",
        "/rules",
        [this](const std::string& args) { handleRules(args); }
    };

    slashCommands_["/models"] = {
        "/models",
        "List available models and switch active reasoning provider",
        "/models",
        [this](const std::string& args) { handleModels(args); }
    };

    slashCommands_["/status"] = {
        "/status",
        "View current session state, claims confidence, and verification telemetry",
        "/status",
        [this](const std::string& args) { handleStatus(args); }
    };

    slashCommands_["/opencode"] = {
        "/opencode",
        "OpenCode integration status, model discovery, and prompt execution",
        "/opencode [status|models|run <prompt>|sync]",
        [this](const std::string& args) { handleOpenCode(args); }
    };

    slashCommands_["/ui"] = {
        "/ui",
        "Launch the Simplistic Web UI dashboard in your browser",
        "/ui [simple|studio]",
        [this](const std::string& args) { handleUI(args); }
    };

    slashCommands_["/boards"] = {
        "/boards",
        "List all supported microcontroller boards and architectures",
        "/boards [filter]",
        [this](const std::string& args) { handleBoards(args); }
    };

    slashCommands_["/board"] = {
        "/board",
        "Inspect microcontroller pinouts, strapping pins, and voltages",
        "/board <board-id>",
        [this](const std::string& args) { handleBoard(args); }
    };

    slashCommands_["/components"] = {
        "/components",
        "List supported sensors, actuators, and communication ICs",
        "/components [filter]",
        [this](const std::string& args) { handleComponents(args); }
    };

    slashCommands_["/scaffold"] = {
        "/scaffold",
        "Scaffold an embedded project with Wokwi diagram and QoS firmware",
        "/scaffold <board> <project_name>",
        [this](const std::string& args) { handleScaffold(args); }
    };

    slashCommands_["/qos"] = {
        "/qos",
        "Evaluate embedded firmware for zero-heap, non-blocking QoS compliance",
        "/qos [path]",
        [this](const std::string& args) { handleQoS(args); }
    };

    slashCommands_["/calculate"] = {
        "/calculate",
        "Calculate circuit values: led, divider, i2c pullup, or battery lifetime",
        "/calculate <led|divider|i2c|battery> <args>",
        [this](const std::string& args) { handleCalculate(args); }
    };

    slashCommands_["/clear"] = {
        "/clear",
        "Clear console screen",
        "/clear",
        [](const std::string&) {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
        }
    };

    slashCommands_["/exit"] = {
        "/exit",
        "Exit interactive REPL",
        "/exit",
        [this](const std::string&) { running_ = false; }
    };

    slashCommands_["/quit"] = {
        "/quit",
        "Exit interactive REPL",
        "/quit",
        [this](const std::string&) { running_ = false; }
    };
}

void ReplEngine::printReplBanner() {
    std::cout << ANSI_CYAN << ANSI_BOLD
              << "====================================================\n"
              << "       ARENAFIGHT - ANTIGRAVITY ENGINE (C++20)      \n"
              << "   Autonomous Multi-Model Multi-Turn Agent Runtime  \n"
              << "----------------------------------------------------\n"
              << " Made by: Lunarmist-byte\n"
              << " GitHub:   https://github.com/Lunarmist-byte\n"
              << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
              << "====================================================\n"
              << ANSI_RESET << "\n"
              << ANSI_DIM << "Type " << ANSI_YELLOW << "/help" << ANSI_DIM << " for slash commands (/goal, /plan, /grill-me, /learn, /schedule).\n"
              << "Use " << ANSI_YELLOW << "@file <path>" << ANSI_DIM << " or " << ANSI_YELLOW << "@git" << ANSI_DIM << " to mention context.\n"
              << "Type " << ANSI_YELLOW << "/exit" << ANSI_DIM << " to quit.\n\n"
              << ANSI_RESET;
}

void ReplEngine::printHelpTable() {
    std::cout << "\n" << ANSI_BOLD << "Available Slash Commands:" << ANSI_RESET << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    for (const auto& [cmd, info] : slashCommands_) {
        std::cout << "  " << ANSI_CYAN << std::left;
        std::cout.width(14);
        std::cout << info.name << ANSI_RESET << " " << info.description << "\n";
        std::cout << "  " << ANSI_DIM << "Usage: " << info.usage << ANSI_RESET << "\n\n";
    }
    std::cout << ANSI_BOLD << "Context Mentions:" << ANSI_RESET << "\n";
    std::cout << "  " << ANSI_YELLOW << "@file <path>" << ANSI_RESET << " : Attach file contents directly to your prompt\n";
    std::cout << "  " << ANSI_YELLOW << "@git" << ANSI_RESET << "        : Attach working tree status and git diff\n";
    std::cout << "  " << ANSI_YELLOW << "@claims" << ANSI_RESET << "     : Attach current evidence & verified facts from ClaimRegistry\n";
    std::cout << "  " << ANSI_YELLOW << "@memory" << ANSI_RESET << "     : Attach discoveries and past session decisions\n\n";
}

std::string ReplEngine::askUserInteractive(const std::string& prompt) {
    std::cout << ANSI_YELLOW << prompt << " " << ANSI_RESET << std::flush;
    std::string response;
    std::getline(std::cin, response);
    return trim(response);
}

void ReplEngine::displayThinking(const std::string& thoughts) {
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

std::string ReplEngine::resolveMentions(const std::string& prompt) {
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

    // Check @file <path>
    size_t filePos = resolved.find("@file ");
    if (filePos != std::string::npos) {
        size_t pathStart = filePos + 6;
        size_t pathEnd = resolved.find_first_of(" \t\n\r", pathStart);
        std::string filePath = (pathEnd == std::string::npos)
            ? resolved.substr(pathStart)
            : resolved.substr(pathStart, pathEnd - pathStart);

        std::filesystem::path p(filePath);
        if (std::filesystem::exists(p)) {
            std::ifstream in(p);
            std::stringstream ss;
            ss << in.rdbuf();
            std::string fileContent = "\n[CONTEXT FILE: " + filePath + "]\n```\n" + ss.str() + "\n```\n";
            resolved.replace(filePos, 6 + filePath.length(), fileContent);
        } else {
            resolved.replace(filePos, 6 + filePath.length(), "[File not found: " + filePath + "]");
        }
    }

    // Check @claims
    size_t claimsPos = resolved.find("@claims");
    if (claimsPos != std::string::npos) {
        std::string claimsReport = agent_.getSession().getClaimRegistry().getEvidenceReportSummary();
        resolved.replace(claimsPos, 7, "\n[CONTEXT CLAIMS]\n" + claimsReport);
    }

    return resolved;
}

void ReplEngine::handleGoal(const std::string& task) {
    if (task.empty()) {
        std::cout << ANSI_RED << "Error: /goal requires a task description.\n"
                  << "Example: /goal Build a multi-threaded crawler with tests and documentation\n"
                  << ANSI_RESET;
        return;
    }

    std::cout << "\n" << ANSI_CYAN << ANSI_BOLD
              << "[ANTIGRAVITY GOAL MODE ACTIVE]" << ANSI_RESET << "\n"
              << "Pursuing objective autonomously until all 6 verification gates pass:\n"
              << "Objective: " << task << "\n\n";

    agent_.startNewSession(task);
    bool success = agent_.run();

    // Create a verification report artifact
    std::string report = "Goal: " + task + "\nResult: " + (success ? "VERIFIED & COMPLETED" : "INCOMPLETE") + "\n";
    artifacts_.createVerificationArtifact(report, success);

    std::cout << "\n" << ANSI_GREEN
              << "Artifact generated: .arena/artifacts/verification_report.md\n"
              << ANSI_RESET;
}

void ReplEngine::handlePlan(const std::string& task) {
    if (task.empty()) {
        std::cout << ANSI_RED << "Error: /plan requires a task description.\n"
                  << "Example: /plan Scaffold a high-reliability hardware project with Wokwi simulation\n"
                  << ANSI_RESET;
        return;
    }

    std::cout << "\n" << ANSI_CYAN << "[ANTIGRAVITY PLANNER]" << ANSI_RESET
              << " Formulating structured execution plan...\n";

    displayThinking("Analyzing workspace structure, dependencies, and requirements for: " + task + "\n"
                    "Checking matched skills and active custom rules...\n"
                    "Generating multi-step task graph with explicit verification milestones.");

    // Match skills
    auto skills = customizations_.matchRelevantSkills(task);
    if (!skills.empty()) {
        std::cout << ANSI_BLUE << "Matched Skills: ";
        for (const auto& s : skills) {
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
           << "1. **Discovery & Environment Inspection**: Examine files and verify toolchains.\n"
           << "2. **Architecture & Implementation**: Author robust source files following standard QoS guidelines.\n"
           << "3. **Self-Healing Verification**: Build code (`cmake` / `cargo check`), execute test suite (`ctest`), and ensure zero unresolved contradictions.\n"
           << "4. **Evidence Grounding**: Confirm physical files exist and all 6 verification gates pass.\n\n"
           << "### Acceptance Criteria\n"
           << "- Zero compile warnings/errors.\n"
           << "- Automated unit tests exit with code 0.\n"
           << "- Author credit properly preserved.\n";

    Artifact planArt = artifacts_.createPlanArtifact(task, planSs.str(), true);

    std::cout << ANSI_BOLD << "====================================================\n"
              << "               PROPOSED EXECUTION PLAN              \n"
              << "====================================================\n" << ANSI_RESET
              << planSs.str() << "\n"
              << ANSI_GREEN << "Artifact saved to: " << planArt.path << ANSI_RESET << "\n\n";

    std::string choice = askUserInteractive("Proceed with execution? ([P]roceed / [E]dit / [C]ancel):");
    if (choice == "p" || choice == "P" || choice == "proceed" || choice == "Proceed") {
        std::cout << "\n" << ANSI_GREEN << "Plan approved! Launching autonomous execution...\n" << ANSI_RESET;
        handleGoal(task);
    } else if (choice == "e" || choice == "E" || choice == "edit") {
        std::string extra = askUserInteractive("Enter modifications or additional constraints:");
        std::string modifiedTask = task + " (Constraints: " + extra + ")";
        handlePlan(modifiedTask);
    } else {
        std::cout << ANSI_YELLOW << "Plan execution cancelled by user.\n" << ANSI_RESET;
    }
}

void ReplEngine::handleGrillMe(const std::string& topic) {
    std::string subject = topic;
    if (subject.empty()) {
        subject = askUserInteractive("What project, feature, or architecture would you like to be grilled on?");
    }

    std::cout << "\n" << ANSI_MAGENTA << ANSI_BOLD
              << "[ANTIGRAVITY /GRILL-ME INTERVIEW]" << ANSI_RESET << "\n"
              << "Probing edge cases, architectural trade-offs, and failure modes for:\n"
              << ANSI_BOLD << subject << ANSI_RESET << "\n\n";

    std::vector<std::string> questions = {
        "1. Concurrency & Reentrancy: How should the system behave under high concurrent loads or abrupt restarts?",
        "2. Failure Modes & Fallbacks: If external APIs or hardware sensors time out, what is the exact degraded behavior?",
        "3. Evidence & Verification: What concrete compiler, test, or telemetry exit codes define successful completion?",
        "4. Memory & Resource Limits: What are the strict memory/VRAM constraints and cleanup guarantees (RAII) required?"
    };

    std::vector<std::string> answers;
    for (const auto& q : questions) {
        std::cout << ANSI_CYAN << q << ANSI_RESET << "\n";
        std::string ans = askUserInteractive("Answer (or press Enter for standard defaults):");
        answers.push_back(ans.empty() ? "Standard resilient defaults" : ans);
        std::cout << "\n";
    }

    std::cout << ANSI_GREEN << ANSI_BOLD << "Interview Complete!" << ANSI_RESET << " Synthesizing aligned requirements...\n";

    std::stringstream alignedTask;
    alignedTask << subject << " [Aligned Spec: ";
    for (size_t i = 0; i < questions.size(); ++i) {
        alignedTask << "Q" << (i+1) << ": " << answers[i] << "; ";
    }
    alignedTask << "]";

    // Save as Knowledge Item
    customizations_.addKnowledge(
        "Grill-Me Spec: " + subject,
        "Requirements synthesized from interactive interview",
        alignedTask.str(),
        {"grill-me", "specification"}
    );

    std::string proceed = askUserInteractive("Generate execution plan for this aligned specification now? ([Y]es / [N]o):");
    if (proceed == "y" || proceed == "Y" || proceed == "yes") {
        handlePlan(alignedTask.str());
    }
}

void ReplEngine::handleLearn(const std::string& insight) {
    if (insight.empty()) {
        std::cout << ANSI_RED << "Error: /learn requires an insight, convention, or rule to persist.\n"
                  << "Example: /learn In this project, all embedded timers must use std::chrono with millisecond precision.\n"
                  << ANSI_RESET;
        return;
    }

    std::string title = askUserInteractive("Enter a short title for this Knowledge Item:");
    if (title.empty()) title = "Learned Convention";

    KnowledgeItem ki = customizations_.addKnowledge(
        title,
        insight,
        insight,
        {"learned", "convention"}
    );

    std::cout << "\n" << ANSI_GREEN << ANSI_BOLD
              << "[KNOWLEDGE PERSISTED]" << ANSI_RESET << "\n"
              << "Knowledge Item ID: " << ki.id << "\n"
              << "Saved to: .arena/knowledge/" << ki.id << "/metadata.json\n"
              << "This insight will be automatically injected into future reasoning contexts.\n\n";
}

void ReplEngine::handleSchedule(const std::string& args) {
    std::stringstream ss(args);
    int delaySec = 0;
    ss >> delaySec;
    std::string task;
    std::getline(ss, task);
    task = trim(task);

    if (delaySec <= 0 || task.empty()) {
        std::cout << ANSI_RED << "Error: /schedule syntax is: /schedule <seconds> <task>\n"
                  << "Example: /schedule 10 Run automated test suite and check memory leaks\n"
                  << ANSI_RESET;
        return;
    }

    std::cout << ANSI_CYAN << "[SCHEDULED TASK CREATED]" << ANSI_RESET << "\n"
              << "Waiting " << delaySec << " seconds before executing: " << task << "...\n";

    // Non-blocking timer thread simulation
    std::thread([this, delaySec, task]() {
        std::this_thread::sleep_for(std::chrono::seconds(delaySec));
        std::cout << "\n\n" << ANSI_GREEN << ANSI_BOLD
                  << "[TIMER FIRED]" << ANSI_RESET << " Executing scheduled task: " << task << "\n";
        agent_.startNewSession(task);
        agent_.run();
        std::cout << "\narena > " << std::flush;
    }).detach();
}

void ReplEngine::handleArtifacts(const std::string& args) {
    artifacts_.reloadFromDisk();
    auto list = artifacts_.listArtifacts();

    if (!args.empty()) {
        auto art = artifacts_.getArtifact(args);
        if (!art) art = artifacts_.getArtifactByFilename(args);
        if (art) {
            std::cout << "\n" << ANSI_BOLD << "Artifact: " << art->title << " (" << art->filename << ")" << ANSI_RESET << "\n"
                      << "Type: " << artifactTypeToString(art->type) << "\n"
                      << "Path: " << art->path << "\n"
                      << "Summary: " << art->metadata.summary << "\n"
                      << "----------------------------------------------------\n"
                      << art->content << "\n"
                      << "----------------------------------------------------\n";
            return;
        }
        std::cout << ANSI_YELLOW << "Artifact [" << args << "] not found.\n" << ANSI_RESET;
    }

    std::cout << "\n" << ANSI_BOLD << "Project Artifacts (" << list.size() << "):" << ANSI_RESET << "\n"
              << "--------------------------------------------------------------------------------\n";
    if (list.empty()) {
        std::cout << ANSI_DIM << "  No artifacts generated yet. Run /plan or /goal to generate artifacts.\n" << ANSI_RESET;
    } else {
        for (const auto& a : list) {
            std::cout << "  " << ANSI_CYAN << std::left;
            std::cout.width(18);
            std::cout << a.id << ANSI_RESET << " " << std::left;
            std::cout.width(24);
            std::cout << a.filename << ANSI_RESET << " [" << artifactTypeToString(a.type) << "] "
                      << a.metadata.summary << "\n";
        }
    }
    std::cout << "\n" << ANSI_DIM << "Tip: Run /artifacts <id> to inspect complete contents.\n\n" << ANSI_RESET;
}

void ReplEngine::handleSkills(const std::string& args) {
    customizations_.refresh();
    const auto& skills = customizations_.getSkills();

    if (!args.empty()) {
        auto s = customizations_.getSkill(args);
        if (s) {
            std::cout << "\n" << ANSI_BOLD << "Skill: " << s->name << ANSI_RESET << "\n"
                      << "Description: " << s->description << "\n"
                      << "File: " << s->skillFilePath << "\n"
                      << "----------------------------------------------------\n"
                      << s->instructions << "\n"
                      << "----------------------------------------------------\n";
            return;
        }
        std::cout << ANSI_YELLOW << "Skill [" << args << "] not found.\n" << ANSI_RESET;
    }

    std::cout << "\n" << ANSI_BOLD << "Discovered Skills (" << skills.size() << "):" << ANSI_RESET << "\n"
              << "--------------------------------------------------------------------------------\n";
    for (const auto& s : skills) {
        std::cout << "  " << ANSI_CYAN << std::left;
        std::cout.width(20);
        std::cout << s.name << ANSI_RESET << " " << s.description << "\n";
    }
    std::cout << "\n" << ANSI_DIM << "Skills directory: .arena/skills/<skill-name>/SKILL.md\n"
              << "Tip: Run /skills <name> to view full skill instructions.\n\n" << ANSI_RESET;
}

void ReplEngine::handleRules(const std::string&) {
    customizations_.refresh();
    const auto& rules = customizations_.getRules();

    std::cout << "\n" << ANSI_BOLD << "Active Custom & System Rules (" << rules.size() << "):" << ANSI_RESET << "\n"
              << "--------------------------------------------------------------------------------\n";
    for (const auto& r : rules) {
        std::cout << "  " << ANSI_CYAN << r.name << ANSI_RESET << " (" << r.sourcePath << "):\n"
                  << "    " << ANSI_DIM << r.content << ANSI_RESET << "\n\n";
    }
}

void ReplEngine::handleModels(const std::string&) {
    agent_.getRouter().refreshModels();
    auto models = agent_.getRouter().getAllAvailableModels();
    TerminalUI::printModelsList(models);
}

void ReplEngine::handleStatus(const std::string&) {
    const auto& session = agent_.getSession();
    std::cout << "\n" << ANSI_BOLD << "Current Session Telemetry:" << ANSI_RESET << "\n"
              << "----------------------------------------------------\n"
              << "Session ID   : " << (session.id.empty() ? "(None active)" : session.id) << "\n"
              << "State        : " << session.currentState << "\n"
              << "Completed    : " << (session.completed ? "YES" : "NO") << "\n"
              << "Iteration    : " << session.iteration << "\n"
              << "Active Tasks : " << session.tasks.size() << "\n"
              << "Memories     : " << session.memories.size() << "\n"
              << "Verified Facts: " << session.getClaimRegistry().getVerifiedClaims().size() << "\n"
              << "Contradictions: " << session.getClaimRegistry().getContradictions().size() << "\n"
              << "Workspace    : " << agent_.getConfig().workspace_dir << "\n"
              << "----------------------------------------------------\n\n";
}

void ReplEngine::handleOpenCode(const std::string& args) {
    std::string trimmed = trim(args);
    auto providerPtr = agent_.getRouter().getProviderByName("OpenCode");
    auto ocProvider = std::dynamic_pointer_cast<OpenCodeProvider>(providerPtr);

    if (trimmed.empty() || trimmed == "status") {
        std::cout << "\n" << ANSI_BOLD << "OpenCode CLI Integration Status:" << ANSI_RESET << "\n"
                  << "----------------------------------------------------\n";
        if (ocProvider) {
            bool avail = ocProvider->available();
            std::cout << "Command      : " << ocProvider->getOpenCodeCommand() << "\n"
                      << "Status       : " << (avail ? (ANSI_GREEN + std::string("AVAILABLE") + ANSI_RESET) : (ANSI_YELLOW + std::string("UNAVAILABLE") + ANSI_RESET)) << "\n"
                      << "Details      : " << ocProvider->getStatusMessage() << "\n";
            if (avail) {
                auto models = ocProvider->discoverModels();
                std::cout << "Models (" << models.size() << ")  :\n";
                for (const auto& m : models) {
                    std::cout << "  - " << ANSI_CYAN << m.id << ANSI_RESET << " (" << m.name << ")\n";
                }
            } else {
                std::cout << "\nTo install OpenCode CLI:\n"
                          << "  " << ANSI_CYAN << "npm install -g @opencode/cli" << ANSI_RESET << "\n"
                          << "  or: " << ANSI_CYAN << "curl -fsSL https://opencode.ai/v2/install | bash" << ANSI_RESET << "\n";
            }
        } else {
            std::cout << ANSI_YELLOW << "OpenCodeProvider is not registered in the active agent router." << ANSI_RESET << "\n";
        }
        std::cout << "----------------------------------------------------\n"
                  << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n\n";
        return;
    }

    if (trimmed == "models") {
        if (ocProvider) {
            auto models = ocProvider->discoverModels();
            std::cout << "\n" << ANSI_BOLD << "OpenCode Models (" << models.size() << "):" << ANSI_RESET << "\n";
            for (const auto& m : models) {
                std::cout << "  * " << ANSI_CYAN << m.id << ANSI_RESET << " : " << m.description << "\n";
            }
            std::cout << "\n";
        }
        return;
    }

    if (trimmed == "sync" || trimmed == "export") {
        bool ok = OpenCodeProvider::exportOpenCodeWorkspace(
            agent_.getConfig().workspace_dir,
            agent_.getSession().objective,
            "Autonomous ArenaFight execution loop with physical verification."
        );
        if (ok) {
            std::cout << "\n" << ANSI_GREEN << ANSI_BOLD << "[OPENCODE WORKSPACE SYNCED]" << ANSI_RESET << "\n"
                      << "Created: .opencode/opencode.json and .opencode/INSTRUCTIONS.md\n"
                      << "OpenCode and ArenaFight can now collaborate in this workspace.\n\n";
        } else {
            std::cout << ANSI_RED << "Failed to export OpenCode workspace configuration." << ANSI_RESET << "\n";
        }
        return;
    }

    if (trimmed.rfind("run ", 0) == 0) {
        std::string prompt = trim(trimmed.substr(4));
        if (prompt.empty()) {
            std::cout << ANSI_RED << "Usage: /opencode run <prompt>" << ANSI_RESET << "\n";
            return;
        }
        if (!ocProvider || !ocProvider->available()) {
            std::cout << ANSI_YELLOW << "OpenCode CLI is not available. Please install it or check configuration." << ANSI_RESET << "\n";
            return;
        }
        std::cout << ANSI_CYAN << "[OpenCode Execution Initiated]..." << ANSI_RESET << "\n";
        AgentRequest req;
        req.userPrompt = prompt;
        AgentResponse res = ocProvider->generate(req);
        if (res.success) {
            std::cout << "\n" << ANSI_GREEN << ANSI_BOLD << "[OpenCode Output]" << ANSI_RESET << "\n"
                      << res.content << "\n\n";
        } else {
            std::cout << ANSI_RED << "[OpenCode Error] " << res.errorMessage << ANSI_RESET << "\n\n";
        }
        return;
    }

    std::cout << ANSI_YELLOW << "Unknown opencode sub-command. Try: /opencode [status|models|run <prompt>|sync]\n" << ANSI_RESET;
}

void ReplEngine::handleUI(const std::string& args) {
    std::string targetFile = "simple_ui.html";
    if (args.find("studio") != std::string::npos) {
        targetFile = "index.html";
    }

    std::filesystem::path p = std::filesystem::absolute(targetFile);
    std::cout << "\n" << ANSI_CYAN << ANSI_BOLD
              << "[LAUNCHING ARENAFIGHT UI]" << ANSI_RESET << "\n"
              << "Interface: " << (targetFile == "simple_ui.html" ? "Simplistic Minimalist UI" : "Advanced Antigravity Studio") << "\n"
              << "File     : " << p.string() << "\n"
              << "URL      : file:///" << p.generic_string() << "\n\n";

    // Launch default browser
    if (ProcessExecutor::isWindows()) {
        std::string launchCmd = "start \"\" \"" + p.string() + "\"";
        ProcessExecutor::executeShell(launchCmd, ".", 2000);
    } else {
        std::string launchCmd = "xdg-open \"" + p.string() + "\" 2>/dev/null || open \"" + p.string() + "\"";
        ProcessExecutor::executeShell(launchCmd, ".", 2000);
    }
}

void ReplEngine::handleHelp(const std::string& args) {
    if (!args.empty()) {
        std::string cmd = args;
        if (cmd.front() != '/') cmd = "/" + cmd;
        auto it = slashCommands_.find(cmd);
        if (it != slashCommands_.end()) {
            std::cout << "\n" << ANSI_BOLD << it->second.name << ANSI_RESET << "\n"
                      << "  " << it->second.description << "\n"
                      << "  " << ANSI_DIM << "Usage: " << it->second.usage << ANSI_RESET << "\n\n";
            return;
        }
    }
    printHelpTable();
}

void ReplEngine::handleBoards(const std::string& filter) {
    auto boards = HardwareBoardRegistry::instance().getAllBoards();
    if (!filter.empty()) {
        std::vector<BoardSpec> filtered;
        for (const auto& b : boards) {
            if (b.id.find(filter) != std::string::npos || b.name.find(filter) != std::string::npos) {
                filtered.push_back(b);
            }
        }
        TerminalUI::printBoardsTable(filtered);
    } else {
        TerminalUI::printBoardsTable(boards);
    }
}

void ReplEngine::handleBoard(const std::string& boardId) {
    std::string id = boardId;
    if (id.empty()) {
        id = askUserInteractive("Enter board ID to inspect (e.g., esp32, pico, arduino_uno, stm32f401):");
    }
    if (id.empty()) id = "esp32";

    auto b = HardwareBoardRegistry::instance().getBoard(id);
    if (!b) b = HardwareBoardRegistry::instance().findBoard(id);

    if (b) {
        TerminalUI::printBoardDetails(*b);
    } else {
        std::cout << ANSI_YELLOW << "Microcontroller board '" << id << "' not found. Run /boards to view available boards.\n" << ANSI_RESET;
    }
}

void ReplEngine::handleComponents(const std::string& filter) {
    auto comps = HardwareBoardRegistry::instance().getAllComponents();
    if (!filter.empty()) {
        std::vector<ComponentSpec> filtered;
        for (const auto& c : comps) {
            if (c.id.find(filter) != std::string::npos || c.name.find(filter) != std::string::npos || c.category.find(filter) != std::string::npos) {
                filtered.push_back(c);
            }
        }
        TerminalUI::printComponentsTable(filtered);
    } else {
        TerminalUI::printComponentsTable(comps);
    }
}

void ReplEngine::handleScaffold(const std::string& args) {
    std::stringstream ss(args);
    std::string boardId, projectName;
    ss >> boardId >> projectName;

    if (boardId.empty()) {
        boardId = askUserInteractive("Enter target microcontroller board (esp32, pico, arduino_uno):");
    }
    if (projectName.empty()) {
        projectName = askUserInteractive("Enter project name:");
    }
    if (boardId.empty()) boardId = "esp32";
    if (projectName.empty()) projectName = "embedded_project";

    std::cout << "\n" << ANSI_CYAN << "[HARDWARE STUDIO]" << ANSI_RESET
              << " Scaffolding embedded project for board: " << boardId << "...\n";

    ScaffoldOptions opts;
    opts.boardId = boardId;
    opts.projectName = projectName;
    opts.outputDir = agent_.getConfig().workspace_dir;
    opts.generateWokwiSimulation = true;

    auto res = HardwareManager::instance().scaffold(opts);
    if (res.success) {
        std::cout << ANSI_GREEN << ANSI_BOLD << "[SCAFFOLD SUCCESS]" << ANSI_RESET << "\n"
                  << "Project Path: " << res.projectDir << "\n"
                  << "Files Created (" << res.filesCreated.size() << "):\n";
        for (const auto& f : res.filesCreated) {
            std::cout << "  * " << f << "\n";
        }
        std::cout << "\nWokwi Simulation Diagram: " << res.wokwiDiagramPath << "\n";

        std::string artContent = "# Hardware Project Scaffold: " + projectName + "\n\n"
            + "- **Target Board:** " + boardId + "\n"
            + "- **Wokwi Simulation:** `" + res.wokwiDiagramPath + "`\n"
            + "- **PlatformIO Configuration:** `" + res.projectDir + "/platformio.ini`\n"
            + "- **QoS Firmware Source:** `" + res.projectDir + "/src/main.cpp`\n\n";

        ArtifactMetadata meta;
        meta.userFacing = true;
        meta.requestFeedback = false;
        meta.summary = "Scaffolded embedded project with Wokwi simulation";

        artifacts_.createArtifact(
            "Hardware Scaffold: " + projectName,
            "hardware_scaffold.md",
            ArtifactType::REPORT,
            artContent,
            meta
        );

        std::cout << ANSI_CYAN << "Artifact recorded: .arena/artifacts/hardware_scaffold.md\n" << ANSI_RESET;
    } else {
        std::cout << ANSI_RED << "[SCAFFOLD FAILED] " << res.message << ANSI_RESET << "\n";
    }
}

void ReplEngine::handleQoS(const std::string& path) {
    std::string target = path;
    if (target.empty()) target = agent_.getConfig().workspace_dir;

    std::cout << "\n" << ANSI_CYAN << "[FIRMWARE QoS EVALUATION]" << ANSI_RESET
              << " Inspecting: " << target << "...\n";

    auto qos = HardwareManager::instance().evaluateProjectQoS(target);
    TerminalUI::printQoSTelemetryReport(qos);
}

void ReplEngine::handleCalculate(const std::string& args) {
    std::stringstream ss(args);
    std::string type;
    ss >> type;

    if (type == "led") {
        double vcc = 5.0, vf = 2.0, ma = 15.0;
        ss >> vcc >> vf >> ma;
        auto res = CircuitCalculator::calculateLedResistor(vcc, vf, ma);
        std::cout << "\n" << ANSI_BOLD << "LED Resistor Calculation:" << ANSI_RESET << "\n"
                  << "  Formula: " << res.formulaUsed << "\n"
                  << "  Summary: " << res.summary << "\n\n";
        for (const auto& rec : res.componentRecommendations) {
            std::cout << "  * " << rec << "\n";
        }
        std::cout << "\n";
    } else if (type == "divider") {
        double vin = 5.0, vout = 3.3, rk = 10.0;
        ss >> vin >> vout >> rk;
        auto res = CircuitCalculator::calculateVoltageDivider(vin, vout, rk);
        std::cout << "\n" << ANSI_BOLD << "Voltage Divider Calculation (5V -> 3.3V Level Shifting):" << ANSI_RESET << "\n"
                  << "  Formula: " << res.formulaUsed << "\n"
                  << "  Summary: " << res.summary << "\n\n";
        for (const auto& rec : res.componentRecommendations) {
            std::cout << "  * " << rec << "\n";
        }
        std::cout << "\n";
    } else if (type == "i2c") {
        double vcc = 3.3;
        int speed = 100;
        ss >> vcc >> speed;
        auto res = CircuitCalculator::calculateI2cPullup(vcc, speed);
        std::cout << "\n" << ANSI_BOLD << "I2C Bus Pull-Up Resistor Calculation:" << ANSI_RESET << "\n"
                  << "  Formula: " << res.formulaUsed << "\n"
                  << "  Summary: " << res.summary << "\n\n";
        for (const auto& rec : res.componentRecommendations) {
            std::cout << "  * " << rec << "\n";
        }
        std::cout << "\n";
    } else if (type == "battery") {
        double mah = 2500.0, actMa = 80.0, slpUa = 15.0, actSec = 2.0, slpSec = 58.0;
        ss >> mah >> actMa >> slpUa >> actSec >> slpSec;
        auto res = CircuitCalculator::calculateBatteryLife(mah, actMa, slpUa, actSec, slpSec);
        std::cout << "\n" << ANSI_BOLD << "Battery & Deep Sleep Power Budget:" << ANSI_RESET << "\n"
                  << "  Formula: " << res.formulaUsed << "\n"
                  << "  Summary: " << res.summary << "\n\n";
        for (const auto& rec : res.componentRecommendations) {
            std::cout << "  * " << rec << "\n";
        }
        std::cout << "\n";
    } else {
        std::cout << ANSI_YELLOW << "Usage: /calculate <led|divider|i2c|battery> [args]\n"
                  << "  /calculate led 5.0 2.0 15        (Supply V, Forward Vf, Current mA)\n"
                  << "  /calculate divider 5.0 3.3 10    (Vin, Target Vout, R2 kOhms)\n"
                  << "  /calculate i2c 3.3 100           (Supply V, Bus speed kHz)\n"
                  << "  /calculate battery 2500 80 15 2 58 (Capacity mAh, Active mA, Sleep uA, Act Sec, Sleep Sec)\n\n"
                  << ANSI_RESET;
    }
}

void ReplEngine::processInput(const std::string& rawInput) {
    std::string line = trim(rawInput);
    if (line.empty()) return;

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
            std::cout << ANSI_YELLOW << "Unknown slash command: " << cmd << ". Type /help for available commands.\n" << ANSI_RESET;
            return;
        }
    }

    // Conversational prompt or question with context mentions
    std::string expandedPrompt = resolveMentions(line);

    displayThinking("Analyzing prompt and retrieving repository context...\n"
                    "Injecting custom rules and relevant skills into context window.");

    // If it looks like a task/action, ask if user wants /goal or direct answer
    std::cout << "\n" << ANSI_CYAN << ANSI_BOLD << "ArenaFight Agent:" << ANSI_RESET << "\n"
              << "I received your request: \"" << line << "\"\n\n"
              << "How would you like to proceed?\n"
              << "  - Type " << ANSI_YELLOW << "/goal " << line << ANSI_RESET << " to execute this task autonomously to completion.\n"
              << "  - Type " << ANSI_YELLOW << "/plan " << line << ANSI_RESET << " to generate an interactive plan artifact first.\n"
              << "  - Type " << ANSI_YELLOW << "/grill-me " << line << ANSI_RESET << " to clarify requirements before implementation.\n\n"
              << ANSI_DIM << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte" << ANSI_RESET << "\n\n";
}

void ReplEngine::run() {
    printReplBanner();

    while (running_) {
        std::cout << ANSI_GREEN << ANSI_BOLD << "arena > " << ANSI_RESET << std::flush;
        std::string input;
        if (!std::getline(std::cin, input)) {
            break;
        }
        processInput(input);
    }

    std::cout << "\nExiting ArenaFight Antigravity REPL. Goodbye!\n";
}

} // namespace arenafight
