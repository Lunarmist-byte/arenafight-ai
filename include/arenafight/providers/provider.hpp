#pragma once

#include <string>
#include <vector>
#include <memory>
#include "arenafight/common/types.hpp"
#include "arenafight/common/config.hpp"

namespace arenafight {

class ModelProvider {
public:
    virtual ~ModelProvider() = default;

    virtual std::string name() const = 0;

    virtual bool available() = 0;

    virtual std::vector<ModelInfo> discoverModels() = 0;

    virtual AgentResponse generate(
        const AgentRequest& request
    ) = 0;

    // Helper to parse tool calls that might be embedded in text markdown blocks
    static std::vector<ToolCall> extractToolCallsFromText(const std::string& text);
};

} // namespace arenafight
