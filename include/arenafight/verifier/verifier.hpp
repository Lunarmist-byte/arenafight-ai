#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/session/memory.hpp"
#include "arenafight/session/context_manager.hpp"
#include "arenafight/router/model_router.hpp"
#include "arenafight/evidence/evidence_types.hpp"
#include "arenafight/evidence/claim_registry.hpp"
#include "arenafight/verifier/independent_reviewer.hpp"

namespace arenafight {

class Verifier {
public:
    Verifier(
        ModelRouter& router,
        ContextManager& contextMgr,
        const std::string& workingDir = "."
    );

    // Section 14 & 27: Comprehensive Evidence-First Verification
    VerificationResult verify(
        Session& session,
        const MemoryManager& memory
    );

    // Const overload for read-only checks
    VerificationResult verify(
        const Session& session,
        const MemoryManager& memory
    );

    // Section 11 & 14: Individual Verification Gates
    GateCheckResult evaluateGate1_FilesExist(const Session& session, ClaimRegistry& registry);
    GateCheckResult evaluateGate2_CodeCompiles(ClaimRegistry& registry);
    GateCheckResult evaluateGate3_TestsPass(ClaimRegistry& registry);
    GateCheckResult evaluateGate4_RequirementsSatisfied(const Session& session, const MemoryManager& memory, ClaimRegistry& registry);
    GateCheckResult evaluateGate5_IndependentReview(const Session& session, ClaimRegistry& registry, ReviewMode mode = ReviewMode::ADVERSARIAL);
    GateCheckResult evaluateGate6_NoContradictions(ClaimRegistry& registry);

    // Automated build check (direct execution)
    bool checkBuild(std::string& buildEvidence, std::string& evidenceId, ClaimRegistry* registry = nullptr);

    // Automated tests check (direct execution)
    bool checkTests(std::string& testEvidence, std::string& evidenceId, ClaimRegistry* registry = nullptr);

    // Check required files presence (direct artifact)
    bool checkRequiredFiles(const Session& session, std::string& fileEvidence, std::string& evidenceId, ClaimRegistry* registry = nullptr);

    // Section 15: Evidence-Based Confidence Calculation
    double calculateEvidenceConfidence(
        const std::vector<GateCheckResult>& gates,
        const ClaimRegistry& registry,
        std::string& outRationale
    );

    // Section 30: Completion Score Calculation
    double calculateCompletionScore(
        const Session& session,
        const std::vector<GateCheckResult>& gates,
        const ClaimRegistry& registry
    );

    // Section 28 & 29: Final Grounding Pass and Structured Output Generation
    GroundedFinalAnswer performGroundingPass(
        const Session& session,
        const MemoryManager& memory,
        const std::vector<GateCheckResult>& gates,
        double completionScore,
        double confidence,
        const std::string& confidenceRationale
    );

    IndependentReviewer& getIndependentReviewer() { return independentReviewer_; }

private:
    ModelRouter& router_;
    ContextManager& contextMgr_;
    std::string workingDir_;
    IndependentReviewer independentReviewer_;

    std::string collectObjectiveEvidence(const Session& session, const MemoryManager& memory, ClaimRegistry& registry);
};

} // namespace arenafight
