#pragma once

#include "arenafight/tools/tool.hpp"
#include "arenafight/hardware/hardware_manager.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class HardwareListBoardsTool : public Tool {
public:
    std::string name() const override { return "hardware_list_boards"; }
    std::string description() const override {
        return "List all supported microcontroller boards, architectures, logic voltages, and memory specs.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class HardwareInspectBoardTool : public Tool {
public:
    std::string name() const override { return "hardware_inspect_board"; }
    std::string description() const override {
        return "Inspect a microcontroller board's full pinout, strapping pins, input-only pins, and bus mappings.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class HardwareValidateTool : public Tool {
public:
    std::string name() const override { return "hardware_validate"; }
    std::string description() const override {
        return "Validate circuit pinout configuration against MCU constraints, strapping pins, voltage logic levels (5V vs 3.3V), and I2C address collisions.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class HardwareCalculateTool : public Tool {
public:
    std::string name() const override { return "hardware_calculate"; }
    std::string description() const override {
        return "Calculate circuit parameters: LED current limiting resistor, 5V-to-3.3V voltage divider, I2C bus pull-up resistor, or battery deep sleep lifetime.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class HardwareScaffoldTool : public Tool {
public:
    std::string name() const override { return "hardware_scaffold"; }
    std::string description() const override {
        return "Scaffold a complete production-grade embedded project (PlatformIO, Arduino, ESP-IDF) with QoS-grade firmware and Wokwi simulation diagram.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

class HardwareVerifyQoSTool : public Tool {
public:
    std::string name() const override { return "hardware_verify_qos"; }
    std::string description() const override {
        return "Analyze embedded firmware source code for extreme Quality of Service (QoS): static memory enforcement (no heap fragmentation), hardware watchdog timer, non-blocking delays, and fail-safe timeouts.";
    }
    nlohmann::json parameters() const override;
    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir = ".") override;
};

} // namespace arenafight
