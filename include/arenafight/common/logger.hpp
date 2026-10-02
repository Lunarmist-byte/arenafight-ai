#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <nlohmann/json.hpp>
#include "arenafight/common/types.hpp"

namespace arenafight {

enum class LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_RECOVERY
};

class Logger {
public:
    static Logger& instance();

    void init(const std::string& sessionId, const std::string& logsDir = "logs");
    void close();

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& msg) { log(LogLevel::LOG_DEBUG, msg); }
    void info(const std::string& msg) { log(LogLevel::LOG_INFO, msg); }
    void warn(const std::string& msg) { log(LogLevel::LOG_WARN, msg); }
    void error(const std::string& msg) { log(LogLevel::LOG_ERROR, msg); }
    void recovery(const std::string& msg) { log(LogLevel::LOG_RECOVERY, msg); }

    // Structured JSONL execution logging
    void logExecution(const ExecutionRecord& record);
    void logEvent(const std::string& eventType, const nlohmann::json& data);

    // Sanitization: removes API keys and sensitive tokens
    static std::string sanitize(const std::string& text);

    void setVerbose(bool verbose) { verbose_ = verbose; }
    bool isVerbose() const { return verbose_; }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex mutex_;
    std::string sessionId_;
    std::ofstream jsonlFile_;
    bool verbose_ = false;
    bool initialized_ = false;
};

} // namespace arenafight
