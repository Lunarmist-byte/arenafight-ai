#include "arenafight/common/logger.hpp"
#include "arenafight/security/security_sandbox.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <regex>

namespace arenafight {

namespace {

std::string getCurrentIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    close();
}

void Logger::init(const std::string& sessionId, const std::string& logsDir) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessionId_ = sessionId;
    if (jsonlFile_.is_open()) {
        jsonlFile_.close();
    }

    try {
        std::filesystem::create_directories(logsDir);
        std::string filename = logsDir + "/session-" + sessionId + ".jsonl";
        jsonlFile_.open(filename, std::ios::app);
        initialized_ = true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to open log file in " << logsDir << ": " << e.what() << std::endl;
    }
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (jsonlFile_.is_open()) {
        jsonlFile_.flush();
        jsonlFile_.close();
    }
    initialized_ = false;
}

std::string Logger::sanitize(const std::string& text) {
    return SecuritySandbox::redactSecrets(text);
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level == LogLevel::LOG_DEBUG && !verbose_) {
        return;
    }

    std::string sanitized = sanitize(message);
    std::string prefix;
    std::string colorCode;
    const std::string resetCode = "\033[0m";

    switch (level) {
        case LogLevel::LOG_DEBUG:
            prefix = "[DEBUG] ";
            colorCode = "\033[90m"; // Dark gray
            break;
        case LogLevel::LOG_INFO:
            prefix = "[INFO]  ";
            colorCode = "\033[36m"; // Cyan
            break;
        case LogLevel::LOG_WARN:
            prefix = "[WARN]  ";
            colorCode = "\033[33m"; // Yellow
            break;
        case LogLevel::LOG_ERROR:
            prefix = "[ERROR] ";
            colorCode = "\033[31m"; // Red
            break;
        case LogLevel::LOG_RECOVERY:
            prefix = "[RECOVERY] ";
            colorCode = "\033[35m"; // Magenta
            break;
    }

    std::cout << colorCode << prefix << sanitized << resetCode << "\n";

    if (jsonlFile_.is_open()) {
        nlohmann::json entry = {
            {"timestamp", getCurrentIsoTimestamp()},
            {"session", sessionId_},
            {"type", "log"},
            {"level", prefix},
            {"message", sanitized}
        };
        jsonlFile_ << entry.dump() << "\n";
        jsonlFile_.flush();
    }
}

void Logger::logExecution(const ExecutionRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!jsonlFile_.is_open()) return;

    nlohmann::json j = record.toJson();
    j["timestamp"] = getCurrentIsoTimestamp();
    j["session"] = sessionId_;
    j["type"] = "execution";

    // Sanitize output
    if (j.contains("result") && j["result"].contains("output")) {
        j["result"]["output"] = sanitize(j["result"]["output"].get<std::string>());
    }
    if (j.contains("result") && j["result"].contains("error")) {
        j["result"]["error"] = sanitize(j["result"]["error"].get<std::string>());
    }

    jsonlFile_ << j.dump() << "\n";
    jsonlFile_.flush();
}

void Logger::logEvent(const std::string& eventType, const nlohmann::json& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!jsonlFile_.is_open()) return;

    nlohmann::json entry = {
        {"timestamp", getCurrentIsoTimestamp()},
        {"session", sessionId_},
        {"type", "event"},
        {"event", eventType},
        {"data", data}
    };

    jsonlFile_ << entry.dump() << "\n";
    jsonlFile_.flush();
}

} // namespace arenafight
