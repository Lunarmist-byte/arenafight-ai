#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "arenafight/common/types.hpp"
#include "arenafight/common/config.hpp"

namespace arenafight {

class Tool {
public:
    virtual ~Tool() = default;

    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual nlohmann::json parameters() const = 0;

    virtual ToolResult execute(
        const nlohmann::json& arguments,
        const std::string& workingDir = "."
    ) = 0;

    ToolDefinition getDefinition() const {
        ToolDefinition def;
        def.name = name();
        def.description = description();
        def.parameters = parameters();
        return def;
    }
};

class ToolRegistry {
public:
    ToolRegistry() = default;

    void registerTool(std::shared_ptr<Tool> tool);
    std::shared_ptr<Tool> getTool(const std::string& name) const;
    std::vector<ToolDefinition> getDefinitions() const;

    ToolResult execute(
        const std::string& toolName,
        const nlohmann::json& arguments,
        const std::string& workingDir = "."
    );

    // Initialize all standard tools with config
    static ToolRegistry createStandardRegistry(const Config& config);

private:
    std::map<std::string, std::shared_ptr<Tool>> tools_;
};

} // namespace arenafight
