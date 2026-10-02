#include "arenafight/tools/user_tools.hpp"
#include <iostream>

namespace arenafight {

nlohmann::json AskUserTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"question", {{"type", "string"}, {"description", "The specific question to ask the user"}}}
        }},
        {"required", {"question"}}
    };
}

ToolResult AskUserTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult res;
    std::string question = arguments.value("question", "");
    if (question.empty()) {
        res.success = false;
        res.error = "Missing 'question' argument";
        return res;
    }

    std::cout << "\n==================== AGENT ASKS ====================\n"
              << question << "\n"
              << "====================================================\n"
              << "Your answer: " << std::flush;

    std::string answer;
    std::getline(std::cin, answer);

    res.success = true;
    res.output = answer;
    return res;
}

} // namespace arenafight
