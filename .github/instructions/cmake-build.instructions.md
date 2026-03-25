---
description: "Use when building, compiling, rebuilding, or configuring this CMake workspace. Always use CMake Tools for CMake builds in this repo, and run CMake Tools Configure first if the project is not configured or build commands are unavailable."
---
# CMake Build Workflow

- Always use CMake Tools for CMake build requests in this workspace.
- If a build cannot start because the project is not configured or the build command is unavailable, run CMake Tools Configure first.
- Prefer CMake Tools target discovery and CMake Tools build commands over terminal `cmake --build` commands.
- Only fall back to terminal-based CMake commands if the user explicitly asks for that or CMake Tools cannot be used.