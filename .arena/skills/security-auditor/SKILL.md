---
name: security-auditor
description: Zero-trust workspace security auditing, path jail containment, DLP secret redaction, command risk level analysis, and tamper-evident SHA-256 audit chaining.
---

# Security Auditor Skill

## Overview
This skill operationalizes ArenaFight's defense-in-depth security model to prevent code execution escapements, credential exfiltration, prompt injections, and path traversal attacks.

## Security Posture and Risk Classification
Commands and file operations are evaluated under four risk tiers:
- **P0 - SAFE**: Read-only workspace queries, non-destructive status checks (`git status`, `read_file`, `list_directory`).
- **P1 - AUDITED**: Workspace file writes, non-destructive builds, clean compilations (`write_file`, `edit_file`, `cmake`, `ninja`). Requires automatic logging to `.arena/security_audit.jsonl`.
- **P2 - RESTRICTED**: Potentially destructive operations (`delete_file`, `git reset`, `rm -rf`, process kills). Requires explicit operator confirmation or confirmation bypass flags.
- **P3 - FORBIDDEN**: Malicious vectors, root filesystem writes, reverse shells, memory dumpers, registry destruction, or arbitrary binary downloads (`curl | sh`, `rm -rf /`). Blocked unconditionally.

## Path Jail Enforcement
1. All file operations (`read_file`, `write_file`, `edit_file`, `delete_file`, `media_generate_image`, `media_generate_video`) must pass through `SecuritySandbox::resolveSafePath()`.
2. Paths containing `..` or leading to parent directories outside the workspace boundary are rejected immediately.
3. Protected system and secret files (`.env`, `id_rsa`, `id_ed25519`, `credentials.json`, Windows `SAM`/`SYSTEM`) are shielded from inspection.

## Data Loss Prevention (DLP)
1. Active scanner monitors outputs for unmasked private API keys (`ghp_`, `sk-`, `AKIA`, `Bearer`).
2. Matched patterns are masked to prevent token leakage in session logs and MCP payloads.

## Cryptographic Audit Trail
1. Every state mutation is logged with monotonic indexing, timestamp, caller ID, event name, previous record SHA-256 hash, and current record SHA-256 hash.
2. The hash chain ensures tamper-evident traceability conforming to FIPS 180-4 SHA-256 integrity standards.

