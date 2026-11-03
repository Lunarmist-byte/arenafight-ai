# ArenaFight Agent — Developer's Guide & Code Tutorial

A comprehensive architectural reference and hands-on coding tutorial for developers building, extending, or embedding **ArenaFight Agent** — a production-grade, local-first autonomous AI agent orchestration runtime in **C++20**.

---

## Made by Lunarmist-byte
* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)

---

## Table of Contents
1. [Core Philosophy & Execution Lifecycle](#1-core-philosophy--execution-lifecycle)
2. [Memory Safety & Reliability Model](#2-memory-safety--reliability-model)
3. [Zero "Works on My Machine" Build Matrix](#3-zero-works-on-my-machine-build-matrix)
4. [Tutorial 1: Embedding ArenaFight in a C++ Application](#4-tutorial-1-embedding-arenafight-in-a-c-application)
5. [Tutorial 2: Authoring Custom Tools](#5-tutorial-2-authoring-custom-tools)
6. [Tutorial 3: Writing a Custom Model Provider](#6-tutorial-3-writing-a-custom-model-provider)
7. [Tutorial 4: Implementing Domain-Specific Quality Verifiers](#7-tutorial-4-implementing-domain-specific-quality-verifiers)
8. [Tutorial 5: Session Management, Persistence & Resumption](#8-tutorial-5-session-management-persistence--resumption)
9. [Hardware Constraints & VRAM Protection (RTX 4060 8GB)](#9-hardware-constraints--vram-protection-rtx-4060-8gb)
10. [Audit Trails & Secret Sanitization](#10-audit-trails--secret-sanitization)

---

## 1. Core Philosophy & Execution Lifecycle

ArenaFight Agent is **not** a chatbot that broadcasts a single prompt to multiple LLMs. It is an **autonomous task execution runtime** modeled after modern agentic development environments (such as autonomous CLI agents).

### The Autonomous Execution State Machine

```text
               +----------------------------------+
               |            USER TASK             |
               +----------------------------------+
                                 |
                                 v
               +----------------------------------+
               |        TASK UNDERSTANDING        |
               +----------------------------------+
                                 |
                                 v
               +----------------------------------+
               |    PLANNER (Task Graph with      |
               |     Explicit Dependencies)       |
               +----------------------------------+
                                 |
                                 v
+------------> +----------------------------------+
|              |      TASK QUEUE DISPATCHER       | <---+
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |          MODEL ROUTER            |     |
|              | (Specialized Model per TaskType) |     |
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |         TOOL EXECUTOR            |     |
|              | (File, Shell, Git, Custom Tools) |     |
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |         OBSERVE & RECORD         |     |
|              | (stdout, stderr, exit code)      |     |
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |         UPDATE MEMORY            |     |
|              |  (Atomic Session Persistence)    |     |
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |      INDEPENDENT VERIFIER        |     |
|              |  (Build, Test & File Evidence)   |     |
|              +----------------------------------+     |
|                                |                      |
|        +-----------------------+------------------+   |
|        |                       |                  |   |
|        v                       v                  v   |
|  [ INCOMPLETE ]            [ FAILED ]        [ COMPLETE ]
|   Replan tasks          Debug, retry or           |
|        |               strategy fallback          v
+--------+                       |         +-------------------+
                                 +-------> |  FINAL EVIDENCE-  |
                                           |   BACKED REPORT   |
                                           +-------------------+
```

### The "Never Fake Completion" Doctrine
A common failure mode of LLM agents is hallucinating success: reporting *"Everything is completed!"* when code does not compile or tests fail.

ArenaFight enforces hard physical verification:
1. **Compilation Evidence:** `cmake --build build` or `cargo check` must return exit code `0`.
2. **Automated Test Evidence:** `ctest`, `cargo test`, or `pytest` must pass with zero failures.
3. **File Evidence:** Generated or edited files must physically exist with valid non-zero content.
4. **Independent Verifier:** The model that verifies completion is deliberately selected to be *different* from the model that wrote the code.

---

## 2. Memory Safety & Reliability Model

ArenaFight is engineered in standard **C++20** with strict memory and resource safety rules:

### 1. Zero Raw Owning Pointers
No raw pointers own memory in the codebase. All resource allocations are managed via `std::unique_ptr`, `std::shared_ptr`, or standard library containers (`std::string`, `std::vector`, `std::unordered_map`).

### 2. RAII Wrappers for C APIs
C libraries like `libcurl` and operating system handles (Windows `HANDLE`, POSIX file descriptors) are wrapped in RAII deleters to prevent resource leaks under exceptions or early returns:

```cpp
// RAII deleter for libcurl handles
struct CurlHandleDeleter {
    void operator()(CURL* c) const noexcept {
        if (c) curl_easy_cleanup(c);
    }
};
using UniqueCurl = std::unique_ptr<CURL, CurlHandleDeleter>;

// RAII deleter for curl header lists
struct CurlSlistDeleter {
    void operator()(curl_slist* s) const noexcept {
        if (s) curl_slist_free_all(s);
    }
};
using UniqueCurlSlist = std::unique_ptr<curl_slist, CurlSlistDeleter>;
```

### 3. Concurrency & Deadlock-Free Process Execution
Both Windows and Linux process executors read `stdout` and `stderr` pipes concurrently in separate worker threads:
* **The Problem:** A child process printing large amounts to `stderr` will exhaust the OS pipe buffer (often 64KB on Linux) and block if the parent process reads `stdout` sequentially first.
* **The Solution:** ArenaFight launches concurrent reader threads for both streams and joins them after the child process terminates, eliminating pipe deadlocks across all platforms.

### 4. Crash-Resilient Atomic Session Persistence
Sessions are persisted to disk using a two-phase atomic write pattern:
```text
Data -> session.json.tmp -> fsync / flush -> atomic rename -> session.json
```
If the process terminates unexpectedly (e.g., power loss, SIGKILL), existing session state is never left in a corrupted, half-written state.

---

## 3. Zero "Works on My Machine" Build Matrix

ArenaFight is tested and guaranteed across multiple compiler toolchains and operating systems.

### Dependency Strategy
* **`nlohmann/json`:** CMake first searches for locally installed system packages (`find_package(nlohmann_json QUIET)`). If not present on the host system, CMake automatically downloads and configures it via `FetchContent`.
* **`Threads::Threads`:** Explicitly linked to ensure POSIX threads (`-pthread`) are correctly attached on Linux/macOS, avoiding runtime crashes in `<mutex>` or `<thread>`.
* **`libcurl`:** Standard HTTP transport library available on all platforms.

### Build Instructions

#### A. Windows (MSYS2 UCRT64 / MinGW)
```bash
# 1. Install required packages
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-curl mingw-w64-ucrt-x86_64-nlohmann-json

# 2. Configure build with CMake & Ninja
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" -DOPENSSL_ROOT_DIR="C:/msys64/ucrt64"

# 3. Build core library, CLI, and test suite
cmake --build build

# 4. Run automated test suite
ctest --test-dir build --output-on-failure
```

#### B. Windows (Visual Studio 2022 / MSVC)
```powershell
# In "x64 Native Tools Command Prompt for VS 2022"
# (Install curl and nlohmann-json via vcpkg if needed: vcpkg install curl nlohmann-json:x64-windows)
cmake -B build -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

#### C. Linux (Ubuntu / Debian)
```bash
# 1. Install prerequisites
sudo apt update && sudo apt install -y build-essential cmake ninja-build libcurl4-openssl-dev nlohmann-json3-dev

# 2. Configure and build
cmake -B build -G "Ninja"
cmake --build build

# 3. Run test suite
ctest --test-dir build --output-on-failure
```

#### D. Linux (Fedora / RHEL)
```bash
sudo dnf install -y gcc-c++ cmake ninja-build libcurl-devel json-devel
cmake -B build -G "Ninja"
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 4. Tutorial 1: Embedding ArenaFight in a C++ Application

You can link `arenafight_core` directly into your own C++ applications to embed an autonomous agent runtime.

### Example: `embed_agent.cpp`

```cpp
#include "arenafight/agent/arena_agent.hpp"
#include "arenafight/common/config.hpp"
#include "arenafight/tools/file_tools.hpp"
#include "arenafight/tools/shell_tools.hpp"
#include <iostream>
#include <memory>

int main() {
    using namespace arenafight;

    // 1. Initialize configuration
    Config config;
    config.ollamaUrl = "http://localhost:11434";
    config.maxIterations = 20;
    config.contextLimit = 32000;
    config.requireConfirmationForDestructiveCommands = true;
    config.workspaceDir = "./workspace";

    // 2. Instantiate central agent runtime
    ArenaAgent agent(config);

    // 3. Register custom or built-in tools
    agent.getTools().registerTool(std::make_unique<ReadFileTool>());
    agent.getTools().registerTool(std::make_unique<WriteFileTool>());
    agent.getTools().registerTool(std::make_unique<EditFileTool>());
    agent.getTools().registerTool(std::make_unique<RunCommandTool>(config.requireConfirmationForDestructiveCommands));

    // 4. Define task objective
    std::string task = "Create a modern C++20 header-only matrix math library with unit tests.";

    std::cout << "Starting ArenaFight Agent execution...\n";
    bool success = agent.runTask(task);

    if (success) {
        std::cout << "\nTask completed with verified physical evidence!\n";
    } else {
        std::cerr << "\nTask halted or reached iteration limit without complete verification.\n";
    }

    return success ? 0 : 1;
}
```

### CMake Setup for Embedding

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyAgentApp CXX)

set(CMAKE_CXX_STANDARD 20)

# Add ArenaFight as subdirectory or link against precompiled library
add_subdirectory(path/to/arenafight arenafight_build)

add_executable(my_agent_app embed_agent.cpp)
target_link_libraries(my_agent_app PRIVATE arenafight_core)
```

---

## 5. Tutorial 2: Authoring Custom Tools

Every tool implements the `Tool` abstract base class:

```cpp
namespace arenafight {

class Tool {
public:
    virtual ~Tool() = default;

    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual nlohmann::json parameters() const = 0;
    virtual ToolResult execute(
        const nlohmann::json& arguments,
        const std::string& workingDir = "."
    ) = 0;
};

} // namespace arenafight
```

### Example: Building a `SqliteQueryTool`

Here is a complete, memory-safe custom tool that executes SQL statements against a SQLite database file:

```cpp
#include "arenafight/tools/tool.hpp"
#include <sqlite3.h> // Ensure sqlite3 is linked
#include <sstream>

namespace arenafight {

class SqliteQueryTool : public Tool {
public:
    std::string name() const override {
        return "sqlite_query";
    }

    std::string description() const override {
        return "Executes an SQL query against a local SQLite database and returns results.";
    }

    nlohmann::json parameters() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"db_path", {{"type", "string"}, {"description", "Path to SQLite database file"}}},
                {"sql", {{"type", "string"}, {"description", "SQL statement to execute"}}}
            }},
            {"required", {"db_path", "sql"}}
        };
    }

    ToolResult execute(const nlohmann::json& arguments, const std::string& workingDir) override {
        ToolResult result;
        std::string dbPath = arguments.value("db_path", "");
        std::string sql = arguments.value("sql", "");

        if (dbPath.empty() || sql.empty()) {
            result.success = false;
            result.error = "Both 'db_path' and 'sql' arguments are required.";
            return result;
        }

        // RAII wrapper for SQLite handle
        sqlite3* rawDb = nullptr;
        if (sqlite3_open(dbPath.c_str(), &rawDb) != SQLITE_OK) {
            result.success = false;
            result.error = "Failed to open SQLite database: " + std::string(sqlite3_errmsg(rawDb));
            if (rawDb) sqlite3_close(rawDb);
            return result;
        }

        std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(rawDb, sqlite3_close);

        char* zErrMsg = nullptr;
        std::stringstream outputStream;

        auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
            auto* stream = static_cast<std::stringstream*>(data);
            for (int i = 0; i < argc; ++i) {
                *stream << (colNames[i] ? colNames[i] : "NULL") << "="
                        << (argv[i] ? argv[i] : "NULL") << (i + 1 < argc ? ", " : "\n");
            }
            return 0;
        };

        int rc = sqlite3_exec(db.get(), sql.c_str(), callback, &outputStream, &zErrMsg);
        if (rc != SQLITE_OK) {
            result.success = false;
            result.error = "SQL execution error: " + (zErrMsg ? std::string(zErrMsg) : "Unknown error");
            if (zErrMsg) sqlite3_free(zErrMsg);
            return result;
        }

        result.success = true;
        result.output = outputStream.str();
        if (result.output.empty()) {
            result.output = "Query executed successfully with 0 rows returned.";
        }
        return result;
    }
};

} // namespace arenafight
```

### Registering the Custom Tool
```cpp
agent.getTools().registerTool(std::make_unique<SqliteQueryTool>());
```

---

## 6. Tutorial 3: Writing a Custom Model Provider

To add a new backend (such as a local `vLLM` server, `llama.cpp` server, or custom enterprise gateway), inherit from `ModelProvider`:

```cpp
#include "arenafight/providers/provider.hpp"
#include "arenafight/common/http_client.hpp"

namespace arenafight {

class VLLMProvider : public ModelProvider {
public:
    explicit VLLMProvider(std::string endpoint = "http://localhost:8000/v1")
        : endpoint_(std::move(endpoint)) {}

    std::string name() const override {
        return "vLLM";
    }

    bool available() override {
        HttpResponse resp = HttpClient::instance().get(endpoint_ + "/models", {}, 5);
        return resp.success;
    }

    std::vector<ModelInfo> discoverModels() override {
        std::vector<ModelInfo> models;
        HttpResponse resp = HttpClient::instance().get(endpoint_ + "/models", {}, 5);
        if (!resp.success) return models;

        try {
            nlohmann::json j = nlohmann::json::parse(resp.body);
            for (const auto& m : j["data"]) {
                ModelInfo info;
                info.id = m.value("id", "");
                info.name = info.id;
                info.provider = "vLLM";
                info.isLocal = true;
                info.contextWindow = 32768;
                info.capabilityScore = 2.4f;
                info.description = "vLLM high-throughput backend model";
                models.push_back(info);
            }
        } catch (...) {}
        return models;
    }

    AgentResponse generate(const AgentRequest& request) override {
        AgentResponse response;

        nlohmann::json payload;
        payload["model"] = request.model;
        payload["temperature"] = request.temperature;
        payload["max_tokens"] = request.maxTokens;

        nlohmann::json messages = nlohmann::json::array();
        if (!request.systemPrompt.empty()) {
            messages.push_back({{"role", "system"}, {"content", request.systemPrompt}});
        }
        for (const auto& msg : request.messages) {
            messages.push_back(msg.toJson());
        }
        payload["messages"] = messages;

        std::vector<std::string> headers = {"Content-Type: application/json"};
        HttpResponse httpResp = HttpClient::instance().post(
            endpoint_ + "/chat/completions",
            payload.dump(),
            headers,
            120
        );

        if (!httpResp.success) {
            response.success = false;
            response.errorMessage = "vLLM HTTP Error: " + httpResp.error;
            return response;
        }

        try {
            nlohmann::json respJson = nlohmann::json::parse(httpResp.body);
            response.success = true;
            response.content = respJson["choices"][0]["message"].value("content", "");
            // Extract tool calls using built-in parser fallback
            response.toolCalls = extractToolCallsFromText(response.content);
        } catch (const std::exception& e) {
            response.success = false;
            response.errorMessage = "Failed to parse vLLM response: " + std::string(e.what());
        }

        return response;
    }

private:
    std::string endpoint_;
};

} // namespace arenafight
```

### Registering the Provider
```cpp
agent.getRouter().registerProvider(std::make_shared<VLLMProvider>("http://localhost:8000/v1"));
```

---

## 7. Tutorial 4: Implementing Domain-Specific Quality Verifiers

The `Verifier` determines whether a task is truly completed. You can enhance it with custom checks (e.g., security audits, code format checks, memory leak detection).

### Custom Memory Safety Verifier (AddressSanitizer / Valgrind Check)

```cpp
bool checkMemoryLeaks(const std::string& workingDir, std::string& leakEvidence) {
    // Run compiled binary with ASan or Valgrind
    ProcessResult pr = ProcessExecutor::executeShell("ctest -R test_memory --output-on-failure", workingDir, 60000);
    if (pr.exitCode == 0 && pr.stdErr.find("ERROR: AddressSanitizer") == std::string::npos) {
        leakEvidence = "MEMORY AUDIT: PASS (No AddressSanitizer violations detected)";
        return true;
    } else {
        leakEvidence = "MEMORY AUDIT: FAIL (Memory leak or illegal memory access detected):\n" + pr.stdErr;
        return false;
    }
}
```

---

## 8. Tutorial 5: Session Management, Persistence & Resumption

Every execution generates a session directory:

```text
sessions/
    <session-id>/
        session.json    # Task graph, completed/failed tasks, execution state
        memory.json     # Categorized discoveries, decisions, file modifications
logs/
    session-<id>.jsonl  # Redacted chronological audit log
```

### Resuming an Interrupted Session

```cpp
// 1. Load persisted session from disk
Session session = Session::loadFromDisk("a8c2f91e", "./sessions");

// 2. Resume execution seamlessly
std::cout << "Resuming session " << session.id << " at iteration " << session.iteration << "...\n";
agent.resumeSession("a8c2f91e");
```

Via CLI:
```bash
# List all previous sessions
arena sessions

# Resume execution
arena resume a8c2f91e
```

---

## 9. Hardware Constraints & VRAM Protection (RTX 4060 8GB)

Local LLM execution requires careful memory management, particularly on consumer GPUs such as an **NVIDIA GeForce RTX 4060 Laptop GPU (8GB VRAM)**.

### Architectural Safeguards
1. **Sequential Local Model Execution:**
   `OllamaProvider` enforces a process-wide mutex (`getLocalGpuMutex()`) around all model generation calls. Multiple concurrent agent tasks never attempt to load two local LLMs into VRAM simultaneously.
2. **Offloading to Cloud / Non-VRAM Backends:**
   * Planning or Verification can be routed to OpenRouter (`anthropic/claude-3.5-sonnet` or `deepseek/deepseek-r1`), consuming **zero** local GPU VRAM.
   * Claude Code CLI runs as a subprocess, avoiding local weight loading.
3. **Context Compaction:**
   When conversation history exceeds `context_limit` (e.g. 32,000 tokens), `ContextManager` summarizes older steps and retains only high-priority decisions, discoveries, and known errors.

---

## 10. Audit Trails & Secret Sanitization

ArenaFight automatically redacts sensitive tokens (OpenRouter API keys, bearer tokens, passwords) before writing to console output or persistent JSONL files:

```cpp
// Automatic secret sanitization pattern in Logger
std::string clean = Logger::sanitize(rawOutput);
```

Sanitization rules:
* Redacts `sk-or-v1-...` and similar API key signatures to `sk-or-v1-[REDACTED]`.
* Replaces `Authorization: Bearer <token>` headers with `Authorization: Bearer [REDACTED]`.

---

## Credits
**Made by Lunarmist-byte**
* GitHub: [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* LinkedIn: [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)
