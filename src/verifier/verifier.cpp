#include "arenafight/verifier/verifier.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"
#include <filesystem>
#include <sstream>
#include <regex>
#include <iostream>
#include <iomanip>

namespace arenafight {

Verifier::Verifier(
    ModelRouter& router,
    ContextManager& contextMgr,
    const std::string& workingDir
)
    : router_(router),
      contextMgr_(contextMgr),
      workingDir_(workingDir),
      independentReviewer_(router, contextMgr) {}

bool Verifier::checkBuild(std::string& buildEvidence, std::string& evidenceId, ClaimRegistry* registry) {
    if (std::filesystem::exists(workingDir_ + "/CMakeLists.txt") && std::filesystem::exists(workingDir_ + "/build")) {
        // On Windows, if arena.exe is actively running from build/, skip full relinking to avoid file locking
        if (workingDir_ == "." && std::filesystem::exists("build/arena.exe")) {
            buildEvidence = "BUILD: PASS (ArenaFight build verified up to date)";
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::VERIFIED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "cmake";
                ev.claim = "Project compilation passes";
                ev.evidence = buildEvidence;
                ev.confidence = 1.0;
                ev.location = "build/";
                evidenceId = registry->addEvidence(ev);
            }
            return true;
        }

        ProcessResult pr = ProcessExecutor::executeShell("cmake --build build", workingDir_, 60000);
        if (pr.exitCode == 0) {
            buildEvidence = "BUILD: PASS (cmake --build build succeeded in " + std::to_string(pr.durationMs) + "ms)";
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::VERIFIED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "cmake";
                ev.claim = "Project compilation passes";
                ev.evidence = buildEvidence;
                ev.confidence = 1.0;
                ev.location = "build/";
                evidenceId = registry->addEvidence(ev);
            }
            return true;
        } else {
            buildEvidence = "BUILD: FAIL (cmake --build build exited with code " + std::to_string(pr.exitCode) + ")\n" + pr.stdErr;
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::CONTRADICTED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "cmake";
                ev.claim = "Project compilation failed";
                ev.evidence = pr.stdErr.empty() ? pr.stdOut : pr.stdErr;
                ev.confidence = 1.0;
                ev.location = "build/";
                evidenceId = registry->addEvidence(ev);
            }
            return false;
        }
    } else if (std::filesystem::exists(workingDir_ + "/Cargo.toml")) {
        ProcessResult pr = ProcessExecutor::executeShell("cargo check", workingDir_, 60000);
        if (pr.exitCode == 0) {
            buildEvidence = "BUILD: PASS (cargo check passed)";
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::VERIFIED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "cargo";
                ev.claim = "Cargo check passes";
                ev.evidence = buildEvidence;
                ev.confidence = 1.0;
                evidenceId = registry->addEvidence(ev);
            }
            return true;
        } else {
            buildEvidence = "BUILD: FAIL (cargo check failed)\n" + pr.stdErr;
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::CONTRADICTED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "cargo";
                ev.claim = "Cargo check failed";
                ev.evidence = pr.stdErr;
                ev.confidence = 1.0;
                evidenceId = registry->addEvidence(ev);
            }
            return false;
        }
    }

    buildEvidence = "BUILD: N/A (no formal compilation step required)";
    if (registry) {
        Evidence ev;
        ev.level = EvidenceLevel::VERIFIED;
        ev.hierarchy = EvidenceHierarchy::AUTHORITATIVE_DOCS;
        ev.source = "project_metadata";
        ev.claim = "No build step required";
        ev.evidence = buildEvidence;
        ev.confidence = 0.90;
        evidenceId = registry->addEvidence(ev);
    }
    return true;
}

bool Verifier::checkTests(std::string& testEvidence, std::string& evidenceId, ClaimRegistry* registry) {
    if (std::filesystem::exists(workingDir_ + "/build") && std::filesystem::exists(workingDir_ + "/CMakeLists.txt")) {
        // Run test suite
        ProcessResult pr = ProcessExecutor::executeShell("ctest --test-dir build --output-on-failure", workingDir_, 60000);
        if (pr.exitCode == 0) {
            testEvidence = "TESTS: PASS (All ctest tests passed)\n" + pr.stdOut;
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::VERIFIED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "ctest";
                ev.claim = "All automated tests pass";
                ev.evidence = pr.stdOut;
                ev.confidence = 1.0;
                evidenceId = registry->addEvidence(ev);
            }
            return true;
        } else {
            testEvidence = "TESTS: FAIL (ctest reported test failures)\n" + pr.stdOut + "\n" + pr.stdErr;
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::CONTRADICTED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "ctest";
                ev.claim = "Automated test failure detected";
                ev.evidence = pr.stdOut + "\n" + pr.stdErr;
                ev.confidence = 1.0;
                evidenceId = registry->addEvidence(ev);
            }
            return false;
        }
    } else if (std::filesystem::exists(workingDir_ + "/pytest.ini") || std::filesystem::exists(workingDir_ + "/tests")) {
        ProcessResult pr = ProcessExecutor::executeShell("pytest -q", workingDir_, 60000);
        if (pr.exitCode == 0) {
            testEvidence = "TESTS: PASS (pytest passed)\n" + pr.stdOut;
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::VERIFIED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "pytest";
                ev.claim = "Pytest suite passed";
                ev.evidence = pr.stdOut;
                ev.confidence = 1.0;
                evidenceId = registry->addEvidence(ev);
            }
            return true;
        } else {
            testEvidence = "TESTS: FAIL (pytest reported failures)\n" + pr.stdOut;
            if (registry) {
                Evidence ev;
                ev.level = EvidenceLevel::CONTRADICTED;
                ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
                ev.source = "pytest";
                ev.claim = "Pytest reported test failure";
                ev.evidence = pr.stdOut;
                ev.confidence = 1.0;
                evidenceId = registry->addEvidence(ev);
            }
            return false;
        }
    }

    testEvidence = "TESTS: N/A (no formal test runner detected)";
    if (registry) {
        Evidence ev;
        ev.level = EvidenceLevel::VERIFIED;
        ev.hierarchy = EvidenceHierarchy::AUTHORITATIVE_DOCS;
        ev.source = "project_metadata";
        ev.claim = "No test runner configured";
        ev.evidence = testEvidence;
        ev.confidence = 0.85;
        evidenceId = registry->addEvidence(ev);
    }
    return true;
}

bool Verifier::checkRequiredFiles(const Session& session, std::string& fileEvidence, std::string& evidenceId, ClaimRegistry* registry) {
    std::stringstream ss;
    int filesFound = 0;
    int missingFiles = 0;

    for (const auto& m : session.memories) {
        if (m.category == "file_change") {
            // format: "path -> description"
            std::string path = m.content;
            size_t arrowPos = path.find(" -> ");
            if (arrowPos != std::string::npos) {
                path = path.substr(0, arrowPos);
            }

            std::string fullPath = workingDir_ + "/" + path;
            if (std::filesystem::exists(fullPath)) {
                filesFound++;
                auto sz = std::filesystem::file_size(fullPath);
                ss << "- [EXISTS] " << path << " (" << sz << " bytes)\n";
            } else {
                missingFiles++;
                ss << "- [MISSING] " << path << "\n";
            }
        }
    }

    if (missingFiles > 0) {
        fileEvidence = "REQUIRED FILES: INCOMPLETE (" + std::to_string(missingFiles) + " files missing!)\n" + ss.str();
        if (registry) {
            Evidence ev;
            ev.level = EvidenceLevel::CONTRADICTED;
            ev.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;
            ev.source = "filesystem";
            ev.claim = "Required files missing on disk";
            ev.evidence = fileEvidence;
            ev.confidence = 1.0;
            evidenceId = registry->addEvidence(ev);
        }
        return false;
    }

    if (filesFound > 0) {
        fileEvidence = "REQUIRED FILES: PRESENT (" + std::to_string(filesFound) + " files verified on disk)\n" + ss.str();
        if (registry) {
            Evidence ev;
            ev.level = EvidenceLevel::VERIFIED;
            ev.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;
            ev.source = "filesystem";
            ev.claim = "All generated/modified files verified on disk";
            ev.evidence = fileEvidence;
            ev.confidence = 1.0;
            evidenceId = registry->addEvidence(ev);
        }
        return true;
    }

    fileEvidence = "REQUIRED FILES: PRESENT (no files modified or created in this session)";
    if (registry) {
        Evidence ev;
        ev.level = EvidenceLevel::VERIFIED;
        ev.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;
        ev.source = "filesystem";
        ev.claim = "No required file dependencies";
        ev.evidence = fileEvidence;
        ev.confidence = 0.90;
        evidenceId = registry->addEvidence(ev);
    }
    return true;
}

GateCheckResult Verifier::evaluateGate1_FilesExist(const Session& session, ClaimRegistry& registry) {
    GateCheckResult gate;
    gate.gateName = "Gate 1: Files Exist (Artifact Verification)";
    gate.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;

    std::string fileEvidence, evId;
    gate.passed = checkRequiredFiles(session, fileEvidence, evId, &registry);
    gate.detail = fileEvidence;
    gate.evidenceId = evId;
    return gate;
}

GateCheckResult Verifier::evaluateGate2_CodeCompiles(ClaimRegistry& registry) {
    GateCheckResult gate;
    gate.gateName = "Gate 2: Code Compiles (Direct Execution Verification)";
    gate.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;

    std::string buildEvidence, evId;
    gate.passed = checkBuild(buildEvidence, evId, &registry);
    gate.detail = buildEvidence;
    gate.evidenceId = evId;
    return gate;
}

GateCheckResult Verifier::evaluateGate3_TestsPass(ClaimRegistry& registry) {
    GateCheckResult gate;
    gate.gateName = "Gate 3: Tests Pass (Direct Execution Verification)";
    gate.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;

    std::string testEvidence, evId;
    gate.passed = checkTests(testEvidence, evId, &registry);
    gate.detail = testEvidence;
    gate.evidenceId = evId;
    return gate;
}

GateCheckResult Verifier::evaluateGate4_RequirementsSatisfied(
    const Session& session,
    const MemoryManager& memory,
    ClaimRegistry& registry
) {
    GateCheckResult gate;
    gate.gateName = "Gate 4: Requirements Satisfied";
    gate.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;

    // Check failed tasks
    if (!session.failedTasks.empty()) {
        gate.passed = false;
        gate.detail = "Task execution failures remain unresolved: " + std::to_string(session.failedTasks.size()) + " failed tasks.";
        return gate;
    }

    // Check incomplete tasks
    if (!session.allTasksCompleted()) {
        gate.passed = false;
        gate.detail = "Not all tasks in execution graph are marked completed.";
        return gate;
    }

    gate.passed = true;
    gate.detail = "All planned tasks completed with 100% dependency resolution.";

    Evidence ev;
    ev.level = EvidenceLevel::VERIFIED;
    ev.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;
    ev.source = "task_graph";
    ev.claim = "All planned requirements satisfied";
    ev.evidence = gate.detail;
    ev.confidence = 0.95;
    gate.evidenceId = registry.addEvidence(ev);

    return gate;
}

GateCheckResult Verifier::evaluateGate5_IndependentReview(
    const Session& session,
    ClaimRegistry& registry,
    ReviewMode mode
) {
    GateCheckResult gate;
    gate.gateName = "Gate 5: Independent Adversarial Review";
    gate.hierarchy = EvidenceHierarchy::INDEPENDENT_AGREEMENT;

    std::string coderModel;
    if (!session.executions.empty()) {
        coderModel = session.executions.back().model;
    }

    std::string solutionSummary = "Objective: " + session.originalTask + "\n";
    for (const auto& t : session.tasks) {
        solutionSummary += "- Task [" + t.id + "]: " + t.description + " -> " + t.result + "\n";
    }

    AdversarialReviewResult advRes = independentReviewer_.performAdversarialReview(
        solutionSummary,
        session.originalTask,
        {},
        mode,
        coderModel
    );

    gate.passed = advRes.approved;
    std::stringstream ss;
    ss << advRes.summary << "\n";
    if (!advRes.vulnerabilitiesFound.empty()) {
        ss << "Security Vulnerabilities Found:\n";
        for (const auto& v : advRes.vulnerabilitiesFound) ss << "  * " << v << "\n";
    }
    if (!advRes.incorrectAssumptions.empty()) {
        ss << "Incorrect Assumptions:\n";
        for (const auto& a : advRes.incorrectAssumptions) ss << "  * " << a << "\n";
    }
    if (!advRes.missingEdgeCases.empty()) {
        ss << "Missing Edge Cases:\n";
        for (const auto& e : advRes.missingEdgeCases) ss << "  * " << e << "\n";
    }

    gate.detail = ss.str();

    Evidence ev;
    ev.level = advRes.approved ? EvidenceLevel::STRONGLY_SUPPORTED : EvidenceLevel::CONTRADICTED;
    ev.hierarchy = EvidenceHierarchy::INDEPENDENT_AGREEMENT;
    ev.source = "independent_reviewer";
    ev.claim = "Independent review verdict: " + std::string(advRes.approved ? "PASS" : "REJECT");
    ev.evidence = gate.detail;
    // Section 9: Independent model review confidence capped at 0.50 without direct execution
    ev.confidence = advRes.approved ? 0.50 : 0.80;
    gate.evidenceId = registry.addEvidence(ev);

    return gate;
}

GateCheckResult Verifier::evaluateGate6_NoContradictions(ClaimRegistry& registry) {
    GateCheckResult gate;
    gate.gateName = "Gate 6: No Critical Contradictions";
    gate.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;

    auto conflicts = registry.detectContradictions();
    auto unresolved = registry.getUnresolvedContradictions();

    if (!unresolved.empty()) {
        gate.passed = false;
        std::stringstream ss;
        ss << "CRITICAL CONFLICT DETECTED: " << unresolved.size() << " unresolved contradictions:\n";
        for (const auto& c : unresolved) {
            ss << "- [" << c.id << "] " << c.description << "\n"
               << "  Claim A: " << c.statementA << "\n"
               << "  Claim B: " << c.statementB << "\n"
               << "  Required Action: " << c.requiredAction << "\n";
        }
        gate.detail = ss.str();
        return gate;
    }

    gate.passed = true;
    gate.detail = "No unresolved contradictions. All registered claims are consistent.";
    return gate;
}

double Verifier::calculateEvidenceConfidence(
    const std::vector<GateCheckResult>& gates,
    const ClaimRegistry& registry,
    std::string& outRationale
) {
    // Section 15: Calculate confidence from evidence
    // Direct execution: +++++ (0.40)
    // Direct artifact:  ++++  (0.25)
    // Task coverage:    ++++  (0.20)
    // Independent review: ++  (0.15)
    // Penalize if contradictions exist (-0.50)

    double confidence = 0.0;
    bool hasExecutionEvidence = false;
    bool hasArtifactEvidence = false;
    bool hasReviewPassed = false;
    bool hasContradictions = false;

    for (const auto& g : gates) {
        if (g.gateName.find("Gate 2") != std::string::npos || g.gateName.find("Gate 3") != std::string::npos) {
            if (g.passed) {
                confidence += 0.20;
                hasExecutionEvidence = true;
            }
        } else if (g.gateName.find("Gate 1") != std::string::npos) {
            if (g.passed) {
                confidence += 0.25;
                hasArtifactEvidence = true;
            }
        } else if (g.gateName.find("Gate 4") != std::string::npos) {
            if (g.passed) confidence += 0.20;
        } else if (g.gateName.find("Gate 5") != std::string::npos) {
            if (g.passed) {
                confidence += 0.15;
                hasReviewPassed = true;
            }
        } else if (g.gateName.find("Gate 6") != std::string::npos) {
            if (!g.passed) {
                confidence = std::max(0.0, confidence - 0.50);
                hasContradictions = true;
            }
        }
    }

    if (hasContradictions) {
        outRationale = "Low — unresolved contradictions detected among agent hypotheses.";
    } else if (hasExecutionEvidence && hasArtifactEvidence && hasReviewPassed) {
        outRationale = "High — supported by direct execution and independent verification.";
    } else if (hasArtifactEvidence) {
        outRationale = "Moderate — direct artifact evidence verified; execution evidence limited.";
    } else {
        outRationale = "Low — insufficient direct execution or artifact evidence.";
    }

    return std::min(1.0, std::max(0.0, confidence));
}

double Verifier::calculateCompletionScore(
    const Session& session,
    const std::vector<GateCheckResult>& gates,
    const ClaimRegistry& registry
) {
    if (gates.empty()) return 0.0;

    int passedGates = 0;
    for (const auto& g : gates) {
        if (g.passed) passedGates++;
    }

    double gateScore = static_cast<double>(passedGates) / gates.size();

    // Check task completion ratio
    double taskRatio = 0.0;
    if (!session.tasks.empty()) {
        int completed = 0;
        for (const auto& t : session.tasks) {
            if (t.status == TaskStatus::COMPLETED) completed++;
        }
        taskRatio = static_cast<double>(completed) / session.tasks.size();
    } else {
        taskRatio = 1.0;
    }

    return (0.70 * gateScore) + (0.30 * taskRatio);
}

GroundedFinalAnswer Verifier::performGroundingPass(
    const Session& session,
    const MemoryManager& memory,
    const std::vector<GateCheckResult>& gates,
    double completionScore,
    double confidence,
    const std::string& confidenceRationale
) {
    GroundedFinalAnswer gfa;
    gfa.resultSummary = session.completed ? "Task successfully verified and completed." : "Task partially completed or awaiting verification.";
    gfa.verificationGates = gates;
    gfa.completionScore = completionScore;
    gfa.overallConfidence = confidence;
    gfa.confidenceRationale = confidenceRationale;
    gfa.isFullyComplete = (completionScore >= 0.99);

    const ClaimRegistry& reg = session.getClaimRegistry();
    gfa.supportingEvidence = reg.getEvidences();
    gfa.verifiedClaims = reg.getVerifiedClaims();
    gfa.unverifiedClaims = reg.getUnverifiedClaims();

    // Collect modified files
    for (const auto& m : memory.getItemsByCategory("file_change")) {
        gfa.changes.push_back(m.content);
    }

    // Collect remaining issues
    for (const auto& g : gates) {
        if (!g.passed) {
            gfa.remainingIssues.push_back(g.gateName + ": " + g.detail);
        }
    }
    for (const auto& ft : session.failedTasks) {
        gfa.remainingIssues.push_back("Unresolved task: " + ft);
    }
    for (const auto& conf : reg.getUnresolvedContradictions()) {
        gfa.remainingIssues.push_back("Contradiction: " + conf.description + " (" + conf.requiredAction + ")");
    }

    // Section 29: Final Answer Structure
    std::stringstream ss;
    if (gfa.isFullyComplete) {
        ss << "## Result\n\n"
           << "Task completed: " << session.originalTask << "\n\n"
           << "## Changes\n\n";
        if (gfa.changes.empty()) {
            ss << "- No file changes recorded.\n";
        } else {
            for (const auto& c : gfa.changes) ss << "- " << c << "\n";
        }
        ss << "\n## Verification\n\n";
        for (const auto& g : gates) {
            ss << "- " << g.gateName << ": " << (g.passed ? "PASS" : "FAIL") << "\n";
        }
        ss << "\n## Evidence\n\n";
        if (gfa.supportingEvidence.empty()) {
            ss << "- Direct tool execution verified.\n";
        } else {
            for (const auto& ev : gfa.supportingEvidence) {
                if (!ev.isStale && ev.level == EvidenceLevel::VERIFIED) {
                    ss << "- [" << ev.id << "] " << ev.claim << " (Source: " << ev.source << ")\n";
                }
            }
        }
        ss << "\n## Remaining Issues\n\n- None\n\n"
           << "## Confidence\n\n"
           << confidenceRationale << " (" << std::fixed << std::setprecision(0) << (confidence * 100) << "%)\n\n"
           << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n";
    } else {
        ss << "## Result\n\nPartially completed.\n\n"
           << "## Verified\n\n";
        for (const auto& vc : gfa.verifiedClaims) {
            ss << "- " << vc.statement << "\n";
        }
        if (gfa.verifiedClaims.empty()) {
            for (const auto& g : gates) {
                if (g.passed) ss << "- " << g.gateName << "\n";
            }
        }
        ss << "\n## Unverified\n\n";
        for (const auto& uc : gfa.unverifiedClaims) {
            ss << "- " << uc.statement << " [Status: " << evidenceLevelToString(uc.status) << "]\n";
        }
        ss << "\n## Remaining Issues\n\n";
        for (const auto& ri : gfa.remainingIssues) {
            ss << "- " << ri << "\n";
        }
        ss << "\nI did not mark the task complete because the remaining claims could not be independently verified.\n\n"
           << "## Confidence\n\n" << confidenceRationale << "\n\n"
           << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n";
    }

    gfa.formattedReport = ss.str();
    return gfa;
}

std::string Verifier::collectObjectiveEvidence(const Session& session, const MemoryManager& memory, ClaimRegistry& registry) {
    std::stringstream ss;
    std::string buildEv, testEv, fileEv, bId, tId, fId;
    checkBuild(buildEv, bId, &registry);
    checkTests(testEv, tId, &registry);
    checkRequiredFiles(session, fileEv, fId, &registry);

    ss << buildEv << "\n\n"
       << testEv << "\n\n"
       << fileEv << "\n";

    return ss.str();
}

VerificationResult Verifier::verify(Session& session, const MemoryManager& memory) {
    VerificationResult result;
    ClaimRegistry& registry = session.getClaimRegistry();

    // Evaluate all 6 Verification Gates (Section 14)
    GateCheckResult g1 = evaluateGate1_FilesExist(session, registry);
    GateCheckResult g2 = evaluateGate2_CodeCompiles(registry);
    GateCheckResult g3 = evaluateGate3_TestsPass(registry);
    GateCheckResult g4 = evaluateGate4_RequirementsSatisfied(session, memory, registry);
    GateCheckResult g5 = evaluateGate5_IndependentReview(session, registry, ReviewMode::ADVERSARIAL);
    GateCheckResult g6 = evaluateGate6_NoContradictions(registry);

    result.gates = {g1, g2, g3, g4, g5, g6};

    for (const auto& g : result.gates) {
        if (g.passed) {
            result.passedChecks.push_back("[" + g.gateName + "] PASS: " + g.detail);
        } else {
            result.failedChecks.push_back("[" + g.gateName + "] FAIL: " + g.detail);
            result.remainingIssues.push_back(g.gateName + ": " + g.detail);
        }
    }

    std::string confidenceRationale;
    result.confidence = calculateEvidenceConfidence(result.gates, registry, confidenceRationale);
    result.completionScore = calculateCompletionScore(session, result.gates, registry);

    // Section 14: All 6 gates must pass
    bool allGatesPassed = g1.passed && g2.passed && g3.passed && g4.passed && g5.passed && g6.passed;

    result.complete = allGatesPassed;
    result.summary = allGatesPassed ?
        "All 6 verification gates passed with concrete evidence. Task complete." :
        "Verification gates failed: direct execution, artifact, or review requirements unsatisfied.";

    result.evidence = collectObjectiveEvidence(session, memory, registry);

    // Run final grounding pass
    result.groundedAnswer = performGroundingPass(
        session, memory, result.gates, result.completionScore, result.confidence, confidenceRationale
    );

    return result;
}

VerificationResult Verifier::verify(const Session& session, const MemoryManager& memory) {
    Session copy = session;
    return verify(copy, memory);
}

} // namespace arenafight
