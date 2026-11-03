# update_readme.py
import re

with open('readme.md', 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update OpenCode section
old_opencode = """### Capabilities
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
```"""

new_opencode = """### Capabilities
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
```"""

# Standardize line endings for matching
text_lf = text.replace('\r\n', '\n')
if old_opencode in text_lf:
    text_lf = text_lf.replace(old_opencode, new_opencode)
    print("OpenCode section updated successfully!")
else:
    print("Could not find old OpenCode section")

# 2. Add arena mcp and arena verify to Section 10 CLI Reference
old_cli_ref = """  arena "<task>"                  Execute a task autonomously (/goal)
  arena goal "<task>"             Autonomous goal pursuit with verification"""

new_cli_ref = """  arena "<task>"                  Execute a task autonomously (/goal)
  arena goal "<task>"             Autonomous goal pursuit with verification
  arena verify                    Run Level 1 physical evidence verification gates
  arena mcp                       Run Model Context Protocol (MCP) server over stdio"""

if old_cli_ref in text_lf:
    text_lf = text_lf.replace(old_cli_ref, new_cli_ref)
    print("CLI reference updated with verify and mcp!")

# 3. Update Release Notes
old_rel_notes = "* **Automated Verification:\n  * 35/35 passing unit tests covering all subsystems."
new_rel_notes = """* **OpenCode Bi-Directional MCP Server & Slash Commands:**
  * Native Model Context Protocol (MCP) server (`arena mcp` / `arena --mcp`) running over stdio JSON-RPC 2.0 with 9 integrated tools (`arena_verify`, `arena_goal`, `arena_plan`, `arena_profile`, `arena_benchmark`, `arena_qos`, `arena_calculate`, `arena_security`, `arena_think`).
  * Automated workspace export (`arena opencode sync`) creating root `opencode.json`, `.opencode/opencode.json`, `.opencode/INSTRUCTIONS.md`, and OpenCode custom slash commands in `.opencode/commands/` (`/arena`, `/verify`, `/profile`, `/benchmark`, `/qos`).
  * Dedicated standalone `arena verify` command to execute Level 1 physical evidence gates directly from terminal or OpenCode bash tool.
* **Automated Verification:**
  * 36/36 passing unit tests covering all subsystems including native MCP server and OpenCode workspace integration."""

if old_rel_notes in text_lf:
    text_lf = text_lf.replace(old_rel_notes, new_rel_notes)
    print("Release notes updated with MCP and 36/36 tests!")
else:
    print("Could not find old release notes text")

# Write back preserving CRLF
with open('readme.md', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(text_lf)

print("readme.md write complete!")

