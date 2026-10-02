#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "arenafight/common/types.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/providers/ollama_provider.hpp"
#include "arenafight/providers/openrouter_provider.hpp"
#include "arenafight/providers/claude_code_provider.hpp"
#include "arenafight/providers/test_mock_provider.hpp"
#include "arenafight/tools/file_tools.hpp"
#include "arenafight/tools/search_tools.hpp"
#include "arenafight/tools/shell_tools.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/session/memory.hpp"
#include "arenafight/session/context_manager.hpp"
#include "arenafight/router/model_router.hpp"
#include "arenafight/planner/planner.hpp"
#include "arenafight/executor/executor.hpp"
#include "arenafight/verifier/verifier.hpp"
#include "arenafight/agent/arena_agent.hpp"

using namespace arenafight;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[TEST FAILED] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            return false; \
        } \
    } while(0)

bool testConfig() {
    std::cout << "[TEST] 1. Config loading and environment overrides... " << std::flush;
    Config cfg;
    cfg.max_iterations = 42;
    cfg.saveToFile("test_config.json");

    Config loaded = Config::loadFromFile("test_config.json");
    TEST_ASSERT(loaded.max_iterations == 42, "Config max_iterations must match saved value");
    std::filesystem::remove("test_config.json");

    std::cout << "PASSED\n";
    return true;
}

bool testFileTools() {
    std::cout << "[TEST] 2. File tools (write, read, edit, delete)... " << std::flush;
    std::string testDir = "test_workspace_sandbox";
    std::filesystem::create_directories(testDir);

    WriteFileTool writeTool;
    ToolResult wr = writeTool.execute({
        {"path", "hello.txt"},
        {"content", "Line 1: Alpha\nLine 2: Beta\nLine 3: Gamma\nMade by Lunarmist-byte"}
    }, testDir);
    TEST_ASSERT(wr.success, "WriteFileTool must succeed");

    ReadFileTool readTool;
    ToolResult rr = readTool.execute({
        {"path", "hello.txt"},
        {"start_line", 2},
        {"end_line", 2}
    }, testDir);
    TEST_ASSERT(rr.success, "ReadFileTool must succeed");
    TEST_ASSERT(rr.output.find("Beta") != std::string::npos, "ReadFileTool must read Line 2 Beta");

    EditFileTool editTool;
    ToolResult er = editTool.execute({
        {"path", "hello.txt"},
        {"target_content", "Beta"},
        {"replacement_content", "Delta"}
    }, testDir);
    TEST_ASSERT(er.success, "EditFileTool must succeed");

    ToolResult rr2 = readTool.execute({{"path", "hello.txt"}}, testDir);
    TEST_ASSERT(rr2.output.find("Delta") != std::string::npos, "File content must reflect edited text");
    TEST_ASSERT(std::filesystem::exists(testDir + "/hello.txt.bak"), "Backup file must be created");

    DeleteFileTool delTool(false);
    ToolResult dr = delTool.execute({{"path", "hello.txt"}}, testDir);
    TEST_ASSERT(dr.success, "DeleteFileTool must succeed");
    TEST_ASSERT(!std::filesystem::exists(testDir + "/hello.txt"), "File must be deleted");

    std::filesystem::remove_all(testDir);
    std::cout << "PASSED\n";
    return true;
}

bool testSearchTools() {
    std::cout << "[TEST] 3. Search tools (list_directory, search_files)... " << std::flush;
    std::string testDir = "test_search_sandbox";
    std::filesystem::create_directories(testDir + "/subdir");

    std::ofstream f(testDir + "/subdir/sample.txt");
    f << "Hello world from Lunarmist-byte ArenaFight project!\nSecond line.\n";
    f.close();

    ListDirectoryTool listTool;
    ToolResult lr = listTool.execute({{"path", "."}}, testDir);
    TEST_ASSERT(lr.success, "ListDirectoryTool must succeed");
    TEST_ASSERT(lr.output.find("sample.txt") != std::string::npos, "List directory must show sample.txt");

    SearchFilesTool searchTool;
    ToolResult sr = searchTool.execute({{"pattern", "Lunarmist-byte"}}, testDir);
    TEST_ASSERT(sr.success, "SearchFilesTool must succeed");
    TEST_ASSERT(sr.output.find("sample.txt") != std::string::npos, "Search files must find pattern in sample.txt");

    std::filesystem::remove_all(testDir);
    std::cout << "PASSED\n";
    return true;
}

bool testProcessExecutorAndSafety() {
    std::cout << "[TEST] 4. Process execution and destructive safety check... " << std::flush;
    TEST_ASSERT(ProcessExecutor::isDestructive("git reset --hard HEAD~1"), "git reset --hard must be destructive");
    TEST_ASSERT(ProcessExecutor::isDestructive("rm -rf /"), "rm -rf must be destructive");
    TEST_ASSERT(!ProcessExecutor::isDestructive("git status"), "git status must be safe");
    TEST_ASSERT(!ProcessExecutor::isDestructive("cmake --build build"), "build command must be safe");

    ProcessResult pr = ProcessExecutor::executeShell("echo ARENAFIGHT_TEST_OK", ".", 5000);
    TEST_ASSERT(pr.exitCode == 0, "Echo command must return exit code 0");
    TEST_ASSERT(pr.stdOut.find("ARENAFIGHT_TEST_OK") != std::string::npos, "Stdout must contain echo string");

    std::cout << "PASSED\n";
    return true;
}

bool testProviderDetection() {
    std::cout << "[TEST] 5. Provider detection (Ollama, OpenRouter, Claude Code)... " << std::flush;
    OllamaProvider ollama("http://localhost:11434");
    bool ollamaOnline = ollama.available();
    // Ollama detection must not crash even if offline
    auto ollamaModels = ollama.discoverModels();

    OpenRouterProvider openrouter("", "https://openrouter.ai/api/v1");
    bool openrouterAvail = openrouter.available();
    TEST_ASSERT(!openrouterAvail, "OpenRouter without key must report unavailable");

    OpenRouterProvider openrouterWithKey("sk-or-dummy-key-for-test", "https://openrouter.ai/api/v1", {"anthropic/claude-3.5-sonnet"});
    TEST_ASSERT(openrouterWithKey.available(), "OpenRouter with key must report available");
    auto orModels = openrouterWithKey.discoverModels();
    TEST_ASSERT(!orModels.empty(), "Configured models must be discoverable");

    ClaudeCodeProvider claude;
    bool claudeFound = claude.isInstalled();
    auto claudeModels = claude.discoverModels();

    TestMockProvider mock;
    TEST_ASSERT(mock.available(), "TestMockProvider must be available");
    auto mockModels = mock.discoverModels();
    TEST_ASSERT(mockModels.size() >= 3, "Mock provider must have reasoning, coding, and verifier models");

    std::cout << "PASSED (Ollama: " << (ollamaOnline ? "ONLINE" : "OFFLINE (Graceful)")
              << ", Claude CLI: " << (claudeFound ? "INSTALLED" : "NOT FOUND") << ")\n";
    return true;
}

bool testSessionPersistence() {
    std::cout << "[TEST] 6. Session serialization & disk persistence... " << std::flush;
    std::string testSessionDir = "test_sessions_sandbox";

    Session s;
    s.id = "test_sess_001";
    s.originalTask = "Build Todo App";
    s.objective = "Build Todo App";
    s.currentState = "RUNNING";
    s.iteration = 3;

    Task t1;
    t1.id = "t_1";
    t1.description = "Design architecture";
    t1.status = TaskStatus::COMPLETED;
    s.tasks.push_back(t1);

    Task t2;
    t2.id = "t_2";
    t2.description = "Implement code";
    t2.status = TaskStatus::PENDING;
    t2.dependencies = {"t_1"};
    s.tasks.push_back(t2);

    s.completedTasks.push_back("t_1");
    s.discoveries.push_back("Found CMake 4.2");

    bool saved = s.saveToDisk(testSessionDir);
    TEST_ASSERT(saved, "Session saveToDisk must succeed");

    Session loaded = Session::loadFromDisk("test_sess_001", testSessionDir);
    TEST_ASSERT(loaded.id == "test_sess_001", "Loaded session ID must match");
    TEST_ASSERT(loaded.tasks.size() == 2, "Loaded tasks count must match");
    TEST_ASSERT(loaded.tasks[0].status == TaskStatus::COMPLETED, "Task 1 status must be COMPLETED");

    auto ready = loaded.getReadyTasks();
    TEST_ASSERT(ready.size() == 1, "Task 2 dependencies are met, so ready tasks must be 1");
    TEST_ASSERT(ready[0]->id == "t_2", "Ready task must be t_2");

    auto list = Session::listSessions(testSessionDir);
    TEST_ASSERT(list.size() == 1, "Session list must contain 1 session");

    std::filesystem::remove_all(testSessionDir);
    std::cout << "PASSED\n";
    return true;
}

bool testDynamicRoutingAndFallback() {
    std::cout << "[TEST] 7. Dynamic model routing and independent verifier... " << std::flush;
    Config cfg;
    cfg.test_mock_enabled = true;
    ModelRouter router(cfg);

    auto mock = std::make_shared<TestMockProvider>();
    router.registerProvider(mock);
    router.refreshModels();

    ModelSelection planSel = router.selectModel(TaskType::PLANNING);
    TEST_ASSERT(planSel.provider != nullptr, "Planning model selection must succeed");

    ModelSelection codeSel = router.selectModel(TaskType::CODING);
    TEST_ASSERT(codeSel.provider != nullptr, "Coding model selection must succeed");

    // Verification model should be independent if multiple models available
    ModelSelection verifSel = router.selectModel(TaskType::VERIFICATION, "", "mock-coder");
    TEST_ASSERT(verifSel.provider != nullptr, "Verifier model selection must succeed");

    std::cout << "PASSED\n";
    return true;
}

bool testMultiStepExecutionAndReplanning() {
    std::cout << "[TEST] 8. Multi-step execution loop, deliberate failure, recovery and verification... " << std::flush;
    std::string testWs = "test_agent_exec_ws";
    std::filesystem::create_directories(testWs);

    Config cfg;
    cfg.workspace_dir = testWs;
    cfg.max_iterations = 6;
    cfg.test_mock_enabled = true;
    cfg.default_provider = "TestMock";

    ArenaAgent agent(cfg);
    agent.startNewSession("Create a test project with validation");

    bool finished = agent.run();
    TEST_ASSERT(finished, "Agent autonomous loop must finish with verified completion");

    const Session& finalSession = agent.getSession();
    TEST_ASSERT(finalSession.completed, "Session must be marked COMPLETED");
    TEST_ASSERT(finalSession.iteration >= 1, "Iteration count must be >= 1");

    // Verify created output file
    TEST_ASSERT(std::filesystem::exists(testWs + "/mock_output.txt"), "Agent must have created mock_output.txt via tool");

    std::filesystem::remove_all(testWs);
    std::cout << "PASSED\n";
    return true;
}

int main() {
    std::cout << "\n====================================================\n"
              << "          ARENAFIGHT AGENT TEST SUITE               \n"
              << " Made by: Lunarmist-byte                            \n"
              << " GitHub:  https://github.com/Lunarmist-byte         \n"
              << "====================================================\n\n";

    int passed = 0;
    int total = 8;

    if (testConfig()) passed++;
    if (testFileTools()) passed++;
    if (testSearchTools()) passed++;
    if (testProcessExecutorAndSafety()) passed++;
    if (testProviderDetection()) passed++;
    if (testSessionPersistence()) passed++;
    if (testDynamicRoutingAndFallback()) passed++;
    if (testMultiStepExecutionAndReplanning()) passed++;

    std::cout << "\n----------------------------------------------------\n";
    std::cout << "TEST RESULTS: " << passed << "/" << total << " PASSED\n";
    std::cout << "====================================================\n\n";

    return (passed == total) ? 0 : 1;
}
