#include "arenafight/services/service_daemon.hpp"
#include "arenafight/common/http_client.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <cstdlib>
#include <mutex>
#include <nlohmann/json.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace arenafight {

namespace {

std::string normalizeUrl(const std::string& url) {
    if (url.empty()) return "http://localhost:11434";
    std::string clean = url;
    while (!clean.empty() && clean.back() == '/') {
        clean.pop_back();
    }
    return clean;
}

#if defined(_WIN32)
std::wstring s2ws(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}
#endif

bool isOllamaProcessRunning() {
#if defined(_WIN32)
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    bool found = false;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            std::wstring exeName = pe.szExeFile;
            std::string lowerName;
            lowerName.reserve(exeName.size());
            for (wchar_t wc : exeName) {
                lowerName.push_back(static_cast<char>(std::tolower(wc)));
            }
            if (lowerName.find("ollama") != std::string::npos) {
                found = true;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return found;
#else
    ProcessResult p = ProcessExecutor::executeShell("pgrep -x ollama 2>/dev/null", ".", 1000);
    return p.exitCode == 0 && !p.stdOut.empty();
#endif
}

} // namespace

bool ServiceDaemon::isOllamaResponding(const std::string& baseUrl) {
    std::string url = normalizeUrl(baseUrl);
    try {
        // Fast-path: Direct IPv4 loopback to avoid IPv6 [::1] timeout delays on Windows
        if (url.find("localhost") != std::string::npos) {
            std::string alt = url;
            size_t pos = alt.find("localhost");
            alt.replace(pos, 9, "127.0.0.1");
            HttpResponse respFast = HttpClient::instance().get(alt + "/api/tags", {}, 1);
            return respFast.success;
        }

        HttpResponse resp = HttpClient::instance().get(url + "/api/tags", {}, 1);
        return resp.success;
    } catch (...) {}
    return false;
}

std::string ServiceDaemon::findOllamaExecutable() {
#if defined(_WIN32)
    // 1. Check PATH lookup on Windows
    ProcessResult pathLookup = ProcessExecutor::executeShell("where.exe ollama", ".", 2000);
    if (pathLookup.exitCode == 0 && !pathLookup.stdOut.empty()) {
        std::string out = pathLookup.stdOut;
        size_t nl = out.find('\n');
        if (nl != std::string::npos) out = out.substr(0, nl);
        // Trim trailing carriage return
        while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) {
            out.pop_back();
        }
        if (std::filesystem::exists(out)) {
            return out;
        }
    }

    // 2. Check standard Windows default install paths
    const char* localApp = std::getenv("LOCALAPPDATA");
    if (localApp) {
        std::filesystem::path p = std::filesystem::path(localApp) / "Programs" / "Ollama" / "ollama.exe";
        if (std::filesystem::exists(p)) return p.string();
    }

    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        std::filesystem::path p = std::filesystem::path(userProfile) / "AppData" / "Local" / "Programs" / "Ollama" / "ollama.exe";
        if (std::filesystem::exists(p)) return p.string();
    }

    std::vector<std::string> standardWinPaths = {
        "C:\\Program Files\\Ollama\\ollama.exe",
        "C:\\Program Files (x86)\\Ollama\\ollama.exe",
        "C:\\Ollama\\ollama.exe"
    };
    for (const auto& sp : standardWinPaths) {
        if (std::filesystem::exists(sp)) return sp;
    }

#else
    // 1. Check PATH lookup on Linux
    ProcessResult pathLookup = ProcessExecutor::executeShell("which ollama 2>/dev/null || command -v ollama 2>/dev/null", ".", 2000);
    if (pathLookup.exitCode == 0 && !pathLookup.stdOut.empty()) {
        std::string out = pathLookup.stdOut;
        size_t nl = out.find('\n');
        if (nl != std::string::npos) out = out.substr(0, nl);
        while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) {
            out.pop_back();
        }
        if (std::filesystem::exists(out)) {
            return out;
        }
    }

    // 2. Check standard Linux install paths
    const char* home = std::getenv("HOME");
    if (home) {
        std::filesystem::path userPath = std::filesystem::path(home) / ".local" / "bin" / "ollama";
        if (std::filesystem::exists(userPath)) return userPath.string();
    }

    std::vector<std::string> standardLinuxPaths = {
        "/usr/local/bin/ollama",
        "/usr/bin/ollama",
        "/bin/ollama",
        "/opt/ollama/bin/ollama"
    };
    for (const auto& sp : standardLinuxPaths) {
        if (std::filesystem::exists(sp)) return sp;
    }
#endif

    return "";
}

bool ServiceDaemon::launchOllamaBackground(const std::string& exePath) {
    std::string targetExe = exePath.empty() ? findOllamaExecutable() : exePath;
    if (targetExe.empty()) {
        Logger::instance().warn("[SERVICE] Cannot launch Ollama: executable not found in PATH or standard system directories.");
        return false;
    }

#if defined(_WIN32)
    // Windows: Use Win32 CreateProcessW with CREATE_NEW_PROCESS_GROUP, CREATE_NO_WINDOW and real log handles
    std::wstring cmdLine = L"\"" + s2ws(targetExe) + L"\" serve";
    std::filesystem::path exeFsPath(targetExe);
    std::wstring workDir = s2ws(exeFsPath.parent_path().string());

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    std::filesystem::path logPath;
    const char* tempEnv = std::getenv("TEMP");
    if (tempEnv && std::filesystem::exists(tempEnv)) {
        logPath = std::filesystem::path(tempEnv) / "ollama_daemon.log";
    } else {
        logPath = exeFsPath.parent_path() / "ollama_daemon.log";
    }

    std::wstring wLogPath = s2ws(logPath.string());
    HANDLE hLog = CreateFileW(
        wLogPath.c_str(),
        FILE_APPEND_DATA | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        &sa,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (hLog != INVALID_HANDLE_VALUE) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = INVALID_HANDLE_VALUE;
        si.hStdOutput = hLog;
        si.hStdError = hLog;
    }
    ZeroMemory(&pi, sizeof(pi));

    DWORD createFlags = CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW;
    BOOL success = CreateProcessW(
        NULL,
        &cmdLine[0],
        NULL,
        NULL,
        (hLog != INVALID_HANDLE_VALUE) ? TRUE : FALSE,
        createFlags,
        NULL,
        workDir.empty() ? NULL : workDir.c_str(),
        &si,
        &pi
    );

    if (hLog != INVALID_HANDLE_VALUE) {
        CloseHandle(hLog);
    }

    if (success) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Logger::instance().info("[SERVICE] Ollama server process spawned in background (PID: " + std::to_string(pi.dwProcessId) + ")");
        return true;
    }

    // Fallback on Windows to PowerShell Start-Process
    std::string psCmd = "Start-Process -FilePath '" + targetExe + "' -ArgumentList 'serve' -WindowStyle Hidden";
    ProcessResult psRes = ProcessExecutor::executeShell(psCmd, workDir.empty() ? "." : exeFsPath.parent_path().string(), 3000);
    return psRes.exitCode == 0;

#else
    // Linux / POSIX: Native detached fork or systemctl service
    // First, try systemd user service if active
    ProcessResult sysd = ProcessExecutor::executeShell("systemctl --user start ollama 2>/dev/null || sudo systemctl start ollama 2>/dev/null", ".", 3000);
    if (sysd.exitCode == 0) {
        Logger::instance().info("[SERVICE] Started Ollama via systemd service.");
        return true;
    }

    // Direct POSIX daemonize
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        // Child daemon
        setsid();
        int devNull = open("/dev/null", O_RDWR);
        if (devNull >= 0) {
            dup2(devNull, STDIN_FILENO);
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }
        execlp(targetExe.c_str(), "ollama", "serve", (char*)NULL);
        _exit(127);
    }

    Logger::instance().info("[SERVICE] Ollama background daemon spawned (PID: " + std::to_string(pid) + ")");
    return true;
#endif
}

bool ServiceDaemon::ensureOllamaRunning(const std::string& baseUrl, int timeoutSeconds) {
    std::string url = normalizeUrl(baseUrl);

    // 1. If already responding, return immediately
    if (isOllamaResponding(url)) {
        return true;
    }

    // 2. Thread-safe rate limiting: at most 1 launch attempt every 8 seconds
    static std::mutex s_launchMutex;
    std::lock_guard<std::mutex> lock(s_launchMutex);

    // Re-check response under lock in case another thread just started it
    if (isOllamaResponding(url)) {
        return true;
    }

    static bool s_hasAttempted = false;
    static auto s_lastAttempt = std::chrono::steady_clock::now();
    if (s_hasAttempted) {
        auto now = std::chrono::steady_clock::now();
        auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(now - s_lastAttempt).count();
        if (elapsedSec < 8) {
            // A launch was already triggered in the last 8 seconds; wait for readiness rather than failing immediately
            int attempts = (timeoutSeconds <= 0 ? 25 : timeoutSeconds) * 2;
            for (int i = 0; i < attempts; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (isOllamaResponding(url)) return true;
            }
            return isOllamaResponding(url);
        }
    }
    s_hasAttempted = true;
    s_lastAttempt = std::chrono::steady_clock::now();

    // 3. Check if an Ollama process is already running on the OS
    if (isOllamaProcessRunning()) {
        Logger::instance().info("[SERVICE] Ollama process found on system; checking port readiness...");
        for (int i = 0; i < 4; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (isOllamaResponding(url)) return true;
        }
        Logger::instance().info("[SERVICE] Ollama process active but server port not responding; initiating background serve daemon...");
    }

    // 4. Locate Ollama executable
    std::string exe = findOllamaExecutable();
    if (exe.empty()) {
        Logger::instance().debug("[SERVICE] Ollama binary not found on system. Auto-start bypassed.");
        return false;
    }

    Logger::instance().info("[SERVICE] Ollama server is offline. Autonomously launching daemon from: " + exe);

    // 5. Launch daemon
    if (!launchOllamaBackground(exe)) {
        Logger::instance().warn("[SERVICE] Failed to launch Ollama background process.");
        return false;
    }

    // 6. Actively poll for readiness
    int maxAttempts = (timeoutSeconds <= 0 ? 25 : timeoutSeconds) * 2; // Check every 500ms
    for (int i = 0; i < maxAttempts; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (isOllamaResponding(url)) {
            Logger::instance().info("[SERVICE] Ollama server is now ONLINE and responding at " + url);
            return true;
        }
    }

    Logger::instance().warn("[SERVICE] Ollama process launched but did not respond within timeout.");
    return false;
}

std::vector<std::string> ServiceDaemon::getInstalledOllamaModels(const std::string& baseUrl) {
    std::vector<std::string> models;
    std::string url = normalizeUrl(baseUrl);
    try {
        HttpResponse resp = HttpClient::instance().get(url + "/api/tags", {}, 3);
        if (resp.success && !resp.body.empty()) {
            nlohmann::json j = nlohmann::json::parse(resp.body);
            if (j.contains("models") && j["models"].is_array()) {
                for (const auto& m : j["models"]) {
                    if (m.contains("name") && m["name"].is_string()) {
                        models.push_back(m["name"].get<std::string>());
                    }
                }
            }
        }
    } catch (...) {}
    return models;
}

bool ServiceDaemon::ensureModelAvailable(
    const std::string& baseUrl,
    const std::string& modelName,
    int timeoutSeconds
) {
    if (modelName.empty()) return false;
    std::string url = normalizeUrl(baseUrl);

    auto existing = getInstalledOllamaModels(url);
    for (const auto& m : existing) {
        if (m == modelName || m.rfind(modelName + ":", 0) == 0 || modelName.rfind(m + ":", 0) == 0) {
            return true;
        }
    }

    Logger::instance().info("[SERVICE] Autonomously pulling model '" + modelName + "' from Ollama registry...");

    nlohmann::json pullPayload = {
        {"name", modelName},
        {"stream", false}
    };

    HttpResponse pullResp = HttpClient::instance().post(
        url + "/api/pull",
        pullPayload.dump(),
        {{"Content-Type", "application/json"}},
        timeoutSeconds
    );

    if (pullResp.success) {
        Logger::instance().info("[SERVICE] Successfully pulled model '" + modelName + "' into Ollama!");
        return true;
    }

    // Fallback to CLI command if HTTP API pull timed out
    std::string cliPull = "ollama pull " + modelName;
    ProcessResult pr = ProcessExecutor::executeShell(cliPull, ".", timeoutSeconds * 1000);
    return pr.exitCode == 0;
}

bool ServiceDaemon::autoStartLocalBackends(const std::string& ollamaUrl) {
    return ensureOllamaRunning(ollamaUrl, 10);
}

} // namespace arenafight

