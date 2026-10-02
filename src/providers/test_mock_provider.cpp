#include "arenafight/providers/test_mock_provider.hpp"
#include <iostream>

namespace arenafight {

TestMockProvider::TestMockProvider() {}

std::vector<ModelInfo> TestMockProvider::discoverModels() {
    return {
        {"mock-reasoner", "Mock Reasoning Model", "TestMock", 32000, true, "Local mock model for planning and reasoning", 2.0f},
        {"mock-coder", "Mock Coding Model", "TestMock", 32000, true, "Local mock model for tool execution and coding", 2.2f},
        {"mock-verifier", "Mock Verifier Model", "TestMock", 32000, true, "Local mock model for independent verification", 2.5f}
    };
}

AgentResponse TestMockProvider::generate(const AgentRequest& request) {
    AgentResponse response;
    response.success = true;
    response.promptTokens = 150;
    response.completionTokens = 80;

    std::string prompt = request.userPrompt;
    for (const auto& msg : request.messages) {
        prompt += "\n" + msg.content;
    }

    step_++;

    // 1. Planning Request
    if (prompt.find("create a structured execution plan") != std::string::npos ||
        prompt.find("Create an execution plan") != std::string::npos ||
        (request.model.find("reasoner") != std::string::npos && prompt.find("TASK OBJECTIVE:") != std::string::npos)) {
        response.content =
            "Execution Plan Created:\n"
            "```json\n"
            "[\n"
            "  {\"id\": \"task_1\", \"description\": \"Inspect workspace structure\", \"dependencies\": []},\n"
            "  {\"id\": \"task_2\", \"description\": \"Implement core module\", \"dependencies\": [\"task_1\"]},\n"
            "  {\"id\": \"task_3\", \"description\": \"Verify implementation and run tests\", \"dependencies\": [\"task_2\"]}\n"
            "]\n"
            "```";
        return response;
    }

    // 2. Verification Request
    if (prompt.find("independent Verifier") != std::string::npos ||
        prompt.find("VERIFICATION EVIDENCE:") != std::string::npos) {
        response.content =
            "```json\n"
            "{\n"
            "  \"complete\": true,\n"
            "  \"summary\": \"All files present, build passed, tests passed\",\n"
            "  \"passedChecks\": [\"Build passed\", \"Tests passed\", \"Files present\"],\n"
            "  \"failedChecks\": []\n"
            "}\n"
            "```";
        return response;
    }

    // 3. Deliberate failure test
    if (deliberateFailure_ && prompt.find("ACTIVE TASK: [task_1]") != std::string::npos &&
        prompt.find("deliberately_non_existent_command_xyz123") == std::string::npos) {
        ToolCall tc;
        tc.id = "call_fail_1";
        tc.name = "run_command";
        tc.arguments = {
            {"command", "deliberately_non_existent_command_xyz123"}
        };
        response.toolCalls.push_back(tc);
        response.content = "Executing verification probe.";
        return response;
    }

    // 4. Active Task: task_1 (Inspect)
    if (prompt.find("ACTIVE TASK: [task_1]") != std::string::npos) {
        if (prompt.find("Tool: list_directory") != std::string::npos ||
            prompt.find("Directory contents for:") != std::string::npos) {
            response.content = "Workspace inspected. Structure is verified.";
            return response;
        }

        ToolCall tc;
        tc.id = "call_list_1";
        tc.name = "list_directory";
        tc.arguments = {{"path", "."}, {"max_depth", 1}};
        response.toolCalls.push_back(tc);
        response.content = "Inspecting workspace directory.";
        return response;
    }

    // 5. Active Task: task_2 (Implement)
    if (prompt.find("ACTIVE TASK: [task_2]") != std::string::npos) {
        if (prompt.find("Tool: write_file") != std::string::npos ||
            prompt.find("File successfully written") != std::string::npos) {
            response.content = "Module mock_output.txt written and validated.";
            return response;
        }

        ToolCall tc;
        tc.id = "call_write_1";
        tc.name = "write_file";
        tc.arguments = {
            {"path", "mock_output.txt"},
            {"content", "ArenaFight Agent test output generated successfully.\nMade by Lunarmist-byte.\nGitHub: https://github.com/Lunarmist-byte"}
        };
        response.toolCalls.push_back(tc);
        response.content = "Writing module output file mock_output.txt.";
        return response;
    }

    // 6. Active Task: task_3 (Verify / Review)
    if (prompt.find("ACTIVE TASK: [task_3]") != std::string::npos) {
        response.content = "Verification of implementation completed successfully.";
        return response;
    }

    // Default fallback
    response.content = "Task step executed successfully with all requirements satisfied.";
    return response;
}

} // namespace arenafight
