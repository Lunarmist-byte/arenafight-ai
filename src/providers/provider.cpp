#include "arenafight/providers/provider.hpp"
#include <regex>
#include <iostream>

namespace arenafight {

std::vector<ToolCall> ModelProvider::extractToolCallsFromText(const std::string& text) {
    std::vector<ToolCall> toolCalls;
    if (text.empty()) return toolCalls;

    // Pattern 1: ```json { "tool": "name", "arguments": { ... } } ```
    // or ```json { "name": "name", "arguments": { ... } } ```
    static const std::regex codeBlockRegex("```(?:json)?\\s*\\n?(\\{[\\s\\S]*?\\})\\s*```");
    auto words_begin = std::sregex_iterator(text.begin(), text.end(), codeBlockRegex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string jsonStr = match[1].str();
        try {
            nlohmann::json parsed = nlohmann::json::parse(jsonStr);
            if (parsed.is_object()) {
                std::string toolName;
                if (parsed.contains("tool") && parsed["tool"].is_string()) {
                    toolName = parsed["tool"].get<std::string>();
                } else if (parsed.contains("name") && parsed["name"].is_string()) {
                    toolName = parsed["name"].get<std::string>();
                } else if (parsed.contains("function") && parsed["function"].is_string()) {
                    toolName = parsed["function"].get<std::string>();
                }

                if (!toolName.empty()) {
                    ToolCall tc;
                    tc.id = "call_" + std::to_string(toolCalls.size() + 1);
                    tc.name = toolName;
                    if (parsed.contains("arguments")) {
                        tc.arguments = parsed["arguments"];
                    } else if (parsed.contains("parameters")) {
                        tc.arguments = parsed["parameters"];
                    } else {
                        // All other keys become arguments
                        nlohmann::json args = parsed;
                        args.erase("tool");
                        args.erase("name");
                        args.erase("function");
                        tc.arguments = args;
                    }
                    toolCalls.push_back(tc);
                }
            }
        } catch (...) {
            // Not a valid tool call json
        }
    }

    // Pattern 2: Single raw JSON object without backticks if entire text is JSON
    if (toolCalls.empty()) {
        try {
            nlohmann::json parsed = nlohmann::json::parse(text);
            if (parsed.is_object()) {
                std::string toolName;
                if (parsed.contains("tool") && parsed["tool"].is_string()) {
                    toolName = parsed["tool"].get<std::string>();
                } else if (parsed.contains("name") && parsed["name"].is_string()) {
                    toolName = parsed["name"].get<std::string>();
                }
                if (!toolName.empty()) {
                    ToolCall tc;
                    tc.id = "call_1";
                    tc.name = toolName;
                    tc.arguments = parsed.value("arguments", parsed.value("parameters", nlohmann::json::object()));
                    toolCalls.push_back(tc);
                }
            }
        } catch (...) {}
    }

    return toolCalls;
}

} // namespace arenafight
