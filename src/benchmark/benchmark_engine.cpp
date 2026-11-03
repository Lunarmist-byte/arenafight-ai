#include "arenafight/benchmark/benchmark_engine.hpp"
#include "arenafight/common/logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

BenchmarkEngine::BenchmarkEngine(ModelRouter& router, const Config& config)
    : router_(router), config_(config) {}

std::vector<BenchmarkTask> BenchmarkEngine::getStandardTasks() {
    return {
        {
            "bench_reasoning",
            "Multi-Step Algorithmic Reasoning & Complexity Planning",
            "Reasoning",
            "BENCHMARK TASK 1: REASONING & ALGORITHMIC COMPLEXITY.\n"
            "Analyze the time and space complexity of finding the longest valid balanced parenthesis substring "
            "in O(N) time and O(1) auxiliary space without using a stack. Break down the state transitions and edge cases.",
            384,
            0.1f,
            {"o(n)", "o(1)", "left", "right", "edge", "boundary"},
            {}
        },
        {
            "bench_coding",
            "C++20 Zero-Heap Lock-Free Concurrent Ring Buffer",
            "Coding",
            "BENCHMARK TASK 2: C++20 CODE ARCHITECTURE & SAFETY INVARIANTS.\n"
            "Design a high-throughput, non-blocking lock-free circular ring buffer in C++20 with cache-line padding, "
            "memory ordering, and zero dynamic heap allocation.",
            512,
            0.1f,
            {"atomic", "memory_order", "alignas", "head", "tail", "noexcept", "template"},
            {}
        },
        {
            "bench_verification",
            "Adversarial Code Audit & Vulnerability Verification",
            "Verification",
            "BENCHMARK TASK 3: INDEPENDENT VERIFICATION & SECURITY AUDIT.\n"
            "Audit this function for security vulnerabilities, concurrency hazards, and compiler UB: "
            "void processPacket(char* buf, size_t len) { char temp[128]; strcpy(temp, buf); int* p = nullptr; if (len > 0) *p = 42; free(buf); free(buf); }",
            384,
            0.1f,
            {"overflow", "strcpy", "null", "double free", "ub"},
            {}
        },
        {
            "bench_throughput",
            "Embedded QoS Real-Time Execution & Latency Telemetry",
            "Throughput",
            "BENCHMARK TASK 4: HARD REAL-TIME EMBEDDED QOS & LATENCY.\n"
            "Provide a concise 3-point checklist for ensuring zero-heap hard real-time QoS execution on ARM Cortex-M microcontrollers.",
            256,
            0.1f,
            {"heap", "interrupt", "deterministic"},
            {}
        }
    };
}

std::string BenchmarkEngine::calculateGrade(float compositeScore) {
    if (compositeScore >= 95.0f) return "S+ (Mastery)";
    if (compositeScore >= 90.0f) return "S (Exceptional)";
    if (compositeScore >= 80.0f) return "A (Production)";
    if (compositeScore >= 70.0f) return "B (Competent)";
    if (compositeScore >= 50.0f) return "C (Baseline)";
    return "F (Unsatisfactory)";
}

float BenchmarkEngine::evaluateTaskResponse(const BenchmarkTask& task, const AgentResponse& response, const std::string& modelId) {
    if (!response.success) return 0.0f;

    std::string text = response.content;
    if (!response.thinking.empty()) {
        text += "\n" + response.thinking;
    }
    std::string lowerText = text;
    for (char& c : lowerText) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    int matches = 0;
    for (const auto& kw : task.requiredKeywords) {
        if (lowerText.find(kw) != std::string::npos) {
            matches++;
        }
    }

    float keywordRatio = task.requiredKeywords.empty() ? 1.0f : (static_cast<float>(matches) / static_cast<float>(task.requiredKeywords.size()));

    // Ability prior bonus based on inferred capability
    ModelAbilities abilities = ModelAbilities::infer(modelId);
    float traitBonus = 0.0f;
    if (task.category == "Reasoning") traitBonus = abilities.reasoning;
    else if (task.category == "Coding") traitBonus = abilities.coding;
    else if (task.category == "Verification") traitBonus = abilities.verification;
    else traitBonus = abilities.speed;

    // Minimum response content check
    float lengthBonus = std::min(1.0f, static_cast<float>(text.length()) / 120.0f);

    float rawScore = (keywordRatio * 0.65f + traitBonus * 0.25f + lengthBonus * 0.10f) * 100.0f;
    return std::min(100.0f, std::max(10.0f, rawScore));
}

ModelBenchmarkResult BenchmarkEngine::benchmarkSingleModel(const ModelInfo& model, std::shared_ptr<ModelProvider> provider, bool verbose) {
    ModelBenchmarkResult result;
    result.modelId = model.id;
    result.providerName = provider ? provider->name() : "Unassigned";

    if (!provider || !provider->available()) {
        result.online = false;
        result.grade = "OFFLINE";
        result.summary = "Backend provider is offline or unreachable.";
        if (verbose) {
            std::cout << "  [BENCHMARK] " << std::left << std::setw(28) << model.id << " -> OFFLINE (Skipped)\n";
        }
        return result;
    }

    result.online = true;
    auto tasks = getStandardTasks();
    float totalReasoning = 0.0f;
    float totalCoding = 0.0f;
    float totalVerification = 0.0f;
    float totalSpeed = 0.0f;
    int totalLatency = 0;
    int totalTokens = 0;

    if (verbose) {
        std::cout << "  [BENCHMARK] Testing: " << model.id << " (" << provider->name() << ")... " << std::flush;
    }

    for (const auto& task : tasks) {
        AgentRequest req;
        req.model = model.id;
        req.userPrompt = task.prompt;
        req.maxTokens = task.maxTokens;
        req.temperature = task.temperature;

        auto tStart = std::chrono::high_resolution_clock::now();
        AgentResponse resp = provider->generate(req);
        auto tEnd = std::chrono::high_resolution_clock::now();

        int durationMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count());
        if (durationMs <= 0) durationMs = 15; // Normalized minimal measurement

        totalLatency += durationMs;
        int producedTokens = resp.completionTokens > 0 ? resp.completionTokens : 60;
        totalTokens += producedTokens;

        float score = evaluateTaskResponse(task, resp, model.id);
        std::string summaryItem = task.category + ": " + std::to_string(static_cast<int>(score)) + "% (" + std::to_string(durationMs) + "ms)";
        result.testSummaries.push_back(summaryItem);

        if (task.category == "Reasoning") totalReasoning += score;
        else if (task.category == "Coding") totalCoding += score;
        else if (task.category == "Verification") totalVerification += score;
        else if (task.category == "Throughput") totalSpeed += score;
    }

    result.latencyMs = totalLatency;
    result.totalTokens = totalTokens;
    float latencySec = static_cast<float>(totalLatency) / 1000.0f;
    result.tokensPerSec = latencySec > 0.001f ? (static_cast<float>(totalTokens) / latencySec) : 0.0f;

    result.reasoningScore = totalReasoning;
    result.codingScore = totalCoding;
    result.verificationScore = totalVerification;

    // Speed scoring based on throughput
    float normalizedSpeed = std::min(100.0f, std::max(20.0f, result.tokensPerSec * 1.2f));
    if (normalizedSpeed < 50.0f && totalSpeed > 0.0f) normalizedSpeed = totalSpeed;
    result.speedScore = normalizedSpeed;

    // Composite weighted score
    result.compositeScore =
        (result.reasoningScore * 0.35f) +
        (result.codingScore * 0.35f) +
        (result.verificationScore * 0.20f) +
        (result.speedScore * 0.10f);

    result.grade = calculateGrade(result.compositeScore);

    // Primary strength
    float bestVal = result.reasoningScore;
    result.primaryStrength = "Reasoning";
    if (result.codingScore > bestVal) { bestVal = result.codingScore; result.primaryStrength = "Coding"; }
    if (result.verificationScore > bestVal) { bestVal = result.verificationScore; result.primaryStrength = "Verification"; }
    if (result.speedScore > bestVal) { bestVal = result.speedScore; result.primaryStrength = "Speed"; }

    std::stringstream ss;
    ss << "Score: " << std::fixed << std::setprecision(1) << result.compositeScore << "% | Grade: "
       << result.grade << " | " << std::fixed << std::setprecision(1) << result.tokensPerSec << " tok/s (" << totalLatency << "ms)";
    result.summary = ss.str();

    if (verbose) {
        std::cout << result.grade << " (" << std::fixed << std::setprecision(1) << result.compositeScore << "% | "
                  << std::fixed << std::setprecision(1) << result.tokensPerSec << " tps)\n";
    }

    return result;
}

BenchmarkSuiteResult BenchmarkEngine::benchmarkAll(bool verbose) {
    auto models = router_.getAllAvailableModels();
    std::vector<std::string> names;
    for (const auto& m : models) {
        names.push_back(m.id);
    }
    return benchmarkModels(names, verbose);
}

BenchmarkSuiteResult BenchmarkEngine::benchmarkModels(const std::vector<std::string>& modelNames, bool verbose) {
    BenchmarkSuiteResult suite;
    suite.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (verbose) {
        std::cout << "\n====================================================\n"
                  << "          ARENAFIGHT MODEL BENCHMARK SUITE          \n"
                  << "   Autonomous Multi-Dimensional Evaluation (C++20)  \n"
                  << "----------------------------------------------------\n"
                  << " Made by: Lunarmist-byte\n"
                  << " GitHub:   https://github.com/Lunarmist-byte\n"
                  << " LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
                  << "====================================================\n\n";
    }

    std::vector<std::string> candidates = modelNames;
    if (candidates.empty()) {
        auto avail = router_.getAllAvailableModels();
        for (const auto& m : avail) candidates.push_back(m.id);
    }

    // Ensure fallback models exist if external backends are unpopulated
    if (candidates.empty()) {
        candidates = {"mock-reasoner", "mock-coder", "mock-verifier"};
    }

    suite.totalModelsTested = static_cast<int>(candidates.size());

    for (const auto& name : candidates) {
        ModelSelection sel = router_.getOrRegisterDynamicModel(name);
        ModelBenchmarkResult res = benchmarkSingleModel(sel.model, sel.provider, verbose);
        if (res.online) {
            suite.onlineModels++;
        }
        suite.leaderboard.push_back(res);
    }

    // Sort leaderboard by compositeScore descending (online models first)
    std::sort(suite.leaderboard.begin(), suite.leaderboard.end(), [](const ModelBenchmarkResult& a, const ModelBenchmarkResult& b) {
        if (a.online != b.online) return a.online > b.online;
        return a.compositeScore > b.compositeScore;
    });

    // Assign rank positions
    int currentRank = 1;
    for (auto& r : suite.leaderboard) {
        if (r.online) {
            r.rank = currentRank++;
        } else {
            r.rank = 999;
        }
    }

    // Determine category champions
    float highestComposite = -1.0f;
    float highestSpeed = -1.0f;
    float highestReasoning = -1.0f;
    float highestCoding = -1.0f;
    float highestVerification = -1.0f;

    for (const auto& r : suite.leaderboard) {
        if (!r.online) continue;
        if (r.compositeScore > highestComposite) { highestComposite = r.compositeScore; suite.topPerformerModel = r.modelId; }
        if (r.tokensPerSec > highestSpeed) { highestSpeed = r.tokensPerSec; suite.fastestModel = r.modelId; }
        if (r.reasoningScore > highestReasoning) { highestReasoning = r.reasoningScore; suite.bestReasoningModel = r.modelId; }
        if (r.codingScore > highestCoding) { highestCoding = r.codingScore; suite.bestCodingModel = r.modelId; }
        if (r.verificationScore > highestVerification) { highestVerification = r.verificationScore; suite.bestVerificationModel = r.modelId; }
    }

    suite.formattedReport = formatLeaderboardReport(suite);

    if (verbose) {
        std::cout << "\n" << suite.formattedReport << "\n";
    }

    // Automatically persist latest benchmark result
    saveBenchmarkResult(suite);
    return suite;
}

std::string BenchmarkEngine::formatLeaderboardReport(const BenchmarkSuiteResult& suite) {
    std::stringstream ss;
    ss << "================================================================================================\n"
       << "                              ARENAFIGHT MODEL BENCHMARK LEADERBOARD                            \n"
       << "================================================================================================\n"
       << std::left
       << std::setw(6)  << "Rank"
       << std::setw(28) << "Model ID"
       << std::setw(14) << "Provider"
       << std::setw(10) << "Reasoning"
       << std::setw(10) << "Coding"
       << std::setw(10) << "Verif"
       << std::setw(10) << "Latency"
       << std::setw(10) << "TPS"
       << std::setw(9)  << "Score"
       << "Grade\n"
       << "------+----------------------------+--------------+---------+---------+---------+---------+---------+--------+-----------------\n";

    for (const auto& r : suite.leaderboard) {
        if (!r.online) {
            ss << std::left
               << std::setw(6)  << "--"
               << std::setw(28) << (r.modelId.length() > 27 ? r.modelId.substr(0, 24) + "..." : r.modelId)
               << std::setw(14) << r.providerName
               << std::setw(10) << "--"
               << std::setw(10) << "--"
               << std::setw(10) << "--"
               << std::setw(10) << "--"
               << std::setw(10) << "--"
               << std::setw(9)  << "0.0%"
               << "OFFLINE (Unreachable)\n";
            continue;
        }

        std::string rankStr = "#" + std::to_string(r.rank);
        std::stringstream sReason, sCode, sVerif, sLat, sTps, sComp;
        sReason << std::fixed << std::setprecision(1) << r.reasoningScore << "%";
        sCode << std::fixed << std::setprecision(1) << r.codingScore << "%";
        sVerif << std::fixed << std::setprecision(1) << r.verificationScore << "%";
        sLat << r.latencyMs << "ms";
        sTps << std::fixed << std::setprecision(1) << r.tokensPerSec;
        sComp << std::fixed << std::setprecision(1) << r.compositeScore << "%";

        ss << std::left
           << std::setw(6)  << rankStr
           << std::setw(28) << (r.modelId.length() > 27 ? r.modelId.substr(0, 24) + "..." : r.modelId)
           << std::setw(14) << r.providerName
           << std::setw(10) << sReason.str()
           << std::setw(10) << sCode.str()
           << std::setw(10) << sVerif.str()
           << std::setw(10) << sLat.str()
           << std::setw(10) << sTps.str()
           << std::setw(9)  << sComp.str()
           << r.grade << "\n";
    }

    ss << "================================================================================================\n"
       << "CATEGORY LEADERS:\n"
       << "  * Overall Champion         : " << (suite.topPerformerModel.empty() ? "None" : suite.topPerformerModel) << "\n"
       << "  * Highest Throughput (TPS) : " << (suite.fastestModel.empty() ? "None" : suite.fastestModel) << "\n"
       << "  * Reasoning Master         : " << (suite.bestReasoningModel.empty() ? "None" : suite.bestReasoningModel) << "\n"
       << "  * Code Invariant Champion  : " << (suite.bestCodingModel.empty() ? "None" : suite.bestCodingModel) << "\n"
       << "  * Verification Guardian    : " << (suite.bestVerificationModel.empty() ? "None" : suite.bestVerificationModel) << "\n"
       << "------------------------------------------------------------------------------------------------\n"
       << " Tested " << suite.totalModelsTested << " models (" << suite.onlineModels << " online)\n"
       << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte | https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
       << "================================================================================================\n";

    return ss.str();
}

bool BenchmarkEngine::saveBenchmarkResult(const BenchmarkSuiteResult& result, const std::string& path) {
    try {
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << result.toJson().dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

BenchmarkSuiteResult BenchmarkEngine::loadBenchmarkResult(const std::string& path) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) return BenchmarkSuiteResult();
        nlohmann::json j = nlohmann::json::parse(f);
        return BenchmarkSuiteResult::fromJson(j);
    } catch (...) {
        return BenchmarkSuiteResult();
    }
}

} // namespace arenafight

