#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include "arenafight/agent/arena_agent.hpp"
#include "arenafight/artifacts/artifact.hpp"
#include "arenafight/customizations/customization_manager.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

struct SlashCommand {
    std::string name;
    std::string description;
    std::string usage;
    std::function<void(const std::string& args)> handler;
};

class ReplEngine {
public:
    explicit ReplEngine(ArenaAgent& agent);

    // Run the main interactive REPL loop
    void run();

    // Process a single line of input (command or chat prompt)
    void processInput(const std::string& rawInput);

    // Slash command handlers
    void handleGoal(const std::string& task);
    void handlePlan(const std::string& task);
    void handleGrillMe(const std::string& topic);
    void handleLearn(const std::string& insight);
    void handleSchedule(const std::string& args);
    void handleArtifacts(const std::string& args);
    void handleSkills(const std::string& args);
    void handleRules(const std::string& args);
    void handleModels(const std::string& args);
    void handleStatus(const std::string& args);
    void handleHelp(const std::string& args);

    // Mentions resolver (@file, @git, @memory, @claims)
    std::string resolveMentions(const std::string& prompt);

    ArtifactManager& getArtifactManager() { return artifacts_; }
    CustomizationManager& getCustomizationManager() { return customizations_; }

private:
    ArenaAgent& agent_;
    ArtifactManager artifacts_;
    CustomizationManager customizations_;
    std::map<std::string, SlashCommand> slashCommands_;
    bool running_ = true;

    void registerCommands();
    void printReplBanner();
    void printHelpTable();
    std::string askUserInteractive(const std::string& prompt);
    void displayThinking(const std::string& thoughts);
};

} // namespace arenafight
