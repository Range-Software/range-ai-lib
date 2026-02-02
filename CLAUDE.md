# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

range-ai-lib is a Qt6-based C++ static library providing AI LLM agent integration for the Range Software ecosystem. It depends on range-base-lib and is consumed by other Range projects (range-family, range-fea, etc.).

## Build Commands

This library is built as part of a larger CMake project. From the parent project root:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/qt6
cmake --build build
```

The library produces `librange-ai-lib.a` (static library).

## Architecture

**Current structure:**
- `include/rai_claude_agent.h` - RClaudeAgent class header (QObject-based)
- `src/rai_claude_agent.cpp` - RClaudeAgent implementation

**Dependencies:**
- Qt6::Core, Qt6::Network
- range-base-lib (Range Software utilities)
- common_defines (CMake target for project-wide definitions)

## Code Conventions

- Class prefix: `R` for Range classes (e.g., `RLlmAgent`)
- File prefix: `rai_` for range-ai-lib files
- Use Qt6 signals/slots for async operations
- C++17 standard
- Header guards: `RAI_CLASSNAME_H` pattern
