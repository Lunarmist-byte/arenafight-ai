#include "arenafight/services/llama_server_daemon.hpp"
#include "arenafight/common/http_client.hpp"
#include "arenafight/common/process.hpp"
#include "arenafight/common/logger.hpp"
#include "arenafight/common/fs_utils.hpp"
#include <nlohmann/json.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <mutex>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

namespace {

#if defined(_WIN32)
std::wstring s2ws(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

DWORD s_serverPid = 0;
#else
pid_t s_serverPid = 0;
#endif

std::string normalizeUrl(const std::string& url) {
    if (url.empty()) return "http://127.0.0.1:8080";
    std::string clean = url;
    while (!clean.empty() && clean.back() == '/') {
        clean.pop_back();
    }
    return clean;
}

std::string extractPort(const std::string& url) {
    size_t colon = url.rfind(':');
    if (colon != std::string::npos && colon + 1 < url.size()) {
        std::string p = url.substr(colon + 1);
        size_t slash = p.find('/');
        if (slash != std::string::npos) p = p.substr(0, slash);
        return p;
    }
    return "8080";
}

} // namespace

std::filesystem::path LlamaServerDaemon::getOllamaModelsDir() {
    const char* envModels = std::getenv("OLLAMA_MODELS");
    if (envModels && std::filesystem::exists(envModels)) {
        return std::filesystem::path(envModels);
    }

#if defined(_WIN32)
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        std::filesystem::path p = std::filesystem::path(userProfile) / ".ollama" / "models";
        if (std::filesystem::exists(p)) return p;
    }
    const char* localApp = std::getenv("LOCALAPPDATA");
    if (localApp) {
        std::filesystem::path p = std::filesystem::path(localApp) / ".ollama" / "models";
        if (std::filesystem::exists(p)) return p;
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        std::filesystem::path p = std::filesystem::path(home) / ".ollama" / "models";
        if (std::filesystem::exists(p)) return p;
    }
#endif

    return "";
}

std::vector<LocalGgufModel> LlamaServerDaemon::discoverModels() {
    std::vector<LocalGgufModel> models;

    // 1. Scan Ollama model manifests & blobs
    std::filesystem::path modelsDir = getOllamaModelsDir();
    if (!modelsDir.empty() && std::filesystem::exists(modelsDir)) {
        std::filesystem::path manifestsDir = modelsDir / "manifests";
        std::filesystem::path blobsDir = modelsDir / "blobs";

        if (std::filesystem::exists(manifestsDir) && std::filesystem::exists(blobsDir)) {
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(manifestsDir)) {
                    if (!entry.is_regular_file()) continue;

                    std::string manifestContent = FileUtils::readTextFile(entry.path().string());
                    if (manifestContent.empty()) continue;

                    nlohmann::json j = nlohmann::json::parse(manifestContent, nullptr, false);
                    if (j.is_discarded() || !j.contains("layers") || !j["layers"].is_array()) {
                        continue;
                    }

                    // Look for layer with model GGUF blob
                    std::string modelDigest;
                    uint64_t modelSize = 0;
                    for (const auto& layer : j["layers"]) {
                        std::string mediaType = layer.value("mediaType", "");
                        if (mediaType == "application/vnd.ollama.image.model") {
                            modelDigest = layer.value("digest", "");
                            modelSize = layer.value("size", 0ULL);
                            break;
                        }
                    }

                    if (modelDigest.empty()) continue;

                    // Convert "sha256:abcd..." to "sha256-abcd..."
                    std::string blobName = modelDigest;
                    size_t colon = blobName.find(':');
                    if (colon != std::string::npos) {
                        blobName.replace(colon, 1, "-");
                    }

                    std::filesystem::path blobPath = blobsDir / blobName;
                    if (!std::filesystem::exists(blobPath)) {
                        continue;
                    }

                    // Model tag is filename; model name is parent directory
                    std::string tag = entry.path().filename().string();
                    std::string modelName = entry.path().parent_path().filename().string();
                    std::string fullId = modelName + ":" + tag;

                    LocalGgufModel gm;
                    gm.id = fullId;
                    gm.name = fullId;
                    gm.ggufPath = blobPath.string();
                    gm.sizeBytes = modelSize;
                    gm.isOllamaImported = true;
                    gm.family = modelName;
                    gm.parameterSize = tag;
                    gm.quantization = "Q4_K_M";
                    gm.contextWindow = 8192;

                    models.push_back(gm);

                    // Also add alias without tag if latest
                    if (tag == "latest") {
                        LocalGgufModel alias = gm;
                        alias.id = modelName;
                        alias.name = modelName;
                        models.push_back(alias);
                    }
                }
            } catch (...) {}
        }
    }

    // 2. Scan local project directories for .gguf files
    std::vector<std::string> localDirs = {".", "models"};
    for (const auto& dir : localDirs) {
        if (!std::filesystem::exists(dir)) continue;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
                    LocalGgufModel gm;
                    gm.id = entry.path().stem().string();
                    gm.name = gm.id;
                    gm.ggufPath = std::filesystem::absolute(entry.path()).string();
                    try {
                        gm.sizeBytes = std::filesystem::file_size(entry.path());
                    } catch (...) {}
                    gm.isOllamaImported = false;
                    gm.contextWindow = 8192;
                    models.push_back(gm);
                }
            }
        } catch (...) {}
    }

    return models;
}

std::string LlamaServerDaemon::resolveModelGgufPath(const std::string& modelId) {
    if (modelId.empty()) return "";

    // 1. Direct path check
    if (std::filesystem::exists(modelId)) {
        return std::filesystem::absolute(modelId).string();
    }

    auto allModels = discoverModels();
    if (allModels.empty()) return "";

    // 2. Exact match
    for (const auto& m : allModels) {
        if (m.id == modelId || m.name == modelId) {
            return m.ggufPath;
        }
    }

    // 3. Substring / case-insensitive match
    std::string lowerTarget = modelId;
    for (char& c : lowerTarget) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    for (const auto& m : allModels) {
        std::string lowerId = m.id;
        for (char& c : lowerId) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lowerId.find(lowerTarget) != std::string::npos || lowerTarget.find(lowerId) != std::string::npos) {
            return m.ggufPath;
        }
    }

    // Fallback: return first available model
    return allModels[0].ggufPath;
}

std::string LlamaServerDaemon::findLlamaServerExecutable() {
#if defined(_WIN32)
    // 1. Check standard PATH lookup
    ProcessResult pathLookup = ProcessExecutor::executeShell("where.exe llama-server", ".", 2000);
    if (pathLookup.exitCode == 0 && !pathLookup.stdOut.empty()) {
        std::string out = pathLookup.stdOut;
        size_t nl = out.find('\n');
        if (nl != std::string::npos) out = out.substr(0, nl);
        while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) {
            out.pop_back();
        }
        if (std::filesystem::exists(out)) return out;
    }

    // 2. Check bundled llama-server in Ollama library directory
    const char* localApp = std::getenv("LOCALAPPDATA");
    if (localApp) {
        std::filesystem::path p = std::filesystem::path(localApp) / "Programs" / "Ollama" / "lib" / "ollama" / "llama-server.exe";
        if (std::filesystem::exists(p)) return p.string();
    }

    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        std::filesystem::path p = std::filesystem::path(userProfile) / "AppData" / "Local" / "Programs" / "Ollama" / "lib" / "ollama" / "llama-server.exe";
        if (std::filesystem::exists(p)) return p.string();
    }

    std::vector<std::string> winStandardPaths = {
        "C:\\Program Files\\llama.cpp\\llama-server.exe",
        "C:\\llama.cpp\\llama-server.exe",
        "C:\\tools\\llama.cpp\\llama-server.exe",
        ".\\llama-server.exe",
        "build\\llama-server.exe"
    };
    for (const auto& sp : winStandardPaths) {
        if (std::filesystem::exists(sp)) return std::filesystem::absolute(sp).string();
    }
#else
    ProcessResult pathLookup = ProcessExecutor::executeShell("which llama-server 2>/dev/null || command -v llama-server 2>/dev/null", ".", 2000);
    if (pathLookup.exitCode == 0 && !pathLookup.stdOut.empty()) {
        std::string out = pathLookup.stdOut;
        size_t nl = out.find('\n');
        if (nl != std::string::npos) out = out.substr(0, nl);
        while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) {
            out.pop_back();
        }
        if (std::filesystem::exists(out)) return out;
    }

    std::vector<std::string> posixPaths = {
        "/usr/local/bin/llama-server",
        "/usr/bin/llama-server",
        "/opt/llama.cpp/bin/llama-server",
        "./llama-server"
    };
    for (const auto& sp : posixPaths) {
        if (std::filesystem::exists(sp)) return sp;
    }
#endif
    return "";
}

bool LlamaServerDaemon::isServerResponding(const std::string& baseUrl) {
    std::string url = normalizeUrl(baseUrl);
    try {
        HttpResponse resp = HttpClient::instance().get(url + "/health", {}, 1);
        if (resp.success) return true;
        // 503 indicates server is online and currently loading model weights into VRAM
        if (resp.statusCode == 503) return true;
    } catch (...) {}
    return false;
}

std::string LlamaServerDaemon::getLoadedModel(const std::string& baseUrl) {
    std::string url = normalizeUrl(baseUrl);
    try {
        HttpResponse resp = HttpClient::instance().get(url + "/v1/models", {}, 2);
        if (resp.success && !resp.body.empty()) {
            nlohmann::json j = nlohmann::json::parse(resp.body, nullptr, false);
            if (!j.is_discarded() && j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
                return j["data"][0].value("id", "");
            }
        }
    } catch (...) {}
    return "";
}

bool LlamaServerDaemon::stopServer() {
#if defined(_WIN32)
    if (s_serverPid != 0) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, s_serverPid);
        if (hProc) {
            TerminateProcess(hProc, 0);
            CloseHandle(hProc);
            s_serverPid = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return true;
        }
    }
#else
    if (s_serverPid != 0) {
        kill(s_serverPid, SIGTERM);
        s_serverPid = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return true;
    }
#endif
    return false;
}

bool LlamaServerDaemon::ensureServerRunning(
    const std::string& modelIdOrGguf,
    const std::string& baseUrl,
    int gpuLayers,
    int contextSize,
    int timeoutSeconds
) {
    std::string url = normalizeUrl(baseUrl);

    // 1. Resolve GGUF model path
    std::string ggufPath = resolveModelGgufPath(modelIdOrGguf);
    if (ggufPath.empty()) {
        Logger::instance().warn("[LLAMA-SERVER] Cannot find GGUF model file for: " + modelIdOrGguf);
        return false;
    }

    // 2. Check if server is already running with this model
    if (isServerResponding(url)) {
        std::string curLoaded = getLoadedModel(url);
        if (curLoaded == modelIdOrGguf || curLoaded.find(modelIdOrGguf) != std::string::npos ||
            modelIdOrGguf.find(curLoaded) != std::string::npos) {
            return true;
        }
        // Different model requested: stop server and reload
        stopServer();
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }

    // 3. Thread-safe launch lock
    static std::mutex s_mutex;
    std::lock_guard<std::mutex> lock(s_mutex);

    if (isServerResponding(url)) {
        return true;
    }

    // 4. Locate llama-server executable
    std::string targetExe = findLlamaServerExecutable();
    if (targetExe.empty()) {
        Logger::instance().warn("[LLAMA-SERVER] llama-server executable not found on host.");
        return false;
    }

    std::string portStr = extractPort(url);

    Logger::instance().info("[LLAMA-SERVER] Starting llama.cpp server for model: " + modelIdOrGguf);
    Logger::instance().info("[LLAMA-SERVER] GGUF path: " + ggufPath);

#if defined(_WIN32)
    std::filesystem::path exeFsPath(targetExe);
    std::filesystem::path workDir = exeFsPath.parent_path();

    // Check for CUDA library subdirectories (e.g. cuda_v13, cuda_v12)
    std::filesystem::path cudaDir13 = workDir / "cuda_v13";
    std::filesystem::path cudaDir12 = workDir / "cuda_v12";
    std::string cudaPathAddition;
    if (std::filesystem::exists(cudaDir13)) {
        cudaPathAddition = cudaDir13.string() + ";" + workDir.string() + ";";
    } else if (std::filesystem::exists(cudaDir12)) {
        cudaPathAddition = cudaDir12.string() + ";" + workDir.string() + ";";
    } else {
        cudaPathAddition = workDir.string() + ";";
    }

    const char* curPath = std::getenv("PATH");
    std::string newPath = "PATH=" + cudaPathAddition + (curPath ? std::string(curPath) : "");
    _putenv(newPath.c_str());

    // Build command line: -m <path> --port <port> -ngl <layers> -c <ctx> --alias <name> --host 127.0.0.1
    std::wstring cmdLine = L"\"" + s2ws(targetExe) + L"\" -m \"" + s2ws(ggufPath) +
                           L"\" --port " + s2ws(portStr) +
                           L" -ngl " + std::to_wstring(gpuLayers) +
                           L" -c " + std::to_wstring(contextSize) +
                           L" --alias \"" + s2ws(modelIdOrGguf) + L"\" --host 127.0.0.1";

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    std::filesystem::path logPath;
    const char* tempEnv = std::getenv("TEMP");
    if (tempEnv && std::filesystem::exists(tempEnv)) {
        logPath = std::filesystem::path(tempEnv) / "llama_server.log";
    } else {
        logPath = workDir / "llama_server.log";
    }

    HANDLE hLog = CreateFileW(
        s2ws(logPath.string()).c_str(),
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

    BOOL success = CreateProcessW(
        NULL,
        &cmdLine[0],
        NULL,
        NULL,
        (hLog != INVALID_HANDLE_VALUE) ? TRUE : FALSE,
        CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW,
        NULL,
        s2ws(workDir.string()).c_str(),
        &si,
        &pi
    );

    if (hLog != INVALID_HANDLE_VALUE) {
        CloseHandle(hLog);
    }

    if (!success) {
        Logger::instance().warn("[LLAMA-SERVER] Failed to spawn llama-server process. Error: " + std::to_string(GetLastError()));
        return false;
    }

    s_serverPid = pi.dwProcessId;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

#else
    pid_t pid = fork();
    if (pid < 0) return false;

    if (pid == 0) {
        setsid();
        int devNull = open("/dev/null", O_RDWR);
        if (devNull >= 0) {
            dup2(devNull, STDIN_FILENO);
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }
        execlp(targetExe.c_str(), "llama-server",
               "-m", ggufPath.c_str(),
               "--port", portStr.c_str(),
               "-ngl", std::to_string(gpuLayers).c_str(),
               "-c", std::to_string(contextSize).c_str(),
               "--alias", modelIdOrGguf.c_str(),
               "--host", "127.0.0.1",
               (char*)NULL);
        _exit(127);
    }
    s_serverPid = pid;
#endif

    // 5. Poll for readiness (checks /health until status is "ok")
    int maxChecks = (timeoutSeconds <= 0 ? 25 : timeoutSeconds) * 2;
    for (int i = 0; i < maxChecks; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        try {
            HttpResponse resp = HttpClient::instance().get(url + "/health", {}, 1);
            if (resp.success && resp.body.find("\"ok\"") != std::string::npos) {
                Logger::instance().info("[LLAMA-SERVER] llama.cpp server is ONLINE and serving at " + url);
                return true;
            }
        } catch (...) {}
    }

    // Secondary check: verify if HTTP 200 or /v1/models is responding
    return isServerResponding(url);
}

bool LlamaServerDaemon::pullOllamaModel(const std::string& modelName, int timeoutSeconds) {
    if (modelName.empty()) return false;
    Logger::instance().info("[LLAMA-SERVER] Autonomously pulling model '" + modelName + "' into local GGUF store...");

    std::string cmd = "ollama pull " + modelName;
    ProcessResult pr = ProcessExecutor::executeShell(cmd, ".", timeoutSeconds * 1000);
    return pr.exitCode == 0;
}

} // namespace arenafight

