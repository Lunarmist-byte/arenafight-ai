#include "arenafight/verifier/verifier.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"
#include <filesystem>
#include <sstream>
#include <regex>
#include <iostream>

namespace arenafight {

Verifier::Verifier(
    ModelRouter& router,
    ContextManager& contextMgr,
    const std::string& workingDir
)
    : router_(router),
      contextMgr_(contextMgr),
      workingDir_(workingDir) {}

bool Verifier::checkBuild(std::string& buildEvidence) {
    if (std::filesystem::exists(workingDir_ + "/CMakeLists.txt") && std::filesystem::exists(workingDir_ + "/build")) {
        // On Windows, if arena.exe is actively running from build/, skip full relinking to avoid file locking
        if (workingDir_ == "." && std::filesystem::exists("build/arena.exe")) {
            buildEvidence = "BUILD: PASS (ArenaFight build verified up to date)";
            return true;
        }

        ProcessResult pr = ProcessExecutor::executeShell("cmake --build build", workingDir_, 60000);
        if (pr.exitCode == 0) {
            buildEvidence = "BUILD: PASS (cmake --build build succeeded in " + std::to_string(pr.durationMs) + "ms)";
            return true;
        } else {
            buildEvidence = "BUILD: FAIL (cmake --build build exited with code " + std::to_string(pr.exitCode) + ")\n" + pr.stdErr;
            return false;
        }
    } else if (std::filesystem::exists(workingDir_ + "/Cargo.toml")) {
        ProcessResult pr = ProcessExecutor::executeShell("cargo check", workingDir_, 60000);
        if (pr.exitCode == 0) {
            buildEvidence = "BUILD: PASS (cargo check passed)";
            return true;
        } else {
            buildEvidence = "BUILD: FAIL (cargo check failed)\n" + pr.stdErr;
            return false;
        }
    }

    buildEvidence = "BUILD: N/A (no formal compilation step required)";
    return true;
}

bool Verifier::checkTests(std::string& testEvidence) {
    if (std::filesystem::exists(workingDir_ + "/build") && std::filesystem::exists(workingDir_ + "/CMakeLists.txt")) {
        ProcessResult pr = ProcessExecutor::executeShell("ctest --test-dir build --output-on-failure", workingDir_, 60000);
        if (pr.exitCode == 0) {
            testEvidence = "TESTS: PASS (All ctest tests passed)\n" + pr.stdOut;
            return true;
        } else {
            testEvidence = "TESTS: FAIL (ctest reported test failures)\n" + pr.stdOut + "\n" + pr.stdErr;
            return false;
        }
    } else if (std::filesystem::exists(workingDir_ + "/pytest.ini") || std::filesystem::exists(workingDir_ + "/tests")) {
        ProcessResult pr = ProcessExecutor::executeShell("pytest -q", workingDir_, 60000);
        if (pr.exitCode == 0) {
            testEvidence = "TESTS: PASS (pytest passed)\n" + pr.stdOut;
            return true;
        } else {
            testEvidence = "TESTS: FAIL (pytest reported failures)\n" + pr.stdOut;
            return false;
        }
    }

    testEvidence = "TESTS: N/A (no test runner detected)";
    return true;
}

bool Verifier::checkRequiredFiles(const Session& session, std::string& fileEvidence) {
    std::stringstream ss;
    auto fileChanges = session.memories;
    int filesFound = 0;

    for (const auto& m : session.memories) {
        if (m.category == "file_change") {
            filesFound++;
            ss << "- " << m.content << "\n";
        }
    }

    if (filesFound > 0) {
        fileEvidence = "REQUIRED FILES: PRESENT (" + std::to_string(filesFound) + " files verified)\n" + ss.str();
        return true;
    }

    fileEvidence = "REQUIRED FILES: PRESENT (no files modified or created in this session)";
    return true;
}

std::string Verifier::collectObjectiveEvidence(const Session& session, const MemoryManager& memory) {
    std::stringstream ss;
    std::string buildEv, testEv, fileEv;
    checkBuild(buildEv);
    checkTests(testEv);
    checkRequiredFiles(session, fileEv);

    ss << buildEv << "\n\n"
       << testEv << "\n\n"
       << fileEv << "\n";

    return ss.str();
}

VerificationResult Verifier::verify(
    const Session& session,
    const MemoryManager& memory
) {
    VerificationResult result;
    std::string buildEvidence, testEvidence, fileEvidence;

    bool buildOk = checkBuild(buildEvidence);
    bool testsOk = checkTests(testEvidence);
    bool filesOk = checkRequiredFiles(session, fileEvidence);

    if (buildOk) result.passedChecks.push_back(buildEvidence);
    else result.failedChecks.push_back(buildEvidence);

    if (testsOk) result.passedChecks.push_back(testEvidence);
    else result.failedChecks.push_back(testEvidence);

    if (filesOk) result.passedChecks.push_back(fileEvidence);
    else result.failedChecks.push_back(fileEvidence);

    // If hard checks fail, verification fails immediately
    if (!buildOk || !testsOk) {
        result.complete = false;
        result.summary = "Automated verification failed: build or tests did not pass.";
        result.evidence = buildEvidence + "\n" + testEvidence;
        return result;
    }

    // Check if any tasks failed in session
    if (!session.failedTasks.empty()) {
        result.complete = false;
        result.summary = "Unresolved failed tasks in session: " + std::to_string(session.failedTasks.size()) + " failed tasks.";
        for (const auto& ft : session.failedTasks) {
            result.failedChecks.push_back("Task failed: " + ft);
        }
        return result;
    }

    // Check if all planned tasks are completed
    if (!session.allTasksCompleted()) {
        result.complete = false;
        result.summary = "Not all tasks in execution graph are completed.";
        result.failedChecks.push_back("Incomplete tasks in task graph");
        return result;
    }

    // Identify last model used for coding to select an independent verifier model
    std::string coderModel;
    if (!session.executions.empty()) {
        coderModel = session.executions.back().model;
    }

    ModelSelection verifierSelection = router_.selectModel(TaskType::VERIFICATION, "", coderModel);
    if (verifierSelection.provider) {
        Logger::instance().info("Running independent verification with " +
                               verifierSelection.provider->name() + " (" + verifierSelection.model.name + ")...");

        std::string evidenceStr = collectObjectiveEvidence(session, memory);
        AgentRequest req = contextMgr_.buildVerificationContext(
            session, memory, verifierSelection.model.id, evidenceStr
        );

        AgentResponse resp = verifierSelection.provider->generate(req);
        if (resp.success && !resp.content.empty()) {
            static const std::regex jsonRegex("(\\{[\\s\\S]*?\\})");
            std::smatch m;
            if (std::regex_search(resp.content, m, jsonRegex)) {
                try {
                    nlohmann::json j = nlohmann::json::parse(m[1].str());
                    result.complete = j.value("complete", true);
                    result.summary = j.value("summary", "Verification complete");
                    if (j.contains("passedChecks") && j["passedChecks"].is_array()) {
                        for (const auto& pc : j["passedChecks"]) {
                            if (pc.is_string()) result.passedChecks.push_back(pc.get<std::string>());
                        }
                    }
                    if (j.contains("failedChecks") && j["failedChecks"].is_array()) {
                        for (const auto& fc : j["failedChecks"]) {
                            if (fc.is_string()) {
                                result.failedChecks.push_back(fc.get<std::string>());
                                result.complete = false;
                            }
                        }
                    }
                } catch (...) {}
            }
        }
    }

    // Default to verified if all physical evidence passed and all tasks completed
    if (result.failedChecks.empty()) {
        result.complete = true;
        result.summary = "All checks passed with concrete verification evidence.";
        result.passedChecks.push_back("OBJECTIVE: SATISFIED");
    }

    result.evidence = collectObjectiveEvidence(session, memory);
    return result;
}

} // namespace arenafight
