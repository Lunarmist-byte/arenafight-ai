#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include "arenafight/common/config.hpp"
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/evidence/claim_registry.hpp"
#include "arenafight/finetune/unsloth_manager.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

enum class RejectionReason {
    NONE,
    MOCK_PROVIDER_DETECTED,
    VERIFICATION_FAILED,
    LOW_COMPLETION_SCORE,
    MISSING_PHYSICAL_FILE,
    CONTRADICTIONS_UNRESOLVED,
    TRIVIAL_OR_EMPTY_PROMPT,
    EMPTY_OR_INSUFFICIENT_OUTPUT,
    HALLUCINATED_CONTENT,
    FAILED_SESSION_STATE,
    CORRUPTED_TRAJECTORY
};

[[nodiscard]] inline std::string rejectionReasonToString(RejectionReason reason) {
    switch (reason) {
        case RejectionReason::NONE: return "NONE";
        case RejectionReason::MOCK_PROVIDER_DETECTED: return "MOCK_PROVIDER_DETECTED";
        case RejectionReason::VERIFICATION_FAILED: return "VERIFICATION_FAILED";
        case RejectionReason::LOW_COMPLETION_SCORE: return "LOW_COMPLETION_SCORE";
        case RejectionReason::MISSING_PHYSICAL_FILE: return "MISSING_PHYSICAL_FILE";
        case RejectionReason::CONTRADICTIONS_UNRESOLVED: return "CONTRADICTIONS_UNRESOLVED";
        case RejectionReason::TRIVIAL_OR_EMPTY_PROMPT: return "TRIVIAL_OR_EMPTY_PROMPT";
        case RejectionReason::EMPTY_OR_INSUFFICIENT_OUTPUT: return "EMPTY_OR_INSUFFICIENT_OUTPUT";
        case RejectionReason::HALLUCINATED_CONTENT: return "HALLUCINATED_CONTENT";
        case RejectionReason::FAILED_SESSION_STATE: return "FAILED_SESSION_STATE";
        case RejectionReason::CORRUPTED_TRAJECTORY: return "CORRUPTED_TRAJECTORY";
        default: return "UNKNOWN";
    }
}

[[nodiscard]] inline RejectionReason rejectionReasonFromString(const std::string& str) {
    if (str == "NONE") return RejectionReason::NONE;
    if (str == "MOCK_PROVIDER_DETECTED") return RejectionReason::MOCK_PROVIDER_DETECTED;
    if (str == "VERIFICATION_FAILED") return RejectionReason::VERIFICATION_FAILED;
    if (str == "LOW_COMPLETION_SCORE") return RejectionReason::LOW_COMPLETION_SCORE;
    if (str == "MISSING_PHYSICAL_FILE") return RejectionReason::MISSING_PHYSICAL_FILE;
    if (str == "CONTRADICTIONS_UNRESOLVED") return RejectionReason::CONTRADICTIONS_UNRESOLVED;
    if (str == "TRIVIAL_OR_EMPTY_PROMPT") return RejectionReason::TRIVIAL_OR_EMPTY_PROMPT;
    if (str == "EMPTY_OR_INSUFFICIENT_OUTPUT") return RejectionReason::EMPTY_OR_INSUFFICIENT_OUTPUT;
    if (str == "HALLUCINATED_CONTENT") return RejectionReason::HALLUCINATED_CONTENT;
    if (str == "FAILED_SESSION_STATE") return RejectionReason::FAILED_SESSION_STATE;
    if (str == "CORRUPTED_TRAJECTORY") return RejectionReason::CORRUPTED_TRAJECTORY;
    return RejectionReason::NONE;
}

struct ValidationReport {
    bool isGroundTruth = false;
    RejectionReason reason = RejectionReason::NONE;
    std::string detail;
    double confidenceScore = 0.0;
    std::vector<std::string> verifiedEvidenceIds;
    std::vector<std::string> validatedFiles;

    nlohmann::json toJson() const {
        return {
            {"isGroundTruth", isGroundTruth},
            {"reason", rejectionReasonToString(reason)},
            {"detail", detail},
            {"confidenceScore", confidenceScore},
            {"verifiedEvidenceIds", verifiedEvidenceIds},
            {"validatedFiles", validatedFiles}
        };
    }
};

struct VerifiedLearningPair {
    std::string id;
    std::string prompt;              // User prompt or task objective
    std::string context;             // Execution context and environment constraints
    std::string solution;            // Verified output / code / reasoning
    std::string domain = "coding";   // "coding", "debugging", "reasoning", "hardware"
    double verificationScore = 0.0;
    std::vector<std::string> evidenceIds;
    std::vector<std::string> verifiedFiles;
    std::string sessionId;
    int64_t timestamp = 0;
    std::string model;               // Active model that generated the verified solution

    DatasetEntry toDatasetEntry() const {
        DatasetEntry entry;
        entry.instruction = prompt;
        entry.input = context;
        entry.output = solution;
        entry.domain = domain;
        return entry;
    }

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"prompt", prompt},
            {"context", context},
            {"solution", solution},
            {"domain", domain},
            {"verificationScore", verificationScore},
            {"evidenceIds", evidenceIds},
            {"verifiedFiles", verifiedFiles},
            {"sessionId", sessionId},
            {"timestamp", timestamp},
            {"model", model}
        };
    }

    static VerifiedLearningPair fromJson(const nlohmann::json& j) {
        VerifiedLearningPair p;
        p.id = j.value("id", "");
        p.prompt = j.value("prompt", "");
        p.context = j.value("context", "");
        p.solution = j.value("solution", "");
        p.domain = j.value("domain", "coding");
        p.verificationScore = j.value("verificationScore", 0.0);
        if (j.contains("evidenceIds") && j["evidenceIds"].is_array()) {
            p.evidenceIds = j["evidenceIds"].get<std::vector<std::string>>();
        }
        if (j.contains("verifiedFiles") && j["verifiedFiles"].is_array()) {
            p.verifiedFiles = j["verifiedFiles"].get<std::vector<std::string>>();
        }
        p.sessionId = j.value("sessionId", "");
        p.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        p.model = j.value("model", "arena-verified");
        return p;
    }
};

struct RejectedCandidate {
    std::string id;
    std::string prompt;
    std::string attemptedSolution;
    RejectionReason reason = RejectionReason::NONE;
    std::string reasonDetail;
    std::string sessionId;
    int64_t timestamp = 0;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"prompt", prompt},
            {"attemptedSolution", attemptedSolution},
            {"reason", rejectionReasonToString(reason)},
            {"reasonDetail", reasonDetail},
            {"sessionId", sessionId},
            {"timestamp", timestamp}
        };
    }

    static RejectedCandidate fromJson(const nlohmann::json& j) {
        RejectedCandidate r;
        r.id = j.value("id", "");
        r.prompt = j.value("prompt", "");
        r.attemptedSolution = j.value("attemptedSolution", "");
        r.reason = rejectionReasonFromString(j.value("reason", "NONE"));
        r.reasonDetail = j.value("reasonDetail", "");
        r.sessionId = j.value("sessionId", "");
        r.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        return r;
    }
};

struct SelfLearningStats {
    size_t totalEvaluated = 0;
    size_t verifiedCount = 0;
    size_t rejectedCount = 0;
    std::map<std::string, int> rejectionBreakdown;
    bool autoLearningEnabled = true;
    std::string activeBaseModel = "unsloth/Qwen2.5-Coder-7B-Instruct-bnb-4bit";
    std::string lastFinetunedModel;
    int64_t lastFinetunedTimestamp = 0;

    nlohmann::json toJson() const {
        return {
            {"totalEvaluated", totalEvaluated},
            {"verifiedCount", verifiedCount},
            {"rejectedCount", rejectedCount},
            {"rejectionBreakdown", rejectionBreakdown},
            {"autoLearningEnabled", autoLearningEnabled},
            {"activeBaseModel", activeBaseModel},
            {"lastFinetunedModel", lastFinetunedModel},
            {"lastFinetunedTimestamp", lastFinetunedTimestamp}
        };
    }

    static SelfLearningStats fromJson(const nlohmann::json& j) {
        SelfLearningStats s;
        s.totalEvaluated = j.value("totalEvaluated", static_cast<size_t>(0));
        s.verifiedCount = j.value("verifiedCount", static_cast<size_t>(0));
        s.rejectedCount = j.value("rejectedCount", static_cast<size_t>(0));
        if (j.contains("rejectionBreakdown") && j["rejectionBreakdown"].is_object()) {
            s.rejectionBreakdown = j["rejectionBreakdown"].get<std::map<std::string, int>>();
        }
        s.autoLearningEnabled = j.value("autoLearningEnabled", true);
        s.activeBaseModel = j.value("activeBaseModel", "unsloth/Qwen2.5-Coder-7B-Instruct-bnb-4bit");
        s.lastFinetunedModel = j.value("lastFinetunedModel", "");
        s.lastFinetunedTimestamp = j.value("lastFinetunedTimestamp", static_cast<int64_t>(0));
        return s;
    }
};

class SelfLearningEngine {
public:
    explicit SelfLearningEngine(
        const Config& config,
        const std::string& storageDir = "self_learning"
    );

    // Anti-Fake Data & Ground-Truth Verification
    ValidationReport validateSession(
        const Session& session,
        const VerificationResult* verif = nullptr
    );

    ValidationReport validatePromptPair(
        const std::string& prompt,
        const std::string& solution,
        const std::string& workspaceDir = "."
    );

    // Ingestion Methods
    bool ingestSession(
        const Session& session,
        const VerificationResult* verif = nullptr
    );

    bool ingestPromptPair(
        const std::string& prompt,
        const std::string& context,
        const std::string& solution,
        const std::string& domain = "coding",
        const std::string& model = "user_curated"
    );

    // Configuration
    void setAutoLearning(bool enabled);
    bool isAutoLearningEnabled() const { return stats_.autoLearningEnabled; }

    // Knowledge Store Queries
    std::vector<VerifiedLearningPair> getVerifiedPairs() const;
    std::vector<RejectedCandidate> getRejectedCandidates() const;
    SelfLearningStats getStats() const { return stats_; }

    // Dataset Export
    std::vector<DatasetEntry> exportVerifiedDataset() const;
    bool exportDatasetToJsonl(const std::string& outputPath) const;

    // Trigger Fine-Tuning on 100% Verified Ground Truth
    bool triggerSelfLearningFineTune(
        UnslothManager& unsloth,
        const UnslothTrainConfig& cfg = UnslothTrainConfig(),
        std::function<void(const std::string&)> logCallback = nullptr,
        ModelRouter* routerToRegister = nullptr
    );

    // Scan and ingest past session archives on disk
    size_t scanAndIngestSessions(const std::string& sessionsDir = "sessions");

    // Maintenance
    void reset();
    bool removeVerifiedPair(const std::string& pairId);

private:
    Config config_;
    std::string storageDir_;
    std::string verifiedPath_;
    std::string rejectedPath_;
    std::string metadataPath_;
    SelfLearningStats stats_;

    void loadStats();
    void saveStats();
    void appendVerifiedPair(const VerifiedLearningPair& pair);
    void appendRejectedCandidate(const RejectedCandidate& rejected);
    std::string generateId(const std::string& prefix);
};

} // namespace arenafight

