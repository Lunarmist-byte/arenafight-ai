#pragma once

#include <vector>
#include <memory>
#include "arenafight/providers/provider.hpp"
#include "arenafight/common/config.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

/**
 * @brief Factory for initializing and registering all configured ModelProviders.
 */
class ProviderFactory {
public:
    static std::vector<std::shared_ptr<ModelProvider>> createConfiguredProviders(const Config& config);
};

} // namespace arenafight

