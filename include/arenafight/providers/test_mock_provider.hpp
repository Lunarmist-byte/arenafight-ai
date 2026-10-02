#pragma once

#include "arenafight/providers/provider.hpp"
#include <map>

namespace arenafight {

class TestMockProvider : public ModelProvider {
public:
    TestMockProvider();

    std::string name() const override { return "TestMock"; }

    bool available() override { return true; }

    std::vector<ModelInfo> discoverModels() override;

    AgentResponse generate(const AgentRequest& request) override;

    void setFailureMode(bool fail) { deliberateFailure_ = fail; }
    void reset() { step_ = 0; }

private:
    int step_ = 0;
    bool deliberateFailure_ = false;
};

} // namespace arenafight
