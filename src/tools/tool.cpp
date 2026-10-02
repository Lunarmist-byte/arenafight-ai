#include "arenafight/tools/tool.hpp"
#include "arenafight/tools/file_tools.hpp"
#include "arenafight/tools/search_tools.hpp"
#include "arenafight/tools/shell_tools.hpp"
#include "arenafight/tools/git_tools.hpp"
#include "arenafight/tools/user_tools.hpp"
#include <iostream>

namespace arenafight {

void ToolRegistry::registerTool(std::shared_ptr<Tool> tool) {
    if (tool) {
        tools_[tool->name()] = tool;
    }
}

std::shared_ptr<Tool> ToolRegistry::getTool(const std::string& name) const {
    auto it = tools_.find(name);
    if (it != tools_.end()) return it->second;
    return nullptr;
}

std::vector<ToolDefinition> ToolRegistry::getDefinitions() const {
    std::vector<ToolDefinition> defs;
    for (const auto& [name, tool] : tools_) {
        defs.push_back(tool->getDefinition());
    }
    return defs;
}

ToolResult ToolRegistry::execute(
    const std::string& toolName,
    const nlohmann::json& arguments,
    const std::string& workingDir
) {
    auto tool = getTool(toolName);
    if (!tool) {
        ToolResult r;
        r.success = false;
        r.error = "Tool not found in registry: " + toolName;
        return r;
    }

    try {
        return tool->execute(arguments, workingDir);
    } catch (const std::exception& e) {
        ToolResult r;
        r.success = false;
        r.error = "Tool exception: " + std::string(e.what());
        return r;
    }
}

ToolRegistry ToolRegistry::createStandardRegistry(const Config& config) {
    ToolRegistry reg;
    bool requireConfirm = config.require_confirmation_for_destructive_commands;

    // File tools
    reg.registerTool(std::make_shared<ReadFileTool>());
    reg.registerTool(std::make_shared<WriteFileTool>());
    reg.registerTool(std::make_shared<EditFileTool>());
    reg.registerTool(std::make_shared<CreateDirectoryTool>());
    reg.registerTool(std::make_shared<DeleteFileTool>(requireConfirm));

    // Search & Inspection
    reg.registerTool(std::make_shared<ListDirectoryTool>());
    reg.registerTool(std::make_shared<SearchFilesTool>());

    // Shell tools
    reg.registerTool(std::make_shared<RunCommandTool>(requireConfirm));
    reg.registerTool(std::make_shared<RunProgramTool>());
    reg.registerTool(std::make_shared<RunTestsTool>());

    // Git tools
    reg.registerTool(std::make_shared<GitStatusTool>());
    reg.registerTool(std::make_shared<GitDiffTool>());
    reg.registerTool(std::make_shared<GitLogTool>());

    // User interaction
    reg.registerTool(std::make_shared<AskUserTool>());

    return reg;
}

} // namespace arenafight
