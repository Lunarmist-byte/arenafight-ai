#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <nlohmann/json.hpp>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

enum class ArtifactType {
    PLAN,
    CODE,
    DIFF,
    ANALYSIS,
    REPORT,
    VERIFICATION,
    CUSTOM
};

inline std::string artifactTypeToString(ArtifactType type) {
    switch (type) {
        case ArtifactType::PLAN: return "PLAN";
        case ArtifactType::CODE: return "CODE";
        case ArtifactType::DIFF: return "DIFF";
        case ArtifactType::ANALYSIS: return "ANALYSIS";
        case ArtifactType::REPORT: return "REPORT";
        case ArtifactType::VERIFICATION: return "VERIFICATION";
        default: return "CUSTOM";
    }
}

inline ArtifactType artifactTypeFromString(const std::string& str) {
    if (str == "PLAN") return ArtifactType::PLAN;
    if (str == "CODE") return ArtifactType::CODE;
    if (str == "DIFF") return ArtifactType::DIFF;
    if (str == "ANALYSIS") return ArtifactType::ANALYSIS;
    if (str == "REPORT") return ArtifactType::REPORT;
    if (str == "VERIFICATION") return ArtifactType::VERIFICATION;
    return ArtifactType::CUSTOM;
}

struct ArtifactMetadata {
    bool userFacing = true;
    bool requestFeedback = false;
    std::string summary;
    std::map<std::string, std::string> extra;

    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"UserFacing", userFacing},
            {"RequestFeedback", requestFeedback},
            {"Summary", summary}
        };
        for (const auto& [k, v] : extra) {
            j[k] = v;
        }
        return j;
    }

    static ArtifactMetadata fromJson(const nlohmann::json& j) {
        ArtifactMetadata m;
        if (j.contains("UserFacing")) m.userFacing = j["UserFacing"].get<bool>();
        if (j.contains("RequestFeedback")) m.requestFeedback = j["RequestFeedback"].get<bool>();
        if (j.contains("Summary")) m.summary = j["Summary"].get<std::string>();
        return m;
    }
};

struct Artifact {
    std::string id;
    std::string title;
    std::string filename;
    std::string path;
    ArtifactType type = ArtifactType::CUSTOM;
    ArtifactMetadata metadata;
    std::string content;
    int64_t createdAtEpoch = 0;
    int64_t updatedAtEpoch = 0;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"title", title},
            {"filename", filename},
            {"path", path},
            {"type", artifactTypeToString(type)},
            {"metadata", metadata.toJson()},
            {"content", content},
            {"createdAtEpoch", createdAtEpoch},
            {"updatedAtEpoch", updatedAtEpoch}
        };
    }

    static Artifact fromJson(const nlohmann::json& j);

    // Render formatted markdown with Antigravity YAML frontmatter
    std::string toMarkdownWithFrontmatter() const;
};

class ArtifactManager {
public:
    explicit ArtifactManager(std::string artifactsDir = ".arena/artifacts");

    void setArtifactsDir(const std::string& dir);
    const std::string& getArtifactsDir() const { return artifactsDir_; }

    // Create and save an artifact
    Artifact createArtifact(
        const std::string& title,
        const std::string& filename,
        ArtifactType type,
        const std::string& content,
        const ArtifactMetadata& metadata
    );

    // Update an existing artifact
    bool updateArtifact(
        const std::string& id,
        const std::string& newContent,
        const std::string& summary = ""
    );

    // Retrieve by ID or filename
    std::shared_ptr<Artifact> getArtifact(const std::string& id);
    std::shared_ptr<const Artifact> getArtifact(const std::string& id) const;
    std::shared_ptr<Artifact> getArtifactByFilename(const std::string& filename);

    // List all artifacts
    std::vector<Artifact> listArtifacts() const;

    // Load from disk directory
    void reloadFromDisk();

    // Helper builders for standard Antigravity artifacts
    Artifact createPlanArtifact(
        const std::string& task,
        const std::string& planMarkdown,
        bool requestFeedback = true
    );

    Artifact createDiffArtifact(
        const std::string& title,
        const std::string& diffContent,
        const std::string& summary
    );

    Artifact createVerificationArtifact(
        const std::string& reportMarkdown,
        bool allPassed
    );

private:
    std::string artifactsDir_;
    std::map<std::string, Artifact> artifacts_;

    void ensureDirectoryExists();
    void persistArtifact(const Artifact& art);
};

} // namespace arenafight
