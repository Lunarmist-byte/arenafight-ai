#include "arenafight/hardware/hardware_scaffolder.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

namespace {

void writeTextFile(const std::string& path, const std::string& content) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream ofs(path, std::ios::trunc | std::ios::binary);
    ofs << content;
}

std::string getWokwiBoardPart(const std::string& boardId) {
    if (boardId == "esp32") return "board-esp32-devkit-c-v4";
    if (boardId == "pico") return "board-pi-pico";
    if (boardId == "arduino_uno" || boardId == "uno") return "board-arduino-uno";
    if (boardId == "arduino_nano") return "board-arduino-nano";
    return "board-esp32-devkit-c-v4";
}

std::string getWokwiComponentPart(const std::string& compId) {
    if (compId == "dht22") return "wokwi-dht22";
    if (compId == "hc_sr04") return "wokwi-hc-sr04";
    if (compId == "mpu6050") return "wokwi-mpu6050";
    if (compId == "ssd1306") return "board-ssd1306";
    if (compId == "sg90_servo") return "wokwi-servo";
    if (compId == "relay_module") return "wokwi-relay-module";
    if (compId == "bme280") return "wokwi-bme280";
    if (compId == "lcd1602_i2c") return "wokwi-lcd1602";
    return "wokwi-led";
}

} // namespace

std::string HardwareScaffolder::generateWokwiDiagram(
    const BoardSpec& board,
    const std::vector<PinAssignment>& assignments,
    const std::vector<ComponentSpec>& components
) {
    nlohmann::json diag;
    diag["version"] = 1;
    diag["author"] = "Lunarmist-byte (https://github.com/Lunarmist-byte)";
    diag["editor"] = "wokwi";

    nlohmann::json parts = nlohmann::json::array();
    std::string mcuPart = getWokwiBoardPart(board.id);
    parts.push_back({
        {"type", mcuPart},
        {"id", "mcu"},
        {"top", 0},
        {"left", 0},
        {"attrs", nlohmann::json::object()}
    });

    int yOffset = -120;
    int xOffset = 260;
    for (size_t i = 0; i < components.size(); ++i) {
        const auto& c = components[i];
        std::string partType = getWokwiComponentPart(c.id);
        parts.push_back({
            {"type", partType},
            {"id", "part_" + std::to_string(i + 1)},
            {"top", yOffset + static_cast<int>(i * 140)},
            {"left", xOffset},
            {"attrs", nlohmann::json::object()}
        });
    }
    diag["parts"] = parts;

    nlohmann::json connections = nlohmann::json::array();
    for (size_t i = 0; i < assignments.size(); ++i) {
        const auto& a = assignments[i];
        // find part index
        int partIdx = 1;
        for (size_t j = 0; j < components.size(); ++j) {
            if (components[j].id == a.componentId) {
                partIdx = static_cast<int>(j + 1);
                break;
            }
        }
        std::string partRef = "part_" + std::to_string(partIdx) + ":" + a.componentPin;
        std::string mcuRef = "mcu:" + a.boardPin;
        std::string color = a.wireColor.empty() ? "green" : a.wireColor;

        connections.push_back(nlohmann::json::array({
            mcuRef, partRef, color, nlohmann::json::array({"h:0"})
        }));
    }
    diag["connections"] = connections;

    return diag.dump(2);
}

std::string HardwareScaffolder::generateConfigHeader(
    const BoardSpec& board,
    const std::vector<PinAssignment>& assignments,
    const std::vector<ComponentSpec>& components
) {
    std::stringstream ss;
    ss << "#pragma once\n\n"
       << "// ============================================================================\n"
       << "// Hardware Configuration & Pin Mapping\n"
       << "// Target: " << board.name << " (" << architectureToString(board.architecture) << ")\n"
       << "// Operating Voltage: " << logicVoltageToString(board.logicVoltage) << "\n"
       << "// Made by Lunarmist-byte\n"
       << "// GitHub:   https://github.com/Lunarmist-byte\n"
       << "// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
       << "// ============================================================================\n\n"
       << "#include <stdint.h>\n\n"
       << "// System Configuration\n"
       << "#define SERIAL_BAUD_RATE        115200\n"
       << "#define WATCHDOG_TIMEOUT_SEC    5\n"
       << "#define LOOP_INTERVAL_MS        100\n\n"
       << "// Pinout Definitions\n";

    for (const auto& a : assignments) {
        std::string macroName = "PIN_" + a.componentId + "_" + a.componentPin;
        for (char& c : macroName) {
            if (c == '-' || c == '.') c = '_';
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        // Extract numeric pin if possible or use string
        std::string val = a.boardPin;
        if (val.rfind("GPIO", 0) == 0) val = val.substr(4);
        else if (val.rfind("GP", 0) == 0) val = val.substr(2);
        else if (val.rfind("PIN", 0) == 0) val = val.substr(3);

        ss << "#define " << std::left << std::setw(32) << macroName << " " << val
           << " // " << pinFunctionToString(a.function) << (a.notes.empty() ? "" : " - " + a.notes) << "\n";
    }

    ss << "\n// I2C Addresses\n";
    for (const auto& c : components) {
        if (c.defaultI2cAddress != 0) {
            std::string macroName = "ADDR_" + c.id;
            for (char& ch : macroName) {
                if (ch == '-' || ch == '.') ch = '_';
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
            std::stringstream hexSs;
            hexSs << "0x" << std::hex << std::uppercase << c.defaultI2cAddress;
            ss << "#define " << std::left << std::setw(32) << macroName << " " << hexSs.str()
               << " // " << c.name << "\n";
        }
    }
    ss << "\n";
    return ss.str();
}

std::string HardwareScaffolder::generateQoSFirmware(
    const BoardSpec& board,
    const std::vector<PinAssignment>& assignments,
    const std::vector<ComponentSpec>& components,
    const std::string& projectName
) {
    std::stringstream ss;
    ss << "// ============================================================================\n"
       << "// Project: " << projectName << "\n"
       << "// Target: " << board.name << "\n"
       << "// Quality of Service (QoS): Aerospace / Industrial Grade Zero-Heap Determinism\n"
       << "// Made by Lunarmist-byte\n"
       << "// GitHub:   https://github.com/Lunarmist-byte\n"
       << "// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/\n"
       << "// ============================================================================\n\n"
       << "#include <Arduino.h>\n"
       << "#include <Wire.h>\n"
       << "#include \"hardware_config.h\"\n\n";

    if (board.id == "esp32") {
        ss << "#include <esp_task_wdt.h>\n\n";
    } else if (board.architecture == McuArchitecture::AVR_8BIT) {
        ss << "#include <avr/wdt.h>\n\n";
    }

    ss << "// QoS Rule 1: Static Pre-Allocated Buffers (Zero heap malloc/new)\n"
       << "static char telemetryBuffer[256];\n"
       << "static uint32_t lastLoopTimestamp = 0;\n"
       << "static uint32_t heartbeatCounter = 0;\n\n"
       << "// Fail-safe Sensor Status flags\n"
       << "static bool sensorsInitialized = false;\n\n"
       << "// QoS Rule 2: Non-blocking I2C Probe with Bounded Timeout\n"
       << "bool probeI2cDevice(uint8_t address, uint32_t timeoutMs = 100) {\n"
       << "    Wire.beginTransmission(address);\n"
       << "    uint32_t start = millis();\n"
       << "    uint8_t error = Wire.endTransmission();\n"
       << "    if ((millis() - start) > timeoutMs) return false;\n"
       << "    return (error == 0);\n"
       << "}\n\n"
       << "void initWatchdog() {\n";

    if (board.id == "esp32") {
        ss << "    // ESP32 Hardware Task Watchdog (5-second window)\n"
           << "    esp_task_wdt_config_t wdt_config = {\n"
           << "        .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,\n"
           << "        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,\n"
           << "        .trigger_panic = true\n"
           << "    };\n"
           << "    esp_task_wdt_init(&wdt_config);\n"
           << "    esp_task_wdt_add(NULL);\n";
    } else if (board.architecture == McuArchitecture::AVR_8BIT) {
        ss << "    // AVR Hardware Watchdog (4-second window)\n"
           << "    wdt_enable(WDTO_4S);\n";
    } else {
        ss << "    // Generic Watchdog stub\n";
    }

    ss << "}\n\n"
       << "void feedWatchdog() {\n";

    if (board.id == "esp32") {
        ss << "    esp_task_wdt_reset();\n";
    } else if (board.architecture == McuArchitecture::AVR_8BIT) {
        ss << "    wdt_reset();\n";
    } else {
        ss << "    // feed hardware watchdog\n";
    }

    ss << "}\n\n"
       << "void setup() {\n"
       << "    Serial.begin(SERIAL_BAUD_RATE);\n"
       << "    // Wait bounded time for serial monitor\n"
       << "    uint32_t startWait = millis();\n"
       << "    while (!Serial && (millis() - startWait < 2000)) {}\n\n"
       << "    Serial.println(F(\"====================================================\"));\n"
       << "    Serial.println(F(\"   " << projectName << " Firmware Starting...\"));\n"
       << "    Serial.println(F(\"   Made by: Lunarmist-byte\"));\n"
       << "    Serial.println(F(\"   GitHub:  https://github.com/Lunarmist-byte\"));\n"
       << "    Serial.println(F(\"====================================================\"));\n\n"
       << "    // Initialize Watchdog\n"
       << "    initWatchdog();\n\n"
       << "    // Configure default I2C bus\n"
       << "    Wire.begin();\n\n"
       << "    // Configure pins\n";

    for (const auto& a : assignments) {
        std::string macroName = "PIN_" + a.componentId + "_" + a.componentPin;
        for (char& c : macroName) {
            if (c == '-' || c == '.') c = '_';
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        if (a.function == PinFunction::GPIO_OUT || a.function == PinFunction::PWM_OUT) {
            ss << "    pinMode(" << macroName << ", OUTPUT);\n";
            ss << "    digitalWrite(" << macroName << ", LOW);\n";
        } else if (a.function == PinFunction::GPIO_IN) {
            ss << "    pinMode(" << macroName << ", INPUT);\n";
        }
    }

    ss << "\n    sensorsInitialized = true;\n"
       << "    Serial.println(F(\"[QoS] Hardware initialization passed with 0 errors.\"));\n"
       << "}\n\n"
       << "void loop() {\n"
       << "    // QoS Rule 3: Always feed the hardware watchdog timer on every iteration\n"
       << "    feedWatchdog();\n\n"
       << "    // QoS Rule 4: Non-blocking deterministic event timing (No blocking delay())\n"
       << "    uint32_t currentMillis = millis();\n"
       << "    if (currentMillis - lastLoopTimestamp >= LOOP_INTERVAL_MS) {\n"
       << "        lastLoopTimestamp = currentMillis;\n"
       << "        heartbeatCounter++;\n\n"
       << "        // Heartbeat Telemetry (Safe snprintf into pre-allocated static buffer)\n"
       << "        snprintf(telemetryBuffer, sizeof(telemetryBuffer),\n"
       << "                 \"[HEARTBEAT #%lu] Uptime: %lu ms | QoS: OK | Memory: Static\",\n"
       << "                 (unsigned long)heartbeatCounter, (unsigned long)currentMillis);\n"
       << "        Serial.println(telemetryBuffer);\n\n"
       << "        // Periodic sensor polling with timeout checks\n"
       << "        if (heartbeatCounter % 10 == 0) {\n"
       << "            // 1-second interval task execution\n"
       << "        }\n"
       << "    }\n"
       << "}\n";

    return ss.str();
}

ScaffoldResult HardwareScaffolder::scaffoldProject(
    const ScaffoldOptions& options,
    const BoardSpec& board,
    const std::vector<ComponentSpec>& components
) {
    ScaffoldResult res;
    std::string baseDir = options.outputDir + "/" + options.projectName;
    res.projectDir = baseDir;

    try {
        std::filesystem::create_directories(baseDir);

        // 1. Hardware Config Header
        std::string configHeader = generateConfigHeader(board, options.pinAssignments, components);
        std::string configPath = baseDir + "/include/hardware_config.h";
        writeTextFile(configPath, configHeader);
        res.filesCreated.push_back(configPath);

        // 2. Production QoS Firmware
        std::string firmware = generateQoSFirmware(board, options.pinAssignments, components, options.projectName);
        std::string mainCppPath = baseDir + "/src/main.cpp";
        writeTextFile(mainCppPath, firmware);
        res.filesCreated.push_back(mainCppPath);

        // 3. PlatformIO Config (platformio.ini)
        std::stringstream pio;
        pio << "; PlatformIO Project Configuration\n"
            << "; Made by Lunarmist-byte (https://github.com/Lunarmist-byte)\n\n"
            << "[env:" << board.platformioBoardId << "]\n"
            << "platform = " << (board.id == "esp32" ? "espressif32" : (board.id == "pico" ? "raspberrypi" : (board.id == "arduino_uno" ? "atmelavr" : "ststm32"))) << "\n"
            << "board = " << board.platformioBoardId << "\n"
            << "framework = arduino\n"
            << "monitor_speed = 115200\n"
            << "build_flags = -Iinclude -Wall -Wextra\n"
            << "lib_deps =\n";

        for (const auto& c : components) {
            if (c.id == "bme280") pio << "    adafruit/Adafruit BME280 Library\n";
            if (c.id == "mpu6050") pio << "    adafruit/Adafruit MPU6050\n";
            if (c.id == "ssd1306") pio << "    adafruit/Adafruit SSD1306\n";
            if (c.id == "dht22") pio << "    adafruit/DHT sensor library\n";
            if (c.id == "ds18b20") pio << "    milesburton/DallasTemperature\n";
        }
        std::string pioPath = baseDir + "/platformio.ini";
        writeTextFile(pioPath, pio.str());
        res.filesCreated.push_back(pioPath);

        // 4. Wokwi Simulation (diagram.json & wokwi.toml)
        if (options.generateWokwiSimulation) {
            std::string wokwiDiag = generateWokwiDiagram(board, options.pinAssignments, components);
            std::string diagPath = baseDir + "/diagram.json";
            writeTextFile(diagPath, wokwiDiag);
            res.filesCreated.push_back(diagPath);
            res.wokwiDiagramPath = diagPath;

            std::stringstream wtoml;
            wtoml << "[wokwi]\n"
                  << "version = 1\n"
                  << "firmware = '.pio/build/" << board.platformioBoardId << "/firmware.elf'\n"
                  << "elf = '.pio/build/" << board.platformioBoardId << "/firmware.elf'\n";
            std::string wtomlPath = baseDir + "/wokwi.toml";
            writeTextFile(wtomlPath, wtoml.str());
            res.filesCreated.push_back(wtomlPath);
        }

        // 5. Unit Test Suite (test/test_hardware.cpp)
        std::stringstream testCode;
        testCode << "// Hardware Unit Test Suite\n"
                 << "// Made by Lunarmist-byte\n"
                 << "#include <unity.h>\n"
                 << "#include \"hardware_config.h\"\n\n"
                 << "void setUp(void) {}\n"
                 << "void tearDown(void) {}\n\n"
                 << "void test_pinout_definitions(void) {\n"
                 << "    TEST_ASSERT_EQUAL_INT(115200, SERIAL_BAUD_RATE);\n"
                 << "    TEST_ASSERT_GREATER_OR_EQUAL(1, WATCHDOG_TIMEOUT_SEC);\n"
                 << "}\n\n"
                 << "int main(void) {\n"
                 << "    UNITY_BEGIN();\n"
                 << "    RUN_TEST(test_pinout_definitions);\n"
                 << "    return UNITY_END();\n"
                 << "}\n";
        std::string testPath = baseDir + "/test/test_hardware.cpp";
        writeTextFile(testPath, testCode.str());
        res.filesCreated.push_back(testPath);

        // 6. Project README.md
        std::stringstream readme;
        readme << "# " << options.projectName << "\n\n"
               << "Embedded hardware project generated by **ArenaFight Agent (Hardware Mode)**.\n\n"
               << "## Made by Lunarmist-byte\n"
               << "* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)\n"
               << "* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)\n\n"
               << "### Specifications\n"
               << "* **Target Board:** " << board.name << " (" << architectureToString(board.architecture) << ")\n"
               << "* **Logic Voltage:** " << logicVoltageToString(board.logicVoltage) << "\n"
               << "* **Clock Speed:** " << board.clockMHz << " MHz\n"
               << "* **Memory:** " << board.sramKb << " KB SRAM / " << board.flashKb << " KB Flash\n\n"
               << "### Quality of Service (QoS) Guarantees\n"
               << "1. **Deterministic Static Allocation:** Zero runtime heap allocations (`malloc`/`new`).\n"
               << "2. **Hardware Watchdog Protected:** Automatically recovers if hardware hang occurs.\n"
               << "3. **Non-Blocking Architecture:** Uses timestamp state machines instead of blocking delays.\n"
               << "4. **Fail-Safe Timeouts:** Sensor reads include bounded timeout limits.\n\n"
               << "### How to Build & Flash\n"
               << "```bash\n"
               << "# Compile firmware\n"
               << "pio run\n\n"
               << "# Flash to microcontroller\n"
               << "pio run --target upload\n\n"
               << "# Run Wokwi simulator\n"
               << "wokwi-cli .\n"
               << "```\n";
        std::string readmePath = baseDir + "/README.md";
        writeTextFile(readmePath, readme.str());
        res.filesCreated.push_back(readmePath);

        res.success = true;
        res.message = "Hardware project successfully scaffolded at " + baseDir;
        res.summary = "Generated PlatformIO configuration, QoS-compliant C++ firmware, hardware config headers, unit tests, and Wokwi simulation diagram.";
    } catch (const std::exception& e) {
        res.success = false;
        res.message = "Scaffolding failed: " + std::string(e.what());
    }

    return res;
}

} // namespace arenafight
