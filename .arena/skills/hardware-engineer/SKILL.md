---
name: hardware-engineer
description: Microcontroller pinouts, circuit design, Wokwi diagram simulation, and extreme QoS embedded firmware.
---

# Hardware Engineer & Firmware QoS Skill

## Overview
This skill operationalizes embedded systems engineering, microcontroller circuit validation, and extreme Quality of Service (QoS) firmware analysis for microcontrollers (ESP32, Raspberry Pi Pico, Arduino Uno, STM32 BluePill, Teensy 4.1).

## Hardware Constraints and Pin Safety
1. **Logic Level Translation**:
   - 3.3V Logic (ESP32, RP2040, STM32) is NOT 5V tolerant unless explicitly stated.
   - Connecting 5V sensors (e.g. HC-SR04 echo) directly to 3.3V GPIO destroys the MCU pin.
   - Always calculate voltage dividers (e.g. 1k / 2k resistor divider) or use bidirectional logic level shifters (BSS138).
2. **Strapping Pins & Flash Memory Pins**:
   - ESP32: GPIO 0, 2, 12, 15 control boot mode. GPIO 6-11 are connected to integrated SPI flash and must never be used.
   - Input-only Pins: GPIO 34, 35, 36, 39 on ESP32 cannot drive outputs (no pull-up/pull-down circuitry).
3. **Bus Protocols & Collisions**:
   - I2C: Requires pull-up resistors (typically 4.7k ohm to 3.3V). Check for address collisions across multi-sensor setups.

## Extreme QoS Firmware Invariants
When authoring or inspecting real-time embedded C++ code:
1. **Zero-Heap ISR Rule**: Interrupt Service Routines (ISRs) must NEVER invoke dynamic memory allocation (`malloc`, `new`, `std::vector::push_back`) or blocking operations (`delay()`, `printf()`, `std::mutex::lock()`).
2. **Non-Blocking Architecture**: Replace all `delay()` calls with non-blocking timer loops using `millis()` or freeRTOS timers.
3. **Hardware Watchdog Timers (WDT)**: Always configure and service the hardware watchdog to recover automatically from unexpected deadlocks or bus stalls.
4. **Wokwi Circuit Simulation**: Generate clean `diagram.json` containing board configurations, part definitions, and pin connections for instant web simulation.
