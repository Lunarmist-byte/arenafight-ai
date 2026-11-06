#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include "arenafight/agent/arena_agent.hpp"

// Made by Lunarmist-byte
// GitHub: https://github.com/Lunarmist-byte
// LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

namespace arenafight {

struct WiFiServerConfig {
    int port = 8080;
    std::string bindAddress = "0.0.0.0"; // Bind to all network interfaces for Wi-Fi access
    std::string documentRoot = "";       // Optional static file directory
    bool enableCors = true;
    int maxThreads = 8;
};

struct NetworkEndpointInfo {
    std::string localUrl;         // e.g. http://localhost:8080
    std::string wifiUrl;          // e.g. http://192.168.1.105:8080
    std::string ipAddress;        // e.g. 192.168.1.105
    int port = 8080;
    bool isBoundToAllInterfaces = true;
};

class WiFiHostServer {
public:
    explicit WiFiHostServer(ArenaAgent& agent, WiFiServerConfig config = {});
    ~WiFiHostServer();

    // Non-copyable
    WiFiHostServer(const WiFiHostServer&) = delete;
    WiFiHostServer& operator=(const WiFiHostServer&) = delete;

    // Start server in background worker thread
    bool start();

    // Stop server and join background thread
    void stop();

    // Check if server is currently running
    bool isRunning() const;

    // Run blocking event loop (used by CLI 'arena host')
    void runBlocking();

    // Process a raw HTTP request string and return the raw HTTP response string.
    // Enables direct in-memory invocation and headless integration testing.
    std::string processRequest(const std::string& rawRequest);

    // Network IP and endpoint discovery
    static std::string detectLocalWifiIp();
    static std::vector<std::string> getAllLocalIps();
    NetworkEndpointInfo getEndpointInfo() const;

    // Print high-visibility Wi-Fi connection banner to console
    void printHostBanner() const;

    // Configuration accessors
    const WiFiServerConfig& getConfig() const { return config_; }
    void setPort(int port) { config_.port = port; }
    void setBindAddress(const std::string& addr) { config_.bindAddress = addr; }

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    ArenaAgent& agent_;
    WiFiServerConfig config_;
    std::atomic<bool> running_{false};
};

} // namespace arenafight

