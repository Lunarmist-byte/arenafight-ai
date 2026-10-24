#pragma once

#include <string>
#include <vector>
#include "arenafight/hardware/hardware_types.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

struct ScaffoldResult {
    bool success = false;
    std::string projectDir;
    std::vector<std::string> filesCreated;
    std::string wokwiDiagramPath;
    std::string message;
    std::string summary;
};

class HardwareScaffolder {
public:
    static ScaffoldResult scaffoldProject(
        const ScaffoldOptions& options,
        const BoardSpec& board,
        const std::vector<ComponentSpec>& components
    );

    // Generate Wokwi diagram.json
    static std::string generateWokwiDiagram(
        const BoardSpec& board,
        const std::vector<PinAssignment>& assignments,
        const std::vector<ComponentSpec>& components
    );

    // Generate production-grade C++ firmware adhering to extreme QoS standards
    static std::string generateQoSFirmware(
        const BoardSpec& board,
        const std::vector<PinAssignment>& assignments,
        const std::vector<ComponentSpec>& components,
        const std::string& projectName
    );

    // Generate hardware configuration header
    static std::string generateConfigHeader(
        const BoardSpec& board,
        const std::vector<PinAssignment>& assignments,
        const std::vector<ComponentSpec>& components
    );
};

} // namespace arenafight
