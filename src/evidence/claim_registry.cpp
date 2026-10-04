#include "arenafight/evidence/claim_registry.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

namespace arenafight {

namespace {

std::string getCurrentIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

int64_t getCurrentEpochMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

} // namespace

ClaimRegistry::ClaimRegistry() {}

std::string ClaimRegistry::generateObsId() {
    std::stringstream ss;
    ss << "OBS_" << std::setw(3) << std::setfill('0') << (nextObsId_++);
    return ss.str();
}

std::string ClaimRegistry::generateEvidenceId() {
    std::stringstream ss;
    ss << "E" << std::setw(3) << std::setfill('0') << (nextEvId_++);
    return ss.str();
}

std::string ClaimRegistry::generateClaimId() {
    std::stringstream ss;
    ss << "C" << std::setw(3) << std::setfill('0') << (nextClaimId_++);
    return ss.str();
}

std::string ClaimRegistry::generateConflictId() {
    std::stringstream ss;
    ss << "X" << std::setw(3) << std::setfill('0') << (nextConflictId_++);
    return ss.str();
}

std::string ClaimRegistry::recordObservation(const Observation& obs) {
    std::lock_guard<std::mutex> lock(mutex_);
    Observation stored = obs;
    if (stored.id.empty()) {
        stored.id = generateObsId();
    }
    if (stored.timestamp == 0) {
        stored.timestamp = getCurrentEpochMs();
    }
    observations_.push_back(stored);

    // Section 18: Derive authoritative Level 1/Level 2 Evidence directly from tool output
    Evidence ev;
    ev.id = generateEvidenceId();
    ev.observationId = stored.id;
    ev.source = stored.tool.empty() ? "system_tool" : stored.tool;
    ev.timestamp = getCurrentIsoTimestamp();
    ev.isStale = false;

    std::string toolLower = toLower(stored.tool);
    std::string cmdLower = toLower(stored.command);

    if (toolLower == "execute_command" || toolLower == "run_command" || toolLower == "shell") {
        ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
        ev.location = "shell: " + stored.command;

        // Check if build command
        if (cmdLower.find("cmake") != std::string::npos ||
            cmdLower.find("make") != std::string::npos ||
            cmdLower.find("cargo build") != std::string::npos ||
            cmdLower.find("ninja") != std::string::npos ||
            cmdLower.find("gcc") != std::string::npos ||
            cmdLower.find("g++") != std::string::npos ||
            cmdLower.find("clang") != std::string::npos) {
            ev.dependentArtifacts = {"build", "source_code"};
            if (stored.exitCode == 0) {
                ev.level = EvidenceLevel::VERIFIED;
                ev.confidence = 1.0;
                ev.claim = "Project compilation / build passed";
                ev.evidence = "Process exited with code 0 in " + std::to_string(stored.duration) + "ms";
            } else {
                ev.level = EvidenceLevel::CONTRADICTED;
                ev.confidence = 1.0;
                ev.claim = "Project compilation / build failed";
                ev.evidence = stored.stderrText.empty() ? stored.stdoutText : stored.stderrText;
            }
        }
        // Check if test command
        else if (cmdLower.find("ctest") != std::string::npos ||
                 cmdLower.find("pytest") != std::string::npos ||
                 cmdLower.find("cargo test") != std::string::npos ||
                 cmdLower.find("npm test") != std::string::npos ||
                 cmdLower.find("arena_test") != std::string::npos) {
            ev.dependentArtifacts = {"tests", "build", "source_code"};
            if (stored.exitCode == 0) {
                ev.level = EvidenceLevel::VERIFIED;
                ev.confidence = 1.0;
                ev.claim = "Automated test suite passed";
                ev.evidence = stored.stdoutText;
            } else {
                ev.level = EvidenceLevel::CONTRADICTED;
                ev.confidence = 1.0;
                ev.claim = "Automated test suite failed";
                ev.evidence = stored.stdoutText + "\n" + stored.stderrText;
            }
        }
        // General shell command
        else {
            if (stored.exitCode == 0) {
                ev.level = EvidenceLevel::VERIFIED;
                ev.confidence = 0.95;
                ev.claim = "Command succeeded: " + stored.command;
                ev.evidence = stored.stdoutText;
            } else {
                ev.level = EvidenceLevel::CONTRADICTED;
                ev.confidence = 0.95;
                ev.claim = "Command failed with exit code " + std::to_string(stored.exitCode);
                ev.evidence = stored.stderrText;
            }
        }
    }
    else if (toolLower == "read_file" || toolLower == "view_file") {
        ev.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;
        ev.level = EvidenceLevel::VERIFIED;
        ev.confidence = 0.95;
        ev.claim = "File inspected: " + stored.command;
        ev.location = stored.command;
        std::string snippet = stored.stdoutText;
        if (snippet.size() > 300) {
            snippet = snippet.substr(0, 300) + "... [truncated]";
        }
        ev.evidence = snippet;
        ev.dependentArtifacts = {stored.command};
    }
    else if (toolLower == "write_file" || toolLower == "edit_file") {
        ev.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;
        ev.level = EvidenceLevel::VERIFIED;
        ev.confidence = 1.0;
        ev.claim = "File modified/created: " + stored.command;
        ev.location = stored.command;
        ev.evidence = stored.stdoutText;
        ev.dependentArtifacts = {stored.command};
    }
    else if (toolLower == "list_directory" || toolLower == "search_files") {
        ev.hierarchy = EvidenceHierarchy::DIRECT_ARTIFACT;
        ev.level = EvidenceLevel::VERIFIED;
        ev.confidence = 0.90;
        ev.claim = "Workspace files inspected";
        ev.evidence = stored.stdoutText;
    }
    else {
        ev.hierarchy = EvidenceHierarchy::DIRECT_EXECUTION;
        ev.level = (stored.exitCode == 0) ? EvidenceLevel::VERIFIED : EvidenceLevel::CONTRADICTED;
        ev.confidence = 0.80;
        ev.claim = "Tool " + stored.tool + " execution";
        ev.evidence = stored.stdoutText;
    }

    evidences_.push_back(ev);
    return stored.id;
}

std::string ClaimRegistry::addEvidence(Evidence evidence) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (evidence.id.empty()) {
        evidence.id = generateEvidenceId();
    }
    if (evidence.timestamp.empty()) {
        evidence.timestamp = getCurrentIsoTimestamp();
    }

    // Default confidence from Section 10 Evidence Hierarchy
    if (evidence.confidence <= 0.0) {
        switch (evidence.hierarchy) {
            case EvidenceHierarchy::DIRECT_EXECUTION:
                evidence.confidence = (evidence.level == EvidenceLevel::CONTRADICTED) ? 1.0 : 0.99;
                break;
            case EvidenceHierarchy::DIRECT_ARTIFACT:
                evidence.confidence = 0.90;
                break;
            case EvidenceHierarchy::AUTHORITATIVE_DOCS:
                evidence.confidence = 0.85;
                break;
            case EvidenceHierarchy::INDEPENDENT_AGREEMENT:
                evidence.confidence = 0.50; // Per Section 9: Agreement is supporting, not proof!
                break;
            case EvidenceHierarchy::SINGLE_MODEL:
                evidence.confidence = 0.25;
                break;
            case EvidenceHierarchy::GUESS_ASSUMPTION:
            default:
                evidence.confidence = 0.05;
                break;
        }
    }

    evidences_.push_back(evidence);
    return evidence.id;
}

std::string ClaimRegistry::registerClaim(Claim claim) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (claim.id.empty()) {
        claim.id = generateClaimId();
    }
    if (claim.timestamp == 0) {
        claim.timestamp = getCurrentEpochMs();
    }

    // Section 5: Separate Facts From Hypotheses
    // A model asserting a fact without direct evidence CANNOT be stored as FACT!
    if (claim.type == KnowledgeType::FACT && claim.evidenceIds.empty()) {
        claim.type = KnowledgeType::HYPOTHESIS;
        claim.status = EvidenceLevel::UNVERIFIED;
        claim.verified = false;
        claim.confidence = 0.10;
    }

    evaluateClaim(claim);
    claims_.push_back(claim);

    // Run contradiction check against previously registered claims
    detectContradictions();

    return claim.id;
}

void ClaimRegistry::linkEvidenceToClaim(const std::string& claimId, const std::string& evidenceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& c : claims_) {
        if (c.id == claimId) {
            if (std::find(c.evidenceIds.begin(), c.evidenceIds.end(), evidenceId) == c.evidenceIds.end()) {
                c.evidenceIds.push_back(evidenceId);
            }
            evaluateClaim(c);
            break;
        }
    }
}

void ClaimRegistry::evaluateClaim(Claim& claim) const {
    if (claim.evidenceIds.empty()) {
        if (claim.modelAgreementCount > 1) {
            // Section 9: 5 models agree + 0 external evidence = MODEL_DERIVED, NOT verified
            claim.status = EvidenceLevel::MODEL_DERIVED;
            claim.verified = false;
            claim.confidence = std::min(0.50, 0.20 + 0.05 * claim.modelAgreementCount);
        } else {
            claim.status = EvidenceLevel::UNVERIFIED;
            claim.verified = false;
            claim.confidence = std::min(claim.confidence, 0.25);
        }
        return;
    }

    bool hasDirectExecutionEvidence = false;
    bool hasDirectExecutionContradiction = false;
    bool hasDirectArtifactEvidence = false;
    bool hasAuthoritativeDocs = false;
    int validEvidenceCount = 0;
    double maxConfidence = 0.0;

    for (const auto& eid : claim.evidenceIds) {
        const Evidence* ev = getEvidence(eid);
        if (!ev) continue;
        if (ev->isStale) continue; // Skip stale evidence (Section 21)

        validEvidenceCount++;
        maxConfidence = std::max(maxConfidence, ev->confidence);

        if (ev->hierarchy == EvidenceHierarchy::DIRECT_EXECUTION) {
            if (ev->level == EvidenceLevel::CONTRADICTED) {
                hasDirectExecutionContradiction = true;
            } else if (ev->level == EvidenceLevel::VERIFIED) {
                hasDirectExecutionEvidence = true;
            }
        } else if (ev->hierarchy == EvidenceHierarchy::DIRECT_ARTIFACT) {
            if (ev->level == EvidenceLevel::VERIFIED) {
                hasDirectArtifactEvidence = true;
            }
        } else if (ev->hierarchy == EvidenceHierarchy::AUTHORITATIVE_DOCS) {
            hasAuthoritativeDocs = true;
        }
    }

    // Section 10: Hierarchy evaluation
    if (hasDirectExecutionContradiction) {
        claim.status = EvidenceLevel::CONTRADICTED;
        claim.verified = false;
        claim.confidence = 0.99; // Highly confident it is contradicted
    } else if (hasDirectExecutionEvidence) {
        claim.status = EvidenceLevel::VERIFIED;
        claim.verified = true;
        claim.confidence = 0.98;
    } else if (hasDirectArtifactEvidence) {
        claim.status = EvidenceLevel::VERIFIED;
        claim.verified = true;
        claim.confidence = 0.90;
    } else if (hasAuthoritativeDocs) {
        claim.status = EvidenceLevel::STRONGLY_SUPPORTED;
        claim.verified = false;
        claim.confidence = 0.85;
    } else if (claim.modelAgreementCount > 1) {
        // Section 9: 5 models agreeing with 0 external evidence is MODEL_DERIVED, NOT verified!
        claim.status = EvidenceLevel::MODEL_DERIVED;
        claim.verified = false;
        claim.confidence = std::min(0.50, 0.20 + 0.05 * claim.modelAgreementCount);
    } else if (validEvidenceCount > 0) {
        claim.status = EvidenceLevel::MODEL_DERIVED;
        claim.verified = false;
        claim.confidence = std::min(0.35, maxConfidence);
    } else {
        claim.status = EvidenceLevel::UNVERIFIED;
        claim.verified = false;
        claim.confidence = 0.10;
    }
}

std::vector<Contradiction> ClaimRegistry::detectContradictions() {
    std::vector<Contradiction> newConflicts;

    // Check pairwise claims for contradictions
    for (size_t i = 0; i < claims_.size(); ++i) {
        for (size_t j = i + 1; j < claims_.size(); ++j) {
            Claim& c1 = claims_[i];
            Claim& c2 = claims_[j];

            // If targeting the same artifact or specific bug with different assertions
            bool targetOverlap = (!c1.targetArtifact.empty() && !c2.targetArtifact.empty() &&
                                  c1.targetArtifact != c2.targetArtifact);
            std::string s1 = toLower(c1.statement);
            std::string s2 = toLower(c2.statement);

            bool isBugLocationConflict = (s1.find("bug is in") != std::string::npos &&
                                          s2.find("bug is in") != std::string::npos &&
                                          targetOverlap);

            bool isPassVsFail = ((s1.find("compil") != std::string::npos && s1.find("pass") != std::string::npos &&
                                  s2.find("compil") != std::string::npos && s2.find("fail") != std::string::npos) ||
                                 (s1.find("test") != std::string::npos && s1.find("pass") != std::string::npos &&
                                  s2.find("test") != std::string::npos && s2.find("fail") != std::string::npos));

            if (isBugLocationConflict || isPassVsFail) {
                // Check if already registered
                bool alreadyTracked = false;
                for (const auto& ex : contradictions_) {
                    if ((ex.claimIdA == c1.id && ex.claimIdB == c2.id) ||
                        (ex.claimIdA == c2.id && ex.claimIdB == c1.id)) {
                        alreadyTracked = true;
                        break;
                    }
                }

                if (!alreadyTracked) {
                    Contradiction conflict;
                    conflict.id = generateConflictId();
                    conflict.claimIdA = c1.id;
                    conflict.claimIdB = c2.id;
                    conflict.statementA = c1.statement;
                    conflict.statementB = c2.statement;
                    conflict.description = "Contradiction detected between [" + c1.id + "] and [" + c2.id + "]";
                    // Section 8: Required action
                    conflict.requiredAction = "Inspect source + reproduce failure.";
                    conflict.resolved = false;

                    contradictions_.push_back(conflict);
                    newConflicts.push_back(conflict);

                    if (!c1.verified) c1.status = EvidenceLevel::CONTRADICTED;
                    if (!c2.verified) c2.status = EvidenceLevel::CONTRADICTED;
                }
            }
        }
    }

    return newConflicts;
}

bool ClaimRegistry::resolveContradiction(
    const std::string& contradictionId,
    const std::string& resolvingEvidenceId,
    const std::string& notes
) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& c : contradictions_) {
        if (c.id == contradictionId) {
            c.resolved = true;
            c.resolutionEvidenceId = resolvingEvidenceId;
            c.resolutionNotes = notes;

            // Update associated claims
            for (auto& clm : claims_) {
                if (clm.id == c.claimIdA || clm.id == c.claimIdB) {
                    evaluateClaim(clm);
                }
            }
            return true;
        }
    }
    return false;
}

int ClaimRegistry::invalidateArtifactEvidence(const std::string& modifiedFile) {
    std::lock_guard<std::mutex> lock(mutex_);
    int invalidatedCount = 0;
    std::string modLower = toLower(modifiedFile);

    // Section 21 & 22: Dependency-Aware Evidence Invalidation
    for (auto& ev : evidences_) {
        if (ev.isStale) continue;

        bool dependsOnModified = false;
        for (const auto& dep : ev.dependentArtifacts) {
            std::string depLower = toLower(dep);
            if (depLower == modLower ||
                depLower.find(modLower) != std::string::npos ||
                modLower.find(depLower) != std::string::npos ||
                depLower == "source_code" ||
                depLower == "build") {
                dependsOnModified = true;
                break;
            }
        }

        if (dependsOnModified) {
            ev.isStale = true;
            invalidatedCount++;
        }
    }

    // Downgrade any claims that depended on now-stale evidence
    for (auto& clm : claims_) {
        evaluateClaim(clm);
    }

    return invalidatedCount;
}

std::string ClaimRegistry::recordModelAssertion(
    const std::string& statement,
    const std::string& modelName,
    KnowledgeType type,
    const std::string& targetArtifact
) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Check if statement already claimed
    for (auto& c : claims_) {
        if (toLower(c.statement) == toLower(statement)) {
            c.modelAgreementCount++;
            c.totalModelsQueried++;
            // Section 9: Model agreement increases confidence up to MODEL_DERIVED, NOT proof
            evaluateClaim(c);
            return c.id;
        }
    }

    // New assertion: cannot be FACT without proof
    Claim c;
    c.id = generateClaimId();
    c.statement = statement;
    c.type = (type == KnowledgeType::FACT) ? KnowledgeType::HYPOTHESIS : type;
    c.sourceModel = modelName;
    c.targetArtifact = targetArtifact;
    c.status = EvidenceLevel::UNVERIFIED;
    c.confidence = 0.20;
    c.requiresVerification = true;
    c.verified = false;
    c.modelAgreementCount = 1;
    c.totalModelsQueried = 1;
    c.timestamp = getCurrentEpochMs();

    claims_.push_back(c);
    return c.id;
}

std::vector<Evidence> ClaimRegistry::getTraceableEvidenceForClaim(const std::string& claimId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Evidence> res;
    const Claim* c = getClaim(claimId);
    if (!c) return res;

    for (const auto& eid : c->evidenceIds) {
        const Evidence* ev = getEvidence(eid);
        if (ev) res.push_back(*ev);
    }
    return res;
}

bool ClaimRegistry::promoteHypothesisToFact(const std::string& claimId, const std::string& evidenceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const Evidence* ev = getEvidence(evidenceId);
    if (!ev || ev->isStale) return false;

    // Must be supported by level 1 or 2 evidence
    if (ev->hierarchy != EvidenceHierarchy::DIRECT_EXECUTION &&
        ev->hierarchy != EvidenceHierarchy::DIRECT_ARTIFACT) {
        return false;
    }
    if (ev->level != EvidenceLevel::VERIFIED) return false;

    for (auto& c : claims_) {
        if (c.id == claimId) {
            c.type = KnowledgeType::FACT;
            c.status = EvidenceLevel::VERIFIED;
            c.verified = true;
            c.confidence = ev->confidence;
            if (std::find(c.evidenceIds.begin(), c.evidenceIds.end(), evidenceId) == c.evidenceIds.end()) {
                c.evidenceIds.push_back(evidenceId);
            }
            return true;
        }
    }
    return false;
}

const Claim* ClaimRegistry::getClaim(const std::string& claimId) const {
    for (const auto& c : claims_) {
        if (c.id == claimId) return &c;
    }
    return nullptr;
}

const Evidence* ClaimRegistry::getEvidence(const std::string& evidenceId) const {
    for (const auto& ev : evidences_) {
        if (ev.id == evidenceId) return &ev;
    }
    return nullptr;
}

const Observation* ClaimRegistry::getObservation(const std::string& obsId) const {
    for (const auto& obs : observations_) {
        if (obs.id == obsId) return &obs;
    }
    return nullptr;
}

std::vector<Contradiction> ClaimRegistry::getUnresolvedContradictions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Contradiction> res;
    for (const auto& c : contradictions_) {
        if (!c.resolved) res.push_back(c);
    }
    return res;
}

std::vector<Claim> ClaimRegistry::getVerifiedClaims() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Claim> res;
    for (const auto& c : claims_) {
        if (c.verified) res.push_back(c);
    }
    return res;
}

std::vector<Claim> ClaimRegistry::getUnverifiedClaims() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Claim> res;
    for (const auto& c : claims_) {
        if (!c.verified && c.status != EvidenceLevel::CONTRADICTED) res.push_back(c);
    }
    return res;
}

std::vector<Claim> ClaimRegistry::getContradictedClaims() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Claim> res;
    for (const auto& c : claims_) {
        if (c.status == EvidenceLevel::CONTRADICTED) res.push_back(c);
    }
    return res;
}

std::string ClaimRegistry::getEvidenceReportSummary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << "EVIDENCE REPORT (" << evidences_.size() << " items recorded):\n";
    for (const auto& ev : evidences_) {
        ss << "- [" << ev.id << "] " << evidenceLevelToString(ev.level)
           << " (" << evidenceHierarchyToString(ev.hierarchy) << ")\n"
           << "  Claim: " << ev.claim << "\n"
           << "  Source: " << ev.source << " | Confidence: " << std::fixed << std::setprecision(2) << ev.confidence << "\n";
        if (ev.isStale) ss << "  Status: STALE (requires fresh re-verification)\n";
    }
    return ss.str();
}

nlohmann::json ClaimRegistry::toJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json obsArr = nlohmann::json::array();
    for (const auto& o : observations_) obsArr.push_back(o.toJson());

    nlohmann::json evArr = nlohmann::json::array();
    for (const auto& e : evidences_) evArr.push_back(e.toJson());

    nlohmann::json clmArr = nlohmann::json::array();
    for (const auto& c : claims_) clmArr.push_back(c.toJson());

    nlohmann::json conArr = nlohmann::json::array();
    for (const auto& x : contradictions_) conArr.push_back(x.toJson());

    return {
        {"observations", obsArr},
        {"evidences", evArr},
        {"claims", clmArr},
        {"contradictions", conArr},
        {"nextObsId", nextObsId_},
        {"nextEvId", nextEvId_},
        {"nextClaimId", nextClaimId_},
        {"nextConflictId", nextConflictId_}
    };
}

void ClaimRegistry::fromJson(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    observations_.clear();
    evidences_.clear();
    claims_.clear();
    contradictions_.clear();

    if (j.contains("observations") && j["observations"].is_array()) {
        for (const auto& o : j["observations"]) observations_.push_back(Observation::fromJson(o));
    }
    if (j.contains("evidences") && j["evidences"].is_array()) {
        for (const auto& e : j["evidences"]) evidences_.push_back(Evidence::fromJson(e));
    }
    if (j.contains("claims") && j["claims"].is_array()) {
        for (const auto& c : j["claims"]) claims_.push_back(Claim::fromJson(c));
    }
    if (j.contains("contradictions") && j["contradictions"].is_array()) {
        for (const auto& x : j["contradictions"]) contradictions_.push_back(Contradiction::fromJson(x));
    }

    nextObsId_ = j.value("nextObsId", static_cast<int>(observations_.size() + 1));
    nextEvId_ = j.value("nextEvId", static_cast<int>(evidences_.size() + 1));
    nextClaimId_ = j.value("nextClaimId", static_cast<int>(claims_.size() + 1));
    nextConflictId_ = j.value("nextConflictId", static_cast<int>(contradictions_.size() + 1));
}

void ClaimRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    observations_.clear();
    evidences_.clear();
    claims_.clear();
    contradictions_.clear();
    nextObsId_ = 1;
    nextEvId_ = 1;
    nextClaimId_ = 1;
    nextConflictId_ = 1;
}

} // namespace arenafight
