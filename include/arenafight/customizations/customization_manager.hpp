#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

struct RuleItem {
    std::string name;
    std::string sourcePath;
    std::string content;
    bool isSystem = false;
};

struct SkillItem {
    std::string name;
    std::string description;
    std::string directory;
    std::string skillFilePath;
    std::string instructions;
};

struct KnowledgeItem {
    std::string id;
    std::string title;
    std::string summary;
    std::string content;
    std::vector<std::string> tags;
    int64_t createdAtEpoch = 0;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"title", title},
            {"summary", summary},
            {"content", content},
            {"tags", tags},
            {"createdAtEpoch", createdAtEpoch}
        };
    }

    static KnowledgeItem fromJson(const nlohmann::json& j);
};

class CustomizationManager {
public:
    explicit CustomizationManager(std::string workspaceRoot = ".");

    void setWorkspaceRoot(const std::string& root);
    const std::string& getWorkspaceRoot() const { return workspaceRoot_; }

    // Rescan workspace for rules, skills, and knowledge items
    void refresh();

    // Rules
    const std::vector<RuleItem>& getRules() const { return rules_; }
    std::string formatRulesForPrompt() const;
    void addRule(const std::string& name, const std::string& content);

    // Skills
    const std::vector<SkillItem>& getSkills() const { return skills_; }
    std::shared_ptr<SkillItem> getSkill(const std::string& name);
    std::vector<SkillItem> matchRelevantSkills(const std::string& taskPrompt) const;
    void createSkill(const std::string& name, const std::string& description, const std::string& instructions);

    // Knowledge Items (KIs)
    const std::vector<KnowledgeItem>& getKnowledgeItems() const { return knowledgeItems_; }
    KnowledgeItem addKnowledge(
        const std::string& title,
        const std::string& summary,
        const std::string& content,
        const std::vector<std::string>& tags = {}
    );
    std::string formatKnowledgeForPrompt() const;

private:
    std::string workspaceRoot_;
    std::vector<RuleItem> rules_;
    std::vector<SkillItem> skills_;
    std::vector<KnowledgeItem> knowledgeItems_;

    void loadSystemRules();
    void discoverWorkspaceRules();
    void discoverSkills();
    void discoverKnowledgeItems();
    void ensureCustomizationDirectories();
};

} // namespace arenafight
