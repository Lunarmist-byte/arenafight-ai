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

enum class McuArchitecture {
    AVR_8BIT,
    ARM_CORTEX_M0PLUS,
    ARM_CORTEX_M3,
    ARM_CORTEX_M4F,
    ARM_CORTEX_M7,
    XTENSA_LX6,
    XTENSA_LX7,
    RISCV_32,
    UNKNOWN
};

inline std::string architectureToString(McuArchitecture arch) {
    switch (arch) {
        case McuArchitecture::AVR_8BIT: return "AVR 8-bit";
        case McuArchitecture::ARM_CORTEX_M0PLUS: return "ARM Cortex-M0+";
        case McuArchitecture::ARM_CORTEX_M3: return "ARM Cortex-M3";
        case McuArchitecture::ARM_CORTEX_M4F: return "ARM Cortex-M4F";
        case McuArchitecture::ARM_CORTEX_M7: return "ARM Cortex-M7";
        case McuArchitecture::XTENSA_LX6: return "Xtensa Dual-Core LX6";
        case McuArchitecture::XTENSA_LX7: return "Xtensa Dual-Core LX7";
        case McuArchitecture::RISCV_32: return "RISC-V 32-bit";
        default: return "Unknown Architecture";
    }
}

enum class LogicVoltage {
    VOLTAGE_3V3,
    VOLTAGE_5V,
    VOLTAGE_1V8,
    VOLTAGE_DUAL
};

inline std::string logicVoltageToString(LogicVoltage v) {
    switch (v) {
        case LogicVoltage::VOLTAGE_3V3: return "3.3V";
        case LogicVoltage::VOLTAGE_5V: return "5.0V";
        case LogicVoltage::VOLTAGE_1V8: return "1.8V";
        case LogicVoltage::VOLTAGE_DUAL: return "3.3V / 5.0V Dual";
        default: return "3.3V";
    }
}

inline double logicVoltageToDouble(LogicVoltage v) {
    switch (v) {
        case LogicVoltage::VOLTAGE_3V3: return 3.3;
        case LogicVoltage::VOLTAGE_5V: return 5.0;
        case LogicVoltage::VOLTAGE_1V8: return 1.8;
        case LogicVoltage::VOLTAGE_DUAL: return 3.3;
        default: return 3.3;
    }
}

enum class PinFunction {
    GPIO_IN,
    GPIO_OUT,
    GPIO_INOUT,
    ANALOG_IN,
    DAC_OUT,
    PWM_OUT,
    I2C_SDA,
    I2C_SCL,
    SPI_MOSI,
    SPI_MISO,
    SPI_SCK,
    SPI_CS,
    UART_TX,
    UART_RX,
    RESET_PIN,
    BOOT_PIN,
    POWER_VCC,
    POWER_GND
};

inline std::string pinFunctionToString(PinFunction fn) {
    switch (fn) {
        case PinFunction::GPIO_IN: return "GPIO_IN";
        case PinFunction::GPIO_OUT: return "GPIO_OUT";
        case PinFunction::GPIO_INOUT: return "GPIO_INOUT";
        case PinFunction::ANALOG_IN: return "ANALOG_IN";
        case PinFunction::DAC_OUT: return "DAC_OUT";
        case PinFunction::PWM_OUT: return "PWM_OUT";
        case PinFunction::I2C_SDA: return "I2C_SDA";
        case PinFunction::I2C_SCL: return "I2C_SCL";
        case PinFunction::SPI_MOSI: return "SPI_MOSI";
        case PinFunction::SPI_MISO: return "SPI_MISO";
        case PinFunction::SPI_SCK: return "SPI_SCK";
        case PinFunction::SPI_CS: return "SPI_CS";
        case PinFunction::UART_TX: return "UART_TX";
        case PinFunction::UART_RX: return "UART_RX";
        case PinFunction::RESET_PIN: return "RESET";
        case PinFunction::BOOT_PIN: return "BOOT";
        case PinFunction::POWER_VCC: return "VCC";
        case PinFunction::POWER_GND: return "GND";
        default: return "GPIO";
    }
}

inline PinFunction pinFunctionFromString(const std::string& str) {
    if (str == "GPIO_IN") return PinFunction::GPIO_IN;
    if (str == "GPIO_OUT") return PinFunction::GPIO_OUT;
    if (str == "ANALOG_IN" || str == "ADC") return PinFunction::ANALOG_IN;
    if (str == "DAC_OUT" || str == "DAC") return PinFunction::DAC_OUT;
    if (str == "PWM_OUT" || str == "PWM") return PinFunction::PWM_OUT;
    if (str == "I2C_SDA" || str == "SDA") return PinFunction::I2C_SDA;
    if (str == "I2C_SCL" || str == "SCL") return PinFunction::I2C_SCL;
    if (str == "SPI_MOSI" || str == "MOSI") return PinFunction::SPI_MOSI;
    if (str == "SPI_MISO" || str == "MISO") return PinFunction::SPI_MISO;
    if (str == "SPI_SCK" || str == "SCK") return PinFunction::SPI_SCK;
    if (str == "SPI_CS" || str == "CS" || str == "SS") return PinFunction::SPI_CS;
    if (str == "UART_TX" || str == "TX") return PinFunction::UART_TX;
    if (str == "UART_RX" || str == "RX") return PinFunction::UART_RX;
    if (str == "RESET") return PinFunction::RESET_PIN;
    if (str == "BOOT") return PinFunction::BOOT_PIN;
    if (str == "VCC") return PinFunction::POWER_VCC;
    if (str == "GND") return PinFunction::POWER_GND;
    return PinFunction::GPIO_INOUT;
}

enum class HardwareIssueSeverity {
    CRITICAL_ERROR,
    WARNING,
    INFO
};

inline std::string severityToString(HardwareIssueSeverity s) {
    switch (s) {
        case HardwareIssueSeverity::CRITICAL_ERROR: return "CRITICAL ERROR";
        case HardwareIssueSeverity::WARNING: return "WARNING";
        case HardwareIssueSeverity::INFO: return "INFO";
        default: return "NOTICE";
    }
}

struct PinSpec {
    std::string name;
    int physicalPin = 0;
    std::vector<PinFunction> supportedFunctions;
    LogicVoltage voltage = LogicVoltage::VOLTAGE_3V3;
    bool isStrappingPin = false;
    std::string strappingWarning;
    bool isInputOnly = false;
    bool hasAdcWifiConflict = false; // e.g. ESP32 ADC2 pins
    bool isPwmCapable = true;
    bool is5VTolerant = false;

    bool supports(PinFunction fn) const {
        for (auto f : supportedFunctions) {
            if (f == fn || (f == PinFunction::GPIO_INOUT && (fn == PinFunction::GPIO_IN || fn == PinFunction::GPIO_OUT))) {
                return true;
            }
        }
        return false;
    }

    nlohmann::json toJson() const {
        std::vector<std::string> funcs;
        for (auto f : supportedFunctions) funcs.push_back(pinFunctionToString(f));
        return {
            {"name", name},
            {"physicalPin", physicalPin},
            {"supportedFunctions", funcs},
            {"voltage", logicVoltageToString(voltage)},
            {"isStrappingPin", isStrappingPin},
            {"strappingWarning", strappingWarning},
            {"isInputOnly", isInputOnly},
            {"hasAdcWifiConflict", hasAdcWifiConflict},
            {"isPwmCapable", isPwmCapable},
            {"is5VTolerant", is5VTolerant}
        };
    }
};

struct BoardSpec {
    std::string id;
    std::string name;
    McuArchitecture architecture = McuArchitecture::UNKNOWN;
    int clockMHz = 16;
    LogicVoltage logicVoltage = LogicVoltage::VOLTAGE_3V3;
    int flashKb = 32;
    int sramKb = 2;
    double deepSleepMicroAmps = 10.0;
    double activeMilliAmps = 50.0;
    std::map<std::string, PinSpec> pins;
    std::string defaultI2cSda;
    std::string defaultI2cScl;
    std::string defaultSpiMosi;
    std::string defaultSpiMiso;
    std::string defaultSpiSck;
    std::string defaultUartTx;
    std::string defaultUartRx;
    std::string defaultLedPin;
    std::string description;
    std::string platformioBoardId;
    std::string arduinoFqbn;

    nlohmann::json toJson() const {
        nlohmann::json pinsJson = nlohmann::json::object();
        for (const auto& [name, pin] : pins) {
            pinsJson[name] = pin.toJson();
        }
        return {
            {"id", id},
            {"name", name},
            {"architecture", architectureToString(architecture)},
            {"clockMHz", clockMHz},
            {"logicVoltage", logicVoltageToString(logicVoltage)},
            {"flashKb", flashKb},
            {"sramKb", sramKb},
            {"deepSleepMicroAmps", deepSleepMicroAmps},
            {"activeMilliAmps", activeMilliAmps},
            {"pins", pinsJson},
            {"defaultI2cSda", defaultI2cSda},
            {"defaultI2cScl", defaultI2cScl},
            {"defaultSpiMosi", defaultSpiMosi},
            {"defaultSpiMiso", defaultSpiMiso},
            {"defaultSpiSck", defaultSpiSck},
            {"defaultUartTx", defaultUartTx},
            {"defaultUartRx", defaultUartRx},
            {"defaultLedPin", defaultLedPin},
            {"description", description},
            {"platformioBoardId", platformioBoardId},
            {"arduinoFqbn", arduinoFqbn}
        };
    }
};

struct ComponentSpec {
    std::string id;
    std::string name;
    std::string category; // "Sensor", "Actuator", "Display", "Driver", "Power"
    int defaultI2cAddress = 0; // 0 if not I2C
    std::vector<int> altI2cAddresses;
    LogicVoltage operatingVoltage = LogicVoltage::VOLTAGE_3V3;
    double currentDrawMa = 10.0;
    bool requiresLevelShiftingWith5v = false;
    bool requiresPullup = false;
    std::vector<std::string> requiredPins; // e.g. ["VCC", "GND", "SDA", "SCL"]
    std::string description;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"name", name},
            {"category", category},
            {"defaultI2cAddress", defaultI2cAddress},
            {"operatingVoltage", logicVoltageToString(operatingVoltage)},
            {"currentDrawMa", currentDrawMa},
            {"requiresLevelShiftingWith5v", requiresLevelShiftingWith5v},
            {"requiresPullup", requiresPullup},
            {"requiredPins", requiredPins},
            {"description", description}
        };
    }
};

struct PinAssignment {
    std::string componentId;
    std::string componentPin;
    std::string boardPin;
    PinFunction function = PinFunction::GPIO_INOUT;
    std::string wireColor = "green";
    std::string notes;

    nlohmann::json toJson() const {
        return {
            {"componentId", componentId},
            {"componentPin", componentPin},
            {"boardPin", boardPin},
            {"function", pinFunctionToString(function)},
            {"wireColor", wireColor},
            {"notes", notes}
        };
    }
};

struct HardwareIssue {
    HardwareIssueSeverity severity = HardwareIssueSeverity::WARNING;
    std::string code;
    std::string targetPinOrComponent;
    std::string message;
    std::string recommendation;

    nlohmann::json toJson() const {
        return {
            {"severity", severityToString(severity)},
            {"code", code},
            {"target", targetPinOrComponent},
            {"message", message},
            {"recommendation", recommendation}
        };
    }
};

struct HardwareValidationReport {
    bool isValid = true;
    int criticalCount = 0;
    int warningCount = 0;
    std::vector<HardwareIssue> issues;
    std::vector<std::string> recommendations;
    std::string asciiWiringSummary;

    nlohmann::json toJson() const {
        nlohmann::json issuesArr = nlohmann::json::array();
        for (const auto& is : issues) issuesArr.push_back(is.toJson());
        return {
            {"isValid", isValid},
            {"criticalCount", criticalCount},
            {"warningCount", warningCount},
            {"issues", issuesArr},
            {"recommendations", recommendations},
            {"asciiWiringSummary", asciiWiringSummary}
        };
    }
};

struct CircuitCalculationResult {
    std::string calculationType; // "LED_RESISTOR", "VOLTAGE_DIVIDER", "I2C_PULLUP", "BATTERY_LIFE"
    nlohmann::json inputParams;
    nlohmann::json resultValues;
    std::string formulaUsed;
    std::vector<std::string> componentRecommendations;
    std::string summary;

    nlohmann::json toJson() const {
        return {
            {"calculationType", calculationType},
            {"inputParams", inputParams},
            {"resultValues", resultValues},
            {"formulaUsed", formulaUsed},
            {"componentRecommendations", componentRecommendations},
            {"summary", summary}
        };
    }
};

struct QoSTelemetry {
    int overallScore = 100; // 0 - 100
    bool staticMemoryCompliant = true;
    bool watchdogCompliant = true;
    bool nonBlockingCompliant = true;
    bool failSafeTimeoutCompliant = true;
    bool powerDecouplingCompliant = true;
    std::vector<std::string> passedGuarantees;
    std::vector<std::string> violations;
    std::string qosTier; // "OUT_OF_THE_ROOF_GRADE", "INDUSTRIAL_GRADE", "STANDARD_GRADE", "DEFECTIVE"

    nlohmann::json toJson() const {
        return {
            {"overallScore", overallScore},
            {"staticMemoryCompliant", staticMemoryCompliant},
            {"watchdogCompliant", watchdogCompliant},
            {"nonBlockingCompliant", nonBlockingCompliant},
            {"failSafeTimeoutCompliant", failSafeTimeoutCompliant},
            {"powerDecouplingCompliant", powerDecouplingCompliant},
            {"passedGuarantees", passedGuarantees},
            {"violations", violations},
            {"qosTier", qosTier}
        };
    }
};

struct ScaffoldOptions {
    std::string projectName = "embedded_project";
    std::string boardId = "esp32";
    std::string framework = "platformio"; // "platformio", "arduino", "esp_idf", "pico_sdk"
    std::vector<std::string> components;
    std::vector<PinAssignment> pinAssignments;
    bool generateWokwiSimulation = true;
    std::string outputDir = ".";
    std::string authorName = "Lunarmist-byte";
    std::string authorGithub = "https://github.com/Lunarmist-byte";
    std::string authorLinkedin = "https://www.linkedin.com/in/amal-s-kumar-ba69a1290/";
};

} // namespace arenafight
