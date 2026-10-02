#pragma once

#include "arenafight/tools/tool.hpp"

namespace arenafight {

class AskUserTool : public Tool {
public:
    std::string name() const override { return "ask_user"; }
    std::string description() const override {
        return "Ask the human user a clarifying question when genuinely blocked by missing keys, conflicting requirements, or dangerous operations.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override;
};

} // namespace arenafight
