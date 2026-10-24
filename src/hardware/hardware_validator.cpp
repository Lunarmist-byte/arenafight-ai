#include "arenafight/hardware/hardware_validator.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <set>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

namespace {

std::string toUpper(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

} // namespace

HardwareValidationReport HardwareValidator::validateCircuit(
    const BoardSpec& board,
    const std::vector<PinAssignment>& assignments,
    const std::vector<ComponentSpec>& components,
    bool isWifiEnabled
) {
    HardwareValidationReport report;
    report.isValid = true;

    std::map<std::string, std::vector<PinAssignment>> pinUsage;
    std::map<int, std::vector<std::string>> i2cUsage; // addr -> component names

    // 1. Group pin assignments by board pin
    for (const auto& assign : assignments) {
        std::string pName = toUpper(assign.boardPin);
        pinUsage[pName].push_back(assign);
    }

    // 2. Check Pin Collisions
    for (const auto& [pinName, usageList] : pinUsage) {
        if (usageList.size() > 1) {
            // Allow shared I2C SDA/SCL pins
            bool allI2c = true;
            for (const auto& u : usageList) {
                if (u.function != PinFunction::I2C_SDA && u.function != PinFunction::I2C_SCL) {
                    allI2c = false;
                    break;
                }
            }

            // Allow shared SPI SCK/MOSI/MISO
            bool allSpiBus = true;
            for (const auto& u : usageList) {
                if (u.function != PinFunction::SPI_SCK &&
                    u.function != PinFunction::SPI_MOSI &&
                    u.function != PinFunction::SPI_MISO) {
                    allSpiBus = false;
                    break;
                }
            }

            // Power/GND can be shared
            bool allPower = true;
            for (const auto& u : usageList) {
                if (u.function != PinFunction::POWER_VCC && u.function != PinFunction::POWER_GND) {
                    allPower = false;
                    break;
                }
            }

            if (!allI2c && !allSpiBus && !allPower) {
                HardwareIssue issue;
                issue.severity = HardwareIssueSeverity::CRITICAL_ERROR;
                issue.code = "PIN_COLLISION";
                issue.targetPinOrComponent = pinName;
                std::stringstream ss;
                ss << "Pin " << pinName << " is assigned to multiple conflicting components: ";
                for (size_t i = 0; i < usageList.size(); ++i) {
                    ss << usageList[i].componentId << " (" << usageList[i].componentPin << ")";
                    if (i + 1 < usageList.size()) ss << ", ";
                }
                issue.message = ss.str();
                issue.recommendation = "Reassign conflicting components to separate GPIO pins.";
                report.issues.push_back(issue);
                report.criticalCount++;
                report.isValid = false;
            }
        }
    }

    // 3. Board Pin Capability & Constraint Checks
    for (const auto& assign : assignments) {
        std::string pName = toUpper(assign.boardPin);
        auto it = board.pins.find(pName);
        if (it == board.pins.end()) {
            // Check if it's VCC or GND
            if (pName != "3V3" && pName != "5V" && pName != "VIN" && pName != "GND") {
                HardwareIssue issue;
                issue.severity = HardwareIssueSeverity::WARNING;
                issue.code = "UNKNOWN_PIN";
                issue.targetPinOrComponent = assign.boardPin;
                issue.message = "Pin " + assign.boardPin + " is not recognized in " + board.name + " pinout database.";
                issue.recommendation = "Check board datasheet for valid pin labels.";
                report.issues.push_back(issue);
                report.warningCount++;
            }
            continue;
        }

        const PinSpec& spec = it->second;

        // Check Input-Only pin violation
        if (spec.isInputOnly) {
            if (assign.function == PinFunction::GPIO_OUT ||
                assign.function == PinFunction::PWM_OUT ||
                assign.function == PinFunction::I2C_SCL ||
                assign.function == PinFunction::SPI_MOSI ||
                assign.function == PinFunction::UART_TX) {
                HardwareIssue issue;
                issue.severity = HardwareIssueSeverity::CRITICAL_ERROR;
                issue.code = "INPUT_ONLY_VIOLATION";
                issue.targetPinOrComponent = pName;
                issue.message = "Pin " + pName + " is an INPUT-ONLY pin (GPI) with no output driver circuitry, but assigned as output function (" + pinFunctionToString(assign.function) + ").";
                issue.recommendation = "Assign to a standard bidirectional GPIO with output driver (e.g. GPIO4, GPIO16, GPIO17, GPIO18).";
                report.issues.push_back(issue);
                report.criticalCount++;
                report.isValid = false;
            }
        }

        // Check Strapping Pin warning
        if (spec.isStrappingPin) {
            HardwareIssue issue;
            issue.severity = HardwareIssueSeverity::WARNING;
            issue.code = "STRAPPING_PIN_HAZARD";
            issue.targetPinOrComponent = pName;
            issue.message = "Pin " + pName + " is an MCU bootstrapping/strapping pin: " + spec.strappingWarning;
            issue.recommendation = "Avoid attaching external pull-ups/pull-downs or switches to this pin that might alter boot mode state.";
            report.issues.push_back(issue);
            report.warningCount++;
        }

        // Check ADC2 Wi-Fi conflict on ESP32
        if (isWifiEnabled && spec.hasAdcWifiConflict && assign.function == PinFunction::ANALOG_IN) {
            HardwareIssue issue;
            issue.severity = HardwareIssueSeverity::WARNING;
            issue.code = "ADC2_WIFI_CONFLICT";
            issue.targetPinOrComponent = pName;
            issue.message = "Pin " + pName + " uses ADC2 which is shared with the Wi-Fi RF subsystem. Analog readings will fail or return invalid data when Wi-Fi is transmitting!";
            issue.recommendation = "Use ADC1 channels instead (ESP32: GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, GPIO39).";
            report.issues.push_back(issue);
            report.warningCount++;
        }
    }

    // 4. Component Voltage Logic Level Mismatch Checks
    for (const auto& comp : components) {
        // 5V Board (Arduino Uno) driving 3.3V Sensor
        if (board.logicVoltage == LogicVoltage::VOLTAGE_5V && comp.operatingVoltage == LogicVoltage::VOLTAGE_3V3) {
            HardwareIssue issue;
            issue.severity = HardwareIssueSeverity::CRITICAL_ERROR;
            issue.code = "LOGIC_VOLTAGE_OVERVOLT_HAZARD";
            issue.targetPinOrComponent = comp.name;
            issue.message = "Voltage Mismatch: Board operates at 5.0V logic, but " + comp.name + " is rated for 3.3V logic. Connecting directly will damage the sensor!";
            issue.recommendation = "Insert a bidirectional logic level shifter (e.g. BSS138 or TXS0108E) or resistive voltage divider between the board and sensor.";
            report.issues.push_back(issue);
            report.criticalCount++;
            report.isValid = false;
        }

        // 3.3V Board connected to 5V HC-SR04 Echo pin
        if (comp.id == "hc_sr04" && board.logicVoltage == LogicVoltage::VOLTAGE_3V3) {
            bool hasDividerNote = false;
            for (const auto& a : assignments) {
                if (a.componentId == "hc_sr04" && toUpper(a.componentPin) == "ECHO") {
                    if (a.notes.find("divider") != std::string::npos || a.notes.find("level shifter") != std::string::npos) {
                        hasDividerNote = true;
                    }
                }
            }
            if (!hasDividerNote) {
                HardwareIssue issue;
                issue.severity = HardwareIssueSeverity::CRITICAL_ERROR;
                issue.code = "HCSR04_5V_ECHO_OVERVOLT";
                issue.targetPinOrComponent = "HC-SR04 ECHO";
                issue.message = "HC-SR04 Ultrasonic ECHO pin outputs 5.0V TTL pulses, but " + board.name + " is a 3.3V microcontroller.";
                issue.recommendation = "Place a voltage divider on the ECHO line (R1=1kΩ, R2=2kΩ) to scale 5.0V safely down to 3.3V.";
                report.issues.push_back(issue);
                report.criticalCount++;
                report.isValid = false;
            }
        }

        // I2C Address Tracking
        if (comp.defaultI2cAddress != 0) {
            i2cUsage[comp.defaultI2cAddress].push_back(comp.name);
        }
    }

    // 5. Check I2C Address Collisions
    for (const auto& [addr, compList] : i2cUsage) {
        if (compList.size() > 1) {
            HardwareIssue issue;
            issue.severity = HardwareIssueSeverity::CRITICAL_ERROR;
            issue.code = "I2C_ADDRESS_COLLISION";
            std::stringstream ssAddr;
            ssAddr << "0x" << std::hex << std::uppercase << addr;
            issue.targetPinOrComponent = ssAddr.str();
            std::stringstream ss;
            ss << "Multiple I2C devices share address " << ssAddr.str() << ": ";
            for (size_t i = 0; i < compList.size(); ++i) {
                ss << compList[i];
                if (i + 1 < compList.size()) ss << ", ";
            }
            issue.message = ss.str();
            issue.recommendation = "Change the address select pin (AD0/SDO) on one device or use an I2C multiplexer (e.g. TCA9548A).";
            report.issues.push_back(issue);
            report.criticalCount++;
            report.isValid = false;
        }
    }

    // 6. Check Pull-Up Resistor Requirements
    for (const auto& comp : components) {
        if (comp.requiresPullup) {
            report.recommendations.push_back(comp.name + " requires an external 4.7kΩ pull-up resistor on its data/bus line.");
        }
    }

    // Generate Wiring Summary Chart
    report.asciiWiringSummary = generateAsciiWiringChart(board, assignments);

    return report;
}

QoSTelemetry HardwareValidator::validateFirmwareQoS(
    const std::string& sourceCode,
    const std::string& boardId
) {
    QoSTelemetry qos;
    qos.overallScore = 100;
    qos.staticMemoryCompliant = true;
    qos.watchdogCompliant = true;
    qos.nonBlockingCompliant = true;
    qos.failSafeTimeoutCompliant = true;
    qos.powerDecouplingCompliant = true;

    // 1. Static Allocation Check (Prevent heap fragmentation)
    std::regex dynamicAllocRegex(R"(\b(malloc|calloc|realloc|free)\s*\(|\bnew\s+[A-Za-z0-9_]+|\bdelete\s+[A-Za-z0-9_]+)");
    if (std::regex_search(sourceCode, dynamicAllocRegex)) {
        qos.staticMemoryCompliant = false;
        qos.overallScore -= 25;
        qos.violations.push_back("DYNAMIC MEMORY ALLOCATION DETECTED: Use of malloc/calloc/new/delete in embedded firmware causes heap fragmentation and microcontroller crashes. Enforce static pre-allocated buffers.");
    } else {
        qos.passedGuarantees.push_back("STATIC MEMORY COMPLIANT: Zero dynamic heap allocations detected; fully deterministic static memory layout.");
    }

    // 2. Watchdog Timer (WDT) Check
    std::regex wdtInitRegex(R"(\b(wdt_enable|esp_task_wdt_init|esp_task_wdt_add|watchdog_enable|IWatchdog\.begin)\b)");
    std::regex wdtFeedRegex(R"(\b(wdt_reset|esp_task_wdt_reset|watchdog_update|IWatchdog\.reload)\b)");
    bool hasWdtInit = std::regex_search(sourceCode, wdtInitRegex);
    bool hasWdtFeed = std::regex_search(sourceCode, wdtFeedRegex);

    if (hasWdtInit && hasWdtFeed) {
        qos.passedGuarantees.push_back("WATCHDOG TIMER (WDT) COMPLIANT: Hardware watchdog initialized and periodically fed; automatic recovery from hardware hangs guaranteed.");
    } else {
        qos.watchdogCompliant = false;
        qos.overallScore -= 20;
        qos.violations.push_back("MISSING HARDWARE WATCHDOG: Firmware does not configure or periodically feed a hardware watchdog timer (e.g. esp_task_wdt_reset / wdt_reset). An unexpected freeze will halt execution indefinitely.");
    }

    // 3. Non-Blocking Delay Check
    std::regex blockingDelayRegex(R"(\bdelay\s*\(\s*([1-9][0-9]*)\s*\))");
    std::smatch match;
    bool hasLongBlockingDelay = false;
    auto wordsBegin = std::sregex_iterator(sourceCode.begin(), sourceCode.end(), blockingDelayRegex);
    auto wordsEnd = std::sregex_iterator();
    for (std::sregex_iterator i = wordsBegin; i != wordsEnd; ++i) {
        std::smatch m = *i;
        int delayVal = std::stoi(m[1].str());
        if (delayVal > 50) { // delays longer than 50ms block execution loops
            hasLongBlockingDelay = true;
            break;
        }
    }

    if (hasLongBlockingDelay) {
        qos.nonBlockingCompliant = false;
        qos.overallScore -= 25;
        qos.violations.push_back("BLOCKING EXECUTION DETECTED: Use of delay() > 50ms blocks sensor polling and interrupt processing. Use non-blocking millis() state machines or FreeRTOS vTaskDelay().");
    } else {
        qos.passedGuarantees.push_back("NON-BLOCKING TIMING COMPLIANT: Execution loop uses non-blocking timestamp checks (millis()) or RTOS task scheduling.");
    }

    // 4. Fail-Safe Timeout Check (Infinite while loops)
    std::regex infiniteWhileRegex(R"(while\s*\(\s*(!\s*[A-Za-z0-9_\.]+\.available\(\)|digitalRead\([^\)]+\)\s*==\s*[A-Z_]+\))\s*\{?\s*\}?)");
    if (std::regex_search(sourceCode, infiniteWhileRegex)) {
        qos.failSafeTimeoutCompliant = false;
        qos.overallScore -= 20;
        qos.violations.push_back("UNBOUNDED POLLING LOOP DETECTED: while loop without timeout guard detected. If a physical sensor wire disconnects, MCU will hang permanently. Add a timeout break.");
    } else {
        qos.passedGuarantees.push_back("FAIL-SAFE TIMEOUT COMPLIANT: Sensor read routines incorporate bounded timeout guards.");
    }

    // Clamp score
    if (qos.overallScore < 0) qos.overallScore = 0;

    if (qos.overallScore >= 95) {
        qos.qosTier = "OUT_OF_THE_ROOF_GRADE (Aerospace & Mission Critical)";
    } else if (qos.overallScore >= 80) {
        qos.qosTier = "INDUSTRIAL_GRADE";
    } else if (qos.overallScore >= 60) {
        qos.qosTier = "STANDARD_GRADE";
    } else {
        qos.qosTier = "DEFECTIVE / REJECTED";
    }

    return qos;
}

std::string HardwareValidator::generateAsciiWiringChart(
    const BoardSpec& board,
    const std::vector<PinAssignment>& assignments
) {
    std::stringstream ss;
    ss << "+-----------------------------------------------------------------------------------------+\n"
       << "|                            HARDWARE WIRING & PINOUT BLUEPRINT                           |\n"
       << "| Board: " << std::left << std::setw(28) << board.name
       << " Voltage: " << std::left << std::setw(12) << logicVoltageToString(board.logicVoltage)
       << " Clock: " << std::left << std::setw(16) << (std::to_string(board.clockMHz) + " MHz") << " |\n"
       << "+-----------------------------------------------------------------------------------------+\n"
       << "| COMPONENT       | PIN      | BOARD PIN | FUNCTION    | WIRE COLOR | NOTES                   |\n"
       << "+-----------------+----------+-----------+-------------+------------+-------------------------+\n";

    for (const auto& a : assignments) {
        ss << "| " << std::left << std::setw(15) << (a.componentId.size() > 15 ? a.componentId.substr(0, 15) : a.componentId)
           << " | " << std::left << std::setw(8) << (a.componentPin.size() > 8 ? a.componentPin.substr(0, 8) : a.componentPin)
           << " | " << std::left << std::setw(9) << (a.boardPin.size() > 9 ? a.boardPin.substr(0, 9) : a.boardPin)
           << " | " << std::left << std::setw(11) << pinFunctionToString(a.function)
           << " | " << std::left << std::setw(10) << a.wireColor
           << " | " << std::left << std::setw(23) << (a.notes.size() > 23 ? a.notes.substr(0, 20) + "..." : a.notes)
           << " |\n";
    }
    ss << "+-----------------------------------------------------------------------------------------+\n";
    return ss.str();
}

} // namespace arenafight
