---
name: opencode-orchestrator
description: Bi-directional orchestration between OpenCode and ArenaFight, OpenCode CLI provider routing, JSON-RPC stdio MCP tools, and workspace instruction synchronization.
---

# OpenCode Orchestrator Skill

## Overview
This skill governs seamless collaboration between the OpenCode AI assistant ecosystem and the ArenaFight autonomous multi-model runtime.

## Dual Operational Modes
1. **OpenCode Subservient (ArenaFight as MCP Engine for OpenCode)**:
   - ArenaFight runs as an MCP stdio server registered in `.opencode/opencode.json`.
   - OpenCode exposes ArenaFight tools:
     - `arena_verify`: Triggers 6 Physical Evidence verification gates.
     - `arena_goal`: Executes end-to-end autonomous goal pursuit with self-healing.
     - `arena_plan`: Generates structured task execution graphs and physical validation plans.
     - `arena_profile`: Audits host CPU, RAM, and NVIDIA CUDA GPU limits.
     - `arena_benchmark`: Benchmarks models on reasoning, coding, and tokens/sec.
     - `arena_qos`: Embedded firmware inspection for zero-heap ISR and watchdog safety.
     - `arena_generate_image`: Neural image generation with Fooocus SDXL.
     - `arena_generate_video`: Motion video synthesis with Wan AI 2.1.
     - `arena_media_inspect`: Physical media container and magic byte inspection.

2. **OpenCode Autonomous (ArenaFight Controlling OpenCode CLI)**:
   - ArenaFight invokes `opencode run` or uses OpenCode CLI as an AI provider when configured.
   - Parses OpenCode outputs and subjects all suggested code to physical compiler and test verification.

## Slash Commands Automation
Operators can invoke ArenaFight capabilities from chat or OpenCode via shortcuts:
- `/verify` : Run 6 physical evidence verification gates on current workspace.
- `/goal <task>` : Autonomous goal pursuit until all verification gates pass.
- `/plan <task>` : Architectural plan generation with dependencies.
- `/qos <path>` : Embedded firmware QoS validation.
- `/security` : Workspace security scan and audit chain verification.
- `/profile` : Host machine hardware profile and dynamic limits.

