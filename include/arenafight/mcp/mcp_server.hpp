#pragma once

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include "arenafight/agent/arena_agent.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

/**
 * Model Context Protocol (MCP) Server for ArenaFight.
 * Enables OpenCode (and other MCP-compatible clients) to directly invoke
 * ArenaFight autonomous execution, 6 physical evidence verification gates,
 * host hardware profiling, QoS auditing, and benchmarking tools.
 */
class MCPServer {
public:
    explicit MCPServer(ArenaAgent& agent);

    // Run the standard I/O JSON-RPC loop (stdio MCP transport)
    void run(std::istream& in = std::cin, std::ostream& out = std::cout);

    // Parse and handle a single incoming JSON-RPC payload
    nlohmann::json handleMessage(const std::string& rawMessage);

    // Handle parsed JSON-RPC request
    nlohmann::json handleRequest(const nlohmann::json& request);

    // Query available tool schemas (MCP tools/list format)
    nlohmann::json getToolsSchema() const;

private:
    ArenaAgent& agent_;
    bool initialized_ = false;

    nlohmann::json handleInitialize(const nlohmann::json& id, const nlohmann::json& params);
    nlohmann::json handleToolsList(const nlohmann::json& id);
    nlohmann::json handleToolsCall(const nlohmann::json& id, const nlohmann::json& params);
    nlohmann::json handlePing(const nlohmann::json& id);

    // Tool execution delegates
    nlohmann::json executeVerify(const nlohmann::json& args);
    nlohmann::json executeGoal(const nlohmann::json& args);
    nlohmann::json executePlan(const nlohmann::json& args);
    nlohmann::json executeProfile(const nlohmann::json& args);
    nlohmann::json executeBenchmark(const nlohmann::json& args);
    nlohmann::json executeQoS(const nlohmann::json& args);
    nlohmann::json executeCalculate(const nlohmann::json& args);
    nlohmann::json executeSecurity(const nlohmann::json& args);
    nlohmann::json executeThink(const nlohmann::json& args);
};

} // namespace arenafight

