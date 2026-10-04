#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <nlohmann/json.hpp>
#include "arenafight/evidence/evidence_types.hpp"

namespace arenafight {

class ClaimRegistry {
public:
    ClaimRegistry();

    // Section 18: Tool Results Are Evidence (Raw Observation Storage)
    std::string recordObservation(const Observation& obs);

    // Section 3 & 10: Evidence Storage & Hierarchy Evaluation
    std::string addEvidence(Evidence evidence);

    // Section 4 & 5: Central Claim Registry & Fact/Hypothesis Separation
    std::string registerClaim(Claim claim);

    // Link an Evidence item to a Claim, updating claim confidence & verification state
    void linkEvidenceToClaim(const std::string& claimId, const std::string& evidenceId);

    // Section 8: Contradiction Detection & Resolution
    std::vector<Contradiction> detectContradictions();
    bool resolveContradiction(
        const std::string& contradictionId,
        const std::string& resolvingEvidenceId,
        const std::string& notes
    );

    // Section 21 & 22: Fresh Verification After Changes & Dependency-Aware Evidence
    // Invalidates any evidence and claims depending on the modified file
    int invalidateArtifactEvidence(const std::string& modifiedFile);

    // Section 6 & 9 & 31: Multi-Model Claims & Independent Agreement
    // Multiple models agreeing increases confidence up to MODEL_DERIVED, but NEVER establishes VERIFIED without tool evidence!
    std::string recordModelAssertion(
        const std::string& statement,
        const std::string& modelName,
        KnowledgeType type = KnowledgeType::HYPOTHESIS,
        const std::string& targetArtifact = ""
    );

    // Section 17: Source Attribution
    std::vector<Evidence> getTraceableEvidenceForClaim(const std::string& claimId) const;

    // Evaluate confidence and status of a claim based on linked evidence
    void evaluateClaim(Claim& claim) const;

    // Promote a verified hypothesis to FACT once concrete evidence is linked
    bool promoteHypothesisToFact(const std::string& claimId, const std::string& evidenceId);

    // Queries
    const std::vector<Observation>& getObservations() const { return observations_; }
    const std::vector<Evidence>& getEvidences() const { return evidences_; }
    const std::vector<Claim>& getClaims() const { return claims_; }
    const std::vector<Contradiction>& getContradictions() const { return contradictions_; }

    const Claim* getClaim(const std::string& claimId) const;
    const Evidence* getEvidence(const std::string& evidenceId) const;
    const Observation* getObservation(const std::string& obsId) const;

    std::vector<Contradiction> getUnresolvedContradictions() const;
    std::vector<Claim> getVerifiedClaims() const;
    std::vector<Claim> getUnverifiedClaims() const;
    std::vector<Claim> getContradictedClaims() const;

    // Evidence summary for reporting
    std::string getEvidenceReportSummary() const;

    // Serialization
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

    void clear();

private:
    mutable std::mutex mutex_;
    std::vector<Observation> observations_;
    std::vector<Evidence> evidences_;
    std::vector<Claim> claims_;
    std::vector<Contradiction> contradictions_;

    int nextObsId_ = 1;
    int nextEvId_ = 1;
    int nextClaimId_ = 1;
    int nextConflictId_ = 1;

    std::string generateObsId();
    std::string generateEvidenceId();
    std::string generateClaimId();
    std::string generateConflictId();
};

} // namespace arenafight
