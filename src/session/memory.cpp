#include "arenafight/session/memory.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>

namespace arenafight {

namespace {

int64_t getNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace

void MemoryManager::addFact(const std::string& fact, const std::string& source) {
    MemoryItem item;
    item.id = "mem_" + std::to_string(items_.size() + 1);
    item.category = "fact";
    item.content = fact;
    item.source = source;
    item.timestamp = getNowMs();
    items_.push_back(item);
}

void MemoryManager::addDiscovery(const std::string& discovery, const std::string& source) {
    MemoryItem item;
    item.id = "mem_" + std::to_string(items_.size() + 1);
    item.category = "discovery";
    item.content = discovery;
    item.source = source;
    item.timestamp = getNowMs();
    items_.push_back(item);
}

void MemoryManager::addDecision(const std::string& decision, const std::string& reason) {
    MemoryItem item;
    item.id = "mem_" + std::to_string(items_.size() + 1);
    item.category = "decision";
    item.content = decision + (reason.empty() ? "" : " (Reason: " + reason + ")");
    item.timestamp = getNowMs();
    items_.push_back(item);
}

void MemoryManager::addError(const std::string& errorContext, const std::string& errorDetails) {
    MemoryItem item;
    item.id = "mem_" + std::to_string(items_.size() + 1);
    item.category = "error";
    item.content = errorContext + ": " + errorDetails;
    item.timestamp = getNowMs();
    items_.push_back(item);
}

void MemoryManager::addFileModification(const std::string& filePath, const std::string& description) {
    MemoryItem item;
    item.id = "mem_" + std::to_string(items_.size() + 1);
    item.category = "file_change";
    item.content = filePath + " -> " + description;
    item.timestamp = getNowMs();
    items_.push_back(item);
}

std::vector<MemoryItem> MemoryManager::getItemsByCategory(const std::string& category) const {
    std::vector<MemoryItem> res;
    for (const auto& item : items_) {
        if (item.category == category) res.push_back(item);
    }
    return res;
}

std::vector<MemoryItem> MemoryManager::search(const std::string& keyword) const {
    std::vector<MemoryItem> res;
    std::string kw = keyword;
    std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);

    for (const auto& item : items_) {
        std::string contentLower = item.content;
        std::transform(contentLower.begin(), contentLower.end(), contentLower.begin(), ::tolower);
        if (contentLower.find(kw) != std::string::npos) {
            res.push_back(item);
        }
    }
    return res;
}

std::string MemoryManager::getCompactSummary() const {
    std::stringstream ss;
    auto facts = getItemsByCategory("fact");
    auto discoveries = getItemsByCategory("discovery");
    auto decisions = getItemsByCategory("decision");
    auto errors = getItemsByCategory("error");
    auto files = getItemsByCategory("file_change");

    if (!facts.empty()) {
        ss << "Known Facts:\n";
        for (const auto& f : facts) ss << "- " << f.content << "\n";
    }
    if (!discoveries.empty()) {
        ss << "Key Discoveries:\n";
        for (const auto& d : discoveries) ss << "- " << d.content << "\n";
    }
    if (!decisions.empty()) {
        ss << "Decisions Made:\n";
        for (const auto& d : decisions) ss << "- " << d.content << "\n";
    }
    if (!errors.empty()) {
        ss << "Past Errors Encountered & Resolved:\n";
        // Show up to the last 5 errors to avoid bloat
        size_t start = errors.size() > 5 ? errors.size() - 5 : 0;
        for (size_t i = start; i < errors.size(); ++i) {
            ss << "- " << errors[i].content << "\n";
        }
    }
    if (!files.empty()) {
        ss << "Modified Files:\n";
        for (const auto& f : files) ss << "- " << f.content << "\n";
    }

    return ss.str();
}

} // namespace arenafight
