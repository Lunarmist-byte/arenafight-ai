#include "arenafight/system/system_profiler.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <regex>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <fstream>
#endif

namespace arenafight {

bool SystemProfiler::isWindows() {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

bool SystemProfiler::isLinux() {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}

HardwareSpec SystemProfiler::profileHostMachine(bool forceCpuOnly) {
    HardwareSpec spec;

    // 1. Thread concurrency fallback
    spec.logicalCores = std::thread::hardware_concurrency();
    if (spec.logicalCores == 0) spec.logicalCores = 1;
    spec.physicalCores = spec.logicalCores; // Refined below if platform supports

#if defined(_WIN32)
    // Windows Hardware Assessment
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    spec.logicalCores = sysInfo.dwNumberOfProcessors;

    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: spec.cpuArchitecture = "x86_64"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: spec.cpuArchitecture = "ARM64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: spec.cpuArchitecture = "x86"; break;
        default: spec.cpuArchitecture = "Unknown"; break;
    }

    spec.osName = "Windows (NT " + spec.cpuArchitecture + ")";

    // Physical RAM detection via Win32 API
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memStatus)) {
        spec.totalRamBytes = memStatus.ullTotalPhys;
        spec.availRamBytes = memStatus.ullAvailPhys;
        spec.memoryLoadPercent = static_cast<double>(memStatus.dwMemoryLoad);
    }

    // Enumerate display adapters to detect integrated GPUs (iGPU) and discrete GPUs
    DISPLAY_DEVICEW dd;
    dd.cb = sizeof(dd);
    for (DWORD devNum = 0; EnumDisplayDevicesW(NULL, devNum, &dd, 0); devNum++) {
        if (dd.DeviceString[0] != L'\0') {
            int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, NULL, 0, NULL, NULL);
            std::string devName(sizeNeeded, 0);
            WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, &devName[0], sizeNeeded, NULL, NULL);
            while (!devName.empty() && devName.back() == '\0') devName.pop_back();

            std::string lowerDev = devName;
            std::transform(lowerDev.begin(), lowerDev.end(), lowerDev.begin(), ::tolower);

            if (lowerDev.find("nvidia") != std::string::npos || lowerDev.find("geforce") != std::string::npos) {
                spec.hasGpu = true;
                spec.gpuName = devName;
            } else if (lowerDev.find("intel") != std::string::npos ||
                       lowerDev.find("uhd") != std::string::npos ||
                       lowerDev.find("iris") != std::string::npos ||
                       lowerDev.find("radeon(tm)") != std::string::npos ||
                       lowerDev.find("radeon graphics") != std::string::npos ||
                       lowerDev.find("vega") != std::string::npos) {
                spec.hasIntegratedGpu = true;
                if (spec.igpuName.empty()) {
                    spec.igpuName = devName;
                }
            }
        }
    }

#elif defined(__linux__)
    // Linux Hardware Assessment
    long confCores = sysconf(_SC_NPROCESSORS_ONLN);
    if (confCores > 0) {
        spec.logicalCores = static_cast<uint32_t>(confCores);
    }

    struct utsname uts;
    if (uname(&uts) == 0) {
        spec.osName = std::string(uts.sysname) + " " + std::string(uts.release);
        spec.cpuArchitecture = uts.machine;
    } else {
        spec.osName = "Linux (POSIX)";
        spec.cpuArchitecture = "x86_64";
    }

    // Parse Linux /proc/meminfo
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        uint64_t memTotalKb = 0;
        uint64_t memAvailKb = 0;
        while (std::getline(meminfo, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                std::istringstream iss(line.substr(9));
                iss >> memTotalKb;
            } else if (line.rfind("MemAvailable:", 0) == 0) {
                std::istringstream iss(line.substr(13));
                iss >> memAvailKb;
            }
        }
        if (memTotalKb > 0) {
            spec.totalRamBytes = memTotalKb * 1024ULL;
            spec.availRamBytes = (memAvailKb > 0 ? memAvailKb : memTotalKb / 2) * 1024ULL;
            if (spec.totalRamBytes > 0) {
                double used = static_cast<double>(spec.totalRamBytes - spec.availRamBytes);
                spec.memoryLoadPercent = (used / static_cast<double>(spec.totalRamBytes)) * 100.0;
            }
        }
    }

    // Detect Linux iGPU / display controllers via /sys/class/drm
    try {
        if (std::filesystem::exists("/sys/class/drm")) {
            for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm")) {
                std::string fname = entry.path().filename().string();
                if (fname.rfind("card", 0) == 0 && fname.find('-') == std::string::npos) {
                    // Check device vendor
                    std::filesystem::path vendorPath = entry.path() / "device" / "vendor";
                    if (std::filesystem::exists(vendorPath)) {
                        std::ifstream vfile(vendorPath);
                        std::string vhex;
                        vfile >> vhex;
                        if (vhex == "0x8086") { // Intel
                            spec.hasIntegratedGpu = true;
                            if (spec.igpuName.empty()) spec.igpuName = "Intel Integrated Graphics (iGPU)";
                        } else if (vhex == "0x1002") { // AMD
                            spec.hasIntegratedGpu = true;
                            if (spec.igpuName.empty()) spec.igpuName = "AMD Radeon Integrated Graphics (iGPU)";
                        }
                    }
                }
            }
        }
    } catch (...) {}

#else
    // Generic POSIX Fallback
    spec.osName = "Generic POSIX";
    spec.cpuArchitecture = "Unknown";
    spec.totalRamBytes = 8ULL * 1024 * 1024 * 1024;
    spec.availRamBytes = 4ULL * 1024 * 1024 * 1024;
    spec.memoryLoadPercent = 50.0;
#endif

    // Estimate physical cores (logical / 2 for hyperthreaded architectures)
    if (spec.logicalCores > 1) {
        spec.physicalCores = std::max<uint32_t>(1, spec.logicalCores / 2);
    } else {
        spec.physicalCores = 1;
    }

    // Force CPU-only override if requested (e.g. testing or low-power mode)
    if (forceCpuOnly) {
        spec.hasGpu = false;
        spec.cudaAvailable = false;
        spec.acceleration = AccelerationType::CPU_ONLY;
        spec.performanceNotice =
            "PERFORMANCE NOTICE: Force CPU-only mode active. Dedicated GPU acceleration is bypassed. "
            "Inference and verification will be NOTICEABLY SLOWER (~2-8 tokens/sec vs 40-100+ on dedicated CUDA GPU).";
        return spec;
    }

    // 2. Cross-platform Dedicated CUDA GPU assessment via nvidia-smi
    try {
        ProcessResult gpuProbe = ProcessExecutor::executeShell(
            "nvidia-smi --query-gpu=driver_version,name,memory.total,memory.free --format=csv,noheader,nounits",
            ".",
            2500
        );
        if (gpuProbe.exitCode == 0 && !gpuProbe.stdOut.empty()) {
            std::string out = gpuProbe.stdOut;
            size_t newlinePos = out.find('\n');
            if (newlinePos != std::string::npos) out = out.substr(0, newlinePos);

            std::vector<std::string> parts;
            std::stringstream ss(out);
            std::string token;
            while (std::getline(ss, token, ',')) {
                size_t first = token.find_first_not_of(" \t\r\n");
                size_t last = token.find_last_not_of(" \t\r\n");
                if (first != std::string::npos && last != std::string::npos) {
                    parts.push_back(token.substr(first, last - first + 1));
                }
            }

            if (parts.size() >= 4) {
                spec.hasGpu = true;
                spec.cudaAvailable = true;
                spec.acceleration = AccelerationType::CUDA;
                spec.cudaVersion = "Driver " + parts[0];
                spec.gpuName = parts[1];
                try {
                    uint64_t totalMb = std::stoull(parts[2]);
                    uint64_t freeMb = std::stoull(parts[3]);
                    spec.gpuTotalVramBytes = totalMb * 1024ULL * 1024ULL;
                    spec.gpuFreeVramBytes = freeMb * 1024ULL * 1024ULL;
                } catch (...) {}
            }
        }
    } catch (...) {}

    // Check for CUDA runtime version if available
    if (spec.cudaAvailable) {
        ProcessResult nvccRes = ProcessExecutor::executeShell("nvcc --version", ".", 1500);
        if (nvccRes.exitCode == 0 && !nvccRes.stdOut.empty()) {
            static const std::regex cudaRegex("release\\s+([0-9]+\\.[0-9]+)");
            std::smatch m;
            if (std::regex_search(nvccRes.stdOut, m, cudaRegex)) {
                spec.cudaVersion = "CUDA " + m[1].str();
            }
        }
        spec.performanceNotice =
            "CUDA Acceleration Active: High-throughput hardware execution enabled on " +
            spec.gpuName + " (" + spec.cudaVersion + ").";
    } else if (spec.hasIntegratedGpu) {
        spec.acceleration = AccelerationType::INTEGRATED_GPU;
        std::string name = spec.igpuName.empty() ? "Integrated GPU" : spec.igpuName;
        spec.performanceNotice =
            "PERFORMANCE NOTICE: Integrated GPU (iGPU: " + name + ") detected without dedicated CUDA acceleration. "
            "Inference, token generation, and verification will be NOTICEABLY SLOWER (~4-12 tokens/sec vs 40-100+ tokens/sec on dedicated CUDA GPU). "
            "System will automatically scale execution budgets to avoid freezing your machine.";
    } else {
        spec.acceleration = AccelerationType::CPU_ONLY;
        spec.performanceNotice =
            "PERFORMANCE NOTICE: Pure CPU-only execution active (No GPU detected). "
            "Inference and verification will be NOTICEABLY SLOWER (~2-8 tokens/sec). "
            "Execution budgets have been tuned conservatively to prevent system unresponsiveness.";
    }

    return spec;
}

DynamicResourceLimits SystemProfiler::calculateDynamicLimits(
    const std::string& processingLevel,
    const HardwareSpec& spec
) {
    DynamicResourceLimits limits;
    std::string lvl = processingLevel;
    std::transform(lvl.begin(), lvl.end(), lvl.begin(), ::tolower);

    bool isPerfect = (lvl == "perfect" || lvl == "perfect_boi" || lvl == "perfect-boi");
    limits.processingLevel = isPerfect ? "perfect_boi" : "fast";
    limits.isSlowVariant = spec.isSlowVariant();

    double totalGb = spec.totalRamGb();
    double availGb = spec.availRamGb();
    uint32_t cores = spec.logicalCores;

    // =========================================================================
    // SLOW VARIANT (iGPU or CPU-ONLY) CALIBRATION
    // =========================================================================
    if (spec.isSlowVariant()) {
        if (!isPerfect) {
            // Fast mode on slow variant: small conservative bounds to keep host responsive
            limits.maxIterations = 8;
            limits.maxTokens = 2048; // Prevent shared memory saturation
            limits.parallelWorkers = 1;
            limits.deepThinkingBudget = 512;
            limits.adversarialReviewPasses = 1;
            limits.verificationTimeoutMs = 90000;
            limits.enableExhaustiveGates = false;
            limits.selfHealingRemediation = false;

            std::ostringstream ss;
            ss << "Fast Mode [SLOW VARIANT: " << accelerationTypeToString(spec.acceleration) << "]: "
               << "Tuned for CPU/iGPU execution (" << limits.maxIterations << " loops, "
               << limits.maxTokens << " tokens, " << limits.parallelWorkers << " worker). Noticeably slower execution.";
            limits.assessmentSummary = ss.str();
            return limits;
        } else {
            // Perfect Boi mode on slow variant: thoroughness with sensible CPU bounds
            limits.maxIterations = 15;
            limits.maxTokens = 4096;
            limits.parallelWorkers = std::clamp(static_cast<int>(cores) / 4, 1, 3);
            limits.deepThinkingBudget = 1536;
            limits.adversarialReviewPasses = 1;
            limits.verificationTimeoutMs = 180000; // Extra time for slow CPU compilation/tests
            limits.enableExhaustiveGates = true;
            limits.selfHealingRemediation = true;

            std::ostringstream ss;
            ss << "Perfect Boi Mode [SLOW VARIANT: " << accelerationTypeToString(spec.acceleration) << "]: "
               << "Tuned for CPU/iGPU (" << limits.maxIterations << " loops, "
               << limits.maxTokens << " tokens, " << limits.deepThinkingBudget << " reasoning tokens). "
               << "Full physical verification enforced; execution will be noticeably slower.";
            limits.assessmentSummary = ss.str();
            return limits;
        }
    }

    // =========================================================================
    // FAST MODE (HARDWARE ACCELERATED / CUDA)
    // =========================================================================
    if (!isPerfect) {
        limits.maxIterations = 10;
        limits.maxTokens = 4096;
        limits.parallelWorkers = std::clamp(static_cast<int>(cores) / 4, 1, 4);
        limits.deepThinkingBudget = 1024;
        limits.adversarialReviewPasses = 1;
        limits.verificationTimeoutMs = 60000;
        limits.enableExhaustiveGates = false;
        limits.selfHealingRemediation = false;

        std::ostringstream ss;
        ss << "Fast Processing Mode: Standard baseline ("
           << limits.maxIterations << " iterations, "
           << limits.maxTokens << " tokens, "
           << limits.parallelWorkers << " workers). Rapid turnaround prioritized.";
        limits.assessmentSummary = ss.str();
        return limits;
    }

    // =========================================================================
    // PERFECT BOI MODE (CUDA HARDWARE ACCELERATED)
    // =========================================================================
    limits.enableExhaustiveGates = true;
    limits.selfHealingRemediation = true;

    // 1. Dynamic Token Limit based on Available RAM
    if (availGb >= 28.0) {
        limits.maxTokens = 16384;
    } else if (availGb >= 14.0) {
        limits.maxTokens = 12288;
    } else if (availGb >= 7.0) {
        limits.maxTokens = 8192;
    } else if (availGb >= 3.5) {
        limits.maxTokens = 4096;
    } else {
        limits.maxTokens = 2048;
    }

    // 2. Dynamic Iteration Budget based on CPU Cores & Memory
    if (cores >= 16 && availGb >= 6.0) {
        limits.maxIterations = 25;
        limits.parallelWorkers = std::clamp(static_cast<int>(cores) - 2, 2, 8);
    } else if (cores >= 8 && availGb >= 4.0) {
        limits.maxIterations = 20;
        limits.parallelWorkers = std::clamp(static_cast<int>(cores) - 2, 2, 4);
    } else if (cores >= 4) {
        limits.maxIterations = 15;
        limits.parallelWorkers = std::max<int>(1, cores / 2);
    } else {
        limits.maxIterations = 12;
        limits.parallelWorkers = 1;
    }

    // 3. Deep Thinking Budget & Adversarial Passes based on GPU & Memory
    if (spec.hasGpu && spec.gpuFreeVramGb() >= 6.0) {
        limits.deepThinkingBudget = 4096;
        limits.adversarialReviewPasses = 2;
        limits.verificationTimeoutMs = 120000;
    } else if (spec.hasGpu && spec.gpuFreeVramGb() >= 3.0) {
        limits.deepThinkingBudget = 2048;
        limits.adversarialReviewPasses = 2;
        limits.verificationTimeoutMs = 90000;
    } else if (availGb >= 12.0) {
        limits.deepThinkingBudget = 3072;
        limits.adversarialReviewPasses = 2;
        limits.verificationTimeoutMs = 90000;
    } else {
        limits.deepThinkingBudget = 1536;
        limits.adversarialReviewPasses = 1;
        limits.verificationTimeoutMs = 60000;
    }

    std::ostringstream ss;
    ss << "Perfect Boi Mode active: Limits dynamically calibrated to host PC ("
       << cores << " cores, "
       << std::fixed << std::setprecision(1) << totalGb << " GB RAM ["
       << availGb << " GB free], ";
    if (spec.isCuda()) {
        ss << spec.gpuName << " [" << std::setprecision(1) << spec.gpuFreeVramGb() << " GB VRAM free, " << spec.cudaVersion << "]";
    } else {
        ss << "CPU native execution";
    }
    ss << "). Dynamic allocation: "
       << limits.maxIterations << " max iterations, "
       << limits.maxTokens << " tokens, "
       << limits.deepThinkingBudget << " reasoning tokens, "
       << limits.parallelWorkers << " workers, "
       << limits.adversarialReviewPasses << " adversarial passes.";
    limits.assessmentSummary = ss.str();

    return limits;
}

DynamicResourceLimits SystemProfiler::assessAndCalculate(const std::string& processingLevel, bool forceCpuOnly) {
    HardwareSpec spec = profileHostMachine(forceCpuOnly);
    return calculateDynamicLimits(processingLevel, spec);
}

std::string SystemProfiler::formatPerformanceBanner(const HardwareSpec& spec) {
    std::ostringstream oss;
    if (spec.isSlowVariant()) {
        oss << "\n+------------------------------------------------------------------------+\n"
            << "| [PERFORMANCE NOTICE] CPU / iGPU EXECUTION ACTIVE                       |\n"
            << "| Hardware Acceleration : " << std::left << std::setw(47) << accelerationTypeToString(spec.acceleration) << "|\n"
            << "| Note: Token generation & verification will be NOTICEABLY SLOWER.       |\n"
            << "| Expected Throughput   : ~3-12 tokens/sec (vs 40-100+ on dedicated CUDA)|\n"
            << "+------------------------------------------------------------------------+\n";
    } else if (spec.isCuda()) {
        oss << "\n+------------------------------------------------------------------------+\n"
            << "| [HARDWARE ACCELERATION] CUDA ACTIVE                                    |\n"
            << "| Device : " << std::left << std::setw(62) << (spec.gpuName + " (" + spec.cudaVersion + ")") << "|\n"
            << "| Status : High-throughput dedicated acceleration enabled               |\n"
            << "+------------------------------------------------------------------------+\n";
    }
    return oss.str();
}

std::string SystemProfiler::formatAssessmentDashboard(
    const HardwareSpec& spec,
    const DynamicResourceLimits& limits
) {
    std::ostringstream oss;
    oss << "\n====================================================\n"
        << "          SYSTEM HARDWARE ASSESSMENT (PC PROFILER)  \n"
        << "====================================================\n"
        << "Host Platform:    " << spec.osName << " [" << spec.cpuArchitecture << "]\n"
        << "CPU Threads:      " << spec.logicalCores << " Logical (" << spec.physicalCores << " Physical Cores)\n"
        << "System RAM:       " << std::fixed << std::setprecision(2) << spec.totalRamGb() << " GB Total | "
        << spec.availRamGb() << " GB Available (" << std::setprecision(1) << spec.memoryLoadPercent << "% Load)\n"
        << "Acceleration:     " << accelerationTypeToString(spec.acceleration) << "\n";

    if (spec.isCuda()) {
        oss << "GPU Model:        " << spec.gpuName << " (" << spec.cudaVersion << ")\n"
            << "GPU VRAM:         " << std::fixed << std::setprecision(2) << spec.gpuTotalVramGb() << " GB Total | "
            << spec.gpuFreeVramGb() << " GB Free\n";
    } else if (spec.isIntegratedGpu()) {
        oss << "Integrated GPU:   " << spec.igpuName << " (Shared System Memory)\n"
            << "Performance Note: NOTICEABLY SLOWER without dedicated CUDA acceleration\n";
    } else {
        oss << "GPU Model:        None (Pure CPU Inference)\n"
            << "Performance Note: NOTICEABLY SLOWER without hardware acceleration\n";
    }

    if (!spec.performanceNotice.empty()) {
        oss << "Notice:           " << spec.performanceNotice << "\n";
    }

    oss << "----------------------------------------------------\n"
        << "Processing Level: "
        << (limits.processingLevel == "perfect_boi" ? "PERFECT BOI (Exhaustive & Adaptive)" : "FAST (Standard)")
        << "\n"
        << "DYNAMICALLY ASSESSED LIMITS (NO HARDCODING):\n"
        << "  * Max Iterations:       " << limits.maxIterations << " loops\n"
        << "  * Max Token Budget:     " << limits.maxTokens << " tokens\n"
        << "  * Deep Thinking Budget: " << limits.deepThinkingBudget << " reasoning tokens\n"
        << "  * Parallel Concurrency: " << limits.parallelWorkers << " workers\n"
        << "  * Adversarial Review:   " << limits.adversarialReviewPasses << " passes\n"
        << "  * Self-Healing Loops:   " << (limits.selfHealingRemediation ? "ENABLED" : "DISABLED") << "\n"
        << "  * Verification Timeout: " << (limits.verificationTimeoutMs / 1000) << " seconds\n"
        << "Summary: " << limits.assessmentSummary << "\n"
        << "====================================================\n"
        << " Made by Lunarmist-byte | https://github.com/Lunarmist-byte\n"
        << "====================================================\n\n";

    return oss.str();
}

nlohmann::json HardwareSpec::toJson() const {
    return {
        {"os_name", osName},
        {"cpu_architecture", cpuArchitecture},
        {"logical_cores", logicalCores},
        {"physical_cores", physicalCores},
        {"total_ram_bytes", totalRamBytes},
        {"avail_ram_bytes", availRamBytes},
        {"memory_load_percent", memoryLoadPercent},
        {"has_gpu", hasGpu},
        {"gpu_name", gpuName},
        {"gpu_total_vram_bytes", gpuTotalVramBytes},
        {"gpu_free_vram_bytes", gpuFreeVramBytes},
        {"cuda_available", cudaAvailable},
        {"cuda_version", cudaVersion},
        {"has_integrated_gpu", hasIntegratedGpu},
        {"igpu_name", igpuName},
        {"acceleration", accelerationTypeToString(acceleration)},
        {"performance_notice", performanceNotice}
    };
}

HardwareSpec HardwareSpec::fromJson(const nlohmann::json& j) {
    HardwareSpec s;
    s.osName = j.value("os_name", "Unknown");
    s.cpuArchitecture = j.value("cpu_architecture", "x86_64");
    s.logicalCores = j.value("logical_cores", 1U);
    s.physicalCores = j.value("physical_cores", 1U);
    s.totalRamBytes = j.value("total_ram_bytes", 0ULL);
    s.availRamBytes = j.value("avail_ram_bytes", 0ULL);
    s.memoryLoadPercent = j.value("memory_load_percent", 0.0);
    s.hasGpu = j.value("has_gpu", false);
    s.gpuName = j.value("gpu_name", "");
    s.gpuTotalVramBytes = j.value("gpu_total_vram_bytes", 0ULL);
    s.gpuFreeVramBytes = j.value("gpu_free_vram_bytes", 0ULL);
    s.cudaAvailable = j.value("cuda_available", false);
    s.cudaVersion = j.value("cuda_version", "");
    s.hasIntegratedGpu = j.value("has_integrated_gpu", false);
    s.igpuName = j.value("igpu_name", "");
    s.performanceNotice = j.value("performance_notice", "");

    std::string acc = j.value("acceleration", "CPU_ONLY");
    if (acc.find("CUDA") != std::string::npos) s.acceleration = AccelerationType::CUDA;
    else if (acc.find("ROCm") != std::string::npos) s.acceleration = AccelerationType::ROCM;
    else if (acc.find("iGPU") != std::string::npos || acc.find("Integrated") != std::string::npos) s.acceleration = AccelerationType::INTEGRATED_GPU;
    else s.acceleration = AccelerationType::CPU_ONLY;

    return s;
}

nlohmann::json DynamicResourceLimits::toJson() const {
    return {
        {"processing_level", processingLevel},
        {"max_iterations", maxIterations},
        {"max_tokens", maxTokens},
        {"parallel_workers", parallelWorkers},
        {"deep_thinking_budget", deepThinkingBudget},
        {"adversarial_review_passes", adversarialReviewPasses},
        {"verification_timeout_ms", verificationTimeoutMs},
        {"enable_exhaustive_gates", enableExhaustiveGates},
        {"self_healing_remediation", selfHealingRemediation},
        {"assessment_summary", assessmentSummary},
        {"is_slow_variant", isSlowVariant}
    };
}

DynamicResourceLimits DynamicResourceLimits::fromJson(const nlohmann::json& j) {
    DynamicResourceLimits l;
    l.processingLevel = j.value("processing_level", "fast");
    l.maxIterations = j.value("max_iterations", 10);
    l.maxTokens = j.value("max_tokens", 4096);
    l.parallelWorkers = j.value("parallel_workers", 1);
    l.deepThinkingBudget = j.value("deep_thinking_budget", 0);
    l.adversarialReviewPasses = j.value("adversarial_review_passes", 1);
    l.verificationTimeoutMs = j.value("verification_timeout_ms", 60000);
    l.enableExhaustiveGates = j.value("enable_exhaustive_gates", false);
    l.selfHealingRemediation = j.value("self_healing_remediation", false);
    l.assessmentSummary = j.value("assessment_summary", "");
    l.isSlowVariant = j.value("is_slow_variant", false);
    return l;
}

} // namespace arenafight
