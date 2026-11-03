# OpenCode Instructions: ArenaFight Autonomous Engine Integration

## Made by Lunarmist-byte
* GitHub: https://github.com/Lunarmist-byte
* LinkedIn: https://www.linkedin.com/in/amal-s-kumar-ba69a1290/

## Current Objective
Autonomous software / hardware engineering with physical verification

## How to Call ArenaFight in OpenCode

You have the **ArenaFight** CLI (`arena`) and MCP tools connected to this OpenCode environment.

### 1. Model Context Protocol (MCP) Tools
OpenCode has registered the `arena` MCP server (`arena mcp`). You can invoke these MCP tools directly:
- `arena_verify`: Validate code mutations against all 6 physical evidence gates (compiler diagnostics, test runner exit codes, file diffs, QoS, security).
- `arena_goal`: Delegate an autonomous engineering task to ArenaFight's self-healing execution loop.
- `arena_plan`: Formulate a structured task graph with verification milestones.
- `arena_profile`: Assess host hardware (CPU cores, RAM headroom, NVIDIA CUDA GPU) and adaptive limits.
- `arena_benchmark`: Benchmark available models across reasoning, coding invariants, and throughput.
- `arena_qos`: Evaluate embedded firmware for zero-heap ISR compliance, non-blocking loops, and watchdog timers.
- `arena_calculate`: Calculate circuit component values (LED current limiters, voltage dividers, I2C pullups, battery lifetime).
- `arena_security`: Run workspace vulnerability scanner, DLP secret detector, and SHA-256 audit chain check.
- `arena_think`: Run fast thinking tournaments comparing pure reasoning without code generation.

### 2. Custom OpenCode Slash Commands
Users can type these slash commands directly in OpenCode:
- `/arena <task or subcommands>` -> Calls ArenaFight CLI
- `/verify` -> Runs all 6 physical evidence verification gates (`arena verify`)
- `/profile` -> Displays host hardware profiling and dynamic limits (`arena profile`)
- `/benchmark` -> Evaluates model capabilities (`arena benchmark`)
- `/qos [path]` -> Evaluates embedded firmware QoS (`arena qos`)

### 3. Shell / Terminal Execution
Whenever you make changes to files, always verify them using the `arena` CLI:
```bash
arena verify
```

### Operating Doctrine: Level 1 Physical Dominance Rule
1. MODEL OUTPUT != TRUTH. Never declare a coding task complete based purely on self-reporting.
2. Physical execution results (exit code 0 from compiler and test suites) dominate any model assertion.
3. Always run `arena_verify` or `arena verify` before concluding any feature or bug fix.

## Additional Constraints
Autonomous ArenaFight execution loop with physical verification.
