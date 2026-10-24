#pragma once

#include <string>
#include <vector>
#include <map>
#include "arenafight/hardware/hardware_types.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class HardwareValidator {
public:
    // Validate pin assignments against board specifications and component requirements
    static HardwareValidationReport validateCircuit(
        const BoardSpec& board,
        const std::vector<PinAssignment>& assignments,
        const std::vector<ComponentSpec>& components,
        bool isWifiEnabled = false
    );

    // Validate embedded C/C++ source code for extreme QoS guarantees
    static QoSTelemetry validateFirmwareQoS(
        const std::string& sourceCode,
        const std::string& boardId = ""
    );

    // Generate comprehensive ASCII wiring chart
    static std::string generateAsciiWiringChart(
        const BoardSpec& board,
        const std::vector<PinAssignment>& assignments
    );
};

} // namespace arenafight
