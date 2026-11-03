#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "arenafight/common/config.hpp"
#include "arenafight/router/model_router.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

struct UnslothStatus {
    bool pythonAvailable = false;
    std::string pythonVersion;
    bool unslothInstalled = false;
    std::string unslothVersion;
    bool torchInstalled = false;
    std::string torchVersion;
    bool cudaAvailable = false;
    std::string gpuName;
    int vramGb = 0;
    std::string statusSummary;

    nlohmann::json toJson() const {
        return {
            {"pythonAvailable", pythonAvailable},
            {"pythonVersion", pythonVersion},
            {"unslothInstalled", unslothInstalled},
            {"unslothVersion", unslothVersion},
            {"torchInstalled", torchInstalled},
            {"torchVersion", torchVersion},
            {"cudaAvailable", cudaAvailable},
            {"gpuName", gpuName},
            {"vramGb", vramGb},
            {"statusSummary", statusSummary}
        };
    }
};

struct UnslothTrainConfig {
    std::string baseModel = "unsloth/Qwen2.5-Coder-7B-Instruct-bnb-4bit";
    std::string datasetPath = "unsloth_dataset.jsonl";
    std::string outputDir = "unsloth_output";
    std::string exportGgufName = "arena-finetuned";
    int maxSeqLength = 2048;
    int loraRank = 16;
    int loraAlpha = 16;
    int maxSteps = 60;
    int epochs = 1;
    float learningRate = 2e-4f;
    int batchSize = 2;
    int gradAccumSteps = 4;
    bool loadIn4Bit = true;
    bool exportGguf = true;
    std::string ggufQuantization = "q4_k_m";
    bool autoRegisterOllama = true;
    bool dryRunSimulation = false;

    nlohmann::json toJson() const {
        return {
            {"baseModel", baseModel},
            {"datasetPath", datasetPath},
            {"outputDir", outputDir},
            {"exportGgufName", exportGgufName},
            {"maxSeqLength", maxSeqLength},
            {"loraRank", loraRank},
            {"loraAlpha", loraAlpha},
            {"maxSteps", maxSteps},
            {"epochs", epochs},
            {"learningRate", learningRate},
            {"batchSize", batchSize},
            {"gradAccumSteps", gradAccumSteps},
            {"loadIn4Bit", loadIn4Bit},
            {"exportGguf", exportGguf},
            {"ggufQuantization", ggufQuantization},
            {"autoRegisterOllama", autoRegisterOllama},
            {"dryRunSimulation", dryRunSimulation}
        };
    }

    static UnslothTrainConfig fromJson(const nlohmann::json& j) {
        UnslothTrainConfig cfg;
        cfg.baseModel = j.value("baseModel", cfg.baseModel);
        cfg.datasetPath = j.value("datasetPath", cfg.datasetPath);
        cfg.outputDir = j.value("outputDir", cfg.outputDir);
        cfg.exportGgufName = j.value("exportGgufName", cfg.exportGgufName);
        cfg.maxSeqLength = j.value("maxSeqLength", cfg.maxSeqLength);
        cfg.loraRank = j.value("loraRank", cfg.loraRank);
        cfg.loraAlpha = j.value("loraAlpha", cfg.loraAlpha);
        cfg.maxSteps = j.value("maxSteps", cfg.maxSteps);
        cfg.epochs = j.value("epochs", cfg.epochs);
        cfg.learningRate = j.value("learningRate", cfg.learningRate);
        cfg.batchSize = j.value("batchSize", cfg.batchSize);
        cfg.gradAccumSteps = j.value("gradAccumSteps", cfg.gradAccumSteps);
        cfg.loadIn4Bit = j.value("loadIn4Bit", cfg.loadIn4Bit);
        cfg.exportGguf = j.value("exportGguf", cfg.exportGguf);
        cfg.ggufQuantization = j.value("ggufQuantization", cfg.ggufQuantization);
        cfg.autoRegisterOllama = j.value("autoRegisterOllama", cfg.autoRegisterOllama);
        cfg.dryRunSimulation = j.value("dryRunSimulation", cfg.dryRunSimulation);
        return cfg;
    }
};

struct DatasetEntry {
    std::string instruction;
    std::string input;
    std::string output;
    std::string domain; // "coding", "hardware", "reasoning", "verification"

    nlohmann::json toJson() const {
        return {
            {"instruction", instruction},
            {"input", input},
            {"output", output},
            {"domain", domain}
        };
    }
};

class UnslothManager {
public:
    explicit UnslothManager(const Config& config);

    // Environment Diagnostics
    UnslothStatus checkEnvironment();

    // Dataset extraction & formatting
    std::vector<DatasetEntry> extractFromSessions(const std::string& sessionsDir = "sessions");
    std::vector<DatasetEntry> generateSyntheticHardwareDataset();
    std::vector<DatasetEntry> generateWorkspaceFineTuneDataset(const std::string& workspaceDir = ".");
    std::vector<DatasetEntry> generateDatasetFromPath(const std::string& targetPath);
    bool saveDatasetToJsonl(const std::vector<DatasetEntry>& dataset, const std::string& outputPath);

    // Python training script generator
    std::string scaffoldTrainingScript(const UnslothTrainConfig& trainCfg);
    bool writeTrainingScript(const std::string& outputPath, const UnslothTrainConfig& trainCfg);

    // Modelfile generator
    std::string scaffoldModelfile(const std::string& ggufPath, const std::string& modelName);
    bool writeModelfile(const std::string& outputPath, const std::string& ggufPath, const std::string& modelName);

    // Run on-the-fly fine-tuning
    bool trainOnTheFly(
        const UnslothTrainConfig& trainCfg,
        std::function<void(const std::string&)> logCallback = nullptr,
        ModelRouter* routerToRegister = nullptr
    );

    // Register fine-tuned model with Ollama CLI
    bool registerWithOllama(const std::string& modelfilePath, const std::string& modelName);

private:
    Config config_;
    std::string pythonBinary_ = "python";
};

} // namespace arenafight

