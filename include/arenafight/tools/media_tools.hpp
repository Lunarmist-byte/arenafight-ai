#pragma once

#include "arenafight/tools/tool.hpp"
#include <string>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class MediaGenerateImageTool : public Tool {
public:
    std::string name() const override { return "media_generate_image"; }
    std::string description() const override {
        return "Generate visual assets using neural diffusion models (Fooocus SDXL, ComfyUI, DALL-E, Flux, or mock fallback) with physical magic byte verification.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class MediaGenerateVideoTool : public Tool {
public:
    std::string name() const override { return "media_generate_video"; }
    std::string description() const override {
        return "Generate video sequences using neural video models (Wan AI 2.1, ComfyUI Video, CogVideoX, LTX, or mock fallback) with container and duration verification.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class MediaInspectTool : public Tool {
public:
    std::string name() const override { return "media_inspect"; }
    std::string description() const override {
        return "Inspect physical evidence of media files: magic byte headers (PNG, JPEG, MP4), dimensions, container info, and SHA-256 cryptographic hash.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class MediaOrchestratePipelineTool : public Tool {
public:
    std::string name() const override { return "media_orchestrate_pipeline"; }
    std::string description() const override {
        return "Autonomous multimodal pipeline orchestrating Ollama (Cognitive Director) -> Fooocus (SDXL Keyframe) -> Wan AI 2.1 (Temporal Video Diffusion) with VRAM scheduling and physical verification.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

} // namespace arenafight

