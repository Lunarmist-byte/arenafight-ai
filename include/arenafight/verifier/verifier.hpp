#pragma once

#include <string>
#include <vector>
#include <memory>
#include "arenafight/common/types.hpp"
#include "arenafight/session/session.hpp"
#include "arenafight/session/memory.hpp"
#include "arenafight/session/context_manager.hpp"
#include "arenafight/router/model_router.hpp"

namespace arenafight {

class Verifier {
public:
    Verifier(
        ModelRouter& router,
        ContextManager& contextMgr,
        const std::string& workingDir = "."
    );

    // Independent verification backed by concrete evidence
    VerificationResult verify(
        const Session& session,
        const MemoryManager& memory
    );

    // Automated build check
    bool checkBuild(std::string& buildEvidence);

    // Automated tests check
    bool checkTests(std::string& testEvidence);

    // Check required files presence
    bool checkRequiredFiles(const Session& session, std::string& fileEvidence);

private:
    ModelRouter& router_;
    ContextManager& contextMgr_;
    std::string workingDir_;

    std::string collectObjectiveEvidence(const Session& session, const MemoryManager& memory);
};

} // namespace arenafight
