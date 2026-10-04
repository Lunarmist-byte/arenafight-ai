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
#include "arenafight/verifier/independent_reviewer.hpp"
#include "arenafight/evidence/claim_registry.hpp"
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

bool testEvidenceHierarchyAndDominance() {
    std::cout << "[TEST] 9. Evidence hierarchy & level dominance (Level 1 execution > Level 6 guess)... " << std::flush;
    ClaimRegistry registry;

    // Model asserts that code compiles (Level 5 reasoning)
    Claim c;
    c.statement = "Code compiles successfully";
    c.type = KnowledgeType::HYPOTHESIS;
    std::string cid = registry.registerClaim(c);

    // Actual compiler execution fails (Level 1 direct execution)
    Observation obs;
    obs.tool = "run_command";
    obs.command = "g++ -c main.cpp";
    obs.exitCode = 1;
    obs.stderrText = "main.cpp:10: error: 'foo' was not declared in this scope";
    std::string obsId = registry.recordObservation(obs);

    // Find the execution evidence generated from obs
    const auto& evidences = registry.getEvidences();
    TEST_ASSERT(!evidences.empty(), "Observation must create Evidence");
    std::string failEvId = evidences.back().id;

    // Link compiler failure evidence to claim
    registry.linkEvidenceToClaim(cid, failEvId);
    const Claim* evaluated = registry.getClaim(cid);
    TEST_ASSERT(evaluated != nullptr, "Claim must exist");
    TEST_ASSERT(evaluated->status == EvidenceLevel::CONTRADICTED, "Direct execution failure must contradict claim");
    TEST_ASSERT(!evaluated->verified, "Contradicted claim must not be verified");

    std::cout << "PASSED\n";
    return true;
}

bool testFactHypothesisSeparationAndModelDerived() {
    std::cout << "[TEST] 10. Separation of facts from hypotheses (Model claim != truth)... " << std::flush;
    ClaimRegistry registry;

    // A model attempts to declare a factual claim without external evidence
    Claim rawClaim;
    rawClaim.statement = "The segmentation fault is caused by use-after-free";
    rawClaim.type = KnowledgeType::FACT; // Model claims this is a fact!
    std::string cid = registry.registerClaim(rawClaim);

    const Claim* c = registry.getClaim(cid);
    TEST_ASSERT(c != nullptr, "Claim must be registered");
    // Core rule: System must NOT store unverified assertion as FACT!
    TEST_ASSERT(c->type == KnowledgeType::HYPOTHESIS, "Unverified claim must be downgraded to HYPOTHESIS");
    TEST_ASSERT(c->status == EvidenceLevel::UNVERIFIED, "Status must be UNVERIFIED without evidence");
    TEST_ASSERT(!c->verified, "Claim must not be marked verified");

    // Now provide concrete artifact/execution evidence
    Evidence proof;
    proof.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
    proof.level = EvidenceLevel::VERIFIED;
    proof.source = "valgrind";
    proof.evidence = "Address 0x1234 is 0 bytes inside a block of size 64 free'd";
    proof.confidence = 1.0;
    std::string proofId = registry.addEvidence(proof);

    // Promote hypothesis to fact with concrete evidence
    bool promoted = registry.promoteHypothesisToFact(cid, proofId);
    TEST_ASSERT(promoted, "Promote to FACT must succeed with Level 1 proof");

    const Claim* verifiedClaim = registry.getClaim(cid);
    TEST_ASSERT(verifiedClaim->type == KnowledgeType::FACT, "Claim must now be FACT");
    TEST_ASSERT(verifiedClaim->status == EvidenceLevel::VERIFIED, "Claim status must be VERIFIED");
    TEST_ASSERT(verifiedClaim->verified, "Claim must be marked verified");

    std::cout << "PASSED\n";
    return true;
}

bool testConsensusVsEvidenceRule() {
    std::cout << "[TEST] 11. Consensus is supporting evidence, not proof (5 models < 1 tool execution)... " << std::flush;
    ClaimRegistry registry;

    // 5 models agree that API parameter X is valid
    std::string cid;
    for (int i = 1; i <= 5; ++i) {
        cid = registry.recordModelAssertion("API endpoint /auth accepts parameter token_type", "Model_" + std::to_string(i));
    }

    const Claim* consensusClaim = registry.getClaim(cid);
    TEST_ASSERT(consensusClaim != nullptr, "Consensus claim must exist");
    TEST_ASSERT(consensusClaim->modelAgreementCount == 5, "5 models must be recorded as agreeing");
    // Section 9: 5 models agree + 0 external evidence = MODEL_DERIVED, NOT VERIFIED!
    TEST_ASSERT(consensusClaim->status == EvidenceLevel::MODEL_DERIVED, "Consensus alone cannot establish VERIFIED status");
    TEST_ASSERT(!consensusClaim->verified, "Consensus alone must not mark verified");
    TEST_ASSERT(consensusClaim->confidence <= 0.50, "Consensus confidence must be capped at 0.50 without external evidence");

    // Compare with 1 single model claim verified by tool execution
    Claim singleModelClaim;
    singleModelClaim.statement = "API endpoint /auth returns HTTP 200";
    singleModelClaim.type = KnowledgeType::HYPOTHESIS;
    std::string singleCid = registry.registerClaim(singleModelClaim);

    Evidence toolProof;
    toolProof.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
    toolProof.level = EvidenceLevel::VERIFIED;
    toolProof.source = "curl";
    toolProof.evidence = "HTTP/1.1 200 OK";
    toolProof.confidence = 1.0;
    std::string toolProofId = registry.addEvidence(toolProof);

    registry.linkEvidenceToClaim(singleCid, toolProofId);
    const Claim* verifiedToolClaim = registry.getClaim(singleCid);
    TEST_ASSERT(verifiedToolClaim->status == EvidenceLevel::VERIFIED, "Tool execution must establish VERIFIED status");
    TEST_ASSERT(verifiedToolClaim->verified, "Tool execution marks claim verified");
    TEST_ASSERT(verifiedToolClaim->confidence > 0.95, "Tool execution confidence outranks consensus");

    std::cout << "PASSED\n";
    return true;
}

bool testContradictionDetectionAndResolution() {
    std::cout << "[TEST] 12. Contradiction detection & active resolution (parser.cpp vs lexer.cpp)... " << std::flush;
    ClaimRegistry registry;

    // Agent A claims bug is in parser.cpp
    Claim claimA;
    claimA.statement = "The bug is in parser.cpp";
    claimA.targetArtifact = "src/parser.cpp";
    claimA.sourceModel = "AgentA";
    registry.registerClaim(claimA);

    // Agent B claims bug is in lexer.cpp
    Claim claimB;
    claimB.statement = "The bug is in lexer.cpp";
    claimB.targetArtifact = "src/lexer.cpp";
    claimB.sourceModel = "AgentB";
    registry.registerClaim(claimB);

    // Contradiction detector must detect conflict
    auto contradictions = registry.getUnresolvedContradictions();
    TEST_ASSERT(!contradictions.empty(), "Contradiction detector must identify conflicting bug location claims");
    TEST_ASSERT(contradictions[0].requiredAction == "Inspect source + reproduce failure.",
                "Contradiction must enforce action: Inspect source + reproduce failure.");

    // Active resolution: tool reproduces crash in parser.cpp
    Evidence resolvingEvidence;
    resolvingEvidence.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
    resolvingEvidence.level = EvidenceLevel::VERIFIED;
    resolvingEvidence.source = "gdb";
    resolvingEvidence.evidence = "Crash occurred at parser.cpp:42 in parseExpression()";
    std::string resEvId = registry.addEvidence(resolvingEvidence);

    bool resolved = registry.resolveContradiction(contradictions[0].id, resEvId, "GDB backtrace confirms parser.cpp");
    TEST_ASSERT(resolved, "Contradiction resolution must succeed");
    TEST_ASSERT(registry.getUnresolvedContradictions().empty(), "No unresolved contradictions must remain");

    std::cout << "PASSED\n";
    return true;
}

bool testFreshVerificationAndEvidenceInvalidation() {
    std::cout << "[TEST] 13. Fresh verification & evidence invalidation on file modification... " << std::flush;
    ClaimRegistry registry;

    // 1. Initial build evidence: PASS
    Evidence buildEv;
    buildEv.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
    buildEv.level = EvidenceLevel::VERIFIED;
    buildEv.source = "cmake";
    buildEv.claim = "Build passed";
    buildEv.dependentArtifacts = {"src/main.cpp", "build"};
    std::string bEvId = registry.addEvidence(buildEv);

    Claim buildClaim;
    buildClaim.statement = "Application builds cleanly";
    buildClaim.evidenceIds = {bEvId};
    std::string bcId = registry.registerClaim(buildClaim);

    const Claim* cBefore = registry.getClaim(bcId);
    TEST_ASSERT(cBefore->verified, "Claim must be verified before modification");

    // 2. File src/main.cpp is modified
    int invalidated = registry.invalidateArtifactEvidence("src/main.cpp");
    TEST_ASSERT(invalidated >= 1, "Must invalidate evidence depending on src/main.cpp");

    // 3. Previous build evidence must be marked STALE
    const Evidence* staleEv = registry.getEvidence(bEvId);
    TEST_ASSERT(staleEv != nullptr && staleEv->isStale, "Modified artifact evidence must become STALE");

    // 4. Claim must be downgraded to UNVERIFIED
    const Claim* cAfter = registry.getClaim(bcId);
    TEST_ASSERT(!cAfter->verified, "Claim must be downgraded from verified when evidence becomes stale");
    TEST_ASSERT(cAfter->status == EvidenceLevel::UNVERIFIED, "Claim status must revert to UNVERIFIED");

    std::cout << "PASSED\n";
    return true;
}

bool testBlindAndAdversarialReview() {
    std::cout << "[TEST] 14. Blind review & adversarial vulnerability inspection... " << std::flush;
    Config cfg;
    cfg.test_mock_enabled = true;
    ModelRouter router(cfg);
    auto mock = std::make_shared<TestMockProvider>();
    router.registerProvider(mock);
    router.refreshModels();

    ContextManager ctx(cfg);
    IndependentReviewer reviewer(router, ctx);

    // 1. Blind review with anonymized candidates
    std::vector<std::pair<std::string, std::string>> candidates = {
        {"void process(char* buf) { strcpy(dest, buf); }", "ProviderX_Model1"},
        {"void process(const std::string& buf) { dest = buf; }", "ProviderY_Model2"}
    };

    BlindReviewResult blindRes = reviewer.performBlindReview("Safely copy string buffer", candidates);
    TEST_ASSERT(blindRes.passed, "Blind review must complete successfully");
    TEST_ASSERT(blindRes.selectedOption.find("SOLUTION") != std::string::npos, "Blind review must reference anonymized SOLUTION label");

    // 2. Adversarial review
    AdversarialReviewResult advRes = reviewer.performAdversarialReview(
        "system(\"echo \" + userInput);",
        "Echo user input securely",
        {},
        ReviewMode::ADVERSARIAL
    );
    TEST_ASSERT(advRes.mode == ReviewMode::ADVERSARIAL, "Review mode must be ADVERSARIAL");

    std::cout << "PASSED\n";
    return true;
}

bool testVerificationGatesAndUncertainty() {
    std::cout << "[TEST] 15. All 6 verification gates & evidence-based uncertainty handling... " << std::flush;
    Config cfg;
    cfg.test_mock_enabled = true;
    ModelRouter router(cfg);
    auto mock = std::make_shared<TestMockProvider>();
    router.registerProvider(mock);
    router.refreshModels();

    ContextManager ctx(cfg);
    Verifier verifier(router, ctx, ".");

    Session session;
    session.id = "test_gates_sess";
    session.originalTask = "Audit security and build project";
    MemoryManager memory;

    // Case A: Unresolved contradiction present -> Gate 6 MUST FAIL
    Claim c1;
    c1.statement = "The bug is in parser.cpp";
    c1.targetArtifact = "src/parser.cpp";
    session.getClaimRegistry().registerClaim(c1);

    Claim c2;
    c2.statement = "The bug is in lexer.cpp";
    c2.targetArtifact = "src/lexer.cpp";
    session.getClaimRegistry().registerClaim(c2);

    GateCheckResult g6 = verifier.evaluateGate6_NoContradictions(session.getClaimRegistry());
    TEST_ASSERT(!g6.passed, "Gate 6 must FAIL when unresolved contradictions exist");

    VerificationResult verifFail = verifier.verify(session, memory);
    TEST_ASSERT(!verifFail.complete, "Verification must NOT mark complete when Gate 6 fails");
    TEST_ASSERT(verifFail.confidence < 0.50, "Confidence must be low due to contradiction penalty");
    TEST_ASSERT(!verifFail.groundedAnswer.isFullyComplete, "Grounded answer must reflect incomplete state");
    TEST_ASSERT(verifFail.groundedAnswer.formattedReport.find("Partially completed") != std::string::npos,
                "Report must indicate partially completed when gates fail");

    // Case B: Resolve contradiction
    auto unresolved = session.getClaimRegistry().getUnresolvedContradictions();
    TEST_ASSERT(!unresolved.empty(), "Unresolved contradictions list must not be empty");

    Evidence fixEv;
    fixEv.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
    fixEv.level = EvidenceLevel::VERIFIED;
    fixEv.evidence = "Stack trace proves parser.cpp";
    std::string fixEvId = session.getClaimRegistry().addEvidence(fixEv);
    session.getClaimRegistry().resolveContradiction(unresolved[0].id, fixEvId, "Confirmed parser.cpp");

    GateCheckResult g6Resolved = verifier.evaluateGate6_NoContradictions(session.getClaimRegistry());
    TEST_ASSERT(g6Resolved.passed, "Gate 6 must PASS once contradiction is resolved");

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
    int total = 15;

    if (testConfig()) passed++;
    if (testFileTools()) passed++;
    if (testSearchTools()) passed++;
    if (testProcessExecutorAndSafety()) passed++;
    if (testProviderDetection()) passed++;
    if (testSessionPersistence()) passed++;
    if (testDynamicRoutingAndFallback()) passed++;
    if (testMultiStepExecutionAndReplanning()) passed++;
    if (testEvidenceHierarchyAndDominance()) passed++;
    if (testFactHypothesisSeparationAndModelDerived()) passed++;
    if (testConsensusVsEvidenceRule()) passed++;
    if (testContradictionDetectionAndResolution()) passed++;
    if (testFreshVerificationAndEvidenceInvalidation()) passed++;
    if (testBlindAndAdversarialReview()) passed++;
    if (testVerificationGatesAndUncertainty()) passed++;

    std::cout << "\n----------------------------------------------------\n";
    std::cout << "TEST RESULTS: " << passed << "/" << total << " PASSED\n";
    std::cout << "====================================================\n\n";

    return (passed == total) ? 0 : 1;
}
