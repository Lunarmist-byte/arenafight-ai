# ArenaFight Agent

An autonomous, local-first multi-model AI agent orchestration runtime and execution system built in **C++20**.

ArenaFight Agent is designed for autonomous coding, research, and debugging workflows (akin to **Antigravity** and **Claude Code** execution). It moves beyond standard chatbot question-and-answer patterns by operating an autonomous, self-evaluating execution loop: understanding tasks, generating structured task graphs with dependencies, invoking tools, updating persistent memory, independently verifying physical evidence, debugging failures, and replanning until the objective is genuinely achieved.

---

## Made by Lunarmist-byte
* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)

---

## 1. Core Architecture & Philosophy

The system executes an autonomous cycle:

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
|              | (File, Shell, Git, Search, User) |     |
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |         OBSERVE & RECORD         |     |
|              |   (stdout, stderr, exit code)    |     |
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
ArenaFight never outputs *"Task complete"* based on conversational agreement. Completion requires **physical, reproducible evidence**:
* **Build Verification:** `cmake --build build` or language equivalent exits with code `0`.
* **Test Suite Verification:** Automated tests (`ctest`, `cargo test`, `pytest`) pass with zero errors.
* **File Presence:** Required source and documentation artifacts are confirmed present on the filesystem.
* **Independent Verifier:** The model assessing verification is selected to be independent from the model that wrote the code.

---

## 2. Supported Model Backends

ArenaFight provides a unified, extensible `ModelProvider` C++ interface:

### 1. Ollama (`OllamaProvider`)
* **Local First:** Discovers locally installed models on the fly via `GET http://localhost:11434/api/tags`.
* **Zero Hardcoding:** Parses model parameter size (7B, 14B, 32B, 70B), quantization level, and family directly from local metadata.
* **Hardware Awareness (RTX 4060 8GB VRAM):** Enforces a process-wide mutex for local model execution to prevent multiple concurrent models from triggering GPU out-of-memory errors.

### 2. OpenRouter (`OpenRouterProvider`)
* Cloud multi-model routing using the `OPENROUTER_API_KEY` environment variable or `config.json`.
* Discovers catalog via OpenRouter's API or configurable lists via `OPENROUTER_MODELS`.
* Redacts secret API keys from console displays and persistent logs.

### 3. Claude Code (`ClaudeCodeProvider`)
* Non-interactive execution using the installed CLI: `claude -p`.
* Leverages existing CLI authentication without requiring duplicate API keys.
* Uses temporary prompt pipes with RAII file cleanup guards to eliminate shell quotation issues.

### 4. Deterministic Test Engine (`TestMockProvider`)
* Built-in deterministic model simulator for self-testing, continuous integration, and offline verification (`arena test` or `--mock`).

---

## 3. Dynamic Model Router

Instead of selecting one model for an entire session, the `ModelRouter` dispatches tasks based on specialized capability:

| Task Type | Routing Strategy | Example Preferred Models |
| :--- | :--- | :--- |
| **Planning** | Strong reasoning capability | `claude-3.5-sonnet`, `deepseek-r1`, `qwen2.5:72b` |
| **Coding** | Specialized code generation | `Claude Code CLI`, `claude-3.7-sonnet`, `deepseek-coder` |
| **Debugging** | Reasoning + error diagnosis | `deepseek-r1`, `claude-3.7-sonnet`, `qwen2.5-coder` |
| **Review** | Cross-provider inspection | Independent reviewer model |
| **Verification** | Independent from Coder | Prefers a different backend than the one that authored the code |

---

## 4. Built-in Tool Registry

Tools return structured `ToolResult` objects containing stdout, stderr, exit code, and execution duration:

* `read_file`: Line-numbered file inspection with optional `start_line` and `end_line`.
* `write_file`: File creation with automatic parent directory generation.
* `edit_file`: Targeted search-and-replace with patch validation and automatic `.bak` backup creation.
* `create_directory`: Recursive directory creation (`mkdir -p`).
* `delete_file`: File deletion with directory protection.
* `list_directory`: Directory inspection with recursive depth control and item filtering.
* `search_files`: Workspace grep and regular expression search across files.
* `run_command`: Safe shell execution (PowerShell on Windows, bash on Linux) with destructive command interception.
* `run_program`: Direct executable execution with argument lists.
* `run_tests`: Automated test runner discovery (ctest, cargo test, npm test, pytest).
* `git_status`: Working tree status (`git status -s -b`).
* `git_diff`: File and directory diff inspection.
* `git_log`: Git history inspection.
* `ask_user`: Human-in-the-loop interactive prompting when blocked or confirming destructive actions.

---

## 5. Memory Safety & Concurrency Architecture

* **Zero Raw Owning Pointers:** All allocations use `std::unique_ptr`, `std::shared_ptr`, and standard containers.
* **RAII for C Handles:** `CURL*`, `curl_slist*`, and escaped strings are wrapped in RAII custom deleters.
* **Deadlock-Free Process Execution:** Windows and Linux process executors consume `stdout` and `stderr` pipes concurrently in separate threads, eliminating pipe buffer deadlocks.
* **Atomic Session Persistence:** Writes to `session.json.tmp` and performs an atomic filesystem rename to prevent corrupt session files on unexpected shutdowns.
* **Secret Redaction:** `Logger::sanitize()` masks API keys (`sk-or-v1-...`) and Authorization headers.

---

## 6. Build Instructions

### Prerequisites
* **C++ Compiler:** C++20 compliant compiler (GCC 12+, Clang 14+, or MSVC 2022+)
* **Build System:** CMake (3.20+) and Ninja (or Make)
* **Libraries:** `libcurl`, `nlohmann/json` (automatically fetched via CMake `FetchContent` if not installed locally)

### Windows (MSYS2 UCRT64 / MinGW)
```bash
# 1. Install prerequisites in MSYS2 UCRT64
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-curl mingw-w64-ucrt-x86_64-nlohmann-json

# 2. Configure build with CMake & Ninja
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64" -DOPENSSL_ROOT_DIR="C:/msys64/ucrt64"

# 3. Build executable and test suite
cmake --build build

# 4. Run automated test suite
ctest --test-dir build --output-on-failure
```

### Windows (Visual Studio 2022 / MSVC)
```powershell
cmake -B build -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Linux (Ubuntu / Debian)
```bash
sudo apt update && sudo apt install -y build-essential cmake ninja-build libcurl4-openssl-dev nlohmann-json3-dev
cmake -B build -G "Ninja"
cmake --build build
ctest --test-dir build --output-on-failure
```

### Linux (Fedora)
```bash
sudo dnf install -y gcc-c++ cmake ninja-build libcurl-devel json-devel
cmake -B build -G "Ninja"
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 7. CLI Usage

```bash
# Launch interactive mode
./build/arena

# Execute a single autonomous task
./build/arena "Build a C++ CLI todo application with persistence, tests, and documentation"

# Run with test mock provider (offline testing)
./build/arena "Create a calculator module" --mock

# Resume a previous session
./build/arena resume <session-id>

# List all past sessions
./build/arena sessions

# List all available models across providers
./build/arena models

# Inspect current configuration
./build/arena config

# Run built-in self-test suite
./build/arena test
```

---

## 8. Configuration (`config.json`)

```json
{
    "ollama_url": "http://localhost:11434",
    "openrouter_api_key": "",
    "openrouter_base_url": "https://openrouter.ai/api/v1",
    "openrouter_models": [
        "anthropic/claude-3.7-sonnet",
        "anthropic/claude-3.5-sonnet",
        "deepseek/deepseek-r1",
        "deepseek/deepseek-chat",
        "google/gemini-2.0-flash-001",
        "meta-llama/llama-3.3-70b-instruct",
        "qwen/qwen-2.5-coder-32b-instruct"
    ],
    "claude_command": "claude",
    "max_iterations": 30,
    "max_tool_calls": 100,
    "max_retries": 3,
    "context_limit": 32000,
    "parallel_models": false,
    "require_confirmation_for_destructive_commands": true,
    "default_provider": "auto",
    "workspace_dir": ".",
    "test_mock_enabled": false
}
```

Environment variables override configuration automatically:
* `OPENROUTER_API_KEY`: Sets the OpenRouter authentication token.
* `OPENROUTER_MODELS`: Comma-separated list of OpenRouter models to load.
* `OLLAMA_URL`: Overrides the default Ollama API endpoint.
* `CLAUDE_COMMAND`: Overrides the CLI invocation for Claude Code.

---

## 9. Developer Guide & Code Architecture

ArenaFight includes a local developer guide and code tutorial (`docs/DEV_TUTORIAL.md`) covering:
* Embedding `ArenaFight` as a static/shared C++20 library in external projects.
* Authoring custom tools inheriting from `Tool` with JSON validation and RAII cleanup.
* Writing custom model providers (e.g. `llama.cpp` server, `vLLM`) via `ModelProvider`.
* Domain-specific verification gates (AddressSanitizer checks, custom quality linters).
* Session resumption, atomic disk layout, and hardware VRAM tuning.

---

## Credits
**Made by Lunarmist-byte**
* GitHub: [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* LinkedIn: [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)
