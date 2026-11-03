#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {
namespace string_utils {

[[nodiscard]] constexpr std::string_view trimLeft(std::string_view sv) noexcept {
    while (!sv.empty() && static_cast<unsigned char>(sv.front()) <= ' ') {
        sv.remove_prefix(1);
    }
    return sv;
}

[[nodiscard]] constexpr std::string_view trimRight(std::string_view sv) noexcept {
    while (!sv.empty() && static_cast<unsigned char>(sv.back()) <= ' ') {
        sv.remove_suffix(1);
    }
    return sv;
}

[[nodiscard]] constexpr std::string_view trim(std::string_view sv) noexcept {
    return trimRight(trimLeft(sv));
}

[[nodiscard]] inline std::string trimCopy(std::string_view sv) {
    auto t = trim(sv);
    return std::string(t);
}

[[nodiscard]] inline std::string toLower(std::string_view sv) {
    std::string result;
    result.reserve(sv.size());
    for (char ch : sv) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

[[nodiscard]] inline std::string toUpper(std::string_view sv) {
    std::string result;
    result.reserve(sv.size());
    for (char ch : sv) {
        result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    return result;
}

[[nodiscard]] constexpr bool startsWith(std::string_view sv, std::string_view prefix) noexcept {
    return sv.size() >= prefix.size() && sv.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] constexpr bool endsWith(std::string_view sv, std::string_view suffix) noexcept {
    return sv.size() >= suffix.size() && sv.substr(sv.size() - suffix.size()) == suffix;
}

[[nodiscard]] constexpr bool contains(std::string_view sv, std::string_view needle) noexcept {
    return sv.find(needle) != std::string_view::npos;
}

[[nodiscard]] inline std::vector<std::string> split(std::string_view sv, char delim) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (start < sv.size()) {
        size_t end = sv.find(delim, start);
        if (end == std::string_view::npos) {
            tokens.emplace_back(sv.substr(start));
            break;
        }
        tokens.emplace_back(sv.substr(start, end - start));
        start = end + 1;
    }
    return tokens;
}

[[nodiscard]] inline std::string join(const std::vector<std::string>& elements, std::string_view delimiter = " ") {
    if (elements.empty()) return "";
    std::string result;
    size_t totalLen = 0;
    for (const auto& el : elements) totalLen += el.size() + delimiter.size();
    result.reserve(totalLen);

    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) result.append(delimiter);
        result.append(elements[i]);
    }
    return result;
}

[[nodiscard]] inline std::string replaceAll(std::string_view source, std::string_view from, std::string_view to) {
    if (from.empty()) return std::string(source);
    std::string result;
    result.reserve(source.size());
    size_t start = 0;
    while (start < source.size()) {
        size_t pos = source.find(from, start);
        if (pos == std::string_view::npos) {
            result.append(source.substr(start));
            break;
        }
        result.append(source.substr(start, pos - start));
        result.append(to);
        start = pos + from.size();
    }
    return result;
}

} // namespace string_utils
} // namespace arenafight

