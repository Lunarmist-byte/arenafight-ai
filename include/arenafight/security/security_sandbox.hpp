#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <nlohmann/json.hpp>

namespace arenafight {

enum class SecurityLevel {
    PERMISSIVE, // Warning logs, basic destructive command blocks
    BALANCED,   // Default: Workspace path jail, blocks dangerous commands & reverse shells, masks secrets
    STRICT      // Maximum lockdown: Strict whitelist, deep scan, zero sensitive file reads
};

enum class CommandRisk {
    SAFE,       // Read-only or safe compilers/tools (echo, git status, cmake, ninja, python)
    AUDITED,    // Writes or standard operations requiring audit logging (git commit, file creation)
    RESTRICTED, // Potentially destructive operations requiring confirmation (rm, git reset, format)
    FORBIDDEN   // Malicious attacks: reverse shells, download-and-execute, fork bombs, registry wipe
};

struct SecurityViolation {
    std::string type;       // "PATH_TRAVERSAL", "MALICIOUS_COMMAND", "SECRET_LEAK", "PROMPT_INJECTION", "PROTECTED_FILE"
    std::string severity;   // "CRITICAL", "HIGH", "MEDIUM", "LOW"
    std::string message;
    std::string target;
    std::string timestamp;
};

struct SecurityAuditRecord {
    int index = 0;
    std::string timestamp;
    std::string event;      // e.g. "PATH_TRAVERSAL_BLOCKED", "SECRET_MASKED", "COMMAND_BLOCKED"
    std::string severity;   // "INFO", "WARN", "CRITICAL"
    std::string caller;
    std::string details;
    std::string prevHash;
    std::string recordHash;

    nlohmann::json toJson() const;
    static SecurityAuditRecord fromJson(const nlohmann::json& j);
};

struct SecurityScanReport {
    int score = 100; // 0 - 100%
    int filesScanned = 0;
    int secretLeaksFound = 0;
    int insecureScriptsFound = 0;
    int pathTraversalVectorsFound = 0;
    int sensitiveFilesExposed = 0;
    std::vector<std::string> findings;
    std::vector<std::string> recommendations;

    nlohmann::json toJson() const;
};

class SecuritySandbox {
public:
    // SHA-256 cryptographic hash
    static std::string sha256(const std::string& input);

    // Path Confinement & Traversal Protection
    static bool isPathSafe(
        const std::string& targetPath,
        const std::string& workspaceDir,
        std::string* violationReason = nullptr,
        bool allowWorkspaceSubdirsOnly = true
    );

    // Canonicalize path safely within workspace boundary
    static std::string resolveSafePath(
        const std::string& targetPath,
        const std::string& workspaceDir,
        std::string* violationReason = nullptr
    );

    // Check if file is a sensitive/protected system file (.env, id_rsa, etc.)
    static bool isProtectedFile(const std::string& path);

    // Command Risk Analysis & Threat Detection
    static CommandRisk analyzeCommandRisk(const std::string& command);
    static bool isCommandPermitted(
        const std::string& command,
        SecurityLevel level,
        std::string* violationReason = nullptr
    );

    // Secret & Credential Redactor (DLP)
    static std::string redactSecrets(const std::string& text);
    static bool containsUnmaskedSecret(const std::string& text, std::string* matchedType = nullptr);

    // Prompt Injection & Adversarial Unicode Sanitizer
    static bool detectPromptInjection(const std::string& text, std::string* matchedPattern = nullptr);
    static std::string sanitizeAdversarialContent(const std::string& text);

    // Tamper-Evident SHA-256 Audit Logger (.arena/security_audit.jsonl)
    static void logAuditEvent(
        const std::string& event,
        const std::string& severity,
        const std::string& caller,
        const std::string& details,
        const std::string& workspaceDir = "."
    );
    static std::vector<SecurityAuditRecord> getAuditLog(const std::string& workspaceDir = ".");
    static bool verifyAuditChain(const std::string& workspaceDir = ".", std::string* failureReason = nullptr);

    // Workspace Vulnerability & Security Scanner
    static SecurityScanReport scanWorkspace(const std::string& workspaceDir = ".");

    // Helpers
    static SecurityLevel stringToLevel(const std::string& str);
    static std::string levelToString(SecurityLevel lvl);
};

} // namespace arenafight

