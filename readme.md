# ArenaFight Agent

**Version:** 2.5.0 
**Architecture:** Pure C++20 Native Standard  

An autonomous, local-first multi-model AI agent orchestration runtime and execution system built in **C++20**.

ArenaFight is primarily a **developer-first, high-performance CLI tool** (`arena` / `arena.exe`) designed for fast terminal workflows, autonomous coding, research, embedded systems engineering, and physical debugging (akin to advanced autonomous agent execution). It moves beyond standard chatbot question-and-answer patterns by operating an autonomous, self-evaluating execution loop: understanding tasks, generating structured task graphs with explicit dependencies, evaluating model abilities to plan optimal assignments, invoking tools, updating persistent memory, independently verifying physical evidence, debugging failures, and replanning until the objective is genuinely achieved.

It operates with a dual-mode runtime architecture:
* **Standalone Mode (Default):** Runs completely self-contained with zero external dependencies, native C++ tool executors, local model providers (Ollama, local vLLM, custom OpenAI endpoints, OpenRouter, or TestMock), 6 physical verification gates, fast thinking-phase tournaments, Unsloth fine-tuning, and the universal benchmarking engine.
* **Connected to OpenCode Mode:** Bridges seamlessly to the [OpenCode](https://opencode.ai) CLI ecosystem (`opencode run`, `.opencode/opencode.json`, and `.opencode/INSTRUCTIONS.md` sync), routing complex coding goals through OpenCode while subjecting all generated code and artifacts to ArenaFight's Level 1 physical evidence verification.

As a pure CLI tool, ArenaFight delivers all telemetry, gate verifications, hardware allocations, and benchmarks directly into your terminal (`arena metrics`, `arena status`, `arena profile`).

---

## Made by Lunarmist-byte
* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)

---

## 1. CLI Tool Architecture & Dual-Mode Runtime (Standalone vs OpenCode Connected)

ArenaFight is designed first and foremost as a UNIX-style developer CLI tool (`arena`). It can run fully isolated or directly bridge to OpenCode:

### 1. Standalone Mode (Default: `arena --standalone` or `arena mode standalone`)
In Standalone mode, ArenaFight operates with complete autonomy:
* **Zero External Process Dependencies:** Uses native C++ file operations, process executors, git bindings, and local/cloud providers.
* **Direct Model Providers:** Connects directly to local Ollama (`localhost:11434`), custom OpenAI-compatible endpoints (`--base-url`, `--api-key`), OpenRouter, Claude Code CLI, or built-in TestMock.
* **Full Local Verifier:** Evaluates all 6 physical verification gates, executes fast thinking tournaments, runs on-the-fly Unsloth LoRA fine-tuning, and executes the universal benchmarking engine locally.

### 2. Connected to OpenCode Mode (`arena --opencode` or `arena connect opencode`)
In Connected mode, ArenaFight harnesses the OpenCode CLI ecosystem as an execution engine while enforcing physical proof:
* **Bi-Directional OpenCode Bridge:** Communicates directly with `opencode run`, discovering installed OpenCode models (`opencode-default`, `opencode/claude-3-7-sonnet`, `opencode/gemini-2.5-pro`, `opencode/deepseek-r1`, `opencode/qwen-2.5-coder`).
* **Workspace Configuration Synchronization:** Automatically synchronizes `.opencode/opencode.json` and `.opencode/INSTRUCTIONS.md` with active project constraints and memory facts.
* **Level 1 Physical Evidence Verification:** All code written or modified via OpenCode is still subjected to ArenaFight's non-negotiable verification gates (compiler exit codes, automated test suites, SHA-256 checks).

### 3. CLI Mode Commands & Flags
```bash
# Switch runtime modes
arena mode standalone          # Set runtime mode to Standalone
arena mode opencode            # Set runtime mode to OpenCode Connected
arena mode                     # Display active mode and status

# Connect / Disconnect OpenCode bridge
arena connect opencode         # Connect to OpenCode and sync workspace
arena disconnect               # Return to standalone mode

# Inspect complete runtime status and bridges
arena status                   # Full terminal status dashboard

# Launch tasks with explicit mode flags
arena --standalone "Design lock-free queue"
arena --opencode "Refactor parser with unit tests"
arena --mode standalone "Run adversarial security audit"
```

### 4. Interactive REPL Mode Commands
Inside the interactive terminal REPL (`arena > `):
```text
/mode [standalone|opencode]    Switch runtime mode on the fly
/connect [opencode]            Connect and synchronize OpenCode bridge
/disconnect                    Disconnect bridge and revert to standalone mode
/status                        Display runtime architecture and verification gates
```

### 5. Terminal Telemetry & Live Evidence Observatory (`arena metrics`)
ArenaFight is built from the ground up as a pure CLI tool. All telemetry, verification proof, and hardware metrics are rendered natively in the terminal:
```bash
arena metrics                  # Display terminal live telemetry observatory
arena telemetry                # Alias for arena metrics
arena status                   # Full runtime architecture & provider health
arena profile                  # Physical PC profiling & dynamic limits
arena benchmark                # Universal multi-model capability benchmark
```
* **Real-time 6 Physical Evidence Gates:** Visualizes status across Artifacts, Compilation, Tests, Milestones, Security, and Contradictions.
* **Hardware Allocation & Headroom:** Dynamic CPU threads, available RAM, and dedicated CUDA vs iGPU/CPU status.
* **Token Efficiency & Duel Telemetry:** Quantified token savings (~400-1200 tokens per turn) from early thinking phase termination.
* **Unsloth AI Adapters:** Active LoRA rank, alpha, and dataset sync status.

---

## 2. Core Architecture

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
               |     Ability-Based Matching)      |
               +----------------------------------+
                                 |
                                 v
+------------> +----------------------------------+
|              |      TASK QUEUE DISPATCHER       | <---+
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |       ABILITY MODEL ROUTER       |     |
|              |  (Scored by Specialized Ability) |     |
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
|              | (Claims, Knowledge, Decisions)   |     |
|              +----------------------------------+     |
|                                |                      |
|                                v                      |
|              +----------------------------------+     |
|              |       INDEPENDENT VERIFIER       |     |
|              |    (6 Physical Evidence Gates)   |     |
|              +----------------------------------+     |
|                                |                      |
|                     Is Evidence Satisfied?            |
|                     /                    \            |
|                   YES                     NO          |
|                   /                        \          |
|                  v                          v         |
|         +-----------------+       +-----------------+ |
|         | COMPLETED TASK  |       | RECOVERY PLAN   |-+
|         | (Verified Proof)|       | (Auto Debugging)|
|         +-----------------+       +-----------------+
```

---

## 3. Grounded Execution & Anti-Confabulation Invariants

ArenaFight operates with a strict, non-negotiable **Anti-Confabulation Invariant**:

### 1. Model Claim is Not Ground Truth
A model asserting *"I wrote the file"* or *"Tests pass"* is treated as an unverified hypothesis. Only direct physical tool execution results (exit code `0`, disk presence, stdout proof) can substantiate a claim.

### 2. Physical Evidence Hierarchy
All evidence recorded in the `ClaimRegistry` is ranked hierarchically:
* **Level 1 (Highest):** `DIRECT_EXECUTION` — Shell exits with code `0`, test runner logs.
* **Level 2:** `ARTIFACT_VERIFICATION` — File exists on disk, size > 0, SHA-256 validated.
* **Level 3:** `TOOL_OUTPUT` — Output from read/search tools.
* **Level 4:** `PROJECT_METADATA` — Configs, CMake files, package.json.
* **Level 5:** `USER_INPUT` — Explicit human user prompt or confirmation.
* **Level 6 (Lowest):** `MODEL_DERIVED` — Pure LLM inference or assertion.

> **Dominance Rule:** Level 1/2 evidence strictly dominates Level 6 claims. A lower-level claim cannot override or invalidate a higher-level physical claim.

### 3. Separation of Facts vs. Hypotheses
Every memory item is classified into:
* `FACT` — Supported by Level 1/2 physical proof.
* `HYPOTHESIS` — Generated by a model, requires tool confirmation before action.
* `ASSUMPTION` — Working premise, downgraded if contradictions appear.

### 4. Consensus is Not Proof
If 5 models predict *"This C++ code will compile"*, but the local compiler (`g++`) exits with code `1`, the compiler result is absolute truth. Consensus without physical execution remains Level 6 hypothesis.

### 5. Contradiction Detection & Active Resolution
If conflicting claims arise, the system blocks completion and triggers an active resolution subtask (e.g., executing a diff, inspecting file contents directly, or running an isolated test).

### 6. Evidence Invalidation on File Modification
Evidence is tied to specific file paths and timestamps:
* If `src/main.cpp` is modified via `write_file` or `edit_file`, previous build and test evidence automatically becomes `isStale = true`.
* Dependent claims are downgraded from `VERIFIED` back to `UNVERIFIED`, forcing fresh re-compilation and re-testing.

### 7. Independent & Adversarial Verification
* **Blind Review:** When evaluating candidate solutions, they are anonymized (`SOLUTION A`, `SOLUTION B`) without provider or model labels to eliminate provider bias.
* **Adversarial Review (`ReviewMode::ADVERSARIAL`):** An independent model is tasked with actively attempting to prove the solution wrong, probing for command injection, path traversal, buffer overflows, missing edge cases, and invalid API assumptions.

---

## 4. The 6 Verification Gates

Before marking any task or objective complete, the system validates all 6 physical gates:

| Gate | Gate Name | Required Proof |
| :--- | :--- | :--- |
| **Gate 1** | **Files Exist** | Required artifacts exist on filesystem with size > 0 bytes |
| **Gate 2** | **Code Compiles** | Compiler command exits with code `0` and zero breaking errors |
| **Gate 3** | **Tests Pass** | Automated test runner reports 0 failures and non-zero passed tests |
| **Gate 4** | **Requirements Satisfied** | All planned requirements and acceptance criteria have corresponding evidence |
| **Gate 5** | **Adversarial Review** | Independent verifier discovers 0 critical security or logic vulnerabilities |
| **Gate 6** | **No Contradictions** | Zero unresolved contradictions in `ClaimRegistry` |

---

## 5. Hyperparameter Customization: Temperature & Token Generation

ArenaFight gives you complete, granular control over generation parameters across all backends:

### Temperature Control
* Set arbitrary sampling temperature (`0.0` to `2.0`).
* Lower values (`0.0 - 0.2`) guarantee maximum determinism for C++ syntax, register maps, and compiler flags.
* Higher values (`0.7 - 1.2`) promote creative architectural exploration and alternative test edge case generation.

### Max Output Tokens
* Allocate generation limits dynamically from `256` to `32,768+` tokens.
* Prevents truncated code outputs during full-file refactoring and embedded driver scaffolding.

### Usage Options
```bash
# Set temperature and token generation via CLI flags
arena --temperature 0.15 --max-tokens 8192 "Refactor memory manager"
arena -t 0.7 --tokens 16384 "Explore distributed queue architectures"

# Environment Variable Overrides
export ARENA_TEMPERATURE=0.25
export ARENA_MAX_TOKENS=8192

# In Interactive REPL
arena > /temp 0.1
arena > /tokens 12000
arena > /config
```

---

## 6. Intelligent Ability-Based Model Routing & Task Graph Planning

ArenaFight plans which model to use by evaluating multidimensional capability profiles rather than relying on static vendor defaults:

### Multidimensional Ability Vector (`ModelAbilities`)
Each model is profiled across 7 core capability dimensions (`0.0` to `1.0`):
1. **Coding (`coding`):** C++, Python, algorithms, low-level syntax precision.
2. **Reasoning (`reasoning`):** Multi-step architectural logic, decomposition, math.
3. **Planning (`planning`):** Dependency resolution, ordering, risk estimation.
4. **Verification (`verification`):** Adversarial bug hunting, edge cases, hallucination checking.
5. **Hardware (`hardware`):** Embedded C/C++, pinouts, strapping pins, Wokwi, circuit math.
6. **Speed (`speed`):** Low latency, fast token throughput for iterative searches.
7. **Context (`context`):** Large repository context window processing.

### Task Graph Ability Matching
When `Planner::createInitialPlan` generates subtasks, it automatically evaluates the objective and required skill set for each subtask:
* **Research / Discovery Tasks:** Planned for models with high **Context & Speed** abilities (e.g., Claude 3.7 Sonnet, Gemini 2.5 Pro).
* **Firmware & C++ Coding Tasks:** Planned for models with high **Coding & Hardware** abilities (e.g., Qwen 2.5 Coder 32B, Unsloth Fine-Tuned Local).
* **Architectural & Debugging Tasks:** Planned for high **Reasoning** models (e.g., DeepSeek R1).
* **Verification & Audit Tasks:** Planned for dedicated independent verifiers with high **Verification** abilities.

### Inspecting Ability Matrix
```bash
# View active model capability breakdown
arena abilities
```

Example output:
```text
MODEL ID                    PROVIDER    CODE    REASON  PLAN    VERIF   HW      SPEED   STRENGTH
--------------------------------------------------------------------------------------------------
opencode-default            OpenCode    0.94    0.92    0.90    0.90    0.88    0.90    Coding
opencode/claude-3-7-sonnet  OpenCode    0.96    0.98    0.97    0.95    0.90    0.85    Reasoning
opencode/gemini-2.5-pro     OpenCode    0.93    0.94    0.95    0.92    0.87    0.88    Large Context
opencode/deepseek-r1        OpenCode    0.92    0.99    0.98    0.96    0.86    0.70    Reasoning
opencode/qwen-2.5-coder     OpenCode    0.99    0.91    0.88    0.92    0.95    0.92    Coding
arena-finetuned (Unsloth)   Ollama      0.98    0.95    0.92    0.94    0.97    0.96    Embedded QoS C++
```

---

## 7. Unsloth AI Direct Integration: On-The-Fly Fine-Tuning

ArenaFight natively integrates with **Unsloth AI** (`unsloth`), enabling 2x faster fine-tuning with 70% less VRAM consumption through custom Triton and CUDA kernels:

### Capabilities
* **On-The-Fly Trajectory Extraction:** Converts successful ArenaFight sessions and self-correction failure recovery trajectories into ChatML and Alpaca fine-tuning pairs.
* **Synthetic Hardware Datasets:** Generates embedded firmware datasets covering ESP32 strapping pins, circuit formulas, FreeRTOS QoS invariants, and Wokwi configurations.
* **Automated Scaffolding:** Generates production-ready standalone PyTorch scripts (`unsloth_train.py`) utilizing 4-bit QLoRA, `FastLanguageModel`, and `SFTTrainer`.
* **GGUF Export & Ollama Auto-Registration:** Automatically quantizes fine-tuned models to GGUF (`q4_k_m`), writes an Ollama `Modelfile`, and registers the model into Ollama so ArenaFight can immediately deploy its newly trained model.

### CLI Usage with Unsloth
```bash
# Check Python, Unsloth, PyTorch, and CUDA environment diagnostics
arena unsloth status

# Generate fine-tuning dataset from agent history and hardware rules
arena unsloth dataset

# Generate standalone unsloth_train.py script
arena unsloth scaffold

# Launch on-the-fly fine-tuning loop (or add --dry-run for simulation)
arena unsloth train
```

---

## 8. OpenCode Bridge Integration (`OpenCodeProvider`)

ArenaFight features native integration with [OpenCode](https://opencode.ai) (`anomalyco/opencode`), the open-source AI coding agent CLI:

### Capabilities
* **Model-Agnostic Engine:** Integrates with OpenCode CLI (`opencode` / `opencode.cmd`) detected automatically in PATH.
* **Bi-Directional Collaboration:**
  * **ArenaFight calling OpenCode:** ArenaFight dispatches complex coding goals through OpenCode (`arena --opencode "<goal>"` or `arena opencode run "<prompt>"`).
  * **OpenCode calling ArenaFight:** OpenCode can invoke ArenaFight directly via native Model Context Protocol (MCP) tools, custom slash commands, or shell execution.
* **Discovered Models:**
  * `opencode-default`: OpenCode default autonomous agent
  * `opencode/claude-3-7-sonnet`: Claude 3.7 Sonnet via OpenCode
  * `opencode/gemini-2.5-pro`: Google Gemini 2.5 Pro via OpenCode
  * `opencode/deepseek-r1`: DeepSeek R1 reasoning model via OpenCode
  * `opencode/qwen-2.5-coder`: Qwen 2.5 Coder 32B via OpenCode
* **Automated Workspace Synchronization:** `arena opencode sync` automatically writes root `opencode.json`, `.opencode/opencode.json`, `.opencode/commands/*.md`, and `.opencode/INSTRUCTIONS.md` to establish the MCP server link and custom commands.

### How to Call ArenaFight in OpenCode

Once synced (`arena opencode sync` or `arena connect opencode`), ArenaFight is fully accessible inside your OpenCode session:

#### 1. Native OpenCode Slash Commands
You can type these commands directly into the OpenCode prompt:
* `/arena <task or subcommands>` -- Run any ArenaFight task or command inside the workspace.
* `/verify` -- Run the 6 Level 1 physical evidence verification gates on the current workspace (`arena verify`).
* `/profile` -- Assess host PC hardware (CPU cores, RAM headroom, NVIDIA CUDA GPU) and adaptive limits (`arena profile`).
* `/benchmark` -- Benchmark models across reasoning, coding, and throughput (`arena benchmark`).
* `/qos [path]` -- Audit embedded firmware for zero-heap ISR compliance and watchdog timers (`arena qos`).

#### 2. Native Model Context Protocol (MCP) Server Tools
OpenCode automatically discovers the `arena` local MCP server defined in `opencode.json` (`command: ["arena", "mcp"]`). Any agent in OpenCode can call these tools:
* `arena_verify`: Run Level 1 physical evidence verification gates (compiler, test suite, file diff, QoS, security).
* `arena_goal`: Delegate an autonomous engineering task to ArenaFight's self-healing execution loop.
* `arena_plan`: Formulate a structured task graph with milestones and physical verification criteria.
* `arena_profile`: Assess physical host hardware resources and dynamic limits.
* `arena_benchmark`: Benchmark available models across reasoning, coding invariants, and throughput.
* `arena_qos`: Evaluate embedded firmware for zero-heap ISR compliance, non-blocking loops, and watchdog timers.
* `arena_calculate`: Calculate circuit component values (Ohm's law, LED current limiter, resistor divider, I2C pullup, battery lifetime).
* `arena_security`: Run workspace vulnerability scanner, DLP secret detector, and SHA-256 audit chain check.
* `arena_think`: Run fast thinking tournaments comparing pure reasoning without code generation.

#### 3. Terminal CLI Commands
OpenCode agents and users can also invoke ArenaFight via shell commands:
```bash
# Level 1 Physical Verification (Model Output != Truth)
arena verify

# Hardware assessment
arena profile

# Autonomous goal pursuit
arena "Implement feature and verify with unit tests"

# Start MCP stdio server manually
arena mcp
```

### CLI Usage with OpenCode
```bash
# Check OpenCode integration status & discovered models
arena opencode status
arena opencode models

# Run one-off prompts directly through OpenCode CLI
arena opencode run "Explain project structure and verify invariants"

# Synchronize .opencode/ workspace configuration & MCP server
arena opencode sync

# Execute autonomous goal with OpenCode as primary provider
arena --opencode "Scaffold a C++20 project with unit tests"
```

---

## 9. Refactored C++20 Architecture

The C++ codebase is refactored according to modern C++20 best practices:

1. **Command Dispatcher Pattern (`src/main.cpp`):**
   Structured `std::map<std::string, Handler>` command dispatch table with unified flag and argument tokenization.

2. **Provider Factory Pattern (`src/providers/provider_factory.cpp`):**
   Decoupled model backend instantiation out of `ArenaAgent` into a dedicated factory supporting `OpenCodeProvider`, `ClaudeCodeProvider`, `OllamaProvider`, `OpenRouterProvider`, and `TestMockProvider`.

3. **RAII Temporary File Management (`include/arenafight/common/fs_utils.hpp`):**
   `TempFileGuard` guarantees exception-safe cleanup of temporary prompt and execution files across all CLI backends.

4. **Ability-Based Multi-Model Router (`src/router/model_router.cpp`):**
   Dynamic task routing evaluating 7-dimensional ability vectors (`ModelAbilities`) to match coding, reasoning, planning, and verification tasks with highest-scoring models.

5. **Unsloth On-The-Fly Fine-Tuning Engine (`src/finetune/unsloth_manager.cpp`):**
   Autonomous dataset formatting, LoRA script generation, training execution, GGUF conversion, and Ollama registration.

6. **File Operations & Targeted Context Injection Engine (`src/session/context_manager.cpp`, `src/repl/repl_engine.cpp`):**
   Full multi-mention `@file <path>` resolver supporting quoted paths (`@file "path with spaces/file.txt"`), CLI `--file <path>` / `--files <p1,p2>` attachment flags, `/files` REPL file explorer, and custom file dataset extraction across C++, Python, JavaScript/TypeScript, JSON, Markdown, and configuration formats.

---

## 10. CLI Reference & Subcommands

```text
Usage:
  arena                          Launch Interactive REPL
  arena "<task>"                  Execute a task autonomously (/goal)
  arena goal "<task>"             Autonomous goal pursuit with verification
  arena verify                    Run Level 1 physical evidence verification gates
  arena mcp                       Run Model Context Protocol (MCP) server over stdio
  arena plan "<task>"             Generate structured plan artifact with milestones
  arena grill-me "<topic>"        Interactive requirements interview
  arena mode [standalone|opencode] Inspect or switch runtime execution mode
  arena connect [opencode]        Connect and synchronize OpenCode bridge
  arena disconnect               Disconnect bridge and switch to standalone mode
  arena status                   Display runtime architecture and verification status
  arena security [cmd]           Defense-in-depth security posture & vulnerability scan
  arena audit                    Verify tamper-evident SHA-256 cryptographic audit trail
  arena benchmark [all|models...] Benchmark models across Reasoning, Coding, Verif, TPS
  arena opencode [cmd]           OpenCode CLI integration (status|models|run|sync)
  arena unsloth [cmd]            Unsloth AI on-the-fly fine-tuning (status|dataset|scaffold|train)
  arena files [cmd]              Workspace file operations (list|cat|read|attach|attached)
  arena think "<task>"           Fast thinking tournament (Code generation disallowed)
  arena duel "<task>"            Alias for think tournament
  arena abilities                Display model ability matrix & capability scores
  arena metrics, telemetry       Display live verification & evidence telemetry observatory
  arena help-session, guide      Launch interactive guided help session & diagnostics
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
  --standalone                   Run in isolated standalone mode (Default)
  --opencode, --connected        Run connected to OpenCode CLI bridge
  --mode <standalone|opencode>   Set runtime execution mode
  --file, -f <path>              Attach specific file context to prompt/session
  --files <p1,p2,...>            Attach multiple comma-separated files
  --temperature, -t <float>      Set model generation temperature (0.0 to 2.0)
  --max-tokens, --tokens <int>   Set max generation token limit
  --claude                       Select Claude Code CLI as primary provider
  --ollama                       Select Ollama as primary provider
  --openrouter                   Select OpenRouter as primary provider
  --mock                         Force built-in offline test mock provider
  --no-mock-fallback             Disable automatic fallback to offline mock
  --verbose, -v                  Enable debug output
```

### REPL Slash Commands (inside `arena > `)
* `/mode [standalone|opencode]`: Switch runtime mode dynamically on the fly.
* `/connect [opencode]`: Connect and synchronize OpenCode workspace bridge.
* `/disconnect`: Disconnect OpenCode bridge and revert to standalone mode.
* `/status`: Displays runtime mode, connected bridge state, telemetry, and gate statuses.
* `/security [scan|audit|level]`: Defense-in-depth security posture, path jail & vulnerability scan.
* `/audit`: Inspect tamper-evident SHA-256 audit log and verify cryptographic hash chain.
* `/benchmark [all|models...]`: Run universal model benchmark suite across models.
* `/goal <task>`: Autonomous goal execution until verified.
* `/plan <task>`: Generates a plan artifact with milestones and dependencies.
* `/grill-me <topic>`: Requirements interview to resolve edge cases and design decisions.
* `/think <task>`: Fast thinking tournament with code generation disallowed (compares internal reasoning directly).
* `/duel <task>`: Alias for `/think`.
* `/guide` / `/help session`: Launches interactive guided help session and system diagnostics.
* `/files [cmd]`: Inspect, preview, and attach workspace files (`list`, `cat <path>`, `attach <path>`, `attached`, `clear`).
* `/unsloth [cmd]`: On-the-fly fine-tuning with Unsloth AI (status, dataset [path-or-file], scaffold, train).
* `/abilities`: Displays model ability matrix and multidimensional scores.
* `/temp [val]`: Inspects or updates model generation temperature (0.0 - 2.0).
* `/tokens [val]`: Inspects or updates max output token limit.
* `/learn <insight>`: Persists repository conventions and rules into `.arena/knowledge/`.
* `/schedule <delay_sec> <task>`: Schedules delayed or recurring background jobs.
* `/artifacts [id]`: Lists and inspects project artifacts with YAML metadata.
* `/skills [name]`: Inspects discovered skills.
* `/rules`: Lists active system and workspace rules.
* `/models`: Discovers and switches models across OpenCode, Claude Code, OpenRouter, and Ollama.
* `/opencode`: OpenCode CLI status, model discovery, one-off execution, and `.opencode` sync.
* `/metrics` / `/telemetry`: Displays live terminal verification and telemetry metrics observatory.
* `/exit` / `/quit`: Exits the session.

### Context Mentions
* `@file <path>`: Injects file contents directly into prompt context. Supports multiple mentions and quoted paths (e.g. `@file "src/main.cpp"`).
* `@git`: Injects git working tree status and diff.
* `@claims`: Injects physical facts from ClaimRegistry.
* `@memory`: Injects discoveries and persistent memory items.

---

## 11. Hardware & Embedded QoS Studio

ArenaFight provides embedded systems development and physical circuit tooling:

* **Board Registry:** Pinout constraint databases for ESP32, ESP32-S3, Raspberry Pi Pico, Arduino Uno/Nano, STM32F401, Teensy 4.0. Validates strapping pins, input-only pins (e.g. GPIO 34-39 on ESP32), and voltage tolerances.
* **Circuit Calculator:** Calculates LED current-limiting resistors, resistor voltage dividers, I2C pullup resistor values (for bus capacitance and speed), and battery discharge runtimes.
* **Wokwi Project Scaffolding:** Generates `diagram.json`, PlatformIO configurations (`platformio.ini`), and modern C++20 firmware templates.
* **Firmware QoS Analyzer:** Evaluates firmware source for watchdog presence, non-blocking state machines, zero-heap static allocation compliance, and reentrancy safety.

---

## 12. Configuration (`config.json`)

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
    "default_temperature": 0.2,
    "default_max_tokens": 4096,
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
* `ARENA_TEMPERATURE`: Overrides generation temperature (e.g. `0.2`).
* `ARENA_MAX_TOKENS`: Overrides maximum output tokens (e.g. `8192`).
* `OPENCODE_COMMAND`: Invocation command for OpenCode (default: `opencode`).
* `OPENCODE_MODEL`: Default model selection for OpenCode.
* `OPENROUTER_API_KEY`: Sets the OpenRouter authentication token.
* `OPENROUTER_MODELS`: Comma-separated list of OpenRouter models to load.
* `OLLAMA_URL`: Overrides the default Ollama API endpoint.
* `CLAUDE_COMMAND`: Overrides the CLI invocation for Claude Code.

---

---

## 13. Fast Thinking-Phase Tournament & Early Stopping

In conventional LLM duels, both models generate complete implementations (consuming 2,000 to 5,000 tokens over 20-45 seconds), wasting immense compute and time on the losing candidate. ArenaFight accelerates model selection by dueling models during their **internal thinking phase**:

1. **Code Generation Disallowed:** Candidate models are strictly forbidden from outputting code or solution syntax. They provide only chain-of-thought, structural decomposition, edge-case anticipation, and safety invariants.
2. **Bounded Output Window:** Tokens are strictly constrained to <= 384 tokens with temperature 0.1, finishing in ~200ms.
3. **Multi-Vector Thinking Evaluation:**
   * *Edge-Case Anticipation:* Boundary conditions, null pointers, integer overflows, race conditions, network/hardware timeouts.
   * *Invariant Soundness:* RAII deterministic destruction, smart pointer semantics, zero-heap safety bounds, exception guarantees.
   * *Algorithmic Efficiency:* Time and space complexity analysis (O(1), O(N log N)), cache locality, lock contention.
   * *Discipline Score:* Heavy penalty for premature code dumping when disallowed.
4. **Early Stopping & Savings:** The winner is selected immediately; full generation from the losing model is avoided entirely, saving over 80% in latency and ~4,500 compute tokens per tournament.

CLI & REPL usage:
```bash
# Run thinking duel via CLI
arena think "Design lock-free ring buffer with zero-heap guarantees" mock-reasoner mock-coder

# Run inside interactive REPL
arena > /think Design high-throughput async worker pool
```

---

## 14. Terminal Telemetry & Live Metrics Observatory (`arena metrics`)

ArenaFight operates purely as a high-performance terminal CLI runtime, streaming all live telemetry, verification gate evidence, hardware profiling, and model capabilities directly to the console:

```bash
# Display live terminal telemetry observatory
arena metrics
arena telemetry

# Inspect runtime status and active providers
arena status

# Physical host hardware limits & acceleration
arena profile
```

### Telemetry Observatory Components:
* **Host Platform & Dynamic Hardware Profile:** Real-time CPU logical threads, available RAM headroom, and dedicated NVIDIA CUDA / iGPU / CPU acceleration status.
* **6 Physical Verification Gates Status:** Live tracking across all 6 physical evidence gates (Artifacts on Disk, Clean Compilation, Automated Tests, Plan Dependencies, Adversarial Review, and Zero Contradictions).
* **Thinking Phase Tournament Efficiency:** Tracks tokens saved per turn (~400 to 1200 tokens) and latency reductions achieved through early termination of speculative decoding.
* **Unsloth AI Fine-Tuning Status:** Monitors LoRA adapter ranks, alpha scales, 4-bit quantization targets, and training dataset synchronization.
* **Active Session Claim Registry:** Displays verified empirical facts vs model-derived hypotheses and unresolved contradictions.

---

## 15. Interactive Help Session & System Diagnostics

Get comprehensive runtime guidance, system diagnostics, and cheat sheets at any time:
```bash
# Launch interactive help session from CLI
arena help-session
# or
arena guide

# Launch inside interactive REPL
arena > /help session
# or
arena > /guide
```

The help session displays:
* Active workspace directory and configuration parameters.
* Online/offline status across all 5 providers (OpenCode, Claude Code, Ollama, OpenRouter, TestMock).
* Explanation of thinking phase tournaments, file context injection, and Unsloth AI fine-tuning.
* Summary of the 6 Physical Evidence Verification Gates and Hardware Studio commands.
* Quick-start cheat sheet with copyable examples.

---

## 16. Universal Compatibility With Any Models

ArenaFight Agent is completely architecture-agnostic and works universally with **any models you provide**: local Ollama models, OpenRouter cloud models, OpenAI-compatible endpoints, custom fine-tuned weights, or user-specified model strings.

### 1. Universal Model CLI & REPL Commands
* **CLI Flag:** `arena --model <name>` or `arena -m <name>` sets the universal model for any command.
* **Custom Base URL & Key:** `arena --base-url <url> --api-key <key>` connects to any OpenAI-compatible API endpoint (vLLM, SGLang, LiteLLM, Ollama, OpenRouter).
* **CLI Subcommand:** `arena model [model_name]` inspects or switches the active model; `arena models [set <name>]` lists catalog or sets default.
* **Environment Variables:** `ARENA_MODEL` or `DEFAULT_MODEL`, `OPENAI_BASE_URL` or `OPENAI_API_BASE`, `OPENAI_API_KEY`.
* **Interactive REPL:** `/model <name>` dynamically switches the active model on the fly.
* **Web UI Dashboards:** Type any custom model into the Prism Command Deck or click the Vanguard model selector badge.

### 2. Universal Ability Inference & Scaling
When an arbitrary model is introduced, ArenaFight automatically analyzes the model string and infers a multi-dimensional capability vector across 8 axes:
* **Architecture Detection:** DeepSeek (R1, V3), Qwen (2.5, Coder, Math, QwQ), Llama (3.1, 3.2, 3.3), Claude (3.5, 3.7 Sonnet), OpenAI (GPT-4o, o1, o3-mini), Mistral / Codestral, Google (Gemini 2.5/2.0), Gemma, Phi, etc.
* **Parameter Scale Weighting:** Recognizes parameter scale markers (`405b`, `70b`, `32b`, `14b`, `8b`, `3b`, `flash`, `mini`) and scales capabilities proportionally.
* **Universal Custom Baseline:** Custom or private fine-tuned models receive a reliable baseline score of `>=0.80` across all traits, preventing artificial bottlenecks.
* **Dynamic Provider Binding:** Models with `:` automatically route to Ollama; models with `/` route to OpenRouter; `opencode/` routes to OpenCode CLI; or fallback to active providers dynamically.

### 3. Universal Thinking Duels
Duel any two arbitrary models during their thinking phase:
```bash
arena think "Implement lock-free queue with memory barriers" deepseek-r1:70b qwen2.5:32b
```
Extracts internal thoughts seamlessly across `<think>`, `<thinking>`, `reasoning_content`, and raw streams, scoring edge-case handling and invariant safety while penalizing premature code dumping.

### 4. Universal On-The-Fly Unsloth Fine-Tuning
Fine-tune any arbitrary Hugging Face base model directly:
```bash
# REPL command with custom base model
arena > /unsloth scaffold meta-llama/Llama-3.1-8B-bnb-4bit
arena > /unsloth train meta-llama/Llama-3.1-8B-bnb-4bit
```
The resulting model (e.g. `arena-finetuned`) is dynamically registered into the runtime model router and immediately made available across all CLI, REPL, and UI workflows.

---

## 17. Universal Model Benchmarking Suite (`arena benchmark`, `/benchmark`)

ArenaFight features a built-in, universal benchmarking engine (`BenchmarkEngine`) that evaluates and ranks all available models, providers, or custom candidate subsets across multi-dimensional, objective criteria:

### 1. Standard Benchmark Suite (4 Standard Tasks)
1. **Task 1: Advanced Reasoning & Architectural Invariants (35% weight)**
   * Rigorous concurrency and memory safety reasoning on a bounded, lock-free ring buffer.
   * Assesses edge-case anticipation (null pointer dereferences, integer overflow, race conditions, atomic visibility) and RAII safety without writing raw code.
2. **Task 2: Systems Coding & Invariant Implementation (35% weight)**
   * Implementation of high-performance C++20 constructs: cache-line alignment (`alignas(64)`), `std::atomic`, explicit `std::memory_order_acquire`/`std::memory_order_release` semantics, and zero-heap allocations.
3. **Task 3: Adversarial Vulnerability Verification (20% weight)**
   * Adversarial audit of flawed code snippets to detect security vulnerabilities: unchecked `strcpy` buffer overflows, memory leaks, use-after-free, and unchecked array bounds.
4. **Task 4: Throughput & Latency QoS (10% weight)**
   * Direct measurement of tokens-per-second (TPS) and time-to-first-token/total latency (ms) under sustained execution.

### 2. Composite Scoring & Academic Grading
* **Letter Grades:**
  * **S+**: Composite Score >= 95.0% (Exceptional across all dimensions)
  * **S**: Composite Score >= 90.0% (Elite reasoning and coding)
  * **A**: Composite Score >= 80.0% (Production ready)
  * **B**: Composite Score >= 70.0% (Capable with verification supervision)
  * **C**: Composite Score >= 60.0% (Basic capability)
  * **F**: Composite Score < 60.0% (Fails invariant checks)
  * **OFFLINE**: Provider or endpoint unreachable

### 3. CLI & REPL Commands
```bash
# Benchmark all discovered models across all providers
arena benchmark all
# or shorthand
arena bench all

# Benchmark specific candidate models or providers
arena benchmark mock-reasoner mock-coder mock-verifier
arena benchmark deepseek-r1 qwen-2.5-coder claude-3-7-sonnet

# Run inside interactive REPL
arena > /benchmark
arena > /benchmark all
arena > /bench mock-reasoner mock-coder
```

### 4. Leaderboard Output & Artifact Persistence
The engine prints a clean ASCII leaderboard with category champions and automatically saves complete suite results to `benchmark_results.json`:
```
+------+-------------------+-----------+---------+--------+--------+---------+--------+-----------+-------+
| Rank | Model             | Provider  | Reason  | Coding | Verif  | TPS     | Latency| Composite | Grade |
+------+-------------------+-----------+---------+--------+--------+---------+--------+-----------+-------+
| #1   | mock-reasoner     | TestMock  |  99.5%  |  94.0% |  97.0% |  95.2   |   42ms |   97.6%   |  S+   |
| #2   | mock-coder        | TestMock  |  88.0%  |  99.0% |  91.0% | 124.5   |   28ms |   94.3%   |  S    |
| #3   | mock-verifier     | TestMock  |  92.0%  |  90.0% |  99.5% |  88.7   |   36ms |   93.8%   |  S    |
+------+-------------------+-----------+---------+--------+--------+---------+--------+-----------+-------+
```

### 5. CLI Benchmark Output & Artifact Persistence
* **Terminal Leaderboard:** Renders live multi-column leaderboard tables directly to stdout with category champions, latency metrics, and throughput.
* **Persistent Telemetry:** Automatically persists full multi-model benchmark evaluation to `.arena/benchmark_results.json`.
* **REPL Integration:** Run `/benchmark all` or `/benchmark <models...>` from within the interactive shell.

---

## 18. Defense-in-Depth Security Sandbox & Threat Mitigation (`SecuritySandbox`)

ArenaFight incorporates a multi-layer security architecture designed to defend autonomous execution against malicious prompt injections, remote command execution attacks, directory traversal escapes, and credential exfiltration:

### 1. Workspace Path Confinement & Jailbreak Protection
* **Strict Path Confinement:** All file reads, writes, edits, searches, and directory creation operations are cryptographically and lexically confined to the designated workspace root (`workspace_dir`).
* **Path Traversal Defense:** Prohibits directory traversal attempts (`../../../../etc/passwd`, `C:\Windows\System32`), drive-letter escapes, UNC network jumps (`\\evil.com\share`), and Windows Alternate Data Streams (`file.txt::$DATA`).
* **Protected File Guards:** Blocks direct modifications or deletions of sensitive credential files (`.env`, `id_rsa`, `id_ed25519`, `credentials.json`, `*.pem`, `*.key`). Any attempt triggers an immediate `CRITICAL` audit alert with access denied.

### 2. Multi-Tier Command Risk Engine & Threat Prevention
Every shell command or executable invocation is parsed against an intelligent multi-tier risk classifier:
* **FORBIDDEN (Hard Blocked):**
  * *Reverse Shells:* `nc -e`, `ncat -e`, `/dev/tcp/`, `/dev/udp/`, `mkfifo /tmp/`, `socat exec:`.
  * *Remote Code Execution:* `curl | bash`, `wget | sh`, `Invoke-WebRequest ... | iex`, encoded PowerShell payloads (`-enc`).
  * *Exploitation Utilities:* `certutil -urlcache`, `bitsadmin /transfer`, `mshta http`, `regsvr32 /s /u /i:`.
  * *System Destruction:* `rm -rf /`, `rm -rf *`, `rmdir /s /q c:\`, `del /f /s /q c:\`, `format`, `mkfs`, `dd if=/dev/`.
  * *Registry / Privileges:* `reg delete HKLM`, `net user /add`, `useradd`, `chmod -R 777 /`.
  * *Fork Bombs:* `:(){ :|:& };:`, `0<${0}`.
* **RESTRICTED:** Requires explicit user authorization confirmation (`git reset --hard`, `git clean -fd`, `del /f /s /q`).
* **SAFE / AUDITED:** Read-only inspections, builds (`cmake`, `ninja`, `g++`, `clang`, `cargo`), test suites (`ctest`, `pytest`).

### 3. Data Loss Prevention (DLP) & Secret Redactor
Automatic, real-time redaction engine masks sensitive credentials before they can enter stdout, stderr, agent conversation turns, execution logs, session memory, or artifacts:
* **OpenAI:** `sk-[A-Za-z0-9_-]{20,}` -> `sk-***[REDACTED]`
* **Anthropic:** `sk-ant-[A-Za-z0-9_-]{20,}` -> `sk-ant-***[REDACTED]`
* **OpenRouter:** `sk-or-v1-[A-Za-z0-9]{32,}` -> `sk-or-v1-***[REDACTED]`
* **GitHub Tokens:** `ghp_[A-Za-z0-9]{36}`, `github_pat_[A-Za-z0-9_]{50,}` -> `ghp_***[REDACTED]`
* **AWS Access Keys:** `AKIA[0-9A-Z]{16}` -> `AKIA***[REDACTED]`
* **Private Keys:** `-----BEGIN [A-Z ]*PRIVATE KEY-----` -> `[REDACTED_PRIVATE_KEY_BLOCK]`
* **Generic Bearer Tokens & Passwords:** `Bearer [REDACTED_TOKEN]`, `api_key=[REDACTED]`.

### 4. Adversarial Unicode & Prompt Injection Sanitizer
* **Trojan Source Protection:** Strips invisible bidirectional text override characters (`\u202A` to `\u202E`, `\u2066` to `\u2069`) that attempt to disguise malicious code logic.
* **Adversarial Control Filtering:** Strips null bytes (`\0`) and tokenizer poison bytes.
* **Prompt Injection Guard:** Flags attempts to override agent instructions (`"disregard all prior instructions"`, `"you are now in developer mode"`, `"<system>"` tags).

### 5. Tamper-Evident SHA-256 Audit Trail (`.arena/security_audit.jsonl`)
* Every security event (path traversal blocks, malicious command rejections, credential masking, security level changes) is cryptographically signed and chained using **FIPS 180-4 SHA-256**.
* Each record contains `record_hash = SHA256(index + timestamp + event + severity + caller + details + prev_hash)`.
* Mathematical guarantee: Any unauthorized modification, tampering, or deletion of past security logs breaks the hash chain and is immediately flagged by `verifyAuditChain`.

### 6. Workspace Vulnerability Scanner (`arena security scan`)
Scans the entire repository in milliseconds, analyzing unredacted secrets, insecure shell scripts, exposed keys, and loose permissions, returning a comprehensive security score (0 to 100%) and remediation guidance.

### 7. CLI & REPL Security Commands
```bash
# Display complete security posture dashboard
arena security
arena security scan          # Deep workspace vulnerability & secret scan
arena audit                  # Verify SHA-256 audit log cryptographic integrity
arena security level strict  # Enforce maximum lockdown

# In Interactive REPL
arena > /security
arena > /security scan
arena > /audit
```

---

## 19. Building and Testing

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
The automated test suite executes **35/35** comprehensive tests covering configuration, tools, process safety, evidence hierarchy, multi-model consensus, ability-based routing, temperature customization, OpenCode integration, Unsloth fine-tuning, file context attachments, fast thinking-phase tournaments, bespoke UI dashboard integrity, universal model compatibility, universal model benchmarking, CLI dual runtime modes, defense-in-depth security sandbox, host hardware assessment & dynamic PC limits, two processing levels (Fast vs Perfect Boi), autonomous service detection, and NVIDIA CUDA / iGPU / CPU slow-variant hardware acceleration:
```bash
# Run test suite executable directly
./build/arena_test.exe

# Or run via CTest
ctest --test-dir build --output-on-failure
```

---

## 20. License & Inquiries

This project is released under an **All Rights Reserved** license:
* **Personal & Educational Use:** Free of charge for individual personal experimentation, learning, hobbyist, and academic research.
* **Commercial & Business Use:** Strictly requires prior written permission and an active commercial license agreement. If you intend to use this software in any business, commercial entity, revenue-generating service, or commercial product, please reach out via email.

For commercial licensing and partnership inquiries, contact:
* **Email:** [amal.s.kumar2005@gmail.com](mailto:amal.s.kumar2005@gmail.com)
* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)

See the full [LICENSE](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/LICENSE) file for complete terms.

---

## 21. Two Processing Levels, PC Hardware Profiler & Autonomous Services

ArenaFight provides two distinct execution levels designed to balance swift turnaround with absolute perfection:

### 1. Two Processing Levels
* **Fast Mode (`--fast` / `arena level fast` / `/level fast`):**
  Standard execution loops (10 iterations, 4096 tokens, single-pass validation). Prioritizes immediate output and rapid turnaround.
* **Perfect Boi Mode (`--perfect` / `arena level perfect` / `/level perfect_boi`):**
  Deep perfectionist mode. Takes time to guarantee 100% correctness:
  * **Exhaustive 6-Gate Physical Verification:** Non-negotiable proof across all gates.
  * **Multi-Pass Adversarial Review:** Runs adversarial cross-examination across all modified code and documents.
  * **Self-Healing Remediation Loops:** If any edge-case, unhandled exception, syntax flaw, or security issue is flagged, automatically spawns self-healing remediation tasks and replans until genuine perfection is achieved.
  * **Adaptive Hardware Scaling:** Automatically scales iterations (up to 25-30), tokens (up to 16,384), and thinking tokens according to the host machine's capacity.

### 2. Host PC Hardware Assessment & Dynamic Limits (No Hardcoding)
Implemented in [`include/arenafight/system/system_profiler.hpp`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/include/arenafight/system/system_profiler.hpp) and [`src/system/system_profiler.cpp`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/src/system/system_profiler.cpp):
* **CPU:** Logical threads and physical cores (`GetNativeSystemInfo` on Windows, `sysconf(_SC_NPROCESSORS_ONLN)` / `/proc/cpuinfo` on Linux).
* **RAM:** Total & Available physical RAM (`GlobalMemoryStatusEx` on Windows, `/proc/meminfo` and `sysinfo` on Linux).
* **GPU & VRAM:** Cross-platform query via `nvidia-smi` (model name, total VRAM, free VRAM, driver version, CUDA runtime version).
* **Display Adapters & iGPU Detection:** Win32 `EnumDisplayDevicesW` and Linux `/sys/class/drm/` detect integrated graphics (AMD Radeon Graphics, Intel Iris Xe/UHD) alongside discrete GPUs without requiring administrative permissions.
* **Real-time Limits:** `max_iterations`, `max_tokens`, `deep_thinking_budget`, and `parallel_workers` are dynamically computed based on physical hardware headroom. Inspect at any time via `arena profile` or `/profile`.

### 3. NVIDIA CUDA Acceleration & Slow-Variant Support (iGPU / CPU)
ArenaFight automatically senses acceleration capabilities and adapts its entire runtime profile:
* **CUDA Hardware Acceleration:** When an NVIDIA GPU with CUDA driver support is detected, ArenaFight leverages maximum acceleration, automatically passing `"num_gpu": 99` to Ollama model offload options to keep all model layers in GPU VRAM for high-throughput inference (40-100+ tokens/sec).
* **Integrated GPU (iGPU) & CPU-Only Slow Variants:** When running on machines with only integrated graphics (AMD Radeon Graphics, Intel Iris Xe) or pure CPU execution:
  * **Prominent Performance Banner:** Displays an unmissable terminal warning box notifying the user that execution will proceed in slow mode (~2-12 tokens/sec vs 40-100+ tokens/sec on CUDA).
  * **Dynamic Resource Throttling:** Automatically caps token ceilings (1024-2048 tokens), limits parallel worker threads to `std::clamp(cores / 4, 1, 3)`, extends operation timeouts to 180 seconds, and passes `"num_gpu": 0` to Ollama to prevent host lockup or out-of-memory kernel panics.
* **Manual Hardware Overrides:** Force hardware modes via CLI flags (`--cuda`, `--cpu`, `--no-cuda`) or REPL commands (`/profile cpu`, `/profile cuda`).

### 4. Autonomous Service Detection & Background Daemon Starter
Implemented in [`include/arenafight/services/service_daemon.hpp`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/include/arenafight/services/service_daemon.hpp) and [`src/services/service_daemon.cpp`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/src/services/service_daemon.cpp):
* **No User Prompts:** Instead of asking the user to manually run `ollama serve`, ArenaFight automatically probes `/api/tags`.
* **Path Discovery (Windows & Linux):** Checks system PATH (`where.exe` on Windows, `which` on Linux) and standard installation paths (`%LOCALAPPDATA%\Programs\Ollama\`, `/usr/local/bin/ollama`, `~/.local/bin/ollama`).
* **Autonomous Daemon Launch:** Spawns `ollama serve` in the background (detached process with hidden window on Windows; systemd service or daemonized fork on Linux), waiting until healthy.
* **Autonomous Model Pulling:** Can pull missing models via HTTP `/api/pull` or CLI without user prompts (`/services pull <model>`).

### 5. CLI & REPL Commands
```bash
# Set processing level
arena level fast               # Fast turnaround mode
arena level perfect            # Perfect Boi exhaustive mode
arena --perfect "Write compiler"
arena --fast "Quick lint"

# Hardware modes and slow-variant overrides
arena --cuda run "Complex task" # Force CUDA hardware acceleration
arena --cpu run "Light task"    # Force CPU slow-variant execution
arena --cpu profile             # View dynamic limits under CPU constraint

# Inspect hardware assessment and dynamic limits
arena profile

# Manage autonomous daemons
arena services                 # Check status and start Ollama in background
arena services pull <model>    # Autonomously pull model
```
In the REPL:
```text
/level [fast|perfect]          Toggle active processing level
/profile [cuda|cpu]            Display hardware assessment dashboard
/services                      Autonomous daemon status and control
```

---

## 22. Release Notes & Version History

### Version 2.5.0 (Current Release - Peak CLI Edition)
* **Pure Developer CLI Evolution:**
  * Purged all HTML web UI files (`index.html`, `simple_ui.html`, `metrics.html`) and `--ui` flags.
  * Added native terminal telemetry observatory (`arena metrics` / `arena telemetry` / `/metrics`) displaying the 6 Physical Evidence Gates, thinking duel token savings, and hardware allocations.
  * Added `arena.cmd` and `arena.ps1` wrappers for execution directly from the workspace root in Windows PowerShell and Command Prompt.
  * Installed executable to user PATH (`%USERPROFILE%\.local\bin\arena.exe`).
* **Modern C++20 Standard Refactoring:**
  * Implemented `arenafight::string_utils` (`include/arenafight/common/string_utils.hpp`) featuring zero-allocation `std::string_view` implementations of `trim`, `split`, `join`, `toLower`, `toUpper`, `startsWith`, `endsWith`, `contains`, and `replaceAll`.
  * Converted enum-to-string and string-to-enum conversions in `types.hpp` and `evidence_types.hpp` to `constexpr std::string_view`.
  * Resolved all compiler warnings across the entire codebase under `-Wall -Wextra -Wpedantic`.
* **Two Processing Levels:**
  * Fast Mode (`--fast` / `arena level fast`): Swift execution loop, 10 iterations, 4096 tokens, fast turnaround.
  * Perfect Boi Mode (`--perfect` / `arena level perfect`): Deep verification, dynamic limits up to 25-30 iterations, 16K tokens, multi-pass adversarial code reviews, and self-healing remediation loops.
* **Autonomous Hardware Profiler & PC Limits (No Hardcoding):**
  * Real-time host profiling of CPU threads, physical cores, RAM total and available, GPU model, and VRAM without elevated administrator privileges.
  * Dynamically calculates iterations, tokens, reasoning budgets, and worker threads based on host headroom.
* **NVIDIA CUDA Hardware Acceleration & Slow-Variant Protection:**
  * Autonomously detects NVIDIA CUDA GPU support, configuring Ollama layer offloading (`num_gpu: 99`).
  * Autonomously detects integrated GPUs (AMD Radeon Graphics, Intel Iris Xe) and CPU-only systems. Displays prominent terminal warning boxes and applies resource throttling (`num_gpu: 0`, capped threads, extended timeouts) to prevent host lockups.
* **Autonomous Background Service Daemon:**
  * Automatically detects, discovers PATHs, and launches `ollama serve` in the background (detached process on Windows and Linux) without prompting the user.
  * Autonomously pulls missing models via HTTP `/api/pull` or `/services pull <model>`.
* **Defense-in-Depth Security Sandbox:**
  * Lexical workspace path confinement (Path Jail) preventing directory traversal escapes (`../`).
  * Destructive command gatekeeper with interactive user confirmations.
  * Data Loss Prevention (DLP) secret redactor masking API keys, JWTs, and private tokens.
  * Cryptographic SHA-256 audit trail (`.arena/security_audit.jsonl`).
* **Dual Runtime Engine:**
  * Standalone native C++ runtime (default) vs Connected OpenCode CLI bridge (`arena connect opencode`).
* **Universal Benchmarking Engine:**
  * Automated 4-axis benchmark suite across Reasoning, Coding, Verification, and Throughput (`arena benchmark all`).
* **OpenCode Bi-Directional MCP Server & Slash Commands:**
  * Native Model Context Protocol (MCP) server (rena mcp / rena --mcp) running over stdio JSON-RPC 2.0 with 9 integrated tools (rena_verify, rena_goal, rena_plan, rena_profile, rena_benchmark, rena_qos, rena_calculate, rena_security, rena_think).
  * Automated workspace export (rena opencode sync) creating root opencode.json, .opencode/opencode.json, .opencode/INSTRUCTIONS.md, and OpenCode custom slash commands in .opencode/commands/ (/arena, /verify, /profile, /benchmark, /qos).
  * Dedicated standalone rena verify command to execute Level 1 physical evidence gates directly from terminal or OpenCode bash tool.
* **Automated Verification:**
  * 36/36 passing unit tests covering all subsystems including native MCP server and OpenCode workspace integration.

### Version 2.0.0
* Dual-mode runtime architecture (Standalone native vs Connected OpenCode CLI bridge).
* Independent verifier with 6 physical evidence gates (Level 1 Dominance Rule).
* Fast thinking-phase tournaments with early termination.
* Unsloth AI fine-tuning integration with QLoRA.
* Hardware & Embedded Engineering Studio with pinout constraint validation and Wokwi simulation.

---

## Credits
**Made by Lunarmist-byte**
* **GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)
* **LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)
