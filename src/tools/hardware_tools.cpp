#include "arenafight/tools/hardware_tools.hpp"
#include <sstream>
#include <iomanip>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

// 1. HardwareListBoardsTool
nlohmann::json HardwareListBoardsTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"filter", {{"type", "string"}, {"description", "Optional search query to filter boards by name or architecture."}}}
        }}
    };
}

ToolResult HardwareListBoardsTool::execute(const nlohmann::json& arguments, const std::string& /*workingDir*/) {
    ToolResult r;
    r.success = true;
    std::string filter = arguments.value("filter", "");

    auto boards = HardwareBoardRegistry::instance().getAllBoards();
    nlohmann::json arr = nlohmann::json::array();
    std::stringstream ss;
    ss << "Supported Microcontroller Boards:\n";

    for (const auto& b : boards) {
        if (!filter.empty()) {
            if (b.id.find(filter) == std::string::npos && b.name.find(filter) == std::string::npos) {
                continue;
            }
        }
        arr.push_back(b.toJson());
        ss << "- [" << b.id << "] " << b.name << " (" << architectureToString(b.architecture)
           << ", " << logicVoltageToString(b.logicVoltage) << ", " << b.clockMHz << "MHz, Flash: "
           << b.flashKb << "KB, SRAM: " << b.sramKb << "KB)\n";
    }

    r.output = ss.str() + "\n" + arr.dump(2);
    return r;
}

// 2. HardwareInspectBoardTool
nlohmann::json HardwareInspectBoardTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"board", {{"type", "string"}, {"description", "Board ID (e.g. 'esp32', 'pico', 'arduino_uno', 'stm32f401', 'teensy41')."}}}
        }},
        {"required", nlohmann::json::array({"board"})}
    };
}

ToolResult HardwareInspectBoardTool::execute(const nlohmann::json& arguments, const std::string& /*workingDir*/) {
    ToolResult r;
    std::string boardId = arguments.value("board", "esp32");
    auto boardOpt = HardwareBoardRegistry::instance().getBoard(boardId);
    if (!boardOpt) boardOpt = HardwareBoardRegistry::instance().findBoard(boardId);

    if (!boardOpt) {
        r.success = false;
        r.error = "Board not found: " + boardId;
        return r;
    }

    r.success = true;
    const auto& b = *boardOpt;
    std::stringstream ss;
    ss << "=== " << b.name << " (" << b.id << ") ===\n"
       << "Architecture:    " << architectureToString(b.architecture) << "\n"
       << "Clock Speed:     " << b.clockMHz << " MHz\n"
       << "Logic Voltage:   " << logicVoltageToString(b.logicVoltage) << "\n"
       << "Memory:          Flash: " << b.flashKb << " KB, SRAM: " << b.sramKb << " KB\n"
       << "Power:           Active: " << b.activeMilliAmps << " mA, Deep Sleep: " << b.deepSleepMicroAmps << " uA\n"
       << "Default I2C:     SDA=" << b.defaultI2cSda << ", SCL=" << b.defaultI2cScl << "\n"
       << "Default SPI:     MOSI=" << b.defaultSpiMosi << ", MISO=" << b.defaultSpiMiso << ", SCK=" << b.defaultSpiSck << "\n"
       << "Default UART:    TX=" << b.defaultUartTx << ", RX=" << b.defaultUartRx << "\n"
       << "Description:     " << b.description << "\n\n"
       << "Pin Specifications (" << b.pins.size() << " pins defined):\n";

    for (const auto& [name, pin] : b.pins) {
        ss << "  * " << std::left << std::setw(8) << name << " | Phys: " << std::setw(3) << pin.physicalPin;
        if (pin.isStrappingPin) ss << " | [STRAPPING PIN: " << pin.strappingWarning << "]";
        if (pin.isInputOnly) ss << " | [INPUT-ONLY]";
        if (pin.hasAdcWifiConflict) ss << " | [ADC2 WIFI CONFLICT]";
        ss << "\n";
    }

    r.output = ss.str();
    return r;
}

// 3. HardwareValidateTool
nlohmann::json HardwareValidateTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"board", {{"type", "string"}, {"description", "Target board identifier (e.g. 'esp32', 'pico', 'arduino_uno')."}}},
            {"components", {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "List of component IDs."}}},
            {"assignments", {{"type", "array"}, {"description", "Array of pin assignment objects: [{componentId, componentPin, boardPin, function, wireColor, notes}]"}}},
            {"is_wifi_enabled", {{"type", "boolean"}, {"description", "Whether Wi-Fi is active (for ADC2 conflict check)."}}}
        }},
        {"required", nlohmann::json::array({"board"})}
    };
}

ToolResult HardwareValidateTool::execute(const nlohmann::json& arguments, const std::string& /*workingDir*/) {
    ToolResult r;
    std::string boardId = arguments.value("board", "esp32");
    bool isWifi = arguments.value("is_wifi_enabled", false);

    std::vector<std::string> compIds;
    if (arguments.contains("components") && arguments["components"].is_array()) {
        for (const auto& c : arguments["components"]) compIds.push_back(c.get<std::string>());
    }

    std::vector<PinAssignment> assigns;
    if (arguments.contains("assignments") && arguments["assignments"].is_array()) {
        for (const auto& item : arguments["assignments"]) {
            PinAssignment a;
            a.componentId = item.value("componentId", "");
            a.componentPin = item.value("componentPin", "");
            a.boardPin = item.value("boardPin", "");
            a.function = pinFunctionFromString(item.value("function", "GPIO_INOUT"));
            a.wireColor = item.value("wireColor", "green");
            a.notes = item.value("notes", "");
            assigns.push_back(a);
        }
    }

    auto report = HardwareManager::instance().validate(boardId, assigns, compIds, isWifi);
    r.success = report.isValid;

    std::stringstream ss;
    ss << "=== HARDWARE VALIDATION REPORT ===\n"
       << "Result:         " << (report.isValid ? "PASSED (Clean Hardware Design)" : "FAILED (Critical Violations Found)") << "\n"
       << "Critical Errors: " << report.criticalCount << "\n"
       << "Warnings:        " << report.warningCount << "\n\n";

    if (!report.issues.empty()) {
        ss << "Issues Found:\n";
        for (const auto& is : report.issues) {
            ss << "[" << severityToString(is.severity) << "] " << is.code << " (" << is.targetPinOrComponent << "):\n"
               << "  Message: " << is.message << "\n"
               << "  Fix:     " << is.recommendation << "\n";
        }
        ss << "\n";
    }

    if (!report.recommendations.empty()) {
        ss << "Engineering Recommendations:\n";
        for (const auto& rec : report.recommendations) ss << "  * " << rec << "\n";
        ss << "\n";
    }

    if (!report.asciiWiringSummary.empty()) {
        ss << report.asciiWiringSummary;
    }

    r.output = ss.str();
    if (!report.isValid) {
        r.error = "Hardware validation failed with " + std::to_string(report.criticalCount) + " critical error(s).";
    }
    return r;
}

// 4. HardwareCalculateTool
nlohmann::json HardwareCalculateTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"type", {{"type", "string"}, {"description", "Calculation type: 'led', 'divider', 'i2c', or 'battery'."}}},
            {"supply_voltage", {{"type", "number"}, {"description", "Supply voltage (V)."}}},
            {"led_forward_voltage", {{"type", "number"}, {"description", "LED forward voltage Vf (e.g. 2.0 for Red, 3.2 for Blue/White)."}}},
            {"led_current_ma", {{"type", "number"}, {"description", "Desired LED forward current in mA (default 15mA)."}}},
            {"input_voltage", {{"type", "number"}, {"description", "Divider input voltage Vin (e.g. 5.0V)."}}},
            {"target_output_voltage", {{"type", "number"}, {"description", "Divider target output voltage Vout (e.g. 3.3V)."}}},
            {"target_resistance_k", {{"type", "number"}, {"description", "Target divider total resistance in kOhms (default 10k)."}}},
            {"bus_speed_khz", {{"type", "integer"}, {"description", "I2C bus clock speed in kHz (100 or 400)."}}},
            {"bus_capacitance_pf", {{"type", "number"}, {"description", "I2C estimated bus capacitance in pF (default 100pF)."}}},
            {"battery_capacity_mah", {{"type", "number"}, {"description", "Battery capacity in mAh (e.g. 2500)."}}},
            {"active_current_ma", {{"type", "number"}, {"description", "Active microcontroller current in mA (e.g. 120)."}}},
            {"sleep_current_ua", {{"type", "number"}, {"description", "Deep sleep current in microamps uA (e.g. 10)."}}},
            {"active_seconds", {{"type", "number"}, {"description", "Active run time seconds per cycle (e.g. 5)."}}},
            {"sleep_seconds", {{"type", "number"}, {"description", "Deep sleep time seconds per cycle (e.g. 300)."}}}
        }},
        {"required", nlohmann::json::array({"type"})}
    };
}

ToolResult HardwareCalculateTool::execute(const nlohmann::json& arguments, const std::string& /*workingDir*/) {
    ToolResult r;
    r.success = true;
    std::string type = arguments.value("type", "led");

    if (type == "led") {
        double vcc = arguments.value("supply_voltage", 5.0);
        double vf = arguments.value("led_forward_voltage", 2.0);
        double ma = arguments.value("led_current_ma", 15.0);
        auto calc = HardwareManager::instance().calculateLed(vcc, vf, ma);
        r.output = calc.summary + "\n\n" + calc.toJson().dump(2);
    } else if (type == "divider") {
        double vin = arguments.value("input_voltage", 5.0);
        double vout = arguments.value("target_output_voltage", 3.3);
        double rk = arguments.value("target_resistance_k", 10.0);
        auto calc = HardwareManager::instance().calculateDivider(vin, vout, rk);
        r.output = calc.summary + "\n\n" + calc.toJson().dump(2);
    } else if (type == "i2c") {
        double vcc = arguments.value("supply_voltage", 3.3);
        int spd = arguments.value("bus_speed_khz", 100);
        double cb = arguments.value("bus_capacitance_pf", 100.0);
        auto calc = HardwareManager::instance().calculateI2c(vcc, spd, cb);
        r.output = calc.summary + "\n\n" + calc.toJson().dump(2);
    } else if (type == "battery") {
        double cap = arguments.value("battery_capacity_mah", 2500.0);
        double actMa = arguments.value("active_current_ma", 120.0);
        double slpUa = arguments.value("sleep_current_ua", 10.0);
        double actSec = arguments.value("active_seconds", 5.0);
        double slpSec = arguments.value("sleep_seconds", 300.0);
        auto calc = HardwareManager::instance().calculateBattery(cap, actMa, slpUa, actSec, slpSec);
        r.output = calc.summary + "\n\n" + calc.toJson().dump(2);
    } else {
        r.success = false;
        r.error = "Unknown calculation type: " + type + ". Choose 'led', 'divider', 'i2c', or 'battery'.";
    }
    return r;
}

// 5. HardwareScaffoldTool
nlohmann::json HardwareScaffoldTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"project_name", {{"type", "string"}, {"description", "Name of the embedded project folder."}}},
            {"board", {{"type", "string"}, {"description", "Board ID (e.g. 'esp32', 'pico', 'arduino_uno', 'stm32f401')."}}},
            {"framework", {{"type", "string"}, {"description", "Framework: 'platformio' (recommended), 'arduino', or 'esp_idf'."}}},
            {"components", {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "Components to include (e.g. ['bme280', 'ssd1306', 'sg90_servo'])."}}},
            {"output_dir", {{"type", "string"}, {"description", "Output directory to create project in (default current dir)."}}}
        }},
        {"required", nlohmann::json::array({"project_name"})}
    };
}

ToolResult HardwareScaffoldTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult r;
    ScaffoldOptions opts;
    opts.projectName = arguments.value("project_name", "iot_sensor_node");
    opts.boardId = arguments.value("board", "esp32");
    opts.framework = arguments.value("framework", "platformio");
    opts.outputDir = arguments.value("output_dir", workingDir);

    if (arguments.contains("components") && arguments["components"].is_array()) {
        for (const auto& c : arguments["components"]) opts.components.push_back(c.get<std::string>());
    }

    // Auto-generate standard pin assignments if none passed
    auto boardOpt = HardwareBoardRegistry::instance().getBoard(opts.boardId);
    if (!boardOpt) boardOpt = HardwareBoardRegistry::instance().findBoard(opts.boardId);
    if (!boardOpt) boardOpt = HardwareBoardRegistry::instance().getBoard("esp32");

    for (const auto& cId : opts.components) {
        auto compOpt = HardwareBoardRegistry::instance().getComponent(cId);
        if (compOpt) {
            for (const auto& pin : compOpt->requiredPins) {
                PinAssignment a;
                a.componentId = compOpt->id;
                a.componentPin = pin;
                if (pin == "SDA") {
                    a.boardPin = boardOpt->defaultI2cSda;
                    a.function = PinFunction::I2C_SDA;
                    a.wireColor = "blue";
                } else if (pin == "SCL") {
                    a.boardPin = boardOpt->defaultI2cScl;
                    a.function = PinFunction::I2C_SCL;
                    a.wireColor = "yellow";
                } else if (pin == "VCC" || pin == "5V" || pin == "VDD" || pin == "VIN") {
                    a.boardPin = (compOpt->operatingVoltage == LogicVoltage::VOLTAGE_5V) ? "5V" : "3V3";
                    a.function = PinFunction::POWER_VCC;
                    a.wireColor = "red";
                } else if (pin == "GND") {
                    a.boardPin = "GND";
                    a.function = PinFunction::POWER_GND;
                    a.wireColor = "black";
                } else {
                    // Assign generic GPIO
                    a.boardPin = boardOpt->defaultLedPin;
                    a.function = PinFunction::GPIO_OUT;
                    a.wireColor = "green";
                }
                opts.pinAssignments.push_back(a);
            }
        }
    }

    auto res = HardwareManager::instance().scaffold(opts);
    r.success = res.success;
    if (res.success) {
        std::stringstream ss;
        ss << res.message << "\n\nFiles Created:\n";
        for (const auto& f : res.filesCreated) ss << "  - " << f << "\n";
        ss << "\nWokwi Simulation Diagram: " << res.wokwiDiagramPath << "\n\n"
           << "Made by Lunarmist-byte (https://github.com/Lunarmist-byte)\n";
        r.output = ss.str();
    } else {
        r.error = res.message;
    }
    return r;
}

// 6. HardwareVerifyQoSTool
nlohmann::json HardwareVerifyQoSTool::parameters() const {
    return {
        {"type", "object"},
        {"properties", {
            {"path", {{"type", "string"}, {"description", "Path to source file or project directory to evaluate."}}}
        }},
        {"required", nlohmann::json::array({"path"})}
    };
}

ToolResult HardwareVerifyQoSTool::execute(const nlohmann::json& arguments, const std::string& workingDir) {
    ToolResult r;
    std::string path = arguments.value("path", ".");
    if (path == "." || path.rfind("./", 0) == 0) {
        path = workingDir + "/" + path;
    }

    auto qos = HardwareManager::instance().evaluateProjectQoS(path);
    r.success = (qos.overallScore >= 75);

    std::stringstream ss;
    ss << "=== EMBEDDED HARDWARE QoS TELEMETRY ===\n"
       << "Overall QoS Score:   " << qos.overallScore << " / 100\n"
       << "QoS Rating Tier:     " << qos.qosTier << "\n"
       << "Made by:             Lunarmist-byte\n"
       << "GitHub:              https://github.com/Lunarmist-byte\n\n"
       << "Quality Guarantees Verified:\n";
    for (const auto& g : qos.passedGuarantees) ss << "  [PASS] " << g << "\n";

    if (!qos.violations.empty()) {
        ss << "\nDetected Quality & Reliability Hazards:\n";
        for (const auto& v : qos.violations) ss << "  [VIOLATION] " << v << "\n";
    }

    r.output = ss.str();
    if (!r.success) {
        r.error = "QoS evaluation failed to meet minimum threshold (Score: " + std::to_string(qos.overallScore) + " / 100).";
    }
    return r;
}

} // namespace arenafight
