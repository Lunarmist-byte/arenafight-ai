#pragma once

#include <string>
#include <vector>
#include "arenafight/hardware/hardware_types.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class CircuitCalculator {
public:
    // LED Current Limiting Resistor
    static CircuitCalculationResult calculateLedResistor(
        double supplyVoltage,
        double ledForwardVoltage,
        double ledCurrentMa = 15.0
    );

    // Voltage Divider for Level Shifting (e.g. 5V -> 3.3V)
    static CircuitCalculationResult calculateVoltageDivider(
        double inputVoltage,
        double targetOutputVoltage,
        double targetResistanceKOhms = 10.0
    );

    // I2C Bus Pull-Up Resistor
    static CircuitCalculationResult calculateI2cPullup(
        double supplyVoltage,
        int busSpeedKhz = 100,
        double busCapacitancePf = 100.0
    );

    // Battery Life & Deep Sleep Power Budget
    static CircuitCalculationResult calculateBatteryLife(
        double batteryCapacityMah,
        double activeCurrentMa,
        double sleepCurrentMicroAmps,
        double activeTimeSeconds,
        double sleepTimeSeconds
    );

    // Find closest standard E12/E24 resistor value
    static double findNearestStandardResistor(double exactOhms);
};

} // namespace arenafight
