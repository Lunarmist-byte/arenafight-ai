---
name: verifier
description: Independent adversarial code review, compiler test verification, 6 physical evidence gates, and hallucination prevention.
---

# Verifier Skill

## Overview
This skill operationalizes ArenaFight's flagship doctrine: **Level 1 Physical Dominance Rule (Model Output != Truth)**. No model assertion or consensus is treated as factual reality without independent physical proof.

## 6 Physical Evidence Verification Gates
To mark any task, pull request, or milestone complete, all 6 gates must evaluate to PASS:
1. **Gate 1 - Physical Artifact Presence**:
   - Inspect the file system directly (`std::filesystem::exists`) to confirm that all declared files, headers, and media assets physically exist on disk with nonzero byte size.
2. **Gate 2 - Clean Compilation Proof**:
   - Execute the compiler (`ninja`, `cmake --build`, `gcc`, `clang`) in the physical shell environment.
   - Zero compilation errors. Compiler exit code 0 is mandatory.
3. **Gate 3 - Automated Test Execution**:
   - Execute the automated test binary (`./build/arena_test.exe`).
   - Every unit, integration, and stress test assertion must evaluate to true.
4. **Gate 4 - Requirement Satisfaction**:
   - Traverse the execution graph to verify that every planned subtask, input constraint, and deliverable is accounted for.
5. **Gate 5 - Security Sandbox & DLP Integrity**:
   - Scan workspace for secret leaks (API tokens, private keys), directory traversal escapes, and forbidden commands.
6. **Gate 6 - Cryptographic SHA-256 Audit Chain**:
   - Verify that the cryptographic hash chain in `.arena/security_audit.jsonl` is intact and uncorrupted.

## Evidence Hierarchy
- **Level 1 (Direct Physical Execution)**: Shell exit codes, test outputs, compiler diagnostics, magic byte checks.
- **Level 2 (Deterministic Rule Validation)**: Static analysis, linting rules, regex constraint matching.
- **Level 3 (Independent Blind Review)**: Separate verifier model analyzing diff without knowing author model.
- **Level 4 (Model Consensus)**: Agreement across multiple LLMs (supporting evidence only; strictly subordinate to Level 1).
- **Level 5 (Unverified Model Assertion)**: A model claiming "the code works" without running tests (considered unproven hypothesis).
