#include "arenafight/hardware/circuit_calculator.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

namespace {

// Standard E24 decade series base values
const double E24_SERIES[] = {
    1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
    3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1
};

std::string formatResistor(double ohms) {
    std::stringstream ss;
    if (ohms >= 1e6) {
        ss << std::fixed << std::setprecision(1) << (ohms / 1e6) << " MΩ";
    } else if (ohms >= 1e3) {
        ss << std::fixed << std::setprecision(1) << (ohms / 1e3) << " kΩ";
    } else {
        ss << std::fixed << std::setprecision(0) << ohms << " Ω";
    }
    return ss.str();
}

} // namespace

double CircuitCalculator::findNearestStandardResistor(double exactOhms) {
    if (exactOhms <= 0) return 0.0;

    double decade = std::pow(10.0, std::floor(std::log10(exactOhms)));
    double normalized = exactOhms / decade;

    double closest = E24_SERIES[0];
    double minDiff = std::abs(normalized - closest);

    for (double val : E24_SERIES) {
        double diff = std::abs(normalized - val);
        if (diff < minDiff) {
            minDiff = diff;
            closest = val;
        }
    }

    return closest * decade;
}

CircuitCalculationResult CircuitCalculator::calculateLedResistor(
    double supplyVoltage,
    double ledForwardVoltage,
    double ledCurrentMa
) {
    CircuitCalculationResult res;
    res.calculationType = "LED_RESISTOR";
    res.formulaUsed = "R = (Vsupply - Vf) / If  ;  P = If^2 * R";

    res.inputParams = {
        {"supplyVoltage_V", supplyVoltage},
        {"ledForwardVoltage_V", ledForwardVoltage},
        {"ledCurrent_mA", ledCurrentMa}
    };

    if (supplyVoltage <= ledForwardVoltage) {
        res.summary = "ERROR: Supply voltage (" + std::to_string(supplyVoltage) + "V) must be greater than LED forward voltage (" + std::to_string(ledForwardVoltage) + "V).";
        res.resultValues = {{"error", "Supply voltage too low"}};
        return res;
    }

    double currentAmps = ledCurrentMa / 1000.0;
    double exactOhms = (supplyVoltage - ledForwardVoltage) / currentAmps;
    double standardOhms = findNearestStandardResistor(exactOhms);
    double actualCurrentMa = ((supplyVoltage - ledForwardVoltage) / standardOhms) * 1000.0;
    double powerWatts = std::pow(actualCurrentMa / 1000.0, 2) * standardOhms;

    std::string powerRating = (powerWatts * 2.0 < 0.25) ? "1/4W (0.25W)" : ((powerWatts * 2.0 < 0.5) ? "1/2W (0.5W)" : "1W");

    res.resultValues = {
        {"exactResistance_Ohms", exactOhms},
        {"standardResistance_Ohms", standardOhms},
        {"standardResistance_Formatted", formatResistor(standardOhms)},
        {"actualCurrent_mA", actualCurrentMa},
        {"powerDissipation_mW", powerWatts * 1000.0},
        {"recommendedPowerRating", powerRating}
    };

    res.componentRecommendations.push_back("Recommended Resistor: " + formatResistor(standardOhms) + " (" + powerRating + " rating)");
    res.componentRecommendations.push_back("Expected Forward Current: " + std::to_string(actualCurrentMa).substr(0, 4) + " mA");

    std::stringstream ss;
    ss << "For Vsupply=" << supplyVoltage << "V and Vf=" << ledForwardVoltage << "V @ " << ledCurrentMa << "mA:\n"
       << "Exact Resistor: " << formatResistor(exactOhms) << " -> Standard E24: " << formatResistor(standardOhms) << "\n"
       << "Power dissipation: " << (powerWatts * 1000.0) << " mW (" << powerRating << " recommended)";
    res.summary = ss.str();

    return res;
}

CircuitCalculationResult CircuitCalculator::calculateVoltageDivider(
    double inputVoltage,
    double targetOutputVoltage,
    double targetResistanceKOhms
) {
    CircuitCalculationResult res;
    res.calculationType = "VOLTAGE_DIVIDER";
    res.formulaUsed = "Vout = Vin * (R2 / (R1 + R2))";

    res.inputParams = {
        {"inputVoltage_V", inputVoltage},
        {"targetOutputVoltage_V", targetOutputVoltage},
        {"targetResistance_kOhms", targetResistanceKOhms}
    };

    if (inputVoltage <= targetOutputVoltage) {
        res.summary = "ERROR: Input voltage must be strictly greater than target output voltage for passive divider.";
        res.resultValues = {{"error", "Input must exceed target output"}};
        return res;
    }

    double ratio = targetOutputVoltage / inputVoltage;
    // R2 / (R1 + R2) = ratio => R1 = R2 * (1 - ratio) / ratio
    double r2Target = (targetResistanceKOhms * 1000.0) * ratio;
    double r1Target = (targetResistanceKOhms * 1000.0) * (1.0 - ratio);

    double r2Standard = findNearestStandardResistor(r2Target);
    double r1Standard = findNearestStandardResistor(r1Target);

    double actualVout = inputVoltage * (r2Standard / (r1Standard + r2Standard));
    double quiescentCurrentMa = (inputVoltage / (r1Standard + r2Standard)) * 1000.0;
    double errorPercent = std::abs((actualVout - targetOutputVoltage) / targetOutputVoltage) * 100.0;

    res.resultValues = {
        {"R1_top_Ohms", r1Standard},
        {"R1_Formatted", formatResistor(r1Standard)},
        {"R2_bottom_Ohms", r2Standard},
        {"R2_Formatted", formatResistor(r2Standard)},
        {"actualOutputVoltage_V", actualVout},
        {"errorPercent", errorPercent},
        {"quiescentCurrent_mA", quiescentCurrentMa}
    };

    res.componentRecommendations.push_back("R1 (top, connected to " + std::to_string(inputVoltage).substr(0, 3) + "V signal): " + formatResistor(r1Standard));
    res.componentRecommendations.push_back("R2 (bottom, connected to GND): " + formatResistor(r2Standard));
    res.componentRecommendations.push_back("Output tapped between R1 and R2 produces " + std::to_string(actualVout).substr(0, 4) + " V");

    std::stringstream ss;
    ss << "Level shifter divider for " << inputVoltage << "V -> " << targetOutputVoltage << "V:\n"
       << "R1=" << formatResistor(r1Standard) << ", R2=" << formatResistor(r2Standard) << " yields Vout = "
       << actualVout << " V (error: " << std::fixed << std::setprecision(1) << errorPercent << "%, quiescent: "
       << quiescentCurrentMa << " mA)";
    res.summary = ss.str();

    return res;
}

CircuitCalculationResult CircuitCalculator::calculateI2cPullup(
    double supplyVoltage,
    int busSpeedKhz,
    double busCapacitancePf
) {
    CircuitCalculationResult res;
    res.calculationType = "I2C_PULLUP";
    res.formulaUsed = "Rmin = (Vdd - Vol) / Iol  ;  Rmax = tr / (0.8473 * Cb)";

    res.inputParams = {
        {"supplyVoltage_V", supplyVoltage},
        {"busSpeed_kHz", busSpeedKhz},
        {"busCapacitance_pF", busCapacitancePf}
    };

    // Standard I2C Specs:
    // Vol = 0.4V, Iol = 3mA
    // tr_max: 1000ns for 100kHz Standard Mode, 300ns for 400kHz Fast Mode
    double vOl = 0.4;
    double iOl = 0.003;
    double trMaxNs = (busSpeedKhz <= 100) ? 1000.0 : 300.0;

    double rMin = (supplyVoltage - vOl) / iOl;
    double rMax = (trMaxNs * 1e-9) / (0.8473 * (busCapacitancePf * 1e-12));

    double optimalOhms = std::sqrt(rMin * rMax);
    double standardOhms = findNearestStandardResistor(optimalOhms);

    if (standardOhms < rMin) standardOhms = findNearestStandardResistor(rMin * 1.2);
    if (standardOhms > rMax) standardOhms = findNearestStandardResistor(rMax * 0.8);

    res.resultValues = {
        {"Rmin_Ohms", rMin},
        {"Rmax_Ohms", rMax},
        {"recommended_Ohms", standardOhms},
        {"recommended_Formatted", formatResistor(standardOhms)}
    };

    res.componentRecommendations.push_back("Recommended I2C Pull-Up: " + formatResistor(standardOhms) + " for both SDA and SCL rails.");
    res.componentRecommendations.push_back("Acceptable Pull-up Range: " + formatResistor(rMin) + " to " + formatResistor(rMax));

    std::stringstream ss;
    ss << "I2C Pull-Up (" << busSpeedKhz << " kHz, Cb=" << busCapacitancePf << "pF, Vdd=" << supplyVoltage << "V):\n"
       << "Valid range: [" << formatResistor(rMin) << " - " << formatResistor(rMax) << "]. Recommended: " << formatResistor(standardOhms);
    res.summary = ss.str();

    return res;
}

CircuitCalculationResult CircuitCalculator::calculateBatteryLife(
    double batteryCapacityMah,
    double activeCurrentMa,
    double sleepCurrentMicroAmps,
    double activeTimeSeconds,
    double sleepTimeSeconds
) {
    CircuitCalculationResult res;
    res.calculationType = "BATTERY_LIFE";
    res.formulaUsed = "Iavg = (Iactive * Tact + Isleep * Tsleep) / (Tact + Tsleep) ; Time = (Capacity * 0.85) / Iavg";

    res.inputParams = {
        {"batteryCapacity_mAh", batteryCapacityMah},
        {"activeCurrent_mA", activeCurrentMa},
        {"sleepCurrent_uA", sleepCurrentMicroAmps},
        {"activeTime_seconds", activeTimeSeconds},
        {"sleepTime_seconds", sleepTimeSeconds}
    };

    double totalCycleSec = activeTimeSeconds + sleepTimeSeconds;
    if (totalCycleSec <= 0) totalCycleSec = 1.0;

    double sleepCurrentMa = sleepCurrentMicroAmps / 1000.0;
    double avgCurrentMa = ((activeCurrentMa * activeTimeSeconds) + (sleepCurrentMa * sleepTimeSeconds)) / totalCycleSec;

    // Usable capacity considering 15% safety factor / self discharge
    double usableCapacityMah = batteryCapacityMah * 0.85;
    double lifeHours = (avgCurrentMa > 0.0) ? (usableCapacityMah / avgCurrentMa) : 0.0;
    double lifeDays = lifeHours / 24.0;
    double lifeMonths = lifeDays / 30.4375;

    res.resultValues = {
        {"averageCurrent_mA", avgCurrentMa},
        {"dutyCyclePercent", (activeTimeSeconds / totalCycleSec) * 100.0},
        {"lifeHours", lifeHours},
        {"lifeDays", lifeDays},
        {"lifeMonths", lifeMonths}
    };

    res.componentRecommendations.push_back("Estimated Operating Time: " + std::to_string(static_cast<int>(lifeDays)) + " days (" + std::to_string(lifeMonths).substr(0, 4) + " months)");
    res.componentRecommendations.push_back("Average Current Draw: " + std::to_string(avgCurrentMa).substr(0, 5) + " mA");

    std::stringstream ss;
    ss << "Battery Lifetime (Capacity " << batteryCapacityMah << " mAh):\n"
       << "Average Current: " << avgCurrentMa << " mA (Duty cycle: " << std::fixed << std::setprecision(2)
       << ((activeTimeSeconds / totalCycleSec) * 100.0) << "%)\n"
       << "Estimated Runtime: " << static_cast<int>(lifeHours) << " hours / "
       << static_cast<int>(lifeDays) << " days (" << std::fixed << std::setprecision(1) << lifeMonths << " months)";
    res.summary = ss.str();

    return res;
}

} // namespace arenafight
