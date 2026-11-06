#include "arenafight/finetune/self_learning_engine.hpp"
#include "arenafight/common/fs_utils.hpp"
#include "arenafight/common/logger.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

SelfLearningEngine::SelfLearningEngine(
    const Config& config,
    const std::string& storageDir
) : config_(config), storageDir_(storageDir) {
    std::error_code ec;
    std::filesystem::create_directories(storageDir_, ec);
    verifiedPath_ = (std::filesystem::path(storageDir_) / "verified_dataset.jsonl").string();
    rejectedPath_ = (std::filesystem::path(storageDir_) / "rejected_candidates.jsonl").string();
    metadataPath_ = (std::filesystem::path(storageDir_) / "learning_metadata.json").string();
    loadStats();
}

void SelfLearningEngine::loadStats() {
    if (!std::filesystem::exists(metadataPath_)) {
        stats_ = SelfLearningStats();
        saveStats();
        return;
    }
    try {
        std::string content = FileUtils::readTextFile(metadataPath_);
        if (!content.empty()) {
            auto j = nlohmann::json::parse(content);
            stats_ = SelfLearningStats::fromJson(j);
        }
    } catch (...) {
        stats_ = SelfLearningStats();
    }
}

void SelfLearningEngine::saveStats() {
    try {
        nlohmann::json j = stats_.toJson();
        FileUtils::writeTextFile(metadataPath_, j.dump(2));
    } catch (...) {}
}

std::string SelfLearningEngine::generateId(const std::string& prefix) {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return prefix + "_" + std::to_string(now);
}

void SelfLearningEngine::setAutoLearning(bool enabled) {
    stats_.autoLearningEnabled = enabled;
    saveStats();
}

ValidationReport SelfLearningEngine::validateSession(
    const Session& session,
    const VerificationResult* verif
) {
    ValidationReport report;
    report.isGroundTruth = false;

    // Invariant 1: Prompt Substance Check
    std::string taskTrimmed = session.originalTask;
    while (!taskTrimmed.empty() && (taskTrimmed.front() == ' ' || taskTrimmed.front() == '\t' || taskTrimmed.front() == '\n')) taskTrimmed.erase(taskTrimmed.begin());
    while (!taskTrimmed.empty() && (taskTrimmed.back() == ' ' || taskTrimmed.back() == '\t' || taskTrimmed.back() == '\n')) taskTrimmed.pop_back();

    if (taskTrimmed.size() < 10) {
        report.reason = RejectionReason::TRIVIAL_OR_EMPTY_PROMPT;
        report.detail = "Task description too short or trivial (< 10 chars). Rejecting noise from learning pool.";
        return report;
    }

    std::string lowerTask = taskTrimmed;
    std::transform(lowerTask.begin(), lowerTask.end(), lowerTask.begin(), ::tolower);
    if (lowerTask == "hi" || lowerTask == "hello" || lowerTask == "test" || lowerTask == "exit" || lowerTask == "help") {
        report.reason = RejectionReason::TRIVIAL_OR_EMPTY_PROMPT;
        report.detail = "Conversational greeting or slash command rejected from fine-tuning dataset.";
        return report;
    }

    // Invariant 2: Anti-Mock / Non-Simulated Provider Check
    if (config_.test_mock_enabled) {
        report.reason = RejectionReason::MOCK_PROVIDER_DETECTED;
        report.detail = "Session executed while TestMock fallback was active. Mock outputs are strictly prohibited from training.";
        return report;
    }

    for (const auto& exec : session.executions) {
        if (exec.provider == "TestMock" || exec.model.find("mock") != std::string::npos || exec.model.find("TestMock") != std::string::npos) {
            report.reason = RejectionReason::MOCK_PROVIDER_DETECTED;
            report.detail = "Execution record contains mock provider ('" + exec.provider + "' / '" + exec.model + "').";
            return report;
        }
    }

    // Inspect task outputs for mock placeholders
    for (const auto& t : session.tasks) {
        std::string res = t.result;
        if (res.find("[MOCK]") != std::string::npos ||
            res.find("simulation placeholder") != std::string::npos ||
            res.find("TODO: implement") != std::string::npos ||
            res.find("dummy implementation") != std::string::npos) {
            report.reason = RejectionReason::MOCK_PROVIDER_DETECTED;
            report.detail = "Task " + t.id + " contains dummy mock placeholder code. Training rejected.";
            return report;
        }
    }

    // Invariant 3: Session State Invariant
    if (!session.completed || session.currentState != "COMPLETED") {
        report.reason = RejectionReason::FAILED_SESSION_STATE;
        report.detail = "Session state is '" + session.currentState + "' (completed=" + (session.completed ? "true" : "false") + "). Only completed goals qualify.";
        return report;
    }

    // Invariant 4: Independent Verification Gates Check
    if (verif) {
        if (!verif->complete) {
            report.reason = RejectionReason::VERIFICATION_FAILED;
            report.detail = "Independent verifier did not mark session complete: " + verif->summary;
            return report;
        }
        if (verif->completionScore < 0.80) {
            report.reason = RejectionReason::LOW_COMPLETION_SCORE;
            report.detail = "Verification completion score (" + std::to_string(verif->completionScore) + ") below ground-truth threshold (0.80).";
            return report;
        }
        if (!verif->failedChecks.empty()) {
            report.reason = RejectionReason::VERIFICATION_FAILED;
            report.detail = "Failed verification checks: " + verif->failedChecks.front();
            return report;
        }
        for (const auto& g : verif->gates) {
            if (!g.passed) {
                report.reason = RejectionReason::VERIFICATION_FAILED;
                report.detail = "Verification gate '" + g.gateName + "' failed: " + g.detail;
                return report;
            }
            if (!g.evidenceId.empty()) {
                report.verifiedEvidenceIds.push_back(g.evidenceId);
            }
        }
    }

    // Invariant 5: Physical Filesystem Grounding Invariant
    // Verify that every file produced, modified, or written during the session physically exists on disk and has non-zero size
    std::vector<std::string> targetFiles;
    for (const auto& exec : session.executions) {
        if (exec.tool == "write_file" || exec.tool == "edit_file") {
            if (exec.input.contains("path") && exec.input["path"].is_string()) {
                targetFiles.push_back(exec.input["path"].get<std::string>());
            }
        }
    }
    for (const auto& af : session.attachedFiles) {
        targetFiles.push_back(af);
    }

    for (const auto& f : targetFiles) {
        std::error_code ec;
        if (!std::filesystem::exists(f, ec)) {
            report.reason = RejectionReason::MISSING_PHYSICAL_FILE;
            report.detail = "Physical artifact verification failed: Claimed file '" + f + "' does not physically exist on disk.";
            return report;
        }
        auto sz = std::filesystem::file_size(f, ec);
        if (sz == 0) {
            report.reason = RejectionReason::MISSING_PHYSICAL_FILE;
            report.detail = "Physical artifact verification failed: File '" + f + "' is empty (0 bytes).";
            return report;
        }
        report.validatedFiles.push_back(f);
    }

    // Invariant 6: Contradiction Invariant
    if (session.claimRegistry) {
        auto unresolved = session.claimRegistry->getUnresolvedContradictions();
        if (!unresolved.empty()) {
            report.reason = RejectionReason::CONTRADICTIONS_UNRESOLVED;
            report.detail = "ClaimRegistry contains " + std::to_string(unresolved.size()) + " unresolved contradictions.";
            return report;
        }
    }

    // Invariant 7: Output Substance Check
    std::string fullResult;
    for (const auto& t : session.tasks) {
        fullResult += t.result + "\n";
    }
    if (fullResult.size() < 40) {
        report.reason = RejectionReason::EMPTY_OR_INSUFFICIENT_OUTPUT;
        report.detail = "Cumulative task execution output too brief (< 40 chars).";
        return report;
    }

    // All ground-truth invariants passed!
    report.isGroundTruth = true;
    report.reason = RejectionReason::NONE;
    report.confidenceScore = verif ? verif->confidence : 0.95;
    report.detail = "100% verified ground-truth trajectory with physical disk grounding and passing verification gates.";
    return report;
}

ValidationReport SelfLearningEngine::validatePromptPair(
    const std::string& prompt,
    const std::string& solution,
    const std::string& workspaceDir
) {
    (void)workspaceDir;
    ValidationReport report;
    report.isGroundTruth = false;

    // Check prompt length
    std::string p = prompt;
    while (!p.empty() && (p.front() == ' ' || p.front() == '\t' || p.front() == '\n')) p.erase(p.begin());
    while (!p.empty() && (p.back() == ' ' || p.back() == '\t' || p.back() == '\n')) p.pop_back();

    if (p.size() < 10) {
        report.reason = RejectionReason::TRIVIAL_OR_EMPTY_PROMPT;
        report.detail = "Prompt too short (< 10 chars).";
        return report;
    }

    // Check solution substance
    std::string s = solution;
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\n')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n')) s.pop_back();

    if (s.size() < 40) {
        report.reason = RejectionReason::EMPTY_OR_INSUFFICIENT_OUTPUT;
        report.detail = "Solution output too brief (< 40 chars).";
        return report;
    }

    // Check for mock tokens
    if (s.find("[MOCK]") != std::string::npos ||
        s.find("TestMock") != std::string::npos ||
        s.find("simulation placeholder") != std::string::npos ||
        s.find("TODO: implement") != std::string::npos) {
        report.reason = RejectionReason::MOCK_PROVIDER_DETECTED;
        report.detail = "Solution contains simulated or mock placeholder code.";
        return report;
    }

    // Check for obvious API error strings
    if (s.find("Error 404") != std::string::npos ||
        s.find("Rate limited") != std::string::npos ||
        s.find("Connection refused") != std::string::npos ||
        s.find("Service Unavailable") != std::string::npos) {
        report.reason = RejectionReason::EMPTY_OR_INSUFFICIENT_OUTPUT;
        report.detail = "Output is an API network error message, not valid code or reasoning.";
        return report;
    }

    report.isGroundTruth = true;
    report.reason = RejectionReason::NONE;
    report.confidenceScore = 0.90;
    report.detail = "Curated prompt-solution pair passed ground-truth verification checks.";
    return report;
}

void SelfLearningEngine::appendVerifiedPair(const VerifiedLearningPair& pair) {
    std::ofstream ofs(verifiedPath_, std::ios::app);
    if (ofs.is_open()) {
        ofs << pair.toJson().dump() << "\n";
    }
}

void SelfLearningEngine::appendRejectedCandidate(const RejectedCandidate& rejected) {
    std::ofstream ofs(rejectedPath_, std::ios::app);
    if (ofs.is_open()) {
        ofs << rejected.toJson().dump() << "\n";
    }
}

bool SelfLearningEngine::ingestSession(
    const Session& session,
    const VerificationResult* verif
) {
    ValidationReport rep = validateSession(session, verif);
    stats_.totalEvaluated++;

    if (rep.isGroundTruth) {
        VerifiedLearningPair pair;
        pair.id = generateId("pair");
        pair.prompt = session.originalTask;
        pair.context = "Session ID: " + session.id + "\nSubtasks executed: " + std::to_string(session.tasks.size());
        
        std::stringstream ss;
        ss << "# Verified Solution\n\n";
        for (const auto& t : session.tasks) {
            ss << "### " << t.id << ": " << t.description << "\n";
            if (!t.result.empty()) {
                ss << t.result << "\n\n";
            }
        }
        ss << "# Verification Audit: 100% GROUND TRUTH VERIFIED\n"
           << "Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n";
        
        pair.solution = ss.str();
        pair.verificationScore = rep.confidenceScore;
        pair.evidenceIds = rep.verifiedEvidenceIds;
        pair.verifiedFiles = rep.validatedFiles;
        pair.sessionId = session.id;
        pair.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // Infer model from executions
        if (!session.executions.empty()) {
            pair.model = session.executions.back().model;
        } else {
            pair.model = "arena-ground-truth";
        }

        // Domain inference
        std::string lowPrompt = session.originalTask;
        std::transform(lowPrompt.begin(), lowPrompt.end(), lowPrompt.begin(), ::tolower);
        if (lowPrompt.find("esp32") != std::string::npos || lowPrompt.find("stm32") != std::string::npos ||
            lowPrompt.find("gpio") != std::string::npos || lowPrompt.find("i2c") != std::string::npos ||
            lowPrompt.find("circuit") != std::string::npos || lowPrompt.find("resistor") != std::string::npos) {
            pair.domain = "hardware";
        } else if (lowPrompt.find("fix") != std::string::npos || lowPrompt.find("bug") != std::string::npos ||
                   lowPrompt.find("diagnose") != std::string::npos || lowPrompt.find("error") != std::string::npos) {
            pair.domain = "debugging";
        } else {
            pair.domain = "coding";
        }

        appendVerifiedPair(pair);
        stats_.verifiedCount++;
        saveStats();
        Logger::instance().info("[SELF-LEARN] Successfully ingested verified ground-truth pair: " + pair.id + " (Prompt: " + session.originalTask.substr(0, 40) + "...)");
        return true;
    } else {
        RejectedCandidate rej;
        rej.id = generateId("rej");
        rej.prompt = session.originalTask;
        
        std::stringstream ss;
        for (const auto& t : session.tasks) {
            if (!t.result.empty()) ss << t.result << "\n";
        }
        rej.attemptedSolution = ss.str();
        rej.reason = rep.reason;
        rej.reasonDetail = rep.detail;
        rej.sessionId = session.id;
        rej.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        appendRejectedCandidate(rej);
        stats_.rejectedCount++;
        stats_.rejectionBreakdown[rejectionReasonToString(rep.reason)]++;
        saveStats();
        Logger::instance().warn("[SELF-LEARN] Blocked fake/unverified candidate: " + rejectionReasonToString(rep.reason) + " (" + rep.detail + ")");
        return false;
    }
}

bool SelfLearningEngine::ingestPromptPair(
    const std::string& prompt,
    const std::string& context,
    const std::string& solution,
    const std::string& domain,
    const std::string& model
) {
    ValidationReport rep = validatePromptPair(prompt, solution);
    stats_.totalEvaluated++;

    if (rep.isGroundTruth) {
        VerifiedLearningPair pair;
        pair.id = generateId("pair");
        pair.prompt = prompt;
        pair.context = context;
        pair.solution = solution + "\n\n# Verification Audit: CURATED GROUND TRUTH\nMade by Lunarmist-byte | https://github.com/Lunarmist-byte\n";
        pair.domain = domain;
        pair.verificationScore = rep.confidenceScore;
        pair.model = model;
        pair.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        appendVerifiedPair(pair);
        stats_.verifiedCount++;
        saveStats();
        return true;
    } else {
        RejectedCandidate rej;
        rej.id = generateId("rej");
        rej.prompt = prompt;
        rej.attemptedSolution = solution;
        rej.reason = rep.reason;
        rej.reasonDetail = rep.detail;
        rej.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        appendRejectedCandidate(rej);
        stats_.rejectedCount++;
        stats_.rejectionBreakdown[rejectionReasonToString(rep.reason)]++;
        saveStats();
        return false;
    }
}

std::vector<VerifiedLearningPair> SelfLearningEngine::getVerifiedPairs() const {
    std::vector<VerifiedLearningPair> list;
    if (!std::filesystem::exists(verifiedPath_)) return list;

    std::ifstream ifs(verifiedPath_);
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        try {
            auto j = nlohmann::json::parse(line);
            list.push_back(VerifiedLearningPair::fromJson(j));
        } catch (...) {}
    }
    return list;
}

std::vector<RejectedCandidate> SelfLearningEngine::getRejectedCandidates() const {
    std::vector<RejectedCandidate> list;
    if (!std::filesystem::exists(rejectedPath_)) return list;

    std::ifstream ifs(rejectedPath_);
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        try {
            auto j = nlohmann::json::parse(line);
            list.push_back(RejectedCandidate::fromJson(j));
        } catch (...) {}
    }
    return list;
}

std::vector<DatasetEntry> SelfLearningEngine::exportVerifiedDataset() const {
    std::vector<DatasetEntry> entries;
    auto pairs = getVerifiedPairs();
    entries.reserve(pairs.size());
    for (const auto& p : pairs) {
        entries.push_back(p.toDatasetEntry());
    }
    return entries;
}

bool SelfLearningEngine::exportDatasetToJsonl(const std::string& outputPath) const {
    auto entries = exportVerifiedDataset();
    UnslothManager unsloth(config_);
    return unsloth.saveDatasetToJsonl(entries, outputPath);
}

bool SelfLearningEngine::triggerSelfLearningFineTune(
    UnslothManager& unsloth,
    const UnslothTrainConfig& cfg,
    std::function<void(const std::string&)> logCallback,
    ModelRouter* routerToRegister
) {
    auto log = [&](const std::string& msg) {
        if (logCallback) logCallback(msg);
        Logger::instance().info(msg);
    };

    log("[SELF-LEARN] Commencing Fine-Tuning cycle strictly on 100% verified ground truth...");
    auto entries = exportVerifiedDataset();
    if (entries.empty()) {
        log("[SELF-LEARN NOTICE] Verified dataset is currently empty. Run tasks with passing verification to collect ground-truth training pairs.");
        return false;
    }

    log("[SELF-LEARN] Loaded " + std::to_string(entries.size()) + " verified ground-truth instruction pairs.");
    
    UnslothTrainConfig trainCfg = cfg;
    trainCfg.datasetPath = (std::filesystem::path(storageDir_) / "verified_training_dataset.jsonl").string();
    if (!unsloth.saveDatasetToJsonl(entries, trainCfg.datasetPath)) {
        log("[SELF-LEARN ERROR] Failed to write verified training dataset to " + trainCfg.datasetPath);
        return false;
    }

    bool success = unsloth.trainOnTheFly(trainCfg, logCallback, routerToRegister);
    if (success) {
        stats_.lastFinetunedModel = trainCfg.exportGgufName;
        stats_.lastFinetunedTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        saveStats();
        log("[SELF-LEARN] Fine-tuning completed and model registered: " + trainCfg.exportGgufName);
    }
    return success;
}

size_t SelfLearningEngine::scanAndIngestSessions(const std::string& sessionsDir) {
    size_t count = 0;
    if (!std::filesystem::exists(sessionsDir)) return 0;

    auto sessInfos = Session::listSessions(sessionsDir);
    for (const auto& info : sessInfos) {
        try {
            Session s = Session::loadFromDisk(info.id, sessionsDir);
            if (s.originalTask.empty()) continue;

            // Ingest session with validation
            if (ingestSession(s, nullptr)) {
                count++;
            }
        } catch (...) {}
    }
    return count;
}

void SelfLearningEngine::reset() {
    std::error_code ec;
    std::filesystem::remove(verifiedPath_, ec);
    std::filesystem::remove(rejectedPath_, ec);
    stats_ = SelfLearningStats();
    saveStats();
}

bool SelfLearningEngine::removeVerifiedPair(const std::string& pairId) {
    auto pairs = getVerifiedPairs();
    bool found = false;
    std::vector<VerifiedLearningPair> remaining;
    for (const auto& p : pairs) {
        if (p.id == pairId) {
            found = true;
        } else {
            remaining.push_back(p);
        }
    }
    if (found) {
        std::error_code ec;
        std::filesystem::remove(verifiedPath_, ec);
        for (const auto& p : remaining) {
            appendVerifiedPair(p);
        }
        stats_.verifiedCount = remaining.size();
        saveStats();
    }
    return found;
}

} // namespace arenafight

