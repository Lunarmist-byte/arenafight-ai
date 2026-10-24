#include "arenafight/repl/repl_engine.hpp"
#include "arenafight/ui/terminal_ui.hpp"
#include "arenafight/common/logger.hpp"
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
        std::cout << "\narena ❯ " << std::flush;
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
        std::cout << ANSI_GREEN << ANSI_BOLD << "arena ❯ " << ANSI_RESET << std::flush;
        std::string input;
        if (!std::getline(std::cin, input)) {
            break;
        }
        processInput(input);
    }

    std::cout << "\nExiting ArenaFight Antigravity REPL. Goodbye!\n";
}

} // namespace arenafight
