#pragma once

#include <string>
#include <vector>
#include <map>
#include "arenafight/common/types.hpp"

namespace arenafight {

class MemoryManager {
public:
    MemoryManager() = default;

    void addFact(const std::string& fact, const std::string& source = "");
    void addVerifiedFact(const std::string& fact, const std::string& evidenceId, const std::string& source = "");
    void addHypothesis(const std::string& hypothesis, const std::string& source = "");
    void addAssumption(const std::string& assumption, const std::string& source = "");
    void addInference(const std::string& inference, const std::string& source = "");
    bool promoteToVerifiedFact(const std::string& memId, const std::string& evidenceId);

    void addDiscovery(const std::string& discovery, const std::string& source = "");
    void addDecision(const std::string& decision, const std::string& reason = "");
    void addError(const std::string& errorContext, const std::string& errorDetails);
    void addFileModification(const std::string& filePath, const std::string& description);

    std::vector<MemoryItem> getItemsByCategory(const std::string& category) const;
    std::vector<MemoryItem> search(const std::string& keyword) const;

    const std::vector<MemoryItem>& getAll() const { return items_; }
    void loadFromItems(const std::vector<MemoryItem>& items) { items_ = items; }

    std::string getCompactSummary() const;

private:
    std::vector<MemoryItem> items_;
};

} // namespace arenafight
