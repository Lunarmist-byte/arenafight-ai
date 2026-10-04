#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace arenafight {

// Section 1: Core Principle - Evidence Levels
enum class EvidenceLevel {
    VERIFIED,
    STRONGLY_SUPPORTED,
    MODEL_DERIVED,
    UNVERIFIED,
    CONTRADICTED,
    UNKNOWN
};

inline std::string evidenceLevelToString(EvidenceLevel level) {
    switch (level) {
        case EvidenceLevel::VERIFIED: return "VERIFIED";
        case EvidenceLevel::STRONGLY_SUPPORTED: return "STRONGLY_SUPPORTED";
        case EvidenceLevel::MODEL_DERIVED: return "MODEL_DERIVED";
        case EvidenceLevel::UNVERIFIED: return "UNVERIFIED";
        case EvidenceLevel::CONTRADICTED: return "CONTRADICTED";
        case EvidenceLevel::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

inline EvidenceLevel evidenceLevelFromString(const std::string& str) {
    if (str == "VERIFIED") return EvidenceLevel::VERIFIED;
    if (str == "STRONGLY_SUPPORTED") return EvidenceLevel::STRONGLY_SUPPORTED;
    if (str == "MODEL_DERIVED") return EvidenceLevel::MODEL_DERIVED;
    if (str == "UNVERIFIED") return EvidenceLevel::UNVERIFIED;
    if (str == "CONTRADICTED") return EvidenceLevel::CONTRADICTED;
    return EvidenceLevel::UNKNOWN;
}

// Section 5: Separate Facts From Hypotheses
enum class KnowledgeType {
    FACT,
    HYPOTHESIS,
    ASSUMPTION,
    INFERENCE,
    UNKNOWN
};

inline std::string knowledgeTypeToString(KnowledgeType type) {
    switch (type) {
        case KnowledgeType::FACT: return "FACT";
        case KnowledgeType::HYPOTHESIS: return "HYPOTHESIS";
        case KnowledgeType::ASSUMPTION: return "ASSUMPTION";
        case KnowledgeType::INFERENCE: return "INFERENCE";
        case KnowledgeType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

inline KnowledgeType knowledgeTypeFromString(const std::string& str) {
    if (str == "FACT") return KnowledgeType::FACT;
    if (str == "HYPOTHESIS") return KnowledgeType::HYPOTHESIS;
    if (str == "ASSUMPTION") return KnowledgeType::ASSUMPTION;
    if (str == "INFERENCE") return KnowledgeType::INFERENCE;
    return KnowledgeType::UNKNOWN;
}

// Section 13: Adversarial Verification Mode
enum class ReviewMode {
    NORMAL,
    ADVERSARIAL
};

inline std::string reviewModeToString(ReviewMode mode) {
    return (mode == ReviewMode::ADVERSARIAL) ? "ADVERSARIAL" : "NORMAL";
}

inline ReviewMode reviewModeFromString(const std::string& str) {
    return (str == "ADVERSARIAL") ? ReviewMode::ADVERSARIAL : ReviewMode::NORMAL;
}

// Section 10: Evidence Hierarchy (Level 1 dominates Level 6)
enum class EvidenceHierarchy {
    DIRECT_EXECUTION = 1,   // compiler, tests, shell exit codes, program output
    DIRECT_ARTIFACT = 2,    // source files, configs, git diff, database contents
    AUTHORITATIVE_DOCS = 3, // official documentation, API schemas, local specs
    INDEPENDENT_AGREEMENT = 4, // multiple independent models
    SINGLE_MODEL = 5,       // single model reasoning / inference
    GUESS_ASSUMPTION = 6    // guess / assumption
};

inline std::string evidenceHierarchyToString(EvidenceHierarchy h) {
    switch (h) {
        case EvidenceHierarchy::DIRECT_EXECUTION: return "LEVEL 1 - Direct Execution Evidence";
        case EvidenceHierarchy::DIRECT_ARTIFACT: return "LEVEL 2 - Direct Artifact Evidence";
        case EvidenceHierarchy::AUTHORITATIVE_DOCS: return "LEVEL 3 - Authoritative Documentation";
        case EvidenceHierarchy::INDEPENDENT_AGREEMENT: return "LEVEL 4 - Independent Model Agreement";
        case EvidenceHierarchy::SINGLE_MODEL: return "LEVEL 5 - Single Model Reasoning";
        case EvidenceHierarchy::GUESS_ASSUMPTION: return "LEVEL 6 - Guess / Assumption";
        default: return "UNKNOWN HIERARCHY";
    }
}

// Section 18: Tool Results Are Evidence (Raw Observation)
struct Observation {
    std::string id;
    std::string tool;
    std::string command;
    std::string stdoutText;
    std::string stderrText;
    int exitCode = 0;
    double duration = 0.0;
    int64_t timestamp = 0;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"tool", tool},
            {"command", command},
            {"stdoutText", stdoutText},
            {"stderrText", stderrText},
            {"exitCode", exitCode},
            {"duration", duration},
            {"timestamp", timestamp}
        };
    }

    static Observation fromJson(const nlohmann::json& j) {
        Observation obs;
        obs.id = j.value("id", "");
        obs.tool = j.value("tool", "");
        obs.command = j.value("command", "");
        obs.stdoutText = j.value("stdoutText", "");
        obs.stderrText = j.value("stderrText", "");
        obs.exitCode = j.value("exitCode", 0);
        obs.duration = j.value("duration", 0.0);
        obs.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        return obs;
    }
};

// Section 3: Evidence Objects
struct Evidence {
    std::string id;
    EvidenceLevel level = EvidenceLevel::UNKNOWN;
    EvidenceHierarchy hierarchy = EvidenceHierarchy::GUESS_ASSUMPTION;
    std::string source;      // e.g. "cmake", "ctest", "read_file", "Model A"
    std::string claim;       // Claim statement it supports or contradicts
    std::string evidence;    // Raw proof: output, test logs, file snippet, diff
    std::string location;    // File path, line number, or URL
    std::string timestamp;   // ISO timestamp
    double confidence = 0.0; // Evidence-calculated confidence (0.0 to 1.0)
    std::string observationId; // Linked raw observation if produced by tool
    bool isStale = false;    // Fresh verification: true if modified artifact invalidated this evidence
    std::vector<std::string> dependentArtifacts; // Files this evidence depends on (e.g. "src/main.cpp")

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"level", evidenceLevelToString(level)},
            {"hierarchy", static_cast<int>(hierarchy)},
            {"source", source},
            {"claim", claim},
            {"evidence", evidence},
            {"location", location},
            {"timestamp", timestamp},
            {"confidence", confidence},
            {"observationId", observationId},
            {"isStale", isStale},
            {"dependentArtifacts", dependentArtifacts}
        };
    }

    static Evidence fromJson(const nlohmann::json& j) {
        Evidence e;
        e.id = j.value("id", "");
        e.level = evidenceLevelFromString(j.value("level", "UNKNOWN"));
        e.hierarchy = static_cast<EvidenceHierarchy>(j.value("hierarchy", 6));
        e.source = j.value("source", "");
        e.claim = j.value("claim", "");
        e.evidence = j.value("evidence", "");
        e.location = j.value("location", "");
        e.timestamp = j.value("timestamp", "");
        e.confidence = j.value("confidence", 0.0);
        e.observationId = j.value("observationId", "");
        e.isStale = j.value("isStale", false);
        e.dependentArtifacts = j.value("dependentArtifacts", std::vector<std::string>{});
        return e;
    }
};

// Section 4: Claim Registry Structures
struct Claim {
    std::string id;
    std::string statement;
    KnowledgeType type = KnowledgeType::HYPOTHESIS;
    std::vector<std::string> evidenceIds;
    EvidenceLevel status = EvidenceLevel::UNVERIFIED;
    double confidence = 0.0;
    bool requiresVerification = true;
    bool verified = false;
    std::string sourceModel;
    int64_t timestamp = 0;
    std::string targetArtifact; // File path or module this claim addresses
    int modelAgreementCount = 1; // Number of independent models asserting this
    int totalModelsQueried = 1;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"statement", statement},
            {"type", knowledgeTypeToString(type)},
            {"evidenceIds", evidenceIds},
            {"status", evidenceLevelToString(status)},
            {"confidence", confidence},
            {"requiresVerification", requiresVerification},
            {"verified", verified},
            {"sourceModel", sourceModel},
            {"timestamp", timestamp},
            {"targetArtifact", targetArtifact},
            {"modelAgreementCount", modelAgreementCount},
            {"totalModelsQueried", totalModelsQueried}
        };
    }

    static Claim fromJson(const nlohmann::json& j) {
        Claim c;
        c.id = j.value("id", "");
        c.statement = j.value("statement", "");
        c.type = knowledgeTypeFromString(j.value("type", "HYPOTHESIS"));
        c.evidenceIds = j.value("evidenceIds", std::vector<std::string>{});
        c.status = evidenceLevelFromString(j.value("status", "UNVERIFIED"));
        c.confidence = j.value("confidence", 0.0);
        c.requiresVerification = j.value("requiresVerification", true);
        c.verified = j.value("verified", false);
        c.sourceModel = j.value("sourceModel", "");
        c.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        c.targetArtifact = j.value("targetArtifact", "");
        c.modelAgreementCount = j.value("modelAgreementCount", 1);
        c.totalModelsQueried = j.value("totalModelsQueried", 1);
        return c;
    }
};

// Section 8: Contradiction Structure
struct Contradiction {
    std::string id;
    std::string claimIdA;
    std::string claimIdB;
    std::string statementA;
    std::string statementB;
    std::string description;
    std::string requiredAction;
    bool resolved = false;
    std::string resolutionEvidenceId;
    std::string resolutionNotes;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"claimIdA", claimIdA},
            {"claimIdB", claimIdB},
            {"statementA", statementA},
            {"statementB", statementB},
            {"description", description},
            {"requiredAction", requiredAction},
            {"resolved", resolved},
            {"resolutionEvidenceId", resolutionEvidenceId},
            {"resolutionNotes", resolutionNotes}
        };
    }

    static Contradiction fromJson(const nlohmann::json& j) {
        Contradiction c;
        c.id = j.value("id", "");
        c.claimIdA = j.value("claimIdA", "");
        c.claimIdB = j.value("claimIdB", "");
        c.statementA = j.value("statementA", "");
        c.statementB = j.value("statementB", "");
        c.description = j.value("description", "");
        c.requiredAction = j.value("requiredAction", "");
        c.resolved = j.value("resolved", false);
        c.resolutionEvidenceId = j.value("resolutionEvidenceId", "");
        c.resolutionNotes = j.value("resolutionNotes", "");
        return c;
    }
};

// Section 14: Verification Gate Results
struct GateCheckResult {
    std::string gateName;
    bool passed = false;
    std::string detail;
    std::string evidenceId;
    EvidenceHierarchy hierarchy = EvidenceHierarchy::GUESS_ASSUMPTION;
};

// Section 29: Final Answer Structure
struct GroundedFinalAnswer {
    std::string resultSummary;
    std::vector<std::string> changes;
    std::vector<GateCheckResult> verificationGates;
    std::vector<Evidence> supportingEvidence;
    std::vector<Claim> verifiedClaims;
    std::vector<Claim> unverifiedClaims;
    std::vector<std::string> remainingIssues;
    double completionScore = 0.0;
    double overallConfidence = 0.0;
    std::string confidenceRationale;
    bool isFullyComplete = false;
    std::string formattedReport;
};

} // namespace arenafight
