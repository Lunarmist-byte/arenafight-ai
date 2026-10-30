#pragma once

#include <string>
#include <filesystem>
#include <system_error>
#include <fstream>
#include <sstream>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

/**
 * @brief RAII Guard for temporary files ensuring deletion upon scope exit.
 */
class TempFileGuard {
public:
    explicit TempFileGuard(std::string path) : path_(std::move(path)) {}

    ~TempFileGuard() {
        cleanup();
    }

    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;

    TempFileGuard(TempFileGuard&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }

    TempFileGuard& operator=(TempFileGuard&& other) noexcept {
        if (this != &other) {
            cleanup();
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }

    const std::string& path() const { return path_; }

    void dismiss() { path_.clear(); }

private:
    std::string path_;

    void cleanup() {
        if (!path_.empty()) {
            std::error_code ec;
            std::filesystem::remove(path_, ec);
        }
    }
};

/**
 * @brief Common filesystem utilities.
 */
class FileUtils {
public:
    static bool writeTextFile(const std::string& path, const std::string& content) {
        std::ofstream stream(path, std::ios::trunc);
        if (!stream.is_open()) return false;
        stream << content;
        return true;
    }

    static std::string readTextFile(const std::string& path) {
        std::ifstream stream(path);
        if (!stream.is_open()) return "";
        std::stringstream ss;
        ss << stream.rdbuf();
        return ss.str();
    }
};

} // namespace arenafight

