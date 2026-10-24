#include "arenafight/hardware/hardware_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

HardwareManager& HardwareManager::instance() {
    static HardwareManager inst;
    return inst;
}

HardwareValidationReport HardwareManager::validate(
    const std::string& boardId,
    const std::vector<PinAssignment>& assignments,
    const std::vector<std::string>& componentIds,
    bool isWifiEnabled
) {
    auto boardOpt = getBoards().getBoard(boardId);
    if (!boardOpt) {
        boardOpt = getBoards().findBoard(boardId);
    }

    if (!boardOpt) {
        HardwareValidationReport rep;
        rep.isValid = false;
        rep.criticalCount = 1;
        HardwareIssue is;
        is.severity = HardwareIssueSeverity::CRITICAL_ERROR;
        is.code = "BOARD_NOT_FOUND";
        is.targetPinOrComponent = boardId;
        is.message = "Target microcontroller board '" + boardId + "' was not found in board registry.";
        is.recommendation = "Use a supported board identifier like 'esp32', 'pico', 'arduino_uno', 'stm32f401', or 'teensy41'.";
        rep.issues.push_back(is);
        return rep;
    }

    std::vector<ComponentSpec> components;
    for (const auto& cId : componentIds) {
        auto cOpt = getBoards().getComponent(cId);
        if (!cOpt) cOpt = getBoards().findComponent(cId);
        if (cOpt) components.push_back(*cOpt);
    }

    return HardwareValidator::validateCircuit(*boardOpt, assignments, components, isWifiEnabled);
}

QoSTelemetry HardwareManager::evaluateProjectQoS(const std::string& projectDir) {
    std::string combinedSource;
    try {
        if (std::filesystem::is_directory(projectDir)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(projectDir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".cpp" || ext == ".c" || ext == ".ino" || ext == ".h" || ext == ".hpp") {
                        std::ifstream ifs(entry.path(), std::ios::binary);
                        std::stringstream ss;
                        ss << ifs.rdbuf();
                        combinedSource += "\n// --- " + entry.path().filename().string() + " ---\n" + ss.str();
                    }
                }
            }
        } else if (std::filesystem::is_regular_file(projectDir)) {
            std::ifstream ifs(projectDir, std::ios::binary);
            std::stringstream ss;
            ss << ifs.rdbuf();
            combinedSource = ss.str();
        }
    } catch (...) {}

    if (combinedSource.empty()) {
        QoSTelemetry qos;
        qos.overallScore = 0;
        qos.qosTier = "DEFECTIVE / NO_SOURCE_FOUND";
        qos.violations.push_back("No C/C++ or Arduino source files found in target directory: " + projectDir);
        return qos;
    }

    return HardwareValidator::validateFirmwareQoS(combinedSource);
}

ScaffoldResult HardwareManager::scaffold(const ScaffoldOptions& options) {
    auto boardOpt = getBoards().getBoard(options.boardId);
    if (!boardOpt) boardOpt = getBoards().findBoard(options.boardId);
    if (!boardOpt) {
        // Fallback default to esp32
        boardOpt = getBoards().getBoard("esp32");
    }

    std::vector<ComponentSpec> components;
    for (const auto& cId : options.components) {
        auto cOpt = getBoards().getComponent(cId);
        if (!cOpt) cOpt = getBoards().findComponent(cId);
        if (cOpt) components.push_back(*cOpt);
    }

    return HardwareScaffolder::scaffoldProject(options, *boardOpt, components);
}

CircuitCalculationResult HardwareManager::calculateLed(double vcc, double vf, double ma) {
    return CircuitCalculator::calculateLedResistor(vcc, vf, ma);
}

CircuitCalculationResult HardwareManager::calculateDivider(double vin, double vout, double rk) {
    return CircuitCalculator::calculateVoltageDivider(vin, vout, rk);
}

CircuitCalculationResult HardwareManager::calculateI2c(double vcc, int speedKhz, double cbPf) {
    return CircuitCalculator::calculateI2cPullup(vcc, speedKhz, cbPf);
}

CircuitCalculationResult HardwareManager::calculateBattery(double capMah, double actMa, double slpUa, double actSec, double slpSec) {
    return CircuitCalculator::calculateBatteryLife(capMah, actMa, slpUa, actSec, slpSec);
}

} // namespace arenafight
