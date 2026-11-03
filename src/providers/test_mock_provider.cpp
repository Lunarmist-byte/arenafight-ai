#include "arenafight/providers/test_mock_provider.hpp"
#include <iostream>

namespace arenafight {

TestMockProvider::TestMockProvider() {}

std::vector<ModelInfo> TestMockProvider::discoverModels() {
    return {
        {"mock-reasoner", "Mock Reasoning Model", "TestMock", 32000, true, "Local mock model for planning and reasoning", 2.0f, {0.85f, 0.98f, 0.97f, 0.88f, 0.80f, 0.90f, 0.90f}},
        {"mock-coder", "Mock Coding Model", "TestMock", 32000, true, "Local mock model for tool execution and coding", 2.2f, {0.98f, 0.86f, 0.84f, 0.88f, 0.94f, 0.92f, 0.88f}},
        {"mock-verifier", "Mock Verifier Model", "TestMock", 32000, true, "Local mock model for independent verification", 2.5f, {0.88f, 0.92f, 0.89f, 0.99f, 0.85f, 0.90f, 0.92f}}
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

    // 0. Fast Thinking Phase Duel (Code generation disallowed)
    if (prompt.find("FAST THINKING PHASE") != std::string::npos ||
        prompt.find("CODE GENERATION DISALLOWED") != std::string::npos) {
        response.promptTokens = 110;
        response.completionTokens = 145;
        if (request.model.find("reasoner") != std::string::npos) {
            response.thinking =
                "<think>\n"
                "1. Edge Cases: Anticipate null pointers, buffer overflow, empty collection, and thread race conditions.\n"
                "2. Invariants: Strict RAII deterministic memory management, zero-heap safety bounds, and atomic state transitions.\n"
                "3. Efficiency: O(N log N) divide-and-conquer strategy with cache locality optimizations.\n"
                "4. Discipline: Code generation disallowed; architecture decomposed into modular stages.\n"
                "</think>\n"
                "Strategic thinking evaluation complete: Verified edge cases, concurrency invariants, and complexity limits.";
        } else {
            response.thinking =
                "<think>\n"
                "1. General Strategy: Step-by-step traversal.\n"
                "2. Edge Cases: Standard empty check.\n"
                "3. Complexity: Linear time scan.\n"
                "</think>\n"
                "Basic algorithmic reasoning formulated.";
        }
        response.content = response.thinking;
        return response;
    }

    // 0.1 Benchmark Suite Tasks
    if (prompt.find("BENCHMARK TASK 1") != std::string::npos) {
        response.promptTokens = 120;
        response.completionTokens = 110;
        if (request.model.find("reasoner") != std::string::npos) {
            response.content =
                "Longest Valid Parentheses Complexity Analysis:\n"
                "Optimal Algorithm: Two-pass linear scan achieving O(N) time and O(1) space.\n"
                "1. Forward Pass: Traverse left-to-right maintaining left and right parentheses counters.\n"
                "   When right > left, reset counters. When left == right, update max length.\n"
                "2. Backward Pass: Traverse right-to-left to handle unclosed left parentheses.\n"
                "Edge Cases and Boundary Checks: Empty string, entirely open parentheses, nested expressions.\n"
                "Result: Guaranteed O(N) time and strictly O(1) auxiliary space without a stack.";
        } else {
            response.content = "Two pass traversal left and right counters scan in O(N) time and O(1) auxiliary space with boundary checks.";
        }
        return response;
    }

    if (prompt.find("BENCHMARK TASK 2") != std::string::npos) {
        response.promptTokens = 130;
        response.completionTokens = 150;
        if (request.model.find("coder") != std::string::npos) {
            response.content =
                "template <typename T, size_t Capacity>\n"
                "class alignas(64) LockFreeRingBuffer {\n"
                "    static_assert((Capacity & (Capacity - 1)) == 0, \"Capacity must be a power of two\");\n"
                "    alignas(64) std::atomic<size_t> head_{0};\n"
                "    alignas(64) std::atomic<size_t> tail_{0};\n"
                "    T buffer_[Capacity];\n"
                "public:\n"
                "    constexpr LockFreeRingBuffer() noexcept = default;\n"
                "    bool push(const T& item) noexcept {\n"
                "        size_t curTail = tail_.load(std::memory_order_relaxed);\n"
                "        if (curTail - head_.load(std::memory_order_acquire) >= Capacity) return false;\n"
                "        buffer_[curTail & (Capacity - 1)] = item;\n"
                "        tail_.store(curTail + 1, std::memory_order_release);\n"
                "        return true;\n"
                "    }\n"
                "};";
        } else {
            response.content = "class alignas(64) LockFreeRingBuffer with atomic head and tail using std::memory_order noexcept template.";
        }
        return response;
    }

    if (prompt.find("BENCHMARK TASK 3") != std::string::npos) {
        response.promptTokens = 140;
        response.completionTokens = 135;
        if (request.model.find("verifier") != std::string::npos) {
            response.content =
                "Adversarial Security Audit Report:\n"
                "1. Critical Buffer Overflow: strcpy(temp, buf) lacks boundary checks and overflows temp[128].\n"
                "2. Null Pointer Dereference: int* p = nullptr; followed by *p = 42 triggers immediate segfault.\n"
                "3. Double Free Vulnerability: Calling free(buf) twice corrupts the heap and enables use-after-free exploits.\n"
                "4. Compiler Undefined Behavior: Unchecked operations result in undefined behavior (UB).";
        } else {
            response.content = "Detected buffer overflow via strcpy, null pointer dereference, double free, and compiler undefined behavior (ub).";
        }
        return response;
    }

    if (prompt.find("BENCHMARK TASK 4") != std::string::npos) {
        response.promptTokens = 90;
        response.completionTokens = 85;
        response.content =
            "Real-Time Embedded QoS Checklist:\n"
            "1. Zero Dynamic Heap: Strictly forbid dynamic heap allocations in deterministic execution paths.\n"
            "2. Non-blocking Interrupt Handlers: Keep ISRs minimal with deterministic execution bounds.\n"
            "3. Hardware Timer Watchdog: Guard control loops with hardware watchdog refresh.";
        return response;
    }

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
            response.content = "Implementation modules written and validated.";
            return response;
        }

        ToolCall tc;
        tc.id = "call_write_1";
        tc.name = "write_file";
        tc.arguments = {
            {"path", "mock_output.txt"},
            {"content", "ArenaFight Agent test output generated successfully.\nMade by Lunarmist-byte.\nGitHub: https://github.com/Lunarmist-byte\nLinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/"}
        };
        response.toolCalls.push_back(tc);

        // Check if objective requests a web app / HTML / color randomizer
        std::string lowerPrompt = prompt;
        for (char& c : lowerPrompt) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (lowerPrompt.find("web") != std::string::npos ||
            lowerPrompt.find("html") != std::string::npos ||
            lowerPrompt.find("color") != std::string::npos ||
            lowerPrompt.find("button") != std::string::npos) {
            ToolCall tc2;
            tc2.id = "call_write_2";
            tc2.name = "write_file";
            tc2.arguments = {
                {"path", "mock_app.html"},
                {"content",
                 "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
                 "    <meta charset=\"UTF-8\">\n"
                 "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                 "    <title>AuraColor - Dynamic Randomizer</title>\n"
                 "    <style>\n"
                 "        body { font-family: sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; margin: 0; transition: background 0.5s ease; }\n"
                 "        .card { background: rgba(255,255,255,0.2); backdrop-filter: blur(10px); padding: 2rem; border-radius: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); text-align: center; }\n"
                 "        button { padding: 0.8rem 1.6rem; font-size: 1.1rem; border-radius: 12px; border: none; cursor: pointer; margin: 0.5rem; font-weight: bold; }\n"
                 "        .credit { margin-top: 1.5rem; font-size: 0.9rem; }\n"
                 "        a { color: inherit; font-weight: bold; }\n"
                 "    </style>\n"
                 "</head>\n<body>\n"
                 "    <div class=\"card\">\n"
                 "        <h1>Dynamic Color Randomizer</h1>\n"
                 "        <p id=\"colorText\">#6366f1</p>\n"
                 "        <button id=\"btn\" onclick=\"randomColor()\">Randomize Color</button>\n"
                 "        <button id=\"autoBtn\" onclick=\"toggleAuto()\">Make It Automatic</button>\n"
                 "        <div class=\"credit\">Made by Lunarmist-byte | <a href=\"https://github.com/Lunarmist-byte\">GitHub</a> | <a href=\"https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\">LinkedIn</a></div>\n"
                 "    </div>\n"
                 "    <script>\n"
                 "        let autoTimer = null;\n"
                 "        function randomColor() {\n"
                 "            const c = '#' + Math.floor(Math.random()*16777215).toString(16).padStart(6, '0');\n"
                 "            document.body.style.backgroundColor = c;\n"
                 "            document.getElementById('colorText').innerText = c;\n"
                 "        }\n"
                 "        function toggleAuto() {\n"
                 "            if (autoTimer) { clearInterval(autoTimer); autoTimer = null; document.getElementById('autoBtn').innerText = 'Make It Automatic'; }\n"
                 "            else { randomColor(); autoTimer = setInterval(randomColor, 1500); document.getElementById('autoBtn').innerText = 'Stop Auto'; }\n"
                 "        }\n"
                 "        randomColor();\n"
                 "    </script>\n"
                 "</body>\n</html>\n"
                }
            };
            response.toolCalls.push_back(tc2);
            response.content = "Writing module output file mock_output.txt and web application mock_app.html.";
        } else {
            response.content = "Writing module output file mock_output.txt.";
        }
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
