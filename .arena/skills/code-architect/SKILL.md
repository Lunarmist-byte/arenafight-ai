---
name: code-architect
description: System architecture, modular design, clean APIs, dependency inversion, and RAII memory safety in C++20 and Python.
---

# Code Architect Skill

## Overview
This skill guides the design, refactoring, and structuring of complex software systems across modern C++20 and Python with strict memory safety and clean interface boundaries.

## Architecture Principles
1. **Separation of Concerns and Modular Boundaries**:
   - Strictly isolate business logic, domain algorithms, network transport, UI rendering, and hardware access.
   - Employ abstract interfaces (`Tool`, `Provider`, `HardwareManager`) to permit seamless substitution and mock testing without breaking callers.
2. **Modern C++20 Memory Safety & RAII**:
   - Zero raw owning pointers. Manage resource lifecycles through `std::unique_ptr`, `std::shared_ptr`, and RAII wrappers.
   - Use standard library containers (`std::vector`, `std::string_view`, `std::optional`) and structured bindings.
   - Prevent memory leaks, buffer overruns, and use-after-free conditions.
3. **Thread Safety and Concurrency**:
   - Synchronize shared mutable states using `std::mutex` and `std::lock_guard`.
   - Prevent deadlock hazards by enforcing strict lock acquisition hierarchies.
4. **Deterministic Testing and Mocking**:
   - Design components to run offline deterministically without requiring live cloud API endpoints or external network connections during automated test runs.
