#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"

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
};

} // namespace arenafight
