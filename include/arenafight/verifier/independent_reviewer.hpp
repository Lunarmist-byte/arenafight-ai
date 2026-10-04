#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>
#include <memory>
#include "arenafight/common/types.hpp"
#include "arenafight/evidence/evidence_types.hpp"
#include "arenafight/evidence/claim_registry.hpp"
#include "arenafight/router/model_router.hpp"
#include "arenafight/session/context_manager.hpp"

namespace arenafight {

struct BlindReviewResult {
    bool passed = false;
    std::string selectedOption; // e.g. "SOLUTION A"
    std::string rationale;
    std::vector<std::string> identifiedWeaknesses;
    std::vector<std::string> unsupportedClaims;
    double confidence = 0.0;
};

struct AdversarialReviewResult {
    bool approved = false;
    std::vector<std::string> vulnerabilitiesFound;
    std::vector<std::string> incorrectAssumptions;
    std::vector<std::string> missingEdgeCases;
    std::vector<std::string> unsupportedClaims;
    std::string summary;
    ReviewMode mode = ReviewMode::ADVERSARIAL;
};

struct IndependentInvestigationResult {
    std::string hypothesisA;
    std::string hypothesisB;
    bool agreement = false;
    std::string sharedConclusion;
    std::string divergence;
    bool conflictDetected = false;
    std::string requiredToolAction;
};

class IndependentReviewer {
public:
    IndependentReviewer(ModelRouter& router, ContextManager& contextMgr);

    // Section 7: Blind Review (anonymizes candidates to avoid provider bias)
    BlindReviewResult performBlindReview(
        const std::string& problemDescription,
        const std::vector<std::pair<std::string, std::string>>& candidateSolutions, // pair<solutionText, realSource>
        const std::string& reviewerModel = ""
    );

    // Section 13 & 26: Adversarial and Security Verification
    AdversarialReviewResult performAdversarialReview(
        const std::string& solutionOrPatch,
        const std::string& taskObjective,
        const std::map<std::string, std::string>& relevantFiles = {},
        ReviewMode mode = ReviewMode::ADVERSARIAL,
        const std::string& reviewerModel = ""
    );

    // Section 6: Independent Multi-Model Investigation (no confirmation bias)
    IndependentInvestigationResult conductIndependentInvestigation(
        const std::string& problemStatement,
        const std::string& workspaceOverview,
        const std::string& modelA = "",
        const std::string& modelB = ""
    );

private:
    ModelRouter& router_;
    ContextManager& contextMgr_;

    std::string buildAdversarialReviewPrompt(
        const std::string& taskObjective,
        const std::string& solution,
        const std::map<std::string, std::string>& relevantFiles,
        ReviewMode mode
    );
};

} // namespace arenafight
