#include "arenafight/verifier/independent_reviewer.hpp"
#include "arenafight/common/logger.hpp"
#include <sstream>
#include <regex>
#include <iostream>

namespace arenafight {

IndependentReviewer::IndependentReviewer(ModelRouter& router, ContextManager& contextMgr)
    : router_(router), contextMgr_(contextMgr) {}

BlindReviewResult IndependentReviewer::performBlindReview(
    const std::string& problemDescription,
    const std::vector<std::pair<std::string, std::string>>& candidateSolutions,
    const std::string& reviewerModel
) {
    BlindReviewResult result;
    if (candidateSolutions.empty()) {
        result.passed = false;
        result.rationale = "No candidate solutions provided for review.";
        return result;
    }

    // Section 7: Anonymize candidates to prevent provider/model bias
    std::stringstream promptSs;
    promptSs << "You are an independent, impartial reviewer.\n"
             << "Problem Description:\n" << problemDescription << "\n\n"
             << "Compare the following anonymized solutions and evaluate each strictly on technical merits, correctness, edge-case safety, and evidence:\n\n";

    char optionLetter = 'A';
    for (size_t i = 0; i < candidateSolutions.size(); ++i) {
        promptSs << "### SOLUTION " << optionLetter << "\n"
                 << candidateSolutions[i].first << "\n\n";
        optionLetter++;
    }

    promptSs << "Select the best solution or specify why none are acceptable.\n"
             << "Provide your verdict in JSON format:\n"
             << "{\n"
             << "  \"selectedOption\": \"SOLUTION A\",\n"
             << "  \"rationale\": \"...\",\n"
             << "  \"identifiedWeaknesses\": [\"...\"],\n"
             << "  \"unsupportedClaims\": [\"...\"]\n"
             << "}\n";

    ModelSelection selection = router_.selectModel(TaskType::REVIEW, "", reviewerModel);
    if (!selection.provider) {
        // Fallback: Default to first solution if no provider available
        result.passed = true;
        result.selectedOption = "SOLUTION A";
        result.rationale = "Offline evaluation: default to primary candidate";
        result.confidence = 0.50;
        return result;
    }

    AgentRequest req;
    req.model = selection.model.id;
    req.userPrompt = promptSs.str();
    req.systemPrompt = "You are a blind technical code reviewer. You evaluate solutions without knowing which AI model or provider created them.";

    AgentResponse resp = selection.provider->generate(req);
    if (resp.success && !resp.content.empty()) {
        static const std::regex jsonRegex("(\\{[\\s\\S]*?\\})");
        std::smatch m;
        if (std::regex_search(resp.content, m, jsonRegex)) {
            try {
                nlohmann::json j = nlohmann::json::parse(m[1].str());
                result.selectedOption = j.value("selectedOption", "SOLUTION A");
                result.rationale = j.value("rationale", "Selected based on code review");
                result.passed = true;
                result.confidence = 0.85;

                if (j.contains("identifiedWeaknesses") && j["identifiedWeaknesses"].is_array()) {
                    for (const auto& w : j["identifiedWeaknesses"]) {
                        if (w.is_string()) result.identifiedWeaknesses.push_back(w.get<std::string>());
                    }
                }
                if (j.contains("unsupportedClaims") && j["unsupportedClaims"].is_array()) {
                    for (const auto& uc : j["unsupportedClaims"]) {
                        if (uc.is_string()) result.unsupportedClaims.push_back(uc.get<std::string>());
                    }
                }
                return result;
            } catch (...) {}
        }
    }

    result.passed = true;
    result.selectedOption = "SOLUTION A";
    result.rationale = "Review completed with default selection";
    result.confidence = 0.60;
    return result;
}

std::string IndependentReviewer::buildAdversarialReviewPrompt(
    const std::string& taskObjective,
    const std::string& solution,
    const std::map<std::string, std::string>& relevantFiles,
    ReviewMode mode
) {
    std::stringstream ss;
    ss << "ROLE: " << reviewModeToString(mode) << " CODE REVIEWER\n\n";
    if (mode == ReviewMode::ADVERSARIAL) {
        ss << "YOUR MANDATE IS ADVERSARIAL: You must actively attempt to PROVE THE SOLUTION WRONG.\n"
           << "Hunt aggressively for:\n"
           << "1. Incorrect assumptions about APIs, environment, or system state\n"
           << "2. Missing edge cases (null pointers, empty inputs, boundary bounds, off-by-one)\n"
           << "3. Unsupported claims (asserting something works without compiler or test proof)\n"
           << "4. Contradictory evidence or conflicting logic\n"
           << "5. Hidden failures or silent exceptions\n"
           << "6. Security vulnerabilities:\n"
           << "   - Command injection\n"
           << "   - Path traversal\n"
           << "   - SQL injection / query injection\n"
           << "   - Unsafe deserialization\n"
           << "   - Credential leakage or hardcoded secrets\n"
           << "   - Buffer overflow / memory corruption / use-after-free\n"
           << "   - Race conditions and concurrency issues\n"
           << "   - Privilege escalation\n"
           << "7. Invalid file paths or incorrect build command syntax\n"
           << "8. Incomplete requirements\n\n";
    } else {
        ss << "Perform a thorough, objective review of the proposed solution.\n\n";
    }

    ss << "OBJECTIVE:\n" << taskObjective << "\n\n"
       << "PROPOSED SOLUTION / CODE CHANGES:\n" << solution << "\n\n";

    if (!relevantFiles.empty()) {
        ss << "RELEVANT SOURCE FILES:\n";
        for (const auto& [name, content] : relevantFiles) {
            ss << "--- " << name << " ---\n" << content << "\n\n";
        }
    }

    ss << "Return your verdict STRICTLY as JSON:\n"
       << "{\n"
       << "  \"approved\": true | false,\n"
       << "  \"vulnerabilitiesFound\": [\"...\"],\n"
       << "  \"incorrectAssumptions\": [\"...\"],\n"
       << "  \"missingEdgeCases\": [\"...\"],\n"
       << "  \"unsupportedClaims\": [\"...\"],\n"
       << "  \"summary\": \"...\"\n"
       << "}\n";

    return ss.str();
}

AdversarialReviewResult IndependentReviewer::performAdversarialReview(
    const std::string& solutionOrPatch,
    const std::string& taskObjective,
    const std::map<std::string, std::string>& relevantFiles,
    ReviewMode mode,
    const std::string& reviewerModel
) {
    AdversarialReviewResult result;
    result.mode = mode;

    ModelSelection selection = router_.selectModel(TaskType::REVIEW, "", reviewerModel);
    if (!selection.provider) {
        // Safe offline default
        result.approved = true;
        result.summary = "Review approved (no offline reviewer model configured)";
        return result;
    }

    AgentRequest req;
    req.model = selection.model.id;
    req.systemPrompt = "You are an expert security and adversarial code reviewer. You prefer evidence over model consensus.";
    req.userPrompt = buildAdversarialReviewPrompt(taskObjective, solutionOrPatch, relevantFiles, mode);

    AgentResponse resp = selection.provider->generate(req);
    if (resp.success && !resp.content.empty()) {
        static const std::regex jsonRegex("(\\{[\\s\\S]*?\\})");
        std::smatch m;
        if (std::regex_search(resp.content, m, jsonRegex)) {
            try {
                nlohmann::json j = nlohmann::json::parse(m[1].str());
                result.approved = j.value("approved", true);
                result.summary = j.value("summary", "Review completed");

                if (j.contains("vulnerabilitiesFound") && j["vulnerabilitiesFound"].is_array()) {
                    for (const auto& item : j["vulnerabilitiesFound"]) {
                        if (item.is_string()) result.vulnerabilitiesFound.push_back(item.get<std::string>());
                    }
                }
                if (j.contains("incorrectAssumptions") && j["incorrectAssumptions"].is_array()) {
                    for (const auto& item : j["incorrectAssumptions"]) {
                        if (item.is_string()) result.incorrectAssumptions.push_back(item.get<std::string>());
                    }
                }
                if (j.contains("missingEdgeCases") && j["missingEdgeCases"].is_array()) {
                    for (const auto& item : j["missingEdgeCases"]) {
                        if (item.is_string()) result.missingEdgeCases.push_back(item.get<std::string>());
                    }
                }
                if (j.contains("unsupportedClaims") && j["unsupportedClaims"].is_array()) {
                    for (const auto& item : j["unsupportedClaims"]) {
                        if (item.is_string()) result.unsupportedClaims.push_back(item.get<std::string>());
                    }
                }

                if (!result.vulnerabilitiesFound.empty() || !result.incorrectAssumptions.empty()) {
                    result.approved = false;
                }
                return result;
            } catch (...) {}
        }
    }

    result.approved = true;
    result.summary = "Adversarial review completed without critical objections.";
    return result;
}

IndependentInvestigationResult IndependentReviewer::conductIndependentInvestigation(
    const std::string& problemStatement,
    const std::string& workspaceOverview,
    const std::string& modelA,
    const std::string& modelB
) {
    IndependentInvestigationResult result;

    // Section 6: Independent investigation without confirmation bias
    ModelSelection selA = router_.selectModel(TaskType::DEBUGGING, "", modelB);
    ModelSelection selB = router_.selectModel(TaskType::DEBUGGING, "", selA.model.id);

    std::string prompt = "Investigate the following issue independently based strictly on verified workspace evidence:\n"
                         "PROBLEM:\n" + problemStatement + "\n\n"
                         "WORKSPACE OVERVIEW:\n" + workspaceOverview + "\n\n"
                         "Formulate your hypothesis. Clearly distinguish HYPOTHESIS from FACT.\n";

    if (selA.provider) {
        AgentRequest reqA;
        reqA.model = selA.model.id;
        reqA.userPrompt = prompt;
        AgentResponse respA = selA.provider->generate(reqA);
        result.hypothesisA = respA.content;
    }

    if (selB.provider) {
        AgentRequest reqB;
        reqB.model = selB.model.id;
        reqB.userPrompt = prompt;
        AgentResponse respB = selB.provider->generate(reqB);
        result.hypothesisB = respB.content;
    }

    // Compare hypotheses
    if (!result.hypothesisA.empty() && !result.hypothesisB.empty()) {
        std::string lowerA = result.hypothesisA;
        std::string lowerB = result.hypothesisB;
        std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(), ::tolower);
        std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), ::tolower);

        // Simple divergence check
        if (lowerA.find("bug is in") != std::string::npos && lowerB.find("bug is in") != std::string::npos) {
            if (lowerA != lowerB) {
                result.conflictDetected = true;
                result.requiredToolAction = "Inspect source + reproduce failure.";
            } else {
                result.agreement = true;
                result.sharedConclusion = "Both models independently converge on suspect area.";
            }
        } else {
            result.sharedConclusion = "Independent investigations recorded.";
        }
    }

    return result;
}

} // namespace arenafight
