#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace arenafight {

// Hardware acceleration backend classification
enum class AccelerationType {
    CUDA,           // Dedicated NVIDIA GPU with CUDA acceleration
    ROCM,           // Dedicated AMD ROCm acceleration
    INTEGRATED_GPU, // Integrated GPU (Intel UHD/Iris, AMD Radeon Graphics) with shared system memory
    CPU_ONLY        // No GPU acceleration (Pure CPU execution - noticeably slower)
};

inline std::string accelerationTypeToString(AccelerationType type) {
    switch (type) {
        case AccelerationType::CUDA: return "CUDA (NVIDIA Hardware Accelerated)";
        case AccelerationType::ROCM: return "ROCm (AMD Hardware Accelerated)";
        case AccelerationType::INTEGRATED_GPU: return "iGPU (Integrated Graphics - Noticeably Slower)";
        case AccelerationType::CPU_ONLY: return "CPU_ONLY (Pure CPU - Noticeably Slower)";
        default: return "CPU_ONLY (Noticeably Slower)";
    }
}

// Hardware specification detected from the physical host machine
struct HardwareSpec {
    std::string osName;             // e.g. "Windows 11 (x86_64)" or "Linux 6.8 (x86_64)"
    std::string cpuArchitecture;    // e.g. "x86_64", "ARM64"
    uint32_t logicalCores = 1;       // Total concurrent execution threads
    uint32_t physicalCores = 1;      // Physical CPU cores
    uint64_t totalRamBytes = 0;     // Total physical RAM in bytes
    uint64_t availRamBytes = 0;     // Available / free physical RAM in bytes
    double memoryLoadPercent = 0.0; // Current memory load (0.0 to 100.0)

    // Dedicated GPU / CUDA capabilities
    bool hasGpu = false;            // GPU acceleration detected
    std::string gpuName;            // e.g. "NVIDIA GeForce RTX 4060 Laptop GPU"
    uint64_t gpuTotalVramBytes = 0; // Total VRAM in bytes
    uint64_t gpuFreeVramBytes = 0;  // Free VRAM in bytes
    bool cudaAvailable = false;     // True if NVIDIA CUDA driver/runtime detected
    std::string cudaVersion;        // e.g. "CUDA 13.1" or "Driver 592.82"

    // Integrated GPU (iGPU) & CPU fallback capabilities
    bool hasIntegratedGpu = false;  // True if Intel Iris/UHD or AMD Radeon iGPU detected
    std::string igpuName;           // e.g. "AMD Radeon(TM) Graphics" or "Intel(R) Iris(R) Xe"
    AccelerationType acceleration = AccelerationType::CPU_ONLY;
    std::string performanceNotice;  // Prominent warning notice when CPU or iGPU is used

    double totalRamGb() const { return static_cast<double>(totalRamBytes) / (1024.0 * 1024.0 * 1024.0); }
    double availRamGb() const { return static_cast<double>(availRamBytes) / (1024.0 * 1024.0 * 1024.0); }
    double gpuTotalVramGb() const { return static_cast<double>(gpuTotalVramBytes) / (1024.0 * 1024.0 * 1024.0); }
    double gpuFreeVramGb() const { return static_cast<double>(gpuFreeVramBytes) / (1024.0 * 1024.0 * 1024.0); }

    bool isCuda() const { return acceleration == AccelerationType::CUDA; }
    bool isIntegratedGpu() const { return acceleration == AccelerationType::INTEGRATED_GPU; }
    bool isCpuOnly() const { return acceleration == AccelerationType::CPU_ONLY; }
    bool isSlowVariant() const { return acceleration == AccelerationType::INTEGRATED_GPU || acceleration == AccelerationType::CPU_ONLY; }

    nlohmann::json toJson() const;
    static HardwareSpec fromJson(const nlohmann::json& j);
};

// Dynamic resource limits calculated by assessing the physical host machine
struct DynamicResourceLimits {
    std::string processingLevel;        // "fast" or "perfect_boi"
    int maxIterations = 10;             // Scaled dynamically by hardware headroom
    int maxTokens = 4096;               // Scaled dynamically by RAM headroom
    int parallelWorkers = 1;            // Scaled dynamically by CPU cores & memory
    int deepThinkingBudget = 0;         // Tokens allocated for deep reasoning phase
    int adversarialReviewPasses = 1;    // Multi-pass adversarial verification passes
    int verificationTimeoutMs = 60000;  // Timeout scaled for thoroughness
    bool enableExhaustiveGates = false; // Strictly require 100% proof across all gates
    bool selfHealingRemediation = false;// Automatically spawn refinement loops on flaws
    std::string assessmentSummary;      // Human-readable summary of how limits were chosen
    bool isSlowVariant = false;         // Indicates CPU or iGPU execution with scaled expectations

    nlohmann::json toJson() const;
    static DynamicResourceLimits fromJson(const nlohmann::json& j);
};

// Cross-platform System Profiler for Windows and Linux
class SystemProfiler {
public:
    // Assess physical machine hardware in real time without hardcoded limits
    static HardwareSpec profileHostMachine(bool forceCpuOnly = false);

    // Compute dynamic, hardware-informed limits tailored for the active processing level
    static DynamicResourceLimits calculateDynamicLimits(
        const std::string& processingLevel,
        const HardwareSpec& spec
    );

    // Convenience method: profile and calculate in one call
    static DynamicResourceLimits assessAndCalculate(const std::string& processingLevel, bool forceCpuOnly = false);

    // Format human-readable terminal dashboard of hardware and dynamic limits
    static std::string formatAssessmentDashboard(
        const HardwareSpec& spec,
        const DynamicResourceLimits& limits
    );

    // Format prominent performance notice box for slow variants (iGPU / CPU)
    static std::string formatPerformanceBanner(const HardwareSpec& spec);

    // Linux and Windows specific helpers
    static bool isLinux();
    static bool isWindows();
};

} // namespace arenafight
