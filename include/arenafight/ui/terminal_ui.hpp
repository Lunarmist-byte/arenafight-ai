#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/hardware/hardware_types.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

class TerminalUI {
public:
    static void printBanner();
    static void printFooter();

    static void printSessionHeader(
        const std::string& task,
        const std::string& sessionId
    );

    static void printPlanStatus(const Session& session);

    static void printActiveExecution(
        const std::string& model,
        const std::string& tool,
        const std::string& detail
    );

    static void printIterationFooter(
        int iteration,
        int tokens,
        int toolCalls,
        int64_t elapsedSec
    );

    static void printErrorBox(
        const std::string& command,
        int exitCode,
        const std::string& errorDetail
    );

    static void printRecoveryNotice(const std::string& recoveryPlan);

    static void printFinalReport(
        bool completed,
        const std::string& objective,
        const VerificationResult& verif,
        const std::vector<std::string>& filesCreated,
        int64_t totalElapsedSec
    );

    static void printSessionsList(const std::vector<SessionInfo>& sessions);
    static void printModelsList(const std::vector<ModelInfo>& models);
    static void printConfigView(const nlohmann::json& configJson);

    // Hardware Studio UI
    static void printHardwareStudioBanner();
    static void printBoardDetails(const BoardSpec& board);
    static void printBoardsTable(const std::vector<BoardSpec>& boards);
    static void printComponentsTable(const std::vector<ComponentSpec>& components);
    static void printHardwareValidation(const HardwareValidationReport& report);
    static void printQoSTelemetryReport(const QoSTelemetry& qos);
};

} // namespace arenafight
