#include "arenafight/hardware/board_registry.hpp"
#include <algorithm>
#include <cctype>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

namespace {

std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

PinSpec createGpio(const std::string& name, int phys, LogicVoltage v = LogicVoltage::VOLTAGE_3V3, bool pwm = true) {
    PinSpec p;
    p.name = name;
    p.physicalPin = phys;
    p.supportedFunctions = { PinFunction::GPIO_IN, PinFunction::GPIO_OUT, PinFunction::GPIO_INOUT };
    p.voltage = v;
    p.isPwmCapable = pwm;
    return p;
}

} // namespace

HardwareBoardRegistry& HardwareBoardRegistry::instance() {
    static HardwareBoardRegistry inst;
    return inst;
}

HardwareBoardRegistry::HardwareBoardRegistry() {
    initStandardBoards();
    initStandardComponents();
}

std::vector<BoardSpec> HardwareBoardRegistry::getAllBoards() const {
    std::vector<BoardSpec> list;
    for (const auto& [_, b] : boards_) list.push_back(b);
    return list;
}

std::optional<BoardSpec> HardwareBoardRegistry::getBoard(const std::string& boardId) const {
    auto it = boards_.find(toLower(boardId));
    if (it != boards_.end()) return it->second;
    return std::nullopt;
}

std::optional<BoardSpec> HardwareBoardRegistry::findBoard(const std::string& query) const {
    std::string q = toLower(query);
    for (const auto& [id, b] : boards_) {
        if (id == q || toLower(b.name).find(q) != std::string::npos) {
            return b;
        }
    }
    return std::nullopt;
}

std::vector<ComponentSpec> HardwareBoardRegistry::getAllComponents() const {
    std::vector<ComponentSpec> list;
    for (const auto& [_, c] : components_) list.push_back(c);
    return list;
}

std::optional<ComponentSpec> HardwareBoardRegistry::getComponent(const std::string& componentId) const {
    auto it = components_.find(toLower(componentId));
    if (it != components_.end()) return it->second;
    return std::nullopt;
}

std::optional<ComponentSpec> HardwareBoardRegistry::findComponent(const std::string& query) const {
    std::string q = toLower(query);
    for (const auto& [id, c] : components_) {
        if (id == q || toLower(c.name).find(q) != std::string::npos) {
            return c;
        }
    }
    return std::nullopt;
}

void HardwareBoardRegistry::registerBoard(BoardSpec spec) {
    boards_[toLower(spec.id)] = std::move(spec);
}

void HardwareBoardRegistry::registerComponent(ComponentSpec spec) {
    components_[toLower(spec.id)] = std::move(spec);
}

void HardwareBoardRegistry::initStandardBoards() {
    // 1. ESP32 WROOM-32 (NodeMCU-32S / ESP32 DevKit)
    {
        BoardSpec esp32;
        esp32.id = "esp32";
        esp32.name = "ESP32 DevKit V1 (WROOM-32)";
        esp32.architecture = McuArchitecture::XTENSA_LX6;
        esp32.clockMHz = 240;
        esp32.logicVoltage = LogicVoltage::VOLTAGE_3V3;
        esp32.flashKb = 4096;
        esp32.sramKb = 520;
        esp32.deepSleepMicroAmps = 10.0;
        esp32.activeMilliAmps = 160.0;
        esp32.defaultI2cSda = "GPIO21";
        esp32.defaultI2cScl = "GPIO22";
        esp32.defaultSpiMosi = "GPIO23";
        esp32.defaultSpiMiso = "GPIO19";
        esp32.defaultSpiSck = "GPIO18";
        esp32.defaultUartTx = "GPIO1";
        esp32.defaultUartRx = "GPIO3";
        esp32.defaultLedPin = "GPIO2";
        esp32.platformioBoardId = "esp32dev";
        esp32.arduinoFqbn = "esp32:esp32:esp32";
        esp32.description = "Popular dual-core 240MHz Wi-Fi & Bluetooth microcontroller with ultra-low deep sleep.";

        // Strapping Pins
        auto addPin = [&](const std::string& name, int phys, bool strap, const std::string& strapWarn, bool inOnly, bool adcWifi, const std::vector<PinFunction>& fns) {
            PinSpec p;
            p.name = name;
            p.physicalPin = phys;
            p.supportedFunctions = fns;
            p.voltage = LogicVoltage::VOLTAGE_3V3;
            p.isStrappingPin = strap;
            p.strappingWarning = strapWarn;
            p.isInputOnly = inOnly;
            p.hasAdcWifiConflict = adcWifi;
            p.isPwmCapable = !inOnly;
            p.is5VTolerant = false;
            esp32.pins[name] = p;
        };

        addPin("GPIO0", 25, true, "Must be HIGH for normal SPI boot; pulled LOW triggers UART download bootloader mode.", false, true, {PinFunction::GPIO_INOUT, PinFunction::BOOT_PIN});
        addPin("GPIO2", 24, true, "Connected to onboard LED. Must be left floating or LOW during flashing.", false, true, {PinFunction::GPIO_INOUT, PinFunction::PWM_OUT});
        addPin("GPIO4", 26, false, "", false, true, {PinFunction::GPIO_INOUT, PinFunction::PWM_OUT, PinFunction::ANALOG_IN});
        addPin("GPIO5", 29, true, "Timing pin for SDIO slave; outputs PWM at boot. Recommended SPI CS.", false, false, {PinFunction::GPIO_INOUT, PinFunction::SPI_CS});
        addPin("GPIO12", 14, true, "MTDI bootstrapping. If pulled HIGH at boot, flash voltage sets to 1.8V instead of 3.3V, causing boot failure!", false, true, {PinFunction::GPIO_INOUT, PinFunction::PWM_OUT});
        addPin("GPIO13", 16, false, "", false, true, {PinFunction::GPIO_INOUT, PinFunction::PWM_OUT, PinFunction::ANALOG_IN});
        addPin("GPIO14", 13, false, "", false, true, {PinFunction::GPIO_INOUT, PinFunction::PWM_OUT, PinFunction::ANALOG_IN});
        addPin("GPIO15", 23, true, "MTDO bootstrapping. If pulled LOW, debug output during boot is silenced.", false, true, {PinFunction::GPIO_INOUT, PinFunction::PWM_OUT});
        addPin("GPIO16", 27, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::UART_RX});
        addPin("GPIO17", 28, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::UART_TX});
        addPin("GPIO18", 30, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::SPI_SCK});
        addPin("GPIO19", 31, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::SPI_MISO});
        addPin("GPIO21", 33, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::I2C_SDA});
        addPin("GPIO22", 36, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::I2C_SCL});
        addPin("GPIO23", 37, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::SPI_MOSI});
        addPin("GPIO25", 10, false, "", false, true, {PinFunction::GPIO_INOUT, PinFunction::DAC_OUT, PinFunction::ANALOG_IN});
        addPin("GPIO26", 11, false, "", false, true, {PinFunction::GPIO_INOUT, PinFunction::DAC_OUT, PinFunction::ANALOG_IN});
        addPin("GPIO27", 12, false, "", false, true, {PinFunction::GPIO_INOUT, PinFunction::PWM_OUT, PinFunction::ANALOG_IN});
        addPin("GPIO32", 7, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::ANALOG_IN, PinFunction::PWM_OUT});
        addPin("GPIO33", 8, false, "", false, false, {PinFunction::GPIO_INOUT, PinFunction::ANALOG_IN, PinFunction::PWM_OUT});
        // Input-Only Pins (GPI)
        addPin("GPIO34", 5, false, "", true, false, {PinFunction::GPIO_IN, PinFunction::ANALOG_IN});
        addPin("GPIO35", 6, false, "", true, false, {PinFunction::GPIO_IN, PinFunction::ANALOG_IN});
        addPin("GPIO36", 3, false, "", true, false, {PinFunction::GPIO_IN, PinFunction::ANALOG_IN}); // SENSOR_VP
        addPin("GPIO39", 4, false, "", true, false, {PinFunction::GPIO_IN, PinFunction::ANALOG_IN}); // SENSOR_VN

        boards_[esp32.id] = esp32;
    }

    // 2. Raspberry Pi Pico (RP2040)
    {
        BoardSpec pico;
        pico.id = "pico";
        pico.name = "Raspberry Pi Pico (RP2040)";
        pico.architecture = McuArchitecture::ARM_CORTEX_M0PLUS;
        pico.clockMHz = 133;
        pico.logicVoltage = LogicVoltage::VOLTAGE_3V3;
        pico.flashKb = 2048;
        pico.sramKb = 264;
        pico.deepSleepMicroAmps = 180.0;
        pico.activeMilliAmps = 40.0;
        pico.defaultI2cSda = "GP4";
        pico.defaultI2cScl = "GP5";
        pico.defaultSpiMosi = "GP19";
        pico.defaultSpiMiso = "GP16";
        pico.defaultSpiSck = "GP18";
        pico.defaultUartTx = "GP0";
        pico.defaultUartRx = "GP1";
        pico.defaultLedPin = "GP25";
        pico.platformioBoardId = "pico";
        pico.arduinoFqbn = "rp2040:rp2040:rpipico";
        pico.description = "Dual Cortex-M0+ 133MHz MCU with programmable I/O (PIO), rich DMA, and 264KB RAM.";

        for (int i = 0; i <= 28; ++i) {
            std::string name = "GP" + std::to_string(i);
            PinSpec p = createGpio(name, i + 1, LogicVoltage::VOLTAGE_3V3, true);
            if (i >= 26 && i <= 28) {
                p.supportedFunctions.push_back(PinFunction::ANALOG_IN);
            }
            if (i == 4) p.supportedFunctions.push_back(PinFunction::I2C_SDA);
            if (i == 5) p.supportedFunctions.push_back(PinFunction::I2C_SCL);
            if (i == 16) p.supportedFunctions.push_back(PinFunction::SPI_MISO);
            if (i == 17) p.supportedFunctions.push_back(PinFunction::SPI_CS);
            if (i == 18) p.supportedFunctions.push_back(PinFunction::SPI_SCK);
            if (i == 19) p.supportedFunctions.push_back(PinFunction::SPI_MOSI);
            if (i == 0) p.supportedFunctions.push_back(PinFunction::UART_TX);
            if (i == 1) p.supportedFunctions.push_back(PinFunction::UART_RX);
            pico.pins[name] = p;
        }
        boards_[pico.id] = pico;
    }

    // 3. Arduino Uno R3 (ATmega328P)
    {
        BoardSpec uno;
        uno.id = "arduino_uno";
        uno.name = "Arduino Uno R3 (ATmega328P)";
        uno.architecture = McuArchitecture::AVR_8BIT;
        uno.clockMHz = 16;
        uno.logicVoltage = LogicVoltage::VOLTAGE_5V; // 5V logic level!
        uno.flashKb = 32;
        uno.sramKb = 2;
        uno.deepSleepMicroAmps = 15000.0;
        uno.activeMilliAmps = 45.0;
        uno.defaultI2cSda = "A4";
        uno.defaultI2cScl = "A5";
        uno.defaultSpiMosi = "D11";
        uno.defaultSpiMiso = "D12";
        uno.defaultSpiSck = "D13";
        uno.defaultUartTx = "D1";
        uno.defaultUartRx = "D0";
        uno.defaultLedPin = "D13";
        uno.platformioBoardId = "uno";
        uno.arduinoFqbn = "arduino:avr:uno";
        uno.description = "Classic 8-bit 16MHz AVR board with 5.0V logic. Requires level shifters for 3.3V sensors.";

        for (int i = 0; i <= 13; ++i) {
            std::string name = "D" + std::to_string(i);
            bool pwm = (i == 3 || i == 5 || i == 6 || i == 9 || i == 10 || i == 11);
            PinSpec p = createGpio(name, i, LogicVoltage::VOLTAGE_5V, pwm);
            p.is5VTolerant = true;
            if (i == 0) p.supportedFunctions.push_back(PinFunction::UART_RX);
            if (i == 1) p.supportedFunctions.push_back(PinFunction::UART_TX);
            if (i == 10) p.supportedFunctions.push_back(PinFunction::SPI_CS);
            if (i == 11) p.supportedFunctions.push_back(PinFunction::SPI_MOSI);
            if (i == 12) p.supportedFunctions.push_back(PinFunction::SPI_MISO);
            if (i == 13) p.supportedFunctions.push_back(PinFunction::SPI_SCK);
            uno.pins[name] = p;
        }
        for (int i = 0; i <= 5; ++i) {
            std::string name = "A" + std::to_string(i);
            PinSpec p = createGpio(name, 14 + i, LogicVoltage::VOLTAGE_5V, false);
            p.supportedFunctions.push_back(PinFunction::ANALOG_IN);
            p.is5VTolerant = true;
            if (i == 4) p.supportedFunctions.push_back(PinFunction::I2C_SDA);
            if (i == 5) p.supportedFunctions.push_back(PinFunction::I2C_SCL);
            uno.pins[name] = p;
        }
        boards_[uno.id] = uno;
        boards_["uno"] = uno;
    }

    // 4. STM32F401 BlackPill (ARM Cortex-M4F)
    {
        BoardSpec stm32;
        stm32.id = "stm32f401";
        stm32.name = "WeAct STM32F401 BlackPill";
        stm32.architecture = McuArchitecture::ARM_CORTEX_M4F;
        stm32.clockMHz = 84;
        stm32.logicVoltage = LogicVoltage::VOLTAGE_3V3;
        stm32.flashKb = 512;
        stm32.sramKb = 96;
        stm32.deepSleepMicroAmps = 15.0;
        stm32.activeMilliAmps = 35.0;
        stm32.defaultI2cSda = "PB7";
        stm32.defaultI2cScl = "PB6";
        stm32.defaultSpiMosi = "PA7";
        stm32.defaultSpiMiso = "PA6";
        stm32.defaultSpiSck = "PA5";
        stm32.defaultUartTx = "PA9";
        stm32.defaultUartRx = "PA10";
        stm32.defaultLedPin = "PC13";
        stm32.platformioBoardId = "blackpill_f401cc";
        stm32.arduinoFqbn = "STMicroelectronics:stm32:GenF4";
        stm32.description = "High performance 84MHz ARM Cortex-M4 with hardware FPU and 5V-tolerant GPIOs.";

        // Key pins
        std::vector<std::string> pinNames = {
            "PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7", "PA8", "PA9", "PA10", "PA11", "PA12", "PA15",
            "PB0", "PB1", "PB3", "PB4", "PB5", "PB6", "PB7", "PB8", "PB9", "PB10", "PB12", "PB13", "PB14", "PB15",
            "PC13", "PC14", "PC15"
        };
        int phys = 1;
        for (const auto& name : pinNames) {
            PinSpec p = createGpio(name, phys++, LogicVoltage::VOLTAGE_3V3, true);
            p.is5VTolerant = true; // STM32 majority of GPIOs are 5V tolerant
            if (name == "PB7") p.supportedFunctions.push_back(PinFunction::I2C_SDA);
            if (name == "PB6") p.supportedFunctions.push_back(PinFunction::I2C_SCL);
            if (name == "PA7") p.supportedFunctions.push_back(PinFunction::SPI_MOSI);
            if (name == "PA6") p.supportedFunctions.push_back(PinFunction::SPI_MISO);
            if (name == "PA5") p.supportedFunctions.push_back(PinFunction::SPI_SCK);
            if (name == "PA9") p.supportedFunctions.push_back(PinFunction::UART_TX);
            if (name == "PA10") p.supportedFunctions.push_back(PinFunction::UART_RX);
            stm32.pins[name] = p;
        }
        boards_[stm32.id] = stm32;
    }

    // 5. Teensy 4.1 (ARM Cortex-M7 600MHz)
    {
        BoardSpec teensy;
        teensy.id = "teensy41";
        teensy.name = "Teensy 4.1 (Cortex-M7 600MHz)";
        teensy.architecture = McuArchitecture::ARM_CORTEX_M7;
        teensy.clockMHz = 600;
        teensy.logicVoltage = LogicVoltage::VOLTAGE_3V3;
        teensy.flashKb = 8192;
        teensy.sramKb = 1024;
        teensy.deepSleepMicroAmps = 100.0;
        teensy.activeMilliAmps = 100.0;
        teensy.defaultI2cSda = "PIN18";
        teensy.defaultI2cScl = "PIN19";
        teensy.defaultSpiMosi = "PIN11";
        teensy.defaultSpiMiso = "PIN12";
        teensy.defaultSpiSck = "PIN13";
        teensy.defaultUartTx = "PIN1";
        teensy.defaultUartRx = "PIN0";
        teensy.defaultLedPin = "PIN13";
        teensy.platformioBoardId = "teensy41";
        teensy.arduinoFqbn = "teensy:avr:teensy41";
        teensy.description = "Ultra-high speed 600MHz ARM Cortex-M7 with 1MB RAM and strict 3.3V GPIO (NOT 5V tolerant).";

        for (int i = 0; i <= 41; ++i) {
            std::string name = "PIN" + std::to_string(i);
            PinSpec p = createGpio(name, i, LogicVoltage::VOLTAGE_3V3, true);
            p.is5VTolerant = false;
            if (i == 18) p.supportedFunctions.push_back(PinFunction::I2C_SDA);
            if (i == 19) p.supportedFunctions.push_back(PinFunction::I2C_SCL);
            if (i == 11) p.supportedFunctions.push_back(PinFunction::SPI_MOSI);
            if (i == 12) p.supportedFunctions.push_back(PinFunction::SPI_MISO);
            if (i == 13) p.supportedFunctions.push_back(PinFunction::SPI_SCK);
            if (i == 1) p.supportedFunctions.push_back(PinFunction::UART_TX);
            if (i == 0) p.supportedFunctions.push_back(PinFunction::UART_RX);
            teensy.pins[name] = p;
        }
        boards_[teensy.id] = teensy;
    }
}

void HardwareBoardRegistry::initStandardComponents() {
    auto addComp = [&](const std::string& id, const std::string& name, const std::string& cat,
                       int i2cAddr, const std::vector<int>& altAddrs, LogicVoltage volt, double ma,
                       bool lvlShift, bool pullup, const std::vector<std::string>& pins, const std::string& desc) {
        ComponentSpec c;
        c.id = id;
        c.name = name;
        c.category = cat;
        c.defaultI2cAddress = i2cAddr;
        c.altI2cAddresses = altAddrs;
        c.operatingVoltage = volt;
        c.currentDrawMa = ma;
        c.requiresLevelShiftingWith5v = lvlShift;
        c.requiresPullup = pullup;
        c.requiredPins = pins;
        c.description = desc;
        components_[toLower(id)] = c;
    };

    // Sensors
    addComp("bme280", "BME280 Environmental Sensor", "Sensor", 0x76, {0x77}, LogicVoltage::VOLTAGE_3V3, 0.5, true, true,
            {"VCC", "GND", "SDA", "SCL"}, "High accuracy temperature, relative humidity, and barometric pressure sensor.");

    addComp("mpu6050", "MPU6050 6-DoF IMU Gyro/Accel", "Sensor", 0x68, {0x69}, LogicVoltage::VOLTAGE_3V3, 3.8, true, true,
            {"VCC", "GND", "SDA", "SCL", "INT"}, "6-axis MotionTracking device combining 3-axis gyroscope and 3-axis accelerometer.");

    addComp("hc_sr04", "HC-SR04 Ultrasonic Distance Sensor", "Sensor", 0, {}, LogicVoltage::VOLTAGE_5V, 15.0, true, false,
            {"VCC", "GND", "TRIG", "ECHO"}, "Ultrasonic ranging sensor 2cm-400cm. Echo pin outputs 5V logic: requires voltage divider on 3.3V MCUs!");

    addComp("dht22", "DHT22 / AM2302 Temp & Humidity Sensor", "Sensor", 0, {}, LogicVoltage::VOLTAGE_3V3, 1.5, false, true,
            {"VCC", "GND", "DATA"}, "Capacitive humidity sensor & thermistor with digital 1-wire protocol. Requires 4.7k pullup.");

    addComp("ds18b20", "DS18B20 1-Wire Digital Thermometer", "Sensor", 0, {}, LogicVoltage::VOLTAGE_3V3, 1.0, false, true,
            {"VDD", "GND", "DQ"}, "Dallas 1-Wire waterproof digital temperature sensor. Requires 4.7k pullup resistor between DQ and VDD.");

    addComp("ina219", "INA219 High-Side DC Current Sensor", "Sensor", 0x40, {0x41, 0x44, 0x45}, LogicVoltage::VOLTAGE_3V3, 1.0, false, true,
            {"VCC", "GND", "SDA", "SCL", "VIN+", "VIN-"}, "I2C bi-directional current/power monitor module up to 26V DC and 3.2A.");

    addComp("max30102", "MAX30102 Pulse Oximeter & Heart Rate", "Sensor", 0x57, {}, LogicVoltage::VOLTAGE_3V3, 1.2, true, true,
            {"VIN", "GND", "SDA", "SCL", "INT"}, "High-sensitivity pulse oximeter and optical heart-rate sensor.");

    // Displays
    addComp("ssd1306", "SSD1306 0.96 inch 128x64 OLED Display", "Display", 0x3C, {0x3D}, LogicVoltage::VOLTAGE_3V3, 20.0, true, true,
            {"VCC", "GND", "SDA", "SCL"}, "128x64 monochrome OLED display with internal charge pump.");

    addComp("lcd1602_i2c", "1602 LCD with PCF8574 I2C Backpack", "Display", 0x27, {0x3F}, LogicVoltage::VOLTAGE_5V, 25.0, true, true,
            {"VCC", "GND", "SDA", "SCL"}, "16x2 alphanumeric LCD display with 8-bit PCF8574 I2C I/O expander.");

    // Actuators
    addComp("sg90_servo", "TowerPro SG90 9g Micro Servo", "Actuator", 0, {}, LogicVoltage::VOLTAGE_5V, 250.0, false, false,
            {"VCC", "GND", "PWM"}, "Lightweight 9g servo motor with 50Hz PWM control (1.0ms - 2.0ms duty). Power from external 5V rail!");

    addComp("relay_module", "Optocoupled 5V Relay Module", "Actuator", 0, {}, LogicVoltage::VOLTAGE_5V, 70.0, false, false,
            {"VCC", "GND", "IN"}, "Galvanically isolated relay for controlling high voltage AC/DC appliances (Active LOW/HIGH).");

    addComp("ws2812b", "WS2812B NeoPixel Addressable RGB LED", "Actuator", 0, {}, LogicVoltage::VOLTAGE_5V, 60.0, true, false,
            {"5V", "GND", "DIN"}, "Individually addressable 5050 RGB LED with 800kHz data protocol. Requires 1000uF cap across power.");
}

} // namespace arenafight
