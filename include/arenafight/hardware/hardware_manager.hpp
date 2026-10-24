#pragma once

#include <string>
#include <vector>
#include <memory>
#include "arenafight/hardware/hardware_types.hpp"
#include "arenafight/hardware/board_registry.hpp"
#include "arenafight/hardware/circuit_calculator.hpp"
#include "arenafight/hardware/hardware_validator.hpp"
#include "arenafight/hardware/hardware_scaffolder.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class HardwareManager {
public:
    static HardwareManager& instance();

    HardwareBoardRegistry& getBoards() { return HardwareBoardRegistry::instance(); }

    // Run circuit validation for a project
    HardwareValidationReport validate(
        const std::string& boardId,
        const std::vector<PinAssignment>& assignments,
        const std::vector<std::string>& componentIds,
        bool isWifiEnabled = false
    );

    // Run firmware QoS analysis on a directory or file
    QoSTelemetry evaluateProjectQoS(const std::string& projectDir);

    // Scaffold a complete embedded project
    ScaffoldResult scaffold(const ScaffoldOptions& options);

    // Run circuit calculations
    CircuitCalculationResult calculateLed(double vcc, double vf, double ma);
    CircuitCalculationResult calculateDivider(double vin, double vout, double rk = 10.0);
    CircuitCalculationResult calculateI2c(double vcc, int speedKhz = 100, double cbPf = 100.0);
    CircuitCalculationResult calculateBattery(double capMah, double actMa, double slpUa, double actSec, double slpSec);

private:
    HardwareManager() = default;
};

} // namespace arenafight
