#include "arenafight/security/security_sandbox.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <chrono>
#include <cstring>

namespace arenafight {

namespace {

// Standard SHA-256 Implementation (FIPS 180-4)
class Sha256Impl {
public:
    Sha256Impl() {
        state[0] = 0x6a09e667;
        state[1] = 0xbb67ae85;
        state[2] = 0x3c6ef372;
        state[3] = 0xa54ff53a;
        state[4] = 0x510e527f;
        state[5] = 0x9b05688c;
        state[6] = 0x1f83d9ab;
        state[7] = 0x5be0cd19;
        count = 0;
    }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buffer[count % 64] = data[i];
            count++;
            if (count % 64 == 0) {
                transform(buffer);
            }
        }
    }

    std::string finalize() {
        uint8_t finalBlock[64];
        size_t bufferLen = count % 64;
        std::memcpy(finalBlock, buffer, bufferLen);

        finalBlock[bufferLen] = 0x80;
        bufferLen++;

        if (bufferLen > 56) {
            std::memset(finalBlock + bufferLen, 0, 64 - bufferLen);
            transform(finalBlock);
            std::memset(finalBlock, 0, 56);
        } else {
            std::memset(finalBlock + bufferLen, 0, 56 - bufferLen);
        }

        uint64_t totalBits = count * 8;
        for (int i = 0; i < 8; ++i) {
            finalBlock[63 - i] = static_cast<uint8_t>(totalBits >> (i * 8));
        }
        transform(finalBlock);

        std::stringstream ss;
        for (int i = 0; i < 8; ++i) {
            ss << std::hex << std::setfill('0') << std::setw(8) << state[i];
        }
        return ss.str();
    }

private:
    uint32_t state[8];
    uint8_t buffer[64];
    uint64_t count;

    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    static uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static uint32_t gam0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t gam1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    static const uint32_t K[64];

    void transform(const uint8_t* block) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(block[i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = gam1(w[i - 2]) + w[i - 7] + gam0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sig1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }
};

const uint32_t Sha256Impl::K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

std::string getCurrentIsoTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

std::string normalizeSeparators(const std::string& path) {
    std::string s = path;
    for (char& c : s) {
        if (c == '\\') c = '/';
    }
    return s;
}

} // namespace

std::string SecuritySandbox::sha256(const std::string& input) {
    Sha256Impl ctx;
    ctx.update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    return ctx.finalize();
}

bool SecuritySandbox::isProtectedFile(const std::string& path) {
    std::string norm = normalizeSeparators(path);
    std::string lower = norm;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Extract filename
    size_t lastSlash = lower.find_last_of('/');
    std::string filename = (lastSlash == std::string::npos) ? lower : lower.substr(lastSlash + 1);

    if (filename == ".env" || filename == ".env.local" || filename == ".env.production" ||
        filename == ".env.development" || filename == ".env.test") {
        return true;
    }

    if (filename == "id_rsa" || filename == "id_ed25519" || filename == "id_ecdsa" || filename == "id_dsa") {
        return true;
    }

    if (filename == "credentials" || filename == "credentials.json" || filename == "secrets.json") {
        return true;
    }

    // Extensions
    if (filename.ends_with(".pem") || filename.ends_with(".pfx") || filename.ends_with(".key") || filename.ends_with(".p12")) {
        return true;
    }

    // Git internal credentials
    if (lower.find(".git/config") != std::string::npos || lower.find(".git/credentials") != std::string::npos) {
        return true;
    }

    return false;
}

bool SecuritySandbox::isPathSafe(
    const std::string& targetPath,
    const std::string& workspaceDir,
    std::string* violationReason,
    bool allowWorkspaceSubdirsOnly
) {
    if (targetPath.empty()) {
        if (violationReason) *violationReason = "Path is empty";
        return false;
    }

    std::string normTarget = normalizeSeparators(targetPath);

    // Check for Windows alternate data streams (e.g. file.txt::$DATA)
    if (normTarget.find("::$") != std::string::npos) {
        if (violationReason) *violationReason = "Alternate Data Stream access is forbidden: " + targetPath;
        return false;
    }

    // Check for Windows reserved device names
    static const std::vector<std::string> reservedDevices = {
        "con", "prn", "aux", "nul",
        "com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9",
        "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"
    };
    std::string lowerTarget = normTarget;
    std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);
    for (const auto& dev : reservedDevices) {
        if (lowerTarget == dev || lowerTarget.starts_with(dev + ".") ||
            lowerTarget.find("/" + dev + ".") != std::string::npos ||
            lowerTarget.ends_with("/" + dev)) {
            if (violationReason) *violationReason = "Reserved device name access is forbidden: " + targetPath;
            return false;
        }
    }

    // Resolve workspace canonical path
    std::error_code ec;
    std::filesystem::path wsPath = std::filesystem::weakly_canonical(std::filesystem::path(workspaceDir), ec);
    if (ec) wsPath = std::filesystem::absolute(std::filesystem::path(workspaceDir));

    std::filesystem::path fullTargetPath;
    if (std::filesystem::path(targetPath).is_absolute()) {
        fullTargetPath = std::filesystem::weakly_canonical(std::filesystem::path(targetPath), ec);
    } else {
        fullTargetPath = std::filesystem::weakly_canonical(wsPath / std::filesystem::path(targetPath), ec);
    }
    if (ec) {
        fullTargetPath = std::filesystem::path(targetPath).lexically_normal();
    }

    std::string normWs = normalizeSeparators(wsPath.string());
    std::string normResolved = normalizeSeparators(fullTargetPath.string());

    // Case-insensitive normalization for comparison on Windows
    std::string lowerWs = normWs;
    std::string lowerResolved = normResolved;
    std::transform(lowerWs.begin(), lowerWs.end(), lowerWs.begin(), ::tolower);
    std::transform(lowerResolved.begin(), lowerResolved.end(), lowerResolved.begin(), ::tolower);

    // Strip trailing slashes
    while (lowerWs.length() > 1 && lowerWs.back() == '/') lowerWs.pop_back();
    while (lowerResolved.length() > 1 && lowerResolved.back() == '/') lowerResolved.pop_back();

    if (allowWorkspaceSubdirsOnly) {
        if (lowerResolved != lowerWs && !lowerResolved.starts_with(lowerWs + "/")) {
            if (violationReason) {
                *violationReason = "Path traversal detected outside workspace sandbox: '" + targetPath +
                                   "' escapes '" + workspaceDir + "'";
            }
            return false;
        }
    }

    return true;
}

std::string SecuritySandbox::resolveSafePath(
    const std::string& targetPath,
    const std::string& workspaceDir,
    std::string* violationReason
) {
    if (!isPathSafe(targetPath, workspaceDir, violationReason, true)) {
        return "";
    }
    std::filesystem::path p(targetPath);
    if (p.is_absolute()) return p.string();
    return (std::filesystem::path(workspaceDir) / p).lexically_normal().string();
}

CommandRisk SecuritySandbox::analyzeCommandRisk(const std::string& command) {
    std::string lower = command;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // 1. FORBIDDEN PATTERNS: Malicious attacks, reverse shells, remote code execution, system wipe
    static const std::vector<std::string> forbiddenSubstrings = {
        // Reverse shells & networking payloads
        "nc -e", "ncat -e", "/dev/tcp/", "/dev/udp/", "mkfifo /tmp/", "socat exec:",
        // Download piped to shell
        "curl | sh", "curl | bash", "wget | bash", "curl | powershell", "wget | sh",
        "| iex", "|iex", "invoke-expression", "invoke-webrequest",
        "iex (iwr", "iex (curl", "iex(iwr", "iex(curl", "iex (", "iex(",
        // Malicious utilities
        "certutil -urlcache", "certutil.exe -urlcache", "bitsadmin /transfer",
        "mshta http", "regsvr32 /s /u /i:",
        // Fork bombs
        ":(){ :|:& };:", "0<${0}",
        // Complete filesystem wipe
        "rm -rf /", "rm -rf ~", "rm -rf *", "rmdir /s /q c:\\", "del /f /s /q c:\\",
        "format c:", "format d:", "mkfs.ext4", "mkfs.vfat", "dd if=/dev/",
        // Database destruction
        "drop database", "drop table", "truncate table",
        // System restart / shutdown
        "shutdown -h", "shutdown /s", "shutdown /r", "init 0", "reboot",
        // Registry destroy
        "reg delete hklm", "reg delete hkcu",
        // Covert tunneling
        "ngrok http", "cloudflared tunnel"
    };

    for (const auto& bad : forbiddenSubstrings) {
        if (lower.find(bad) != std::string::npos) {
            return CommandRisk::FORBIDDEN;
        }
    }

    // Regex check for encoded powershell commands or curl | sh variants
    static const std::regex encodedPsRegex("powershell.*-(e|enc|encodedcommand)\\s+[a-za-z0-9+/=]{10,}", std::regex::icase);
    if (std::regex_search(lower, encodedPsRegex)) {
        return CommandRisk::FORBIDDEN;
    }

    static const std::regex curlPipeRegex("curl\\s+[^|]+\\|\\s*(sh|bash|powershell|python)", std::regex::icase);
    if (std::regex_search(lower, curlPipeRegex)) {
        return CommandRisk::FORBIDDEN;
    }

    static const std::regex wgetPipeRegex("wget\\s+[^|]+\\|\\s*(sh|bash|powershell|python)", std::regex::icase);
    if (std::regex_search(lower, wgetPipeRegex)) {
        return CommandRisk::FORBIDDEN;
    }

    static const std::regex iexPipeRegex("\\|\\s*(iex|invoke-expression)", std::regex::icase);
    if (std::regex_search(lower, iexPipeRegex)) {
        return CommandRisk::FORBIDDEN;
    }

    static const std::regex rmAllRegex("rm\\s+-rf?\\s+([/\\*]+|\\.\\.)", std::regex::icase);
    if (std::regex_search(lower, rmAllRegex)) {
        return CommandRisk::FORBIDDEN;
    }

    // 2. RESTRICTED: Potentially destructive local operations
    static const std::vector<std::string> restrictedSubstrings = {
        "git reset --hard", "git clean -fd", "git clean -f",
        "rm -rf", "rmdir /s /q", "del /f /s /q", "del /s /q",
        "kill -9", "taskkill /f", "git push --force"
    };
    for (const auto& res : restrictedSubstrings) {
        if (lower.find(res) != std::string::npos) {
            return CommandRisk::RESTRICTED;
        }
    }

    // 3. SAFE: Read-only, inspection, build, or unit test commands
    static const std::vector<std::string> safePrefixes = {
        "echo ", "git status", "git diff", "git log", "git branch",
        "cmake", "ninja", "ctest", "make", "g++", "gcc", "clang",
        "python", "py", "pytest", "cargo build", "cargo test",
        "ls", "dir", "cat ", "type ", "pwd", "cd ", "whoami"
    };
    for (const auto& safe : safePrefixes) {
        if (lower.starts_with(safe) || lower == safe.substr(0, safe.length() - 1)) {
            return CommandRisk::SAFE;
        }
    }

    return CommandRisk::AUDITED;
}

bool SecuritySandbox::isCommandPermitted(
    const std::string& command,
    SecurityLevel level,
    std::string* violationReason
) {
    CommandRisk risk = analyzeCommandRisk(command);

    if (risk == CommandRisk::FORBIDDEN) {
        if (violationReason) {
            *violationReason = "FORBIDDEN: Command flagged as critical security threat (reverse shell, system wipe, or unverified script download).";
        }
        return false;
    }

    if (level == SecurityLevel::STRICT) {
        if (risk == CommandRisk::RESTRICTED) {
            if (violationReason) {
                *violationReason = "STRICT LOCKDOWN: Destructive operations are prohibited in strict security mode.";
            }
            return false;
        }
    }

    return true;
}

std::string SecuritySandbox::redactSecrets(const std::string& text) {
    std::string result = text;

    // 1. OpenAI Keys (sk-...)
    static const std::regex openAiRegex("sk-[A-Za-z0-9_-]{20,}");
    result = std::regex_replace(result, openAiRegex, "sk-***[REDACTED]");

    // 2. Anthropic Keys (sk-ant-...)
    static const std::regex anthropicRegex("sk-ant-[A-Za-z0-9_-]{20,}");
    result = std::regex_replace(result, anthropicRegex, "sk-ant-***[REDACTED]");

    // 3. OpenRouter Keys (sk-or-v1-...)
    static const std::regex openRouterRegex("sk-or-v1-[A-Za-z0-9]{32,}");
    result = std::regex_replace(result, openRouterRegex, "sk-or-v1-***[REDACTED]");

    // 4. GitHub Tokens (ghp_..., github_pat_...)
    static const std::regex githubRegex("ghp_[A-Za-z0-9]{36}|github_pat_[A-Za-z0-9_]{50,}");
    result = std::regex_replace(result, githubRegex, "ghp_***[REDACTED]");

    // 5. AWS Access Key (AKIA...)
    static const std::regex awsRegex("AKIA[0-9A-Z]{16}");
    result = std::regex_replace(result, awsRegex, "AKIA***[REDACTED]");

    // 6. Generic Bearer Tokens
    static const std::regex bearerRegex("Bearer\\s+([A-Za-z0-9_\\-\\.\\+]{20,})", std::regex::icase);
    result = std::regex_replace(result, bearerRegex, "Bearer [REDACTED_TOKEN]");

    // 7. Generic API key assignments
    static const std::regex keyAssignRegex("(api[_-]?key|secret|password|passwd|auth_token)[\"']?\\s*[:=]\\s*[\"']?([A-Za-z0-9_\\-]{16,})[\"']?", std::regex::icase);
    result = std::regex_replace(result, keyAssignRegex, "$1=[REDACTED]");

    // 8. Private Key Blocks
    static const std::regex privateKeyRegex("-----BEGIN [A-Z ]*PRIVATE KEY-----[\\s\\S]*?-----END [A-Z ]*PRIVATE KEY-----");
    result = std::regex_replace(result, privateKeyRegex, "[REDACTED_PRIVATE_KEY_BLOCK]");

    return result;
}

bool SecuritySandbox::containsUnmaskedSecret(const std::string& text, std::string* matchedType) {
    if (std::regex_search(text, std::regex("sk-[A-Za-z0-9_-]{24,}"))) {
        if (matchedType) *matchedType = "OpenAI API Key";
        return true;
    }
    if (std::regex_search(text, std::regex("sk-ant-[A-Za-z0-9_-]{24,}"))) {
        if (matchedType) *matchedType = "Anthropic API Key";
        return true;
    }
    if (std::regex_search(text, std::regex("sk-or-v1-[A-Za-z0-9]{32,}"))) {
        if (matchedType) *matchedType = "OpenRouter API Key";
        return true;
    }
    if (std::regex_search(text, std::regex("ghp_[A-Za-z0-9]{36}"))) {
        if (matchedType) *matchedType = "GitHub Personal Access Token";
        return true;
    }
    if (std::regex_search(text, std::regex("AKIA[0-9A-Z]{16}"))) {
        if (matchedType) *matchedType = "AWS Access Key ID";
        return true;
    }
    static const std::regex privateKeyRealRegex("-----BEGIN [A-Z ]*PRIVATE KEY-----\\r?\\n[A-Za-z0-9+/=\\s]{20,}\\r?\\n-----END");
    if (std::regex_search(text, privateKeyRealRegex)) {
        if (matchedType) *matchedType = "Cryptographic Private Key Block";
        return true;
    }
    return false;
}

bool SecuritySandbox::detectPromptInjection(const std::string& text, std::string* matchedPattern) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    static const std::vector<std::string> injectionPayloads = {
        "ignore previous instructions",
        "disregard all prior instructions",
        "ignore all prior instructions",
        "forget all instructions",
        "you are now in developer mode",
        "jailbreak enabled",
        "dan mode",
        "<system>",
        "[system instruction]",
        "<|im_start|>system",
        "you are no longer an ai agent"
    };

    for (const auto& payload : injectionPayloads) {
        if (lower.find(payload) != std::string::npos) {
            if (matchedPattern) *matchedPattern = payload;
            return true;
        }
    }
    return false;
}

std::string SecuritySandbox::sanitizeAdversarialContent(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    // Filter dangerous invisible unicode control chars (Trojan Source, zero-width, null bytes)
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\0') continue; // Strip null bytes

        // BiDi override character detection (UTF-8 multi-byte sequences for \u202A - \u202E, \u2066 - \u2069)
        if (c == 0xE2 && i + 2 < text.size()) {
            unsigned char b1 = static_cast<unsigned char>(text[i + 1]);
            unsigned char b2 = static_cast<unsigned char>(text[i + 2]);
            if (b1 == 0x80 && (b2 >= 0xAA && b2 <= 0xAE)) {
                i += 2; // Skip Trojan Source BiDi overrides
                continue;
            }
            if (b1 == 0x81 && (b2 >= 0xA6 && b2 <= 0xA9)) {
                i += 2; // Skip BiDi isolates
                continue;
            }
        }
        result.push_back(text[i]);
    }
    return result;
}

nlohmann::json SecurityAuditRecord::toJson() const {
    return {
        {"index", index},
        {"timestamp", timestamp},
        {"event", event},
        {"severity", severity},
        {"caller", caller},
        {"details", details},
        {"prev_hash", prevHash},
        {"record_hash", recordHash}
    };
}

SecurityAuditRecord SecurityAuditRecord::fromJson(const nlohmann::json& j) {
    SecurityAuditRecord r;
    r.index = j.value("index", 0);
    r.timestamp = j.value("timestamp", "");
    r.event = j.value("event", "");
    r.severity = j.value("severity", "");
    r.caller = j.value("caller", "");
    r.details = j.value("details", "");
    r.prevHash = j.value("prev_hash", "");
    r.recordHash = j.value("record_hash", "");
    return r;
}

nlohmann::json SecurityScanReport::toJson() const {
    return {
        {"score", score},
        {"files_scanned", filesScanned},
        {"secret_leaks_found", secretLeaksFound},
        {"insecure_scripts_found", insecureScriptsFound},
        {"path_traversal_vectors_found", pathTraversalVectorsFound},
        {"sensitive_files_exposed", sensitiveFilesExposed},
        {"findings", findings},
        {"recommendations", recommendations}
    };
}

void SecuritySandbox::logAuditEvent(
    const std::string& event,
    const std::string& severity,
    const std::string& caller,
    const std::string& details,
    const std::string& workspaceDir
) {
    try {
        std::string auditDir = workspaceDir + "/.arena";
        std::filesystem::create_directories(auditDir);
        std::string auditPath = auditDir + "/security_audit.jsonl";

        // Read last record to link hash chain
        std::string lastHash = "0000000000000000000000000000000000000000000000000000000000000000";
        int nextIndex = 1;

        if (std::filesystem::exists(auditPath)) {
            std::ifstream inFile(auditPath);
            std::string line;
            while (std::getline(inFile, line)) {
                if (line.empty()) continue;
                try {
                    auto j = nlohmann::json::parse(line);
                    if (j.contains("record_hash")) {
                        lastHash = j["record_hash"].get<std::string>();
                    }
                    if (j.contains("index")) {
                        nextIndex = j["index"].get<int>() + 1;
                    }
                } catch (...) {}
            }
        }

        SecurityAuditRecord rec;
        rec.index = nextIndex;
        rec.timestamp = getCurrentIsoTime();
        rec.event = event;
        rec.severity = severity;
        rec.caller = caller;
        rec.details = redactSecrets(details);
        rec.prevHash = lastHash;

        // Compute hash of record
        std::string recordPayload = std::to_string(rec.index) + "|" +
                                   rec.timestamp + "|" +
                                   rec.event + "|" +
                                   rec.severity + "|" +
                                   rec.caller + "|" +
                                   rec.details + "|" +
                                   rec.prevHash;
        rec.recordHash = sha256(recordPayload);

        std::ofstream outFile(auditPath, std::ios::app);
        if (outFile.is_open()) {
            outFile << rec.toJson().dump() << "\n";
        }
    } catch (...) {}
}

std::vector<SecurityAuditRecord> SecuritySandbox::getAuditLog(const std::string& workspaceDir) {
    std::vector<SecurityAuditRecord> records;
    std::string auditPath = workspaceDir + "/.arena/security_audit.jsonl";
    if (!std::filesystem::exists(auditPath)) return records;

    std::ifstream file(auditPath);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try {
            records.push_back(SecurityAuditRecord::fromJson(nlohmann::json::parse(line)));
        } catch (...) {}
    }
    return records;
}

bool SecuritySandbox::verifyAuditChain(const std::string& workspaceDir, std::string* failureReason) {
    auto records = getAuditLog(workspaceDir);
    if (records.empty()) return true;

    std::string expectedPrev = "0000000000000000000000000000000000000000000000000000000000000000";
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& rec = records[i];

        if (rec.prevHash != expectedPrev) {
            if (failureReason) {
                *failureReason = "Tamper detected at record index " + std::to_string(rec.index) +
                                 ": prev_hash mismatch. Expected: " + expectedPrev + ", Found: " + rec.prevHash;
            }
            return false;
        }

        std::string payload = std::to_string(rec.index) + "|" +
                              rec.timestamp + "|" +
                              rec.event + "|" +
                              rec.severity + "|" +
                              rec.caller + "|" +
                              rec.details + "|" +
                              rec.prevHash;
        std::string computedHash = sha256(payload);

        if (computedHash != rec.recordHash) {
            if (failureReason) {
                *failureReason = "Tamper detected at record index " + std::to_string(rec.index) +
                                 ": record_hash mismatch. Computed: " + computedHash + ", Logged: " + rec.recordHash;
            }
            return false;
        }

        expectedPrev = rec.recordHash;
    }
    return true;
}

SecurityScanReport SecuritySandbox::scanWorkspace(const std::string& workspaceDir) {
    SecurityScanReport report;
    int penalty = 0;

    std::error_code ec;
    auto iter = std::filesystem::recursive_directory_iterator(workspaceDir, std::filesystem::directory_options::skip_permission_denied, ec);
    auto end = std::filesystem::recursive_directory_iterator();

    while (!ec && iter != end) {
        if (iter->is_directory()) {
            std::string dirName = iter->path().filename().string();
            if (dirName == ".git" || dirName == "node_modules" || dirName == "build" ||
                dirName == "venv" || dirName == ".arena" || dirName == ".gemini") {
                iter.disable_recursion_pending();
                iter.increment(ec);
                continue;
            }
        }

        if (iter->is_regular_file()) {
            report.filesScanned++;
            std::string filePath = iter->path().string();
            std::string filename = iter->path().filename().string();

            // 1. Check for sensitive files left exposed
            if (isProtectedFile(filePath)) {
                report.sensitiveFilesExposed++;
                report.findings.push_back("Sensitive protected file exposed: " + filePath);
                penalty += 15;
            }

            // Read file content for scanning if small (< 1MB) and not self-scan
            uintmax_t size = 0;
            try { size = iter->file_size(); } catch (...) {}
            if (size > 0 && size < 1024 * 1024 && !filename.ends_with(".o") && !filename.ends_with(".obj") && !filename.ends_with(".exe") && !filename.ends_with(".a")) {
                std::ifstream f(filePath);
                if (f.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

                    // 2. Secret leak scan
                    std::string secretType;
                    if (containsUnmaskedSecret(content, &secretType)) {
                        report.secretLeaksFound++;
                        report.findings.push_back("Unmasked credential (" + secretType + ") detected in: " + filePath);
                        penalty += 25;
                    }

                    // 3. Insecure script scan
                    if (filename.ends_with(".sh") || filename.ends_with(".ps1") || filename.ends_with(".bat")) {
                        if (analyzeCommandRisk(content) == CommandRisk::FORBIDDEN) {
                            report.insecureScriptsFound++;
                            report.findings.push_back("High-risk or malicious script command detected in: " + filePath);
                            penalty += 20;
                        }
                    }
                }
            }
        }
        iter.increment(ec);
    }

    report.score = std::max(0, 100 - penalty);

    if (report.secretLeaksFound > 0) {
        report.recommendations.push_back("Redact all hardcoded API keys and store them in secure environment variables or vault.");
    }
    if (report.sensitiveFilesExposed > 0) {
        report.recommendations.push_back("Add sensitive credential files (.env, id_rsa, .key) to .gitignore and restrict file permissions.");
    }
    if (report.insecureScriptsFound > 0) {
        report.recommendations.push_back("Remove unsafe piped commands (e.g. curl | sh, Invoke-Expression) from automation scripts.");
    }
    if (report.findings.empty()) {
        report.recommendations.push_back("All security gates satisfied. Workspace path confinement and secret DLP active.");
    }

    return report;
}

SecurityLevel SecuritySandbox::stringToLevel(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "strict") return SecurityLevel::STRICT;
    if (lower == "permissive") return SecurityLevel::PERMISSIVE;
    return SecurityLevel::BALANCED;
}

std::string SecuritySandbox::levelToString(SecurityLevel lvl) {
    switch (lvl) {
        case SecurityLevel::STRICT: return "strict";
        case SecurityLevel::PERMISSIVE: return "permissive";
        case SecurityLevel::BALANCED: default: return "balanced";
    }
}

} // namespace arenafight
