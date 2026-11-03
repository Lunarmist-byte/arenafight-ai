#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>
#include "arenafight/common/types.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/router/model_router.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

struct BenchmarkTask {
    std::string id;
    std::string title;
    std::string category; // "Reasoning", "Coding", "Verification", "Throughput"
    std::string prompt;
    int maxTokens = 512;
    float temperature = 0.1f;
    std::vector<std::string> requiredKeywords;
    std::vector<std::string> prohibitedPatterns;
};

struct ModelBenchmarkResult {
    std::string modelId;
    std::string providerName;
    bool online = false;
    float reasoningScore = 0.0f;     // 0.0 to 100.0%
    float codingScore = 0.0f;        // 0.0 to 100.0%
    float verificationScore = 0.0f;  // 0.0 to 100.0%
    float speedScore = 0.0f;         // 0.0 to 100.0%
    float compositeScore = 0.0f;     // 0.0 to 100.0%
    int latencyMs = 0;               // Total response latency in ms
    float tokensPerSec = 0.0f;       // Throughput
    int totalTokens = 0;             // Tokens produced
    int rank = 0;                    // Rank in leaderboard (1 = top)
    std::string grade;               // S+, S, A, B, C, F
    std::string primaryStrength;
    std::string summary;
    std::vector<std::string> testSummaries;

    nlohmann::json toJson() const {
        return {
            {"modelId", modelId},
            {"providerName", providerName},
            {"online", online},
            {"reasoningScore", reasoningScore},
            {"codingScore", codingScore},
            {"verificationScore", verificationScore},
            {"speedScore", speedScore},
            {"compositeScore", compositeScore},
            {"latencyMs", latencyMs},
            {"tokensPerSec", tokensPerSec},
            {"totalTokens", totalTokens},
            {"rank", rank},
            {"grade", grade},
            {"primaryStrength", primaryStrength},
            {"summary", summary},
            {"testSummaries", testSummaries}
        };
    }

    static ModelBenchmarkResult fromJson(const nlohmann::json& j) {
        ModelBenchmarkResult r;
        r.modelId = j.value("modelId", "");
        r.providerName = j.value("providerName", "");
        r.online = j.value("online", false);
        r.reasoningScore = j.value("reasoningScore", 0.0f);
        r.codingScore = j.value("codingScore", 0.0f);
        r.verificationScore = j.value("verificationScore", 0.0f);
        r.speedScore = j.value("speedScore", 0.0f);
        r.compositeScore = j.value("compositeScore", 0.0f);
        r.latencyMs = j.value("latencyMs", 0);
        r.tokensPerSec = j.value("tokensPerSec", 0.0f);
        r.totalTokens = j.value("totalTokens", 0);
        r.rank = j.value("rank", 0);
        r.grade = j.value("grade", "F");
        r.primaryStrength = j.value("primaryStrength", "General");
        r.summary = j.value("summary", "");
        r.testSummaries = j.value("testSummaries", std::vector<std::string>{});
        return r;
    }
};

struct BenchmarkSuiteResult {
    std::string suiteName = "ArenaFight Universal Model Benchmark Suite";
    int64_t timestamp = 0;
    int totalModelsTested = 0;
    int onlineModels = 0;
    std::vector<ModelBenchmarkResult> leaderboard;
    std::string topPerformerModel;
    std::string fastestModel;
    std::string bestReasoningModel;
    std::string bestCodingModel;
    std::string bestVerificationModel;
    std::string formattedReport;

    nlohmann::json toJson() const {
        nlohmann::json board = nlohmann::json::array();
        for (const auto& r : leaderboard) {
            board.push_back(r.toJson());
        }
        return {
            {"suiteName", suiteName},
            {"timestamp", timestamp},
            {"totalModelsTested", totalModelsTested},
            {"onlineModels", onlineModels},
            {"topPerformerModel", topPerformerModel},
            {"fastestModel", fastestModel},
            {"bestReasoningModel", bestReasoningModel},
            {"bestCodingModel", bestCodingModel},
            {"bestVerificationModel", bestVerificationModel},
            {"leaderboard", board}
        };
    }

    static BenchmarkSuiteResult fromJson(const nlohmann::json& j) {
        BenchmarkSuiteResult res;
        res.suiteName = j.value("suiteName", "ArenaFight Universal Model Benchmark Suite");
        res.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        res.totalModelsTested = j.value("totalModelsTested", 0);
        res.onlineModels = j.value("onlineModels", 0);
        res.topPerformerModel = j.value("topPerformerModel", "");
        res.fastestModel = j.value("fastestModel", "");
        res.bestReasoningModel = j.value("bestReasoningModel", "");
        res.bestCodingModel = j.value("bestCodingModel", "");
        res.bestVerificationModel = j.value("bestVerificationModel", "");
        if (j.contains("leaderboard") && j["leaderboard"].is_array()) {
            for (const auto& item : j["leaderboard"]) {
                res.leaderboard.push_back(ModelBenchmarkResult::fromJson(item));
            }
        }
        return res;
    }
};

class BenchmarkEngine {
public:
    explicit BenchmarkEngine(ModelRouter& router, const Config& config = Config());

    // Run standardized benchmark suite against all registered/available models
    BenchmarkSuiteResult benchmarkAll(bool verbose = true);

    // Run benchmark against an explicit list of model candidates
    BenchmarkSuiteResult benchmarkModels(const std::vector<std::string>& modelNames, bool verbose = true);

    // Benchmark an individual model candidate
    ModelBenchmarkResult benchmarkSingleModel(const ModelInfo& model, std::shared_ptr<ModelProvider> provider, bool verbose = true);

    // Standard suite of benchmark tasks
    static std::vector<BenchmarkTask> getStandardTasks();

    // Presentation helpers
    static std::string formatLeaderboardReport(const BenchmarkSuiteResult& suite);
    static std::string calculateGrade(float compositeScore);

    // Disk persistence
    bool saveBenchmarkResult(const BenchmarkSuiteResult& result, const std::string& path = "benchmark_results.json");
    static BenchmarkSuiteResult loadBenchmarkResult(const std::string& path = "benchmark_results.json");

private:
    ModelRouter& router_;
    Config config_;

    float evaluateTaskResponse(const BenchmarkTask& task, const AgentResponse& response, const std::string& modelId);
};

} // namespace arenafight

