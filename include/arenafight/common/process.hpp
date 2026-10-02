#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace arenafight {

struct ProcessResult {
    int exitCode = -1;
    std::string stdOut;
    std::string stdErr;
    int64_t durationMs = 0;
    bool timedOut = false;
};

class ProcessExecutor {
public:
    // Execute a shell command (PowerShell on Windows, bash on Linux/macOS)
    static ProcessResult executeShell(
        const std::string& command,
        const std::string& workingDir = ".",
        int timeoutMs = 120000
    );

    // Execute executable directly without shell wrapping
    static ProcessResult executeProgram(
        const std::string& program,
        const std::vector<std::string>& args,
        const std::string& workingDir = ".",
        int timeoutMs = 120000
    );

    // Check if command is considered dangerous/destructive
    static bool isDestructive(const std::string& command);

    // Platform detection
    static bool isWindows();
};

} // namespace arenafight
