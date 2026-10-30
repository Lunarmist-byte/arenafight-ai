# ArenaFight Agent

An autonomous, local-first multi-model AI agent orchestration runtime and execution system built in **C++20**.

ArenaFight Agent is designed for autonomous coding, research, embedded systems engineering, and physical debugging workflows (akin to **Google Antigravity** and **Claude Code** execution). It moves beyond standard chatbot question-and-answer patterns by operating an autonomous, self-evaluating execution loop: understanding tasks, generating structured task graphs with explicit dependencies, invoking tools, updating persistent memory, independently verifying physical evidence, debugging failures, and replanning until the objective is genuinely achieved.

---

## Made by Lunarmist-byte
* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)

---

## 1. Core Architecture

The system executes an autonomous closed-loop cycle:

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

## 2. Hallucination-Resistant Multi-Model Architecture

ArenaFight enforces an inviolable system invariant: **`MODEL OUTPUT != TRUTH`**.

Multiple models frequently share identical incorrect assumptions, training data artifacts, outdated API knowledge, or reasoning errors. Therefore, ArenaFight **prefers concrete evidence over model consensus**.

```text
MODEL OUTPUT
     |
     v
   CLAIM (Hypothesis / Inference)
     |
     v
  EVIDENCE (Compiler, Tests, Files, Exit Codes)
     |
     v
 VERIFICATION (6 Verification Gates)
     |
     v
 TRUSTED STATE (Persistent Verified Memory)
```

### 1. Evidence Levels (`EvidenceLevel`)
Every significant claim is tracked in the central `ClaimRegistry` with an explicit evidence status:
* `VERIFIED`: Proven by Level 1/Level 2 tool execution (compiler, test runner, exact file content).
* `STRONGLY_SUPPORTED`: Backed by authoritative documentation, specifications, or strong indirect evidence.
* `MODEL_DERIVED`: Asserted by one or more models without external tool proof.
* `UNVERIFIED`: Proposed as hypothesis or assumption without supporting tool proof yet.
* `CONTRADICTED`: Directly refuted by compiler errors, test failures, or opposing facts.
* `UNKNOWN`: Insufficient evidence to validate or refute.

### 2. The 6-Tier Evidence Hierarchy
Evidence types are ranked hierarchically. Higher levels strictly dominate lower levels:
1. **LEVEL 1 — Direct Execution Evidence:** Compiler output, test runners, shell exit codes, program stdout/stderr.
2. **LEVEL 2 — Direct Artifact Evidence:** Source files, config files, Git diffs, database contents.
3. **LEVEL 3 — Authoritative Documentation:** Official docs, API schemas, local project specifications.
4. **LEVEL 4 — Independent Model Agreement:** Multiple independent models converging without prior bias.
5. **LEVEL 5 — Single Model Reasoning:** Single model inference or deductions.
6. **LEVEL 6 — Guess / Assumption:** Unsubstantiated model guesses.

### 3. Separation of Facts from Hypotheses
The system rigorously classifies claims as `FACT`, `HYPOTHESIS`, `ASSUMPTION`, `INFERENCE`, or `UNKNOWN`.
* If a model asserts: *"The segmentation fault is caused by use-after-free"*, the system registers it as `KnowledgeType::HYPOTHESIS` with status `UNVERIFIED`.
* It is **strictly prevented** from entering persistent memory as a `FACT` until concrete Level 1/Level 2 evidence (e.g. AddressSanitizer/GDB backtrace or test reproduction) is linked.

### 4. Consensus Is Supporting Evidence, Not Proof
* **5 models agree, 0 external evidence:** Status remains `MODEL_DERIVED`, confidence is capped at `0.50`.
* **1 model claims X, compiler/tests prove X:** Status becomes `VERIFIED`, confidence is `> 0.95`.
Evidence outranks consensus in every evaluation.

### 5. Contradiction Detection & Active Resolution
If independent agents produce conflicting claims (e.g., Agent A claims *"The bug is in parser.cpp"*, while Agent B claims *"The bug is in lexer.cpp"*):
* The system detects the contradiction and registers an explicit `Contradiction` record.
* Sets claim statuses to `CONTRADICTED`.
* Enforces the required action: **`Inspect source + reproduce failure.`**
* Contradictions must be actively resolved via tool evidence before a task can pass verification.

### 6. Dependency-Aware Fresh Verification (Evidence Invalidation)
Any file modification invalidates previous verification that depends on the modified artifact:
* If `src/main.cpp` is modified via `write_file` or `edit_file`, previous build and test evidence automatically becomes `isStale = true`.
* Dependent claims are downgraded from `VERIFIED` back to `UNVERIFIED`, forcing fresh re-compilation and re-testing.

### 7. Independent & Adversarial Verification
* **Blind Review:** When evaluating multiple candidate solutions, they are anonymized (`SOLUTION A`, `SOLUTION B`) without provider or model labels to eliminate provider bias.
* **Adversarial Review (`ReviewMode::ADVERSARIAL`):** An independent model is tasked with actively attempting to prove the solution wrong, probing for command injection, path traversal, buffer overflows, missing edge cases, and invalid API assumptions.

---

## 3. The 6 Verification Gates

Before marking any task or objective complete, the system validates all 6 gates:

| Gate | Gate Name | Required Proof |
| :--- | :--- | :--- |
| **Gate 1** | **Files Exist** | Required artifacts exist on filesystem with size > 0 bytes |
| **Gate 2** | **Code Compiles** | Compiler command exits with code `0` and zero breaking errors |
| **Gate 3** | **Tests Pass** | Automated test runner reports 0 failures and non-zero passed tests |
| **Gate 4** | **Requirements Satisfied** | All planned requirements and acceptance criteria have corresponding evidence |
| **Gate 5** | **Adversarial Review** | Independent verifier discovers 0 critical security or logic vulnerabilities |
| **Gate 6** | **No Contradictions** | Zero unresolved contradictions in `ClaimRegistry` |

---

## 4. OpenCode Integration (`OpenCodeProvider`)

ArenaFight features native integration with [OpenCode](https://opencode.ai) (`anomalyco/opencode`), the open-source AI coding agent CLI:

### Capabilities
* **Model-Agnostic Engine:** Integrates with OpenCode CLI (`opencode` / `opencode.cmd`) detected automatically in PATH.
* **Discovered Models:**
  * `opencode-default`: OpenCode default autonomous agent
  * `opencode/claude-3-7-sonnet`: Claude 3.7 Sonnet via OpenCode
  * `opencode/gemini-2.5-pro`: Google Gemini 2.5 Pro via OpenCode
  * `opencode/deepseek-r1`: DeepSeek R1 reasoning model via OpenCode
  * `opencode/qwen-2.5-coder`: Qwen 2.5 Coder 32B via OpenCode
* **Workspace Synchronization:** Automatically syncs verified invariants and constraints into `.opencode/opencode.json` and `.opencode/INSTRUCTIONS.md`.

### CLI Usage with OpenCode
```bash
# Check OpenCode integration status & discovered models
arena opencode status
arena opencode models

# Run one-off prompts directly through OpenCode CLI
arena opencode run "Explain project structure and verify invariants"

# Synchronize .opencode/ workspace configuration
arena opencode sync

# Execute autonomous goal with OpenCode as primary provider
arena --opencode "Scaffold a C++20 project with unit tests"
```

---

## 5. User Interfaces

### 1. Simplistic UI (`simple_ui.html`)
A focused, distraction-free minimalist interface designed for rapid developer iteration:
* **Zen Layout:** Single-column clean card design with zero visual clutter.
* **Zero Emojis:** Clean inline SVG icons and modern typography.
* **Backend Selector:** Instantly toggle between **OpenCode CLI**, **Claude Code CLI**, **Ollama**, **OpenRouter**, or **Offline Mock**.
* **Mode Actions:** Execute autonomously (`/goal`), draft milestones (`/plan`), run physical verification directly, bridge into OpenCode (`opencode run`), or conduct requirements interviews (`/grill-me`).
* **6-Gate Verification Board:** Real-time visual pass/fail badges for physical verification invariants.
* **Collapsible Terminal:** Telemetry and execution logs with copy and auto-scroll.
* **One-Click Launch:**
```bash
# Launch directly from ArenaFight CLI
arena ui

# Or open in browser
explorer simple_ui.html
```

### 2. Antigravity 2.0 Studio (`index.html`)
For complex, multi-pane workflows requiring full IDE capabilities:
* **Left Sidebar**: Workspace indicator, session history, skills drawer, scheduled tasks monitor.
* **Chat Canvas**: Interactive multi-turn feed with collapsible Thinking Process disclosures, live Tool Execution cards, interactive Plan Cards with "Proceed with Execution", and slash command autocomplete (`/`).
* **Auxiliary Pane**: Artifacts Viewer, Visual Task Graph, 6 Verification Gates, and ANSI Terminal Console.
* Easily toggle between Studio and Simplistic UI via the top navigation header button (`Simplistic UI`).

---

## 6. Refactored C++20 Architecture

The C++ codebase is refactored according to modern C++20 best practices:

1. **Command Dispatcher Pattern (`src/main.cpp`):**
   Replaced legacy chained `if-else` blocks with a structured `std::map<std::string, Handler>` command dispatch table with unified flag and argument tokenization.

2. **Provider Factory Pattern (`src/providers/provider_factory.cpp`):**
   Decoupled model backend instantiation out of `ArenaAgent` into a dedicated factory supporting `OpenCodeProvider`, `ClaudeCodeProvider`, `OllamaProvider`, `OpenRouterProvider`, and `TestMockProvider`.

3. **RAII Temporary File Management (`include/arenafight/common/fs_utils.hpp`):**
   `TempFileGuard` guarantees exception-safe cleanup of temporary prompt and execution files across all CLI backends.

4. **Multi-Model Router Scoring (`src/router/model_router.cpp`):**
   Dynamic task routing supports OpenCode backends with high capability scoring for coding and debugging tasks alongside Claude and local models.

5. **Decoupled Verification Subsystems:**
   `Verifier`, `IndependentReviewer`, and `ClaimRegistry` enforce strict separation between model hypotheses and physical tool evidence.

---

## 7. CLI Reference & Subcommands

```text
Usage:
  arena                          Launch Antigravity Interactive REPL
  arena "<task>"                  Execute a task autonomously (/goal)
  arena goal "<task>"             Autonomous goal pursuit with verification
  arena plan "<task>"             Generate structured plan artifact with milestones
  arena grill-me "<topic>"        Interactive requirements interview
  arena opencode [cmd]           OpenCode CLI integration (status|models|run|sync)
  arena ui [simple|studio]       Launch Simplistic Web UI dashboard in browser
  arena providers                Inspect all AI model backends status
  arena artifacts                List and inspect project artifacts
  arena skills                   List active skills
  arena rules                    Display active workspace and system rules
  arena boards                   List supported microcontroller boards
  arena board <id>               Inspect microcontroller pinout & strapping pins
  arena components               List supported sensors, actuators & ICs
  arena scaffold <board> <proj>  Scaffold hardware project with Wokwi simulation
  arena qos [path]               Evaluate embedded C++ firmware QoS
  arena calculate <type> <args>  Calculate circuit values (led, divider, i2c, battery)
  arena resume <session-id>      Resume an existing session
  arena sessions                 List past saved sessions
  arena models                   Discover and list models across all backends
  arena config                   Display current configuration
  arena test                     Run built-in automated self-tests

Options:
  --opencode                     Select OpenCode as primary provider
  --claude                       Select Claude Code CLI as primary provider
  --ollama                       Select Ollama as primary provider
  --openrouter                   Select OpenRouter as primary provider
  --mock                         Force built-in offline test mock provider
  --no-mock-fallback             Disable automatic fallback to offline mock
  --verbose, -v                  Enable debug output
```

### REPL Slash Commands (inside `arena > `)
* `/goal <task>`: Autonomous goal execution until verified.
* `/plan <task>`: Generates a plan artifact with milestones and dependencies.
* `/grill-me <topic>`: Requirements interview to resolve edge cases and design decisions.
* `/learn <insight>`: Persists repository conventions and rules into `.arena/knowledge/`.
* `/schedule <delay_sec> <task>`: Schedules delayed or recurring background jobs.
* `/artifacts [id]`: Lists and inspects project artifacts with YAML metadata.
* `/skills [name]`: Inspects discovered skills.
* `/rules`: Lists active system and workspace rules.
* `/models`: Discovers and switches models across OpenCode, Claude Code, OpenRouter, and Ollama.
* `/opencode`: OpenCode CLI status, model discovery, one-off execution, and `.opencode` sync.
* `/ui`: Launches the Simplistic Web UI dashboard in your browser.
* `/status`: Displays session telemetry, verified facts, and gate statuses.
* `/exit` / `/quit`: Exits the session.

### Context Mentions (`@`)
* `@file <path>`: Attaches file content directly into prompt context.
* `@git`: Attaches working tree status and diff.
* `@claims`: Injects ClaimRegistry verified evidence.
* `@memory`: Injects recent discoveries and decisions.

---

## 8. Hardware & Embedded QoS Studio

ArenaFight provides embedded systems development and physical circuit tooling:

* **Board Registry:** Pinout constraint databases for ESP32, ESP32-S3, Raspberry Pi Pico, Arduino Uno/Nano, STM32F401, Teensy 4.0. Validates strapping pins, input-only pins (e.g. GPIO 34-39 on ESP32), and voltage tolerances.
* **Circuit Calculator:** Calculates LED current-limiting resistors, resistor voltage dividers, I2C pullup resistor values (for bus capacitance and speed), and battery discharge runtimes.
* **Wokwi Project Scaffolding:** Generates `diagram.json`, PlatformIO configurations (`platformio.ini`), and modern C++20 firmware templates.
* **Firmware QoS Analyzer:** Evaluates firmware source for watchdog presence, non-blocking state machines, zero-heap static allocation compliance, and reentrancy safety.

---

## 9. Configuration (`config.json`)

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
    "anthropic_api_key": "",
    "opencode_command": "opencode",
    "opencode_model": "opencode-default",
    "max_iterations": 30,
    "max_tool_calls": 100,
    "max_retries": 3,
    "context_limit": 32000,
    "parallel_models": false,
    "require_confirmation_for_destructive_commands": true,
    "default_provider": "auto",
    "workspace_dir": ".",
    "test_mock_enabled": false,
    "auto_mock_fallback": true
}
```

Environment variables override configuration automatically:
* `OPENCODE_COMMAND`: Invocation command for OpenCode (default: `opencode`).
* `OPENCODE_MODEL`: Default model selection for OpenCode.
* `OPENROUTER_API_KEY`: Sets the OpenRouter authentication token.
* `OPENROUTER_MODELS`: Comma-separated list of OpenRouter models to load.
* `OLLAMA_URL`: Overrides the default Ollama API endpoint.
* `CLAUDE_COMMAND`: Overrides the CLI invocation for Claude Code.

---

## 10. Building and Testing

### Prerequisites
* **C++ Compiler:** Supporting C++20 (`g++` 11+, `clang++` 13+, or MSVC 2022)
* **Build System:** `cmake` (>= 3.20) and `ninja`
* **Dependencies:** `libcurl`, `Threads` (nlohmann_json is automatically fetched via FetchContent if not found)

### Building
```bash
# Configure build directory
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build executable and test suite
ninja -C build
```

### Running Tests
The automated test suite executes **21** assertions covering configuration, tools, process safety, evidence hierarchy, multi-model consensus, and OpenCode integration:
```bash
# Run test suite executable directly
./build/arena_test.exe

# Or run via CTest
ctest --test-dir build --output-on-failure
```

---

## Credits
**Made by Lunarmist-byte**
* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)
